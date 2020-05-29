// Copyright (C) 2013-2018 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_multicasting.h"

#ifdef FUNAPI_UE4
#include "FunapiPrivatePCH.h"
#endif

#include "funapi_utils.h"
#include "funapi_encryption.h"
#include "funapi_session.h"

#define kMulticastMsgType "_multicast"
#define kChannelId "_channel"
#define kSender "_sender"
#define kJoin "_join"
#define kLeave "_leave"
#define kErrorCode "_error_code"
#define kChannelListId "_channels"
#define kToken "_token"


namespace fun {

////////////////////////////////////////////////////////////////////////////////
// FunapiMulticastImpl implementation.

class FunapiMulticastImpl : public std::enable_shared_from_this<FunapiMulticastImpl> {
 public:
  typedef FunapiMulticast::ChannelNotify ChannelNotify;
  typedef FunapiMulticast::ErrorNotify ErrorNotify;
  typedef FunapiMulticast::ChannelListNotify ChannelListNotify;

  typedef FunapiMulticast::JsonChannelMessageHandler JsonChannelMessageHandler;
  typedef FunapiMulticast::ProtobufChannelMessageHandler ProtobufChannelMessageHandler;

  FunapiMulticastImpl() = delete;
  FunapiMulticastImpl(const char* sender, const char* hostname_or_ip, const uint16_t port, const FunEncoding encoding, const bool reliability, const TransportProtocol protocol);
  FunapiMulticastImpl(const char* sender, const std::shared_ptr<FunapiSession> &session, const TransportProtocol protocol);
  FunapiMulticastImpl(const char* sender,
                      const char* hostname_or_ip,
                      const uint16_t port,
                      const FunEncoding encoding,
                      const TransportProtocol protocol,
                      const std::shared_ptr<FunapiTransportOption> &transport_opt,
                      const std::shared_ptr<FunapiSessionOption> &session_opt);

  virtual ~FunapiMulticastImpl();

  bool IsConnected() const;
  bool IsInChannel(const fun::string &channel_id) const;

  // Deprecated function. Use FunapiMulticast::JoinChannel(chnnel_id, message_handler, token) function
  bool JoinChannel(const fun::string &channel_id, const fun::string &token);

  bool JoinChannel(const fun::string &channel_id, const JsonChannelMessageHandler &handler, const fun::string &token);
  bool JoinChannel(const fun::string &channel_id, const ProtobufChannelMessageHandler &handler, const fun::string &token);

  bool LeaveChannel(const fun::string &channel_id);
  bool LeaveAllChannels();

  bool SendToChannel(const fun::string &channel_id, FunMessage &msg, const bool bounce = true);
  bool SendToChannel(const fun::string &channel_id, fun::string &json_string, const bool bounce = true);

  void AddJoinedCallback(const ChannelNotify &handler);
  void AddLeftCallback(const ChannelNotify &handler);
  void AddErrorCallback(const ErrorNotify &handler);
  void AddChannelListCallback(const ChannelListNotify &handler);

  // Deprecated fucntion. Use FunapiMulticast::JoinChannel(chnnel_id, protobuf_channel_message_handler, token) function
  void AddProtobufChannelMessageCallback(const fun::string &channel_id, const ProtobufChannelMessageHandler &handler);

  // Deprecated function. Use FunapiMulticast::JoinChannel(chnnel_id, json_channel_message_handler, token) function
  void AddJsonChannelMessageCallback(const fun::string &channel_id, const JsonChannelMessageHandler &handler);

  bool RequestChannelList();

  void Update();

  FunEncoding GetEncoding();

  void AddSessionEventCallback(const FunapiMulticast::SessionEventHandler &handler);
  void AddTransportEventCallback(const FunapiMulticast::TransportEventHandler &handler);

  void RegistWeakPtr(const std::weak_ptr<FunapiMulticast>& weak);
  void Connect();
  void Close();
  void InitSessionCallback();

 private:
  void OnReceived(const fun::string &json_string);
  void OnReceived(const FunMessage &message);
  bool OnReceived(const fun::string &channel_id, const fun::string &sender, const bool join, const bool leave, const int error_code);


  std::shared_ptr<fun::FunapiSession> session_ = nullptr;
  std::weak_ptr<fun::FunapiMulticast> multicast_;

