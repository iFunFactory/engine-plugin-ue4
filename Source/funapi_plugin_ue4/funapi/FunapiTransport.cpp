// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin_ue4.h"

#include "FunapiUtils.h"
#include "FunapiTransport.h"
#include "FunapiNetwork.h"


namespace fun {

namespace {

////////////////////////////////////////////////////////////////////////////////
// Constants.

// Buffer-related constants.
const int kUnitBufferSize = 65536;
const int kUnitBufferPaddingSize = 1;

// Funapi header-related constants.
const char kHeaderDelimeter[] = "\n";
const char kHeaderFieldDelimeter[] = ":";
const char kVersionHeaderField[] = "VER";
const char kLengthHeaderField[] = "LEN";
const int kCurrentFunapiProtocolVersion = 1;

}  // unnamed namespace



////////////////////////////////////////////////////////////////////////////////
// FunapiTransportBase implementation.

class FunapiTransportBase : public std::enable_shared_from_this<FunapiTransportBase> {
 public:
  typedef FunapiTransport::OnReceived OnReceived;
  typedef FunapiTransport::OnStopped OnStopped;

  FunapiTransportBase(FunapiTransportType type);
  virtual ~FunapiTransportBase();

  void RegisterEventHandlers(const OnReceived &cb1, const OnStopped &cb2);

  void SendMessage(Json &message);
  void SendMessage(FJsonObject &message);
  void SendMessage(FunMessage &message);

  void SetNetwork(FunapiNetwork* network);

 protected:
  void SendMessage(const char *body);

 private:
  virtual void EncodeThenSendMessage() = 0;

  bool TryToDecodeHeader();
  bool TryToDecodeBody();

 protected:
  typedef std::map<std::string, std::string> HeaderFields;

  void Init();
  bool EncodeMessage();
  bool DecodeMessage(int nRead);

  // Registered event handlers.
  OnReceived on_received_;
  OnStopped on_stopped_;

  // State-related.
  std::mutex mutex_;
  FunapiTransportType type_;
  FunapiTransportState state_;
  IoVecList sending_;
  IoVecList pending_;
  std::vector<uint8_t> receiving_;
  int receiving_len_;
  bool header_decoded_;
  int received_size_;
  int next_decoding_offset_;
  HeaderFields header_fields_;

