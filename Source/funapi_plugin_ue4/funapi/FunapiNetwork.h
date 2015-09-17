// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

/** @file */

#ifndef SRC_FUNAPI_NETWORK_H_
#define SRC_FUNAPI_NETWORK_H_

#include <functional>
#include "rapidjson/document.h"
#include "funapi/network/fun_message.pb.h"


namespace fun {

#define LOG(x) UE_LOG(LogClass, Log, TEXT(x));
#define LOG1(x, a1) UE_LOG(LogClass, Log, TEXT(x), a1);
#define LOG2(x, a1, a2) UE_LOG(LogClass, Log, TEXT(x), a1, a2);

typedef std::string string;
typedef rapidjson::Document Json;


enum EncodingScheme {
  kUnknownEncoding = 0,
  kJsonEncoding,
  kProtobufEncoding,
};

// Funapi version
enum class FunapiVersion : int
{
  kProtocolVersion = 1,
  kPluginVersion = 1,
};

// Funapi transport protocol
enum class TransportProtocol : int
{
  kDefault = 0,
  kTcp,
  kUdp,
  kHttp
};

// Message encoding type
enum class FunEncoding
{
  kNone,
  kJson,
  kProtobuf
};

namespace {
  struct AsyncRequest;
}

class FunapiNetwork;
class FunapiTransport {
 public:
  typedef std::map<string, string> HeaderType;

  typedef std::function<void(const HeaderType&,const string&)> OnReceived;
  typedef std::function<void(void)> OnStopped;

  virtual ~FunapiTransport() {}
  virtual void RegisterEventHandlers(const OnReceived &cb1, const OnStopped &cb2) = 0;
  virtual void Start() = 0;
  virtual void Stop() = 0;
  virtual void SendMessage(Json &message) = 0;
  virtual void SendMessage(FunMessage &message) = 0;
  virtual bool Started() const = 0;
  virtual TransportProtocol Protocol() const = 0;
  virtual void SetNetwork(FunapiNetwork* network) = 0;

 protected:
  FunapiTransport() {}
};


class FunapiTransportImpl;
class FunapiTcpTransport : public FunapiTransport {
 public:
  FunapiTcpTransport(const string &hostname_or_ip, uint16_t port);
  virtual ~FunapiTcpTransport();

  virtual void RegisterEventHandlers(const OnReceived &cb1, const OnStopped &cb2);
  virtual void Start();
  virtual void Stop();
  virtual void SendMessage(Json &message);
  virtual void SendMessage(FunMessage &message);
  virtual bool Started() const;
  virtual TransportProtocol Protocol() const;
  virtual void SetNetwork(FunapiNetwork* network);

 private:
  FunapiTransportImpl *impl_;
};

class FunapiUdpTransport : public FunapiTransport {
 public:
  FunapiUdpTransport(const string &hostname_or_ip, uint16_t port);
  virtual ~FunapiUdpTransport();

  virtual void RegisterEventHandlers(const OnReceived &cb1, const OnStopped &cb2);
  virtual void Start();
  virtual void Stop();
  virtual void SendMessage(Json &message);
  virtual void SendMessage(FunMessage &message);
  virtual bool Started() const;
  virtual TransportProtocol Protocol() const;
  virtual void SetNetwork(FunapiNetwork* network);

 private:
  FunapiTransportImpl *impl_;
};


class FunapiHttpTransportImpl;
class FunapiHttpTransport : public FunapiTransport {
 public:
  FunapiHttpTransport(const string &hostname_or_ip, uint16_t port, bool https = false);
  virtual ~FunapiHttpTransport();

  virtual void RegisterEventHandlers(const OnReceived &cb1, const OnStopped &cb2);
  virtual void Start();
  virtual void Stop();
  virtual void SendMessage(Json &message);
  virtual void SendMessage(FunMessage &message);
  virtual bool Started() const;
  virtual TransportProtocol Protocol() const;
  virtual void SetNetwork(FunapiNetwork* network);

 private:
  FunapiHttpTransportImpl *impl_;
};


class FunapiNetworkImpl;
class FunapiNetwork {
 public:
  typedef std::function<void(const string &, const std::vector<uint8_t>&)> MessageHandler;
  typedef std::function<void(const string &)> OnSessionInitiated;
  typedef std::function<void()> OnSessionClosed;

  static void Initialize(time_t session_timeout = 3600);
  static void Finalize();

  FunapiNetwork(FunapiTransport *funapi_transport, int type,
                const OnSessionInitiated &on_session_initiated,
                const OnSessionClosed &on_session_closed);
  ~FunapiNetwork();

  void RegisterHandler(const string msg_type, const MessageHandler &handler);
  void Start();
  void Stop();
  void SendMessage(const string &msg_type, Json &body, TransportProtocol protocol = TransportProtocol::kDefault);
  void SendMessage(FunMessage &message, TransportProtocol protocol = TransportProtocol::kDefault);
  bool Started() const;
  bool Connected(TransportProtocol protocol = TransportProtocol::kDefault) const;
  void Update();
  void AttachTransport(FunapiTransport *funapi_transport);
  void PushTaskQueue(const std::function<void()> task);
  void PushAsyncQueue(const AsyncRequest r);

 private:
  FunapiNetworkImpl *impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_NETWORK_H_
