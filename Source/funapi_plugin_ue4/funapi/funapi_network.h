// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_NETWORK_H_
#define SRC_FUNAPI_NETWORK_H_

#include "funapi_transport.h"


namespace fun {

enum class FunapiVersion : int
{
  kProtocolVersion = 1,
  kPluginVersion = 3,
};

class FunapiNetworkImpl;
class FunapiNetwork : public std::enable_shared_from_this<FunapiNetwork> {
 public:
  typedef std::function<void(const TransportProtocol, const std::string &, const std::vector<uint8_t> &)> MessageEventHandler;
  typedef std::function<void(const std::string &)> SessionInitHandler;
  typedef std::function<void()> SessionCloseHandler;
  typedef std::function<void()> NotifyHandler;
  typedef FunapiTransport::TransportEventHandler TransportEventHandler;

  FunapiNetwork(bool session_reliability = false);
  ~FunapiNetwork();

  void RegisterHandler(const std::string msg_type, const MessageEventHandler &handler);
  void Start();
  void Stop();
  void SendMessage(const std::string &msg_type, std::string &json_string, TransportProtocol protocol = TransportProtocol::kDefault);
  void SendMessage(FunMessage &message, TransportProtocol protocol = TransportProtocol::kDefault);

  bool IsStarted() const;
  bool IsConnected(const TransportProtocol protocol = TransportProtocol::kDefault) const;
  void Update();
  void AttachTransport(const std::shared_ptr<FunapiTransport> &transport);
  void PushTaskQueue(const std::function<void()> &task);

  std::shared_ptr<FunapiTransport> GetTransport(const TransportProtocol protocol) const;  
  bool HasTransport(const TransportProtocol protocol) const;
  void SetDefaultProtocol(const TransportProtocol protocol);

  bool IsSessionReliability() const;

  FunEncoding GetEncoding(const TransportProtocol protocol) const;
  TransportProtocol GetDefaultProtocol() const;

  void AddSessionInitiatedCallback(const SessionInitHandler &handler);
  void AddSessionClosedCallback(const SessionCloseHandler &handler);
  void AddMaintenanceCallback(const MessageEventHandler &handler);
  void AddStoppedAllTransportCallback(const NotifyHandler &handler);
  void AddTransportConnectFailedCallback(const TransportEventHandler &handler);
  void AddTransportDisconnectedCallback(const TransportEventHandler &handler);
  void AddTransportConnectTimeoutCallback(const TransportEventHandler &handler);  

 private:
  std::shared_ptr<FunapiNetworkImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_NETWORK_H_
