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
  kDefault,
};


inline FUNAPI_API std::string TransportProtocolToString(TransportProtocol protocol) {
  std::string ret("");

  switch (protocol) {
    case TransportProtocol::kDefault:
      ret = "Unknown";
      break;

    case TransportProtocol::kTcp:
      ret = "Tcp";
      break;

    case TransportProtocol::kUdp:
      ret = "Udp";
      break;

    case TransportProtocol::kHttp:
      ret = "Http";
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

enum class FUNAPI_API EncryptionType : int;


class FunapiErrorImpl;
class FUNAPI_API FunapiError : public std::enable_shared_from_this<FunapiError> {
 public:
  enum class ErrorCode : int {
    kNone,
    kRedirectConnectInvalidToken,
    kRedirectConnectExpired,
    kRedirectConnectAuthFailed,
  };

  FunapiError() = delete;
  FunapiError(ErrorCode code);
  virtual ~FunapiError() = default;

  static std::shared_ptr<FunapiError> Create(ErrorCode error_code);

  ErrorCode GetErrorCode();

 private:
  std::shared_ptr<FunapiErrorImpl> impl_;
};


class FUNAPI_API FunapiTransportOption : public std::enable_shared_from_this<FunapiTransportOption> {
 public:
  FunapiTransportOption() = default;
  virtual ~FunapiTransportOption() = default;

  virtual void SetEncryptionType(EncryptionType type) = 0;
  virtual EncryptionType GetEncryptionType() = 0;
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

  void SetEncryptionType(EncryptionType type);
  EncryptionType GetEncryptionType();

  void SetEncryptionType(EncryptionType type, const std::string &public_key);
  const std::string& GetPublicKey();

 private:
  std::shared_ptr<FunapiTcpTransportOptionImpl> impl_;
};


class FunapiUdpTransportOptionImpl;
class FUNAPI_API FunapiUdpTransportOption : public FunapiTransportOption {
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

  void SetEncryptionType(EncryptionType type);
  EncryptionType GetEncryptionType();

  void SetCACertFilePath(const std::string &path);
  const std::string& GetCACertFilePath();

 private:
  std::shared_ptr<FunapiHttpTransportOptionImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_TRANSPORT_H_
