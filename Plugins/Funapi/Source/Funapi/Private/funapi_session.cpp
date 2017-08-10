// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin.h"
#include "funapi_utils.h"
#include "funapi_tasks.h"
#include "funapi_session.h"
#include "funapi_http.h"
#include "funapi_socket.h"
#include "funapi_encryption.h"
#include "funapi_version.h"
#include "funapi/network/ping_message.pb.h"
#include "funapi/service/redirect_message.pb.h"

namespace fun {

////////////////////////////////////////////////////////////////////////////////
// FunapiQueue implementation.
class FunapiMessage;
class FunapiQueue : public std::enable_shared_from_this<FunapiQueue> {
 public:
  FunapiQueue();
  virtual ~FunapiQueue();

  static std::shared_ptr<FunapiQueue> Create();

  bool Empty();
  std::shared_ptr<FunapiMessage> Front();
  void PushBack(std::shared_ptr<FunapiMessage> msg);
  void PopFront();

 private:
  std::deque<std::shared_ptr<FunapiMessage>> queue_;
  std::mutex mutex_;
};


FunapiQueue::FunapiQueue() {
}


FunapiQueue::~FunapiQueue() {
  // DebugUtils::Log("%s", __FUNCTION__);
}


std::shared_ptr<FunapiQueue> FunapiQueue::Create() {
  return std::make_shared<FunapiQueue>();
}


bool FunapiQueue::Empty() {
  std::unique_lock<std::mutex> lock(mutex_);
  return queue_.empty();
}


std::shared_ptr<FunapiMessage> FunapiQueue::Front() {
  std::unique_lock<std::mutex> lock(mutex_);
  return queue_.front();
}


void FunapiQueue::PushBack(std::shared_ptr<FunapiMessage> msg) {
  std::unique_lock<std::mutex> lock(mutex_);
  queue_.push_back(msg);
}


void FunapiQueue::PopFront() {
  std::unique_lock<std::mutex> lock(mutex_);
  queue_.pop_front();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiSessionId implementation.
class FunapiSessionId : public std::enable_shared_from_this<FunapiSessionId> {
 public:
  FunapiSessionId();
  virtual ~FunapiSessionId();

  static std::shared_ptr<FunapiSessionId> Create();

  std::string Get(const FunEncoding encoding);
  void Set(const std::string &session_id, const FunEncoding encoding);

 private:
  std::string json_;
  std::string protobuf_;
  std::mutex mutex_;
};


FunapiSessionId::FunapiSessionId() {
}


FunapiSessionId::~FunapiSessionId() {
  // DebugUtils::Log("%s", __FUNCTION__);
}


std::shared_ptr<FunapiSessionId> FunapiSessionId::Create() {
  return std::make_shared<FunapiSessionId>();
}


std::string FunapiSessionId::Get(const FunEncoding encoding) {
  std::unique_lock<std::mutex> lock(mutex_);

  if (encoding == FunEncoding::kProtobuf) {
    return protobuf_;
  }

  return json_;
}


void FunapiSessionId::Set(const std::string &session_id, const FunEncoding encoding) {
  std::unique_lock<std::mutex> lock(mutex_);

  if (encoding == FunEncoding::kJson) {
    json_ = session_id;
    protobuf_ = json_;
  }
  else if (encoding == FunEncoding::kProtobuf) {
    json_ = FunapiUtil::StringFromBytes(session_id);
    protobuf_ = session_id;
  }
}


////////////////////////////////////////////////////////////////////////////////
// FunapiMessage implementation.

class FunapiMessage : public std::enable_shared_from_this<FunapiMessage> {
 public:
  FunapiMessage() = delete;
  virtual ~FunapiMessage();

  FunapiMessage(const rapidjson::Document &json, const EncryptionType type);
  FunapiMessage(const FunMessage &pbuf, const EncryptionType type);
  FunapiMessage(const std::vector<uint8_t> &body, const EncryptionType type);
  FunapiMessage(const FunEncoding encoding,
                const std::vector<uint8_t> &body,
                const EncryptionType type);

  static std::shared_ptr<FunapiMessage> Create(const FunEncoding encoding,
                                               const std::vector<uint8_t> &body);

  static std::shared_ptr<FunapiMessage> Create(const rapidjson::Document &json,
                                               const EncryptionType type);
  static std::shared_ptr<FunapiMessage> Create(const FunMessage &pbuf,
                                               const EncryptionType type);
  static std::shared_ptr<FunapiMessage> Create(const std::vector<uint8_t> &body,
                                               const EncryptionType type);
  static std::shared_ptr<FunapiMessage> Create(const FunEncoding encoding,
                                               const std::vector<uint8_t> &body,
                                               const EncryptionType type);

  rapidjson::Document& GetJsonDocumenet();
  FunMessage& GetProtobufMessage();
  std::vector<uint8_t>& GetBody();

  void SetUseSentQueue(bool use);
  bool UseSentQueue();

  void SetUseSeq(bool use);
  bool UseSeq();

  uint32_t GetSeq();
  void SetSeq(uint32_t seq);

  FunEncoding GetEncoding();

  EncryptionType GetEncryptionType();

 private:
  bool use_sent_queue_ = false;
  bool use_seq_ = false;
  uint32_t seq_ = 0;
  FunEncoding encoding_ = FunEncoding::kNone;
  std::vector<uint8_t> body_;
  rapidjson::Document json_document_;
  FunMessage protobuf_message_;
  EncryptionType encryption_type_ = EncryptionType::kNoneEncryption;
};


FunapiMessage::FunapiMessage(const rapidjson::Document &json,
                             const EncryptionType type)
: encoding_(FunEncoding::kJson), encryption_type_(type) {
  json_document_.CopyFrom(json, json_document_.GetAllocator());
}


FunapiMessage::FunapiMessage(const FunMessage &message,
                             const EncryptionType type)
: encoding_(FunEncoding::kProtobuf),
  protobuf_message_(message),
  encryption_type_(type) {
}


FunapiMessage::FunapiMessage(const std::vector<uint8_t> &body,
                             const EncryptionType type)
: encoding_(FunEncoding::kNone),
  body_(body),
  encryption_type_(type) {
}


FunapiMessage::FunapiMessage(const FunEncoding encoding,
                             const std::vector<uint8_t> &body,
                             const EncryptionType type)
: encoding_(encoding),
  encryption_type_(type) {
  if (encoding_ == FunEncoding::kJson) {
    json_document_.Parse<0>(reinterpret_cast<char*>(const_cast<uint8_t*>(body.data())));
  }
  else if (encoding_ == FunEncoding::kProtobuf) {
    protobuf_message_.ParseFromArray(body.data(), static_cast<int>(body.size()));
  }
  else {
    body_ = body;
  }
}


FunapiMessage::~FunapiMessage() {
}


std::shared_ptr<FunapiMessage> FunapiMessage::Create(const rapidjson::Document &json,
                                                     const EncryptionType type) {
  return std::make_shared<FunapiMessage>(json, type);
}


std::shared_ptr<FunapiMessage> FunapiMessage::Create(const FunMessage &message,
                                                     const EncryptionType type) {
  return std::make_shared<FunapiMessage>(message, type);
}


std::shared_ptr<FunapiMessage> FunapiMessage::Create(const std::vector<uint8_t>  &body,
                                                     const EncryptionType type) {
  return std::make_shared<FunapiMessage>(body, type);
}


std::shared_ptr<FunapiMessage> FunapiMessage::Create(const FunEncoding encoding,
                                                     const std::vector<uint8_t>  &body,
                                                     const EncryptionType type) {
  return std::make_shared<FunapiMessage>(encoding, body, type);
}


std::shared_ptr<FunapiMessage> FunapiMessage::Create(const FunEncoding encoding,
                                                     const std::vector<uint8_t>  &body) {
  return FunapiMessage::Create(encoding, body, EncryptionType::kNoneEncryption);
}


bool FunapiMessage::UseSentQueue() {
  return use_sent_queue_;
}


void FunapiMessage::SetUseSentQueue(bool use) {
  use_sent_queue_ = use;
}


bool FunapiMessage::UseSeq() {
  return use_seq_;
}


void FunapiMessage::SetUseSeq(bool use) {
  use_seq_ = use;
}


uint32_t FunapiMessage::GetSeq() {
  return seq_;
}


void FunapiMessage::SetSeq(uint32_t seq) {
  seq_ = seq;
}


rapidjson::Document& FunapiMessage::GetJsonDocumenet() {
  return json_document_;
}


FunMessage& FunapiMessage::GetProtobufMessage() {
  return protobuf_message_;
}


FunEncoding FunapiMessage::GetEncoding() {
  return encoding_;
}


EncryptionType FunapiMessage::GetEncryptionType() {
  return encryption_type_;
}


std::vector<uint8_t>& FunapiMessage::GetBody() {
  if (encoding_ == FunEncoding::kProtobuf) {
    body_.resize(protobuf_message_.ByteSize());
    protobuf_message_.SerializeToArray(body_.data(), protobuf_message_.ByteSize());
  }
  else if (encoding_ == FunEncoding::kJson) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    json_document_.Accept(writer);

    std::string temp_string = buffer.GetString();
    body_ = std::vector<uint8_t>(temp_string.cbegin(), temp_string.cend());
  }

  return body_;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiSessionImpl declaration.

class FunapiTransport;
class FunapiSessionImpl : public std::enable_shared_from_this<FunapiSessionImpl> {
 public:
  typedef std::map<std::string, std::string> HeaderFields;
  typedef std::function<void(const TransportProtocol,
    const std::string &,
    const std::vector<uint8_t> &,
    const std::shared_ptr<FunapiMessage>)> MessageEventHandler;
  typedef std::function<void(const std::string &)> SessionInitHandler;
  typedef std::function<void()> SessionCloseHandler;
  typedef std::function<void()> NotifyHandler;

  typedef std::function<void(const TransportProtocol,
    const FunEncoding,
    const HeaderFields &,
    const std::vector<uint8_t> &,
    const std::shared_ptr<FunapiMessage>)> TransportReceivedHandler;

  typedef FunapiSession::TransportEventHandler TransportEventHandler;
  typedef FunapiSession::SessionEventHandler SessionEventHandler;
  typedef FunapiSession::ProtobufRecvHandler ProtobufRecvHandler;
  typedef FunapiSession::JsonRecvHandler JsonRecvHandler;
  typedef FunapiSession::RecvTimeoutHandler RecvTimeoutHandler;
  typedef FunapiSession::RecvTimeoutIntHandler RecvTimeoutIntHandler;
  typedef FunapiSession::TransportOptionHandler TransportOptionHandler;

  FunapiSessionImpl() = delete;
  FunapiSessionImpl(const char* hostname_or_ip, std::shared_ptr<FunapiSessionOption> option);
  virtual ~FunapiSessionImpl();

  void Connect(const std::weak_ptr<FunapiSession>& session, const TransportProtocol protocol, int port, FunEncoding encoding, std::shared_ptr<FunapiTransportOption> option = nullptr);
  void Connect(const TransportProtocol protocol);
  void Close();
  void Close(const TransportProtocol protocol);

  void Update();
  void UpdateTasks();
  void UpdateTrasnports();
  void UpdateSocketSelect();
  static void UpdateAll();

  void SendMessage(const std::string &msg_type,
                   const std::string &json_string,
                   const TransportProtocol protocol,
                   const EncryptionType encryption_type = EncryptionType::kDefaultEncryption);

  void SendMessage(const FunMessage& message,
                   const TransportProtocol protocol,
                   const EncryptionType encryption_type = EncryptionType::kDefaultEncryption);

  void AddSessionEventCallback(const SessionEventHandler &handler);
  void AddTransportEventCallback(const TransportEventHandler &handler);

  bool IsConnected(const TransportProtocol protocol) const;
  bool IsConnected() const;
  bool IsReliableSession() const;

  std::shared_ptr<FunapiTransport> GetTransport(const TransportProtocol protocol) const;
  bool HasTransport(const TransportProtocol protocol) const;

  void AddProtobufRecvCallback(const ProtobufRecvHandler &handler);
  void AddJsonRecvCallback(const JsonRecvHandler &handler);

  void AddSessionInitiatedCallback(const SessionInitHandler &handler);
  void AddSessionClosedCallback(const SessionCloseHandler &handler);

  TransportProtocol GetDefaultProtocol() const;
  void SetDefaultProtocol(const TransportProtocol protocol);

  FunEncoding GetEncoding(const TransportProtocol protocol) const;

  void AddRecvTimeoutCallback(const RecvTimeoutHandler &handler);
  void AddRecvTimeoutCallback(const RecvTimeoutIntHandler &handler);
  void SetRecvTimeout(const std::string &msg_type, const int seconds);
  void SetRecvTimeout(const int32_t msg_type, const int seconds);
  void EraseRecvTimeout(const std::string &msg_type);
  void EraseRecvTimeout(const int32_t msg_type);

  void SetTransportOptionCallback(const TransportOptionHandler &handler);

  static void Add(std::shared_ptr<FunapiSessionImpl> s);
  static std::vector<std::shared_ptr<FunapiSessionImpl>> GetSessionImpls();

  void OnTransportReceived(const TransportProtocol protocol,
                           const FunEncoding encoding,
                           const HeaderFields &header,
                           const std::vector<uint8_t> &body,
                           const std::shared_ptr<FunapiMessage> message);

  void SendAck(const TransportProtocol protocol,
               const uint32_t ack,
               const EncryptionType encryption_type = EncryptionType::kDefaultEncryption);

  std::string GetSessionId(const FunEncoding encoding);
  std::shared_ptr<FunapiSessionId> GetSessionIdSharedPtr();
  std::shared_ptr<FunapiQueue> GetQueueSharedPtr(const TransportProtocol protocol);

  void OnTransportConnectFailed(const TransportProtocol protocol, std::shared_ptr<FunapiError> error = nullptr);
  void OnTransportDisconnected(const TransportProtocol protocol, std::shared_ptr<FunapiError> error = nullptr);
  void OnTransportConnectTimeout(const TransportProtocol protocol, std::shared_ptr<FunapiError> error = nullptr);
  void OnTransportStarted(const TransportProtocol protocol, std::shared_ptr<FunapiError> error = nullptr);
  void OnTransportClosed(const TransportProtocol protocol, std::shared_ptr<FunapiError> error = nullptr);

  bool UseSodium(const TransportProtocol protocol);
  void SendEmptyMessage(const TransportProtocol protocol,
                        const EncryptionType encryption_type = EncryptionType::kDefaultEncryption);
  bool SendClientPingMessage(const TransportProtocol protocol,
                             const EncryptionType encryption_type = EncryptionType::kDefaultEncryption);

  void PushNetworkThreadTask(const FunapiThread::TaskHandler &handler);
  void PushTaskQueue(const std::function<bool()> &task);

  void CheckRedirect();

 private:
  void OnClose();
  void OnClose(const TransportProtocol protocol);

  void Start();
  void OnConnect(const TransportProtocol protocol);

  bool IsRedirecting() const;

  void RegisterHandler(const std::string &msg_type, const MessageEventHandler &handler);

  void SetSessionId(const std::string &session_id, const FunEncoding encoding);

  void AttachTransport(const std::shared_ptr<FunapiTransport> &transport);

  void OnSessionOpen(const TransportProtocol protocol,
    const std::string &msg_type,
    const std::vector<uint8_t>&v_body,
    const std::shared_ptr<FunapiMessage> message);
  void OnSessionClose(const TransportProtocol protocol, const std::string &msg_type,
    const std::vector<uint8_t>&v_body,
    const std::shared_ptr<FunapiMessage> message);

  void OnProtobufRecv(const TransportProtocol protocol, const FunMessage &message);
  void OnJsonRecv(const TransportProtocol protocol, const std::string &msg_type, const std::string &json_string);

  void OnSessionEvent(const TransportProtocol protocol,
                      const FunEncoding encoding,
                      const SessionEventType type,
                      const std::string &session_id,
                      const std::shared_ptr<FunapiError> &error);
  void OnTransportEvent(const TransportProtocol protocol,
                        const FunEncoding encoding,
                        const TransportEventType type,
                        std::shared_ptr<FunapiError> error);

  void OnMaintenance(const TransportProtocol, const std::string &, const std::vector<uint8_t> &);

  void OnServerPingMessage(const TransportProtocol protocol, const std::string &msg_type,
    const std::vector<uint8_t>&v_body,
    const std::shared_ptr<FunapiMessage> message);
  void OnClientPingMessage(const TransportProtocol protocol, const std::string &msg_type, const std::vector<uint8_t>&v_body, const std::shared_ptr<FunapiMessage> message);

  void OnRedirectMessage(const TransportProtocol protocol, const std::string &msg_type,
    const std::vector<uint8_t>&v_body,
    const std::shared_ptr<FunapiMessage> message);
  void OnRedirectConnectMessage(const TransportProtocol protocol, const std::string &msg_type,
    const std::vector<uint8_t>&v_body,
    const std::shared_ptr<FunapiMessage> message);

  void Initialize();

  void SendRedirectConenectMessage(const TransportProtocol protocol,
                                   const std::string &token,
                                   const EncryptionType encryption_type = EncryptionType::kDefaultEncryption);

  void SendMessage(std::shared_ptr<FunapiMessage> &message,
                   const TransportProtocol protocol,
                   bool priority = false,
                   bool handshake = false);

  bool SendHandShakeMessages(const TransportProtocol protocol);

  bool started_;

  std::vector<std::shared_ptr<FunapiTransport>> transports_;
  mutable std::mutex transports_mutex_;
  TransportProtocol default_protocol_ = TransportProtocol::kDefault;

  typedef std::unordered_map<std::string, MessageEventHandler> MessageHandlerMap;
  MessageHandlerMap message_handlers_;

  std::shared_ptr<FunapiTasks> tasks_;

  FunapiEvent<ProtobufRecvHandler> on_protobuf_recv_;
  FunapiEvent<JsonRecvHandler> on_json_recv_;

  FunapiEvent<SessionEventHandler> on_session_event_;
  FunapiEvent<TransportEventHandler> on_transport_event_;

  FunapiEvent<RecvTimeoutHandler> on_recv_timeout_;
  FunapiEvent<RecvTimeoutIntHandler> on_recv_timeout_int_;

  std::string hostname_or_ip_;
  std::weak_ptr<FunapiSession> session_;

  std::vector<TransportProtocol> v_protocols_ = { TransportProtocol::kTcp, TransportProtocol::kHttp, TransportProtocol::kUdp };

  std::unordered_map<std::string, std::shared_ptr<FunapiTimer>> m_recv_timeout_;
  std::unordered_map<int32_t, std::shared_ptr<FunapiTimer>> m_recv_timeout_int_;
  std::mutex m_recv_timeout_mutex_;

  void CheckRecvTimeout();
  void OnRecvTimeout(const std::string &msg_type);
  void OnRecvTimeout(const int32_t msg_type);

  std::string token_;

  std::shared_ptr<FunapiTcpTransportOption> tcp_option_ = nullptr;
  std::shared_ptr<FunapiUdpTransportOption> udp_option_ = nullptr;
  std::shared_ptr<FunapiHttpTransportOption> http_option_ = nullptr;

  TransportOptionHandler transport_option_handler_ = nullptr;

  std::shared_ptr<FunapiSessionId> session_id_ = nullptr;

  static std::vector<std::weak_ptr<FunapiSessionImpl>> vec_sessions_;
  static std::mutex vec_sessions_mutex_;

  std::shared_ptr<FunapiThread> network_thread_ = nullptr;

  std::shared_ptr<FunapiSessionOption> session_option_ = nullptr;

  std::vector<std::shared_ptr<FunapiQueue>> send_queues_;

  std::shared_ptr<FunapiMessage> funapi_message_redirect_ = nullptr;
  TransportProtocol protocol_redirect_ = TransportProtocol::kDefault;
  void OnRedirect();
};


////////////////////////////////////////////////////////////////////////////////
// FunapiTransport implementation.

class FunapiTransport : public std::enable_shared_from_this<FunapiTransport> {
 public:
  typedef std::map<std::string, std::string> HeaderFields;
  typedef std::function<void(const TransportProtocol,
                             const FunEncoding,
                             const HeaderFields &,
                             const std::vector<uint8_t> &,
                             const std::shared_ptr<FunapiMessage>)> TransportReceivedHandler;

  static const size_t kMaxSend = 32;

  FunapiTransport(std::weak_ptr<FunapiSessionImpl> session,
                  TransportProtocol protocol,
                  const std::string &hostname_or_ip,
                  uint16_t port, FunEncoding encoding);
  virtual ~FunapiTransport();

  bool IsStarted();
  virtual void Start() = 0;
  virtual void Stop(bool use_force = false, std::shared_ptr<FunapiError> error = nullptr);

  void SendMessage(std::shared_ptr<FunapiMessage> message,
                   bool priority,
                   bool handshake);

  virtual TransportProtocol GetProtocol() { return TransportProtocol::kDefault; };
  FunEncoding GetEncoding() { return encoding_; };

  void SetConnectTimeout(const time_t timeout);

  virtual void ResetClientPingTimeout();

  void SetEncryptionType(EncryptionType type);
  void SetEncryptionType(EncryptionType type, const std::string &public_key);

  void SetState(TransportState state);
  TransportState GetState();

  virtual void Send(bool send_all = false);
  virtual void Update();

  void SetSendSessionIdOnlyOnce(const bool once);
  void SetUseFirstSessionId(const bool use);

 protected:
  void PushNetworkThreadTask(const FunapiThread::TaskHandler &handler);

  void OnReceived(const TransportProtocol protocol,
                  const FunEncoding encoding,
                  const HeaderFields &header,
                  const std::vector<uint8_t> &body);

  bool OnAckReceived(const uint32_t ack);
  bool OnSeqReceived(const uint32_t seq);

  virtual bool EncodeThenSendMessage(std::shared_ptr<FunapiMessage> message,
                                     std::vector<uint8_t> &body,
                                     const EncryptionType encryption_type) = 0;
  virtual bool EncodeThenSendMessage(std::shared_ptr<FunapiMessage> message);

  void PushSendQueue(std::shared_ptr<FunapiMessage> message,
                     bool priority,
                     bool handshake);

  bool DecodeMessage(int nRead, std::vector<uint8_t> &receiving, int &next_decoding_offset, bool &header_decoded, HeaderFields &header_fields);
  bool TryToDecodeHeader(std::vector<uint8_t> &receiving, int &next_decoding_offset, bool &header_decoded, HeaderFields &header_fields);
  bool TryToDecodeBody(std::vector<uint8_t> &receiving, int &next_decoding_offset, bool &header_decoded, HeaderFields &header_fields);
  bool EncodeMessage(std::shared_ptr<FunapiMessage> message,
                     std::vector<uint8_t> &body,
                     const EncryptionType encryption_type = EncryptionType::kDefaultEncryption);
  bool DecodeMessage(int nRead, std::vector<uint8_t> &receiving);

  std::string MakeHeaderString(const std::vector<uint8_t> &body);
  void MakeHeaderFields(HeaderFields &header_fields, const std::vector<uint8_t> &body);
  void MakeHeaderString(std::string &header_string, const HeaderFields &header_fields);

  void PushUnsent(const uint32_t ack);

  void OnTransportStarted(const TransportProtocol protocol, std::shared_ptr<FunapiError> error = nullptr);
  void OnTransportClosed(const TransportProtocol protocol, std::shared_ptr<FunapiError> error = nullptr);
  void OnTransportConnectFailed(const TransportProtocol protocol, std::shared_ptr<FunapiError> error = nullptr);
  void OnTransportConnectTimeout(const TransportProtocol protocol, std::shared_ptr<FunapiError> error = nullptr);
  void OnTransportDisconnected(const TransportProtocol protocol, std::shared_ptr<FunapiError> error = nullptr);

  bool IsReliableSession() const;

  std::string GetSessionId();

  std::shared_ptr<FunapiQueue> send_queue_;
  std::shared_ptr<FunapiQueue> send_priority_queue_;
  std::shared_ptr<FunapiQueue> send_handshake_queue_;
  std::shared_ptr<FunapiQueue> sent_queue_;

  // Encoding-serializer-releated member variables.
  FunEncoding encoding_ = FunEncoding::kNone;

  time_t connect_timeout_seconds_ = 10;
  FunapiTimer connect_timeout_timer_;

  uint32_t seq_ = 0;
  uint32_t seq_recvd_ = 0;
  bool seq_receiving_ = false;
  bool ack_receiving_ = false;
  bool reconnect_first_ack_receiving_ = false;

  std::shared_ptr<FunapiEncryption> encrytion_;
  bool sequence_number_validation_ = false;

  std::weak_ptr<FunapiSessionImpl> session_impl_;

  void OnTransportReceived(const TransportProtocol protocol,
                           const FunEncoding encoding,
                           const HeaderFields &header,
                           const std::vector<uint8_t> &body,
                           const std::shared_ptr<FunapiMessage> message);
  void OnSendAck(const TransportProtocol protocol, const uint32_t seq);

  TransportProtocol protocol_;

  std::string hostname_or_ip_;
  uint16_t port_;

  void SetSeq(std::shared_ptr<FunapiMessage> message);
  void SetSessionId(std::shared_ptr<FunapiMessage> message);

  bool UseSendSessionIdOnlyOnce();
  bool first_set_session_id_ = true;
  bool UseFirstSessionId();

  virtual void OnDisconnecting(std::shared_ptr<FunapiError> error = nullptr);

 private:
  TransportState state_ = TransportState::kDisconnected;
  std::mutex state_mutex_;

  // Message-related.
  bool first_sending_ = true;

  std::shared_ptr<FunapiSessionId> session_id_ = nullptr;

  bool use_send_session_id_only_once_ = false;
};


FunapiTransport::FunapiTransport(std::weak_ptr<FunapiSessionImpl> session,
                                 TransportProtocol protocol,
                                 const std::string &hostname_or_ip,
                                 uint16_t port, FunEncoding encoding)
: encoding_(encoding),
  encrytion_(std::make_shared<FunapiEncryption>()),
  session_impl_(session),
  protocol_(protocol),
  hostname_or_ip_(hostname_or_ip),
  port_(port),
  state_(TransportState::kDisconnected)
{
  std::random_device rd;
  std::default_random_engine re(rd());
  std::uniform_int_distribution<uint32_t> dist(0,std::numeric_limits<uint32_t>::max());
  seq_ = dist(re);

  if (auto s = session_impl_.lock()) {
    session_id_ = s->GetSessionIdSharedPtr();
    send_queue_ = s->GetQueueSharedPtr(protocol);
  }

  send_priority_queue_ = FunapiQueue::Create();
  send_handshake_queue_ = FunapiQueue::Create();
  sent_queue_ = FunapiQueue::Create();
}


FunapiTransport::~FunapiTransport() {
  // DebugUtils::Log("%s", __FUNCTION__);
}


bool FunapiTransport::EncodeMessage(std::shared_ptr<FunapiMessage> message,
                                    std::vector<uint8_t> &body,
                                    const EncryptionType encryption_type) {
  HeaderFields header_fields;
  MakeHeaderFields(header_fields, body);

  // log
  // std::string body_string(body.cbegin(), body.cend());
  // DebugUtils::Log("Header to send: %s", header_string.c_str());
  // DebugUtils::Log("send message: %s", body_string.c_str());
  // //

  if (false == encrytion_->Encrypt(header_fields, body, encryption_type))
    return false;

  std::string header_string;
  MakeHeaderString(header_string, header_fields);

#ifdef DEBUG_LOG
  {
    std::stringstream ss;
    ss << "[C->S] " << TransportProtocolToString(GetProtocol()) << "/" << EncodingToString(GetEncoding()) << ": ";

    // header
    ss << "{";
    for (auto i : header_fields) {
      ss << "(" << i.first << "=" << i.second << ")";
    }
    ss << "} ";

    // body
    if (message->GetEncoding() == FunEncoding::kProtobuf) {
      ss << message->GetProtobufMessage().ShortDebugString();
    }
    else if (message->GetEncoding() == FunEncoding::kJson) {
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      message->GetJsonDocumenet().Accept(writer);

      ss << buffer.GetString();
    }

    DebugUtils::Log("%s", ss.str().c_str());
  }
#endif

  // log
  // std::string body_string(body.cbegin(), body.cend());
  // DebugUtils::Log("Header to send: %s", header_string.c_str());
  // DebugUtils::Log("send message: %s", body_string.c_str());
  // //

  body.insert(body.begin(), header_string.cbegin(), header_string.cend());

  return true;
}


bool FunapiTransport::DecodeMessage(int read_length,
                                    std::vector<uint8_t> &receiving,
                                    int &next_decoding_offset,
                                    bool &header_decoded, HeaderFields &header_fields) {
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
          receiving.erase(receiving.begin(), receiving.begin() + next_decoding_offset);
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


bool FunapiTransport::DecodeMessage(int read_length, std::vector<uint8_t> &receiving) {
  int next_decoding_offset = 0;
  bool header_decoded = false;
  HeaderFields header_fields;

  // DebugUtils::Log("Received %d bytes.", read_length);

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


bool FunapiTransport::IsReliableSession() const {
  if (auto s = session_impl_.lock()) {
    return (s->IsReliableSession() && protocol_ == TransportProtocol::kTcp);
  }

  return false;
}


void FunapiTransport::SendMessage(std::shared_ptr<FunapiMessage> message,
                                  bool priority,
                                  bool handshake) {
  PushSendQueue(message, priority, handshake);
}


void FunapiTransport::SetSeq(std::shared_ptr<FunapiMessage> message) {
  if (message->UseSeq()) {
    if (IsReliableSession() || sequence_number_validation_) {
      FunEncoding encoding = message->GetEncoding();

      uint32_t seq = 0;
      if (encoding != FunEncoding::kNone) {
        seq = seq_;
        ++seq_;

        message->SetSeq(seq);
      }

      if (encoding == FunEncoding::kJson) {
        auto &msg = message->GetJsonDocumenet();
        rapidjson::Value seq_node;
        seq_node.SetUint(seq);
        msg.AddMember(rapidjson::StringRef(kSeqNumAttributeName), seq_node, msg.GetAllocator());
      }
      else if (encoding == FunEncoding::kProtobuf) {
        auto &msg = message->GetProtobufMessage();
        msg.set_seq(seq);
      }
    }
  }
}


void FunapiTransport::SetUseFirstSessionId(const bool use) {
  first_set_session_id_ = use;
}


bool FunapiTransport::UseFirstSessionId() {
  return first_set_session_id_;
}


void FunapiTransport::SetSendSessionIdOnlyOnce(const bool once) {
  use_send_session_id_only_once_ = once;
}


bool FunapiTransport::UseSendSessionIdOnlyOnce() {
  return use_send_session_id_only_once_;
}


void FunapiTransport::SetSessionId(std::shared_ptr<FunapiMessage> message) {
  if (UseSendSessionIdOnlyOnce() && false == UseFirstSessionId()) {
    return;
  }

  std::string session_id = GetSessionId();

  if (!session_id.empty()) {
    FunEncoding encoding = message->GetEncoding();

    if (encoding == FunEncoding::kJson) {
      rapidjson::Document &msg = message->GetJsonDocumenet();

      rapidjson::Value session_id_node;
      session_id_node.SetString(rapidjson::StringRef(session_id.c_str()), msg.GetAllocator());
      msg.AddMember(rapidjson::StringRef(kSessionIdAttributeName), session_id_node, msg.GetAllocator());
    }
    else if (encoding == FunEncoding::kProtobuf) {
      auto &msg = message->GetProtobufMessage();
      msg.set_sid(session_id);
    }

    if (UseFirstSessionId() && GetProtocol() != TransportProtocol::kUdp) {
      SetUseFirstSessionId(false);
    }
  }
}


bool FunapiTransport::TryToDecodeHeader(std::vector<uint8_t> &receiving,
                                        int &next_decoding_offset,
                                        bool &header_decoded,
                                        HeaderFields &header_fields) {
  // DebugUtils::Log("Trying to decode header fields.");
  int received_size = static_cast<int>(receiving.size());
  for (; next_decoding_offset < received_size;) {
    char *base = reinterpret_cast<char *>(receiving.data());
    char *ptr =
    std::search(base + next_decoding_offset,
                base + received_size,
                kHeaderDelimeter,
                kHeaderDelimeter + strlen(kHeaderDelimeter));

    int eol_offset = static_cast<int>(ptr - base);
    if (eol_offset >= received_size) {
      // Not enough bytes. Wait for more bytes to come.
      // DebugUtils::Log("We need more bytes for a header field. Waiting.");
      return false;
    }

    // Generates a null-termianted string by replacing the delimeter with \0.
    *ptr = '\0';
    char *line = base + next_decoding_offset;
    // DebugUtils::Log("Header line: %s", line);

    size_t line_length = eol_offset - next_decoding_offset;
    next_decoding_offset = static_cast<int>(eol_offset + 1);

    if (line_length == 0) {
      // End of header.
      header_decoded = true;
      // DebugUtils::Log("End of header reached. Will decode body from now.");
      return true;
    }

    ptr = std::search(line, line + line_length, kHeaderFieldDelimeter,
                      kHeaderFieldDelimeter + strlen(kHeaderFieldDelimeter));
    assert((ptr - base) < eol_offset);

    // Generates null-terminated string by replacing the delimeter with \0.
    *ptr = '\0';
    char *e1 = line, *e2 = ptr + 1;
    while (*e2 == ' ' || *e2 == '\t') ++e2;
    // DebugUtils::Log("Decoded header field '%s' => '%s'", e1, e2);
    header_fields[e1] = e2;
  }
  return false;
}


bool FunapiTransport::TryToDecodeBody(std::vector<uint8_t> &receiving,
                                      int &next_decoding_offset,
                                      bool &header_decoded,
                                      HeaderFields &header_fields) {
  int received_size = static_cast<int>(receiving.size());
  // Version header
  HeaderFields::const_iterator it = header_fields.find(kVersionHeaderField);
  assert(it != header_fields.end());
  int version = atoi(it->second.c_str());
  assert(version == static_cast<int>(FunapiVersion::kProtocolVersion));

  // Length header
  it = header_fields.find(kLengthHeaderField);
  int body_length = atoi(it->second.c_str());
  // DebugUtils::Log("We need %d bytes for a message body.", body_length);

  if (received_size - next_decoding_offset < body_length) {
    // Need more bytes.
    // DebugUtils::Log("We need more bytes for a message body. Waiting.");
    return false;
  }

  std::vector<EncryptionType> encryption_types;

#ifdef DEBUG_LOG
  if (body_length == 0)
  {
    std::stringstream ss;
    ss << "[S->C] " << TransportProtocolToString(GetProtocol()) << "/" << EncodingToString(GetEncoding()) << ": ";

    // header
    ss << "{";
    for (auto i : header_fields) {
      ss << "(" << i.first << "=" << i.second << ")";
    }
    ss << "} ";

    DebugUtils::Log("%s", ss.str().c_str());
  }
#endif

  if (body_length > 0) {
    std::vector<uint8_t> v(receiving.begin() + next_decoding_offset, receiving.begin() + next_decoding_offset + body_length);
    encrytion_->Decrypt(header_fields, v, encryption_types);
    v.push_back('\0');

    // Moves the read offset.
    next_decoding_offset += body_length;

    // log
    // std::string temp_string(v.begin(), v.end());
    // DebugUtils::Log("recv message: %s", temp_string.c_str());
    // //

    // The network module eats the fields and invokes registered handler
    OnReceived(GetProtocol(), GetEncoding(), header_fields, v);
  }
  else {
    // init encrytion
    encrytion_->Decrypt(header_fields, receiving, encryption_types);
  }

  if (GetProtocol() == TransportProtocol::kTcp && encryption_types.size() > 0) {
    if (auto s = session_impl_.lock()) {
      for (auto type : encryption_types) {
        if (type == EncryptionType::kDefaultEncryption) {
          if (GetSessionId().empty()) {
            s->SendEmptyMessage(GetProtocol(), type);
          }
        }
        else {
          s->SendEmptyMessage(GetProtocol(), type);
        }
      }
    }

    if (reconnect_first_ack_receiving_) {
      OnSendAck(TransportProtocol::kTcp, seq_recvd_ + 1);
    }
  }

  // Prepares for a next message.
  header_decoded = false;
  header_fields.clear();

  return true;
}


std::string FunapiTransport::MakeHeaderString(const std::vector<uint8_t> &body) {
  std::stringstream ss;

  ss << kVersionHeaderField << kHeaderFieldDelimeter << static_cast<int>(FunapiVersion::kProtocolVersion) << kHeaderDelimeter;

  if (first_sending_) {
    ss << kPluginVersionHeaderField << kHeaderFieldDelimeter << static_cast<int>(FunapiVersion::kPluginVersion) << kHeaderDelimeter;
    first_sending_ = false;
  }

  ss << kLengthHeaderField << kHeaderFieldDelimeter << static_cast<unsigned long>(body.size()) << kHeaderDelimeter << kHeaderDelimeter;

  return ss.str();
}


void FunapiTransport::MakeHeaderFields(HeaderFields &header_fields,
                                       const std::vector<uint8_t> &body) {
  std::stringstream ss_protocol_version;
  ss_protocol_version << static_cast<int>(FunapiVersion::kProtocolVersion);
  header_fields[kVersionHeaderField] = ss_protocol_version.str();

  if (first_sending_) {
    first_sending_ = false;

    std::stringstream ss_plugin_version;
    ss_plugin_version << static_cast<int>(FunapiVersion::kPluginVersion);
    header_fields[kPluginVersionHeaderField] = ss_plugin_version.str();
  }

  std::stringstream ss_length;
  ss_length << static_cast<size_t>(body.size());
  header_fields[kLengthHeaderField] = ss_length.str();
}


void FunapiTransport::MakeHeaderString(std::string &header_string,
                                       const HeaderFields &header_fields) {
  std::stringstream ss;

  for (auto it : header_fields) {
    ss << it.first << kHeaderFieldDelimeter << it.second << kHeaderDelimeter;
  }

  ss << kHeaderDelimeter;

  header_string = ss.str();
}


bool FunapiTransport::IsStarted() {
  return (GetState() == TransportState::kConnected);
}


void FunapiTransport::PushSendQueue(std::shared_ptr<FunapiMessage> message,
                                    bool priority,
                                    bool handshake) {
  if (priority) {
    send_priority_queue_->PushBack(message);
  }
  else if (handshake) {
    send_handshake_queue_->PushBack(message);
  }
  else {
    send_queue_->PushBack(message);
  }
}


bool FunapiTransport::EncodeThenSendMessage(std::shared_ptr<FunapiMessage> message) {
  SetSessionId(message);
  SetSeq(message);

  return EncodeThenSendMessage(message, message->GetBody(), message->GetEncryptionType());
}


void FunapiTransport::Send(bool send_all) {
}


void FunapiTransport::SetConnectTimeout(const time_t timeout) {
  connect_timeout_seconds_ = timeout;
}


void FunapiTransport::OnTransportStarted(const TransportProtocol protocol, std::shared_ptr<FunapiError> error) {
  if (auto s = session_impl_.lock()) {
    s->OnTransportStarted(protocol, error);
  }
}


void FunapiTransport::OnTransportClosed(const TransportProtocol protocol, std::shared_ptr<FunapiError> error) {
  if (auto s = session_impl_.lock()) {
    s->OnTransportClosed(protocol, error);
  }
}


void FunapiTransport::OnTransportConnectFailed(const TransportProtocol protocol, std::shared_ptr<FunapiError> error) {
  if (auto s = session_impl_.lock()) {
    s->OnTransportConnectFailed(protocol, error);
  }
}


void FunapiTransport::OnTransportConnectTimeout(const TransportProtocol protocol, std::shared_ptr<FunapiError> error) {
  if (auto s = session_impl_.lock()) {
    s->OnTransportConnectTimeout(protocol, error);
  }
}


void FunapiTransport::OnTransportDisconnected(const TransportProtocol protocol, std::shared_ptr<FunapiError> error) {
  if (auto s = session_impl_.lock()) {
    s->OnTransportDisconnected(protocol, error);
  }
}


std::string FunapiTransport::GetSessionId() {
  return session_id_->Get(GetEncoding());
}


void FunapiTransport::SetEncryptionType(EncryptionType type) {
  if (EncryptionType::kNoneEncryption != type) {
    encrytion_->SetEncryptionType(type);
  }
}


void FunapiTransport::SetEncryptionType(EncryptionType type, const std::string &public_key) {
  if (public_key.length() > 0) {
    encrytion_->SetEncryptionType(type, public_key);
  }
  else {
    SetEncryptionType(type);
  }
}


bool FunapiTransport::OnAckReceived(const uint32_t ack) {
  ack_receiving_ = true;

  while (!sent_queue_->Empty()) {
    auto msg = sent_queue_->Front();
    if (FunapiUtil::SeqLess(msg->GetSeq(), ack)) {
      sent_queue_->PopFront();
    }
    else {
      break;
    }
  }

  if (reconnect_first_ack_receiving_) {
    reconnect_first_ack_receiving_ = false;
    PushUnsent(ack);
  }

  return true;
}


void FunapiTransport::OnSendAck(const TransportProtocol protocol, const uint32_t seq) {
  if (auto s = session_impl_.lock()) {
    s->SendAck(protocol, seq);
  }
}


bool FunapiTransport::OnSeqReceived(const uint32_t seq) {
  if (seq_receiving_ == false) {
    seq_receiving_ = true;
  }
  else {
    if (!FunapiUtil::SeqLess(seq_recvd_, seq)) {
      // DebugUtils::Log("Last sequence number is %d but %d received. Skipping message.", seq_recvd_, seq);
      return false;
    }
    else if (seq != seq_recvd_ + 1) {
      // DebugUtils::Log("Received wrong sequence number %d. %d expected.", seq, seq_recvd_ + 1);
      std::stringstream ss;
      ss << "Received wrong sequence number " << seq <<". " << (seq_recvd_ + 1) << "expected.";
      Stop(true, FunapiError::Create(FunapiError::ErrorType::kSeq, 0, ss.str()));
      return false;
    }
  }

  seq_recvd_ = seq;
  OnSendAck(GetProtocol(), seq + 1);

  return true;
}


void FunapiTransport::PushUnsent(const uint32_t ack) {
  while (!sent_queue_->Empty()) {
    auto msg = sent_queue_->Front();
    sent_queue_->PopFront();
    send_priority_queue_->PushBack(msg);
  }
}


void FunapiTransport::OnTransportReceived(const TransportProtocol protocol,
                                          const FunEncoding encoding,
                                          const HeaderFields &header,
                                          const std::vector<uint8_t> &body,
                                          const std::shared_ptr<FunapiMessage> message) {
  if (auto s = session_impl_.lock()) {
    s->OnTransportReceived(protocol, encoding, header, body, message);
  }
}


void FunapiTransport::OnReceived(const TransportProtocol protocol,
                                 const FunEncoding encoding,
                                 const HeaderFields &header,
                                 const std::vector<uint8_t> &body) {
  auto message = FunapiMessage::Create(encoding, body);

  OnTransportReceived(protocol, encoding, header, body, message);

  std::string msg_type;
  uint32_t ack = 0;
  uint32_t seq = 0;
  bool hasAck = false;
  bool hasSeq = false;

  if (encoding == FunEncoding::kJson) {
    rapidjson::Document &json = message->GetJsonDocumenet();

    if (json.HasMember(kMessageTypeAttributeName)) {
      const rapidjson::Value &msg_type_node = json[kMessageTypeAttributeName];
      assert(msg_type_node.IsString());
      msg_type = msg_type_node.GetString();
    }

    hasAck = json.HasMember(kAckNumAttributeName);
    if (hasAck) {
      ack = json[kAckNumAttributeName].GetUint();
      json.RemoveMember(kAckNumAttributeName);
    }

    hasSeq = json.HasMember(kSeqNumAttributeName);
    if (hasSeq) {
      seq = json[kSeqNumAttributeName].GetUint();
      json.RemoveMember(kSeqNumAttributeName);
    }
  } else if (encoding == FunEncoding::kProtobuf) {
    FunMessage &proto = message->GetProtobufMessage();

    msg_type = proto.msgtype();

    hasAck = proto.has_ack();
    ack = proto.ack();

    hasSeq = proto.has_seq();
    seq = proto.seq();
  }

  if (IsReliableSession()) {
    if (hasAck) {
      OnAckReceived(ack);
      return;
    }

    if (hasSeq) {
      if (!OnSeqReceived(seq))
        return;
    }
  }

  if (msg_type.compare(kClientPingMessageType) == 0) {
    ResetClientPingTimeout();
  }
}


void FunapiTransport::OnDisconnecting(std::shared_ptr<FunapiError> error) {
  SetState(TransportState::kDisconnected);

  OnTransportClosed(GetProtocol(), error);

  if (auto s = session_impl_.lock()) {
    s->CheckRedirect();
  }
}


void FunapiTransport::ResetClientPingTimeout() {
}


void FunapiTransport::PushNetworkThreadTask(const FunapiThread::TaskHandler &handler) {
  if (auto s = session_impl_.lock()) {
    std::weak_ptr<FunapiTransport> weak = shared_from_this();
    s->PushNetworkThreadTask([weak, this, handler]()->bool{
      if (auto t = weak.lock()) {
        return handler();
      }

      return true;
    });
  }
}


void FunapiTransport::SetState(TransportState state) {
  std::unique_lock<std::mutex> lock(state_mutex_);
  state_ = state;
}


TransportState FunapiTransport::GetState() {
  std::unique_lock<std::mutex> lock(state_mutex_);
  return state_;
}


void FunapiTransport::Stop(bool use_force, std::shared_ptr<FunapiError> error) {
  if (GetState() == TransportState::kDisconnected)
    return;

  SetState(TransportState::kDisconnecting);

  if (send_queue_->Empty() || use_force) {
    PushNetworkThreadTask([this, error]()->bool {
      OnDisconnecting(error);
      return true;
    });
  }
}


void FunapiTransport::Update() {
}

////////////////////////////////////////////////////////////////////////////////
// FunapiTcpTransport implementation.

class FunapiTcpTransport : public FunapiTransport {
 public:
  FunapiTcpTransport(std::weak_ptr<FunapiSessionImpl> session, TransportProtocol protocol,
                     const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding);
  virtual ~FunapiTcpTransport();

  static std::shared_ptr<FunapiTcpTransport> Create(std::weak_ptr<FunapiSessionImpl> session,
                                                    const std::string &hostname_or_ip,
                                                    uint16_t port,
                                                    FunEncoding encoding);

  TransportProtocol GetProtocol();

  void Start();
  void SetDisableNagle(const bool disable_nagle);
  void SetAutoReconnect(const bool auto_reconnect);
  void SetSequenceNumberValidation(const bool validation);
  void SetEnablePing(const bool enable_ping);
  void ResetClientPingTimeout();

  bool UseSodium();
  bool SendHandShakeMessages();

  void Update();
  void Send(bool send_all = false);

 protected:
  bool EncodeThenSendMessage(std::shared_ptr<FunapiMessage> message,
                             std::vector<uint8_t> &body,
                             const EncryptionType encryption_type);
  void Connect();
  void Connect(struct addrinfo *addrinfo_res);
  void OnConnectCompletion(const bool isFailed, const bool isTimedOut, const int error_code, const std::string &error_string, struct addrinfo *addrinfo_res);

  void OnDisconnecting(std::shared_ptr<FunapiError> error = nullptr);

 private:
  // Ping message-related constants.
  static const time_t kPingIntervalSecond = 3;
  static const time_t kPingTimeoutSeconds = 20;

  static const int kMaxReconnectCount = 3;
  static const time_t kMaxReconnectWaitSeconds = 10;

  enum class UpdateState : int {
    kNone = 0,
    kPing,
    kWaitForAutoReconnect,
  };
  UpdateState update_state_ = UpdateState::kNone;
  std::mutex update_state_mutex_;

  void SetUpdateState(FunapiTcpTransport::UpdateState state);
  FunapiTcpTransport::UpdateState GetUpdateState();

  void Ping();
  void WaitForAutoReconnect();

  int reconnect_count_ = 0;
  FunapiTimer reconnect_wait_timer_;
  time_t reconnect_wait_seconds_;

  int offset_ = 0;
  std::vector<uint8_t> receiving_vector_;
  int next_decoding_offset_ = 0;
  bool header_decoded_ = false;
  HeaderFields header_fields_;

  bool disable_nagle_ = true;
  bool auto_reconnect_ = false;
  bool enable_ping_ = false;

  FunapiTimer client_ping_timeout_timer_;
  FunapiTimer ping_send_timer_;

  std::function<bool(const TransportProtocol protocol)> send_client_ping_message_handler_;

  std::shared_ptr<FunapiTcp> tcp_;
  std::vector<uint8_t> send_buffer_;
  struct addrinfo *addrinfo_res_ = nullptr;
};


FunapiTcpTransport::FunapiTcpTransport(std::weak_ptr<FunapiSessionImpl> session,
                                       TransportProtocol protocol,
                                       const std::string &hostname_or_ip,
                                       uint16_t port,
                                       FunEncoding encoding)
: FunapiTransport(session, protocol, hostname_or_ip, port, encoding) {
}


FunapiTcpTransport::~FunapiTcpTransport() {
  // DebugUtils::Log("%s", __FUNCTION__);
}


std::shared_ptr<FunapiTcpTransport> FunapiTcpTransport::Create
  (std::weak_ptr<FunapiSessionImpl> session,
   const std::string &hostname_or_ip,
   uint16_t port,
   FunEncoding encoding) {
    return std::make_shared<FunapiTcpTransport>(session, TransportProtocol::kTcp, hostname_or_ip, port, encoding);
}


TransportProtocol FunapiTcpTransport::GetProtocol() {
  return TransportProtocol::kTcp;
}


void FunapiTcpTransport::SetUpdateState(FunapiTcpTransport::UpdateState state) {
  std::unique_lock<std::mutex> lock(update_state_mutex_);
  update_state_ = state;
}


FunapiTcpTransport::UpdateState FunapiTcpTransport::GetUpdateState() {
  std::unique_lock<std::mutex> lock(update_state_mutex_);
  return update_state_;
}


void FunapiTcpTransport::Start() {
  if (GetState() != TransportState::kDisconnected)
    return;

  SetState(TransportState::kConnecting);
  SetUpdateState(UpdateState::kNone);

  PushNetworkThreadTask([this]()->bool {
    reconnect_count_ = 0;
    reconnect_wait_seconds_ = 1;

    Connect();

    return true;
  });
}


void FunapiTcpTransport::OnDisconnecting(std::shared_ptr<FunapiError> error) {
  tcp_ = nullptr;

  if (ack_receiving_) {
    reconnect_first_ack_receiving_ = true;
  }

  FunapiTransport::OnDisconnecting(error);
}


void FunapiTcpTransport::Ping() {
  if (enable_ping_) {
    if (ping_send_timer_.IsExpired()){
      ping_send_timer_.SetTimer(kPingIntervalSecond);

      if (auto s = session_impl_.lock()) {
        s->SendClientPingMessage(GetProtocol());
      }
    }

    if (client_ping_timeout_timer_.IsExpired()) {
      // DebugUtils::Log("Network seems disabled. Stopping the transport.");
      Stop(true, FunapiError::Create(FunapiError::ErrorType::kPing, 0, "Network seems disabled. Stopping the transport."));
      return;
    }
  }
}


void FunapiTcpTransport::WaitForAutoReconnect() {
  if (reconnect_wait_timer_.IsExpired()) {
    reconnect_wait_seconds_ *= 2;
    if (kMaxReconnectWaitSeconds < reconnect_wait_seconds_) {
      reconnect_wait_seconds_ = kMaxReconnectWaitSeconds;
    }

    PushNetworkThreadTask([this]()->bool{
      Connect(addrinfo_res_);
      return true;
    });
  }
}


void FunapiTcpTransport::SetDisableNagle(const bool disable_nagle) {
  disable_nagle_ = disable_nagle;
}


void FunapiTcpTransport::SetAutoReconnect(const bool auto_reconnect) {
  auto_reconnect_ = auto_reconnect;
}


void FunapiTcpTransport::SetEnablePing(const bool enable_ping) {
  enable_ping_ = enable_ping;
}


void FunapiTcpTransport::ResetClientPingTimeout() {
  client_ping_timeout_timer_.SetTimer(kPingTimeoutSeconds);
}


bool FunapiTcpTransport::EncodeThenSendMessage(std::shared_ptr<FunapiMessage> message,
                                               std::vector<uint8_t> &body,
                                               const EncryptionType encryption_type) {
  if (!EncodeMessage(message, body, encryption_type)) {
    return false;
  }

  send_buffer_.insert(send_buffer_.end(), body.cbegin(), body.cend());
  return true;
}


void FunapiTcpTransport::OnConnectCompletion(const bool isFailed,
                                             const bool isTimedOut,
                                             const int error_code,
                                             const std::string &error_string,
                                             struct addrinfo *addrinfo_res) {
  addrinfo_res_ = addrinfo_res;
  std::string hostname_or_ip = FunapiSocket::GetStringFromAddrInfo(addrinfo_res);

  if (isFailed) {
    SetUpdateState(UpdateState::kNone);
    SetState(TransportState::kDisconnected);

    if (isTimedOut) {
      DebugUtils::Log("Tcp connection timed out: %s (%d) %s", hostname_or_ip.c_str(), error_code, error_string.c_str());
      OnTransportConnectTimeout(TransportProtocol::kTcp, FunapiError::Create(FunapiError::ErrorType::kSocket, error_code, error_string));
    }
    else
    {
      DebugUtils::Log("Failed to tcp connection: %s (%d) %s", hostname_or_ip.c_str(), error_code, error_string.c_str());
      OnTransportConnectFailed(TransportProtocol::kTcp, FunapiError::Create(FunapiError::ErrorType::kSocket, error_code, error_string));
    }

    if (auto_reconnect_) {
      ++reconnect_count_;

      if (kMaxReconnectCount < reconnect_count_) {
        addrinfo_res_ = addrinfo_res_->ai_next;
        reconnect_count_ = 0;
        reconnect_wait_seconds_ = 1;

        Connect(addrinfo_res_);
      }
      else {
        reconnect_wait_timer_.SetTimer(reconnect_wait_seconds_);
        DebugUtils::Log("Wait %d seconds for connect to Tcp transport.", static_cast<int>(reconnect_wait_seconds_));
        SetUpdateState(UpdateState::kWaitForAutoReconnect);
      }
    }
  }
  else {
    client_ping_timeout_timer_.SetTimer(kPingIntervalSecond + kPingTimeoutSeconds);
    ping_send_timer_.SetTimer(kPingIntervalSecond);

    SetUpdateState(UpdateState::kPing);
    SetState(TransportState::kConnected);

    OnTransportStarted(TransportProtocol::kTcp);
  }
}


void FunapiTcpTransport::Connect() {
  SetUpdateState(UpdateState::kNone);
  SetState(TransportState::kConnecting);

  // Tries to connect.
  DebugUtils::Log("Try to tcp connect to server: %s %d", hostname_or_ip_.c_str(), port_);

  if (GetSessionId().empty()) {
    SetUseFirstSessionId(false);
  }
  else {
    SetUseFirstSessionId(true);
  }

  tcp_ = FunapiTcp::Create();
  std::weak_ptr<FunapiTransport> weak = shared_from_this();
  tcp_->Connect(hostname_or_ip_.c_str(),
                port_,
                connect_timeout_seconds_,
                disable_nagle_,
                [weak, this](const bool is_failed,
                             const bool is_timed_out,
                             const int error_code,
                             const std::string &error_string,
                             struct addrinfo *addrinfo_res)
  {
    if (auto t = weak.lock()) {
      OnConnectCompletion(is_failed, is_timed_out, error_code, error_string, addrinfo_res);
    }
  }, [weak, this]() {
    if (auto t = weak.lock()) {
      Send();
    }
  }, [weak, this](const bool is_failed,
                  const int error_code,
                  const std::string &error_string,
                  const int read_length, std::vector<uint8_t> &receiving)
  {
    if (auto t = weak.lock()) {
      if (is_failed) {
        /*
        if (error_code != 0) {
          DebugUtils::Log("Tcp recv error : (%d) %s", error_code, error_string.c_str());
        }
        */
        auto error = FunapiError::Create(FunapiError::ErrorType::kSocket, error_code, error_string);
        OnTransportDisconnected(TransportProtocol::kTcp, error);
        Stop(true, error);
      }
      else {
        receiving_vector_.insert(receiving_vector_.end(), receiving.cbegin(), receiving.cbegin() + read_length);
        DecodeMessage(read_length, receiving_vector_, next_decoding_offset_, header_decoded_, header_fields_);
      }
    }
  });
}


void FunapiTcpTransport::Connect(struct addrinfo *addrinfo_res) {
  SetState(TransportState::kConnecting);

  if (addrinfo_res && tcp_) {
    std::weak_ptr<FunapiTransport> weak = shared_from_this();
    tcp_->Connect
    (addrinfo_res,
     [weak, this]
     (const bool is_failed,
      const bool is_timed_out,
      const int error_code,
      const std::string &error_string,
      struct addrinfo *ai_res)
    {
      if (auto t = weak.lock()) {
        OnConnectCompletion(is_failed, is_timed_out, error_code, error_string, ai_res);
      }
    });
  }
}


void FunapiTcpTransport::Update() {
  auto state = GetUpdateState();
  if (state == UpdateState::kPing) {
    Ping();
  }
  else if (state == UpdateState::kWaitForAutoReconnect) {
    WaitForAutoReconnect();
  }
}


void FunapiTcpTransport::SetSequenceNumberValidation(const bool validation) {
  sequence_number_validation_ = validation;
}


bool FunapiTcpTransport::UseSodium() {
  return encrytion_->UseSodium();
}


bool FunapiTcpTransport::SendHandShakeMessages() {
  bool use_send = false;

  if (auto s = session_impl_.lock()) {
    for (auto type
         : {EncryptionType::kChacha20Encryption,
           EncryptionType::kAes128Encryption}) {
             if (encrytion_->HasEncryption(type)) {
               if (false == encrytion_->IsHandShakeCompleted(type))
               {
                 use_send = true;
                 s->SendEmptyMessage(TransportProtocol::kTcp, type);
               }
             }
           }

    if (use_send) {
      s->SendEmptyMessage(TransportProtocol::kTcp);
    }
  }

  return use_send;
}


void FunapiTcpTransport::Send(bool send_all) {
  send_buffer_.resize(0);
  std::shared_ptr<FunapiMessage> msg;

  if (!send_handshake_queue_->Empty()) {
    while (!send_handshake_queue_->Empty()) {
      msg = send_handshake_queue_->Front();
      if (!FunapiTransport::EncodeThenSendMessage(msg)) {
        return;
      }
      else {
        send_handshake_queue_->PopFront();
      }
    }
  }
  else if (!send_priority_queue_->Empty()) {
    if (false == encrytion_->IsHandShakeCompleted()) {
      return;
    }

    msg = send_priority_queue_->Front();
    if (!FunapiTransport::EncodeThenSendMessage(msg)) {
      return;
    }
    else {
      send_priority_queue_->PopFront();
    }
  }
  else {
    if (!GetSessionId().empty() &&
        !reconnect_first_ack_receiving_ &&
        encrytion_->IsHandShakeCompleted()) {
      size_t send_count = 0;

      while (true) {
        if (send_queue_->Empty()) {
          break;
        }
        else {
          msg = send_queue_->Front();
        }

        if (FunapiTransport::EncodeThenSendMessage(msg)) {
          send_queue_->PopFront();
          if (msg->UseSentQueue()) {
            sent_queue_->PushBack(msg);
          }
        } else {
          break;
        }

        ++send_count;
        if (send_count >= kMaxSend && send_all == false)
          break;
      }
    }
  }

  if (!send_buffer_.empty()) {
    std::weak_ptr<FunapiTransport> weak = shared_from_this();
    tcp_->Send(send_buffer_,
               [weak, this](const bool is_failed,
                            const int error_code,
                            const std::string &error_string,
                            const int sent_length)
    {
      if (auto t = weak.lock()) {
        if (is_failed) {
          /*
          if (error_code != 0) {
            DebugUtils::Log("Tcp send error: (%d) %s", error_code, error_string.c_str());
          }
          */
          auto error = FunapiError::Create(FunapiError::ErrorType::kSocket, error_code, error_string);
          OnTransportDisconnected(TransportProtocol::kTcp, error);
          Stop(true, error);
        }
        else {
          // DebugUtils::Log("Sent %d bytes", sent_length);
        }

        if (GetState() == TransportState::kDisconnecting && send_queue_->Empty()) {
          OnDisconnecting();
        }
      }
    });
  }
}


////////////////////////////////////////////////////////////////////////////////
// FunapiUdpTransport implementation.

class FunapiUdpTransport : public FunapiTransport {
 public:
  FunapiUdpTransport(std::weak_ptr<FunapiSessionImpl> session,
                     TransportProtocol protocol,
                     const std::string &hostname_or_ip,
                     uint16_t port,
                     FunEncoding encoding);
  virtual ~FunapiUdpTransport();

  static std::shared_ptr<FunapiUdpTransport> Create(std::weak_ptr<FunapiSessionImpl> session,
                                                    const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding);

  TransportProtocol GetProtocol();

  void Start();
  void Send(bool send_all = false);

 protected:
  bool EncodeThenSendMessage(std::shared_ptr<FunapiMessage> message,
                             std::vector<uint8_t> &body,
                             const EncryptionType encryption_type);
  void OnDisconnecting(std::shared_ptr<FunapiError> error = nullptr);

 private:
  std::shared_ptr<FunapiUdp> udp_;
};


FunapiUdpTransport::FunapiUdpTransport(std::weak_ptr<FunapiSessionImpl> session,
                                       TransportProtocol protocol,
                                       const std::string &hostname_or_ip,
                                       uint16_t port,
                                       FunEncoding encoding)
: FunapiTransport(session, protocol, hostname_or_ip, port, encoding) {
}


FunapiUdpTransport::~FunapiUdpTransport() {
  // DebugUtils::Log("%s", __FUNCTION__);
}


std::shared_ptr<FunapiUdpTransport> FunapiUdpTransport::Create(std::weak_ptr<FunapiSessionImpl> session,
                                                               const std::string &hostname_or_ip,
                                                               uint16_t port,
                                                               FunEncoding encoding) {
  return std::make_shared<FunapiUdpTransport>(session,
                                              TransportProtocol::kUdp,
                                              hostname_or_ip,
                                              port,
                                              encoding);
}


TransportProtocol FunapiUdpTransport::GetProtocol() {
  return TransportProtocol::kUdp;
}


void FunapiUdpTransport::Start() {
  if (GetState() != TransportState::kDisconnected)
    return;

  SetState(TransportState::kConnecting);

  std::weak_ptr<FunapiTransport> weak = shared_from_this();
  PushNetworkThreadTask([weak, this]()->bool {
    if (auto t = weak.lock())
    {
      SetUseFirstSessionId(true);
      udp_ = FunapiUdp::Create
      (hostname_or_ip_.c_str(),
       port_,
       [weak, this]
       (const bool isFailed,
        const int error_code,
        const std::string &error_string)
       {
         if (auto t2 = weak.lock()) {
           if (isFailed) {
             // DebugUtils::Log("Udp socket failed: (%d) %s", error_code, error_string.c_str());
             Stop(true, FunapiError::Create(FunapiError::ErrorType::kSocket, error_code, error_string));
           }
           else {
             SetState(TransportState::kConnected);
             OnTransportStarted(TransportProtocol::kUdp);
           }
         }
       }, [weak, this]()
       {
         if (auto t2 = weak.lock()) {
           Send();
         }
       },[weak, this]
       (const bool isFailed,
        const int error_code,
        const std::string &error_string,
        const int read_length,
        std::vector<uint8_t> &receiving)
       {
         if (auto t2 = weak.lock()) {
           if (isFailed) {
             // DebugUtils::Log("Udp recvfrom error : (%d) %s", error_code, error_string.c_str());
             Stop(true, FunapiError::Create(FunapiError::ErrorType::kSocket, error_code, error_string));
           }
           else {
             DecodeMessage(read_length, receiving);
           }
         }
       });
    }

    return true;
  });
}


void FunapiUdpTransport::OnDisconnecting(std::shared_ptr<FunapiError> error) {
  udp_ = nullptr;

  FunapiTransport::OnDisconnecting(error);
}


bool FunapiUdpTransport::EncodeThenSendMessage(std::shared_ptr<FunapiMessage> message,
                                               std::vector<uint8_t> &body,
                                               const EncryptionType encryption_type) {
  if (!EncodeMessage(message, body, encryption_type)) {
    return false;
  }

  bool bRet = false;

  std::weak_ptr<FunapiTransport> weak = shared_from_this();
  udp_->Send
  (body,
   [weak, this, &bRet]
   (const bool is_failed,
    const int error_code,
    const std::string &error_string,
    const int sent_length)
  {
    if (auto t = weak.lock()) {
      if (is_failed) {
        Stop(true, FunapiError::Create(FunapiError::ErrorType::kSocket, error_code, error_string));
      }
      else {
        bRet = true;
        // DebugUtils::Log("Sent %d bytes", sent_length);
      }
    }
  });

  return bRet;
}


void FunapiUdpTransport::Send(bool send_all) {
  std::shared_ptr<FunapiMessage> msg;

  while (!send_handshake_queue_->Empty())
  {
    msg = send_handshake_queue_->Front();

    if (FunapiTransport::EncodeThenSendMessage(msg)) {
      send_handshake_queue_->PopFront();
    }
    else {
      return;
    }
  }

  if (GetSessionId().empty()) {
    return;
  }

  size_t send_count = 0;

  while (!send_queue_->Empty())
  {
    msg = send_queue_->Front();

    if (FunapiTransport::EncodeThenSendMessage(msg)) {
      send_queue_->PopFront();
    }
    else {
      break;
    }

    ++send_count;
    if (send_count >= kMaxSend && send_all == false)
      break;
  }

  if (GetState() == TransportState::kDisconnecting) {
    if (send_queue_->Empty()) {
      OnDisconnecting();
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
// FunapiHttpTransport implementation.

class FunapiHttpTransport : public FunapiTransport {
 public:
  FunapiHttpTransport(std::weak_ptr<FunapiSessionImpl> session,
                      const std::string &hostname_or_ip, uint16_t port, bool https, FunEncoding encoding);
  virtual ~FunapiHttpTransport();

  static std::shared_ptr<FunapiHttpTransport> Create(std::weak_ptr<FunapiSessionImpl> session,
                                                     const std::string &hostname_or_ip, uint16_t port, bool https, FunEncoding encoding);

  TransportProtocol GetProtocol();

  void Start();

  void Update();

  void SetSequenceNumberValidation(const bool validation);
  void SetCACertFilePath(const std::string &path);

  void Send(bool send_all = false);

 protected:
  bool EncodeThenSendMessage(std::shared_ptr<FunapiMessage> message,
                             std::vector<uint8_t> &body,
                             const EncryptionType encryption_type);
  void OnDisconnecting(std::shared_ptr<FunapiError> error = nullptr);

 private:
  void WebResponseHeaderCb(const void *data, int len, HeaderFields &header_fields);
  void WebResponseBodyCb(const void *data, int len, std::vector<uint8_t> &receiving);

  std::string host_url_;
  std::string cookie_;

  std::shared_ptr<FunapiHttp> http_ = nullptr;
  std::string cert_file_path_;

  std::shared_ptr<FunapiThread> http_thread_ = nullptr;
};


FunapiHttpTransport::FunapiHttpTransport(std::weak_ptr<FunapiSessionImpl> session,
                                         const std::string &hostname_or_ip,
                                         uint16_t port, bool https, FunEncoding encoding)
: FunapiTransport(session, TransportProtocol::kHttp, hostname_or_ip, port, encoding) {
  std::stringstream ss_url;
  ss_url << (https ? "https" : "http") << "://" << hostname_or_ip << ":" << port << "/v" << static_cast<int>(FunapiVersion::kProtocolVersion) << "/";
  host_url_ = ss_url.str();

  // DebugUtils::Log("Host url : %s", host_url_.c_str());

  http_thread_ = FunapiThread::Get("_http");
}


FunapiHttpTransport::~FunapiHttpTransport() {
  // DebugUtils::Log("%s", __FUNCTION__);
}


std::shared_ptr<FunapiHttpTransport> FunapiHttpTransport::Create(std::weak_ptr<FunapiSessionImpl> session,
                                                                 const std::string &hostname_or_ip,
                                                                 uint16_t port, bool https, FunEncoding encoding) {
  return std::make_shared<FunapiHttpTransport>(session, hostname_or_ip, port, https, encoding);
}


TransportProtocol FunapiHttpTransport::GetProtocol() {
  return TransportProtocol::kHttp;
}


void FunapiHttpTransport::Start() {
  if (GetState() != TransportState::kDisconnected)
    return;

  SetState(TransportState::kConnecting);

  if (http_thread_) {
    std::weak_ptr<FunapiTransport> weak = shared_from_this();
    http_thread_->Push([weak, this]()->bool {
      if (auto t = weak.lock()) {
        http_ = FunapiHttp::Create(cert_file_path_);
        http_->SetConnectTimeout(static_cast<long>(connect_timeout_seconds_));

        SetState(TransportState::kConnected);

        OnTransportStarted(TransportProtocol::kHttp);
      }

      return true;
    });
  }
}


void FunapiHttpTransport::OnDisconnecting(std::shared_ptr<FunapiError> error) {
  FunapiTransport::OnDisconnecting(error);
}


void FunapiHttpTransport::Send(bool send_all) {
  std::shared_ptr<FunapiMessage> msg;

  if (!send_handshake_queue_->Empty()) {
    msg = send_handshake_queue_->Front();

    if (FunapiTransport::EncodeThenSendMessage(msg)) {
      send_handshake_queue_->PopFront();
    }
  }
  else {
    if (GetSessionId().empty()) {
      return;
    }

    if (send_queue_->Empty()) {
      return;
    }
    else {
      msg = send_queue_->Front();
      if (FunapiTransport::EncodeThenSendMessage(msg)) {
        send_queue_->PopFront();
      }
    }
  }

  if (GetState() == TransportState::kDisconnecting && send_queue_->Empty()) {
    OnDisconnecting();
  }
}


bool FunapiHttpTransport::EncodeThenSendMessage(std::shared_ptr<FunapiMessage> message,
                                                std::vector<uint8_t> &body,
                                                const EncryptionType encryption_type) {
  if (GetState() == TransportState::kDisconnected) return false;

  HeaderFields header_fields_for_send;
  MakeHeaderFields(header_fields_for_send, body);

  encrytion_->Encrypt(header_fields_for_send, body, encryption_type);
  encrytion_->SetHeaderFieldsForHttpSend(header_fields_for_send);

  if (!cookie_.empty()) {
    header_fields_for_send[kCookieRequestHeaderField] = cookie_;
  }

#ifdef DEBUG_LOG
  {
    std::stringstream ss;
    ss << "[C->S] " << TransportProtocolToString(GetProtocol()) << "/" << EncodingToString(GetEncoding()) << ": ";

    // header
    ss << "{";
    for (auto i : header_fields_for_send) {
      ss << "(" << i.first << "=" << i.second << ")";
    }
    ss << "} ";

    // body
    if (message->GetEncoding() == FunEncoding::kProtobuf) {
      ss << message->GetProtobufMessage().ShortDebugString();
    }
    else if (message->GetEncoding() == FunEncoding::kJson) {
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      message->GetJsonDocumenet().Accept(writer);

      ss << buffer.GetString();
    }

    DebugUtils::Log("%s", ss.str().c_str());
  }
#endif

  bool is_ok = true;
  std::weak_ptr<FunapiTransport> weak = shared_from_this();
  http_->PostRequest(host_url_,
                     header_fields_for_send,
                     body,
                     [weak, this, &is_ok](int code,
                                          const std::string error_string)
  {
    if (auto t = weak.lock()) {
      // DebugUtils::Log("Error from cURL: %d, %s", code, error_string.c_str());

      is_ok = false;

      if (GetSessionId().empty()) {
        send_handshake_queue_ = FunapiQueue::Create();
      }

      Stop(true, FunapiError::Create(FunapiError::ErrorType::kCurl, code, error_string));
    }
  }, [weak, this](const std::vector<std::string> &v_header,
                  const std::vector<uint8_t> &v_body)
  {
    if (auto t = weak.lock()) {
      HeaderFields header_fields;
      auto temp_header = const_cast<std::vector<std::string>&>(v_header);
      auto temp_body = const_cast<std::vector<uint8_t>&>(v_body);

      for (size_t i=1;i<temp_header.size();++i) {
        WebResponseHeaderCb(temp_header[i].c_str(), static_cast<int>(temp_header[i].length()), header_fields);
      }

      // std::to_string is not supported on android, using std::stringstream instead.
      std::stringstream ss_protocol_version;
      ss_protocol_version << static_cast<int>(FunapiVersion::kProtocolVersion);
      header_fields[kVersionHeaderField] = ss_protocol_version.str();

      std::stringstream ss_version_header_field;
      ss_version_header_field << temp_body.size();
      header_fields[kLengthHeaderField] = ss_version_header_field.str();

      // cookie
      auto it = header_fields.find(kCookieResponseHeaderField);
      if (it != header_fields.end()) {
        cookie_ = it->second;
      }

      encrytion_->SetHeaderFieldsForHttpRecv(header_fields);

      bool header_decoded = true;
      int next_decoding_offset = 0;
      TryToDecodeBody(temp_body, next_decoding_offset, header_decoded, header_fields);
    }
  });

  return is_ok;
}


void FunapiHttpTransport::WebResponseHeaderCb(const void *data, int len, HeaderFields &header_fields) {
  std::vector<uint8_t> receiving(len);
  memcpy(receiving.data(), data, len);
  char *buf = reinterpret_cast<char*>(receiving.data());

  buf[len-2] = '\0';

  char *ptr = std::search(buf, buf + len, kHeaderFieldDelimeter,
                          kHeaderFieldDelimeter + strlen(kHeaderFieldDelimeter));
  size_t eol_offset = ptr - buf;
  if (eol_offset >= (size_t)len)
    return;

  // Generates null-terminated string by replacing the delimeter with \0.
  *ptr = '\0';
  const char *e1 = buf, *e2 = ptr + 1;
  while (*e2 == ' ' || *e2 == '\t') ++e2;
  // DebugUtils::Log("Decoded header field '%s' => '%s'", e1, e2);

  std::string cookie_field(kCookieResponseHeaderField);
  bool is_cookie_field = false;

  if (strlen(e1) == cookie_field.length()) {
    std::string temp(e1);
    std::transform(temp.begin(), temp.end(), temp.begin(), ::toupper);

    if (temp.compare(cookie_field) == 0) {
      is_cookie_field = true;
    }
  }

  if (is_cookie_field) {
    header_fields[cookie_field] = e2;
  }
  else {
    header_fields[e1] = e2;
  }
}


void FunapiHttpTransport::WebResponseBodyCb(const void *data, int len, std::vector<uint8_t> &receiving) {
  receiving.insert(receiving.end(), (uint8_t*)data, (uint8_t*)data + len);
}


void FunapiHttpTransport::Update() {
  if (false == send_queue_->Empty()) {
    if (http_thread_)
    {
      std::weak_ptr<FunapiTransport> weak = shared_from_this();
      http_thread_->Push([weak, this]()->bool {
        if (auto t = weak.lock()) {
          if (GetState() != TransportState::kDisconnected) {
            Send();
          }
        }

        return true;
      });
    }
  }
}


void FunapiHttpTransport::SetSequenceNumberValidation(const bool validation) {
  sequence_number_validation_ = validation;
}


void FunapiHttpTransport::SetCACertFilePath(const std::string &path) {
  cert_file_path_ = path;
}

////////////////////////////////////////////////////////////////////////////////
// FunapiSessionImpl implementation.

std::vector<std::weak_ptr<FunapiSessionImpl>> FunapiSessionImpl::vec_sessions_;
std::mutex FunapiSessionImpl::vec_sessions_mutex_;


std::vector<std::shared_ptr<FunapiSessionImpl>> FunapiSessionImpl::GetSessionImpls() {
  std::vector<std::shared_ptr<FunapiSessionImpl>> v_sessions;
  std::vector<std::weak_ptr<FunapiSessionImpl>> v_weak_sessions;
  {
    std::unique_lock<std::mutex> lock(vec_sessions_mutex_);
    if (!vec_sessions_.empty()) {
      for (auto i : vec_sessions_) {
        if (auto s = i.lock()) {
          v_sessions.push_back(s);
          v_weak_sessions.push_back(i);
        }
      }

      vec_sessions_.swap(v_weak_sessions);
    }
  }

  return v_sessions;
}


void FunapiSessionImpl::Add(std::shared_ptr<FunapiSessionImpl> s) {
  std::unique_lock<std::mutex> lock(vec_sessions_mutex_);
  vec_sessions_.push_back(s);
}


FunapiSessionImpl::FunapiSessionImpl(const char* hostname_or_ip,
                                     std::shared_ptr<FunapiSessionOption> option)
: hostname_or_ip_(hostname_or_ip), token_(""), session_option_(option) {
  Initialize();
}


FunapiSessionImpl::~FunapiSessionImpl() {
  // DebugUtils::Log("%s", __FUNCTION__);
}


void FunapiSessionImpl::Initialize() {
  transports_.resize(3);

  send_queues_.resize(3);
  send_queues_[static_cast<int>(TransportProtocol::kTcp)] = FunapiQueue::Create();
  send_queues_[static_cast<int>(TransportProtocol::kUdp)] = FunapiQueue::Create();
  send_queues_[static_cast<int>(TransportProtocol::kHttp)] = FunapiQueue::Create();

  // Installs event handlers.
  // session
  message_handlers_[kSessionOpenedMessageType] =
    [this](const TransportProtocol &p,
           const std::string &s,
           const std::vector<uint8_t> &v,
           const std::shared_ptr<FunapiMessage> m)
  {
    OnSessionOpen(p, s, v, m);
  };
  message_handlers_[kSessionClosedMessageType] =
    [this](const TransportProtocol &p,
           const std::string &s,
           const std::vector<uint8_t> &v,
           const std::shared_ptr<FunapiMessage> m)
  {
    OnSessionClose(p, s, v, m);
  };

  // ping
  message_handlers_[kServerPingMessageType] =
    [this](const TransportProtocol &p,
           const std::string &s,
           const std::vector<uint8_t> &v,
           const std::shared_ptr<FunapiMessage> m)
  {
    OnServerPingMessage(p, s, v, m);
  };
  message_handlers_[kClientPingMessageType] =
    [this](const TransportProtocol &p,
           const std::string &s,
           const std::vector<uint8_t> &v,
           const std::shared_ptr<FunapiMessage> m)
  {
    OnClientPingMessage(p, s, v, m);
  };

  // redirect
  message_handlers_[kRedirectMessageType] =
    [this](const TransportProtocol &p,
           const std::string &s,
           const std::vector<uint8_t> &v,
           const std::shared_ptr<FunapiMessage> m)
  {
    OnRedirectMessage(p, s, v, m);
  };
  message_handlers_[kRedirectConnectMessageType] =
    [this](const TransportProtocol &p,
           const std::string &s,
           const std::vector<uint8_t> &v,
           const std::shared_ptr<FunapiMessage> m)
  {
    OnRedirectConnectMessage(p, s, v, m);
  };

  tasks_ = FunapiTasks::Create();

  session_id_ = FunapiSessionId::Create();

  network_thread_ = FunapiThread::Get("_network");
}


void FunapiSessionImpl::Connect(const std::weak_ptr<FunapiSession>& session, const TransportProtocol protocol, int port, FunEncoding encoding, std::shared_ptr<FunapiTransportOption> option) {
  session_ = session;

  if (HasTransport(protocol))
  {
    Connect(protocol);
  }
  else {
    std::shared_ptr<FunapiTransport> transport = nullptr;

    if (IsRedirecting() == false && option == nullptr && transport_option_handler_) {
      option = transport_option_handler_(protocol, "");
    }

    if (protocol == TransportProtocol::kTcp) {
      transport = FunapiTcpTransport::Create(shared_from_this(), hostname_or_ip_, static_cast<uint16_t>(port), encoding);

      transport->SetSendSessionIdOnlyOnce(session_option_->GetSendSessionIdOnlyOnce());

      if (option) {
        tcp_option_ = std::static_pointer_cast<FunapiTcpTransportOption>(option);
        auto tcp_transport = std::static_pointer_cast<FunapiTcpTransport>(transport);

        tcp_transport->SetAutoReconnect(tcp_option_->GetAutoReconnect());
        tcp_transport->SetEnablePing(tcp_option_->GetEnablePing());
        tcp_transport->SetDisableNagle(tcp_option_->GetDisableNagle());
        tcp_transport->SetConnectTimeout(tcp_option_->GetConnectTimeout());
        tcp_transport->SetSequenceNumberValidation(tcp_option_->GetSequenceNumberValidation());
        auto encryption_types = tcp_option_->GetEncryptionTypes();
        for (auto type : encryption_types) {
          tcp_transport->SetEncryptionType(type, tcp_option_->GetPublicKey(type));
        }
      }
    }
    else if (protocol == fun::TransportProtocol::kUdp) {
      transport = FunapiUdpTransport::Create(shared_from_this(), hostname_or_ip_, static_cast<uint16_t>(port), encoding);

      transport->SetSendSessionIdOnlyOnce(session_option_->GetSendSessionIdOnlyOnce());

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

      transport = FunapiHttpTransport::Create(shared_from_this(), hostname_or_ip_, static_cast<uint16_t>(port), https, encoding);

      if (option) {
        http_option_ = std::static_pointer_cast<FunapiHttpTransportOption>(option);
        auto http_transport = std::static_pointer_cast<FunapiHttpTransport>(transport);

        http_transport->SetSequenceNumberValidation(http_option_->GetSequenceNumberValidation());
        http_transport->SetEncryptionType(http_option_->GetEncryptionType());
        http_transport->SetCACertFilePath(http_option_->GetCACertFilePath());
        http_transport->SetConnectTimeout(http_option_->GetConnectTimeout());
      }
    }

    AttachTransport(transport);

    PushTaskQueue([this]()->bool {
      Start();
      return true;
    });

  }
}


void FunapiSessionImpl::OnConnect(const TransportProtocol protocol) {
  if (auto transport = GetTransport(protocol)) {
    auto state = transport->GetState();
    if (state == TransportState::kDisconnected) {
      if (protocol == TransportProtocol::kTcp) {
        if (tcp_option_) {
          auto encryption_types = tcp_option_->GetEncryptionTypes();
          for (auto type : encryption_types) {
            std::static_pointer_cast<FunapiTcpTransport>(transport)->SetEncryptionType(type, tcp_option_->GetPublicKey(type));
          }
        }
      }
      else if (protocol == TransportProtocol::kUdp) {
        if (udp_option_) {
          std::static_pointer_cast<FunapiUdpTransport>(transport)->SetEncryptionType(udp_option_->GetEncryptionType());
        }
      }
      else if (protocol == TransportProtocol::kHttp) {
        if (http_option_) {
          std::static_pointer_cast<FunapiHttpTransport>(transport)->SetEncryptionType(http_option_->GetEncryptionType());
        }
      }

      transport->Start();
    }
    else if (state == TransportState::kDisconnecting) {
      Connect(protocol);
    }
  }
}


void FunapiSessionImpl::Connect(const TransportProtocol protocol) {
  PushTaskQueue([this, protocol]()->bool {
    OnConnect(protocol);
    return true;
  });
}


void FunapiSessionImpl::RegisterHandler(const std::string &msg_type,
                                        const MessageEventHandler &handler) {
  // DebugUtils::Log("New handler for message type %s", msg_type.c_str());
  message_handlers_[msg_type] = handler;
}


void FunapiSessionImpl::Start() {
  if (GetSessionId(FunEncoding::kJson).empty()) {
    for (auto protocol : v_protocols_) {
      if (auto transport = GetTransport(protocol)) {
        transport->Start();
        break;
      }
    }
  }
  else {
    for (auto protocol : v_protocols_) {
      if (auto transport = GetTransport(protocol)) {
        if (!transport->IsStarted()) {
          transport->Start();
        }
        else {
          Connect(protocol);
        }
      }
    }
  }
}


void FunapiSessionImpl::OnClose() {
  for (auto protocol : v_protocols_) {
    OnClose(protocol);
  }
}


void FunapiSessionImpl::Close() {
  PushTaskQueue([this]()->bool{
    OnClose();
    return true;
  });
}


void FunapiSessionImpl::OnClose(const TransportProtocol protocol) {
  if (auto transport = GetTransport(protocol)) {
    auto state = transport->GetState();
    if (state == TransportState::kConnected) {
      transport->Stop();
    }
    else if (state == TransportState::kConnecting) {
      Close(protocol);
    }
  }
}


void FunapiSessionImpl::Close(const TransportProtocol protocol) {
  PushTaskQueue([this, protocol]()->bool{
    OnClose(protocol);
    return true;
  });
}


void FunapiSessionImpl::SendMessage(std::shared_ptr<FunapiMessage> &message,
                                    const TransportProtocol protocol,
                                    bool priority,
                                    bool handshake) {
  TransportProtocol protocol_for_send;
  if (protocol == TransportProtocol::kDefault) {
    protocol_for_send = GetDefaultProtocol();
  }
  else {
    protocol_for_send = protocol;
  }

  if (handshake || priority) {
    std::shared_ptr<FunapiTransport> transport = GetTransport(protocol_for_send);
    if (transport) {
      transport->SendMessage(message, priority, handshake);
    }
  }
  else if (protocol_for_send != TransportProtocol::kDefault) {
    PushTaskQueue([this, protocol_for_send, message]()->bool{
      send_queues_[static_cast<int>(protocol_for_send)]->PushBack(message);
      return true;
    });
  }
}


void FunapiSessionImpl::SendMessage(const std::string &msg_type,
                                    const std::string &json_string,
                                    const TransportProtocol protocol,
                                    const EncryptionType encryption_type) {
  rapidjson::Document body;
  body.Parse<0>(json_string.c_str());

  // Encodes a messsage type.
  if (msg_type.length() > 0) {
    rapidjson::Value msg_type_node;
    msg_type_node.SetString(rapidjson::StringRef(msg_type.c_str()), body.GetAllocator());
    body.AddMember(rapidjson::StringRef(kMessageTypeAttributeName), msg_type_node, body.GetAllocator());
  }

  auto message = FunapiMessage::Create(body, encryption_type);
  message->SetUseSeq(true);
  message->SetUseSentQueue(IsReliableSession());

  SendMessage(message, protocol);
}


void FunapiSessionImpl::SendMessage(const FunMessage& temp_message,
                                    const TransportProtocol protocol,
                                    const EncryptionType encryption_type) {
  auto message = FunapiMessage::Create(temp_message, encryption_type);
  message->SetUseSeq(true);
  message->SetUseSentQueue(IsReliableSession());

  SendMessage(message, protocol);
}


bool FunapiSessionImpl::IsRedirecting() const {
  return (funapi_message_redirect_ != nullptr);
}


bool FunapiSessionImpl::IsConnected() const {
  for (auto protocol : v_protocols_) {
    if (auto transport = GetTransport(protocol)) {
      if (transport->IsStarted()) {
        return true;
      }
    }
  }

  return false;
}


bool FunapiSessionImpl::IsConnected(const TransportProtocol protocol) const {
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
  if (transport) {
    return transport->IsStarted();
  }

  return false;
}


void FunapiSessionImpl::OnTransportReceived(const TransportProtocol protocol,
                                            const FunEncoding encoding,
                                            const HeaderFields &header,
                                            const std::vector<uint8_t> &body,
                                            const std::shared_ptr<FunapiMessage> message) {
  std::string msg_type;
  std::string session_id;
  int32_t msg_type2 = 0;

  if (encoding == FunEncoding::kJson) {
    rapidjson::Document &json = message->GetJsonDocumenet();

    if (json.HasMember(kMessageTypeAttributeName)) {
      const rapidjson::Value &msg_type_node = json[kMessageTypeAttributeName];
      assert(msg_type_node.IsString());
      msg_type = msg_type_node.GetString();
    }

    if (json.HasMember(kSessionIdAttributeName)) {
      const rapidjson::Value &session_id_node = json[kSessionIdAttributeName];
      assert(session_id_node.IsString());
      session_id = session_id_node.GetString();
    }
  } else if (encoding == FunEncoding::kProtobuf) {
    FunMessage &proto = message->GetProtobufMessage();

    if (proto.has_msgtype()) {
      msg_type = proto.msgtype();
    }

    if (proto.has_sid()) {
      session_id = proto.sid();
    }

    if (proto.has_msgtype2()) {
      msg_type2 = proto.msgtype2();
    }
  }

#ifdef DEBUG_LOG
  {
    std::stringstream ss;
    ss << "[S->C] " << TransportProtocolToString(protocol) << "/" << EncodingToString(encoding) << ": ";

    // header
    ss << "{";
    for (auto i : header) {
      ss << "(" << i.first << "=" << i.second << ")";
    }
    ss << "} ";

    // body
    if (encoding == FunEncoding::kProtobuf) {
      ss << message->GetProtobufMessage().ShortDebugString();
    }
    else if (encoding == FunEncoding::kJson) {
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      message->GetJsonDocumenet().Accept(writer);

      ss << buffer.GetString();
    }

    DebugUtils::Log("%s", ss.str().c_str());
  }
#endif

  if (session_id.length() > 0) {
    if (protocol == TransportProtocol::kUdp) {
      if (auto t = GetTransport(TransportProtocol::kUdp)) {
        t->SetUseFirstSessionId(false);
      }
    }

    std::string session_id_old = GetSessionId(encoding);
    if (session_id_old.empty()) {
      SetSessionId(session_id, encoding);
      // DebugUtils::Log("New session id: %s", GetSessionId(FunEncoding::kJson).c_str());
    }
    else if (session_id_old.compare(session_id) != 0) {
      if (session_id_old.length() != session_id.length() && encoding == FunEncoding::kProtobuf) {
        SetSessionId(session_id, encoding);
      }
      else {
        std::string old_string = GetSessionId(FunEncoding::kJson);
        SetSessionId(session_id, encoding);

        std::string new_string = GetSessionId(FunEncoding::kJson);
        // DebugUtils::Log("Session id changed: %s => %s", old_string.c_str(), new_string.c_str());
        std::stringstream ss;
        ss << old_string << " -> " << new_string;

        OnSessionEvent(protocol, encoding, SessionEventType::kChanged, ss.str(), nullptr);
      }
    }
  }

  if (session_option_->GetSessionReliability()) {
    if (msg_type.empty() && 0 == msg_type2)
      return;
  }

  if (!msg_type.empty()) {
    EraseRecvTimeout(msg_type);
  }
  else if (msg_type2 != 0) {
    EraseRecvTimeout(msg_type2);
  }

  MessageHandlerMap::iterator it = message_handlers_.find(msg_type);
  if (it != message_handlers_.end()) {
    it->second(protocol, msg_type, body, message);
    return;
  }

  if (encoding == FunEncoding::kJson) {
    OnJsonRecv(protocol, msg_type, std::string(body.begin(), body.end()));
  }
  else if (encoding == FunEncoding::kProtobuf) {
    OnProtobufRecv(protocol, message->GetProtobufMessage());
  }
}


void FunapiSessionImpl::OnSessionOpen(const TransportProtocol protocol,
                                      const std::string &msg_type,
                                      const std::vector<uint8_t>&v_body,
                                      const std::shared_ptr<FunapiMessage> message) {
  OnSessionEvent(protocol, GetEncoding(protocol), SessionEventType::kOpened, GetSessionId(FunEncoding::kJson), nullptr);

  if (IsRedirecting()) {
    SendRedirectConenectMessage(protocol, token_);
  }

  for (auto p : v_protocols_) {
    if (auto t = GetTransport(p)) {
      if (false == t->IsStarted()) {
        t->Start();
      }
    }
  }
}


void FunapiSessionImpl::OnSessionClose(const TransportProtocol protocol,
                                       const std::string &msg_type,
                                       const std::vector<uint8_t>&v_body,
                                       const std::shared_ptr<FunapiMessage> message) {
  // DebugUtils::Log("Session timed out. Resetting my session id. The server will send me another one next time.");

  auto encoding = GetEncoding(protocol);

  std::string temp_session_id = GetSessionId(FunEncoding::kJson);

  {
    std::unique_lock<std::mutex> lock(transports_mutex_);
    for (auto i : v_protocols_) {
      transports_[static_cast<int>(i)] = nullptr;
    }
  }

  send_queues_[static_cast<int>(TransportProtocol::kTcp)] = FunapiQueue::Create();
  send_queues_[static_cast<int>(TransportProtocol::kUdp)] = FunapiQueue::Create();
  send_queues_[static_cast<int>(TransportProtocol::kHttp)] = FunapiQueue::Create();

  tasks_ = FunapiTasks::Create();
  session_id_ = FunapiSessionId::Create();

  OnSessionEvent(protocol, encoding, SessionEventType::kClosed, temp_session_id, nullptr);
}


void FunapiSessionImpl::OnServerPingMessage(const TransportProtocol protocol,
                                            const std::string &msg_type,
                                            const std::vector<uint8_t>&v_body,
                                            const std::shared_ptr<FunapiMessage> m) {
  fun::FunEncoding encoding = GetEncoding(protocol);
  assert(encoding!=FunEncoding::kNone);

  std::shared_ptr<FunapiMessage> message = m;

  message->SetUseSentQueue(false);
  message->SetUseSeq(false);

  SendMessage(message, protocol);
}


void FunapiSessionImpl::OnClientPingMessage(const TransportProtocol protocol,
                                            const std::string &msg_type,
                                            const std::vector<uint8_t>&v_body,
                                            const std::shared_ptr<FunapiMessage> message) {
  fun::FunEncoding encoding = GetEncoding(protocol);
  assert(encoding!=FunEncoding::kNone);

  std::string body(v_body.cbegin(), v_body.cend());
  int64_t timestamp_ms = 0;

  if (encoding == FunEncoding::kJson)
  {
    rapidjson::Document &document = message->GetJsonDocumenet();

    timestamp_ms = document[kPingTimestampField].GetInt64();
  }

  if (encoding == FunEncoding::kProtobuf)
  {
    FunMessage &msg = message->GetProtobufMessage();

    FunPingMessage ping_message = msg.GetExtension(cs_ping);
    timestamp_ms = ping_message.timestamp();
  }

  // int64_t ping_time_ms = (std::chrono::system_clock::now().time_since_epoch().count() - timestamp_ms) / 1000;

  // DebugUtils::Log("Receive %s ping - timestamp:%lld time=%lld ms", "Tcp", timestamp_ms, ping_time_ms);
}


void FunapiSessionImpl::CheckRedirect() {
  if (IsRedirecting()) {
    for (auto p : v_protocols_) {
      if (auto t = GetTransport(p)) {
        auto state = t->GetState();
        if (state != TransportState::kDisconnected) {
          return;
        }
      }
    }

    OnRedirect();
  }
}


void FunapiSessionImpl::OnRedirect() {
  assert(funapi_message_redirect_);
  std::shared_ptr<FunapiMessage> message = funapi_message_redirect_;

  fun::FunEncoding encoding = message->GetEncoding();
  assert(encoding!=FunEncoding::kNone);

  FunMessage &msg = message->GetProtobufMessage();
  FunRedirectMessage *redirect_message = nullptr;

  if (encoding == FunEncoding::kJson) {
    redirect_message = msg.MutableExtension(_sc_redirect);

    // log
    // std::string temp_string(v_body.begin(), v_body.end());
    // DebugUtils::Log("json string = %s", temp_string.c_str());
    // //

    rapidjson::Document &document = message->GetJsonDocumenet();

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
    redirect_message = msg.MutableExtension(_sc_redirect);
  }

  token_ = redirect_message->token();

  {
    std::unique_lock<std::mutex> lock(transports_mutex_);
    for (auto i : v_protocols_) {
      transports_[static_cast<int>(i)] = nullptr;
    }
  }

  std::string old_session_id = GetSessionId(FunEncoding::kJson);
  session_id_ = FunapiSessionId::Create();

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

  OnSessionEvent(protocol_redirect_,
                 GetEncoding(protocol_redirect_),
                 SessionEventType::kRedirectStarted,
                 old_session_id,
                 nullptr);
}


void FunapiSessionImpl::OnRedirectMessage(const TransportProtocol protocol,
                                          const std::string &msg_type,
                                          const std::vector<uint8_t>&v_body,
                                          const std::shared_ptr<FunapiMessage> message) {
  funapi_message_redirect_ = message;
  protocol_redirect_ = protocol;

  for (auto i : v_protocols_) {
    if (auto transport = GetTransport(i)) {
      transport->Stop();
    }
  }
}


void FunapiSessionImpl::OnRedirectConnectMessage(const TransportProtocol protocol,
                                                 const std::string &msg_type,
                                                 const std::vector<uint8_t>&v_body,
                                                 const std::shared_ptr<FunapiMessage> message) {
  fun::FunEncoding encoding = GetEncoding(protocol);
  assert(encoding!=FunEncoding::kNone);

  FunMessage &msg = message->GetProtobufMessage();
  FunRedirectConnectMessage *redirect = nullptr;

  if (encoding == FunEncoding::kJson) {
    redirect = msg.MutableExtension(_cs_redirect_connect);

    // log
    // std::string temp_string(v_body.begin(), v_body.end());
    // DebugUtils::Log("json string = %s", temp_string.c_str());
    // //

    rapidjson::Document &document = message->GetJsonDocumenet();

    if (document.HasMember("result")) {
      redirect->set_result(static_cast<FunRedirectConnectMessage_Result>(document["result"].GetInt()));
    }
  }

  if (encoding == FunEncoding::kProtobuf)
  {
    redirect = msg.MutableExtension(_cs_redirect_connect);
  }

  FunRedirectConnectMessage_Result result = redirect->result();

  if (result == FunRedirectConnectMessage_Result_OK) {
    token_ = "";
    funapi_message_redirect_ = nullptr;

    OnSessionEvent(protocol,
                   GetEncoding(protocol),
                   SessionEventType::kRedirectSucceeded,
                   GetSessionId(FunEncoding::kJson),
                   nullptr);
  }
  else {
    token_ = "";
    funapi_message_redirect_ = nullptr;

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
                   GetEncoding(protocol),
                   SessionEventType::kRedirectFailed,
                   GetSessionId(FunEncoding::kJson),
                   fun::FunapiError::Create(FunapiError::ErrorType::kRedirect, code));
  }
}

void FunapiSessionImpl::UpdateTasks() {
  if (tasks_) {
    tasks_->Update();
  }

  if (false == IsRedirecting()) {
    CheckRecvTimeout();
  }
}


void FunapiSessionImpl::UpdateTrasnports() {
  for (auto p : v_protocols_) {
    if (auto t = GetTransport(p)) {
      t->Update();
    }
  }
}


void FunapiSessionImpl::UpdateSocketSelect() {
  if (network_thread_) {
    if (network_thread_->Size() == 0) {
      network_thread_->Push([]()->bool {
        FunapiSocket::Select();
        return true;
      });
    }
  }
}


void FunapiSessionImpl::Update() {
  UpdateTasks();
  UpdateTrasnports();
  UpdateSocketSelect();
}


void FunapiSessionImpl::AttachTransport(const std::shared_ptr<FunapiTransport> &transport) {
  if (HasTransport(transport->GetProtocol()))
  {
    // DebugUtils::Log("AttachTransport - transport of '%d' type already exists.", static_cast<int>(transport->GetProtocol()));
  }
  else
  {
    {
      std::unique_lock<std::mutex> lock(transports_mutex_);
      transports_[static_cast<int>(transport->GetProtocol())] = transport;
    }

    if (GetDefaultProtocol() == TransportProtocol::kDefault)
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


std::shared_ptr<FunapiSessionId> FunapiSessionImpl::GetSessionIdSharedPtr() {
  return session_id_;
}


std::shared_ptr<FunapiQueue> FunapiSessionImpl::GetQueueSharedPtr(const TransportProtocol protocol) {
  return send_queues_[static_cast<int>(protocol)];
}


std::string FunapiSessionImpl::GetSessionId(const FunEncoding encoding) {
  return session_id_->Get(encoding);
}


void FunapiSessionImpl::SetSessionId(const std::string &session_id, const FunEncoding encoding) {
  session_id_->Set(session_id, encoding);
}


void FunapiSessionImpl::PushTaskQueue(const std::function<bool()> &task)
{
  if (tasks_) {
    std::weak_ptr<FunapiSessionImpl> weak = shared_from_this();
    tasks_->Push([weak, task]()->bool{
      if (auto s = weak.lock()) {
        return task();
      }

      return true;
    });
  }
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
                                       const FunEncoding encoding,
                                       const SessionEventType type,
                                       const std::string &session_id,
                                       const std::shared_ptr<FunapiError> &error) {

  bool use_skip = IsRedirecting() &&
    type != SessionEventType::kRedirectStarted &&
    type != SessionEventType::kRedirectSucceeded &&
    type != SessionEventType::kRedirectFailed;

  // log
  {
    std::stringstream ss;
    if (use_skip) {
      ss << "(Skip)";
    }

    ss << "session event " << TransportProtocolToString(protocol)
      << "/" << EncodingToString(encoding)
      << "(" << SessionEventTypeToString(type)
      << "): sid=" << session_id;

    if (error) {
      ss << ", " << error->DebugString();
    }

    DebugUtils::Log("%s", ss.str().c_str());
  }
  // //

  if (false == use_skip) {
    PushTaskQueue([this, protocol, type, session_id, error]()->bool {
      if (auto s = session_.lock()) {
        on_session_event_(s, protocol, type, session_id, error);
      }
      return true;
    });
  }
}


void FunapiSessionImpl::OnTransportEvent(const TransportProtocol protocol,
                                         const FunEncoding encoding,
                                         const TransportEventType type,
                                         std::shared_ptr<FunapiError> error) {
  // log
  {
    std::stringstream ss;
    if (IsRedirecting()) {
      ss << "(Skip)";
    }

    ss << "transport event " << TransportProtocolToString(protocol)
      << "/" << EncodingToString(encoding)
      << "(" << TransportEventTypeToString(type)
      << "):";

    if (GetSessionId(FunEncoding::kJson).length() > 0) {
      ss << " sid=" << GetSessionId(FunEncoding::kJson);
    }

    if (error) {
      ss << ", " << error->DebugString();
    }

    DebugUtils::Log("%s", ss.str().c_str());
  }
  // //

  if (IsRedirecting() == false ||
      type == TransportEventType::kConnectionFailed ||
      type == TransportEventType::kConnectionTimedOut) {
    PushTaskQueue([this, protocol, type, error]()->bool {
      if (auto s = session_.lock()) {
        on_transport_event_(s, protocol, type, error);
      }
      return true;
    });
  }
}


void FunapiSessionImpl::OnTransportConnectFailed(const TransportProtocol protocol, std::shared_ptr<FunapiError> error) {
  OnTransportEvent(protocol, GetEncoding(protocol), TransportEventType::kConnectionFailed, error);
}


void FunapiSessionImpl::OnTransportConnectTimeout(const TransportProtocol protocol, std::shared_ptr<FunapiError> error) {
  OnTransportEvent(protocol, GetEncoding(protocol), TransportEventType::kConnectionTimedOut, error);
}


void FunapiSessionImpl::OnTransportDisconnected(const TransportProtocol protocol, std::shared_ptr<FunapiError> error) {
  OnTransportEvent(protocol, GetEncoding(protocol), TransportEventType::kDisconnected, error);
}


void FunapiSessionImpl::OnTransportStarted(const TransportProtocol protocol, std::shared_ptr<FunapiError> error) {
  if (GetSessionId(FunEncoding::kJson).empty() &&
      protocol != TransportProtocol::kTcp) {
    SendEmptyMessage(protocol);
  }

  OnTransportEvent(protocol, GetEncoding(protocol), TransportEventType::kStarted, error);
}


void FunapiSessionImpl::OnTransportClosed(const TransportProtocol protocol, std::shared_ptr<FunapiError> error) {
  OnTransportEvent(protocol, GetEncoding(protocol), TransportEventType::kStopped, error);
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


void FunapiSessionImpl::SendEmptyMessage(const TransportProtocol protocol,
                                         const EncryptionType encryption_type) {
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
  FunEncoding encoding = transport->GetEncoding();

  assert(encoding!=FunEncoding::kNone);

  std::shared_ptr<FunapiMessage> message;

  if (encoding == FunEncoding::kJson) {
    rapidjson::Document d;
    d.SetObject();

    message = FunapiMessage::Create(d, encryption_type);
  }
  else if (encoding == FunEncoding::kProtobuf) {
    FunMessage msg;
    message = FunapiMessage::Create(msg, encryption_type);
  }

  message->SetUseSentQueue(false);
  message->SetUseSeq(false);

  SendMessage(message, protocol, false, true);
}


bool FunapiSessionImpl::SendClientPingMessage(const TransportProtocol protocol,
                                              const EncryptionType encryption_type) {
  assert(protocol==TransportProtocol::kTcp);

  if (GetSessionId(FunEncoding::kJson).empty())
    return false;

  FunEncoding encoding = GetEncoding(protocol);
  assert(encoding!=FunEncoding::kNone);

  int64_t timestamp = std::chrono::system_clock::now().time_since_epoch().count();
  // DebugUtils::Log("Send Tcp ping - timestamp: %lld", timestamp);

  std::shared_ptr<FunapiMessage> message;

  if (encoding == FunEncoding::kProtobuf) {
    FunMessage msg;
    msg.set_msgtype(kClientPingMessageType);
    FunPingMessage *ping_message = msg.MutableExtension(cs_ping);
    ping_message->set_timestamp(timestamp);
    // ping_message->set_data("77777777");

    message = FunapiMessage::Create(msg, encryption_type);
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
    msg_type_node.SetString(rapidjson::StringRef(kClientPingMessageType), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kMessageTypeAttributeName), msg_type_node, msg.GetAllocator());

    message = FunapiMessage::Create(msg, encryption_type);
  }

  message->SetUseSentQueue(false);
  message->SetUseSeq(false);

  SendMessage(message, protocol);

  return true;
}


void FunapiSessionImpl::SendAck(const TransportProtocol protocol,
                                const uint32_t ack,
                                const EncryptionType encryption_type) {
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
  if (transport == nullptr) return;

  // DebugUtils::Log("Tcp send ack message - ack:%d", ack);

  std::shared_ptr<FunapiMessage> message;

  if (transport->GetEncoding() == FunEncoding::kJson) {
    rapidjson::Document msg;
    msg.SetObject();
    rapidjson::Value key(kAckNumAttributeName, msg.GetAllocator());
    rapidjson::Value ack_node(rapidjson::kNumberType);
    ack_node.SetUint(ack);
    msg.AddMember(key, ack_node, msg.GetAllocator());

    message = FunapiMessage::Create(msg, encryption_type);
  }
  else if (transport->GetEncoding() == FunEncoding::kProtobuf) {
    FunMessage msg;
    msg.set_ack(ack);

    message = FunapiMessage::Create(msg, encryption_type);
  }

  message->SetUseSentQueue(false);
  message->SetUseSeq(false);

  SendMessage(message, protocol, true);
}


void FunapiSessionImpl::SendRedirectConenectMessage(const TransportProtocol protocol,
                                                    const std::string &token,
                                                    const EncryptionType encryption_type) {
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
  if (transport == nullptr) return;

  std::shared_ptr<FunapiMessage> message;

  if (transport->GetEncoding() == FunEncoding::kJson) {
    rapidjson::Document msg;
    msg.SetObject();
    rapidjson::Value message_node(token.c_str(), msg.GetAllocator());
    msg.AddMember("token", message_node, msg.GetAllocator());

    // Encodes a messsage type.
    rapidjson::Value msg_type_node;
    msg_type_node.SetString(rapidjson::StringRef(kRedirectConnectMessageType), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kMessageTypeAttributeName), msg_type_node, msg.GetAllocator());

    message = FunapiMessage::Create(msg, encryption_type);
  }
  else if (transport->GetEncoding() == FunEncoding::kProtobuf) {
    FunMessage msg;
    msg.set_msgtype(kRedirectConnectMessageType);

    FunRedirectConnectMessage *redirect = msg.MutableExtension(_cs_redirect_connect);
    redirect->set_token(token);

    message = FunapiMessage::Create(msg, encryption_type);
  }

  message->SetUseSentQueue(false);
  message->SetUseSeq(false);

  SendMessage(message, protocol);
}


bool FunapiSessionImpl::IsReliableSession() const {
  return session_option_->GetSessionReliability();
}


void FunapiSessionImpl::AddRecvTimeoutCallback(const RecvTimeoutHandler &handler) {
  on_recv_timeout_ += handler;
}


void FunapiSessionImpl::AddRecvTimeoutCallback(const RecvTimeoutIntHandler &handler) {
  on_recv_timeout_int_ += handler;
}


void FunapiSessionImpl::SetRecvTimeout(const std::string &msg_type, const int seconds) {
  std::unique_lock<std::mutex> lock(m_recv_timeout_mutex_);
  m_recv_timeout_[msg_type] = std::make_shared<FunapiTimer>(seconds);
}


void FunapiSessionImpl::SetRecvTimeout(const int32_t msg_type, const int seconds) {
  std::unique_lock<std::mutex> lock(m_recv_timeout_mutex_);
  m_recv_timeout_int_[msg_type] = std::make_shared<FunapiTimer>(seconds);
}


void FunapiSessionImpl::EraseRecvTimeout(const std::string &msg_type) {
  std::unique_lock<std::mutex> lock(m_recv_timeout_mutex_);
  m_recv_timeout_.erase(msg_type);
}


void FunapiSessionImpl::EraseRecvTimeout(const int32_t msg_type) {
  std::unique_lock<std::mutex> lock(m_recv_timeout_mutex_);
  m_recv_timeout_int_.erase(msg_type);
}


void FunapiSessionImpl::CheckRecvTimeout() {
  std::vector<std::string> msg_types;
  std::vector<int32_t> msg_types2;

  {
    std::unique_lock<std::mutex> lock(m_recv_timeout_mutex_);
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

  {
    std::unique_lock<std::mutex> lock(m_recv_timeout_mutex_);
    for (auto i : m_recv_timeout_int_) {
      if (i.second->IsExpired()) {
        msg_types2.push_back(i.first);
      }
    }

    for (auto t : msg_types2) {
      m_recv_timeout_int_.erase(t);
    }
  }

  for (auto t : msg_types2) {
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


void FunapiSessionImpl::OnRecvTimeout(const int32_t msg_type) {
  PushTaskQueue([this, msg_type]()->bool {
    if (auto s = session_.lock()) {
      on_recv_timeout_int_(s, msg_type);
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


bool FunapiSessionImpl::SendHandShakeMessages(const TransportProtocol protocol) {
  if (protocol == TransportProtocol::kTcp) {
    if (auto t = std::static_pointer_cast<FunapiTcpTransport>(GetTransport(TransportProtocol::kTcp))) {
      return t->SendHandShakeMessages();
    }
  }
  return false;
}


void FunapiSessionImpl::UpdateAll() {
  auto v_sessions = FunapiSessionImpl::GetSessionImpls();

  if (!v_sessions.empty()) {
    for (auto s : v_sessions) {
      s->UpdateTasks();
      s->UpdateTrasnports();
    }
  }

  if (auto nt = FunapiThread::Get("_network")) {
    if (nt->Size() == 0) {
      nt->Push([]()->bool {
        FunapiSocket::Select();
        return true;
      });
    }
  }
}


void FunapiSessionImpl::PushNetworkThreadTask(const FunapiThread::TaskHandler &handler) {
  if (network_thread_) {
    network_thread_->Push(handler);
  }
}

////////////////////////////////////////////////////////////////////////////////
// FunapiSession implementation.


FunapiSession::FunapiSession(const char* hostname_or_ip, std::shared_ptr<FunapiSessionOption> option)
: impl_(std::make_shared<FunapiSessionImpl>(hostname_or_ip, option)) {
  FunapiSessionImpl::Add(impl_);
}


FunapiSession::~FunapiSession() {
  // DebugUtils::Log("%s", __FUNCTION__);
}


std::shared_ptr<FunapiSession> FunapiSession::Create(const char* hostname_or_ip,
                                                     std::shared_ptr<FunapiSessionOption> option) {
  return std::make_shared<FunapiSession>(hostname_or_ip, option);
}


std::shared_ptr<FunapiSession> FunapiSession::Create(const char* hostname_or_ip,
                                                     bool reliability) {
  auto option = FunapiSessionOption::Create();
  option->SetSessionReliability(reliability);

  return FunapiSession::Create(hostname_or_ip, option);
}


void FunapiSession::Connect(const TransportProtocol protocol,
                            int port,
                            FunEncoding encoding,
                            std::shared_ptr<FunapiTransportOption> option) {
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


void FunapiSession::SendMessage(const std::string &msg_type,
                                const std::string &json_string,
                                const TransportProtocol protocol,
                                const EncryptionType encryption_type) {
  impl_->SendMessage(msg_type, json_string, protocol, encryption_type);
}


void FunapiSession::SendMessage(const FunMessage& message,
                                const TransportProtocol protocol,
                                const EncryptionType encryption_type) {
  impl_->SendMessage(message, protocol, encryption_type);
}


bool FunapiSession::IsConnected(const TransportProtocol protocol) const {
  return impl_->IsConnected(protocol);
}


bool FunapiSession::IsConnected() const {
  return impl_->IsConnected();
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


void FunapiSession::AddRecvTimeoutCallback(const RecvTimeoutIntHandler &handler) {
  impl_->AddRecvTimeoutCallback(handler);
}


void FunapiSession::SetRecvTimeout(const int32_t msg_type, const int seconds) {
  impl_->SetRecvTimeout(msg_type, seconds);
}


void FunapiSession::EraseRecvTimeout(const int32_t msg_type) {
  impl_->EraseRecvTimeout(msg_type);
}


void FunapiSession::SetTransportOptionCallback(const TransportOptionHandler &handler) {
  impl_->SetTransportOptionCallback(handler);
}


void FunapiSession::UpdateAll() {
  FunapiSessionImpl::UpdateAll();
}

////////////////////////////////////////////////////////////////////////////////
// FunapiSessionOptionImpl implementation.

class FunapiSessionOptionImpl : public std::enable_shared_from_this<FunapiSessionOptionImpl> {
 public:
  FunapiSessionOptionImpl() = default;
  virtual ~FunapiSessionOptionImpl() = default;

  void SetSessionReliability(const bool reliability);
  bool GetSessionReliability();

  void SetSendSessionIdOnlyOnce(const bool once);
  bool GetSendSessionIdOnlyOnce();

 private:
  bool use_session_reliability_ = false;
  bool use_send_session_id_only_once_ = false;
};


void FunapiSessionOptionImpl::SetSessionReliability(const bool reliability) {
  use_session_reliability_ = reliability;
}


bool FunapiSessionOptionImpl::GetSessionReliability() {
  return use_session_reliability_;
}


void FunapiSessionOptionImpl::SetSendSessionIdOnlyOnce(const bool once) {
  use_send_session_id_only_once_ = once;
}


bool FunapiSessionOptionImpl::GetSendSessionIdOnlyOnce() {
  return use_send_session_id_only_once_;
}

////////////////////////////////////////////////////////////////////////////////
// FunapiSessionOption implementation.

FunapiSessionOption::FunapiSessionOption ()
: impl_(std::make_shared<FunapiSessionOptionImpl>()) {
}


std::shared_ptr<FunapiSessionOption> FunapiSessionOption::Create() {
  return std::make_shared<FunapiSessionOption>();
}


void FunapiSessionOption::SetSessionReliability(const bool reliability) {
  impl_->SetSessionReliability(reliability);
}


bool FunapiSessionOption::GetSessionReliability() {
  return impl_->GetSessionReliability();
}


void FunapiSessionOption::SetSendSessionIdOnlyOnce(const bool once) {
  impl_->SetSendSessionIdOnlyOnce(once);
}


bool FunapiSessionOption::GetSendSessionIdOnlyOnce() {
  return impl_->GetSendSessionIdOnlyOnce();
}

}  // namespace fun
