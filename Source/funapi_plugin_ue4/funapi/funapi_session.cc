// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "network/ping_message.pb.h"
#include "service/redirect_message.pb.h"
#include "funapi_plugin.h"
#include "funapi_utils.h"
#include "funapi_tasks.h"
#include "funapi_session.h"
#include "funapi_http.h"

namespace fun {

////////////////////////////////////////////////////////////////////////////////
// FunapiSessionImpl implementation.

class FunapiSessionImpl : public std::enable_shared_from_this<FunapiSessionImpl> {
 public:
  typedef FunapiTransport::HeaderFields HeaderFields;
  typedef std::function<void(const TransportProtocol, const std::string &, const std::vector<uint8_t> &)> MessageEventHandler;
  typedef std::function<void(const std::string &)> SessionInitHandler;
  typedef std::function<void()> SessionCloseHandler;
  typedef std::function<void()> NotifyHandler;

  typedef FunapiSession::TransportEventHandler TransportEventHandler;
  typedef FunapiSession::SessionEventHandler SessionEventHandler;

  typedef FunapiSession::ProtobufRecvHandler ProtobufRecvHandler;
  typedef FunapiSession::JsonRecvHandler JsonRecvHandler;

  typedef FunapiSession::RecvTimeoutHandler RecvTimeoutHandler;

  typedef FunapiSession::TransportOptionHandler TransportOptionHandler;

  FunapiSessionImpl() = delete;
  FunapiSessionImpl(const char* hostname_or_ip, bool reliability = false);
  ~FunapiSessionImpl();

  void Connect(const std::weak_ptr<FunapiSession>& session, const TransportProtocol protocol, int port, FunEncoding encoding, std::shared_ptr<FunapiTransportOption> option = nullptr);
  void Connect(const TransportProtocol protocol);
  void Close();
  void Close(const TransportProtocol protocol);

  void Update();

  void SendMessage(const std::string &msg_type, std::string &json_string, TransportProtocol protocol);
  void SendMessage(FunMessage& message, TransportProtocol protocol);

  void AddSessionEventCallback(const SessionEventHandler &handler);
  void AddTransportEventCallback(const TransportEventHandler &handler);

  bool IsConnected(const TransportProtocol protocol) const;
  bool IsReliableSession() const;

  std::shared_ptr<FunapiTransport> GetTransport(const TransportProtocol protocol) const;
  bool HasTransport(const TransportProtocol protocol) const;

  void AddProtobufRecvCallback(const ProtobufRecvHandler &handler);
  void AddJsonRecvCallback(const JsonRecvHandler &handler);

  void AddSessionInitiatedCallback(const SessionInitHandler &handler);
  void AddSessionClosedCallback(const SessionCloseHandler &handler);
  void AddMaintenanceCallback(const MessageEventHandler &handler);
  void AddTransportConnectFailedCallback(const FunapiTransport::TransportEventHandler &handler);
  void AddTransportDisconnectedCallback(const FunapiTransport::TransportEventHandler &handler);
  void AddTransportConnectTimeoutCallback(const FunapiTransport::TransportEventHandler &handler);
  void AddTransportStartedCallback(const FunapiTransport::TransportEventHandler &handler);
  void AddTransportClosedCallback(const FunapiTransport::TransportEventHandler &handler);

  TransportProtocol GetDefaultProtocol() const;
  void SetDefaultProtocol(const TransportProtocol protocol);

  FunEncoding GetEncoding(const TransportProtocol protocol) const;

  void AddRecvTimeoutCallback(const RecvTimeoutHandler &handler);
  void SetRecvTimeout(const std::string &msg_type, const int seconds);
  void EraseRecvTimeout(const std::string &msg_type);

  void SetTransportOptionCallback(const TransportOptionHandler &handler);

 private:
  void Start();
  bool IsStarted() const;
  bool IsRedirecting() const;

  void RegisterHandler(const std::string &msg_type, const MessageEventHandler &handler);

  std::string GetSessionId();
  void SetSessionId(const std::string &session_id);

  void AttachTransport(const std::shared_ptr<FunapiTransport> &transport);

  void PushTaskQueue(const std::function<bool()> &task);

  void OnSessionOpen(const TransportProtocol protocol, const std::string &msg_type, const std::vector<uint8_t>&v_body);
  void OnSessionClose(const TransportProtocol protocol, const std::string &msg_type, const std::vector<uint8_t>&v_body);

  void OnProtobufRecv(const TransportProtocol protocol, const FunMessage &message);
  void OnJsonRecv(const TransportProtocol protocol, const std::string &msg_type, const std::string &json_string);

  void OnSessionEvent(const TransportProtocol protocol,
                      const SessionEventType type,
                      const std::string &session_id,
                      const std::shared_ptr<FunapiError> &error);
  void OnTransportEvent(const TransportProtocol protocol, const TransportEventType type);

  void OnMaintenance(const TransportProtocol, const std::string &, const std::vector<uint8_t> &);

  void OnTransportConnectFailed(const TransportProtocol protocol);
  void OnTransportDisconnected(const TransportProtocol protocol);
  void OnTransportConnectTimeout(const TransportProtocol protocol);
  void OnTransportStarted(const TransportProtocol protocol);
  void OnTransportClosed(const TransportProtocol protocol);

  void OnTransportReceived(const TransportProtocol protocol, const FunEncoding encoding, const HeaderFields &header, const std::vector<uint8_t> &body);
  void OnTransportStopped();

  void OnServerPingMessage(const TransportProtocol protocol, const std::string &msg_type, const std::vector<uint8_t>&v_body);
  void OnClientPingMessage(const TransportProtocol protocol, const std::string &msg_type, const std::vector<uint8_t>&v_body);

  void OnRedirectMessage(const TransportProtocol protocol, const std::string &msg_type, const std::vector<uint8_t>&v_body);
  void OnRedirectConnectMessage(const TransportProtocol protocol, const std::string &msg_type, const std::vector<uint8_t>&v_body);

  void Initialize();
  void Finalize();