  FunEncoding encoding_ = FunEncoding::kNone;
  fun::string sender_;
  uint16_t port_;
  TransportProtocol protocol_ = TransportProtocol::kDefault;

  std::shared_ptr<FunapiTransportOption> transport_opt_ = nullptr;
  std::shared_ptr<FunapiSessionOption> session_opt_ = nullptr;

  FunapiEvent<ChannelNotify> on_joined_;
  FunapiEvent<ChannelNotify> on_left_;
  FunapiEvent<ErrorNotify> on_error_;
  FunapiEvent<ChannelListNotify> on_channel_list_;

  void OnJoined(const fun::string &channel_id, const fun::string &sender);
  void OnLeft(const fun::string &channel_id, const fun::string &sender);
  void OnError(const fun::string &channel_id, const int error);

  void OnChannelList(const rapidjson::Document &msg);
  void OnChannelList(FunMulticastMessage *msg);
  void OnChannelList(const fun::map<fun::string, int> &cl);

  fun::unordered_set<fun::string> channels_;
  fun::unordered_map<fun::string, FunapiEvent<JsonChannelMessageHandler>> json_channel_handlers_;
  fun::unordered_map<fun::string, FunapiEvent<ProtobufChannelMessageHandler>> protobuf_channel_handlers_;
  mutable std::mutex channels_mutex_;

