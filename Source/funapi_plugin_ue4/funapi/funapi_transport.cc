// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin.h"
#include "funapi_utils.h"
#include "funapi_transport.h"
#include "funapi_network.h"

namespace fun {

// Funapi header-related constants.
static const char* kHeaderDelimeter = "\n";
static const char* kHeaderFieldDelimeter = ":";
static const char* kVersionHeaderField = "VER";
static const char* kPluginVersionHeaderField = "PVER";
static const char* kLengthHeaderField = "LEN";
// static const char* kEncryptionHeaderField = "ENC";

////////////////////////////////////////////////////////////////////////////////
// FunapiTransportImpl implementation.

class FunapiTransportImpl : public std::enable_shared_from_this<FunapiTransportImpl> {
 public:
  typedef FunapiTransport::TransportEventHandler TransportEventHandler;
  typedef FunapiTransport::OnReceived OnReceived;

  // Buffer-related constants.
  static const int kUnitBufferSize = 65536;

  FunapiTransportImpl(TransportProtocol type, FunEncoding encoding);
  virtual ~FunapiTransportImpl();

  bool IsStarted();
  virtual void Start() = 0;
  virtual void Stop() = 0;

  void SendMessage(rapidjson::Document &message);
  void SendMessage(FunMessage &message);
  void SendMessage(const char *body);

  void SetNetwork(std::weak_ptr<FunapiNetwork> network);

  virtual TransportProtocol GetProtocol() { return TransportProtocol::kDefault; };
  FunEncoding GetEncoding() { return encoding_; };

  void SetConnectTimeout(time_t timeout);

  void AddStartedCallback(const TransportEventHandler &handler);
  void AddStoppedCallback(const TransportEventHandler &handler);
  void AddConnectFailedCallback(const TransportEventHandler &handler);
  void AddConnectTimeoutCallback(const TransportEventHandler &handler);
  void AddDisconnectedCallback(const TransportEventHandler &handler);

  virtual void ResetClientPingTimeout() {};

  void SetReceivedHandler(OnReceived handler);

 protected:
  typedef std::map<std::string, std::string> HeaderFields;

  virtual bool EncodeThenSendMessage(std::vector<uint8_t> body) = 0;

  void PushSendQueue(std::function<bool()> task);
  void Send();
  void PushTaskQueue(std::function<void()> task);
  void PushStopTask();

  bool DecodeMessage(int nRead, std::vector<uint8_t> &receiving, int &next_decoding_offset, bool &header_decoded, HeaderFields &header_fields);
  bool TryToDecodeHeader(std::vector<uint8_t> &receiving, int &next_decoding_offset, bool &header_decoded, HeaderFields &header_fields);
  bool TryToDecodeBody(std::vector<uint8_t> &receiving, int &next_decoding_offset, bool &header_decoded, HeaderFields &header_fields);
  bool EncodeMessage(std::vector<uint8_t> &body);
  bool DecodeMessage(int nRead, std::vector<uint8_t> &receiving);
  std::string MakeHeaderString(const std::vector<uint8_t> &body);

  // Registered event handlers.
  OnReceived on_received_;

  std::weak_ptr<FunapiNetwork> network_;

  std::vector<std::function<bool()>> v_send_;
//  std::mutex v_send_mutex_;

  // Encoding-serializer-releated member variables.
  FunEncoding encoding_ = FunEncoding::kNone;

  time_t connect_timeout_seconds_ = 10;
  FunapiTimer connect_timeout_timer_;

  void OnTransportStarted(const TransportProtocol protocol);
  void OnTransportClosed(const TransportProtocol protocol);

  void OnTransportConnectFailed(const TransportProtocol protocol);
  void OnTransportConnectTimeout(const TransportProtocol protocol);
  void OnTransportDisconnected(const TransportProtocol protocol);

  FunapiTransportState state_;

 private:
  // Message-related.
  bool first_sending_ = true;

  TransportProtocol protocol_;