  void SendMessage(const std::string &body, TransportProtocol protocol, bool priority = false);
  void SendEmptyMessage(const TransportProtocol protocol);
  bool SendClientPingMessage(const TransportProtocol protocol);
  void SendAck(const TransportProtocol protocol, const uint32_t ack);
  void SendRedirectConenectMessage(const TransportProtocol protocol, const std::string &token);

  bool UseSodium(const TransportProtocol protocol);

  bool started_;
  bool session_reliability_ = false;

  // std::map<TransportProtocol, std::shared_ptr<FunapiTransport>> transports_;
  std::vector<std::shared_ptr<FunapiTransport>> transports_;
  mutable std::mutex transports_mutex_;
  TransportProtocol default_protocol_ = TransportProtocol::kDefault;

  std::string session_id_;
  std::mutex session_id_mutex_;

  typedef std::unordered_map<std::string, MessageEventHandler> MessageHandlerMap;
  MessageHandlerMap message_handlers_;
  time_t last_received_ = 0;

  bool initialized_ = false;
  time_t epoch_ = 0;
  const time_t funapi_session_timeout_ = 3600;

  std::shared_ptr<FunapiTasks> tasks_;

  std::vector<TransportProtocol> connect_protocols_;

  FunapiEvent<SessionInitHandler> on_session_initiated_;
  FunapiEvent<SessionCloseHandler> on_session_closed_;
  FunapiEvent<MessageEventHandler> on_maintenance_;
  FunapiEvent<FunapiTransport::TransportEventHandler> on_transport_connect_failed_;
  FunapiEvent<FunapiTransport::TransportEventHandler> on_transport_disconnected_;
  FunapiEvent<FunapiTransport::TransportEventHandler> on_transport_connect_timeout_;
  FunapiEvent<FunapiTransport::TransportEventHandler> on_transport_started_;
  FunapiEvent<FunapiTransport::TransportEventHandler> on_transport_closed_;

  FunapiEvent<ProtobufRecvHandler> on_protobuf_recv_;
  FunapiEvent<JsonRecvHandler> on_json_recv_;

  FunapiEvent<SessionEventHandler> on_session_event_;
  FunapiEvent<TransportEventHandler> on_transport_event_;

  FunapiEvent<RecvTimeoutHandler> on_recv_timeout_;

  std::string hostname_or_ip_;
  std::weak_ptr<FunapiSession> session_;

  std::vector<TransportProtocol> v_protocols_ = { TransportProtocol::kTcp, TransportProtocol::kHttp, TransportProtocol::kUdp };

  std::unordered_map<std::string, std::shared_ptr<FunapiTimer>> m_recv_timeout_;
  std::mutex m_recv_timeout_mutex_;

  void CheckRecvTimeout();
  void OnRecvTimeout(const std::string &msg_type);

  std::string token_;

  std::shared_ptr<FunapiTcpTransportOption> tcp_option_ = nullptr;
  std::shared_ptr<FunapiUdpTransportOption> udp_option_ = nullptr;
  std::shared_ptr<FunapiHttpTransportOption> http_option_ = nullptr;

