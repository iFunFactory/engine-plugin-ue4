// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_TRANSPORT_H_
#define SRC_FUNAPI_TRANSPORT_H_

#include "funapi_plugin.h"

class FunMessage;

namespace fun {

static const char* kHeaderDelimeter = "\n";
static const char* kHeaderFieldDelimeter = ":";
static const char* kVersionHeaderField = "VER";
static const char* kPluginVersionHeaderField = "PVER";
static const char* kLengthHeaderField = "LEN";

static const char* kMessageTypeAttributeName = "_msgtype";
static const char* kSessionIdAttributeName = "_sid";
static const char* kSeqNumAttributeName = "_seq";
static const char* kAckNumAttributeName = "_ack";

static const char* kSessionOpenedMessageType = "_session_opened";
static const char* kSessionClosedMessageType = "_session_closed";
static const char* kMaintenanceMessageType = "_maintenance";
static const char* kServerPingMessageType = "_ping_s";
static const char* kClientPingMessageType = "_ping_c";
static const char* kRedirectMessageType = "_sc_redirect";
static const char* kRedirectConnectMessageType = "_cs_redirect_connect";
static const char* kPingTimestampField = "timestamp";

// http header
static const char* kCookieRequestHeaderField = "Cookie";
static const char* kCookieResponseHeaderField = "SET-COOKIE";

typedef std::function<void(void*, const int)> AsyncWebResponseCallback;

enum class TransportState : int {
  kDisconnected = 0,
  kDisconnecting,
  kConnecting,
  kConnected
};

// Funapi transport protocol
enum class TransportProtocol : int
{
  kTcp = 0,
  kUdp,
  kHttp,
  kDefault,
};

// Message encoding type
enum class FunEncoding
{
  kNone,
  kJson,
  kProtobuf
};

enum class EncryptionType : int;


class FunapiErrorImpl;
class FunapiError : public std::enable_shared_from_this<FunapiError> {
 public:
  enum class ErrorCode : int {
    kNone,
    kRedirectConnectInvalidToken,
    kRedirectConnectExpired,
    kRedirectConnectAuthFailed,
  };

  FunapiError() = delete;
  FunapiError(ErrorCode code);
  ~FunapiError() = default;

  static std::shared_ptr<FunapiError> Create(ErrorCode error_code);

  ErrorCode GetErrorCode();

 private:
  std::shared_ptr<FunapiErrorImpl> impl_;
};


class FunapiTransportOption : public std::enable_shared_from_this<FunapiTransportOption> {
 public:
  FunapiTransportOption() = default;
  virtual ~FunapiTransportOption() = default;

  virtual void SetEncryptionType(EncryptionType type) = 0;
  virtual EncryptionType GetEncryptionType() = 0;
};


class FunapiTcpTransportOptionImpl;
class FunapiTcpTransportOption : public FunapiTransportOption {
 public:
  FunapiTcpTransportOption();
  virtual ~FunapiTcpTransportOption() = default;

  static std::shared_ptr<FunapiTcpTransportOption> Create();

  void SetDisableNagle(const bool disable_nagle);
  bool GetDisableNagle();

  void SetAutoReconnect(const bool auto_reconnect);
  bool GetAutoReconnect();

  void SetEnablePing(const bool enable_ping);
  bool GetEnablePing();

  void SetSequenceNumberValidation(const bool validation);
  bool GetSequenceNumberValidation();

  void SetConnectTimeout(const int seconds);
  int GetConnectTimeout();

  void SetEncryptionType(EncryptionType type);
  EncryptionType GetEncryptionType();

 private:
  std::shared_ptr<FunapiTcpTransportOptionImpl> impl_;
};


class FunapiUdpTransportOptionImpl;
class FunapiUdpTransportOption : public FunapiTransportOption {
 public:
  FunapiUdpTransportOption();
  virtual ~FunapiUdpTransportOption() = default;

  static std::shared_ptr<FunapiUdpTransportOption> Create();

  void SetEncryptionType(EncryptionType type);
  EncryptionType GetEncryptionType();

 private:
  std::shared_ptr<FunapiUdpTransportOptionImpl> impl_;
};


class FunapiHttpTransportOptionImpl;
class FunapiHttpTransportOption : public FunapiTransportOption {
 public:
  FunapiHttpTransportOption();
  virtual ~FunapiHttpTransportOption() = default;

