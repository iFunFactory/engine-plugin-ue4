// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_MULTICASTING_H_
#define SRC_FUNAPI_MULTICASTING_H_

#include "funapi_session.h"

namespace fun {

/*** DEPRECATED: The FunapiMulticastClient class should no longer be used.  FunapiMulticast is the replacement for FunapiMulticastClient ***/
class FunapiNetwork;
class FunapiMulticastClientImpl;
class FunapiMulticastClient : public std::enable_shared_from_this<FunapiMulticastClient> {
 public:
  typedef std::function<void(const std::string &, const std::string &)> ChannelNotify;
  typedef std::function<void(const std::string &, const std::string &, const std::vector<uint8_t> &)> ChannelMessage;
  typedef std::function<void(const int)> ErrorNotify;
  typedef std::function<void(const std::map<std::string, int> &)> ChannelListNotify;

  FunapiMulticastClient(std::shared_ptr<FunapiNetwork> network, FunEncoding encoding);
  ~FunapiMulticastClient();

  void SetSender(const std::string &sender);
  void SetEncoding(const FunEncoding encodig);

  void AddJoinedCallback(const ChannelNotify &handler);
  void AddLeftCallback(const ChannelNotify &handler);
  void AddErrorCallback(const ErrorNotify &handler);
  void AddChannelListCallback(const ChannelListNotify &handler);

  bool IsConnected() const;
  bool IsInChannel(const std::string &channel_id) const;

  bool JoinChannel(const std::string &channel_id, const ChannelMessage &handler);
  bool LeaveChannel(const std::string &channel_id);
  bool LeaveAllChannels();

  bool SendToChannel(FunMessage &msg);
  bool SendToChannel(std::string &json_string);

  bool RequestChannelList();

 private:
  std::shared_ptr<FunapiMulticastClientImpl> impl_;
};


class FunapiMulticastImpl;
class FunapiMulticast : public std::enable_shared_from_this<FunapiMulticast> {
public:
  typedef std::function<void(const std::shared_ptr<FunapiMulticast>&,
                             const std::string &,
                             const std::string &)> ChannelNotify;

  typedef std::function<void(const std::shared_ptr<FunapiMulticast>&,
                             const int)> ErrorNotify;

  typedef std::function<void(const std::shared_ptr<FunapiMulticast>&,
                             const std::map<std::string, int> &)> ChannelListNotify;

  typedef std::function<void(const std::shared_ptr<FunapiMulticast>&,
                             const std::string&,
                             const std::string&,
                             const std::string&)> JsonChannelMessageHandler;

  typedef std::function<void(const std::shared_ptr<FunapiMulticast>&,
                             const std::string&,
                             const std::string&,
                             const FunMessage&)> ProtobufChannelMessageHandler;

  typedef std::function<void(const std::shared_ptr<FunapiMulticast>&,
                             const SessionEventType,
                             const std::string&)> SessionEventHandler;

  typedef std::function<void(const std::shared_ptr<FunapiMulticast>&,
                             const TransportEventType,
                             const std::shared_ptr<FunapiError>&)>TransportEventHandler;

  FunapiMulticast() = delete;
  FunapiMulticast(const char* sender, const char* hostname_or_ip, uint16_t port, FunEncoding encoding, bool reliability);
  ~FunapiMulticast();

  static std::shared_ptr<FunapiMulticast> create(const char* sender,
                                                 const char* hostname_or_ip,
                                                 uint16_t port,
                                                 FunEncoding encoding,
                                                 bool reliability = false);

  void AddJoinedCallback(const ChannelNotify &handler);
  void AddLeftCallback(const ChannelNotify &handler);
  void AddErrorCallback(const ErrorNotify &handler);
  void AddChannelListCallback(const ChannelListNotify &handler);
  void AddProtobufChannelMessageCallback(const std::string &channel_id, const ProtobufChannelMessageHandler &handler);
  void AddJsonChannelMessageCallback(const std::string &channel_id, const JsonChannelMessageHandler &handler);
  void AddSessionEventCallback(const FunapiMulticast::SessionEventHandler &handler);
  void AddTransportEventCallback(const FunapiMulticast::TransportEventHandler &handler);

  bool IsConnected() const;
  bool IsInChannel(const std::string &channel_id) const;

  bool JoinChannel(const std::string &channel_id);

  bool LeaveChannel(const std::string &channel_id);
  bool LeaveAllChannels();

  bool SendToChannel(const std::string &channel_id, FunMessage &msg, const bool bounce = true);
  bool SendToChannel(const std::string &channel_id, std::string &json_string, const bool bounce = true);

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
