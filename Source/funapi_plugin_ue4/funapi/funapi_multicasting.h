// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_MULTICASTING_H_
#define SRC_FUNAPI_MULTICASTING_H_

#include "funapi_transport.h"
#include "pb/service/multicast_message.pb.h"

namespace fun {

class FunapiNetwork;
class FunapiMulticastClientImpl;
class FunapiMulticastClient : public std::enable_shared_from_this<FunapiMulticastClient> {
 public:
  typedef std::function<void(const std::string &, const std::string &)> ChannelNotify;
  typedef std::function<void(const std::string &, const std::string &, const std::vector<uint8_t> &)> ChannelMessage;
  typedef std::function<void(const int)> ErrorNotify;

  FunapiMulticastClient(std::shared_ptr<FunapiNetwork> network, FunEncoding encoding);
  ~FunapiMulticastClient();

  void SetSender(const std::string &sender);
  void SetEncoding(const FunEncoding encodig);

  void AddJoinedCallback(const ChannelNotify &handler);
  void AddLeftCallback(const ChannelNotify &handler);
  void AddErrorCallback(const ErrorNotify &handler);

  bool IsConnected() const;
  bool IsInChannel(const std::string &channel_id) const;

  bool JoinChannel(const std::string &channel_id, const ChannelMessage &handler);
  bool LeaveChannel(const std::string &channel_id);

  bool SendToChannel(FunMulticastMessage &mcast_msg);
  bool SendToChannel(std::string &json_string);

 private:
  std::shared_ptr<FunapiMulticastClientImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_MULTICASTING_H_
