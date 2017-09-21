// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_TRANSPORT_H_
#define SRC_FUNAPI_TRANSPORT_H_

#include "funapi_build_config.h"

#include <memory>
#include <functional>
#include <string>
#include <assert.h>

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

enum class FUNAPI_API TransportState : int {
  kDisconnected = 0,
  kDisconnecting,
  kConnecting,
  kConnected
};

// Funapi transport protocol
enum class FUNAPI_API TransportProtocol : int
{
  kTcp = 0,
  kUdp,
  kHttp,
  kWebsocket,
  kDefault,
};


inline FUNAPI_API std::string TransportProtocolToString(TransportProtocol protocol) {
  std::string ret("");

  switch (protocol) {
    case TransportProtocol::kDefault:
      ret = "Default";
      break;

    case TransportProtocol::kTcp:
      ret = "TCP";
      break;

    case TransportProtocol::kUdp:
      ret = "UDP";
      break;

    case TransportProtocol::kHttp:
      ret = "HTTP";
      break;

    case TransportProtocol::kWebsocket:
      ret = "WEBSOCKET";
      break;

    default:
      assert(false);
  }

  return ret;
}


// Message encoding type
enum class FUNAPI_API FunEncoding
{
  kNone,
  kJson,
  kProtobuf
};


inline FUNAPI_API std::string EncodingToString(FunEncoding encoding) {
  std::string ret("");

  switch (encoding) {
    case FunEncoding::kJson:
      ret = "JSON";
      break;

    case FunEncoding::kProtobuf:
      ret = "Protobuf";
      break;

    default:
      assert(false);
  }

  return ret;
}


enum class FUNAPI_API EncryptionType : int;


class FunapiErrorImpl;
class FUNAPI_API FunapiError : public std::enable_shared_from_this<FunapiError> {
 public:
  enum class ErrorType : int {
    kDefault,
    kRedirect,
    kSocket,
    kCurl,
    kSeq,
    kPing,
    kWebsocket,
  };

  // legacy
  enum class ErrorCode : int {
    kNone,
    kRedirectConnectInvalidToken,
    kRedirectConnectExpired,
    kRedirectConnectAuthFailed,
  };

  FunapiError() = delete;
  FunapiError(const ErrorType type, const int code, const std::string& error_string);
  virtual ~FunapiError() = default;

  static std::shared_ptr<FunapiError> Create(const ErrorType type, const int code, const std::string& error_string = "");
  static std::shared_ptr<FunapiError> Create(const ErrorType type, const ErrorCode code);

  ErrorType GetErrorType();
  int GetErrorCode();
  std::string GetErrorString();

  std::string GetErrorTypeString();
  std::string DebugString();

 private:
  std::shared_ptr<FunapiErrorImpl> impl_;
};


class FUNAPI_API FunapiTransportOption : public std::enable_shared_from_this<FunapiTransportOption> {
 public:
  FunapiTransportOption() = default;
  virtual ~FunapiTransportOption() = default;
};


class FunapiTcpTransportOptionImpl;
class FUNAPI_API FunapiTcpTransportOption : public FunapiTransportOption {
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

  void SetEncryptionType(const EncryptionType type);
  std::vector<EncryptionType> GetEncryptionTypes();

  void SetEncryptionType(const EncryptionType type, const std::string &public_key);
  std::string GetPublicKey(const EncryptionType type);

 private:
  std::shared_ptr<FunapiTcpTransportOptionImpl> impl_;
};


class FunapiUdpTransportOptionImpl;
class FUNAPI_API FunapiUdpTransportOption : public FunapiTransportOption {
 public:
  FunapiUdpTransportOption();
  virtual ~FunapiUdpTransportOption() = default;

  static std::shared_ptr<FunapiUdpTransportOption> Create();

  void SetEncryptionType(const EncryptionType type);
  EncryptionType GetEncryptionType();

 private:
  std::shared_ptr<FunapiUdpTransportOptionImpl> impl_;
};


class FunapiHttpTransportOptionImpl;
class FUNAPI_API FunapiHttpTransportOption : public FunapiTransportOption {
 public:
  FunapiHttpTransportOption();
  virtual ~FunapiHttpTransportOption() = default;

  static std::shared_ptr<FunapiHttpTransportOption> Create();

  void SetSequenceNumberValidation(const bool validation);
  bool GetSequenceNumberValidation();

  void SetConnectTimeout(const time_t seconds);
  time_t GetConnectTimeout();

  void SetUseHttps(const bool https);
  bool GetUseHttps();

  void SetEncryptionType(const EncryptionType type);
  EncryptionType GetEncryptionType();

  void SetCACertFilePath(const std::string &path);
  const std::string& GetCACertFilePath();

 private:
  std::shared_ptr<FunapiHttpTransportOptionImpl> impl_;
};


class FunapiWebsocketTransportOptionImpl;
class FUNAPI_API FunapiWebsocketTransportOption : public FunapiTransportOption {
 public:
  FunapiWebsocketTransportOption();
  virtual ~FunapiWebsocketTransportOption() = default;

  static std::shared_ptr<FunapiWebsocketTransportOption> Create();

  void SetUseWss(const bool use_wss);
  bool GetUseWss();

 private:
  std::shared_ptr<FunapiWebsocketTransportOptionImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_TRANSPORT_H_
