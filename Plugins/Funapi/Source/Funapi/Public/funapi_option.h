// Copyright (C) 2013-2018 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_OPTION_H_
#define SRC_FUNAPI_OPTION_H_

#include "funapi_plugin.h"

namespace fun {

enum class FUNAPI_API EncryptionType : int;
enum class FUNAPI_API CompressionType : int;

class FUNAPI_API FunapiTransportOption : public std::enable_shared_from_this<FunapiTransportOption> {
 public:
  FunapiTransportOption() = default;
  virtual ~FunapiTransportOption() = default;

  virtual void SetCompressionType(const CompressionType type) = 0;
  virtual std::vector<CompressionType> GetCompressionTypes() = 0;

#if FUNAPI_HAVE_ZSTD
  // virtual void SetZstdDictBase64String(const std::string &zstd_dict_base64string) = 0;
  virtual std::string GetZstdDictBase64String() = 0;
#endif
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

  void SetCompressionType(const CompressionType type);
  std::vector<CompressionType> GetCompressionTypes();

#if FUNAPI_HAVE_ZSTD
  // void SetZstdDictBase64String(const std::string &zstd_dict_base64string);
  std::string GetZstdDictBase64String();
#endif

  void SetEncryptionType(const EncryptionType type, const std::string &public_key);
  std::string GetPublicKey(const EncryptionType type);

#if FUNAPI_HAVE_TCP_TLS
  void SetUseTLS(const bool use_tls);
#endif
  bool GetUseTLS();

#ifdef FUNAPI_UE4_PLATFORM_PS4
  void SetCACert(const std::string &cert);
#else
  void SetCACertFilePath(const std::string &path);
#endif
  const std::string& GetCACertFilePath();

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

  void SetCompressionType(const CompressionType type);
  std::vector<CompressionType> GetCompressionTypes();

#if FUNAPI_HAVE_ZSTD
  // void SetZstdDictBase64String(const std::string &zstd_dict_base64string);
  std::string GetZstdDictBase64String();
#endif

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

  void SetCompressionType(const CompressionType type);
  std::vector<CompressionType> GetCompressionTypes();

#if FUNAPI_HAVE_ZSTD
  // void SetZstdDictBase64String(const std::string &zstd_dict_base64string);
  std::string GetZstdDictBase64String();
#endif

#ifdef FUNAPI_UE4_PLATFORM_PS4
  void SetCACert(const std::string &cert);
#else
  void SetCACertFilePath(const std::string &path);
#endif
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

  // void SetUseWss(const bool use_wss);
  bool GetUseWss();

  void SetCompressionType(const CompressionType type);
  std::vector<CompressionType> GetCompressionTypes();

#if FUNAPI_HAVE_ZSTD
  // void SetZstdDictBase64String(const std::string &zstd_dict_base64string);
  std::string GetZstdDictBase64String();
#endif

 private:
  std::shared_ptr<FunapiWebsocketTransportOptionImpl> impl_;
};


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


class FunapiSessionOptionImpl;
class FUNAPI_API FunapiSessionOption : public std::enable_shared_from_this<FunapiSessionOption> {
 public:
  FunapiSessionOption();
  virtual ~FunapiSessionOption() = default;

  static std::shared_ptr<FunapiSessionOption> Create();

  void SetSessionReliability(const bool reliability);
  bool GetSessionReliability();

#if FUNAPI_HAVE_DELAYED_ACK
  void SetDelayedAckIntervalMillisecond(const int millisecond);
#endif
  int GetDelayedAckIntervalMillisecond();

  void SetSendSessionIdOnlyOnce(const bool once);
  bool GetSendSessionIdOnlyOnce();

 private:
  std::shared_ptr<FunapiSessionOptionImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_OPTION_H_
