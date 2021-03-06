// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_MULTICASTING_H_
#define SRC_FUNAPI_MULTICASTING_H_

#include "funapi_plugin.h"
#include "funapi_session.h"
#include "funapi/service/multicast_message.pb.h"

class FunMessage;

namespace fun {

class FunapiMulticastImpl;
class FUNAPI_API FunapiMulticast : public std::enable_shared_from_this<FunapiMulticast> {
public:
  typedef std::function<void(const std::shared_ptr<FunapiMulticast>&,
                             const fun::string &,
                             const fun::string &)> ChannelNotify;

  typedef std::function<void(const std::shared_ptr<FunapiMulticast>&,
                             const int)> ErrorNotify;

  typedef std::function<void(const std::shared_ptr<FunapiMulticast>&,
                             const fun::map<fun::string, int> &)> ChannelListNotify;

  typedef std::function<void(const std::shared_ptr<FunapiMulticast>&,
                             const fun::string&,
                             const fun::string&,
                             const fun::string&)> JsonChannelMessageHandler;

  typedef std::function<void(const std::shared_ptr<FunapiMulticast>&,
                             const fun::string&,
                             const fun::string&,
                             const FunMessage&)> ProtobufChannelMessageHandler;

  typedef std::function<void(const std::shared_ptr<FunapiMulticast>&,
                             const SessionEventType,
                             const fun::string&,
                             const std::shared_ptr<FunapiError> &error)> SessionEventHandler;

  typedef std::function<void(const std::shared_ptr<FunapiMulticast>&,
                             const TransportEventType,
                             const std::shared_ptr<FunapiError>&)>TransportEventHandler;

  FunapiMulticast() = delete;
  FunapiMulticast(const char* sender,
                  const char* hostname_or_ip,
                  const uint16_t port,
                  const FunEncoding encoding,
                  const bool reliability,
                  const TransportProtocol protocol);

  FunapiMulticast(const char* sender,
                  const std::shared_ptr<FunapiSession> &session,
                  const TransportProtocol protocol);

  FunapiMulticast(const char* sender,
                  const char* hostname_or_ip,
                  const uint16_t port,
                  const FunEncoding encoding,
                  const TransportProtocol protocl,
                  const std::shared_ptr<FunapiTransportOption> &transport_opt,
                  const std::shared_ptr<FunapiSessionOption> &session_opt);

  virtual ~FunapiMulticast();

  static std::shared_ptr<FunapiMulticast> Create(const char* sender,
                                                 const char* hostname_or_ip,
                                                 const uint16_t port,
                                                 const FunEncoding encoding,
                                                 const bool reliability = false,
                                                 const TransportProtocol protocol = TransportProtocol::kTcp);

  static std::shared_ptr<FunapiMulticast> Create(const char* sender,
                                                 const std::shared_ptr<FunapiSession> &session,
                                                 const TransportProtocol protocol = TransportProtocol::kTcp);

  static std::shared_ptr<FunapiMulticast> Create(const char* sender,
                                                 const char* hostname_or_ip,
                                                 const uint16_t port,
                                                 const FunEncoding encoding,
                                                 const TransportProtocol protocl,
                                                 const std::shared_ptr<FunapiTransportOption> &transport_opt,
                                                 const std::shared_ptr<FunapiSessionOption> &session_opt);

  void AddJoinedCallback(const ChannelNotify &handler);
  void AddLeftCallback(const ChannelNotify &handler);
  void AddErrorCallback(const ErrorNotify &handler);
  void AddChannelListCallback(const ChannelListNotify &handler);

  // Deprecated fucntion. Please Use FunapiMulticast::JoinChannel(chnnel_id, protobuf_channel_message_handler, token) function
  void AddProtobufChannelMessageCallback(const fun::string &channel_id, const ProtobufChannelMessageHandler &handler);

  // Deprecated function. Please Use FunapiMulticast::JoinChannel(chnnel_id, json_channel_message_handler, token) function
  void AddJsonChannelMessageCallback(const fun::string &channel_id, const JsonChannelMessageHandler &handler);

  void AddSessionEventCallback(const FunapiMulticast::SessionEventHandler &handler);
  void AddTransportEventCallback(const FunapiMulticast::TransportEventHandler &handler);

  bool IsConnected() const;
  bool IsInChannel(const fun::string &channel_id) const;

  // Deprecated function. Please Use FunapiMulticast::JoinChannel(chnnel_id, message_handler, token) function
  bool JoinChannel(const fun::string &channel_id, const fun::string &token = "");

  bool JoinChannel(const fun::string &channel_id, const JsonChannelMessageHandler &handler, const fun::string &token = "");
  bool JoinChannel(const fun::string &channel_id, const ProtobufChannelMessageHandler &handler, const fun::string &token = "");

  bool LeaveChannel(const fun::string &channel_id);
  bool LeaveAllChannels();

  bool SendToChannel(const fun::string &channel_id, FunMessage &msg, const bool bounce = true);
  bool SendToChannel(const fun::string &channel_id, fun::string &json_string, const bool bounce = true);

  bool RequestChannelList();

  void Update();

  FunEncoding GetEncoding();

  void Connect();
  void Close();

private:
  std::shared_ptr<FunapiMulticastImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_MULTICASTING_H_