  FunapiNetwork *network_ = nullptr;
};


FunapiTransportBase::FunapiTransportBase(FunapiTransportType type)
    : type_(type), state_(kDisconnected), header_decoded_(false),
      received_size_(0), next_decoding_offset_(0) {
  receiving_len_ = kUnitBufferSize;
  receiving_.resize(kUnitBufferSize + kUnitBufferPaddingSize);
  assert(receiving_.iov_base);
}


FunapiTransportBase::~FunapiTransportBase() {
  header_fields_.clear();
  sending_.clear();
  pending_.clear();
}


void FunapiTransportBase::RegisterEventHandlers(
    const OnReceived &on_received, const OnStopped &on_stopped) {
  std::unique_lock<std::mutex> lock(mutex_);
  on_received_ = on_received;
  on_stopped_ = on_stopped;
}


void FunapiTransportBase::Init() {
  // Resets states.
  header_decoded_ = false;
  received_size_ = 0;
  next_decoding_offset_ = 0;
  header_fields_.clear();
  sending_.clear();
}


bool FunapiTransportBase::EncodeMessage() {
  assert(!sending_.empty());

  for (IoVecList::iterator itr = sending_.begin(), itr_end = sending_.end();
      itr != itr_end;
      ++itr) {
    auto &body_as_bytes = *itr;

    char header[1024];
    size_t offset = 0;
    if (type_ == kTcp || type_ == kUdp) {
      offset += snprintf(header + offset, sizeof(header) - offset, "%s%s%d%s",
                         kVersionHeaderField, kHeaderFieldDelimeter,
                         kCurrentFunapiProtocolVersion, kHeaderDelimeter);
      offset += snprintf(header + offset, sizeof(header)- offset, "%s%s%lu%s",
                         kLengthHeaderField, kHeaderFieldDelimeter,
                         (unsigned long)body_as_bytes.size(), kHeaderDelimeter);
      offset += snprintf(header + offset, sizeof(header)- offset, "%s",
                          kHeaderDelimeter);
    }

    std::string b(body_as_bytes.begin(), body_as_bytes.end());
    header[offset] = '\0';
    LOG1("Header to send: %s", *FString(header));
    LOG1("send message: %s", *FString(b.c_str()));

    std::vector<uint8_t> header_as_bytes(offset);
    memcpy(header_as_bytes.data(),
      header,
      offset);

    if (type_ == kTcp || type_ == kHttp)
    {
      sending_.insert(itr, header_as_bytes);
    }
    else if (type_ == kUdp) {
      body_as_bytes.insert(body_as_bytes.begin(), header_as_bytes.begin(), header_as_bytes.end());
    }
  }

  return true;
}


bool FunapiTransportBase::DecodeMessage(int nRead) {
  if (nRead <= 0) {
    if (nRead < 0) {
      LOG1("receive failed: %s", *FString(strerror(errno)));
    } else {
      if (received_size_ > next_decoding_offset_) {
        LOG1("Buffer has %d bytes. But they failed to decode. Discarding.", (received_size_ - next_decoding_offset_));
      }
    }
    return false;
  }

  LOG2("Received %d bytes. Buffer has %d bytes.", nRead, (received_size_ - next_decoding_offset_));
  {
    std::unique_lock<std::mutex> lock(mutex_);
    received_size_ += nRead;

    // Tries to decode as many messags as possible.
    while (true) {
      if (header_decoded_ == false) {
        if (TryToDecodeHeader() == false) {
          break;
        }
      }
      if (header_decoded_) {
        if (TryToDecodeBody() == false) {
          break;
        }
      }
    }

    // Checks buvffer space before starting another async receive.
    if (receiving_len_ - received_size_ == 0) {
      // If there are space can be collected, compact it first.
      // Otherwise, increase the receiving buffer size.
      if (next_decoding_offset_ > 0) {
        LOG1("Compacting a receive buffer to save %d bytes.", next_decoding_offset_);
        uint8_t *base = reinterpret_cast<uint8_t *>(receiving_.data());
        memmove(base, base + next_decoding_offset_,
                received_size_ - next_decoding_offset_);
        received_size_ -= next_decoding_offset_;
        next_decoding_offset_ = 0;
      } else {
        size_t new_size = receiving_len_ + kUnitBufferSize;
        LOG1("Resizing a buffer to %d bytes.", new_size);
        receiving_.resize(new_size + kUnitBufferPaddingSize);
        receiving_len_ = new_size;
      }
    }
  }
  return true;
}


void FunapiTransportBase::SendMessage(Json &message) {
  rapidjson::StringBuffer string_buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(string_buffer);
  message.Accept(writer);

  SendMessage(string_buffer.GetString());
}


void FunapiTransportBase::SendMessage(FJsonObject &message) {
  TSharedRef<FJsonObject> json_object = MakeShareable(new FJsonObject(message));

  FString output_string;
  TSharedRef<TJsonWriter<TCHAR>> writer = TJsonWriterFactory<TCHAR>::Create(&output_string);
  FJsonSerializer::Serialize(json_object, writer);

  SendMessage(TCHAR_TO_ANSI(*output_string));
}


void FunapiTransportBase::SendMessage(FunMessage &message) {
  std::string body = message.SerializeAsString();
  SendMessage(body.c_str());
}


void FunapiTransportBase::SendMessage(const char *body) {
  std::vector<uint8_t> body_as_bytes(strlen(body));
  memcpy(body_as_bytes.data(), body, strlen(body));

  bool sendable = false;

  {
    std::unique_lock<std::mutex> lock(mutex_);
    pending_.push_back(body_as_bytes);
    if (state_ == kConnected && sending_.size() == 0) {
      sending_.swap(pending_);
      sendable = true;
    }
  }

  if (sendable)
      EncodeThenSendMessage();
}


// The caller must lock mutex_ before call this function.
bool FunapiTransportBase::TryToDecodeHeader() {
  LOG("Trying to decode header fields.");
  for (; next_decoding_offset_ < received_size_; ) {
    char *base = reinterpret_cast<char *>(receiving_.data());
    char *ptr =
        std::search(base + next_decoding_offset_,
                    base + received_size_,
                    kHeaderDelimeter,
                    kHeaderDelimeter + sizeof(kHeaderDelimeter) - 1);

    ssize_t eol_offset = ptr - base;
    if (eol_offset >= received_size_) {
      // Not enough bytes. Wait for more bytes to come.
      LOG("We need more bytes for a header field. Waiting.");
      return false;
    }

    // Generates a null-termianted string by replacing the delimeter with \0.
    *ptr = '\0';
    char *line = base + next_decoding_offset_;
    LOG1("Header line: %s", *FString(line));

    ssize_t line_length = eol_offset - next_decoding_offset_;
    next_decoding_offset_ = eol_offset + 1;

    if (line_length == 0) {
      // End of header.
      header_decoded_ = true;
      LOG("End of header reached. Will decode body from now.");
      return true;
    }

    ptr = std::search(
        line, line + line_length, kHeaderFieldDelimeter,
        kHeaderFieldDelimeter + sizeof(kHeaderFieldDelimeter) - 1);
    assert((ptr - base) < eol_offset);

    // Generates null-terminated string by replacing the delimeter with \0.
    *ptr = '\0';
    char *e1 = line, *e2 = ptr + 1;
    while (*e2 == ' ' || *e2 == '\t') ++e2;
    LOG2("Decoded header field '%s' => '%s'", *FString(e1), *FString(e2));
    header_fields_[e1] = e2;
  }
  return false;
}


// The caller must lock mutex_ before call this function.
bool FunapiTransportBase::TryToDecodeBody() {
  // version header 읽기
  HeaderFields::const_iterator it = header_fields_.find(kVersionHeaderField);
  assert(it != header_fields_.end());
  int version = atoi(it->second.c_str());
  assert(version == kCurrentFunapiProtocolVersion);

  // length header 읽기
  it = header_fields_.find(kLengthHeaderField);
  int body_length = atoi(it->second.c_str());
  LOG2("We need %d bytes for a message body. Buffer has %d bytes.",
       body_length, (received_size_ - next_decoding_offset_));

  if (received_size_ - next_decoding_offset_ < body_length) {
    // Need more bytes.
    LOG("We need more bytes for a message body. Waiting.");
    return false;
  }

  if (body_length > 0) {
    assert(state_ == kConnected);

    if (state_ != kConnected) {
      LOG("unexpected message");
      return false;
    }

    char *base = reinterpret_cast<char *>(receiving_.data());

    // Generates a null-termianted string for convenience.
    char tmp = base[next_decoding_offset_ + body_length];
    base[next_decoding_offset_ + body_length] = '\0';

    std::string buffer;
    buffer.assign(base + next_decoding_offset_);
    base[next_decoding_offset_ + body_length] = tmp;

    // Moves the read offset.
    next_decoding_offset_ += body_length;

    // The network module eats the fields and invokes registered handler
    // LOG("Invoking a receive handler.");
    const OnReceived received = on_received_;
    const HeaderFields fields = header_fields_;
    network_->PushTaskQueue([received, fields, buffer](){ received(fields, buffer); });
  }

  // Prepares for a next message.
  header_decoded_ = false;
  header_fields_.clear();
  return true;
}

void FunapiTransportBase::SetNetwork(FunapiNetwork* network) {
  network_ = network;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiTransportImpl implementation.

class FunapiTransportImpl : public FunapiTransportBase {
 public:
  FunapiTransportImpl(FunapiTransportType type,
                      const std::string &hostname_or_ip, uint16_t port);
  virtual ~FunapiTransportImpl();

  void Start();
  void Stop();
  bool Started();

 private:
  void StartCb(int rc);
  void SendBytesCb(ssize_t rc);
  void ReceiveBytesCb(ssize_t nRead);

  virtual void EncodeThenSendMessage();

  void AsyncConnect(int sock, const Endpoint &endpoint, const AsyncConnectCallback &callback);
  void AsyncSend(int sock, const IoVecList &sending, AsyncSendCallback callback);
  void AsyncReceive(int sock, const struct iovec &receiving, AsyncReceiveCallback callback);
  void AsyncSendTo(int sock, const Endpoint &ep, const IoVecList &sending, AsyncSendCallback callback);
  void AsyncReceiveFrom(int sock, const Endpoint &ep, const struct iovec &receiving, AsyncReceiveCallback callback);

  // State-related.
  Endpoint endpoint_;
  Endpoint recv_endpoint_;
  int sock_;
};


FunapiTransportImpl::FunapiTransportImpl(FunapiTransportType type,
                                         const std::string &hostname_or_ip,
                                         uint16_t port)
    : FunapiTransportBase(type), sock_(-1) {

  struct hostent *entry = gethostbyname(hostname_or_ip.c_str());
  assert(entry);

  struct in_addr **l = reinterpret_cast<struct in_addr **>(entry->h_addr_list);
  assert(l);
  assert(l[0]);

  if (type == kTcp) {
    endpoint_.sin_family = AF_INET;
    endpoint_.sin_addr = *l[0];
    endpoint_.sin_port = htons(port);
  } else if (type == kUdp) {
    endpoint_.sin_family = AF_INET;
    endpoint_.sin_addr = *l[0];
    endpoint_.sin_port = htons(port);

    recv_endpoint_.sin_family = AF_INET;
    recv_endpoint_.sin_addr.s_addr = htonl(INADDR_ANY);
    recv_endpoint_.sin_port = htons(port);
  }
}


FunapiTransportImpl::~FunapiTransportImpl() {
}


void FunapiTransportImpl::Start() {
  std::unique_lock<std::mutex> lock(mutex_);
  Init();

  // Initiates a new socket.
  if (type_ == kTcp) {
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock_ >= 0);
    state_ = kConnecting;

    // connecting
    AsyncConnect(sock_, endpoint_,
      [this](const int rc) { StartCb(rc); });
  } else if (type_ == kUdp) {
    sock_ = socket(AF_INET, SOCK_DGRAM, 0);
    assert(sock_ >= 0);
    state_ = kConnected;

    // Wait for message
    struct iovec temp_iovec;
    temp_iovec.iov_base = receiving_.data();
    temp_iovec.iov_len = receiving_.size();
    AsyncReceiveFrom(sock_, recv_endpoint_, temp_iovec,
      [this](const ssize_t nRead){ReceiveBytesCb(nRead);});
  }
}


void FunapiTransportImpl::Stop() {
  std::unique_lock<std::mutex> lock(mutex_);
  state_ = kDisconnected;
  if (sock_ >= 0) {
#if PLATFORM_WINDOWS
    closesocket(sock_);
#else
    close(sock_);
#endif
    sock_ = -1;
  }
}


bool FunapiTransportImpl::Started() {
  std::unique_lock<std::mutex> lock(mutex_);
  return (state_ == kConnected);
}


void FunapiTransportImpl::StartCb(int rc) {
  LOG("StartCb called");

  if (rc != 0) {
    LOG1("Connect failed: %s", *FString(strerror(rc)));
    Stop();
    on_stopped_();
    return;
  }

  LOG("Connected.");

  // Makes a state transition.
  state_ = kConnected;

  // Start to handle incoming messages.
  struct iovec temp_iovec;
  temp_iovec.iov_len = receiving_.size();
  temp_iovec.iov_base = receiving_.data();
  AsyncReceive(
      sock_, temp_iovec,
      [this](const ssize_t nRead){ ReceiveBytesCb(nRead); });
  LOG("Ready to receive");

  // Starts to process if there any data already queue.
  if (pending_.size() > 0) {
    LOG("Flushing pending messages.");
    sending_.swap(pending_);
    EncodeThenSendMessage();
  }
}


void FunapiTransportImpl::SendBytesCb(ssize_t nSent) {
  if (nSent < 0) {
    LOG1("send failed: %s", *FString(strerror(errno)));
    Stop();
    on_stopped_();
    return;
  }

  LOG1("Sent %d bytes", nSent);

  bool sendable = false;

  // Now releases memory that we have been holding for transmission.
  {
    std::unique_lock<std::mutex> lock(mutex_);
    assert(!sending_.empty());

    if (!sending_.empty())
    {
      IoVecList::iterator itr = sending_.begin();
      sending_.erase(itr);
    }

    if (pending_.size() > 0 && sending_.empty()) {
      sending_.swap(pending_);
      sendable = true;
    }
  }

  if (sendable)
      EncodeThenSendMessage();
}


void FunapiTransportImpl::ReceiveBytesCb(ssize_t nRead) {
  if (!DecodeMessage(nRead)) {
    if (nRead == 0)
      LOG1("Socket [%d] closed.", sock_);

    Stop();
    on_stopped_();
    return;
  }

  // Starts another async receive.
  {
    std::unique_lock<std::mutex> lock(mutex_);

    struct iovec residual;
    residual.iov_len = receiving_len_ - received_size_;
    residual.iov_base =
        reinterpret_cast<uint8_t *>(receiving_.data()) + received_size_;
    if (type_ == kTcp) {
      AsyncReceive(sock_, residual,
        [this](const ssize_t nRead){ ReceiveBytesCb(nRead); });
    } else if (type_ == kUdp) {
      AsyncReceiveFrom(sock_, recv_endpoint_, residual,
        [this](const ssize_t nRead){ ReceiveBytesCb(nRead); });
    }
    LOG1("Ready to receive more. We can receive upto %ld  more bytes.",
         (receiving_len_ - received_size_));
  }
}


void FunapiTransportImpl::EncodeThenSendMessage() {
  assert(state_ == kConnected);

  if (!EncodeMessage()) {
    Stop();
    return;
  }

  assert(!sending_.empty());
  if (type_ == kTcp) {
    AsyncSend(sock_, sending_,
      [this](const ssize_t nSent) { SendBytesCb(nSent); });
  } else if (type_ == kUdp) {
    AsyncSendTo(sock_, endpoint_, sending_,
      [this](const ssize_t nSent) { SendBytesCb(nSent); });
  }
}


void FunapiTransportImpl::AsyncConnect(int sock,
  const Endpoint &endpoint, const AsyncConnectCallback &callback) {
  // Makes the fd non-blocking.
#if PLATFORM_WINDOWS
  u_long argp = 0;
  int flag = ioctlsocket(sock, FIONBIO, &argp);
  assert(flag >= 0);
  int rc;
#else
  int flag = fcntl(sock, F_GETFL);
  assert(flag >= 0);
  int rc = fcntl(sock, F_SETFL, O_NONBLOCK | flag);
  assert(rc >= 0);
#endif

  // Tries to connect.
  rc = connect(sock, reinterpret_cast<const struct sockaddr *>(&endpoint),
    sizeof(endpoint));
  assert(rc == 0 || (rc < 0 && errno == EINPROGRESS));

  LOG("Queueing 1 async connect.");

  AsyncRequest r;
  r.type_ = AsyncRequest::kConnect;
  r.sock_ = sock;
  r.connect_.callback_ = callback;
  r.connect_.endpoint_ = endpoint;

  network_->PushAsyncQueue(r);
}


void FunapiTransportImpl::AsyncSend(int sock,
  const IoVecList &sending, AsyncSendCallback callback) {
  assert(!sending.empty());
  LOG1("Queueing %d async sends.", sending.size());

  for (IoVecList::const_iterator itr = sending.begin(), itr_end = sending.end();
    itr != itr_end;
    ++itr) {
    auto &e = *itr;
    AsyncRequest r;
    r.type_ = AsyncRequest::kSend;
    r.sock_ = sock;
    r.send_.callback_ = callback;
    r.send_.buf_ = const_cast<uint8_t *>(e.data());
    r.send_.buf_len_ = e.size();
    r.send_.buf_offset_ = 0;

    network_->PushAsyncQueue(r);
  }
}


void FunapiTransportImpl::AsyncReceive(int sock,
  const struct iovec &receiving, AsyncReceiveCallback callback) {
  assert(receiving.iov_len != 0);
  LOG("Queueing 1 async receive.");

  AsyncRequest r;
  r.type_ = AsyncRequest::kReceive;
  r.sock_ = sock;
  r.recv_.callback_ = callback;
  r.recv_.buf_ = reinterpret_cast<uint8_t *>(receiving.iov_base);
  r.recv_.capacity_ = receiving.iov_len;

  network_->PushAsyncQueue(r);
}


void FunapiTransportImpl::AsyncSendTo(int sock, const Endpoint &ep,
  const IoVecList &sending, AsyncSendCallback callback) {
  assert(!sending.empty());
  LOG1("Queueing %d async sends.", sending.size());

  for (IoVecList::const_iterator itr = sending.begin(), itr_end = sending.end();
    itr != itr_end;
    ++itr) {
    auto &e = *itr;

    AsyncRequest r;
    r.type_ = AsyncRequest::kSendTo;
    r.sock_ = sock;
    r.sendto_.endpoint_ = ep;
    r.sendto_.callback_ = callback;
    r.sendto_.buf_offset_ = 0;
    r.sendto_.buf_len_ = e.size();
    r.sendto_.buf_ = const_cast<uint8_t *>(e.data());

    network_->PushAsyncQueue(r);
  }
}


void FunapiTransportImpl::AsyncReceiveFrom(int sock, const Endpoint &ep,
  const struct iovec &receiving, AsyncReceiveCallback callback) {
  assert(receiving.iov_len != 0);
  LOG("Queueing 1 async receive.");

  AsyncRequest r;
  r.type_ = AsyncRequest::kReceiveFrom;
  r.sock_ = sock;
  r.recvfrom_.endpoint_ = ep;
  r.recvfrom_.callback_ = callback;
  r.recvfrom_.buf_ = reinterpret_cast<uint8_t *>(receiving.iov_base);
  r.recvfrom_.capacity_ = receiving.iov_len;

  network_->PushAsyncQueue(r);
}


////////////////////////////////////////////////////////////////////////////////
// FunapiHttpTransportImpl implementation.

class FunapiHttpTransportImpl : public FunapiTransportBase {
 public:
  FunapiHttpTransportImpl(const std::string &hostname_or_ip, uint16_t port, bool https);
  virtual ~FunapiHttpTransportImpl();