  static std::shared_ptr<FunapiHttpTransportOption> Create();

  void SetSequenceNumberValidation(const bool validation);
  bool GetSequenceNumberValidation();

  void SetUseHttps(const bool https);
  bool GetUseHttps();

  void SetEncryptionType(EncryptionType type);
  EncryptionType GetEncryptionType();

 private:
  std::shared_ptr<FunapiHttpTransportOptionImpl> impl_;
};


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
  virtual void SendMessage(const std::string &body, bool use_sent_queue, uint32_t seq, bool priority = false) = 0;

  virtual TransportProtocol GetProtocol() const = 0;
  virtual FunEncoding GetEncoding() const = 0;

  virtual void AddStartedCallback(const TransportEventHandler &handler) = 0;
  virtual void AddClosedCallback(const TransportEventHandler &handler) = 0;
  virtual void AddConnectFailedCallback(const TransportEventHandler &handler) = 0;
  virtual void AddConnectTimeoutCallback(const TransportEventHandler &handler) = 0;
  virtual void AddDisconnectedCallback(const TransportEventHandler &handler) = 0;

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

  static std::shared_ptr<FunapiTcpTransport> Create(const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding);

  void Start(); // Start connecting
  void Stop(); // Disconnection
  bool IsStarted() const; // Check connection

  // Send a message
  void SendMessage(rapidjson::Document &message);
  void SendMessage(FunMessage &message);
  void SendMessage(const std::string &body, bool use_sent_queue, uint32_t seq, bool priority = false);

  TransportProtocol GetProtocol() const;
  FunEncoding GetEncoding() const;

  void SetConnectTimeout(time_t timeout);

  void AddStartedCallback(const TransportEventHandler &handler);
  void AddClosedCallback(const TransportEventHandler &handler);
  void AddConnectFailedCallback(const TransportEventHandler &handler);
  void AddConnectTimeoutCallback(const TransportEventHandler &handler);
  void AddDisconnectedCallback(const TransportEventHandler &handler);

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

  static std::shared_ptr<FunapiUdpTransport> Create(const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding);

  void Start(); // Start connecting
  void Stop(); // Disconnection
  bool IsStarted() const; // Check connection

  // Send a message
  void SendMessage(rapidjson::Document &message);
  void SendMessage(FunMessage &message);
  void SendMessage(const std::string &body, bool use_sent_queue, uint32_t seq, bool priority = false);

  TransportProtocol GetProtocol() const;
  FunEncoding GetEncoding() const;

  void SetConnectTimeout(time_t timeout);
  void SetEncryptionType(EncryptionType type);

  void AddStartedCallback(const TransportEventHandler &handler);
  void AddClosedCallback(const TransportEventHandler &handler);
  void AddConnectFailedCallback(const TransportEventHandler &handler);
  void AddConnectTimeoutCallback(const TransportEventHandler &handler);
  void AddDisconnectedCallback(const TransportEventHandler &handler);

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

  static std::shared_ptr<FunapiHttpTransport> Create(const std::string &hostname_or_ip, uint16_t port, bool https, FunEncoding encoding);

  void Start(); // Start connecting
  void Stop(); // Disconnection
  bool IsStarted() const; // Check connection

  // Send a message
  void SendMessage(rapidjson::Document &message);
  void SendMessage(FunMessage &message);
  void SendMessage(const std::string &body, bool use_sent_queue, uint32_t seq, bool priority = false);

  TransportProtocol GetProtocol() const;
  FunEncoding GetEncoding() const;

  void SetConnectTimeout(time_t timeout);
  void SetSequenceNumberValidation(const bool validation);
  void SetEncryptionType(EncryptionType type);

  void AddStartedCallback(const TransportEventHandler &handler);
  void AddClosedCallback(const TransportEventHandler &handler);
  void AddConnectFailedCallback(const TransportEventHandler &handler);
  void AddConnectTimeoutCallback(const TransportEventHandler &handler);
  void AddDisconnectedCallback(const TransportEventHandler &handler);

  void SetReceivedHandler(TransportReceivedHandler handler);
  void SetIsReliableSessionHandler(std::function<bool()> handler);
  void SetSendAckHandler(std::function<void(const TransportProtocol protocol, const uint32_t seq)> handler);

 private:
  std::shared_ptr<FunapiHttpTransportImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_TRANSPORT_H_
