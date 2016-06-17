// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_TRANSPORT_H_
#define SRC_FUNAPI_TRANSPORT_H_

#include "funapi_plugin.h"
#include "network/fun_message.pb.h"


namespace fun {

// Funapi header-related constants.
static const char* kHeaderDelimeter = "\n";
static const char* kHeaderFieldDelimeter = ":";
static const char* kVersionHeaderField = "VER";
static const char* kPluginVersionHeaderField = "PVER";
static const char* kLengthHeaderField = "LEN";

// Funapi message-related constants.
static const char* kMsgTypeBodyField = "_msgtype";
static const char* kSessionIdBodyField = "_sid";
static const char* kSeqNumberField = "_seq";
static const char* kAckNumberField = "_ack";
static const char* kNewSessionMessageType = "_session_opened";
static const char* kSessionClosedMessageType = "_session_closed";
static const char* kMaintenanceMessageType = "_maintenance";

// Ping message-related constants.
static const char* kServerPingMessageType = "_ping_s";
static const char* kClientPingMessageType = "_ping_c";
static const char* kPingTimestampField = "timestamp";

// http header
static const char* kCookieRequestHeaderField = "Cookie";
static const char* kCookieResponseHeaderField = "SET-COOKIE";

typedef sockaddr_in Endpoint;
typedef std::function<void(void*, const int)> AsyncWebResponseCallback;

enum class TransportState : int {
  kDisconnected = 0,
  kConnecting,
  kConnected
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

enum class EncryptionType : int;

class FunapiNetwork;
class FunapiTransportImpl;
class FunapiTransport : public std::enable_shared_from_this<FunapiTransport> {
 public:
  typedef std::map<std::string, std::string> HeaderFields;

  typedef std::function<void(const TransportProtocol, const FunEncoding, const HeaderFields &, const std::vector<uint8_t> &)> TransportReceivedHandler;

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
  virtual void SendMessage(const char *body, bool use_sent_queue, uint32_t seq, bool priority = false) = 0;

  virtual TransportProtocol GetProtocol() const = 0;
  virtual FunEncoding GetEncoding() const = 0;

  virtual void SetConnectTimeout(time_t timeout) = 0;

  virtual void AddStartedCallback(const TransportEventHandler &handler) = 0;
  virtual void AddClosedCallback(const TransportEventHandler &handler) = 0;
  virtual void AddConnectFailedCallback(const TransportEventHandler &handler) = 0;
  virtual void AddConnectTimeoutCallback(const TransportEventHandler &handler) = 0;

  virtual void SetDisableNagle(const bool disable_nagle);
  virtual void SetAutoReconnect(const bool auto_reconnect);
  virtual void SetEnablePing(const bool enable_ping);
  virtual void SetSequenceNumberValidation(const bool validation);
  virtual void SetEncryptionType(EncryptionType type) = 0;

  virtual void SetSendClientPingMessageHandler(std::function<bool(const TransportProtocol protocol)> handler);
  virtual void SetReceivedHandler(TransportReceivedHandler handler) = 0;
  virtual void SetIsReliableSessionHandler(std::function<bool()> handler) = 0;
  virtual void SetSendAckHandler(std::function<void(const TransportProtocol protocol, const uint32_t seq)> handler);
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
  void SendMessage(const char *body, bool use_sent_queue, uint32_t seq, bool priority = false);

  TransportProtocol GetProtocol() const;
  FunEncoding GetEncoding() const;

  void SetConnectTimeout(time_t timeout);

  void AddStartedCallback(const TransportEventHandler &handler);
  void AddClosedCallback(const TransportEventHandler &handler);
  void AddConnectFailedCallback(const TransportEventHandler &handler);
  void AddConnectTimeoutCallback(const TransportEventHandler &handler);

  void SetDisableNagle(const bool disable_nagle);
  void SetAutoReconnect(const bool auto_reconnect);
  void SetEnablePing(const bool enable_ping);
  void SetSequenceNumberValidation(const bool validation);
  void SetEncryptionType(EncryptionType type);

  void SetSendClientPingMessageHandler(std::function<bool(const TransportProtocol protocol)> handler);
  void SetReceivedHandler(TransportReceivedHandler handler);
  void SetIsReliableSessionHandler(std::function<bool()> handler);
  void SetSendAckHandler(std::function<void(const TransportProtocol protocol, const uint32_t seq)> handler);

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
  void SendMessage(const char *body, bool use_sent_queue, uint32_t seq, bool priority = false);

  TransportProtocol GetProtocol() const;
  FunEncoding GetEncoding() const;

  void SetConnectTimeout(time_t timeout);

  void AddStartedCallback(const TransportEventHandler &handler);
  void AddClosedCallback(const TransportEventHandler &handler);
  void AddConnectFailedCallback(const TransportEventHandler &handler);
  void AddConnectTimeoutCallback(const TransportEventHandler &handler);

  void SetEncryptionType(EncryptionType type);

  void SetReceivedHandler(TransportReceivedHandler handler);
  void SetIsReliableSessionHandler(std::function<bool()> handler);
  void SetSendAckHandler(std::function<void(const TransportProtocol protocol, const uint32_t seq)> handler);

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
  void SendMessage(const char *body, bool use_sent_queue, uint32_t seq, bool priority = false);

  TransportProtocol GetProtocol() const;
  FunEncoding GetEncoding() const;

  void SetConnectTimeout(time_t timeout);
  void SetSequenceNumberValidation(const bool validation);
  void SetEncryptionType(EncryptionType type);

  void AddStartedCallback(const TransportEventHandler &handler);
  void AddClosedCallback(const TransportEventHandler &handler);
  void AddConnectFailedCallback(const TransportEventHandler &handler);
  void AddConnectTimeoutCallback(const TransportEventHandler &handler);

  void SetReceivedHandler(TransportReceivedHandler handler);
  void SetIsReliableSessionHandler(std::function<bool()> handler);
  void SetSendAckHandler(std::function<void(const TransportProtocol protocol, const uint32_t seq)> handler);

 private:
  std::shared_ptr<FunapiHttpTransportImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_TRANSPORT_H_
