// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_TRANSPORT_H_
#define SRC_FUNAPI_TRANSPORT_H_

#include "funapi_plugin.h"
#include "pb/network/fun_message.pb.h"


namespace fun {

typedef sockaddr_in Endpoint;
typedef std::function<void(void*, const int)> AsyncWebResponseCallback;

enum FunapiTransportState {
  kDisconnected = 0,
  kConnecting,
  kConnected,
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

// Error code
enum class ErrorCode
{
  kNone,
  kConnectFailed,
  kSendFailed,
  kReceiveFailed,
  kEncryptionFailed,
  kInvalidEncryption,
  kUnknownEncryption,
  kRequestTimeout,
  kDisconnected,
  kExceptionError
};

class FunapiNetwork;
class FunapiTransportImpl;
class FunapiTransport : public std::enable_shared_from_this<FunapiTransport> {
 public:
  enum class State : int
  {
    kUnknown = 0,
    kConnecting,
    kEncryptionHandshaking,
    kConnected,
    kWaitForSession,
    kWaitForAck,
    kEstablished
  };

  enum class ConnectState : int
  {
    kUnknown = 0,
    kConnecting,
    kReconnecting,
    kRedirecting,
    kConnected
  };

  typedef std::map<std::string, std::string> HeaderType;

  typedef std::function<void(const TransportProtocol, const FunEncoding, const HeaderType &, const std::vector<uint8_t> &)> OnReceived;

  // Event handler delegate
  typedef std::function<void(const TransportProtocol protocol)> TransportEventHandler;

  FunapiTransport() = default;
  virtual ~FunapiTransport() = default;

  virtual void Start() = 0; // Start connecting
  virtual void Stop() = 0; // Disconnection
  virtual bool IsStarted() const = 0; // Check connection

  // Send a message
  virtual void SendMessage(rapidjson::Document &message) = 0;
  virtual void SendMessage(FunMessage &message) = 0;
  virtual void SendMessage(const char *body) = 0;

  virtual TransportProtocol GetProtocol() const = 0;
  virtual FunEncoding GetEncoding() const = 0;

  virtual void SetNetwork(std::weak_ptr<FunapiNetwork> network) = 0;
  virtual void SetConnectTimeout(time_t timeout) = 0;

  virtual void AddStartedCallback(const TransportEventHandler &handler) = 0;
  virtual void AddStoppedCallback(const TransportEventHandler &handler) = 0;
  virtual void AddConnectFailedCallback(const TransportEventHandler &handler) = 0;
  virtual void AddConnectTimeoutCallback(const TransportEventHandler &handler) = 0;

  virtual void ResetClientPingTimeout() = 0;

  virtual int GetSocket();

  virtual void AddInitSocketCallback(const TransportEventHandler &handler);
  virtual void AddCloseSocketCallback(const TransportEventHandler &handler);

  virtual void OnSocketSelect(const fd_set rset, const fd_set wset, const fd_set eset);
  virtual void Update();

  virtual void SetDisableNagle(const bool disable_nagle);
  virtual void SetAutoReconnect(const bool auto_reconnect);
  virtual void SetEnablePing(const bool enable_ping);

  virtual void SetSendClientPingMessageHandler(std::function<bool(const TransportProtocol protocol)> handler);
  virtual void SetReceivedHandler(OnReceived handler) = 0;
};


class FunapiTcpTransportImpl;
class FunapiTcpTransport : public FunapiTransport {
 public:
  FunapiTcpTransport(const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding);
  virtual ~FunapiTcpTransport() = default;

  static std::shared_ptr<FunapiTcpTransport> create(const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding);

  void Start(); // Start connecting
  void Stop(); // Disconnection
  bool IsStarted() const; // Check connection

  // Send a message
  void SendMessage(rapidjson::Document &message);
  void SendMessage(FunMessage &message);
  void SendMessage(const char *body);

  TransportProtocol GetProtocol() const;
  FunEncoding GetEncoding() const;

