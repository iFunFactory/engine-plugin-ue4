// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin.h"
#include "funapi_utils.h"
#include "funapi_network.h"
#include "funapi_multicasting.h"

namespace fun {

static const char* kMulticastMsgType = "_multicast";
static const char* kChannelId = "_channel";
static const char* kSender = "_sender";
static const char* kJoin = "_join";
static const char* kLeave = "_leave";
static const char* kErrorCode = "_error_code";

////////////////////////////////////////////////////////////////////////////////
// FunapiMulticastClientImpl implementation.

class FunapiMulticastClientImpl : public std::enable_shared_from_this<FunapiMulticastClientImpl> {
 public:
  typedef FunapiMulticastClient::ChannelNotify ChannelNotify;
  typedef FunapiMulticastClient::ChannelMessage ChannelMessage;
  typedef FunapiMulticastClient::ErrorNotify ErrorNotify;

  FunapiMulticastClientImpl(std::shared_ptr<FunapiNetwork> network, FunEncoding encoding);
  ~FunapiMulticastClientImpl();

  void SetSender(const std::string &sender);
  void SetEncoding(const FunEncoding encodig);

  bool IsConnected() const;
  bool IsInChannel(const std::string &channel_id) const;

  bool JoinChannel(const std::string &channel_id, const ChannelMessage &handler);
  bool LeaveChannel(const std::string &channel_id);

  bool SendToChannel(FunMulticastMessage &mcast_msg);
  bool SendToChannel(std::string &json_string);

  void AddJoinedCallback(const ChannelNotify &handler);
  void AddLeftCallback(const ChannelNotify &handler);
  void AddErrorCallback(const ErrorNotify &handler);

  void OnReceived(const fun::TransportProtocol protocol, const std::string &type, const std::vector<uint8_t> &v_body);

 private:
  std::shared_ptr<fun::FunapiNetwork> network_ = nullptr;

  FunEncoding encoding_ = FunEncoding::kNone;
  std::string sender_;

  FunapiEvent<ChannelNotify> on_joined_;
  FunapiEvent<ChannelNotify> on_left_;
  FunapiEvent<ErrorNotify> on_error_;

  void OnJoined(const std::string &channel_id, const std::string &sender);
  void OnLeft(const std::string &channel_id, const std::string &sender);
  void OnError(int error);

  std::map<std::string, ChannelMessage> channels_;
};


FunapiMulticastClientImpl::FunapiMulticastClientImpl(std::shared_ptr<FunapiNetwork> network, FunEncoding encoding)
  : network_(network), encoding_(encoding) {
  network_->RegisterHandler(kMulticastMsgType, [this](const fun::TransportProtocol p, const std::string &type, const std::vector<uint8_t> &v_body){ OnReceived(p, type, v_body); });
}


FunapiMulticastClientImpl::~FunapiMulticastClientImpl() {
  network_->RegisterHandler(kMulticastMsgType, [](const fun::TransportProtocol p, const std::string &type, const std::vector<uint8_t> &v_body) { });
}


void FunapiMulticastClientImpl::SetSender(const std::string &sender) {
  sender_ = sender;
}


void FunapiMulticastClientImpl::SetEncoding(const FunEncoding encodig) {
  encoding_ = encodig;
}


void FunapiMulticastClientImpl::AddJoinedCallback(const ChannelNotify &handler) {
  on_joined_ += handler;
}


void FunapiMulticastClientImpl::AddLeftCallback(const ChannelNotify &handler) {
  on_left_ += handler;
}


void FunapiMulticastClientImpl::AddErrorCallback(const ErrorNotify &handler) {
  on_error_ += handler;
}


void FunapiMulticastClientImpl::OnJoined(const std::string &channel_id, const std::string &sender) {
  on_joined_(channel_id, sender);
}


void FunapiMulticastClientImpl::OnLeft(const std::string &channel_id, const std::string &sender) {
  on_left_(channel_id, sender);
}


void FunapiMulticastClientImpl::OnError(const int error) {
  on_error_(error);
}


bool FunapiMulticastClientImpl::IsConnected() const {
  if (network_) {
    if (network_->IsConnected()) {
      return true;
    }
  }

  return false;
}


bool FunapiMulticastClientImpl::IsInChannel(const std::string &channel_id) const {
  if (channels_.find(channel_id) != channels_.end()) {
    return true;
  }

  return false;
}


bool FunapiMulticastClientImpl::JoinChannel(const std::string &channel_id, const ChannelMessage &handler) {
  if (!IsConnected()) {
    DebugUtils::Log("Not connected. First connect before join a multicast channel.");
    return false;
  }

  if (IsInChannel(channel_id)) {
    DebugUtils::Log("Already joined the channel: %s", channel_id.c_str());
    return false;
  }

  channels_[channel_id] = handler;

  if (encoding_ == FunEncoding::kJson) {
    rapidjson::Document msg;
    msg.SetObject();

    rapidjson::Value channel_id_node(channel_id.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kChannelId), channel_id_node, msg.GetAllocator());

    rapidjson::Value sender_node(sender_.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kSender), sender_node, msg.GetAllocator());

    rapidjson::Value join_node(true);
    msg.AddMember(rapidjson::StringRef(kJoin), join_node, msg.GetAllocator());

    // Convert JSON document to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msg.Accept(writer);
    std::string json_string = buffer.GetString();

    network_->SendMessage(kMulticastMsgType, json_string);
  }

  if (encoding_ == FunEncoding::kProtobuf) {

  }

  return true;
}