  void SendLeaveMessage(const fun::string &channel_id);
};


FunapiMulticastImpl::FunapiMulticastImpl(const char* sender,
                                         const char* hostname_or_ip,
                                         const uint16_t port,
                                         const FunEncoding encoding,
                                         const bool reliability,
                                         const TransportProtocol protocol)
: encoding_(encoding), sender_(sender), port_(port), protocol_(protocol) {
  session_ = FunapiSession::Create(hostname_or_ip, reliability);
}


FunapiMulticastImpl::FunapiMulticastImpl(const char* sender,
                                         const std::shared_ptr<FunapiSession> &session,
                                         const TransportProtocol protocol)
: session_(session), sender_(sender), port_(0), protocol_(protocol) {
  encoding_ = session->GetEncoding(protocol_);
}


FunapiMulticastImpl::FunapiMulticastImpl(const char* sender,
                                         const char* hostname_or_ip,
                                         const uint16_t port,
                                         const FunEncoding encoding,
                                         const TransportProtocol protocol,
                                         const std::shared_ptr<FunapiTransportOption> &transport_opt,
                                         const std::shared_ptr<FunapiSessionOption> &session_opt)
  : encoding_(encoding), sender_(sender), port_(port), protocol_(protocol), transport_opt_(transport_opt), session_opt_(session_opt)
{
  session_ = FunapiSession::Create(hostname_or_ip, session_opt_);
}


FunapiMulticastImpl::~FunapiMulticastImpl() {
}


void FunapiMulticastImpl::InitSessionCallback() {
  std::weak_ptr<FunapiMulticastImpl> weak = shared_from_this();
  session_->
  AddJsonRecvCallback([weak, this](const std::shared_ptr<fun::FunapiSession> &session,
                                   const fun::TransportProtocol protocol,
                                   const fun::string &msg_type,
                                   const fun::string &json_string)
  {
    if (auto t = weak.lock())
    {
      if (msg_type.compare(kMulticastMsgType) == 0)
      {
        OnReceived(json_string);
      }
    }
  });

  session_->
  AddProtobufRecvCallback([weak, this](const std::shared_ptr<fun::FunapiSession> &session,
                                       const fun::TransportProtocol protocol,
                                       const FunMessage &message)
  {
    if (auto t = weak.lock())
    {
      if (message.msgtype().compare(kMulticastMsgType) == 0)
      {
        OnReceived(message);
      }
    }
  });
}


void FunapiMulticastImpl::AddJoinedCallback(const ChannelNotify &handler) {
  on_joined_ += handler;
}


void FunapiMulticastImpl::AddLeftCallback(const ChannelNotify &handler) {
  on_left_ += handler;
}


void FunapiMulticastImpl::AddErrorCallback(const ErrorNotify &handler) {
  on_error_ += handler;
}


void FunapiMulticastImpl::AddChannelListCallback(const ChannelListNotify &handler) {
  on_channel_list_ += handler;
}


void FunapiMulticastImpl::OnJoined(const fun::string &channel_id,
                                   const fun::string &sender) {
  if (auto m = multicast_.lock()) {
    on_joined_(m, channel_id, sender);
  }
}


void FunapiMulticastImpl::OnLeft(const fun::string &channel_id,
                                 const fun::string &sender) {
  if (auto m = multicast_.lock()) {
    on_left_(m, channel_id, sender);
  }
}


void FunapiMulticastImpl::OnError(const fun::string &channel_id, const int error) {
  {
    std::unique_lock<std::mutex> lock(channels_mutex_);
    channels_.erase(channel_id);
  }

  if (auto m = multicast_.lock()) {
    on_error_(m, error);
  }
}


void FunapiMulticastImpl::OnChannelList(const fun::map<fun::string, int> &cl) {
  if (auto m = multicast_.lock()) {
    on_channel_list_(m, cl);
  }
}


void FunapiMulticastImpl::OnChannelList(FunMulticastMessage *msg) {
  fun::map<fun::string, int> cl;

  for (int i=0;i<msg->channels_size();++i) {
    cl[msg->channels(i).channel_name()] = msg->channels(i).num_members();
  }

  OnChannelList(cl);
}


void FunapiMulticastImpl::OnChannelList(const rapidjson::Document &msg) {
  const rapidjson::Value &d = msg[kChannelListId];

  fun::map<fun::string, int> cl;

  for (unsigned int i=0;i<d.Size();++i) {
    const rapidjson::Value &v = d[i];
    if (v.HasMember("_name") && v.HasMember("_members")) {
      cl[v["_name"].GetString()] = v["_members"].GetInt();
    }
  }

  OnChannelList(cl);
}


bool FunapiMulticastImpl::IsConnected() const {
  if (session_) {
    if (session_->IsConnected(protocol_)) {
      return true;
    }
  }

  return false;
}


bool FunapiMulticastImpl::IsInChannel(const fun::string &channel_id) const {
  std::unique_lock<std::mutex> lock(channels_mutex_);
  if (channels_.find(channel_id) != channels_.end()) {
    return true;
  }

  return false;
}


bool FunapiMulticastImpl::JoinChannel(const fun::string &channel_id, const fun::string &token) {
  if (!IsConnected()) {
    DebugUtils::Log("Not connected. First connect before join a multicast channel.");
    return false;
  }

  if (IsInChannel(channel_id)) {
    DebugUtils::Log("Already joined the channel: %s", channel_id.c_str());
    return false;
  }

  {
    std::unique_lock<std::mutex> lock(channels_mutex_);
    channels_.insert(channel_id);
  }

  if (encoding_ == FunEncoding::kJson) {
    rapidjson::Document msg;
    msg.SetObject();

    rapidjson::Value channel_id_node(channel_id.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kChannelId), channel_id_node, msg.GetAllocator());

    rapidjson::Value sender_node(sender_.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kSender), sender_node, msg.GetAllocator());

    rapidjson::Value join_node(true);
    msg.AddMember(rapidjson::StringRef(kJoin), join_node, msg.GetAllocator());

    if (token.length() > 0) {
      rapidjson::Value token_node(token.c_str(), msg.GetAllocator());
      msg.AddMember(rapidjson::StringRef(kToken), token_node, msg.GetAllocator());
    }

    // Convert JSON document to fun::string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msg.Accept(writer);
    fun::string json_string = buffer.GetString();

    session_->SendMessage(kMulticastMsgType, json_string, protocol_);
  }

  if (encoding_ == FunEncoding::kProtobuf) {
    FunMessage msg;
    msg.set_msgtype(kMulticastMsgType);

    FunMulticastMessage *mcast_msg = msg.MutableExtension(multicast);
    mcast_msg->set_channel(channel_id.c_str());
    mcast_msg->set_join(true);
    mcast_msg->set_sender(sender_.c_str());

    if (token.length() > 0) {
      mcast_msg->set_token(token.c_str());
    }

    session_->SendMessage(msg, protocol_);
  }

  return true;
}