  TransportOptionHandler transport_option_handler_ = nullptr;
};


FunapiSessionImpl::FunapiSessionImpl(const char* hostname_or_ip,
                                     const bool reliability)
  : session_reliability_(reliability), session_id_(""), hostname_or_ip_(hostname_or_ip), token_("") {
  Initialize();
}


FunapiSessionImpl::~FunapiSessionImpl() {
  Finalize();
  Close();
  Update();
}


void FunapiSessionImpl::Initialize() {
  assert(!initialized_);

  transports_.resize(3);

  // Installs event handlers.
  // session
  message_handlers_[kSessionOpenedMessageType] =
    [this](const TransportProtocol &p, const std::string&s, const std::vector<uint8_t>&v) { OnSessionOpen(p, s, v); };
  message_handlers_[kSessionClosedMessageType] =
    [this](const TransportProtocol &p, const std::string&s, const std::vector<uint8_t>&v) { OnSessionClose(p, s, v); };

  // ping
  message_handlers_[kServerPingMessageType] =
    [this](const TransportProtocol &p, const std::string&s, const std::vector<uint8_t>&v) { OnServerPingMessage(p, s, v); };
  message_handlers_[kClientPingMessageType] =
    [this](const TransportProtocol &p, const std::string&s, const std::vector<uint8_t>&v) { OnClientPingMessage(p, s, v); };

  // redirect
  message_handlers_[kRedirectMessageType] =
    [this](const TransportProtocol &p, const std::string&s, const std::vector<uint8_t>&v) { OnRedirectMessage(p, s, v); };
  message_handlers_[kRedirectConnectMessageType] =
    [this](const TransportProtocol &p, const std::string&s, const std::vector<uint8_t>&v) { OnRedirectConnectMessage(p, s, v); };

  tasks_ = FunapiTasks::Create();

  // Now ready.
  initialized_ = true;
}


void FunapiSessionImpl::Finalize() {
  assert(initialized_);

  // All set.
  initialized_ = false;
}


void FunapiSessionImpl::Connect(const std::weak_ptr<FunapiSession>& session, const TransportProtocol protocol, int port, FunEncoding encoding, std::shared_ptr<FunapiTransportOption> option) {
  session_ = session;

  if (HasTransport(protocol))
  {
    // DebugUtils::Log("AttachTransport - transport of '%d' type already exists.", static_cast<int>(transport->GetProtocol()));
    // DebugUtils::Log(" You should call DetachTransport first.");
    if (!IsConnected(protocol)) {
      Connect(protocol);
    }
  }
  else {
    std::shared_ptr<FunapiTransport> transport = nullptr;

    if (IsRedirecting() == false && option == nullptr && transport_option_handler_) {
      option = transport_option_handler_(protocol, "");
    }

    if (protocol == TransportProtocol::kTcp) {
      transport = FunapiTcpTransport::Create(hostname_or_ip_, static_cast<uint16_t>(port), encoding);

      if (option) {
        tcp_option_ = std::static_pointer_cast<FunapiTcpTransportOption>(option);
        auto tcp_transport = std::static_pointer_cast<FunapiTcpTransport>(transport);

        tcp_transport->SetAutoReconnect(tcp_option_->GetAutoReconnect());
        tcp_transport->SetEnablePing(tcp_option_->GetEnablePing());
        tcp_transport->SetDisableNagle(tcp_option_->GetDisableNagle());
        tcp_transport->SetConnectTimeout(tcp_option_->GetConnectTimeout());
        tcp_transport->SetSequenceNumberValidation(tcp_option_->GetSequenceNumberValidation());
        tcp_transport->SetEncryptionType(tcp_option_->GetEncryptionType(), tcp_option_->GetPublicKey());
      }
    }
    else if (protocol == fun::TransportProtocol::kUdp) {
      transport = fun::FunapiUdpTransport::Create(hostname_or_ip_, static_cast<uint16_t>(port), encoding);

      if (option) {
        udp_option_ = std::static_pointer_cast<FunapiUdpTransportOption>(option);
        auto udp_transport = std::static_pointer_cast<FunapiUdpTransport>(transport);

        udp_transport->SetEncryptionType(udp_option_->GetEncryptionType());
      }
    }
    else if (protocol == fun::TransportProtocol::kHttp) {
      bool https = false;
      if (option) {
        https = std::static_pointer_cast<FunapiHttpTransportOption>(option)->GetUseHttps();
      }

      transport = fun::FunapiHttpTransport::Create(hostname_or_ip_, static_cast<uint16_t>(port), https, encoding);

      if (option) {
        http_option_ = std::static_pointer_cast<FunapiHttpTransportOption>(option);
        auto http_transport = std::static_pointer_cast<FunapiHttpTransport>(transport);

        http_transport->SetSequenceNumberValidation(http_option_->GetSequenceNumberValidation());
        http_transport->SetEncryptionType(http_option_->GetEncryptionType());
        http_transport->SetCACertFilePath(http_option_->GetCACertFilePath());
      }
    }

    AttachTransport(transport);

    tasks_->Set([this]()->bool {
      Start();
      return true;
    });
  }
}


void FunapiSessionImpl::Connect(const TransportProtocol protocol) {
  if (auto transport = GetTransport(protocol)) {
    started_ = true;

    if (protocol == TransportProtocol::kTcp) {
      std::static_pointer_cast<FunapiTcpTransport>(transport)->SetEncryptionType(tcp_option_->GetEncryptionType(), tcp_option_->GetPublicKey());
    }
    else if (protocol == TransportProtocol::kUdp) {
      std::static_pointer_cast<FunapiUdpTransport>(transport)->SetEncryptionType(udp_option_->GetEncryptionType());
    }
    else if (protocol == TransportProtocol::kHttp) {
      std::static_pointer_cast<FunapiHttpTransport>(transport)->SetEncryptionType(http_option_->GetEncryptionType());
    }

    transport->Start();
  }
}


void FunapiSessionImpl::RegisterHandler(
    const std::string &msg_type, const MessageEventHandler &handler) {
  DebugUtils::Log("New handler for message type %s", msg_type.c_str());
  message_handlers_[msg_type] = handler;
}


void FunapiSessionImpl::Start() {
  started_ = true;
  last_received_ = epoch_ = time(NULL);

  tasks_->Set(nullptr);

  if (GetSessionId().empty()) {
    connect_protocols_.clear();

    for (auto protocol : v_protocols_) {
      if (auto transport = GetTransport(protocol)) {
        connect_protocols_.push_back(protocol);
      }
    }

    if (!connect_protocols_.empty()) {
      if (auto transport = GetTransport(connect_protocols_[0])) {
        transport->Start();
      }
    }
  }
  else {
    for (auto protocol : v_protocols_) {
      if (auto transport = GetTransport(protocol)) {
        if (!transport->IsStarted()) {
          transport->Start();
        }
      }
    }
  }
}


void FunapiSessionImpl::Close() {
  if (!started_)
    return;

  // DebugUtils::Log("Close - Session");

  // Turns off the flag.
  started_ = false;

  for (auto protocol : v_protocols_) {
    auto transport = GetTransport(protocol);
    if (transport)
      transport->Stop();
  }
}


void FunapiSessionImpl::Close(const TransportProtocol protocol) {
  if (!started_)
    return;

  if (auto transport = GetTransport(protocol)) {
    transport->Stop();
  }
}


void FunapiSessionImpl::SendMessage(const std::string &msg_type, std::string &json_string, TransportProtocol protocol = TransportProtocol::kDefault) {
  rapidjson::Document body;
  body.Parse<0>(json_string.c_str());

  // Encodes a messsage type.
  if (msg_type.length() > 0) {
    rapidjson::Value msg_type_node;
    msg_type_node.SetString(rapidjson::StringRef(msg_type.c_str()));
    body.AddMember(rapidjson::StringRef(kMessageTypeAttributeName), msg_type_node, body.GetAllocator());
  }

  // Encodes a session id, if any.
  std::string session_id = GetSessionId();
  if (!session_id.empty()) {
    rapidjson::Value session_id_node;
    session_id_node.SetString(rapidjson::StringRef(session_id.c_str()));
    body.AddMember(rapidjson::StringRef(kSessionIdAttributeName), session_id_node, body.GetAllocator());
  }

  if (protocol == TransportProtocol::kDefault)
    protocol = default_protocol_;

  // Sends the manipulated JSON object through the transport.
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
  if (transport)
    transport->SendMessage(body);
}


void FunapiSessionImpl::SendMessage(FunMessage& message, TransportProtocol protocol = TransportProtocol::kDefault) {
  // Encodes a session id, if any.
  std::string session_id = GetSessionId();
  if (!session_id.empty()) {
    message.set_sid(session_id.c_str());
  }

  if (protocol == TransportProtocol::kDefault)
    protocol = default_protocol_;

  // Sends the manipulated Protobuf object through the transport.
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
  if (transport)
    transport->SendMessage(message);
}


void FunapiSessionImpl::SendMessage(const std::string &body, TransportProtocol protocol = TransportProtocol::kDefault, bool priority) {
  if (protocol == TransportProtocol::kDefault)
    protocol = default_protocol_;

  // Sends the manipulated Protobuf object through the transport.
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
  if (transport)
    transport->SendMessage(body, false, 0, priority);
}


bool FunapiSessionImpl::IsStarted() const {
  return started_;
}


bool FunapiSessionImpl::IsRedirecting() const {
  return !token_.empty();
}


bool FunapiSessionImpl::IsConnected(const TransportProtocol protocol = TransportProtocol::kDefault) const {
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
  if (transport)
    return transport->IsStarted();

  return false;
}


void FunapiSessionImpl::OnTransportReceived(const TransportProtocol protocol,
                                            const FunEncoding encoding,
                                            const HeaderFields &header,
                                            const std::vector<uint8_t> &body) {
  last_received_ = time(NULL);

  std::string msg_type;
  std::string session_id;
  FunMessage proto;

  if (encoding == FunEncoding::kJson) {
    // Parses the given json string.
    rapidjson::Document json;
    json.Parse<0>(reinterpret_cast<char*>(const_cast<uint8_t*>(body.data())));
    assert(json.IsObject());

    if (json.HasMember(kMessageTypeAttributeName)) {
      const rapidjson::Value &msg_type_node = json[kMessageTypeAttributeName];
      assert(msg_type_node.IsString());
      msg_type = msg_type_node.GetString();
      json.RemoveMember(kMessageTypeAttributeName);
    }

    if (json.HasMember(kSessionIdAttributeName)) {
      const rapidjson::Value &session_id_node = json[kSessionIdAttributeName];
      assert(session_id_node.IsString());
      session_id = session_id_node.GetString();
      json.RemoveMember(kSessionIdAttributeName);
    }
  } else if (encoding == FunEncoding::kProtobuf) {
    // FunMessage proto;
    proto.ParseFromArray(body.data(), static_cast<int>(body.size()));

    msg_type = proto.msgtype();
    session_id = proto.sid();
  }

  std::string session_id_old = GetSessionId();
  if (session_id_old.empty()) {
    SetSessionId(session_id);
    // DebugUtils::Log("New session id: %s", session_id.c_str());
  }
  else if (session_id_old.compare(session_id) != 0) {
    DebugUtils::Log("Session id changed: %s => %s", session_id_old.c_str(), session_id.c_str());
    SetSessionId(session_id);
    OnSessionEvent(protocol, SessionEventType::kChanged, session_id, nullptr);
  }

  if (session_reliability_) {
    if (msg_type.empty())
      return;
  }

  EraseRecvTimeout(msg_type);

  MessageHandlerMap::iterator it = message_handlers_.find(msg_type);
  if (it != message_handlers_.end()) {
    it->second(protocol, msg_type, body);
    return;
  }

  if (encoding == FunEncoding::kJson) {
    OnJsonRecv(protocol, msg_type, std::string(body.begin(), body.end()));
  }
  else if (encoding == FunEncoding::kProtobuf) {
    OnProtobufRecv(protocol, proto);
  }
}


void FunapiSessionImpl::OnTransportStopped() {
  // DebugUtils::Log("Transport terminated. Stopping. You may restart again.");
  Close();
}


void FunapiSessionImpl::OnSessionOpen(const TransportProtocol protocol,
                                     const std::string &msg_type,
                                     const std::vector<uint8_t>&v_body) {
  OnSessionEvent(protocol, SessionEventType::kOpened, GetSessionId(), nullptr);

  for (int i=1;i<connect_protocols_.size();++i) {
    if (auto transport = GetTransport(connect_protocols_[i])) {
      transport->Start();
    }
  }
}


void FunapiSessionImpl::OnSessionClose(const TransportProtocol protocol,
                                          const std::string &msg_type,
                                          const std::vector<uint8_t>&v_body) {
  // DebugUtils::Log("Session timed out. Resetting my session id. The server will send me another one next time.");

  OnSessionEvent(protocol, SessionEventType::kClosed, GetSessionId(), nullptr);
  SetSessionId("");

  Close();
}


void FunapiSessionImpl::OnServerPingMessage(
  const TransportProtocol protocol, const std::string &msg_type, const std::vector<uint8_t>&v_body) {

  fun::FunEncoding encoding = GetEncoding(protocol);
  assert(encoding!=FunEncoding::kNone);

  if (encoding == FunEncoding::kJson)
  {
    SendMessage(std::string(v_body.cbegin(), v_body.cend()), protocol);
  }

  if (encoding == FunEncoding::kProtobuf)
  {
    FunMessage msg;
    msg.ParseFromArray(v_body.data(), static_cast<int>(v_body.size()));

    SendMessage(msg.SerializeAsString(), protocol);
  }
}


void FunapiSessionImpl::OnClientPingMessage(
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


void FunapiSessionImpl::OnRedirectMessage(const TransportProtocol protocol,
                                            const std::string &msg_type,
                                            const std::vector<uint8_t>&v_body) {
  fun::FunEncoding encoding = GetEncoding(protocol);
  assert(encoding!=FunEncoding::kNone);

  FunMessage msg;
  FunRedirectMessage *redirect_message = nullptr;

  if (encoding == FunEncoding::kJson) {
    redirect_message = msg.MutableExtension(_sc_redirect);

    std::string temp_string(v_body.begin(), v_body.end());
    DebugUtils::Log("json string = %s", temp_string.c_str());

    rapidjson::Document document;
    document.Parse<0>(reinterpret_cast<char*>(const_cast<uint8_t*>(v_body.data())));

    if (document.HasMember("token")) {
      redirect_message->set_token(document["token"].GetString());
    }

    if (document.HasMember("host")) {
      redirect_message->set_host(document["host"].GetString());
    }

    if (document.HasMember("flavor")) {
      redirect_message->set_flavor(document["flavor"].GetString());
    }

    if (document.HasMember("ports")) {
      rapidjson::Value &ports = document["ports"];
      int count = ports.Size();

      for (int i=0;i<count;++i) {
        rapidjson::Value &v = ports[i];
        FunRedirectMessage_ServerPort *server_port = redirect_message->add_ports();

        if (v.HasMember("port")) {
          server_port->set_port(v["port"].GetInt());
        }

        if (v.HasMember("protocol")) {
          server_port->set_protocol(static_cast<FunRedirectMessage_Protocol>(v["protocol"].GetInt()));
        }

        if (v.HasMember("encoding")) {
          server_port->set_encoding(static_cast<FunRedirectMessage_Encoding>(v["encoding"].GetInt()));
        }
      }
    }
  }

  if (encoding == FunEncoding::kProtobuf)
  {
    msg.ParseFromArray(v_body.data(), static_cast<int>(v_body.size()));

    redirect_message = msg.MutableExtension(_sc_redirect);
  }

  token_ = redirect_message->token();

  for (auto i : v_protocols_) {
    if (auto transport = GetTransport(i)) {
      transport->Stop();
    }
  }
  {
    std::unique_lock<std::mutex> lock(transports_mutex_);
    for (auto i : v_protocols_) {
      transports_[static_cast<int>(i)] = nullptr;
    }
  }

  SetSessionId("");
  hostname_or_ip_ = redirect_message->host();
  std::string flavor = redirect_message->flavor();

  std::vector<std::shared_ptr<FunapiTransportOption>> v_option(FunRedirectMessage_Protocol_Protocol_MAX + 1);
  v_option[FunRedirectMessage_Protocol_PROTO_TCP] = tcp_option_;
  v_option[FunRedirectMessage_Protocol_PROTO_UDP] = udp_option_;
  v_option[FunRedirectMessage_Protocol_PROTO_HTTP] = http_option_;

  int ports_count = redirect_message->ports_size();
  for (int i=0;i<ports_count;++i) {
    FunRedirectMessage_ServerPort server_port = redirect_message->ports(i);

    int32_t port = server_port.port();

    FunEncoding connect_encoding = FunEncoding::kNone;
    FunRedirectMessage_Encoding server_port_encoding = server_port.encoding();
    if (server_port_encoding == FunRedirectMessage_Encoding_ENCODING_JSON) {
      connect_encoding = FunEncoding::kJson;
    }
    else if (server_port_encoding == FunRedirectMessage_Encoding_ENCODING_PROTOBUF) {
      connect_encoding = FunEncoding::kProtobuf;
    }
    else {
      continue;
    }

    FunRedirectMessage::Protocol server_port_protocol = server_port.protocol();
    TransportProtocol connect_protocol = TransportProtocol::kDefault;
    if (server_port_protocol == FunRedirectMessage_Protocol_PROTO_TCP) {
      connect_protocol = TransportProtocol::kTcp;
    }
    else if (server_port_protocol == FunRedirectMessage_Protocol_PROTO_UDP) {
      connect_protocol = TransportProtocol::kUdp;
    }
    else if (server_port_protocol == FunRedirectMessage_Protocol_PROTO_HTTP) {
      connect_protocol = TransportProtocol::kHttp;
    }
    else {
      continue;
    }

    std::shared_ptr<FunapiTransportOption> option = nullptr;
    if (transport_option_handler_) {
      option = transport_option_handler_(connect_protocol, flavor);
    } else if (auto o = v_option[static_cast<int>(server_port_protocol)]) {
      option = o;
    }

    Connect(session_, connect_protocol, port, connect_encoding, option);
  }

  OnSessionEvent(protocol, SessionEventType::kRedirectStarted, GetSessionId(), nullptr);
}


void FunapiSessionImpl::OnRedirectConnectMessage(const TransportProtocol protocol,
                                                  const std::string &msg_type,
                                                  const std::vector<uint8_t>&v_body) {
  fun::FunEncoding encoding = GetEncoding(protocol);
  assert(encoding!=FunEncoding::kNone);

  FunMessage msg;
  FunRedirectConnectMessage *redirect = nullptr;

  if (encoding == FunEncoding::kJson) {
    redirect = msg.MutableExtension(_cs_redirect_connect);

    std::string temp_string(v_body.begin(), v_body.end());
    DebugUtils::Log("json string = %s", temp_string.c_str());

    rapidjson::Document document;
    document.Parse<0>(reinterpret_cast<char*>(const_cast<uint8_t*>(v_body.data())));

    if (document.HasMember("result")) {
      redirect->set_result(static_cast<FunRedirectConnectMessage_Result>(document["result"].GetInt()));
    }
  }

  if (encoding == FunEncoding::kProtobuf)
  {
    msg.ParseFromArray(v_body.data(), static_cast<int>(v_body.size()));
    redirect = msg.MutableExtension(_cs_redirect_connect);
  }

  FunRedirectConnectMessage_Result result = redirect->result();

   if (result == FunRedirectConnectMessage_Result_OK) {
     token_ = "";
     OnSessionEvent(protocol, SessionEventType::kRedirectSucceeded, GetSessionId(), nullptr);
   }
   else {
     token_ = "";

     FunapiError::ErrorCode code = FunapiError::ErrorCode::kNone;
     if (result == FunRedirectConnectMessage_Result_EXPIRED) {
       code = FunapiError::ErrorCode::kRedirectConnectExpired;
     }
     else if (result == FunRedirectConnectMessage_Result_INVALID_TOKEN) {
       code = FunapiError::ErrorCode::kRedirectConnectInvalidToken;
     }
     else if (result == FunRedirectConnectMessage_Result_AUTH_FAILED) {
       code = FunapiError::ErrorCode::kRedirectConnectAuthFailed;
     }

     OnSessionEvent(protocol,
                    SessionEventType::kRedirectFailed,
                    GetSessionId(),
                    fun::FunapiError::Create(code));
   }
}


void FunapiSessionImpl::Update() {
  tasks_->Update();
}


void FunapiSessionImpl::AttachTransport(const std::shared_ptr<FunapiTransport> &transport) {
  if (HasTransport(transport->GetProtocol()))
  {
    DebugUtils::Log("AttachTransport - transport of '%d' type already exists.", static_cast<int>(transport->GetProtocol()));
    // DebugUtils::Log(" You should call DetachTransport first.");
  }
  else {
    auto self = shared_from_this();
    transport->SetReceivedHandler([this, self](const TransportProtocol protocol, const FunEncoding encoding, const HeaderFields &header, const std::vector<uint8_t> &body){ OnTransportReceived(protocol, encoding, header, body);
    });
    transport->SetIsReliableSessionHandler([this, self]() {
      return IsReliableSession();
    });
    transport->SetSendAckHandler([this, self](const TransportProtocol protocol, const uint32_t seq) {
      SendAck(protocol, seq);
    });

    transport->AddStartedCallback([this, self](const TransportProtocol protocol){
      if (UseSodium(protocol)) {
        SendEmptyMessage(protocol);
      }

      if (GetSessionId().empty()) {
        SendEmptyMessage(protocol);
      }

      OnTransportStarted(protocol);
    });
    transport->AddClosedCallback([this, self](const TransportProtocol protocol){
      OnTransportClosed(protocol);
    });
    transport->AddConnectFailedCallback([this, self](const TransportProtocol protocol){ OnTransportConnectFailed(protocol); });
    transport->AddConnectTimeoutCallback([this, self](const TransportProtocol protocol){ OnTransportConnectTimeout(protocol); });
    transport->AddDisconnectedCallback([this, self](const TransportProtocol protocol){ OnTransportDisconnected(protocol); });

    if (transport->GetProtocol() == TransportProtocol::kTcp) {
      transport->SetSendClientPingMessageHandler([this, self](const TransportProtocol protocol)->bool{ return SendClientPingMessage(protocol); });
    }

    {
      std::unique_lock<std::mutex> lock(transports_mutex_);
      transports_[static_cast<int>(transport->GetProtocol())] = transport;
    }

    if (default_protocol_ == TransportProtocol::kDefault)
    {
      SetDefaultProtocol(transport->GetProtocol());
    }
  }
}


std::shared_ptr<FunapiTransport> FunapiSessionImpl::GetTransport(const TransportProtocol protocol) const {
  std::unique_lock<std::mutex> lock(transports_mutex_);

  if (protocol == TransportProtocol::kDefault) {
    for (auto p : v_protocols_) {
      if (auto t = transports_[static_cast<int>(p)]) {
        return t;
      }
    }
  }
  else if (auto t = transports_[static_cast<int>(protocol)]) {
    return t;
  }

  return nullptr;
}


std::string FunapiSessionImpl::GetSessionId() {
  std::unique_lock<std::mutex> lock(session_id_mutex_);
  return session_id_;
}


void FunapiSessionImpl::SetSessionId(const std::string &session_id) {
  std::unique_lock<std::mutex> lock(session_id_mutex_);
  session_id_ = session_id;
}


void FunapiSessionImpl::PushTaskQueue(const std::function<bool()> &task)
{
  auto self = shared_from_this();
  tasks_->Push([self, task]()->bool{ return task(); });
}


void FunapiSessionImpl::AddSessionInitiatedCallback(const SessionInitHandler &handler) {
  on_session_initiated_ += handler;
}


void FunapiSessionImpl::AddSessionClosedCallback(const SessionCloseHandler &handler) {
  on_session_closed_ += handler;
}


void FunapiSessionImpl::AddMaintenanceCallback(const MessageEventHandler &handler) {
  on_maintenance_ += handler;
}


void FunapiSessionImpl::AddTransportConnectFailedCallback(const FunapiTransport::TransportEventHandler &handler) {
  on_transport_connect_failed_ += handler;
}


void FunapiSessionImpl::AddTransportConnectTimeoutCallback(const FunapiTransport::TransportEventHandler &handler) {
  on_transport_connect_timeout_ += handler;
}


void FunapiSessionImpl::AddTransportDisconnectedCallback(const FunapiTransport::TransportEventHandler &handler) {
  on_transport_disconnected_ += handler;
}


void FunapiSessionImpl::AddTransportStartedCallback(const FunapiTransport::TransportEventHandler &handler) {
  on_transport_started_ += handler;
}


void FunapiSessionImpl::AddTransportClosedCallback(const FunapiTransport::TransportEventHandler &handler) {
  on_transport_closed_ += handler;
}


void FunapiSessionImpl::AddProtobufRecvCallback(const ProtobufRecvHandler &handler) {
  on_protobuf_recv_ += handler;
}


void FunapiSessionImpl::AddJsonRecvCallback(const JsonRecvHandler &handler) {
  on_json_recv_ += handler;
}


void FunapiSessionImpl::AddSessionEventCallback(const SessionEventHandler &handler) {
  on_session_event_ += handler;
}


void FunapiSessionImpl::AddTransportEventCallback(const TransportEventHandler &handler) {
  on_transport_event_ += handler;
}


void FunapiSessionImpl::OnProtobufRecv(const TransportProtocol protocol, const FunMessage &message) {
  PushTaskQueue([this, protocol, message]()->bool {
    if (auto s = session_.lock()) {
      on_protobuf_recv_(s, protocol, message);
    }
    return true;
  });
}


void FunapiSessionImpl::OnJsonRecv(const TransportProtocol protocol, const std::string &msg_type, const std::string &json_string) {
  PushTaskQueue([this, protocol, msg_type, json_string]()->bool {
    if (auto s = session_.lock()) {
      on_json_recv_(s, protocol, msg_type, json_string);
    }
    return true;
  });
}


void FunapiSessionImpl::OnSessionEvent(const TransportProtocol protocol,
                                       const SessionEventType type,
                                       const std::string &session_id,
                                       const std::shared_ptr<FunapiError> &error) {
  if (IsRedirecting() == false) {
    PushTaskQueue([this, protocol, type, session_id, error]()->bool {
      if (auto s = session_.lock()) {
        on_session_event_(s, protocol, type, session_id, error);
      }
      return true;
    });
  }
}


void FunapiSessionImpl::OnTransportEvent(const TransportProtocol protocol, const TransportEventType type) {
  if (IsRedirecting() == false ||
      type == TransportEventType::kConnectionFailed ||
      type == TransportEventType::kConnectionTimedOut) {
    PushTaskQueue([this, protocol, type]()->bool {
      if (auto s = session_.lock()) {
        on_transport_event_(s, protocol, type, nullptr);
      }
      return true;
    });
  }
}


void FunapiSessionImpl::OnMaintenance (const TransportProtocol protocol, const std::string &msg_type, const std::vector<uint8_t> &v_body) {
  PushTaskQueue([this, protocol, msg_type, v_body]()->bool {
    on_maintenance_(protocol, msg_type, v_body);
    return true;
  });
}


void FunapiSessionImpl::OnTransportConnectFailed(const TransportProtocol protocol) {
  OnTransportEvent(protocol, TransportEventType::kConnectionFailed);
}


void FunapiSessionImpl::OnTransportConnectTimeout(const TransportProtocol protocol) {
  OnTransportEvent(protocol, TransportEventType::kConnectionTimedOut);
}


void FunapiSessionImpl::OnTransportDisconnected(const TransportProtocol protocol) {
  OnTransportEvent(protocol, TransportEventType::kDisconnected);
}


void FunapiSessionImpl::OnTransportStarted(const TransportProtocol protocol) {
  if (IsRedirecting()) {
    bool is_started = true;
    for (auto p : connect_protocols_) {
      if (auto t = GetTransport(p)) {
        if (!t->IsStarted()) {
          is_started = false;
        }
      }
      else {
        is_started = false;
      }

      if (is_started == false) {
        break;
      }
    }

    if (is_started) {
      SendRedirectConenectMessage(connect_protocols_[0], token_);
    }
  }

  OnTransportEvent(protocol, TransportEventType::kStarted);

  tasks_->Set([this]()->bool{
    CheckRecvTimeout();
    return true;
  });
}


void FunapiSessionImpl::OnTransportClosed(const TransportProtocol protocol) {
  if (IsRedirecting() == false) {
    tasks_->Set(nullptr);
  }

  OnTransportEvent(protocol, TransportEventType::kStopped);
}


bool FunapiSessionImpl::HasTransport(const TransportProtocol protocol) const {
  if (GetTransport(protocol))
    return true;

  return false;
}


void FunapiSessionImpl::SetDefaultProtocol(const TransportProtocol protocol) {
  default_protocol_ = protocol;
}


FunEncoding FunapiSessionImpl::GetEncoding(const TransportProtocol protocol) const {
  auto transport = GetTransport(protocol);
  if (transport)
    return transport->GetEncoding();

  return FunEncoding::kNone;
}


TransportProtocol FunapiSessionImpl::GetDefaultProtocol() const {
  return default_protocol_;
}


void FunapiSessionImpl::SendEmptyMessage(const TransportProtocol protocol) {
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
  FunEncoding encoding = transport->GetEncoding();

  assert(encoding!=FunEncoding::kNone);

  if (encoding == FunEncoding::kJson) {
    SendMessage("{}", protocol);
  }
  else if (encoding == FunEncoding::kProtobuf) {
    FunMessage msg;
    SendMessage(msg.SerializeAsString(), protocol);
  }
}


bool FunapiSessionImpl::SendClientPingMessage(const TransportProtocol protocol) {
  assert(protocol==TransportProtocol::kTcp);

  if (GetSessionId().empty())
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

    std::string session_id = GetSessionId();
    if (!session_id.empty()) {
      msg.set_sid(session_id.c_str());
    }

    SendMessage(msg.SerializeAsString(), protocol);
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
    msg.AddMember(rapidjson::StringRef(kMessageTypeAttributeName), msg_type_node, msg.GetAllocator());

    // Encodes a session id, if any.
    std::string session_id = GetSessionId();
    if (!session_id.empty()) {
      rapidjson::Value session_id_node;
      session_id_node.SetString(rapidjson::StringRef(session_id.c_str()));
      msg.AddMember(rapidjson::StringRef(kSessionIdAttributeName), session_id_node, msg.GetAllocator());
    }

    // Convert JSON document to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msg.Accept(writer);
    std::string json_string = buffer.GetString();

    SendMessage(json_string, protocol);
  }

  return true;
}


void FunapiSessionImpl::SendAck(const TransportProtocol protocol, const uint32_t ack) {
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
  if (transport == nullptr) return;

  DebugUtils::Log("TCP send ack message - ack:%d", ack);

  if (transport->GetEncoding() == FunEncoding::kJson) {
    rapidjson::Document msg;
    msg.SetObject();
    rapidjson::Value key(kAckNumAttributeName, msg.GetAllocator());
    rapidjson::Value ack_node(rapidjson::kNumberType);
    ack_node.SetUint(ack);
    msg.AddMember(key, ack_node, msg.GetAllocator());

    std::string session_id = GetSessionId();
    if (!session_id.empty()) {
      rapidjson::Value session_id_node;
      session_id_node.SetString(rapidjson::StringRef(session_id.c_str()));
      msg.AddMember(rapidjson::StringRef(kSessionIdAttributeName), session_id_node, msg.GetAllocator());
    }

    // Convert JSON document to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msg.Accept(writer);
    std::string json_string = buffer.GetString();

    SendMessage(json_string, protocol, true);
  }
  else if (transport->GetEncoding() == FunEncoding::kProtobuf) {
    FunMessage msg;
    msg.set_ack(ack);

    std::string session_id = GetSessionId();
    if (!session_id.empty()) {
      msg.set_sid(session_id.c_str());
    }

    SendMessage(msg.SerializeAsString(), protocol, true);
  }
}


void FunapiSessionImpl::SendRedirectConenectMessage(const TransportProtocol protocol, const std::string &token) {
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
  if (transport == nullptr) return;

  if (transport->GetEncoding() == FunEncoding::kJson) {
    rapidjson::Document msg;
    msg.SetObject();
    rapidjson::Value message_node(token.c_str(), msg.GetAllocator());
    msg.AddMember("token", message_node, msg.GetAllocator());

    // Convert JSON document to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msg.Accept(writer);
    std::string json_string = buffer.GetString();

    SendMessage(kRedirectConnectMessageType, json_string, protocol);
  }
  else if (transport->GetEncoding() == FunEncoding::kProtobuf) {
    FunMessage msg;
    msg.set_msgtype(kRedirectConnectMessageType);

    std::string session_id = GetSessionId();
    if (!session_id.empty()) {
      msg.set_sid(session_id.c_str());
    }

    FunRedirectConnectMessage *redirect = msg.MutableExtension(_cs_redirect_connect);
    redirect->set_token(token);

    SendMessage(msg.SerializeAsString(), protocol);
  }
}


bool FunapiSessionImpl::IsReliableSession() const {
  return session_reliability_;
}


void FunapiSessionImpl::AddRecvTimeoutCallback(const RecvTimeoutHandler &handler) {
  on_recv_timeout_ += handler;
}


void FunapiSessionImpl::SetRecvTimeout(const std::string &msg_type, const int seconds) {
  std::unique_lock<std::mutex> lock(m_recv_timeout_mutex_);
  m_recv_timeout_[msg_type] = std::make_shared<FunapiTimer>(seconds);
}


void FunapiSessionImpl::EraseRecvTimeout(const std::string &msg_type) {
  std::unique_lock<std::mutex> lock(m_recv_timeout_mutex_);
  m_recv_timeout_.erase(msg_type);
}


void FunapiSessionImpl::CheckRecvTimeout() {
  std::vector<std::string> msg_types;

  {
    std::unique_lock<std::mutex> lock(m_recv_timeout_mutex_);
    // m_recv_timeout_.erase(msg_type);
    for (auto i : m_recv_timeout_) {
      if (i.second->IsExpired()) {
        msg_types.push_back(i.first);
      }
    }

    for (auto t : msg_types) {
      m_recv_timeout_.erase(t);
    }
  }

  for (auto t : msg_types) {
    OnRecvTimeout(t);
  }
}


void FunapiSessionImpl::OnRecvTimeout(const std::string &msg_type) {
  PushTaskQueue([this, msg_type]()->bool {
    if (auto s = session_.lock()) {
      on_recv_timeout_(s, msg_type);
    }
    return true;
  });
}


void FunapiSessionImpl::SetTransportOptionCallback(const TransportOptionHandler &handler) {
  transport_option_handler_ = handler;
}


bool FunapiSessionImpl::UseSodium(const TransportProtocol protocol) {
  if (protocol == TransportProtocol::kTcp) {
    if (auto t = std::static_pointer_cast<FunapiTcpTransport>(GetTransport(TransportProtocol::kTcp))) {
      return t->UseSodium();
    }
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// FunapiSession implementation.


FunapiSession::FunapiSession(const char* hostname_or_ip, bool reliability)
  : impl_(std::make_shared<FunapiSessionImpl>(hostname_or_ip, reliability)) {
}


FunapiSession::~FunapiSession() {
}


std::shared_ptr<FunapiSession> FunapiSession::Create(const char* hostname_or_ip, bool reliability) {
  return std::make_shared<FunapiSession>(hostname_or_ip, reliability);
}


void FunapiSession::Connect(const TransportProtocol protocol, int port, FunEncoding encoding, std::shared_ptr<FunapiTransportOption> option) {
  impl_->Connect(shared_from_this(), protocol, port, encoding, option);
}


void FunapiSession::Connect(const TransportProtocol protocol) {
  impl_->Connect(protocol);
}


void FunapiSession::Close() {
  impl_->Close();
}


void FunapiSession::Close(const TransportProtocol protocol) {
  impl_->Close(protocol);
}


void FunapiSession::SendMessage(const std::string &msg_type, std::string &json_string, TransportProtocol protocol) {
  impl_->SendMessage(msg_type, json_string, protocol);
}


void FunapiSession::SendMessage(FunMessage& message, TransportProtocol protocol) {
  impl_->SendMessage(message, protocol);
}


bool FunapiSession::IsConnected(const TransportProtocol protocol) const {
  return impl_->IsConnected(protocol);
}


void FunapiSession::Update() {
  impl_->Update();
}


void FunapiSession::AddProtobufRecvCallback(const ProtobufRecvHandler &handler) {
  impl_->AddProtobufRecvCallback(handler);
}


void FunapiSession::AddJsonRecvCallback(const JsonRecvHandler &handler) {
  impl_->AddJsonRecvCallback(handler);
}


void FunapiSession::AddSessionEventCallback(const SessionEventHandler &handler) {
  impl_->AddSessionEventCallback(handler);
}


void FunapiSession::AddTransportEventCallback(const TransportEventHandler &handler) {
  impl_->AddTransportEventCallback(handler);
}


bool FunapiSession::HasTransport(const TransportProtocol protocol) const {
  return impl_->HasTransport(protocol);
}


void FunapiSession::SetDefaultProtocol(const TransportProtocol protocol) {
  impl_->SetDefaultProtocol(protocol);
}


bool FunapiSession::IsReliableSession() const {
  return impl_->IsReliableSession();
}


FunEncoding FunapiSession::GetEncoding(const TransportProtocol protocol) const {
  return impl_->GetEncoding(protocol);
}


TransportProtocol FunapiSession::GetDefaultProtocol() const {
  return impl_->GetDefaultProtocol();
}


void FunapiSession::AddRecvTimeoutCallback(const RecvTimeoutHandler &handler) {
  impl_->AddRecvTimeoutCallback(handler);
}


void FunapiSession::SetRecvTimeout(const std::string &msg_type, const int seconds) {
  impl_->SetRecvTimeout(msg_type, seconds);
}


void FunapiSession::EraseRecvTimeout(const std::string &msg_type) {
  impl_->EraseRecvTimeout(msg_type);
}


void FunapiSession::SetTransportOptionCallback(const TransportOptionHandler &handler) {
  impl_->SetTransportOptionCallback(handler);
}

}  // namespace fun
