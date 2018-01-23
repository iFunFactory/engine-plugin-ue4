// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifdef FUNAPI_UE4
#include "FunapiPrivatePCH.h"
#endif

#include "funapi_transport.h"
#include "funapi_version.h"
#include "funapi_utils.h"
#include "funapi_encryption.h"
#include "funapi_tasks.h"
#include "funapi_http.h"
#include "funapi/network/fun_message.pb.h"

namespace fun {

////////////////////////////////////////////////////////////////////////////////
// FunapiErrorImpl implementation.

class FunapiErrorImpl : public std::enable_shared_from_this<FunapiErrorImpl> {
 public:
  typedef FunapiError::ErrorType ErrorType;
  typedef FunapiError::ErrorCode ErrorCode;

  FunapiErrorImpl() = delete;
  FunapiErrorImpl(const ErrorType type,
                  const int code,
                  const std::string& error_string);
  virtual ~FunapiErrorImpl() = default;

  ErrorType GetErrorType();
  int GetErrorCode();
  std::string GetErrorString();

  std::string GetErrorTypeString();
  std::string DebugString();

 private:
  ErrorType error_type_;
  int error_code_;
  std::string error_string_;
};


FunapiErrorImpl::FunapiErrorImpl (const ErrorType type, const int code, const std::string& error_string)
: error_type_(type), error_code_(code), error_string_(error_string) {
}


FunapiErrorImpl::ErrorType FunapiErrorImpl::GetErrorType() {
  return error_type_;
}


int FunapiErrorImpl::GetErrorCode() {
  return error_code_;
}


std::string FunapiErrorImpl::GetErrorString() {
  return error_string_;
}


std::string FunapiErrorImpl::GetErrorTypeString() {
  std::string ret;

  switch (error_type_) {
    case ErrorType::kDefault:
      ret = "Default";
      break;

    case ErrorType::kRedirect:
      ret = "Redirect";
      break;

    case ErrorType::kSocket:
      ret = "Socket";
      break;

    case ErrorType::kCurl:
      ret = "Curl";
      break;

    case ErrorType::kSeq:
      ret = "Seq";
      break;

    case ErrorType::kPing:
      ret = "Ping";
      break;

    case ErrorType::kWebsocket:
      ret = "Websocket";
      break;

    default:
      ret = "None";
  }

  return ret;
}


std::string FunapiErrorImpl::DebugString() {
  std::stringstream ss;

  ss << "ErrorType=" << GetErrorTypeString();
  ss << ":";

  if (error_code_ != 0) {
    ss << "(" << error_code_ << ")";
  }

  if (error_string_.length() > 0) {
    ss << " " << error_string_;
  }

  return ss.str();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiError implementation.

FunapiError::FunapiError (const ErrorType type, const int code, const std::string& error_string)
: impl_(std::make_shared<FunapiErrorImpl>(type, code, error_string)) {
}


std::shared_ptr<FunapiError> FunapiError::Create(const ErrorType type, const int code, const std::string& error_string) {
  return std::make_shared<FunapiError>(type, code, error_string);
}


std::shared_ptr<FunapiError> FunapiError::Create(const ErrorType type, const ErrorCode code) {
  return std::make_shared<FunapiError>(type, static_cast<int>(code), std::string());
}


FunapiError::ErrorType FunapiError::GetErrorType() {
  return impl_->GetErrorType();
}


int FunapiError::GetErrorCode() {
  return impl_->GetErrorCode();
}


std::string FunapiError::GetErrorString() {
  return impl_->GetErrorString();
}


std::string FunapiError::GetErrorTypeString() {
  return impl_->GetErrorTypeString();
}


std::string FunapiError::DebugString() {
  return impl_->DebugString();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiTransportOptionImpl implementation.

class FunapiTransportOptionImpl : public std::enable_shared_from_this<FunapiTransportOptionImpl> {
 public:
  FunapiTransportOptionImpl() = default;
  virtual ~FunapiTransportOptionImpl() = default;
};


////////////////////////////////////////////////////////////////////////////////
// FunapiTcpTransportOptionImpl implementation.

class FunapiTcpTransportOptionImpl : public FunapiTransportOptionImpl {
 public:
  FunapiTcpTransportOptionImpl() = default;
  virtual ~FunapiTcpTransportOptionImpl() = default;

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
  void SetEncryptionType(const EncryptionType type,
                         const std::string &public_key);
  std::vector<EncryptionType> GetEncryptionTypes();
  std::string GetPublicKey(const EncryptionType type);

  void SetUseTLS(const bool use_tls);
  bool GetUseTLS();

  void SetCACertFilePath(const std::string &path);
  const std::string& GetCACertFilePath();

 private:
  bool disable_nagle_ = true;
  bool auto_reconnect_ = false;
  bool enable_ping_ = false;
  bool sequence_number_validation_ = false;
  int timeout_seconds_ = 10;
  std::vector<EncryptionType> encryption_types_;
  std::unordered_map<int32_t, std::string> pubilc_keys_;
  bool use_tls_ = false;
  std::string cert_file_path_;
};


void FunapiTcpTransportOptionImpl::SetDisableNagle(const bool disable_nagle) {
  disable_nagle_ = disable_nagle;
}


bool FunapiTcpTransportOptionImpl::GetDisableNagle() {
  return disable_nagle_;
}


void FunapiTcpTransportOptionImpl::SetAutoReconnect(const bool auto_reconnect) {
  auto_reconnect_ = auto_reconnect;
}


bool FunapiTcpTransportOptionImpl::GetAutoReconnect() {
  return auto_reconnect_;
}


void FunapiTcpTransportOptionImpl::SetEnablePing(const bool enable_ping) {
  enable_ping_ = enable_ping;
}


bool FunapiTcpTransportOptionImpl::GetEnablePing() {
  return enable_ping_;
}


void FunapiTcpTransportOptionImpl::SetSequenceNumberValidation(const bool validation) {
  sequence_number_validation_ = validation;
}


bool FunapiTcpTransportOptionImpl::GetSequenceNumberValidation() {
  return sequence_number_validation_;
}


void FunapiTcpTransportOptionImpl::SetConnectTimeout(const int seconds) {
  timeout_seconds_ = seconds;
}


int FunapiTcpTransportOptionImpl::GetConnectTimeout() {
  return timeout_seconds_;
}


void FunapiTcpTransportOptionImpl::SetEncryptionType(const EncryptionType type) {
  encryption_types_.push_back(type);
}


void FunapiTcpTransportOptionImpl::SetEncryptionType(const EncryptionType type,
                                                     const std::string &public_key) {
  SetEncryptionType(type);
  pubilc_keys_[static_cast<int32_t>(type)] = public_key;
}


std::vector<EncryptionType> FunapiTcpTransportOptionImpl::GetEncryptionTypes() {
  return encryption_types_;
}


std::string FunapiTcpTransportOptionImpl::GetPublicKey(const EncryptionType type) {
  if (pubilc_keys_.find(static_cast<int32_t>(type)) != pubilc_keys_.end()) {
    return pubilc_keys_[static_cast<int32_t>(type)];
  }

  return "";
}


void FunapiTcpTransportOptionImpl::SetUseTLS(const bool use_tls) {
  use_tls_ = use_tls;
}


bool FunapiTcpTransportOptionImpl::GetUseTLS() {
  return use_tls_;
}


void FunapiTcpTransportOptionImpl::SetCACertFilePath(const std::string &path) {
  cert_file_path_ = path;
}


const std::string& FunapiTcpTransportOptionImpl::GetCACertFilePath() {
  return cert_file_path_;
}

////////////////////////////////////////////////////////////////////////////////
// FunapiUdpTransportOptionImpl implementation.

class FunapiUdpTransportOptionImpl : public FunapiTransportOptionImpl {
 public:
  FunapiUdpTransportOptionImpl() = default;
  virtual ~FunapiUdpTransportOptionImpl() = default;

  void SetEncryptionType(const EncryptionType type);
  EncryptionType GetEncryptionType();

 private:
  EncryptionType encryption_type_ = static_cast<EncryptionType>(0);
};


void FunapiUdpTransportOptionImpl::SetEncryptionType(const EncryptionType type) {
  encryption_type_ = type;
}


EncryptionType FunapiUdpTransportOptionImpl::GetEncryptionType() {
  return encryption_type_;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiHttpTransportOptionImpl implementation.

class FunapiHttpTransportOptionImpl : public FunapiTransportOptionImpl {
 public:
  FunapiHttpTransportOptionImpl() = default;
  virtual ~FunapiHttpTransportOptionImpl() = default;

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
  bool sequence_number_validation_ = false;
  bool use_https_ = false;
  EncryptionType encryption_type_ = EncryptionType::kNoneEncryption;
  std::string cert_file_path_;
  time_t timeout_seconds_ = 5;
};


void FunapiHttpTransportOptionImpl::SetSequenceNumberValidation(const bool validation) {
  sequence_number_validation_ = validation;
}


bool FunapiHttpTransportOptionImpl::GetSequenceNumberValidation() {
  return sequence_number_validation_;
}


void FunapiHttpTransportOptionImpl::SetConnectTimeout(const time_t seconds) {
  timeout_seconds_ = seconds;
}


time_t FunapiHttpTransportOptionImpl::GetConnectTimeout() {
  return timeout_seconds_;
}


void FunapiHttpTransportOptionImpl::SetUseHttps(const bool https) {
  use_https_ = https;
}


bool FunapiHttpTransportOptionImpl::GetUseHttps() {
  return use_https_;
}


void FunapiHttpTransportOptionImpl::SetEncryptionType(EncryptionType type) {
  encryption_type_ = type;
}


EncryptionType FunapiHttpTransportOptionImpl::GetEncryptionType() {
  return encryption_type_;
}


void FunapiHttpTransportOptionImpl::SetCACertFilePath(const std::string &path) {
  cert_file_path_ = path;
}


const std::string& FunapiHttpTransportOptionImpl::GetCACertFilePath() {
  return cert_file_path_;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiWebsocketTransportOptionImpl implementation.

class FunapiWebsocketTransportOptionImpl : public FunapiTransportOptionImpl {
public:
  FunapiWebsocketTransportOptionImpl() = default;
  virtual ~FunapiWebsocketTransportOptionImpl() = default;

  void SetUseWss(const bool use_wss);
  bool GetUseWss();

private:
  bool use_wss_ = false;
};


void FunapiWebsocketTransportOptionImpl::SetUseWss(const bool wss) {
  use_wss_ = wss;
}


bool FunapiWebsocketTransportOptionImpl::GetUseWss() {
  return use_wss_;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiTcpTransportOption implementation.

FunapiTcpTransportOption::FunapiTcpTransportOption ()
: impl_(std::make_shared<FunapiTcpTransportOptionImpl>()) {
}


std::shared_ptr<FunapiTcpTransportOption> FunapiTcpTransportOption::Create() {
  return std::make_shared<FunapiTcpTransportOption>();
}


void FunapiTcpTransportOption::SetDisableNagle(const bool disable_nagle) {
  impl_->SetDisableNagle(disable_nagle);
}


bool FunapiTcpTransportOption::GetDisableNagle() {
  return impl_->GetDisableNagle();
}


void FunapiTcpTransportOption::SetAutoReconnect(const bool auto_reconnect) {
  return impl_->SetAutoReconnect(auto_reconnect);

}


bool FunapiTcpTransportOption::GetAutoReconnect() {
  return impl_->GetAutoReconnect();
}


void FunapiTcpTransportOption::SetEnablePing(const bool enable_ping) {
  impl_->SetEnablePing(enable_ping);
}


bool FunapiTcpTransportOption::GetEnablePing() {
  return impl_->GetEnablePing();
}


void FunapiTcpTransportOption::SetSequenceNumberValidation(const bool validation) {
  impl_->SetSequenceNumberValidation(validation);
}


bool FunapiTcpTransportOption::GetSequenceNumberValidation() {
  return impl_->GetSequenceNumberValidation();
}


void FunapiTcpTransportOption::SetConnectTimeout(const int seconds) {
  impl_->SetConnectTimeout(seconds);
}


int FunapiTcpTransportOption::GetConnectTimeout() {
  return impl_->GetConnectTimeout();
}


void FunapiTcpTransportOption::SetEncryptionType(const EncryptionType type) {
  impl_->SetEncryptionType(type);
}


std::vector<EncryptionType> FunapiTcpTransportOption::GetEncryptionTypes() {
  return impl_->GetEncryptionTypes();
}


void FunapiTcpTransportOption::SetEncryptionType(const EncryptionType type,
                                                 const std::string &public_key) {
  impl_->SetEncryptionType(type, public_key);
}


std::string FunapiTcpTransportOption::GetPublicKey(const EncryptionType type) {
  return impl_->GetPublicKey(type);
}


void FunapiTcpTransportOption::SetUseTLS(const bool use_tls) {
  impl_->SetUseTLS(use_tls);
}


bool FunapiTcpTransportOption::GetUseTLS() {
  return impl_->GetUseTLS();
}


#ifdef FUNAPI_UE4_PLATFORM_PS4
void FunapiTcpTransportOption::SetCACert(const std::string &cert) {
  impl_->SetCACertFilePath(cert);
}
#else
void FunapiTcpTransportOption::SetCACertFilePath(const std::string &path) {
  impl_->SetCACertFilePath(path);
}
#endif


const std::string& FunapiTcpTransportOption::GetCACertFilePath() {
  return impl_->GetCACertFilePath();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiUdpTransportOption implementation.

FunapiUdpTransportOption::FunapiUdpTransportOption ()
: impl_(std::make_shared<FunapiUdpTransportOptionImpl>()) {
}


std::shared_ptr<FunapiUdpTransportOption> FunapiUdpTransportOption::Create() {
  return std::make_shared<FunapiUdpTransportOption>();
}


void FunapiUdpTransportOption::SetEncryptionType(const EncryptionType type) {
  impl_->SetEncryptionType(type);
}


EncryptionType FunapiUdpTransportOption::GetEncryptionType() {
  return impl_->GetEncryptionType();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiHttpTransportOption implementation.

FunapiHttpTransportOption::FunapiHttpTransportOption ()
: impl_(std::make_shared<FunapiHttpTransportOptionImpl>()) {
}


std::shared_ptr<FunapiHttpTransportOption> FunapiHttpTransportOption::Create() {
  return std::make_shared<FunapiHttpTransportOption>();
}


void FunapiHttpTransportOption::SetSequenceNumberValidation(const bool validation) {
  impl_->SetSequenceNumberValidation(validation);
}


bool FunapiHttpTransportOption::GetSequenceNumberValidation() {
  return impl_->GetSequenceNumberValidation();
}


void FunapiHttpTransportOption::SetUseHttps(const bool https) {
  impl_->SetUseHttps(https);
}


bool FunapiHttpTransportOption::GetUseHttps() {
  return impl_->GetUseHttps();
}


void FunapiHttpTransportOption::SetEncryptionType(const EncryptionType type) {
  impl_->SetEncryptionType(type);
}


EncryptionType FunapiHttpTransportOption::GetEncryptionType() {
  return impl_->GetEncryptionType();
}


#ifdef FUNAPI_UE4_PLATFORM_PS4
void FunapiHttpTransportOption::SetCACert(const std::string &cert) {
  impl_->SetCACertFilePath(cert);
}
#else
void FunapiHttpTransportOption::SetCACertFilePath(const std::string &path) {
  impl_->SetCACertFilePath(path);
}
#endif


const std::string& FunapiHttpTransportOption::GetCACertFilePath() {
  return impl_->GetCACertFilePath();
}


void FunapiHttpTransportOption::SetConnectTimeout(const time_t seconds) {
  impl_->SetConnectTimeout(seconds);
}


time_t FunapiHttpTransportOption::GetConnectTimeout() {
  return impl_->GetConnectTimeout();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiWebsocketTransportOption implementation.

FunapiWebsocketTransportOption::FunapiWebsocketTransportOption()
  : impl_(std::make_shared<FunapiWebsocketTransportOptionImpl>()) {
}


std::shared_ptr<FunapiWebsocketTransportOption> FunapiWebsocketTransportOption::Create() {
  return std::make_shared<FunapiWebsocketTransportOption>();
}


/*
void FunapiWebsocketTransportOption::SetUseWss(const bool use_wss) {
  impl_->SetUseWss(use_wss);
}
*/


bool FunapiWebsocketTransportOption::GetUseWss() {
  return impl_->GetUseWss();
}

}  // namespace fun