bool FunapiMulticastImpl::JoinChannel(const fun::string &channel_id, const JsonChannelMessageHandler &handler, const fun::string &token)
{
  if (!IsConnected())
  {
    DebugUtils::Log("Connect first before joining a multicast channel.");
    return false;
  }

  // encoding 은 생성시점에 결졍되고 상태가 변경되지 않는다.
  if (encoding_ != FunEncoding::kJson)
  {
    DebugUtils::Log("This multicast was created with json encoding. You should pass JsonChannelMessageHandler.");
    return false;
  }

  if (IsInChannel(channel_id))
  {
    DebugUtils::Log("Already joined the channel: %s", channel_id.c_str());
    return false;
  }

  {
    std::unique_lock<std::mutex> lock(channels_mutex_);
    channels_.insert(channel_id);
    json_channel_handlers_[channel_id] += handler;
  }

  // Send Join message
  rapidjson::Document msg;
  msg.SetObject();

  rapidjson::Value channel_id_node(channel_id.c_str(), msg.GetAllocator());
  msg.AddMember(rapidjson::StringRef(kChannelId), channel_id_node, msg.GetAllocator());

  rapidjson::Value sender_node(sender_.c_str(), msg.GetAllocator());
  msg.AddMember(rapidjson::StringRef(kSender), sender_node, msg.GetAllocator());

  rapidjson::Value join_node(true);
  msg.AddMember(rapidjson::StringRef(kJoin), join_node, msg.GetAllocator());

  if (token.length() > 0)
  {
    rapidjson::Value token_node(token.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kToken), token_node, msg.GetAllocator());
  }

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  msg.Accept(writer);
  fun::string json_string = buffer.GetString();

  session_->SendMessage(kMulticastMsgType, json_string, protocol_);
  return true;
}


bool FunapiMulticastImpl::JoinChannel(const fun::string &channel_id, const ProtobufChannelMessageHandler &handler, const fun::string &token)
{
  if (!IsConnected())
  {
    DebugUtils::Log("Connect first before joining a multicast channel.");
    return false;
  }

  // encoding 은 생성시점에 결졍되고 상태가 변경되지 않는다.
  if (encoding_ != FunEncoding::kProtobuf)
  {
    DebugUtils::Log("This multicast was created with protobuf encoding. You should pass ProtobufChannelMessageHandler.");
    return false;
  }

  if (IsInChannel(channel_id)) {
    DebugUtils::Log("Already joined the channel: %s", channel_id.c_str());
    return false;
  }

  {
    std::unique_lock<std::mutex> lock(channels_mutex_);
    channels_.insert(channel_id);
    protobuf_channel_handlers_[channel_id] += handler;
  }

  // Send Join message
  FunMessage msg;
  msg.set_msgtype(kMulticastMsgType);

  FunMulticastMessage *mcast_msg = msg.MutableExtension(multicast);
  mcast_msg->set_channel(channel_id.c_str());
  mcast_msg->set_join(true);
  mcast_msg->set_sender(sender_.c_str());

  if (token.length() > 0)
  {
    mcast_msg->set_token(token.c_str());
  }

  session_->SendMessage(msg, protocol_);
  return true;
}




bool FunapiMulticastImpl::LeaveChannel(const fun::string &channel_id) {
  if (!IsConnected()) {
    DebugUtils::Log("Not connected. If you are trying to leave a channel in which you were, connect first while preserving the session id you used for join.");
    return false;
  }

  if (!IsInChannel(channel_id)) {
    DebugUtils::Log("You are not in the channel: %s", channel_id.c_str());
    return false;
  }

  SendLeaveMessage(channel_id);
  OnLeft(channel_id, sender_);

  {
    std::unique_lock<std::mutex> lock(channels_mutex_);
    channels_.erase(channel_id);
  }

  json_channel_handlers_.erase(channel_id);
  protobuf_channel_handlers_.erase(channel_id);

  return true;
}


bool FunapiMulticastImpl::LeaveAllChannels() {
  auto channels_set(channels_);
  for (auto c : channels_set) {
    LeaveChannel(c);
  }

  return true;
}