  FunapiEvent<TransportEventHandler> on_transport_stared_;
  FunapiEvent<TransportEventHandler> on_transport_closed_;
  FunapiEvent<TransportEventHandler> on_transport_connect_failed_;
  FunapiEvent<TransportEventHandler> on_transport_connect_timeout_;
  FunapiEvent<TransportEventHandler> on_transport_disconnect_;
};


FunapiTransportImpl::FunapiTransportImpl(TransportProtocol protocol, FunEncoding encoding)
  : protocol_(protocol), encoding_(encoding), state_(kDisconnected) {
}


FunapiTransportImpl::~FunapiTransportImpl() {
}


bool FunapiTransportImpl::EncodeMessage(std::vector<uint8_t> &body) {
  std::string header = MakeHeaderString(body);

  std::string body_string(body.cbegin(), body.cend());
  DebugUtils::Log("Header to send: %s", header.c_str());
  DebugUtils::Log("send message: %s", body_string.c_str());

  body.insert(body.begin(), header.cbegin(), header.cend());

  return true;
}


bool FunapiTransportImpl::DecodeMessage(int nRead, std::vector<uint8_t> &receiving, int &next_decoding_offset, bool &header_decoded, HeaderFields &header_fields) {
  if (nRead < 0) {
    DebugUtils::Log("receive failed: %s", strerror(errno));

    return false;
  }

  // Tries to decode as many messags as possible.
  while (true) {
    if (header_decoded == false) {
      if (TryToDecodeHeader(receiving, next_decoding_offset, header_decoded, header_fields) == false) {
        break;
      }
    }
    if (header_decoded) {
      if (TryToDecodeBody(receiving, next_decoding_offset, header_decoded, header_fields) == false) {
        break;
      }
      else {
        int new_length = static_cast<int>(receiving.size() - next_decoding_offset);
        if (new_length > 0) {
          receiving.assign(receiving.begin() + next_decoding_offset, receiving.end());
        }
        else {
          new_length = 0;
        }
        next_decoding_offset = 0;
        receiving.resize(new_length);
      }
    }
  }

  return true;
}


bool FunapiTransportImpl::DecodeMessage(int nRead, std::vector<uint8_t> &receiving) {
  if (nRead < 0) {
    DebugUtils::Log("receive failed: %s", strerror(errno));

    return false;
  }

  int next_decoding_offset = 0;
  bool header_decoded = false;
  HeaderFields header_fields;

  DebugUtils::Log("Received %d bytes.", nRead);

  // Tries to decode as many messags as possible.
  while (true) {
    if (header_decoded == false) {
      if (TryToDecodeHeader(receiving, next_decoding_offset, header_decoded, header_fields) == false) {
        break;
      }
    }
    if (header_decoded) {
      if (TryToDecodeBody(receiving, next_decoding_offset, header_decoded, header_fields) == false) {
        break;
      }
    }
  }

  return true;
}


void FunapiTransportImpl::SendMessage(rapidjson::Document &message) {
  rapidjson::StringBuffer string_buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(string_buffer);
  message.Accept(writer);

  SendMessage(string_buffer.GetString());
}


void FunapiTransportImpl::SendMessage(FunMessage &message) {
  std::string body = message.SerializeAsString();
  SendMessage(body.c_str());
}


void FunapiTransportImpl::SendMessage(const char *body) {
  std::vector<uint8_t> v_body(strlen(body));
  memcpy(v_body.data(), body, strlen(body));

  PushSendQueue([this,v_body]()->bool{ return EncodeThenSendMessage(v_body); });
}


bool FunapiTransportImpl::TryToDecodeHeader(std::vector<uint8_t> &receiving, int &next_decoding_offset, bool &header_decoded, HeaderFields &header_fields) {
  DebugUtils::Log("Trying to decode header fields.");
  int received_size = static_cast<int>(receiving.size());
  for (; next_decoding_offset < received_size;) {
    char *base = reinterpret_cast<char *>(receiving.data());
    char *ptr =
      std::search(base + next_decoding_offset,
      base + received_size,
      kHeaderDelimeter,
      kHeaderDelimeter + strlen(kHeaderDelimeter));

    ssize_t eol_offset = ptr - base;
    if (eol_offset >= received_size) {
      // Not enough bytes. Wait for more bytes to come.
      DebugUtils::Log("We need more bytes for a header field. Waiting.");
      return false;
    }

    // Generates a null-termianted string by replacing the delimeter with \0.
    *ptr = '\0';
    char *line = base + next_decoding_offset;
    DebugUtils::Log("Header line: %s", line);

    ssize_t line_length = eol_offset - next_decoding_offset;
    next_decoding_offset = static_cast<int>(eol_offset + 1);

    if (line_length == 0) {
      // End of header.
      header_decoded = true;
      DebugUtils::Log("End of header reached. Will decode body from now.");
      return true;
    }

    ptr = std::search(
      line, line + line_length, kHeaderFieldDelimeter,
      kHeaderFieldDelimeter + strlen(kHeaderFieldDelimeter));
    assert((ptr - base) < eol_offset);

    // Generates null-terminated string by replacing the delimeter with \0.
    *ptr = '\0';
    char *e1 = line, *e2 = ptr + 1;
    while (*e2 == ' ' || *e2 == '\t') ++e2;
    DebugUtils::Log("Decoded header field '%s' => '%s'", e1, e2);
    header_fields[e1] = e2;
  }
  return false;
}


bool FunapiTransportImpl::TryToDecodeBody(std::vector<uint8_t> &receiving, int &next_decoding_offset, bool &header_decoded, HeaderFields &header_fields) {
  int received_size = static_cast<int>(receiving.size());
  // version header
  HeaderFields::const_iterator it = header_fields.find(kVersionHeaderField);
  assert(it != header_fields.end());
  int version = atoi(it->second.c_str());
  assert(version == static_cast<int>(FunapiVersion::kProtocolVersion));

  // length header
  it = header_fields.find(kLengthHeaderField);
  int body_length = atoi(it->second.c_str());
  DebugUtils::Log("We need %d bytes for a message body.", body_length);

  if (received_size - next_decoding_offset < body_length) {
    // Need more bytes.
    DebugUtils::Log("We need more bytes for a message body. Waiting.");
    return false;
  }

  if (body_length > 0) {
    assert(state_ == kConnected);

    if (state_ != kConnected) {
      DebugUtils::Log("unexpected message");
      return false;
    }

    std::vector<uint8_t> v(receiving.begin() + next_decoding_offset, receiving.begin() + next_decoding_offset + body_length);
    v.push_back('\0');

    // Moves the read offset.
    next_decoding_offset += body_length;

    // The network module eats the fields and invokes registered handler
    PushTaskQueue([this, header_fields, v](){ on_received_(GetProtocol(), GetEncoding(), header_fields, v); });
  }

  // Prepares for a next message.
  header_decoded = false;
  header_fields.clear();

  return true;
}


std::string FunapiTransportImpl::MakeHeaderString(const std::vector<uint8_t> &body) {
  std::string header;
  char buffer[1024];

  snprintf(buffer, sizeof(buffer), "%s%s%d%s",
    kVersionHeaderField, kHeaderFieldDelimeter,
    static_cast<int>(FunapiVersion::kProtocolVersion), kHeaderDelimeter);
  header += buffer;

  if (first_sending_) {
    snprintf(buffer, sizeof(buffer), "%s%s%d%s",
             kPluginVersionHeaderField, kHeaderFieldDelimeter,
             static_cast<int>(FunapiVersion::kPluginVersion), kHeaderDelimeter);
    header += buffer;

    first_sending_ = false;
  }

  snprintf(buffer, sizeof(buffer), "%s%s%lu%s",
    kLengthHeaderField, kHeaderFieldDelimeter,
    static_cast<unsigned long>(body.size()), kHeaderDelimeter);
  header += buffer;

  header += kHeaderDelimeter;

  return header;
}


void FunapiTransportImpl::SetNetwork(std::weak_ptr<FunapiNetwork> network)
{
  network_ = network;
}


bool FunapiTransportImpl::IsStarted() {
  return (state_ == kConnected);
}


void FunapiTransportImpl::PushSendQueue(std::function<bool()> task) {
//  std::unique_lock<std::mutex> lock(v_send_mutex_);
  v_send_.push_back(task);
}


void FunapiTransportImpl::Send() {
  std::function<bool()> task;
  while (true) {
    task = nullptr;
    {
//      std::unique_lock<std::mutex> lock(v_send_mutex_);
      if (v_send_.empty()) {
        break;
      }
      else {
        task = std::move(v_send_.front());
        v_send_.erase(v_send_.begin());
      }
    }

    if (task) {
      if (false == task()) {
//        std::unique_lock<std::mutex> lock(v_send_mutex_);
        v_send_.insert(v_send_.begin(), task);
        break;
      }
    }
  }
}


void FunapiTransportImpl::PushTaskQueue(std::function<void()> task) {
  auto network = network_.lock();
  if (network) {
    auto self(shared_from_this());
    network->PushTaskQueue([self, task]{ task(); });
  }
}


void FunapiTransportImpl::PushStopTask() {
  PushTaskQueue([this](){
    Stop();
  });
}


void FunapiTransportImpl::SetConnectTimeout(time_t timeout) {
  connect_timeout_seconds_ = timeout;
}


void FunapiTransportImpl::AddStartedCallback(const TransportEventHandler &handler) {
  on_transport_stared_ += handler;
}


void FunapiTransportImpl::AddStoppedCallback(const TransportEventHandler &handler) {
  on_transport_closed_ += handler;
}


void FunapiTransportImpl::AddConnectFailedCallback(const TransportEventHandler &handler) {
  on_transport_connect_failed_ += handler;
}


void FunapiTransportImpl::AddConnectTimeoutCallback(const TransportEventHandler &handler) {
  on_transport_connect_timeout_ += handler;
}


void FunapiTransportImpl::AddDisconnectedCallback(const TransportEventHandler &handler) {
  on_transport_disconnect_ += handler;
}


void FunapiTransportImpl::OnTransportStarted(const TransportProtocol protocol) {
  PushTaskQueue([this, protocol] { on_transport_stared_(protocol); });
}


void FunapiTransportImpl::OnTransportClosed(const TransportProtocol protocol) {
  PushTaskQueue([this, protocol] { on_transport_closed_(protocol); });
}


void FunapiTransportImpl::OnTransportConnectFailed(const TransportProtocol protocol) {
  PushTaskQueue([this, protocol] { on_transport_connect_failed_(protocol); });
}


void FunapiTransportImpl::OnTransportConnectTimeout(const TransportProtocol protocol) {
  PushTaskQueue([this, protocol] { on_transport_connect_timeout_(protocol); });
}


void FunapiTransportImpl::OnTransportDisconnected(const TransportProtocol protocol) {
  PushTaskQueue([this, protocol] { on_transport_disconnect_(protocol); });
}


void FunapiTransportImpl::SetReceivedHandler(OnReceived handler) {
  on_received_ = handler;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiSocketTransportImpl implementation.

class FunapiSocketTransportImpl : public FunapiTransportImpl {
 public:
  FunapiSocketTransportImpl(TransportProtocol protocol,
                      const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding);
  virtual ~FunapiSocketTransportImpl();
  void Stop();
  virtual void ResetClientPingTimeout() {};
  virtual int GetSocket();

  virtual void AddInitSocketCallback(const TransportEventHandler &handler);
  virtual void AddCloseSocketCallback(const TransportEventHandler &handler);

  virtual void OnSocketSelect(const fd_set rset, const fd_set wset, const fd_set eset);
  virtual void Update();

 protected:
  void CloseSocket();
  virtual void Recv() = 0;

  // State-related.
  Endpoint endpoint_;
  int socket_;

  std::vector<struct in_addr> in_addrs_;
  uint16_t port_;

  void OnInitSocket(const TransportProtocol protocol);
  void OnCloseSocket(const TransportProtocol protocol);

 private:
  FunapiEvent<TransportEventHandler> on_init_socket_;
  FunapiEvent<TransportEventHandler> on_close_socket_;
};


FunapiSocketTransportImpl::FunapiSocketTransportImpl(TransportProtocol protocol,
                                         const std::string &hostname_or_ip,
                                         uint16_t port, FunEncoding encoding)
    : FunapiTransportImpl(protocol, encoding), socket_(-1), port_(port) {
  struct hostent *entry = gethostbyname(hostname_or_ip.c_str());
  assert(entry);

  struct in_addr addr;

  if (entry) {
    int index = 0;
    while (entry->h_addr_list[index]) {
      memcpy(&addr, entry->h_addr_list[index], entry->h_length);
      in_addrs_.push_back(addr);
      ++index;
    }

    addr = in_addrs_[0];

    // log
    /*
    for (auto t : in_addrs_)
    {
      printf ("%s\n", inet_ntoa(t));
    }
    */
  }
  else {
    addr.s_addr = INADDR_NONE;
  }

  endpoint_.sin_family = AF_INET;
  endpoint_.sin_addr = addr;
  endpoint_.sin_port = htons(port);
}


FunapiSocketTransportImpl::~FunapiSocketTransportImpl() {
}


void FunapiSocketTransportImpl::Stop() {
  CloseSocket();

  OnTransportClosed(GetProtocol());

  state_ = kDisconnected;
}


void FunapiSocketTransportImpl::CloseSocket() {
  if (socket_ >= 0) {
    OnCloseSocket(GetProtocol());

#ifdef FUNAPI_PLATFORM_WINDOWS
    closesocket(socket_);
#else
    close(socket_);
#endif
    socket_ = -1;
  }
}


int FunapiSocketTransportImpl::GetSocket() {
  return socket_;
}


void FunapiSocketTransportImpl::AddInitSocketCallback(const TransportEventHandler &handler) {
  on_init_socket_ += handler;
}


void FunapiSocketTransportImpl::AddCloseSocketCallback(const TransportEventHandler &handler) {
  on_close_socket_ += handler;
}


void FunapiSocketTransportImpl::OnInitSocket(const TransportProtocol protocol) {
  // PushTaskQueue([this, protocol] { on_init_socket_(protocol); });
  on_init_socket_(protocol);
}


void FunapiSocketTransportImpl::OnCloseSocket(const TransportProtocol protocol) {
  // PushTaskQueue([this, protocol] { on_close_socket_(protocol); });
  on_close_socket_(protocol);
}


void FunapiSocketTransportImpl::OnSocketSelect(const fd_set rset, const fd_set wset, const fd_set eset) {
  if (FD_ISSET(socket_, &rset)) {
    Recv();
  }

  if (FD_ISSET(socket_, &wset)) {
    Send();
  }
}


void FunapiSocketTransportImpl::Update() {
}


////////////////////////////////////////////////////////////////////////////////
// FunapiTcpTransportImpl implementation.

class FunapiTcpTransportImpl : public FunapiSocketTransportImpl {
 public:
  FunapiTcpTransportImpl(TransportProtocol protocol,
    const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding);
  virtual ~FunapiTcpTransportImpl();

  TransportProtocol GetProtocol();

  void Start();
  void SetDisableNagle(const bool disable_nagle);
  void SetAutoReconnect(const bool auto_reconnect);
  void SetEnablePing(const bool enable_ping);
  void ResetClientPingTimeout();

  void OnSocketSelect(const fd_set rset, const fd_set wset, const fd_set eset);
  void Update();

  void SetSendClientPingMessageHandler(std::function<bool(const TransportProtocol protocol)> handler);

 protected:
  bool EncodeThenSendMessage(std::vector<uint8_t> body);
  void Connect();
  void Recv();

 private:
  void InitSocket();

  // Ping message-related constants.
  static const time_t kPingIntervalSecond = 3;
  static const time_t kPingTimeoutSeconds = 20;

  static const int kMaxReconnectCount = 3;
  static const time_t kMaxReconnectWaitSeconds = 10;
  int reconnect_count_ = 0;
  int connect_addr_index_ = 0;
  FunapiTimer reconnect_wait_timer_;
  time_t reconnect_wait_seconds_;

  std::function<void(const fd_set rset, const fd_set wset, const fd_set eset)> on_socket_select_;
  std::function<void()> on_update_;

  void Ping();
  void CheckConnectTimeout();
  void WaitForAutoReconnect();

  bool disable_nagle_ = false;
  bool auto_reconnect_ = false;
  bool enable_ping_ = false;

  FunapiTimer client_ping_timeout_timer_;

  std::function<bool(const TransportProtocol protocol)> send_client_ping_message_handler_;
};


FunapiTcpTransportImpl::FunapiTcpTransportImpl(TransportProtocol protocol,
  const std::string &hostname_or_ip,
  uint16_t port,
  FunEncoding encoding)
  : FunapiSocketTransportImpl(protocol, hostname_or_ip, port, encoding) {
}


FunapiTcpTransportImpl::~FunapiTcpTransportImpl() {
  CloseSocket();
}


TransportProtocol FunapiTcpTransportImpl::GetProtocol() {
  return TransportProtocol::kTcp;
}


void FunapiTcpTransportImpl::Start() {
  state_ = kConnecting;

  on_update_ = [](){};
  on_socket_select_ = [this](const fd_set rset, const fd_set wset, const fd_set eset) {
    if (FD_ISSET(socket_, &rset) && FD_ISSET(socket_, &wset)) {
      bool isConnected = true;

#ifdef FUNAPI_PLATFORM_WINDOWS
      if (FD_ISSET(socket_, &eset)) {
        isConnected = false;
      }
#else
      int e;
      socklen_t e_size = sizeof(e);
      int r = getsockopt(socket_, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&e), &e_size);
      if (r < 0 || e != 0) {
        isConnected = false;
      }
#endif

      if (isConnected) {
        // Makes a state transition.
        state_ = kConnected;

        on_socket_select_ = [this](const fd_set rs, const fd_set ws, const fd_set es) {
          FunapiSocketTransportImpl::OnSocketSelect(rs, ws, es);
        };

        client_ping_timeout_timer_.SetTimer(kPingIntervalSecond + kPingTimeoutSeconds);

        on_update_ = [this](){
          Ping();
        };

        OnTransportStarted(TransportProtocol::kTcp);
      }
      else {
        DebugUtils::Log("failed - tcp connect");
        OnTransportConnectFailed(TransportProtocol::kTcp);

        ++connect_addr_index_;
        Connect();
      }
    }
  };

  reconnect_count_ = 0;
  connect_addr_index_ = 0;
  reconnect_wait_seconds_ = 1;
  Connect();
}


void FunapiTcpTransportImpl::Ping() {
  if (enable_ping_) {
    static FunapiTimer ping_send_timer(kPingIntervalSecond);

    if (ping_send_timer.IsExpired()){
      ping_send_timer.SetTimer(kPingIntervalSecond);
      send_client_ping_message_handler_(GetProtocol());
    }

    if (client_ping_timeout_timer_.IsExpired()) {
      DebugUtils::Log("Network seems disabled. Stopping the transport.");
      PushStopTask();
      return;
    }
  }
}


void FunapiTcpTransportImpl::CheckConnectTimeout() {
  if (connect_timeout_timer_.IsExpired()) {
    DebugUtils::Log("failed - tcp connect - timeout");
    // PushTaskQueue([this](){ OnConnectTimeout(TransportProtocol::kTcp); });
    OnTransportConnectTimeout(TransportProtocol::kTcp);

    if (auto_reconnect_) {
      ++reconnect_count_;

      if (kMaxReconnectCount < reconnect_count_) {
        ++connect_addr_index_;
        reconnect_count_ = 0;
        reconnect_wait_seconds_ = 1;

        Connect();
      }
      else {
        reconnect_wait_timer_.SetTimer(reconnect_wait_seconds_);

        DebugUtils::Log("Wait %d seconds for connect to TCP transport.", static_cast<int>(reconnect_wait_seconds_));

        on_update_ = [this](){
          WaitForAutoReconnect();
        };
      }
    }
    else {
      ++connect_addr_index_;
      Connect();
    }
  }
}


void FunapiTcpTransportImpl::WaitForAutoReconnect() {
  if (reconnect_wait_timer_.IsExpired()) {
    reconnect_wait_seconds_ *= 2;
    if (kMaxReconnectWaitSeconds < reconnect_wait_seconds_) {
      reconnect_wait_seconds_ = kMaxReconnectWaitSeconds;
    }

    Connect();
  }
}


void FunapiTcpTransportImpl::SetDisableNagle(const bool disable_nagle) {
  disable_nagle_ = disable_nagle;
}


void FunapiTcpTransportImpl::SetAutoReconnect(const bool auto_reconnect) {
  auto_reconnect_ = auto_reconnect;
}


void FunapiTcpTransportImpl::SetEnablePing(const bool enable_ping) {
  enable_ping_ = enable_ping;
}


void FunapiTcpTransportImpl::ResetClientPingTimeout() {
  client_ping_timeout_timer_.SetTimer(kPingTimeoutSeconds);
}


bool FunapiTcpTransportImpl::EncodeThenSendMessage(std::vector<uint8_t> body) {
  if (!EncodeMessage(body)) {
    PushStopTask();
    return false;
  }

  static int offset = 0;

  if (socket_ < 0)
    return false;

  int nSent = static_cast<int>(send(socket_, reinterpret_cast<char*>(body.data()) + offset, body.size() - offset, 0));

  if (nSent < 0) {
    PushStopTask();
    return false;
  }
  else {
    offset += nSent;
  }

  DebugUtils::Log("Sent %d bytes", nSent);

  if (offset == body.size()) {
    offset = 0;
    return true;
  }

  return false;
}


void FunapiTcpTransportImpl::InitSocket() {
  // Initiates a new socket.
  socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  assert(socket_ >= 0);

  // Makes the fd non-blocking.
#ifdef FUNAPI_PLATFORM_WINDOWS
  u_long argp = 0;
  int flag = ioctlsocket(socket_, FIONBIO, &argp);
  assert(flag >= 0);
#else
  int flag = fcntl(socket_, F_GETFL);
  assert(flag >= 0);
  int rc = fcntl(socket_, F_SETFL, O_NONBLOCK | flag);
  assert(rc >= 0);
#endif

  if (disable_nagle_) {
    int nagle_flag = 1;
    int result = setsockopt(socket_,
                            IPPROTO_TCP,
                            TCP_NODELAY,
                            reinterpret_cast<char*>(&nagle_flag),
                            sizeof(int));
    if (result < 0) {
      DebugUtils::Log("error - TCP_NODELAY");
    }
  }

  OnInitSocket(TransportProtocol::kTcp);
}


void FunapiTcpTransportImpl::Connect() {
  CloseSocket();

  if (connect_addr_index_ >= in_addrs_.size()) {
    on_update_ = [](){};
    on_socket_select_ = [](const fd_set rset, const fd_set wset, const fd_set eset){};
    return;
  }

  Endpoint endpoint;
  endpoint.sin_family = AF_INET;
  endpoint.sin_addr = in_addrs_[connect_addr_index_];
  endpoint.sin_port = htons(port_);

  InitSocket();

  connect_timeout_timer_.SetTimer(connect_timeout_seconds_);

  // Tries to connect.
  int rc = connect(socket_,
                   reinterpret_cast<const struct sockaddr *>(&endpoint_),
                   sizeof(endpoint_));
  assert(rc == 0 || (rc < 0 && errno == EINPROGRESS));

  DebugUtils::Log("Try to connect to server - %s", inet_ntoa(endpoint.sin_addr));

  on_update_ = [this](){
    CheckConnectTimeout();
  };
}


void FunapiTcpTransportImpl::Recv() {
  static std::vector<uint8_t> receiving_vector;
  static int next_decoding_offset = 0;
  static bool header_decoded = false;
  static HeaderFields header_fields;

  if (socket_ < 0)
    return;

  std::vector<uint8_t> buffer(kUnitBufferSize);

  int nRead = static_cast<int>(recv(socket_, reinterpret_cast<char*>(buffer.data()), kUnitBufferSize, 0));

  if (nRead <= 0) {
    if (nRead < 0) {
      DebugUtils::Log("receive failed: %s", strerror(errno));
    }
    PushStopTask();
    OnTransportDisconnected(GetProtocol());
    return;
  }

  receiving_vector.insert(receiving_vector.end(), buffer.cbegin(), buffer.cbegin() + nRead);

  if (!DecodeMessage(nRead, receiving_vector, next_decoding_offset, header_decoded, header_fields)) {
    if (nRead == 0) {
      DebugUtils::Log("Socket [%d] closed.", socket_);
      OnTransportDisconnected(GetProtocol());
    }

    PushStopTask();
  }
}


void FunapiTcpTransportImpl::OnSocketSelect(const fd_set rset, const fd_set wset, const fd_set eset)
{
  on_socket_select_(rset, wset, eset);
}


void FunapiTcpTransportImpl::Update() {
  on_update_();
}


void FunapiTcpTransportImpl::SetSendClientPingMessageHandler(std::function<bool(const TransportProtocol protocol)> handler) {
  send_client_ping_message_handler_ = handler;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiUdpTransportImpl implementation.

class FunapiUdpTransportImpl : public FunapiSocketTransportImpl {
public:
  FunapiUdpTransportImpl(TransportProtocol protocol,
    const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding);
  virtual ~FunapiUdpTransportImpl();

  TransportProtocol GetProtocol();

  void Start();

protected:
  bool EncodeThenSendMessage(std::vector<uint8_t> body);
  void Recv();

private:
  Endpoint recv_endpoint_;
};


FunapiUdpTransportImpl::FunapiUdpTransportImpl(TransportProtocol protocol,
  const std::string &hostname_or_ip,
  uint16_t port,
  FunEncoding encoding)
  : FunapiSocketTransportImpl(protocol, hostname_or_ip, port, encoding) {
  recv_endpoint_.sin_family = AF_INET;
  recv_endpoint_.sin_addr.s_addr = htonl(INADDR_ANY);
  recv_endpoint_.sin_port = htons(port);
}


FunapiUdpTransportImpl::~FunapiUdpTransportImpl() {
  CloseSocket();
}


TransportProtocol FunapiUdpTransportImpl::GetProtocol() {
  return TransportProtocol::kUdp;
}


void FunapiUdpTransportImpl::Start() {
  CloseSocket();

  socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  assert(socket_ >= 0);
  state_ = kConnected;

  OnInitSocket(TransportProtocol::kUdp);
  OnTransportStarted(TransportProtocol::kUdp);
}


bool FunapiUdpTransportImpl::EncodeThenSendMessage(std::vector<uint8_t> body) {
  if (!EncodeMessage(body)) {
    PushStopTask();
    return false;
  }

  // log
  // std::string temp_string(body.cbegin(), body.cend());
  // FUNAPI_LOG("Send = %s", *FString(temp_string.c_str()));

  socklen_t len = sizeof(endpoint_);

  if (socket_ < 0)
    return false;

  int nSent = static_cast<int>(sendto(socket_, reinterpret_cast<char*>(body.data()), body.size(), 0, reinterpret_cast<struct sockaddr*>(&endpoint_), len));

  if (nSent < 0) {
    PushStopTask();
    return false;
  }

  // FUNAPI_LOG("Sent %d bytes", nSent);

  return true;
}


void FunapiUdpTransportImpl::Recv() {
  std::vector<uint8_t> receiving_vector(kUnitBufferSize);
  socklen_t len = sizeof(recv_endpoint_);

  if (socket_<0)
    return;

  int nRead = static_cast<int>(recvfrom(socket_, reinterpret_cast<char*>(receiving_vector.data()), receiving_vector.size(), 0, reinterpret_cast<struct sockaddr*>(&recv_endpoint_), &len));

  // FUNAPI_LOG("nRead = %d", nRead);

  if (nRead < 0) {
    DebugUtils::Log("receive failed: %s", strerror(errno));
    OnTransportDisconnected(TransportProtocol::kUdp);
    PushStopTask();
    return;
  }

  if (!DecodeMessage(nRead, receiving_vector)) {
    if (nRead == 0) {
      DebugUtils::Log("Socket [%d] closed.", socket_);
      OnTransportDisconnected(TransportProtocol::kUdp);
    }

    PushStopTask();
  }
}


////////////////////////////////////////////////////////////////////////////////
// FunapiHttpTransportImpl implementation.

class FunapiHttpTransportImpl : public FunapiTransportImpl {
 public:
  FunapiHttpTransportImpl(const std::string &hostname_or_ip, uint16_t port, bool https, FunEncoding encoding);
  virtual ~FunapiHttpTransportImpl();

  TransportProtocol GetProtocol();

  void Start();
  void Stop();

  void Update();

 protected:
  bool EncodeThenSendMessage(std::vector<uint8_t> body);

 private:
  static size_t HttpResponseCb(void *data, size_t size, size_t count, void *cb);
  void WebResponseHeaderCb(void *data, int len, HeaderFields &header_fields);
  void WebResponseBodyCb(void *data, int len, std::vector<uint8_t> &receiving);

  std::string host_url_;
};


FunapiHttpTransportImpl::FunapiHttpTransportImpl(const std::string &hostname_or_ip,
                                                 uint16_t port, bool https, FunEncoding encoding)
  : FunapiTransportImpl(TransportProtocol::kHttp, encoding) {
  char url[1024];
  sprintf(url, "%s://%s:%d/v%d/",
      https ? "https" : "http", hostname_or_ip.c_str(), port,
      static_cast<int>(FunapiVersion::kProtocolVersion));
  host_url_ = url;

  DebugUtils::Log("Host url : %s", host_url_.c_str());
}


FunapiHttpTransportImpl::~FunapiHttpTransportImpl() {
}


TransportProtocol FunapiHttpTransportImpl::GetProtocol() {
  return TransportProtocol::kHttp;
}


void FunapiHttpTransportImpl::Start() {
  state_ = kConnected;
  DebugUtils::Log("Started.");

  OnTransportStarted(TransportProtocol::kHttp);
}


void FunapiHttpTransportImpl::Stop() {
  if (state_ == kDisconnected)
    return;

  OnTransportClosed(TransportProtocol::kHttp);

  state_ = kDisconnected;
  DebugUtils::Log("Stopped.");
}


size_t FunapiHttpTransportImpl::HttpResponseCb(void *data, size_t size, size_t count, void *cb) {
  AsyncWebResponseCallback *callback = (AsyncWebResponseCallback*)(cb);
  if (callback != NULL)
    (*callback)(data, static_cast<int>(size * count));
  return size * count;
}


bool FunapiHttpTransportImpl::EncodeThenSendMessage(std::vector<uint8_t> body) {
  std::string header = MakeHeaderString(body);

  // log
  // std::string temp_string(body.cbegin(), body.cend());
  // FUNAPI_LOG("HTTP Send header = %s \n body = %s", *FString(header.c_str()), *FString(temp_string.c_str()));

  CURL *ctx = curl_easy_init();
  if (ctx == NULL) {
    DebugUtils::Log("Unable to initialize cURL interface.");
    return false;
  }

  HeaderFields header_fields;
  std::vector<uint8_t> receiving;

  AsyncWebResponseCallback receive_header = [this, &header_fields](void* data, int len){ WebResponseHeaderCb(data, len, header_fields); };
  AsyncWebResponseCallback receive_body = [this, &receiving](void* data, int len){ WebResponseBodyCb(data, len, receiving); };

  struct curl_slist *chunk = NULL;
  chunk = curl_slist_append(chunk, header.c_str());
  curl_easy_setopt(ctx, CURLOPT_HTTPHEADER, chunk);
  curl_easy_setopt(ctx, CURLOPT_URL, host_url_.c_str());
  curl_easy_setopt(ctx, CURLOPT_POST, 1L);
  curl_easy_setopt(ctx, CURLOPT_POSTFIELDS, body.data());
  curl_easy_setopt(ctx, CURLOPT_POSTFIELDSIZE, body.size());
  curl_easy_setopt(ctx, CURLOPT_HEADERDATA, &receive_header);
  curl_easy_setopt(ctx, CURLOPT_WRITEDATA, &receive_body);
  curl_easy_setopt(ctx, CURLOPT_WRITEFUNCTION, &FunapiHttpTransportImpl::HttpResponseCb);

  CURLcode res = curl_easy_perform(ctx);
  if (res != CURLE_OK) {
    DebugUtils::Log("Error from cURL: %s", curl_easy_strerror(res));
    return false;
  }
  else {
    // std::to_string is not supported on android, using std::stringstream instead.
    std::stringstream ss_protocol_version;
    ss_protocol_version << static_cast<int>(FunapiVersion::kProtocolVersion);
    header_fields[kVersionHeaderField] = ss_protocol_version.str();

    std::stringstream ss_version_header_field;
    ss_version_header_field << receiving.size();
    header_fields[kLengthHeaderField] = ss_version_header_field.str();

    bool header_decoded = true;
    int next_decoding_offset = 0;
    if (TryToDecodeBody(receiving, next_decoding_offset, header_decoded, header_fields) == false) {
      PushStopTask();
    }
  }

  curl_easy_cleanup(ctx);
  curl_slist_free_all(chunk);

  return true;
}


void FunapiHttpTransportImpl::WebResponseHeaderCb(void *data, int len, HeaderFields &header_fields) {
  char buf[1024];
  memcpy(buf, data, len);
  buf[len-2] = '\0';

  char *ptr = std::search(buf, buf + len, kHeaderFieldDelimeter,
      kHeaderFieldDelimeter + strlen(kHeaderFieldDelimeter));
  ssize_t eol_offset = ptr - buf;
  if (eol_offset >= len)
    return;

  // Generates null-terminated string by replacing the delimeter with \0.
  *ptr = '\0';
  const char *e1 = buf, *e2 = ptr + 1;
  while (*e2 == ' ' || *e2 == '\t') ++e2;
  DebugUtils::Log("Decoded header field '%s' => '%s'", e1, e2);
  header_fields[e1] = e2;
}


void FunapiHttpTransportImpl::WebResponseBodyCb(void *data, int len, std::vector<uint8_t> &receiving) {
  receiving.insert(receiving.end(), (uint8_t*)data, (uint8_t*)data + len);
}


void FunapiHttpTransportImpl::Update() {
  Send();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiTcpTransport implementation.

int FunapiTransport::GetSocket() {
  return -1;
}


void FunapiTransport::AddInitSocketCallback(const TransportEventHandler &handler) {
}


void FunapiTransport::AddCloseSocketCallback(const TransportEventHandler &handler) {
}


void FunapiTransport::OnSocketSelect(const fd_set rset, const fd_set wset, const fd_set eset) {
}


void FunapiTransport::Update() {
}


void FunapiTransport::SetDisableNagle(const bool disable_nagle) {
}


void FunapiTransport::SetAutoReconnect(const bool disable_nagle) {
}


void FunapiTransport::SetEnablePing(const bool disable_nagle) {
}


void FunapiTransport::SetSendClientPingMessageHandler(std::function<bool(const TransportProtocol protocol)> handler) {
}


////////////////////////////////////////////////////////////////////////////////
// FunapiTcpTransport implementation.

FunapiTcpTransport::FunapiTcpTransport (const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding)
  : impl_(std::make_shared<FunapiTcpTransportImpl>(TransportProtocol::kTcp, hostname_or_ip, port, encoding)) {
}


std::shared_ptr<FunapiTcpTransport> FunapiTcpTransport::create(const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding) {
  return std::make_shared<FunapiTcpTransport>(hostname_or_ip, port, encoding);
}


TransportProtocol FunapiTcpTransport::GetProtocol() const {
  return impl_->GetProtocol();
}


void FunapiTcpTransport::Start() {
  impl_->Start();
}


void FunapiTcpTransport::Stop() {
  impl_->Stop();
}


void FunapiTcpTransport::SendMessage(rapidjson::Document &message) {
  impl_->SendMessage(message);
}


void FunapiTcpTransport::SendMessage(FunMessage &message) {
  impl_->SendMessage(message);
}


void FunapiTcpTransport::SendMessage(const char *body) {
  impl_->SendMessage(body);
}


bool FunapiTcpTransport::IsStarted() const {
  return impl_->IsStarted();
}


void FunapiTcpTransport::SetNetwork(std::weak_ptr<FunapiNetwork> network) {
  impl_->SetNetwork(network);
}


FunEncoding FunapiTcpTransport::GetEncoding() const {
  return impl_->GetEncoding();
}


void FunapiTcpTransport::SetConnectTimeout(time_t timeout) {
  return impl_->SetConnectTimeout(timeout);
}


void FunapiTcpTransport::AddStartedCallback(const TransportEventHandler &handler) {
  return impl_->AddStartedCallback(handler);
}


void FunapiTcpTransport::AddStoppedCallback(const TransportEventHandler &handler) {
  return impl_->AddStoppedCallback(handler);
}


void FunapiTcpTransport::AddConnectFailedCallback(const TransportEventHandler &handler) {
  return impl_->AddConnectFailedCallback(handler);
}


void FunapiTcpTransport::AddConnectTimeoutCallback(const TransportEventHandler &handler) {
  return impl_->AddConnectTimeoutCallback(handler);
}


void FunapiTcpTransport::SetDisableNagle(const bool disable_nagle) {
  return impl_->SetDisableNagle(disable_nagle);
}


void FunapiTcpTransport::SetAutoReconnect(const bool auto_reconnect) {
  return impl_->SetAutoReconnect(auto_reconnect);
}


void FunapiTcpTransport::SetEnablePing(const bool enable_ping) {
  return impl_->SetEnablePing(enable_ping);
}


void FunapiTcpTransport::ResetClientPingTimeout() {
  return impl_->ResetClientPingTimeout();
}


int FunapiTcpTransport::GetSocket() {
  return impl_->GetSocket();
}


void FunapiTcpTransport::AddInitSocketCallback(const TransportEventHandler &handler) {
  return impl_->AddInitSocketCallback(handler);
}


void FunapiTcpTransport::AddCloseSocketCallback(const TransportEventHandler &handler) {
  return impl_->AddCloseSocketCallback(handler);
}


void FunapiTcpTransport::OnSocketSelect(const fd_set rset, const fd_set wset, const fd_set eset)
{
  return impl_->OnSocketSelect(rset, wset, eset);
}


void FunapiTcpTransport::Update() {
  return impl_->Update();
}


void FunapiTcpTransport::SetSendClientPingMessageHandler(std::function<bool(const TransportProtocol protocol)> handler) {
  return impl_->SetSendClientPingMessageHandler(handler);
}


void FunapiTcpTransport::SetReceivedHandler(OnReceived handler) {
  return impl_->SetReceivedHandler(handler);
}


////////////////////////////////////////////////////////////////////////////////
// FunapiUdpTransport implementation.

FunapiUdpTransport::FunapiUdpTransport (const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding)
  : impl_(std::make_shared<FunapiUdpTransportImpl>(TransportProtocol::kUdp, hostname_or_ip, port, encoding)) {
}


std::shared_ptr<FunapiUdpTransport> FunapiUdpTransport::create(const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding) {
  return std::make_shared<FunapiUdpTransport>(hostname_or_ip, port, encoding);
}


TransportProtocol FunapiUdpTransport::GetProtocol() const {
  return impl_->GetProtocol();
}


void FunapiUdpTransport::Start() {
  impl_->Start();
}


void FunapiUdpTransport::Stop() {
  impl_->Stop();
}


void FunapiUdpTransport::SendMessage(rapidjson::Document &message) {
  impl_->SendMessage(message);
}


void FunapiUdpTransport::SendMessage(FunMessage &message) {
  impl_->SendMessage(message);
}


void FunapiUdpTransport::SendMessage(const char *body) {
  impl_->SendMessage(body);
}


bool FunapiUdpTransport::IsStarted() const {
  return impl_->IsStarted();
}


void FunapiUdpTransport::SetNetwork(std::weak_ptr<FunapiNetwork> network) {
  impl_->SetNetwork(network);
}


FunEncoding FunapiUdpTransport::GetEncoding() const {
  return impl_->GetEncoding();
}


void FunapiUdpTransport::SetConnectTimeout(time_t timeout) {
  return impl_->SetConnectTimeout(timeout);
}


void FunapiUdpTransport::AddStartedCallback(const TransportEventHandler &handler) {
  return impl_->AddStartedCallback(handler);
}


void FunapiUdpTransport::AddStoppedCallback(const TransportEventHandler &handler) {
  return impl_->AddStoppedCallback(handler);
}


void FunapiUdpTransport::AddConnectFailedCallback(const TransportEventHandler &handler) {
  return impl_->AddConnectFailedCallback(handler);
}


void FunapiUdpTransport::AddConnectTimeoutCallback(const TransportEventHandler &handler) {
  return impl_->AddConnectTimeoutCallback(handler);
}


void FunapiUdpTransport::ResetClientPingTimeout() {
  return impl_->ResetClientPingTimeout();
}


int FunapiUdpTransport::GetSocket() {
  return impl_->GetSocket();
}


void FunapiUdpTransport::AddInitSocketCallback(const TransportEventHandler &handler) {
  return impl_->AddInitSocketCallback(handler);
}


void FunapiUdpTransport::AddCloseSocketCallback(const TransportEventHandler &handler) {
  return impl_->AddCloseSocketCallback(handler);
}


void FunapiUdpTransport::OnSocketSelect(const fd_set rset, const fd_set wset, const fd_set eset) {
    return impl_->OnSocketSelect(rset, wset, eset);
}


void FunapiUdpTransport::Update() {
  return impl_->Update();
}


void FunapiUdpTransport::SetReceivedHandler(OnReceived handler) {
  return impl_->SetReceivedHandler(handler);
}


////////////////////////////////////////////////////////////////////////////////
// FunapiHttpTransport implementation.

FunapiHttpTransport::FunapiHttpTransport (const std::string &hostname_or_ip,
  uint16_t port, bool https, FunEncoding encoding)
  : impl_(std::make_shared<FunapiHttpTransportImpl>(hostname_or_ip, port, https, encoding)) {
}


std::shared_ptr<FunapiHttpTransport> FunapiHttpTransport::create(const std::string &hostname_or_ip,
  uint16_t port, bool https, FunEncoding encoding) {
  return std::make_shared<FunapiHttpTransport>(hostname_or_ip, port, https, encoding);
}


TransportProtocol FunapiHttpTransport::GetProtocol() const {
  return impl_->GetProtocol();
}


void FunapiHttpTransport::Start() {
  impl_->Start();
}


void FunapiHttpTransport::Stop() {
  impl_->Stop();
}


void FunapiHttpTransport::SendMessage(rapidjson::Document &message) {
  impl_->SendMessage(message);
}


void FunapiHttpTransport::SendMessage(FunMessage &message) {
  impl_->SendMessage(message);
}


void FunapiHttpTransport::SendMessage(const char *body) {
  impl_->SendMessage(body);
}


bool FunapiHttpTransport::IsStarted() const {
  return impl_->IsStarted();
}


void FunapiHttpTransport::SetNetwork(std::weak_ptr<FunapiNetwork> network) {
  impl_->SetNetwork(network);
}


FunEncoding FunapiHttpTransport::GetEncoding() const {
  return impl_->GetEncoding();
}


void FunapiHttpTransport::SetConnectTimeout(time_t timeout) {
  return impl_->SetConnectTimeout(timeout);
}


void FunapiHttpTransport::AddStartedCallback(const TransportEventHandler &handler) {
  return impl_->AddStartedCallback(handler);
}


void FunapiHttpTransport::AddStoppedCallback(const TransportEventHandler &handler) {
  return impl_->AddStoppedCallback(handler);
}


void FunapiHttpTransport::AddConnectFailedCallback(const TransportEventHandler &handler) {
  return impl_->AddConnectFailedCallback(handler);
}


void FunapiHttpTransport::AddConnectTimeoutCallback(const TransportEventHandler &handler) {
  return impl_->AddConnectTimeoutCallback(handler);
}


void FunapiHttpTransport::ResetClientPingTimeout() {
  return impl_->ResetClientPingTimeout();
}


void FunapiHttpTransport::Update() {
  return impl_->Update();
}


void FunapiHttpTransport::SetReceivedHandler(OnReceived handler) {
  return impl_->SetReceivedHandler(handler);
}

}  // namespace fun