  void SetNetwork(std::weak_ptr<FunapiNetwork> network);
  void SetConnectTimeout(time_t timeout);

  void AddStartedCallback(const TransportEventHandler &handler);
  void AddStoppedCallback(const TransportEventHandler &handler);
  void AddConnectFailedCallback(const TransportEventHandler &handler);
  void AddConnectTimeoutCallback(const TransportEventHandler &handler);

  void SetDisableNagle(const bool disable_nagle);
  void SetAutoReconnect(const bool auto_reconnect);
  void SetEnablePing(const bool enable_ping);

  void ResetClientPingTimeout();

  int GetSocket();

  void AddInitSocketCallback(const TransportEventHandler &handler);
  void AddCloseSocketCallback(const TransportEventHandler &handler);

  void OnSocketSelect(const fd_set rset, const fd_set wset, const fd_set eset);
  void Update();

  void SetSendClientPingMessageHandler(std::function<bool(const TransportProtocol protocol)> handler);
  void SetReceivedHandler(OnReceived handler);

 private:
  std::shared_ptr<FunapiTcpTransportImpl> impl_;
};


class FunapiUdpTransportImpl;
class FunapiUdpTransport : public FunapiTransport {
 public:
  FunapiUdpTransport(const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding);
  virtual ~FunapiUdpTransport() = default;

  static std::shared_ptr<FunapiUdpTransport> create(const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding);

  void Start(); // Start connecting
  void Stop(); // Disconnection
  bool IsStarted() const; // Check connection

  // Send a message
  void SendMessage(rapidjson::Document &message);
  void SendMessage(FunMessage &message);
  void SendMessage(const char *body);

  TransportProtocol GetProtocol() const;
  FunEncoding GetEncoding() const;

  void SetNetwork(std::weak_ptr<FunapiNetwork> network);
  void SetConnectTimeout(time_t timeout);

  void AddStartedCallback(const TransportEventHandler &handler);
  void AddStoppedCallback(const TransportEventHandler &handler);
  void AddConnectFailedCallback(const TransportEventHandler &handler);
  void AddConnectTimeoutCallback(const TransportEventHandler &handler);

  void ResetClientPingTimeout();

  int GetSocket();

  void AddInitSocketCallback(const TransportEventHandler &handler);
  void AddCloseSocketCallback(const TransportEventHandler &handler);

  void OnSocketSelect(const fd_set rset, const fd_set wset, const fd_set eset);
  void Update();

  void SetReceivedHandler(OnReceived handler);

 private:
  std::shared_ptr<FunapiUdpTransportImpl> impl_;
};


class FunapiHttpTransportImpl;
class FunapiHttpTransport : public FunapiTransport {
 public:
  FunapiHttpTransport(const std::string &hostname_or_ip, uint16_t port, bool https, FunEncoding encoding);
  virtual ~FunapiHttpTransport() = default;

  static std::shared_ptr<FunapiHttpTransport> create(const std::string &hostname_or_ip, uint16_t port, bool https, FunEncoding encoding);

  void Start(); // Start connecting
  void Stop(); // Disconnection
  bool IsStarted() const; // Check connection

  // Send a message
  void SendMessage(rapidjson::Document &message);
  void SendMessage(FunMessage &message);
  void SendMessage(const char *body);

  TransportProtocol GetProtocol() const;
  FunEncoding GetEncoding() const;

  void SetNetwork(std::weak_ptr<FunapiNetwork> network);
  void SetConnectTimeout(time_t timeout);

  void AddStartedCallback(const TransportEventHandler &handler);
  void AddStoppedCallback(const TransportEventHandler &handler);
  void AddConnectFailedCallback(const TransportEventHandler &handler);
  void AddConnectTimeoutCallback(const TransportEventHandler &handler);

  void ResetClientPingTimeout();

  void Update();

  void SetReceivedHandler(OnReceived handler);

 private:
  std::shared_ptr<FunapiHttpTransportImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_TRANSPORT_H_