bool FunapiMulticastClientImpl::LeaveChannel(const std::string &channel_id) {
  if (!IsConnected()) {
    DebugUtils::Log("Not connected. If you are trying to leave a channel in which you were, connect first while preserving the session id you used for join.");
    return false;
  }

  if (!IsInChannel(channel_id)) {
    DebugUtils::Log("You are not in the channel: %s", channel_id.c_str());
    return false;
  }

  if (encoding_ == FunEncoding::kJson) {
    rapidjson::Document msg;
    msg.SetObject();

    rapidjson::Value channel_id_node(channel_id.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kChannelId), channel_id_node, msg.GetAllocator());

    rapidjson::Value sender_node(sender_.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kSender), sender_node, msg.GetAllocator());

    rapidjson::Value leave_node(true);
    msg.AddMember(rapidjson::StringRef(kLeave), leave_node, msg.GetAllocator());

    // Convert JSON document to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msg.Accept(writer);
    std::string json_string = buffer.GetString();

    network_->SendMessage(kMulticastMsgType, json_string);
  }

  if (encoding_ == FunEncoding::kProtobuf) {
    
  }

  channels_.erase(channel_id);

  return true;
}


bool FunapiMulticastClientImpl::SendToChannel(FunMulticastMessage &mcast_msg) {
  return true;
}


bool FunapiMulticastClientImpl::SendToChannel(std::string &json_string) {
  if (!IsConnected()) {
    DebugUtils::Log("Not connected. If you are trying to leave a channel in which you were, connect first while preserving the session id you used for join.");
    return false;
  }

  rapidjson::Document msg;
  msg.Parse<0>(json_string.c_str());

  std::string channel_id = msg[kChannelId].GetString();

  if (!IsInChannel(channel_id)) {
    DebugUtils::Log("You are not in the channel: %s", channel_id.c_str());
    return false;
  }

  rapidjson::Value sender_node(sender_.c_str(), msg.GetAllocator());
  msg.AddMember(rapidjson::StringRef(kSender), sender_node, msg.GetAllocator());

  // Convert JSON document to string
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  msg.Accept(writer);
  std::string send_json_string = buffer.GetString();

  network_->SendMessage(kMulticastMsgType, send_json_string);

  return true;
}


void FunapiMulticastClientImpl::OnReceived(const fun::TransportProtocol protocol, const std::string &type, const std::vector<uint8_t> &v_body) {
  fun::DebugUtils::Log("OnReceived");

  std::string channel_id = "";
  std::string sender = "";
  bool join = false;
  bool leave = false;
  int error_code = 0;

  if (encoding_ == fun::FunEncoding::kJson) {
    std::string body(v_body.cbegin(), v_body.cend());
    fun::DebugUtils::Log("multicast message\n%s", body.c_str());

    rapidjson::Document msg;
    msg.Parse<0>(body.c_str());

    channel_id = msg[kChannelId].GetString();

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

  if (encoding_ == fun::FunEncoding::kProtobuf) {
  }

  if (error_code != 0) {
    DebugUtils::Log("Multicast error - code: %d", error_code);
    OnError(error_code);
  }

  if (!IsInChannel(channel_id)) {
    DebugUtils::Log("You are not in the channel: %s", channel_id.c_str());
    return;
  }

  if (join) {
    DebugUtils::Log("%s joined the '%s' channel", sender.c_str(), channel_id.c_str());
    OnJoined(channel_id, sender);
  }
  else if (leave) {
    DebugUtils::Log("%s left the '%s' channel", sender.c_str(), channel_id.c_str());
    OnLeft(channel_id, sender);
  }
  else {
    channels_[channel_id](channel_id, sender, v_body);
  }
}


////////////////////////////////////////////////////////////////////////////////
// FunapiMulticastClient implementation.

FunapiMulticastClient::FunapiMulticastClient(std::shared_ptr<FunapiNetwork> network, FunEncoding encoding)
  : impl_(std::make_shared<FunapiMulticastClientImpl>(network, encoding)) {
}


FunapiMulticastClient::~FunapiMulticastClient() {
}


void FunapiMulticastClient::SetSender(const std::string &sender) {
  impl_->SetSender(sender);
}


void FunapiMulticastClient::SetEncoding(const FunEncoding encodig) {
  impl_->SetEncoding(encodig);
}


void FunapiMulticastClient::AddJoinedCallback(const ChannelNotify &handler) {
  impl_->AddJoinedCallback(handler);
}


void FunapiMulticastClient::AddLeftCallback(const ChannelNotify &handler) {
  impl_->AddLeftCallback(handler);
}


void FunapiMulticastClient::AddErrorCallback(const ErrorNotify &handler) {
  impl_->AddErrorCallback(handler);
}


bool FunapiMulticastClient::IsConnected() const {
  return impl_->IsConnected();
}


bool FunapiMulticastClient::IsInChannel(const std::string &channel_id) const {
  return impl_->IsInChannel(channel_id);
}


bool FunapiMulticastClient::JoinChannel(const std::string &channel_id, const ChannelMessage &handler) {
  return impl_->JoinChannel(channel_id, handler);
}


bool FunapiMulticastClient::LeaveChannel(const std::string &channel_id) {
  return impl_->LeaveChannel(channel_id);
}


bool FunapiMulticastClient::SendToChannel(FunMulticastMessage &mcast_msg) {
  return impl_->SendToChannel(mcast_msg);
}


bool FunapiMulticastClient::SendToChannel(std::string &json_string) {
  return impl_->SendToChannel(json_string);
}

}  // namespace fun