bool FunapiMulticastImpl::SendToChannel(const fun::string &channel_id, FunMessage &msg, const bool bounce) {
  if (!IsInChannel(channel_id)) {
    DebugUtils::Log("You are not in the channel: %s", channel_id.c_str());
    return false;
  }

  FunMulticastMessage *mcast_msg = msg.MutableExtension(multicast);
  mcast_msg->set_channel(channel_id.c_str());
  mcast_msg->set_bounce(bounce);
  mcast_msg->set_sender(sender_.c_str());

  msg.set_msgtype(kMulticastMsgType);

  session_->SendMessage(msg, protocol_);

  return true;
}


bool FunapiMulticastImpl::SendToChannel(const fun::string &channel_id, fun::string &json_string, const bool bounce) {
  if (!IsConnected()) {
    DebugUtils::Log("Not connected. If you are trying to leave a channel in which you were, connect first while preserving the session id you used for join.");
    return false;
  }

  rapidjson::Document msg;
  msg.Parse<0>(json_string.c_str());

  if (!IsInChannel(channel_id)) {
    DebugUtils::Log("You are not in the channel: %s", channel_id.c_str());
    return false;
  }

  rapidjson::Value channel_id_node(channel_id.c_str(), msg.GetAllocator());
  msg.AddMember(rapidjson::StringRef("_channel"), channel_id_node, msg.GetAllocator());

  rapidjson::Value bounce_node(bounce);
  msg.AddMember(rapidjson::StringRef("_bounce"), bounce_node, msg.GetAllocator());

  rapidjson::Value sender_node(sender_.c_str(), msg.GetAllocator());
  msg.AddMember(rapidjson::StringRef(kSender), sender_node, msg.GetAllocator());

  // Convert JSON document to fun::string
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  msg.Accept(writer);
  fun::string send_json_string = buffer.GetString();

  session_->SendMessage(kMulticastMsgType, send_json_string, protocol_);

  return true;
}


void FunapiMulticastImpl::SendLeaveMessage(const fun::string &channel_id) {
  if (encoding_ == FunEncoding::kJson) {
    rapidjson::Document msg;
    msg.SetObject();

    rapidjson::Value channel_id_node(channel_id.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kChannelId), channel_id_node, msg.GetAllocator());

    rapidjson::Value sender_node(sender_.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kSender), sender_node, msg.GetAllocator());

    rapidjson::Value leave_node(true);
    msg.AddMember(rapidjson::StringRef(kLeave), leave_node, msg.GetAllocator());

    // Convert JSON document to fun::string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msg.Accept(writer);
    fun::string json_string = buffer.GetString();

    session_->SendMessage(kMulticastMsgType, json_string, protocol_);
  }

  if (encoding_ == FunEncoding::kProtobuf) {
    FunMessage msg;
    msg.set_msgtype(kMulticastMsgType);

    FunMulticastMessage *mcast_msg = msg.MutableExtension(multicast);
    mcast_msg->set_channel(channel_id.c_str());
    mcast_msg->set_leave(true);
    mcast_msg->set_sender(sender_.c_str());

    session_->SendMessage(msg, protocol_);
  }
}


void FunapiMulticastImpl::OnReceived(const fun::string &json_string) {
  fun::string channel_id = "";
  fun::string sender = "";
  bool join = false;
  bool leave = false;
  int error_code = 0;

  if (encoding_ == fun::FunEncoding::kJson) {
    // log
    // fun::string body(v_body.cbegin(), v_body.cend());
    // fun::DebugUtils::Log("multicast message\n%s", body.c_str());
    // //

    rapidjson::Document msg;
    msg.Parse<0>(json_string.c_str());

    if (msg.HasMember(kChannelListId)) {
      OnChannelList(msg);
      return;
    }

    if (msg.HasMember(kChannelId)) {
      channel_id = msg[kChannelId].GetString();
    }

    if (msg.HasMember(kSender)) {
      sender = msg[kSender].GetString();
    }

    if (msg.HasMember(kErrorCode)) {
      error_code = msg[kErrorCode].GetInt();
    }

    if (msg.HasMember(kJoin)) {
      join = msg[kJoin].GetBool();
    }

    if (msg.HasMember(kLeave)) {
      leave = msg[kLeave].GetBool();
    }
  }

  if (OnReceived(channel_id, sender, join, leave, error_code) == false) {
    if (json_channel_handlers_.find(channel_id) != json_channel_handlers_.end()) {
      if (auto m = multicast_.lock()) {
        json_channel_handlers_[channel_id](m, channel_id, sender, json_string);
      }
    }
  }
}


