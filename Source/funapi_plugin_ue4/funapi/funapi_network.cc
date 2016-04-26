// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "pb/network/ping_message.pb.h"
#include "funapi_plugin.h"
#include "funapi_utils.h"
#include "funapi_network.h"

namespace fun {

////////////////////////////////////////////////////////////////////////////////
// FunapiNetworkImpl implementation.

class FunapiNetworkImpl : public std::enable_shared_from_this<FunapiNetworkImpl> {
 public:
  typedef FunapiTransport::HeaderFields HeaderFields;
  typedef FunapiNetwork::MessageEventHandler MessageEventHandler;
  typedef FunapiNetwork::SessionInitHandler SessionInitHandler;
  typedef FunapiNetwork::SessionCloseHandler SessionCloseHandler;
  typedef FunapiNetwork::NotifyHandler NotifyHandler;
  typedef FunapiTransport::TransportEventHandler TransportEventHandler;

  FunapiNetworkImpl(const bool &session_reliability = false);
  ~FunapiNetworkImpl();

  void RegisterHandler(const std::string &msg_type, const MessageEventHandler &handler);

  void Start();
  void Stop();
  void Update();

  void SendMessage(const std::string &msg_type, std::string &json_string, TransportProtocol protocol);
  void SendMessage(FunMessage& message, TransportProtocol protocol);

  bool IsStarted() const;
  bool IsConnected(const TransportProtocol protocol) const;
  bool IsReliableSession() const;

  void AttachTransport(const std::shared_ptr<FunapiTransport> &transport, const std::weak_ptr<FunapiNetwork> &network);
  std::shared_ptr<FunapiTransport> GetTransport(const TransportProtocol protocol) const;
  bool HasTransport(const TransportProtocol protocol) const;

  void AddSessionInitiatedCallback(const SessionInitHandler &handler);
  void AddSessionClosedCallback(const SessionCloseHandler &handler);
  void AddMaintenanceCallback(const MessageEventHandler &handler);
  void AddStoppedAllTransportCallback(const NotifyHandler &handler);
  void AddTransportConnectFailedCallback(const TransportEventHandler &handler);
  void AddTransportDisconnectedCallback(const TransportEventHandler &handler);
  void AddTransportConnectTimeoutCallback(const TransportEventHandler &handler);
  void AddTransportStartedCallback(const TransportEventHandler &handler);
  void AddTransportClosedCallback(const TransportEventHandler &handler);

  TransportProtocol GetDefaultProtocol() const;
  void SetDefaultProtocol(const TransportProtocol protocol);

  FunEncoding GetEncoding(const TransportProtocol protocol) const;

 private:
  void CheckSessionTimeout();
  void PushTaskQueue(const std::function<void()> &task);

  void OnSessionInitiated(const std::string &session_id);
  void OnSessionClosed();

  void OnMaintenance(const TransportProtocol, const std::string &, const std::vector<uint8_t> &);
  void OnStoppedAllTransport();
  void OnTransportConnectFailed(const TransportProtocol protocol);
  void OnTransportDisconnected(const TransportProtocol protocol);
  void OnTransportConnectTimeout(const TransportProtocol protocol);
  void OnTransportStarted(const TransportProtocol protocol);
  void OnTransportClosed(const TransportProtocol protocol);

  void OnTransportReceived(const TransportProtocol protocol, const FunEncoding encoding, const HeaderFields &header, const std::vector<uint8_t> &body);
  void OnTransportStopped();
  void OnNewSession(const std::string &msg_type, const std::vector<uint8_t>&v_body);
  void OnSessionTimedout(const std::string &msg_type, const std::vector<uint8_t>&v_body);

  void OnServerPingMessage(const TransportProtocol protocol, const std::string &msg_type, const std::vector<uint8_t>&v_body);
  void OnClientPingMessage(const TransportProtocol protocol, const std::string &msg_type, const std::vector<uint8_t>&v_body);

  void Initialize();
  void Finalize();

  void SendMessage(const char *body, TransportProtocol protocol, bool priority = false);
  void SendEmptyMessage(const TransportProtocol protocol);
  bool SendClientPingMessage(const TransportProtocol protocol);
  void SendAck(const TransportProtocol protocol, const uint32_t ack);

  bool started_;
  bool session_reliability_ = false;

  // Transport-releated member variables.
  std::map<TransportProtocol, std::shared_ptr<FunapiTransport>> transports_;
  mutable std::mutex transports_mutex_;
  TransportProtocol default_protocol_ = TransportProtocol::kDefault;

