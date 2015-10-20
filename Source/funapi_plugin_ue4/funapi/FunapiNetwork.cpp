// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin_ue4.h"

#include "FunapiUtils.h"
#include "FunapiNetwork.h"


namespace fun {

////////////////////////////////////////////////////////////////////////////////
// FunapiNetworkImpl implementation.

class FunapiNetworkImpl : public std::enable_shared_from_this<FunapiNetworkImpl> {
 public:
  typedef FunapiTransport::HeaderType HeaderType;
  typedef FunapiNetwork::MessageHandler MessageHandler;
  typedef FunapiNetwork::OnSessionInitiated OnSessionInitiated;
  typedef FunapiNetwork::OnSessionClosed OnSessionClosed;

  FunapiNetworkImpl(std::shared_ptr<FunapiTransport> transport, int type,
                    OnSessionInitiated on_session_initiated,
                    OnSessionClosed on_session_closed,
                    FunapiNetwork *network);

  ~FunapiNetworkImpl();

  void RegisterHandler(const std::string msg_type, const MessageHandler &handler);
  void Start();
  void Stop();
  void SendMessage(const std::string &msg_type, Json &body, const TransportProtocol protocol);
  void SendMessage(const std::string &msg_type, FJsonObject &body, const TransportProtocol protocol);
  void SendMessage(FunMessage& message, const TransportProtocol protocol);
  bool Started() const;
  bool Connected(const TransportProtocol protocol);
  void Update();
  void AttachTransport(std::shared_ptr<FunapiTransport> transport, FunapiNetwork *network);
  std::shared_ptr<FunapiTransport> GetTransport(const TransportProtocol protocol) const;
  void PushTaskQueue(std::function<void()> task);
  void PushAsyncQueue(const AsyncRequest r);

  // Funapi message-related constants.
  const std::string kMsgTypeBodyField = "_msgtype";
  const std::string kSessionIdBodyField = "_sid";
  const std::string kNewSessionMessageType = "_session_opened";
  const std::string kSessionClosedMessageType = "_session_closed";

 private:
  void OnTransportReceived(const HeaderType &header, const std::string &body);
  void OnTransportStopped();
  void OnNewSession(const std::string &msg_type, const std::vector<uint8_t>&v_body);
  void OnSessionTimedout(const std::string &msg_type, const std::vector<uint8_t>&v_body);

  void Initialize();
  void Finalize();
  int AsyncQueueThreadProc();
  static size_t HttpResponseCb(void *data, size_t size, size_t count, void *cb);

  bool started_;
  int encoding_type_;
  std::map<TransportProtocol, std::shared_ptr<FunapiTransport>> transports_;
  OnSessionInitiated on_session_initiated_;
  OnSessionClosed on_session_closed_;
  std::string session_id_;
  typedef std::map<std::string, MessageHandler> MessageHandlerMap;
  MessageHandlerMap message_handlers_;
  time_t last_received_;
  std::mutex mutex_transports_;

  bool initialized_ = false;
  time_t epoch_ = 0;
  const time_t funapi_session_timeout_ = 3600;

  typedef std::list<AsyncRequest> AsyncQueue;
  AsyncQueue async_queue_;

  bool async_thread_run_ = false;
  std::thread async_queue_thread_;
  std::mutex async_queue_mutex_;
  std::condition_variable_any async_queue_cond_;