void FunapiMulticastImpl::OnReceived(const FunMessage &message) {
  fun::string channel_id = "";
  fun::string sender = "";
  bool join = false;
  bool leave = false;
  int error_code = 0;

  FunMulticastMessage mcast_msg = message.GetExtension(multicast);

  channel_id = mcast_msg.channel();

  if (mcast_msg.has_sender()) {
    sender = mcast_msg.sender();
  }

  if (mcast_msg.has_error_code()) {
    error_code = mcast_msg.error_code();
  }

  if (mcast_msg.has_join()) {
    join = mcast_msg.join();
  }

  if (mcast_msg.has_leave()) {
    leave = mcast_msg.leave();
  }

  if (channel_id.length() == 0 && sender.length() == 0) {
    OnChannelList(&mcast_msg);
    return;
  }

  if (OnReceived(channel_id, sender, join, leave, error_code) == false) {
    if (protobuf_channel_handlers_.find(channel_id) != protobuf_channel_handlers_.end()) {
      if (auto m = multicast_.lock()) {
        protobuf_channel_handlers_[channel_id](m, channel_id, sender, message);
      }
    }
  }
}


bool FunapiMulticastImpl::OnReceived(const fun::string &channel_id, const fun::string &sender, const bool join, const bool leave, const int error_code) {
  if (error_code != 0) {
    OnError(channel_id, error_code);
  }

  if (join) {
    // DebugUtils::Log("%s joined the '%s' channel", sender.c_str(), channel_id.c_str());
    OnJoined(channel_id, sender);
  }
  else if (leave) {
    // DebugUtils::Log("%s left the '%s' channel", sender.c_str(), channel_id.c_str());
    OnLeft(channel_id, sender);
  }
  else {
    return false;
  }

  return true;
}


bool FunapiMulticastImpl::RequestChannelList() {
  if (encoding_ == FunEncoding::kJson) {
    rapidjson::Document msg;
    msg.SetObject();

    rapidjson::Value sender_node(sender_.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kSender), sender_node, msg.GetAllocator());

    // Convert JSON document to fun::string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msg.Accept(writer);
    fun::string json_string = buffer.GetString();

    session_->SendMessage(kMulticastMsgType, json_string, protocol_);
  }

  if (encoding_ == FunEncoding::kProtobuf) {
    FunMessage msg;
    msg.set_msgtype(kMulticastMsgType);

    FunMulticastMessage *mcast_msg = msg.MutableExtension(multicast);
    mcast_msg->set_sender(sender_.c_str());

    session_->SendMessage(msg, protocol_);
  }

  return true;
}


void FunapiMulticastImpl::Update() {
  if (session_) {
    session_->Update();
  }
}


FunEncoding FunapiMulticastImpl::GetEncoding() {
  return encoding_;
}


void FunapiMulticastImpl::AddSessionEventCallback(const FunapiMulticast::SessionEventHandler &handler)
{
  if (session_)
  {
    std::weak_ptr<FunapiMulticastImpl> weak = shared_from_this();
    session_->AddSessionEventCallback
    ([weak ,this, handler]
     (const std::shared_ptr<fun::FunapiSession> &session,
      const fun::TransportProtocol protocol,
      const fun::SessionEventType type,
      const fun::string &session_id,
      const std::shared_ptr<FunapiError> &error)
    {
      if (auto t = weak.lock())
      {
        if (auto m = multicast_.lock())
        {
          handler(m, type, session_id, error);
        }
      }
    });
  }
}


void FunapiMulticastImpl::AddTransportEventCallback(const FunapiMulticast::TransportEventHandler &handler)
{
  if (session_)
  {
    std::weak_ptr<FunapiMulticastImpl> weak = shared_from_this();
    session_->AddTransportEventCallback
    ([weak, this, handler]
     (const std::shared_ptr<fun::FunapiSession> &session,
      const fun::TransportProtocol protocol,
      const fun::TransportEventType type,
      const std::shared_ptr<FunapiError> &error)
    {
      if (auto t = weak.lock())
      {
        if (auto m = multicast_.lock())
        {
          handler(m, type, error);
        }
      }
    });
  }
}


