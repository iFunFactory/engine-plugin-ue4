// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

/** @file */

#ifndef SRC_FUNAPI_NETWORK_H_
#define SRC_FUNAPI_NETWORK_H_

#include "funapi_transport.h"

namespace fun {

// Funapi version
enum class FunapiVersion : int
{
  kProtocolVersion = 1,
  kPluginVersion = 2,
};

class FunapiNetworkImpl;
class FunapiNetwork : public std::enable_shared_from_this<FunapiNetwork> {
 public:
  typedef std::function<void(const std::string &, const std::vector<uint8_t>&)> MessageHandler;
  typedef std::function<void(const std::string &)> OnSessionInitiated;
  typedef std::function<void()> OnSessionClosed;

  static void Initialize(time_t session_timeout = 3600);
  static void Finalize();

  FunapiNetwork(int type,
                const OnSessionInitiated &on_session_initiated,
                const OnSessionClosed &on_session_closed);
  ~FunapiNetwork();

  void RegisterHandler(const std::string msg_type, const MessageHandler &handler);
  void Start();
  void Stop();
  void SendMessage(const std::string &msg_type, Json &body, TransportProtocol protocol = TransportProtocol::kDefault);
  void SendMessage(const std::string &msg_type, FJsonObject &body, TransportProtocol protocol = TransportProtocol::kDefault);
  void SendMessage(FunMessage &message, TransportProtocol protocol = TransportProtocol::kDefault);
  bool Started() const;
  bool Connected(TransportProtocol protocol = TransportProtocol::kDefault) const;
  void Update();
  void AttachTransport(std::shared_ptr<FunapiTransport> funapi_transport);
  void PushTaskQueue(const std::function<void()> task);

 private:
   std::shared_ptr<FunapiNetworkImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_NETWORK_H_
