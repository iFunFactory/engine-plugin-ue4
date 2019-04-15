// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_RPC_H_
#define SRC_FUNAPI_RPC_H_

#include "funapi_plugin.h"

#if FUNAPI_HAVE_RPC

#include "funapi/distribution/fun_dedicated_server_rpc_message.pb.h"

namespace fun {

class FunapiRpcOptionImpl;
class FUNAPI_API FunapiRpcOption : public std::enable_shared_from_this<FunapiRpcOption> {
 public:
  FunapiRpcOption();
  virtual ~FunapiRpcOption() = default;

  static std::shared_ptr<FunapiRpcOption> Create();

  void AddInitializer(const std::string &hostname_or_ip, const int port);
  std::vector<std::tuple<std::string, int>>& GetInitializers();

  void SetDisableNagle(const bool disable_nagle);
  bool GetDisableNagle();

  void SetConnectTimeout(const int seconds);
  int GetConnectTimeout();

  void SetTag(const std::string &tag);
  std::string GetTag();

 private:
  std::shared_ptr<FunapiRpcOptionImpl> impl_;
};


class FunapiRpcImpl;
class FUNAPI_API FunapiRpc : public std::enable_shared_from_this<FunapiRpc> {
 public:
  enum class EventType : int {
    kConnected,
    kDisconnected,
    kConnectionFailed,
    kConnectionTimedOut,
    kPeerId,
  };

  typedef std::function<void(const EventType type,
                             const std::string &hostname_or_ip,
                             const int port,
                             const std::string &peer_id)> EventHandler;

  typedef std::function<void(const FunDedicatedServerRpcMessage &response_message)> ResponseHandler;

  typedef std::function<void(const std::string &type,
                             const FunDedicatedServerRpcMessage &request_message,
                             const ResponseHandler &response_handler)> RpcHandler;

  FunapiRpc();
  virtual ~FunapiRpc() = default;

  static std::shared_ptr<FunapiRpc> Create();

  void Start(std::shared_ptr<FunapiRpcOption> option);
  void SetPeerEventHandler(const EventHandler &handler);
  void SetHandler(const std::string &type, const RpcHandler &handler);
  void Update();

  // debug function // don't use
  // void DebugCall(const FunDedicatedServerRpcMessage &debug_request);
  // void DebugMasterCall(const FunDedicatedServerRpcMessage &debug_request);

 private:
  std::shared_ptr<FunapiRpcImpl> impl_;
};

}  // namespace fun

#endif // FUNAPI_HAVE_RPC

#endif // SRC_FUNAPI_RPC_H_
