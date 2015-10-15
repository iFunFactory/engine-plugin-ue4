// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

/** @file */

#pragma once

#include "funapi/network/fun_message.pb.h"

namespace fun {

////////////////////////////////////////////////////////////////////////////////
// Types.

typedef sockaddr_in Endpoint;
typedef std::function<void(const int)> AsyncConnectCallback;
typedef std::function<void(const size_t)> AsyncSendCallback;
typedef std::function<void(const size_t)> AsyncReceiveCallback;
typedef std::function<void(const int)> AsyncWebRequestCallback;
typedef std::function<void(void*, const int)> AsyncWebResponseCallback;

enum FunapiTransportType {
  kTcp = 1,
  kUdp,
  kHttp,
};

enum FunapiTransportState {
  kDisconnected = 0,
  kConnecting,
  kConnected,
};

enum WebRequestState {
  kWebRequestStart = 0,
  kWebRequestEnd,
};

struct AsyncRequest {
  enum RequestType {
    kConnect = 0,
    kSend,
    kSendTo,
    kReceive,
    kReceiveFrom,
    kWebRequest,
  };

  RequestType type_;
  int sock_;

  struct {
    AsyncConnectCallback callback_;
    Endpoint endpoint_;
  } connect_;

  struct AsyncSendContext {
    AsyncSendCallback callback_;
    uint8_t *buf_;
    size_t buf_len_;
    size_t buf_offset_;
  } send_;

  struct AsyncReceiveContext {
    AsyncReceiveCallback callback_;
    uint8_t *buf_;
    size_t capacity_;
  } recv_;

  struct AsyncSendToContext : AsyncSendContext {
    Endpoint endpoint_;
  } sendto_;

  struct AsyncReceiveFromContext : AsyncReceiveContext {
    Endpoint endpoint_;
  } recvfrom_;

  struct WebRequestContext {
    AsyncWebRequestCallback request_callback_;
    AsyncWebResponseCallback receive_header_;
    AsyncWebResponseCallback receive_body_;
    const char *url_;
    const char *header_;
    uint8_t *body_;
    size_t body_len_;
  } web_request_;
};

typedef std::list<struct iovec> IoVecList;

enum EncodingScheme {
  kUnknownEncoding = 0,
  kJsonEncoding,
  kProtobufEncoding,
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

class FunapiNetwork;
class FunapiTransport {
 public:
  typedef std::map<std::string, std::string> HeaderType;

  typedef std::function<void(const HeaderType&,const std::string&)> OnReceived;
  typedef std::function<void(void)> OnStopped;

  virtual ~FunapiTransport() {}
  virtual void RegisterEventHandlers(const OnReceived &cb1, const OnStopped &cb2) = 0;
  virtual void Start() = 0;
  virtual void Stop() = 0;
  virtual void SendMessage(Json &message) = 0;
  virtual void SendMessage(FJsonObject &message) = 0;
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
  FunapiTcpTransport(const std::string &hostname_or_ip, uint16_t port);
  virtual ~FunapiTcpTransport();

  virtual void RegisterEventHandlers(const OnReceived &cb1, const OnStopped &cb2);
  virtual void Start();
  virtual void Stop();
  virtual void SendMessage(Json &message);
  virtual void SendMessage(FJsonObject &message);
  virtual void SendMessage(FunMessage &message);
  virtual bool Started() const;
  virtual TransportProtocol Protocol() const;
  virtual void SetNetwork(FunapiNetwork* network);

 private:
  std::shared_ptr<FunapiTransportImpl> impl_;
};

class FunapiUdpTransport : public FunapiTransport {
 public:
  FunapiUdpTransport(const std::string &hostname_or_ip, uint16_t port);
  virtual ~FunapiUdpTransport();

  virtual void RegisterEventHandlers(const OnReceived &cb1, const OnStopped &cb2);
  virtual void Start();
  virtual void Stop();
  virtual void SendMessage(Json &message);
  virtual void SendMessage(FJsonObject &message);
  virtual void SendMessage(FunMessage &message);
  virtual bool Started() const;
  virtual TransportProtocol Protocol() const;
  virtual void SetNetwork(FunapiNetwork* network);

 private:
  std::shared_ptr<FunapiTransportImpl> impl_;
};

class FunapiHttpTransportImpl;
class FunapiHttpTransport : public FunapiTransport {
 public:
  FunapiHttpTransport(const std::string &hostname_or_ip, uint16_t port, bool https = false);
  virtual ~FunapiHttpTransport();

  virtual void RegisterEventHandlers(const OnReceived &cb1, const OnStopped &cb2);
  virtual void Start();
  virtual void Stop();
  virtual void SendMessage(Json &message);
  virtual void SendMessage(FJsonObject &message);
  virtual void SendMessage(FunMessage &message);
  virtual bool Started() const;
  virtual TransportProtocol Protocol() const;
  virtual void SetNetwork(FunapiNetwork* network);

 private:
  std::shared_ptr<FunapiHttpTransportImpl> impl_;
};

}  // namespace fun