  std::string session_id_;
  typedef std::map<std::string, MessageEventHandler> MessageHandlerMap;
  MessageHandlerMap message_handlers_;
  time_t last_received_;

  bool initialized_ = false;
  time_t epoch_ = 0;
  const time_t funapi_session_timeout_ = 3600;

  std::queue<std::function<void()>> tasks_queue_;
  std::mutex tasks_queue_mutex_;

  // Funapi message-related events.
  FunapiEvent<SessionInitHandler> on_session_initiated_;
  FunapiEvent<SessionCloseHandler> on_session_closed_;
  FunapiEvent<MessageEventHandler> on_maintenance_;
  FunapiEvent<NotifyHandler> on_stopped_all_transport_;
  FunapiEvent<TransportEventHandler> on_transport_connect_failed_;
  FunapiEvent<TransportEventHandler> on_transport_disconnected_;
  FunapiEvent<TransportEventHandler> on_transport_connect_timeout_;
  FunapiEvent<TransportEventHandler> on_transport_started_;
  FunapiEvent<TransportEventHandler> on_transport_closed_;
};


FunapiNetworkImpl::FunapiNetworkImpl(const bool &session_reliability)
  : session_id_(""), last_received_(0), session_reliability_(session_reliability) {
  Initialize();
}


FunapiNetworkImpl::~FunapiNetworkImpl() {
  Finalize();
  Stop();
  Update();
}


void FunapiNetworkImpl::Initialize() {
  assert(!initialized_);

  // Installs event handlers.
  message_handlers_[kNewSessionMessageType] =
    [this](const TransportProtocol &p, const std::string&s, const std::vector<uint8_t>&v) { OnNewSession(s, v); };
  message_handlers_[kSessionClosedMessageType] =
    [this](const TransportProtocol &p, const std::string&s, const std::vector<uint8_t>&v) { OnSessionTimedout(s, v); };

  // ping
  message_handlers_[kServerPingMessageType] =
    [this](const TransportProtocol &p, const std::string&s, const std::vector<uint8_t>&v) { OnServerPingMessage(p, s, v); };
  message_handlers_[kClientPingMessageType] =
    [this](const TransportProtocol &p, const std::string&s, const std::vector<uint8_t>&v) { OnClientPingMessage(p, s, v); };

  // Maintenance
  message_handlers_[kMaintenanceMessageType] =
    [this](const TransportProtocol &p, const std::string&s, const std::vector<uint8_t>&v) { OnMaintenance(p, s, v); };

  // Now ready.
  initialized_ = true;
}


void FunapiNetworkImpl::Finalize() {
  assert(initialized_);

  // All set.
  initialized_ = false;
}


void FunapiNetworkImpl::RegisterHandler(
    const std::string &msg_type, const MessageEventHandler &handler) {
  DebugUtils::Log("New handler for message type %s", msg_type.c_str());
  message_handlers_[msg_type] = handler;
}


void FunapiNetworkImpl::Start() {
  DebugUtils::Log("Starting a network module.");

  std::vector<TransportProtocol> v_protocols;

  {
    std::unique_lock<std::mutex> lock(transports_mutex_);
    for (auto iter : transports_) {
      if (!iter.second->IsStarted())
        v_protocols.push_back(iter.second->GetProtocol());
    }
  }

  for (auto protocol : v_protocols) {
    auto transport = GetTransport(protocol);
    if (transport)
      transport->Start();
  }

  // Ok. We are ready.
  started_ = true;
  last_received_ = epoch_ = time(NULL);
}


void FunapiNetworkImpl::Stop() {
  if (!started_)
    return;

  DebugUtils::Log("Stopping a network module.");

  // Turns off the flag.
  started_ = false;

  // remove session id
  if (!session_reliability_) {
    session_id_ = "";
  }

  // Then, requests the transport to stop.
  std::vector<TransportProtocol> v_protocols;

  {
    std::unique_lock<std::mutex> lock(transports_mutex_);
    for (auto iter : transports_) {
      if (iter.second->IsStarted())
        v_protocols.push_back(iter.second->GetProtocol());
    }
  }

  for (auto protocol : v_protocols) {
    auto transport = GetTransport(protocol);
    if (transport)
      transport->Stop();
  }
}

void FunapiNetworkImpl::CheckSessionTimeout() {
  // Invalidates session id if it is too stale.
  time_t now = time(NULL);
  time_t delta = funapi_session_timeout_;

  if (last_received_ != epoch_ && last_received_ + delta < now) {
    DebugUtils::Log("Session is too stale. The server might have invalidated my session. Resetting.");
    session_id_ = "";
  }
}

void FunapiNetworkImpl::SendMessage(const std::string &msg_type, std::string &json_string, TransportProtocol protocol = TransportProtocol::kDefault) {
  CheckSessionTimeout();

  rapidjson::Document body;
  body.Parse<0>(json_string.c_str());

  // Encodes a messsage type.
  if (msg_type.length() > 0) {
    rapidjson::Value msg_type_node;
    msg_type_node.SetString(rapidjson::StringRef(msg_type.c_str()));
    body.AddMember(rapidjson::StringRef(kMsgTypeBodyField), msg_type_node, body.GetAllocator());
  }

  // Encodes a session id, if any.
  if (!session_id_.empty()) {
    rapidjson::Value session_id_node;
    session_id_node.SetString(rapidjson::StringRef(session_id_.c_str()));
    body.AddMember(rapidjson::StringRef(kSessionIdBodyField), session_id_node, body.GetAllocator());
  }

  if (protocol == TransportProtocol::kDefault)
    protocol = default_protocol_;

  // Sends the manipulated JSON object through the transport.
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
  if (transport)
    transport->SendMessage(body);
}


void FunapiNetworkImpl::SendMessage(FunMessage& message, TransportProtocol protocol = TransportProtocol::kDefault) {
  CheckSessionTimeout();

  // Encodes a session id, if any.
  if (!session_id_.empty()) {
    message.set_sid(session_id_.c_str());
  }

  if (protocol == TransportProtocol::kDefault)
    protocol = default_protocol_;

  // Sends the manipulated Protobuf object through the transport.
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
  if (transport)
    transport->SendMessage(message);
}


void FunapiNetworkImpl::SendMessage(const char *body, TransportProtocol protocol = TransportProtocol::kDefault, bool priority) {
  CheckSessionTimeout();

  if (protocol == TransportProtocol::kDefault)
    protocol = default_protocol_;

  // Sends the manipulated Protobuf object through the transport.
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
  if (transport)
    transport->SendMessage(body, false, 0, priority);
}


bool FunapiNetworkImpl::IsStarted() const {
  return started_;
}


bool FunapiNetworkImpl::IsConnected(const TransportProtocol protocol = TransportProtocol::kDefault) const {
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
  if (transport)
    return transport->IsStarted();

  return false;
}


void FunapiNetworkImpl::OnTransportReceived(
  const TransportProtocol protocol, const FunEncoding encoding, const HeaderFields &header, const std::vector<uint8_t> &body) {
  // DebugUtils::Log("OnReceived invoked");

  last_received_ = time(NULL);

  std::string msg_type;
  std::string session_id;

  if (encoding == FunEncoding::kJson) {
    // Parses the given json string.
    rapidjson::Document json;
    json.Parse<0>(reinterpret_cast<char*>(const_cast<uint8_t*>(body.data())));
    assert(json.IsObject());

    if (json.HasMember(kMsgTypeBodyField)) {
      const rapidjson::Value &msg_type_node = json[kMsgTypeBodyField];
      assert(msg_type_node.IsString());
      msg_type = msg_type_node.GetString();
      json.RemoveMember(kMsgTypeBodyField);
    }

    if (json.HasMember(kSessionIdBodyField)) {
      const rapidjson::Value &session_id_node = json[kSessionIdBodyField];
      assert(session_id_node.IsString());
      session_id = session_id_node.GetString();
      json.RemoveMember(kSessionIdBodyField);
    }
  } else if (encoding == FunEncoding::kProtobuf) {
    FunMessage proto;
    proto.ParseFromArray(body.data(), static_cast<int>(body.size()));

    msg_type = proto.msgtype();
    session_id = proto.sid();
  }

  if (session_id_.empty()) {
    session_id_ = session_id;
    DebugUtils::Log("New session id: %s", session_id.c_str());
    OnSessionInitiated(session_id);
  }

  if (session_id_.compare(session_id) != 0) {
    DebugUtils::Log("Session id changed: %s => %s", session_id_.c_str(), session_id.c_str());
    session_id_ = session_id;
    OnSessionClosed();
    OnSessionInitiated(session_id);
  }

  if (session_reliability_) {
    if (msg_type.empty())
      return;
  }

  MessageHandlerMap::iterator it = message_handlers_.find(msg_type);
  if (it == message_handlers_.end()) {
    DebugUtils::Log("No handler for message '%s'. Ignoring.", msg_type.c_str());
  } else {
    MessageEventHandler handler = it->second;
    PushTaskQueue([handler, protocol, msg_type, body](){
      handler(protocol, msg_type, body);
    });
  }
}


void FunapiNetworkImpl::OnTransportStopped() {
  DebugUtils::Log("Transport terminated. Stopping. You may restart again.");
  Stop();
  OnStoppedAllTransport();
}


void FunapiNetworkImpl::OnNewSession(
  const std::string &msg_type, const std::vector<uint8_t>&v_body) {
  // ignore
}


void FunapiNetworkImpl::OnSessionTimedout(
  const std::string &msg_type, const std::vector<uint8_t>&v_body) {
  DebugUtils::Log("Session timed out. Resetting my session id. The server will send me another one next time.");

  session_id_ = "";

  OnSessionClosed();
}


void FunapiNetworkImpl::OnServerPingMessage(
  const TransportProtocol protocol, const std::string &msg_type, const std::vector<uint8_t>&v_body) {

  fun::FunEncoding encoding = GetEncoding(protocol);
  assert(encoding!=FunEncoding::kNone);

  if (encoding == FunEncoding::kJson)
  {
    std::string body(v_body.cbegin(), v_body.cend());
    SendMessage(body.c_str(), protocol);
  }

  if (encoding == FunEncoding::kProtobuf)
  {
    FunMessage msg;
    msg.ParseFromArray(v_body.data(), static_cast<int>(v_body.size()));

    SendMessage(msg.SerializeAsString().c_str(), protocol);
  }
}


void FunapiNetworkImpl::OnClientPingMessage(
  const TransportProtocol protocol, const std::string &msg_type, const std::vector<uint8_t>&v_body) {

  fun::FunEncoding encoding = GetEncoding(protocol);
  assert(encoding!=FunEncoding::kNone);

  std::string body(v_body.cbegin(), v_body.cend());
  int64_t timestamp_ms = 0;

  if (encoding == FunEncoding::kJson)
  {
    rapidjson::Document document;
    document.Parse<0>(reinterpret_cast<char*>(const_cast<uint8_t*>(v_body.data())));

    timestamp_ms = document[kPingTimestampField].GetInt64();
  }

  if (encoding == FunEncoding::kProtobuf)
  {
    FunMessage msg;
    msg.ParseFromArray(v_body.data(), static_cast<int>(v_body.size()));

    FunPingMessage ping_message = msg.GetExtension(cs_ping);
    timestamp_ms = ping_message.timestamp();
  }

  int64_t ping_time_ms = (std::chrono::system_clock::now().time_since_epoch().count() - timestamp_ms) / 1000;

  DebugUtils::Log("Receive %s ping - timestamp:%lld time=%lld ms", "TCP", timestamp_ms, ping_time_ms);
}


void FunapiNetworkImpl::Update() {
  std::function<void()> task = nullptr;
  while (true) {
    {
      std::unique_lock<std::mutex> lock(tasks_queue_mutex_);
      if (tasks_queue_.empty()) {
        break;
      }
      else {
        task = std::move(tasks_queue_.front());
        tasks_queue_.pop();
      }
    }

    task();
  }
}


void FunapiNetworkImpl::AttachTransport(const std::shared_ptr<FunapiTransport> &transport, const std::weak_ptr<FunapiNetwork> &network) {
  transport->SetReceivedHandler([this](const TransportProtocol protocol, const FunEncoding encoding, const HeaderFields &header, const std::vector<uint8_t> &body){ OnTransportReceived(protocol, encoding, header, body);
  });
  transport->SetIsReliableSessionHandler([this]() {
    return IsReliableSession();
  });
  transport->SetSendAckHandler([this](const TransportProtocol protocol, const uint32_t seq) {
    SendAck(protocol, seq);
  });

  transport->AddStartedCallback([this](const TransportProtocol protocol){
    if (session_id_.empty()) {
      SendEmptyMessage(protocol);
    }

    OnTransportStarted(protocol);
  });
  transport->AddClosedCallback([this](const TransportProtocol protocol){
    OnTransportStopped();
    OnTransportClosed(protocol);
  });
  transport->AddConnectFailedCallback([this](const TransportProtocol protocol){ OnTransportConnectFailed(protocol); });
  transport->AddConnectTimeoutCallback([this](const TransportProtocol protocol){ OnTransportConnectTimeout(protocol); });

  if (transport->GetProtocol() == TransportProtocol::kTcp) {
    transport->SetSendClientPingMessageHandler([this](const TransportProtocol protocol)->bool{ return SendClientPingMessage(protocol); });
  }

  if (HasTransport(transport->GetProtocol()))
  {
    DebugUtils::Log("AttachTransport - transport of '%d' type already exists.", static_cast<int>(transport->GetProtocol()));
    DebugUtils::Log(" You should call DetachTransport first.");
  }
  else {
    {
      std::unique_lock<std::mutex> lock(transports_mutex_);
      transports_[transport->GetProtocol()] = transport;
    }

    if (default_protocol_ == TransportProtocol::kDefault)
    {
      SetDefaultProtocol(transport->GetProtocol());
    }
  }
}


std::shared_ptr<FunapiTransport> FunapiNetworkImpl::GetTransport(const TransportProtocol protocol) const {
  std::unique_lock<std::mutex> lock(transports_mutex_);
  if (protocol == TransportProtocol::kDefault)
    return transports_.cbegin()->second;

  auto iter = transports_.find(protocol);
  if (iter != transports_.cend())
    return iter->second;

  // DebugUtils::Log("Invaild Protocol - Transport is not founded");
  return nullptr;
}


void FunapiNetworkImpl::PushTaskQueue(const std::function<void()> &task)
{
  std::unique_lock<std::mutex> lock(tasks_queue_mutex_);
  tasks_queue_.push(task);
}


void FunapiNetworkImpl::AddSessionInitiatedCallback(const SessionInitHandler &handler) {
  on_session_initiated_ += handler;
}


void FunapiNetworkImpl::AddSessionClosedCallback(const SessionCloseHandler &handler) {
  on_session_closed_ += handler;
}


void FunapiNetworkImpl::AddMaintenanceCallback(const MessageEventHandler &handler) {
  on_maintenance_ += handler;
}


void FunapiNetworkImpl::AddStoppedAllTransportCallback(const NotifyHandler &handler) {
  on_stopped_all_transport_ += handler;
}


void FunapiNetworkImpl::AddTransportConnectFailedCallback(const TransportEventHandler &handler) {
  on_transport_connect_failed_ += handler;
}


void FunapiNetworkImpl::AddTransportConnectTimeoutCallback(const TransportEventHandler &handler) {
  on_transport_connect_timeout_ += handler;
}


void FunapiNetworkImpl::AddTransportDisconnectedCallback(const TransportEventHandler &handler) {
  on_transport_disconnected_ += handler;
}


void FunapiNetworkImpl::AddTransportStartedCallback(const TransportEventHandler &handler) {
  on_transport_started_ += handler;
}


void FunapiNetworkImpl::AddTransportClosedCallback(const TransportEventHandler &handler) {
  on_transport_closed_ += handler;
}


void FunapiNetworkImpl::OnSessionInitiated(const std::string &session_id) {
  PushTaskQueue([this]() {
    on_session_initiated_(session_id_);
  });
}


void FunapiNetworkImpl::OnSessionClosed() {
  PushTaskQueue([this]() {
    on_session_closed_();
  });
}


void FunapiNetworkImpl::OnMaintenance (const TransportProtocol protocol, const std::string &msg_type, const std::vector<uint8_t> &v_body) {
  PushTaskQueue([this, protocol, msg_type, v_body]() {
    on_maintenance_(protocol, msg_type, v_body);
  });
}


void FunapiNetworkImpl::OnStoppedAllTransport() {
  PushTaskQueue([this]() {
    on_stopped_all_transport_();
  });
}


void FunapiNetworkImpl::OnTransportConnectFailed(const TransportProtocol protocol) {
  PushTaskQueue([this, protocol]() {
    on_transport_connect_failed_(protocol);
  });
}


void FunapiNetworkImpl::OnTransportConnectTimeout(const TransportProtocol protocol) {
  PushTaskQueue([this, protocol]() {
    on_transport_connect_timeout_(protocol);
  });
}


void FunapiNetworkImpl::OnTransportDisconnected(const TransportProtocol protocol) {
  PushTaskQueue([this, protocol]() {
    on_transport_disconnected_(protocol);
  });
}


void FunapiNetworkImpl::OnTransportStarted(const TransportProtocol protocol) {
  PushTaskQueue([this, protocol]() {
    on_transport_started_(protocol);
  });
}


void FunapiNetworkImpl::OnTransportClosed(const TransportProtocol protocol) {
  PushTaskQueue([this, protocol]() {
    on_transport_closed_(protocol);
  });
}


bool FunapiNetworkImpl::HasTransport(const TransportProtocol protocol) const {
  if (GetTransport(protocol))
    return true;

  return false;
}


void FunapiNetworkImpl::SetDefaultProtocol(const TransportProtocol protocol) {
  default_protocol_ = protocol;
}


FunEncoding FunapiNetworkImpl::GetEncoding(const TransportProtocol protocol) const {
  auto transport = GetTransport(protocol);
  if (transport)
    return transport->GetEncoding();

  return FunEncoding::kNone;
}


TransportProtocol FunapiNetworkImpl::GetDefaultProtocol() const {
  return default_protocol_;
}


void FunapiNetworkImpl::SendEmptyMessage(const TransportProtocol protocol) {
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
  FunEncoding encoding = transport->GetEncoding();

  if (session_id_.empty()) {
    assert(encoding!=FunEncoding::kNone);

    if (encoding == FunEncoding::kJson) {
      SendMessage("{}");
    }
    else if (encoding == FunEncoding::kProtobuf) {
      FunMessage msg;
      SendMessage(msg.SerializeAsString().c_str());
    }
  }
}


bool FunapiNetworkImpl::SendClientPingMessage(const TransportProtocol protocol) {
  assert(protocol==TransportProtocol::kTcp);

  if (session_id_.empty())
    return false;

  FunEncoding encoding = GetEncoding(protocol);
  assert(encoding!=FunEncoding::kNone);

  int64_t timestamp = std::chrono::system_clock::now().time_since_epoch().count();
  DebugUtils::Log("Send TCP ping - timestamp: %lld", timestamp);

  if (encoding == FunEncoding::kProtobuf) {
    FunMessage msg;
    msg.set_msgtype(kClientPingMessageType);
    FunPingMessage *ping_message = msg.MutableExtension(cs_ping);
    ping_message->set_timestamp(timestamp);
    // ping_message->set_data("77777777");

    if (!session_id_.empty()) {
      msg.set_sid(session_id_.c_str());
    }

    SendMessage(msg.SerializeAsString().c_str(), protocol);
  }
  else if (encoding == fun::FunEncoding::kJson) {
    rapidjson::Document msg;
    msg.SetObject();
    rapidjson::Value key(kPingTimestampField, msg.GetAllocator());
    rapidjson::Value timestamp_node(rapidjson::kNumberType);
    timestamp_node.SetInt64(timestamp);
    msg.AddMember(key, timestamp_node, msg.GetAllocator());

    // Encodes a messsage type.
    rapidjson::Value msg_type_node;
    msg_type_node.SetString(rapidjson::StringRef(kClientPingMessageType));
    msg.AddMember(rapidjson::StringRef(kMsgTypeBodyField), msg_type_node, msg.GetAllocator());

    // Encodes a session id, if any.
    if (!session_id_.empty()) {
      rapidjson::Value session_id_node;
      session_id_node.SetString(rapidjson::StringRef(session_id_.c_str()));
      msg.AddMember(rapidjson::StringRef(kSessionIdBodyField), session_id_node, msg.GetAllocator());
    }

    // Convert JSON document to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msg.Accept(writer);
    std::string json_string = buffer.GetString();

    SendMessage(json_string.c_str(), protocol);
  }

  return true;
}


void FunapiNetworkImpl::SendAck(const TransportProtocol protocol, const uint32_t ack) {
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
  if (transport == nullptr) return;

  DebugUtils::Log("TCP send ack message - ack:%d", ack);

  if (transport->GetEncoding() == FunEncoding::kJson) {
    rapidjson::Document msg;
    msg.SetObject();
    rapidjson::Value key(kAckNumberField, msg.GetAllocator());
    rapidjson::Value ack_node(rapidjson::kNumberType);
    ack_node.SetUint(ack);
    msg.AddMember(key, ack_node, msg.GetAllocator());

    if (!session_id_.empty()) {
      rapidjson::Value session_id_node;
      session_id_node.SetString(rapidjson::StringRef(session_id_.c_str()));
      msg.AddMember(rapidjson::StringRef(kSessionIdBodyField), session_id_node, msg.GetAllocator());
    }

    // Convert JSON document to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msg.Accept(writer);
    std::string json_string = buffer.GetString();

    SendMessage(json_string.c_str(), protocol, true);
  }
  else if (transport->GetEncoding() == FunEncoding::kProtobuf) {
    FunMessage msg;
    msg.set_ack(ack);

    if (!session_id_.empty()) {
      msg.set_sid(session_id_.c_str());
    }

    SendMessage(msg.SerializeAsString().c_str(), protocol, true);
  }
}


bool FunapiNetworkImpl::IsReliableSession() const {
  return session_reliability_;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiNetwork implementation.

FunapiNetwork::FunapiNetwork(bool session_reliability)
  : impl_(std::make_shared<FunapiNetworkImpl>(session_reliability)) {
}


FunapiNetwork::~FunapiNetwork() {
}


void FunapiNetwork::RegisterHandler(
    const std::string msg_type, const MessageEventHandler &handler) {
  return impl_->RegisterHandler(msg_type, handler);
}


void FunapiNetwork::Start() {
  return impl_->Start();
}


void FunapiNetwork::Stop() {
  return impl_->Stop();
}


void FunapiNetwork::SendMessage(const std::string &msg_type, std::string &json_string, TransportProtocol protocol) {
  return impl_->SendMessage(msg_type, json_string, protocol);
}


void FunapiNetwork::SendMessage(FunMessage& message, TransportProtocol protocol) {
  return impl_->SendMessage(message, protocol);
}


bool FunapiNetwork::IsStarted() const {
  return impl_->IsStarted();
}


bool FunapiNetwork::IsConnected(const TransportProtocol protocol) const {
  return impl_->IsConnected(protocol);
}


void FunapiNetwork::Update() {
  return impl_->Update();
}


void FunapiNetwork::AttachTransport(const std::shared_ptr<FunapiTransport> &transport) {
  return impl_->AttachTransport(transport, shared_from_this());
}


void FunapiNetwork::AddSessionInitiatedCallback(const SessionInitHandler &handler) {
  return impl_->AddSessionInitiatedCallback(handler);
}


void FunapiNetwork::AddSessionClosedCallback(const SessionCloseHandler &handler) {
  return impl_->AddSessionClosedCallback(handler);
}


void FunapiNetwork::AddMaintenanceCallback(const MessageEventHandler &handler) {
  return impl_->AddMaintenanceCallback(handler);
}


void FunapiNetwork::AddStoppedAllTransportCallback(const NotifyHandler &handler) {
  return impl_->AddStoppedAllTransportCallback(handler);
}


void FunapiNetwork::AddTransportConnectFailedCallback(const TransportEventHandler &handler) {
  return impl_->AddTransportConnectFailedCallback(handler);
}


void FunapiNetwork::AddTransportConnectTimeoutCallback(const TransportEventHandler &handler) {
  return impl_->AddTransportConnectTimeoutCallback(handler);
}


void FunapiNetwork::AddTransportDisconnectedCallback(const TransportEventHandler &handler) {
  return impl_->AddTransportDisconnectedCallback(handler);
}


void FunapiNetwork::AddTransportStartedCallback(const TransportEventHandler &handler) {
  return impl_->AddTransportStartedCallback(handler);
}


void FunapiNetwork::AddTransportClosedCallback(const TransportEventHandler &handler) {
  return impl_->AddTransportClosedCallback(handler);
}


std::shared_ptr<FunapiTransport> FunapiNetwork::GetTransport(const TransportProtocol protocol) const {
  return impl_->GetTransport(protocol);
}


bool FunapiNetwork::HasTransport(const TransportProtocol protocol) const {
  return impl_->HasTransport(protocol);
}


void FunapiNetwork::SetDefaultProtocol(const TransportProtocol protocol) {
  return impl_->SetDefaultProtocol(protocol);
}


bool FunapiNetwork::IsReliableSession() const {
  return impl_->IsReliableSession();
}


FunEncoding FunapiNetwork::GetEncoding(const TransportProtocol protocol) const {
  return impl_->GetEncoding(protocol);
}


TransportProtocol FunapiNetwork::GetDefaultProtocol() const {
  return impl_->GetDefaultProtocol();
}

}  // namespace fun