  std::queue<std::function<void()>> tasks_queue_;
  std::mutex tasks_queue_mutex_;
};



FunapiNetworkImpl::FunapiNetworkImpl(std::shared_ptr<FunapiTransport> transport, int type,
                                     OnSessionInitiated on_session_initiated,
                                     OnSessionClosed on_session_closed,
                                     FunapiNetwork *network)
    : started_(false), encoding_type_(type),
      on_session_initiated_(on_session_initiated),
      on_session_closed_(on_session_closed),
      session_id_(""), last_received_(0) {
  AttachTransport(transport, network);

  Initialize();
}


FunapiNetworkImpl::~FunapiNetworkImpl() {
  Finalize();

  message_handlers_.clear();

  {
    std::unique_lock<std::mutex> lock(mutex_transports_);
    transports_.clear();
  }
}

void FunapiNetworkImpl::Initialize() {
  assert(!initialized);

  // Creates a thread to handle async operations.
  async_thread_run_ = true;
  async_queue_thread_ = std::thread([this](){ AsyncQueueThreadProc(); });

  // Now ready.
  initialized_ = true;
}

void FunapiNetworkImpl::Finalize() {
  assert(initialized);

  // Terminates the thread for async operations.
  async_thread_run_ = false;
  async_queue_cond_.notify_all();
  if (async_queue_thread_.joinable())
    async_queue_thread_.join();

  // All set.
  initialized_ = false;
}

////////////////////////////////////////////////////////////////////////////////
// Asynchronous operation related..
int FunapiNetworkImpl::AsyncQueueThreadProc() {
  LOG("Thread for async operation has been created.");

  while (async_thread_run_) {
    // Waits until we have something to process.
    AsyncQueue work_queue;
    {
      std::unique_lock<std::mutex> lock(async_queue_mutex_);

      while (async_thread_run_ && async_queue_.empty()) {
        async_queue_cond_.wait(async_queue_mutex_);
      }

      // Moves element to a worker queue and releaes the mutex
      // for contention prevention.
      work_queue.swap(async_queue_);
    }

    fd_set rset, wset;
    FD_ZERO(&rset);
    FD_ZERO(&wset);

    // Makes fd_sets for select().
    int max_fd = -1;
    for (AsyncQueue::const_iterator i = work_queue.cbegin();
      i != work_queue.cend(); ++i) {

      switch (i->type_) {
      case AsyncRequest::kConnect:
      case AsyncRequest::kSend:
      case AsyncRequest::kSendTo:
        //LOG("Checking " << i->type_ << " with fd [" << i->sock_ << "]");
        FD_SET(i->sock_, &wset);
        max_fd = std::max(max_fd, i->sock_);
        break;
      case AsyncRequest::kReceive:
      case AsyncRequest::kReceiveFrom:
        //LOG("Checking " << i->type_ << " with fd [" << i->sock_ << "]");
        FD_SET(i->sock_, &rset);
        max_fd = std::max(max_fd, i->sock_);
        break;
      case AsyncRequest::kWebRequest:
        break;
      default:
        assert(false);
        break;
      }
    }

    // Waits until some events happen to fd in the sets.
    if (max_fd != -1) {
      struct timeval timeout = { 0, 1 };
      int r = select(max_fd + 1, &rset, &wset, NULL, &timeout);

      // Some fd can be invalidated if other thread closed.
      assert(r >= 0 || (r < 0 && errno == EBADF));
      // LOG("woke up: " << r);
    }

    // Checks if the fd is ready.
    for (AsyncQueue::iterator i = work_queue.begin(); i != work_queue.end();) {
      bool remove_request = true;
      // Ready. Handles accordingly.
      switch (i->type_) {
      case AsyncRequest::kConnect:
        if (!FD_ISSET(i->sock_, &wset)) {
          remove_request = false;
        }
        else {
          int e;
          socklen_t e_size = sizeof(e);
          int r = getsockopt(i->sock_, SOL_SOCKET, SO_ERROR, (char *)&e, &e_size);
          if (r == 0) {
            i->connect_.callback_(e);
          }
          else {
            assert(r < 0 && errno == EBADF);
          }
        }
        break;
      case AsyncRequest::kSend:
        if (!FD_ISSET(i->sock_, &wset)) {
          remove_request = false;
        }
        else {
          ssize_t nSent =
            send(i->sock_, (char *)i->send_.buf_ + i->send_.buf_offset_,
            i->send_.buf_len_ - i->send_.buf_offset_, 0);
          if (nSent < 0) {
            i->send_.callback_(nSent);
          }
          else if (nSent + i->send_.buf_offset_ == i->send_.buf_len_) {
            i->send_.callback_(i->send_.buf_len_);
          }
          else {
            i->send_.buf_offset_ += nSent;
            remove_request = false;
          }
        }
        break;
      case AsyncRequest::kReceive:
        if (!FD_ISSET(i->sock_, &rset)) {
          remove_request = false;
        }
        else {
          ssize_t nRead =
            recv(i->sock_, (char *)i->recv_.buf_, i->recv_.capacity_, 0);
          i->recv_.callback_(nRead);
        }
        break;
      case AsyncRequest::kSendTo:
        if (!FD_ISSET(i->sock_, &wset)) {
          remove_request = false;
        }
        else {
          socklen_t len = sizeof(i->sendto_.endpoint_);
          ssize_t nSent =
            sendto(i->sock_, (char *)i->sendto_.buf_ + i->sendto_.buf_offset_,
            i->sendto_.buf_len_ - i->sendto_.buf_offset_, 0,
            (struct sockaddr *)&i->sendto_.endpoint_, len);
          if (nSent < 0) {
            i->sendto_.callback_(nSent);
          }
          else if (nSent + i->sendto_.buf_offset_ == i->sendto_.buf_len_) {
            i->sendto_.callback_(i->sendto_.buf_len_);
          }
          else {
            i->sendto_.buf_offset_ += nSent;
            remove_request = false;
          }
        }
        break;
      case AsyncRequest::kReceiveFrom:
        if (!FD_ISSET(i->sock_, &rset)) {
          remove_request = false;
        }
        else {
          socklen_t len = sizeof(i->recvfrom_.endpoint_);
          ssize_t nRead =
            recvfrom(i->sock_, (char *)i->recvfrom_.buf_, i->recvfrom_.capacity_, 0,
            (struct sockaddr *)&i->recvfrom_.endpoint_, &len);
          i->recvfrom_.callback_(nRead);
        }
        break;
      case AsyncRequest::kWebRequest:
        CURL *ctx = curl_easy_init();
        if (ctx == NULL) {
          LOG("Unable to initialize cURL interface.");
          break;
        }

        i->web_request_.request_callback_(kWebRequestStart);

        struct curl_slist *chunk = NULL;
        chunk = curl_slist_append(chunk, i->web_request_.header_.c_str());
        curl_easy_setopt(ctx, CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(ctx, CURLOPT_URL, i->web_request_.url_.c_str());
        curl_easy_setopt(ctx, CURLOPT_POST, 1L);
        curl_easy_setopt(ctx, CURLOPT_POSTFIELDS, i->web_request_.body_.data());
        curl_easy_setopt(ctx, CURLOPT_POSTFIELDSIZE, i->web_request_.body_len_);
        curl_easy_setopt(ctx, CURLOPT_HEADERDATA, &i->web_request_.receive_header_);
        curl_easy_setopt(ctx, CURLOPT_WRITEDATA, &i->web_request_.receive_body_);
        curl_easy_setopt(ctx, CURLOPT_WRITEFUNCTION, &FunapiNetworkImpl::HttpResponseCb);

        CURLcode res = curl_easy_perform(ctx);
        if (res != CURLE_OK) {
          LOG1("Error from cURL: %s", *FString(curl_easy_strerror(res)));
          assert(false);
        }
        else {
          i->web_request_.request_callback_(kWebRequestEnd);
        }

        curl_easy_cleanup(ctx);
        curl_slist_free_all(chunk);
        break;
      }

      // If we should keep the fd, puts it back to the queue.
      if (remove_request) {
        AsyncQueue::iterator to_delete = i++;
        work_queue.erase(to_delete);
      }
      else {
        ++i;
      }
    }

    // Puts back requests that requires more work.
    // We should respect the order.
    {
      std::unique_lock<std::mutex> lock(async_queue_mutex_);
      async_queue_.splice(async_queue_.cbegin(), work_queue);
    }
  }

  LOG("Thread for async operation is terminating.");
  return 0;
}


size_t FunapiNetworkImpl::HttpResponseCb(void *data, size_t size, size_t count, void *cb) {
  AsyncWebResponseCallback *callback = (AsyncWebResponseCallback*)(cb);
  if (callback != NULL)
    (*callback)(data, size * count);
  return size * count;
}


void FunapiNetworkImpl::RegisterHandler(
    const std::string msg_type, const MessageHandler &handler) {
  LOG1("New handler for message type %s", *FString(msg_type.c_str()));
  message_handlers_[msg_type] = handler;
}


void FunapiNetworkImpl::Start() {
  // Installs event handlers.
  message_handlers_[kNewSessionMessageType] =
    [this](const std::string&s, const std::vector<uint8_t>&v) { OnNewSession(s, v); };
  message_handlers_[kSessionClosedMessageType] =
    [this](const std::string&s, const std::vector<uint8_t>&v) { OnSessionTimedout(s, v); };

  // Then, asks the transport to work.
  LOG("Starting a network module.");
  {
    std::unique_lock<std::mutex> lock(mutex_transports_);
    for (auto iter : transports_) {
      if (!iter.second->Started())
        iter.second->Start();
    }
  }

  // Ok. We are ready.
  started_ = true;
  last_received_ = epoch_ = time(NULL);
}


void FunapiNetworkImpl::Stop() {
  LOG("Stopping a network module.");

  if (!started_)
    return;

  // Turns off the flag.
  started_ = false;

  // Then, requests the transport to stop.
  {
    std::unique_lock<std::mutex> lock(mutex_transports_);
    for (auto iter : transports_) {
      if (iter.second->Started())
        iter.second->Stop();
    }
  }
}


void FunapiNetworkImpl::SendMessage(const std::string &msg_type, Json &body, const TransportProtocol protocol = TransportProtocol::kDefault) {
  // Invalidates session id if it is too stale.
  time_t now = time(NULL);
  time_t delta = funapi_session_timeout_;

  if (last_received_ != epoch_ && last_received_ + delta < now) {
    LOG("Session is too stale. The server might have invalidated my session. Resetting.");
    session_id_ = "";
  }

  // Encodes a messsage type.
  rapidjson::Value msg_type_node;
  msg_type_node = msg_type.c_str();
  body.AddMember(kMsgTypeBodyField.c_str(), msg_type_node, body.GetAllocator());

  // Encodes a session id, if any.
  if (!session_id_.empty()) {
    rapidjson::Value session_id_node;
    session_id_node = session_id_.c_str();
    body.AddMember(kSessionIdBodyField.c_str(), session_id_node, body.GetAllocator());
  }

  // Sends the manipulated JSON object through the transport.
  {
    std::unique_lock<std::mutex> lock(mutex_transports_);
    std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
    if (transport) {
      transport->SendMessage(body);
    }
    else {
      LOG("Invaild Protocol - Transport is not founded");
    }
  }
}


void FunapiNetworkImpl::SendMessage(const std::string &msg_type, FJsonObject &body, const TransportProtocol protocol = TransportProtocol::kDefault) {
  // Invalidates session id if it is too stale.
  time_t now = time(NULL);
  time_t delta = funapi_session_timeout_;

  if (last_received_ != epoch_ && last_received_ + delta < now) {
    LOG("Session is too stale. The server might have invalidated my session. Resetting.");
    session_id_ = "";
  }

  body.SetStringField(FString(kMsgTypeBodyField.c_str()), FString(msg_type.c_str()));

  // Encodes a session id, if any.
  if (!session_id_.empty()) {
    body.SetStringField(FString(kSessionIdBodyField.c_str()), FString(session_id_.c_str()));
  }

  // Sends the manipulated JSON object through the transport.
  {
    std::unique_lock<std::mutex> lock(mutex_transports_);
    std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
    if (transport) {
      transport->SendMessage(body);
    }
    else {
      LOG("Invaild Protocol - Transport is not founded");
    }
  }
}


void FunapiNetworkImpl::SendMessage(FunMessage& message, const TransportProtocol protocol = TransportProtocol::kDefault) {
  // Invalidates session id if it is too stale.
  time_t now = time(NULL);
  time_t delta = funapi_session_timeout_;

  if (last_received_ != epoch_ && last_received_ + delta < now) {
    LOG("Session is too stale. The server might have invalidated my session. Resetting.");
    session_id_ = "";
  }

  // Encodes a session id, if any.
  if (!session_id_.empty()) {
    message.set_sid(session_id_.c_str());
  }

  // Sends the manipulated Protobuf object through the transport.
  {
    std::unique_lock<std::mutex> lock(mutex_transports_);
    std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
    if (transport) {
      transport->SendMessage(message);
    }
    else {
      LOG("Invaild Protocol - Transport is not founded");
    }
  }
}


bool FunapiNetworkImpl::Started() const {
  return started_;
}


bool FunapiNetworkImpl::Connected(TransportProtocol protocol = TransportProtocol::kDefault) {
  std::unique_lock<std::mutex> lock(mutex_transports_);
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);

  if (transport)
    return transport->Started();

  return false;
}


void FunapiNetworkImpl::OnTransportReceived(
    const HeaderType &header, const std::string &body) {
  LOG("OnReceived invoked");

  last_received_ = time(NULL);

  std::string msg_type;
  std::string session_id;

  if (encoding_type_ == kJsonEncoding) {
    // Parses the given json string.
    Json json;
    json.Parse<0>(body.c_str());
    assert(json.IsObject());

    const rapidjson::Value &msg_type_node = json[kMsgTypeBodyField.c_str()];
    assert(msg_type_node.IsString());
    msg_type = msg_type_node.GetString();
    json.RemoveMember(kMsgTypeBodyField.c_str());

    const rapidjson::Value &session_id_node = json[kSessionIdBodyField.c_str()];
    assert(session_id_node.IsString());
    session_id = session_id_node.GetString();
    json.RemoveMember(kSessionIdBodyField.c_str());

  } else if (encoding_type_ == kProtobufEncoding) {
    FunMessage proto;
    proto.ParseFromString(body);

    msg_type = proto.msgtype();
    session_id = proto.sid();
  }

  if (session_id_.empty()) {
    session_id_ = session_id;
    LOG1("New session id: %s", *FString(session_id.c_str()));
    on_session_initiated_(session_id_);
  }

  if (session_id_ != session_id) {
    LOG2("Session id changed: %s => %s", *FString(session_id_.c_str()), *FString(session_id.c_str()));
    session_id_ = session_id;
    on_session_closed_();
    on_session_initiated_(session_id_);
  }

  MessageHandlerMap::iterator it = message_handlers_.find(msg_type);
  if (it == message_handlers_.end()) {
    LOG1("No handler for message '%s'. Ignoring.", *FString(msg_type.c_str()));
  } else {
    std::vector<uint8_t> v;
    std::copy(body.cbegin(), body.cend(), std::back_inserter(v));
    v.push_back('\0');
    it->second(msg_type, v);
  }
}


void FunapiNetworkImpl::OnTransportStopped() {
  LOG("Transport terminated. Stopping. You may restart again.");
  Stop();
}


void FunapiNetworkImpl::OnNewSession(
  const std::string &msg_type, const std::vector<uint8_t>&v_body) {
  // ignore
}


void FunapiNetworkImpl::OnSessionTimedout(
  const std::string &msg_type, const std::vector<uint8_t>&v_body) {
  LOG("Session timed out. Resetting my session id. The server will send me another one next time.");

  session_id_ = "";
  on_session_closed_();
}

void FunapiNetworkImpl::Update() {
  std::unique_lock<std::mutex> lock(tasks_queue_mutex_);
  while (!tasks_queue_.empty())
  {
    (tasks_queue_.front())();
    tasks_queue_.pop();
  }
}

void FunapiNetworkImpl::AttachTransport(std::shared_ptr<FunapiTransport> transport, FunapiNetwork *network) {
  transport->RegisterEventHandlers(
    [this](const HeaderType &header, const std::string &body){ OnTransportReceived(header, body); },
    [this](){ OnTransportStopped(); });
  transport->SetNetwork(network);

  {
    std::unique_lock<std::mutex> lock(mutex_transports_);
    if (GetTransport(transport->Protocol()))
    {
      LOG1("AttachTransport - transport of '%d' type already exists.", static_cast<int>(transport->Protocol()));
      LOG(" You should call DetachTransport first.");
    } else {
      transports_[transport->Protocol()] = transport;
    }
  }
}

// The caller must lock mutex_transports_ before call this function.
std::shared_ptr<FunapiTransport> FunapiNetworkImpl::GetTransport(const TransportProtocol protocol) const {
  if (protocol == TransportProtocol::kDefault) {
    return transports_.cbegin()->second;
  }

  auto iter = transports_.find(protocol);
  if (iter != transports_.cend())
    return iter->second;

  return nullptr;
}


void FunapiNetworkImpl::PushTaskQueue(const std::function<void()> task)
{
  std::unique_lock<std::mutex> lock(tasks_queue_mutex_);
  tasks_queue_.push(task);
}


void FunapiNetworkImpl::PushAsyncQueue(const AsyncRequest r)
{
  std::unique_lock<std::mutex> lock(async_queue_mutex_);
  async_queue_.push_back(r);
  async_queue_cond_.notify_one();
}

////////////////////////////////////////////////////////////////////////////////
// FunapiNetwork implementation.

void FunapiNetwork::Initialize(time_t session_timeout) {
  LOG("This will be deprecated");
}


void FunapiNetwork::Finalize() {
  LOG("This will be deprecated");
}


FunapiNetwork::FunapiNetwork(
    std::shared_ptr<FunapiTransport> transport, int type,
    const OnSessionInitiated &on_session_initiated,
    const OnSessionClosed &on_session_closed)
      : impl_(std::make_shared<FunapiNetworkImpl>(transport, type,
        on_session_initiated, on_session_closed, this)) {
}


FunapiNetwork::~FunapiNetwork() {
}


void FunapiNetwork::RegisterHandler(
    const std::string msg_type, const MessageHandler &handler) {
  return impl_->RegisterHandler(msg_type, handler);
}


void FunapiNetwork::Start() {
  return impl_->Start();
}


void FunapiNetwork::Stop() {
  return impl_->Stop();
}


void FunapiNetwork::SendMessage(const std::string &msg_type, Json &body, const TransportProtocol protocol) {
  return impl_->SendMessage(msg_type, body, protocol);
}


void FunapiNetwork::SendMessage(const std::string &msg_type, FJsonObject &body, const TransportProtocol protocol) {
  return impl_->SendMessage(msg_type, body, protocol);
}


void FunapiNetwork::SendMessage(FunMessage& message, const TransportProtocol protocol) {
  return impl_->SendMessage(message, protocol);
}


bool FunapiNetwork::Started() const {
  return impl_->Started();
}


bool FunapiNetwork::Connected(const TransportProtocol protocol) const {
  return impl_->Connected(protocol);
}

void FunapiNetwork::Update() {
  return impl_->Update();
}

void FunapiNetwork::AttachTransport(std::shared_ptr<FunapiTransport> transport) {
  return impl_->AttachTransport(transport, this);
}

void FunapiNetwork::PushTaskQueue(const std::function<void()> task)
{
  return impl_->PushTaskQueue(task);
}

void FunapiNetwork::PushAsyncQueue(const AsyncRequest r)
{
  return impl_->PushAsyncQueue(r);
}

}  // namespace fun