void FunapiMulticastImpl::RegistWeakPtr(
    const std::weak_ptr<FunapiMulticast>& weak)
{
  multicast_ = weak;
}


void FunapiMulticastImpl::Connect()
{
  FunapiUtil::Assert(session_ != nullptr);

  if (transport_opt_)
  {
    session_->Connect(protocol_, port_, encoding_, transport_opt_);
    return;
  }

  session_->Connect(protocol_, port_, encoding_);
}


void FunapiMulticastImpl::Close() {
  if (session_) {
    session_->Close();
  }
}


void FunapiMulticastImpl::AddProtobufChannelMessageCallback(const fun::string &channel_id,
                                                        const ProtobufChannelMessageHandler &handler) {
  protobuf_channel_handlers_[channel_id] += handler;
}


void FunapiMulticastImpl::AddJsonChannelMessageCallback(const fun::string &channel_id,
                                                    const JsonChannelMessageHandler &handler) {
  json_channel_handlers_[channel_id] += handler;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiMulticast implementation.

FunapiMulticast::FunapiMulticast(const char* sender, const char* hostname_or_ip, const uint16_t port,
                                 const FunEncoding encoding, const bool reliability, const TransportProtocol protocol)
: impl_(std::make_shared<FunapiMulticastImpl>(sender, hostname_or_ip, port, encoding, reliability, protocol)) {
  impl_->InitSessionCallback();
}


FunapiMulticast::FunapiMulticast(const char* sender, const std::shared_ptr<FunapiSession> &session,
                                 const TransportProtocol protocol)
: impl_(std::make_shared<FunapiMulticastImpl>(sender, session, protocol)) {
  impl_->InitSessionCallback();
}


FunapiMulticast::FunapiMulticast(const char* sender, const char* hostname_or_ip, const uint16_t port,
                                 const FunEncoding encoding, const TransportProtocol protocol,
                                 const std::shared_ptr<FunapiTransportOption> &transport_opt,
                                 const std::shared_ptr<FunapiSessionOption> &session_opt)
  : impl_(std::make_shared<FunapiMulticastImpl>(sender, hostname_or_ip, port, encoding, protocol, transport_opt, session_opt))
{
  impl_->InitSessionCallback();
}


FunapiMulticast::~FunapiMulticast() {
}


std::shared_ptr<FunapiMulticast> FunapiMulticast::Create(
    const char* sender, const char* hostname_or_ip,
    const uint16_t port, const FunEncoding encoding,
    const bool reliability, const TransportProtocol protocol)
{
  if (protocol != TransportProtocol::kTcp &&
      protocol != TransportProtocol::kWebsocket)
  {
    DebugUtils::Log(
        "FunapiMulticast only supports Tcp and Websocket Transport.");
    return nullptr;
  }

  auto instance =
      std::make_shared<FunapiMulticast>(sender, hostname_or_ip, port,
                                        encoding, reliability, protocol);
  std::shared_ptr<FunapiMulticastImpl> multicast_impl = instance->impl_;
  multicast_impl->RegistWeakPtr(instance);
  return instance;
}


std::shared_ptr<FunapiMulticast> FunapiMulticast::Create(
    const char* sender, const std::shared_ptr<FunapiSession> &session,
    const TransportProtocol protocol)
{
  if (protocol != TransportProtocol::kTcp &&
      protocol != TransportProtocol::kWebsocket)
  {
    DebugUtils::Log(
        "FunapiMulticast only supports Tcp and Websocket Transport.");
    return nullptr;
  }

  if (session)
  {
    if (session->IsConnected(protocol))
    {
      auto instance =
          std::make_shared<FunapiMulticast>(sender, session, protocol);
      std::shared_ptr<FunapiMulticastImpl> multicast_impl = instance->impl_;
      multicast_impl->RegistWeakPtr(instance);
      return instance;
    }
  }

  return nullptr;
}


std::shared_ptr<FunapiMulticast> FunapiMulticast::Create(
    const char* sender, const char* hostname_or_ip,
    const uint16_t port, const FunEncoding encoding,
    const TransportProtocol protocol,
    const std::shared_ptr<FunapiTransportOption> &transport_opt,
    const std::shared_ptr<FunapiSessionOption> &session_opt)
{
  if (protocol != TransportProtocol::kTcp &&
    protocol != TransportProtocol::kWebsocket)
  {
    DebugUtils::Log(
        "FunapiMulticast only supports Tcp and Websocket Transport.");
    return nullptr;
  }

  auto instance =
      std::make_shared<FunapiMulticast>(sender, hostname_or_ip, port, encoding,
                                        protocol, transport_opt, session_opt);
  std::shared_ptr<FunapiMulticastImpl> multicast_impl = instance->impl_;
  multicast_impl->RegistWeakPtr(instance);
  return instance;
}


void FunapiMulticast::AddJoinedCallback(const ChannelNotify &handler) {
  impl_->AddJoinedCallback(handler);
}


void FunapiMulticast::AddLeftCallback(const ChannelNotify &handler) {
  impl_->AddLeftCallback(handler);
}


void FunapiMulticast::AddErrorCallback(const ErrorNotify &handler) {
  impl_->AddErrorCallback(handler);
}


void FunapiMulticast::AddChannelListCallback(const ChannelListNotify &handler) {
  impl_->AddChannelListCallback(handler);
}


bool FunapiMulticast::IsConnected() const {
  return impl_->IsConnected();
}


bool FunapiMulticast::IsInChannel(const fun::string &channel_id) const {
  return impl_->IsInChannel(channel_id);
}


bool FunapiMulticast::LeaveChannel(const fun::string &channel_id) {
  return impl_->LeaveChannel(channel_id);
}


bool FunapiMulticast::LeaveAllChannels() {
  return impl_->LeaveAllChannels();
}


bool FunapiMulticast::SendToChannel(const fun::string &channel_id, FunMessage &msg, const bool bounce) {
  return impl_->SendToChannel(channel_id, msg, bounce);
}


bool FunapiMulticast::SendToChannel(const fun::string &channel_id, fun::string &json_string, const bool bounce) {
  return impl_->SendToChannel(channel_id, json_string, bounce);
}


bool FunapiMulticast::RequestChannelList() {
  return impl_->RequestChannelList();
}


void FunapiMulticast::Update() {
  impl_->Update();
}


FunEncoding FunapiMulticast::GetEncoding() {
  return impl_->GetEncoding();
}


void FunapiMulticast::AddSessionEventCallback(const FunapiMulticast::SessionEventHandler &handler) {
  impl_->AddSessionEventCallback(handler);
}


void FunapiMulticast::AddTransportEventCallback(const FunapiMulticast::TransportEventHandler &handler) {
  impl_->AddTransportEventCallback(handler);
}


void FunapiMulticast::Connect()
{
  impl_->Connect();
}


void FunapiMulticast::Close() {
  impl_->Close();
}


bool FunapiMulticast::JoinChannel(const fun::string &channel_id, const fun::string &token)
{
  DebugUtils::Log("JoinChannel(channel_id, token) function was deprecated. Please Use FunapiMulticast::JoinChannel(chnnel_id, message_handler, token) function");
  return impl_->JoinChannel(channel_id, token);
}


bool FunapiMulticast::JoinChannel(const fun::string &channel_id, const JsonChannelMessageHandler &handler, const fun::string &token)
{
  return impl_->JoinChannel(channel_id, handler, token);
}


bool FunapiMulticast::JoinChannel(const fun::string &channel_id, const ProtobufChannelMessageHandler &handler, const fun::string &token)
{
  return impl_->JoinChannel(channel_id, handler, token);
}


void FunapiMulticast::AddProtobufChannelMessageCallback(const fun::string &channel_id,
                                                        const ProtobufChannelMessageHandler &handler)
{
  DebugUtils::Log("FunapiMulticast::AddProtobufChannelMessageCallback function was deprecated. Please Use FunapiMulticast::JoinChannel(chnnel_id, message_handler, token) function");
  impl_->AddProtobufChannelMessageCallback(channel_id, handler);
}


void FunapiMulticast::AddJsonChannelMessageCallback(const fun::string &channel_id,
                                                    const JsonChannelMessageHandler &handler)
{
  DebugUtils::Log("FunapiMulticast::AddJsonChannelMessageCallback function was deprecated. Please Use FunapiMulticast::JoinChannel(chnnel_id, message_handler, token) function");
  impl_->AddJsonChannelMessageCallback(channel_id, handler);
}

}  // namespace fun