  void Start();
  void Stop();
  bool Started();

 private:
  virtual void EncodeThenSendMessage();

  void WebRequestCb(int state);
  void WebResponseHeaderCb(void *data, int len);
  void WebResponseBodyCb(void *data, int len);

  void AsyncWebRequest(const char* host_url, const IoVecList &sending,
    AsyncWebRequestCallback callback,
    AsyncWebResponseCallback receive_header,
    AsyncWebResponseCallback receive_body);

  std::string host_url_;
  int recv_length_;

  static bool curl_initialized_;
  static int curl_initialized_count_;
};

bool FunapiHttpTransportImpl::curl_initialized_ = false;
int FunapiHttpTransportImpl::curl_initialized_count_ = 0;

FunapiHttpTransportImpl::FunapiHttpTransportImpl(const std::string &hostname_or_ip,
                                                 uint16_t port, bool https)
    : FunapiTransportBase(kHttp), recv_length_(0) {
  char url[1024];
  sprintf(url, "%s://%s:%d/v%d/",
      https ? "https" : "http", hostname_or_ip.c_str(), port,
      kCurrentFunapiProtocolVersion);
  host_url_ = url;
  LOG1("Host url : %s", *FString(host_url_.c_str()));

  if (!curl_initialized_) {
    if (CURLE_OK == curl_global_init(CURL_GLOBAL_ALL)) {
      curl_initialized_ = true;
    }
  }

  if (curl_initialized_)
    ++curl_initialized_count_;
}


FunapiHttpTransportImpl::~FunapiHttpTransportImpl() {
  if (curl_initialized_) {
    --curl_initialized_count_;
    if (0 == curl_initialized_count_) {
      curl_global_cleanup();
      curl_initialized_ = false;
    }
  }
}


void FunapiHttpTransportImpl::Start() {
  std::unique_lock<std::mutex> lock(mutex_);
  Init();
  state_ = kConnected;
  recv_length_ = 0;
  LOG("Started.");
}


void FunapiHttpTransportImpl::Stop() {
  if (state_ == kDisconnected)
    return;

  {
    std::unique_lock<std::mutex> lock(mutex_);
    state_ = kDisconnected;
    LOG("Stopped.");

    // TODO : clear list

    on_stopped_();
  }
}


bool FunapiHttpTransportImpl::Started() {
  std::unique_lock<std::mutex> lock(mutex_);
  return (state_ == kConnected);
}


void FunapiHttpTransportImpl::EncodeThenSendMessage() {
  assert(state_ == kConnected);
  if (!EncodeMessage()) {
    Stop();
    return;
  }

  assert(!sending_.empty());

  AsyncWebRequest(host_url_.c_str(), sending_,
    [this](int state){ WebRequestCb(state);  },
    [this](void* data, int len){ WebResponseHeaderCb(data, len); },
    [this](void* data, int len){ WebResponseBodyCb(data, len); });
}


void FunapiHttpTransportImpl::WebRequestCb(int state) {
  LOG1("WebRequestCb called. state: %d", state);
  if (state == kWebRequestStart) {
    recv_length_ = 0;
    return;
  } else if (state == kWebRequestEnd) {
    {
      std::unique_lock<std::mutex> lock(mutex_);;
      assert(sending_.size() >= 2);

      IoVecList::iterator itr = sending_.begin();
      int index = 0;
      while (itr != sending_.end() && index < 2) {
        itr = sending_.erase(itr);
        ++index;
      }

      std::stringstream version;
      version << kCurrentFunapiProtocolVersion;
      header_fields_[kVersionHeaderField] = version.str();
      std::stringstream length;
      length << recv_length_;
      header_fields_[kLengthHeaderField] = length.str();
      header_decoded_ = true;
    }

    if (!DecodeMessage(recv_length_)) {
      Stop();
      on_stopped_();
      return;
    }

    LOG1("Ready to receive more. We can receive upto %ld more bytes.",
         (receiving_len_ - received_size_));
  }
}


void FunapiHttpTransportImpl::WebResponseHeaderCb(void *data, int len) {
  char buf[1024];
  memcpy(buf, data, len);
  buf[len-2] = '\0';

  char *ptr = std::search(buf, buf + len, kHeaderFieldDelimeter,
      kHeaderFieldDelimeter + sizeof(kHeaderFieldDelimeter) - 1);
  ssize_t eol_offset = ptr - buf;
  if (eol_offset >= len)
    return;

  // Generates null-terminated string by replacing the delimeter with \0.
  *ptr = '\0';
  const char *e1 = buf, *e2 = ptr + 1;
  while (*e2 == ' ' || *e2 == '\t') ++e2;
  LOG2("Decoded header field '%s' => '%s'", *FString(e1), *FString(e2));
  header_fields_[e1] = e2;
}


void FunapiHttpTransportImpl::WebResponseBodyCb(void *data, int len) {
  int offset = received_size_ + recv_length_;
  if (offset + len >= receiving_len_) {
    size_t new_size = receiving_len_ + kUnitBufferSize;
    LOG1("Resizing a buffer to %dbytes.", new_size);
    receiving_.resize(new_size + kUnitBufferPaddingSize);
    receiving_len_ = new_size;
  }

  uint8_t *buf = reinterpret_cast<uint8_t*>(receiving_.data());
  memcpy(buf + offset, data, len);
  recv_length_ += len;
}


void FunapiHttpTransportImpl::AsyncWebRequest(const char* host_url, const IoVecList &sending,
  AsyncWebRequestCallback callback,
  AsyncWebResponseCallback receive_header,
  AsyncWebResponseCallback receive_body) {
  assert(!sending.empty());
  LOG1("Queueing %d async sends.", sending.size());

  for (IoVecList::const_iterator itr = sending.begin(), itr_end = sending.end();
    itr != itr_end;
    ++itr) {
    const auto &header = *itr;
    const auto &body = *(++itr);

    AsyncRequest r;
    r.type_ = AsyncRequest::kWebRequest;
    r.web_request_.url_ = host_url;
    r.web_request_.request_callback_ = callback;
    r.web_request_.receive_header_ = receive_header;
    r.web_request_.receive_body_ = receive_body;
    r.web_request_.header_ = std::string(header.begin(), header.end());
    r.web_request_.body_ = body;
    r.web_request_.body_len_ = body.size();

    network_->PushAsyncQueue(r);
  }
}




////////////////////////////////////////////////////////////////////////////////
// FunapiTcpTransport implementation.

FunapiTcpTransport::FunapiTcpTransport(
    const std::string &hostname_or_ip, uint16_t port)
    : impl_(std::make_shared<FunapiTransportImpl>(kTcp, hostname_or_ip, port)) {
}


FunapiTcpTransport::~FunapiTcpTransport() {
}


void FunapiTcpTransport::RegisterEventHandlers(
    const OnReceived &on_received, const OnStopped &on_stopped) {
  impl_->RegisterEventHandlers(on_received, on_stopped);
}


void FunapiTcpTransport::Start() {
  impl_->Start();
}


void FunapiTcpTransport::Stop() {
  impl_->Stop();
}


void FunapiTcpTransport::SendMessage(Json &message) {
  impl_->SendMessage(message);
}


void FunapiTcpTransport::SendMessage(FJsonObject &message) {
  impl_->SendMessage(message);
}


void FunapiTcpTransport::SendMessage(FunMessage &message) {
  impl_->SendMessage(message);
}


bool FunapiTcpTransport::Started() const {
  return impl_->Started();
}

TransportProtocol FunapiTcpTransport::Protocol() const {
  return TransportProtocol::kTcp;
}


void FunapiTcpTransport::SetNetwork(FunapiNetwork* network)
{
  impl_->SetNetwork(network);
}


////////////////////////////////////////////////////////////////////////////////
// FunapiUdpTransport implementation.

FunapiUdpTransport::FunapiUdpTransport(
    const std::string &hostname_or_ip, uint16_t port)
    : impl_(std::make_shared<FunapiTransportImpl>(kUdp, hostname_or_ip, port)) {
}


FunapiUdpTransport::~FunapiUdpTransport() {
}


void FunapiUdpTransport::RegisterEventHandlers(
    const OnReceived &on_received, const OnStopped &on_stopped) {
  impl_->RegisterEventHandlers(on_received, on_stopped);
}


void FunapiUdpTransport::Start() {
  impl_->Start();
}


void FunapiUdpTransport::Stop() {
  impl_->Stop();
}


void FunapiUdpTransport::SendMessage(Json &message) {
  impl_->SendMessage(message);
}


void FunapiUdpTransport::SendMessage(FJsonObject &message) {
  impl_->SendMessage(message);
}


void FunapiUdpTransport::SendMessage(FunMessage &message) {
  impl_->SendMessage(message);
}


bool FunapiUdpTransport::Started() const {
  return impl_->Started();
}

TransportProtocol FunapiUdpTransport::Protocol() const {
  return TransportProtocol::kUdp;
}

void FunapiUdpTransport::SetNetwork(FunapiNetwork* network)
{
  impl_->SetNetwork(network);
}

////////////////////////////////////////////////////////////////////////////////
// FunapiHttpTransport implementation.

FunapiHttpTransport::FunapiHttpTransport(
    const std::string &hostname_or_ip, uint16_t port, bool https /*= false*/)
    : impl_(std::make_shared<FunapiHttpTransportImpl>(hostname_or_ip, port, https)) {
}


FunapiHttpTransport::~FunapiHttpTransport() {
}


void FunapiHttpTransport::RegisterEventHandlers(
    const OnReceived &on_received, const OnStopped &on_stopped) {
  impl_->RegisterEventHandlers(on_received, on_stopped);
}


void FunapiHttpTransport::Start() {
  impl_->Start();
}


void FunapiHttpTransport::Stop() {
  impl_->Stop();
}


void FunapiHttpTransport::SendMessage(Json &message) {
  impl_->SendMessage(message);
}


void FunapiHttpTransport::SendMessage(FJsonObject &message) {
  impl_->SendMessage(message);
}


void FunapiHttpTransport::SendMessage(FunMessage &message) {
  impl_->SendMessage(message);
}


bool FunapiHttpTransport::Started() const {
  return impl_->Started();
}

TransportProtocol FunapiHttpTransport::Protocol() const {
  return TransportProtocol::kHttp;
}

void FunapiHttpTransport::SetNetwork(FunapiNetwork *network) {
  impl_->SetNetwork(network);
}

}  // namespace fun
