// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin.h"
#include "funapi_version.h"
#include "funapi_utils.h"
#include "funapi_transport.h"
#include "funapi_encryption.h"
#include "funapi_tasks.h"
#include "funapi_http.h"
#include "funapi/network/fun_message.pb.h"

namespace fun {

////////////////////////////////////////////////////////////////////////////////
// FunapiErrorImpl implementation.

class FunapiErrorImpl : public std::enable_shared_from_this<FunapiErrorImpl> {
 public:
  typedef FunapiError::ErrorCode ErrorCode;

  FunapiErrorImpl() = delete;
  FunapiErrorImpl(ErrorCode error_code);
  ~FunapiErrorImpl() = default;

  ErrorCode GetErrorCode();

 private:
  ErrorCode error_code_;
};


FunapiErrorImpl::FunapiErrorImpl (ErrorCode error_code)
: error_code_(error_code) {
}


FunapiError::ErrorCode FunapiErrorImpl::GetErrorCode() {
  return error_code_;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiError implementation.

FunapiError::FunapiError (ErrorCode error_code)
: impl_(std::make_shared<FunapiErrorImpl>(error_code)) {
}


std::shared_ptr<FunapiError> FunapiError::Create(ErrorCode error_code) {
  return std::make_shared<FunapiError>(error_code);
}


FunapiError::ErrorCode FunapiError::GetErrorCode() {
  return impl_->GetErrorCode();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiTransportOptionImpl implementation.

class FunapiTransportOptionImpl : public std::enable_shared_from_this<FunapiTransportOptionImpl> {
 public:
  FunapiTransportOptionImpl() = default;
  ~FunapiTransportOptionImpl() = default;

  virtual void SetEncryptionType(EncryptionType type) = 0;
  virtual EncryptionType GetEncryptionType() = 0;
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

  void SetEncryptionType(EncryptionType type);
  void SetEncryptionType(EncryptionType type, const std::string &public_key);
  EncryptionType GetEncryptionType();
  const std::string& GetPublicKey();

 private:
  bool disable_nagle_ = true;
  bool auto_reconnect_ = false;
  bool enable_ping_ = false;
  bool sequence_number_validation_ = false;
  int timeout_seconds_ = 10;
  EncryptionType encryption_type_ = static_cast<EncryptionType>(0);
  std::string public_key_;
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


void FunapiTcpTransportOptionImpl::SetEncryptionType(EncryptionType type) {
  encryption_type_ = type;
}


void FunapiTcpTransportOptionImpl::SetEncryptionType(EncryptionType type, const std::string &public_key) {
  encryption_type_ = type;
  public_key_ = public_key;
}


EncryptionType FunapiTcpTransportOptionImpl::GetEncryptionType() {
  return encryption_type_;
}


const std::string& FunapiTcpTransportOptionImpl::GetPublicKey() {
  return public_key_;
}

////////////////////////////////////////////////////////////////////////////////
// FunapiUdpTransportOptionImpl implementation.

class FunapiUdpTransportOptionImpl : public FunapiTransportOptionImpl {
 public:
  FunapiUdpTransportOptionImpl() = default;
  virtual ~FunapiUdpTransportOptionImpl() = default;

  void SetEncryptionType(EncryptionType type);
  EncryptionType GetEncryptionType();

 private:
  EncryptionType encryption_type_ = static_cast<EncryptionType>(0);
};


void FunapiUdpTransportOptionImpl::SetEncryptionType(EncryptionType type) {
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

  void SetEncryptionType(EncryptionType type);
  EncryptionType GetEncryptionType();

  void SetCACertFilePath(const std::string &path);
  const std::string& GetCACertFilePath();

 private:
  bool sequence_number_validation_ = false;
  bool use_https_ = false;
  EncryptionType encryption_type_ = static_cast<EncryptionType>(0);
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


void FunapiTcpTransportOption::SetEncryptionType(EncryptionType type) {
  impl_->SetEncryptionType(type);
}


EncryptionType FunapiTcpTransportOption::GetEncryptionType() {
  return impl_->GetEncryptionType();
}


void FunapiTcpTransportOption::SetEncryptionType(EncryptionType type, const std::string &public_key) {
  impl_->SetEncryptionType(type, public_key);
}


const std::string& FunapiTcpTransportOption::GetPublicKey() {
  return impl_->GetPublicKey();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiUdpTransportOption implementation.

FunapiUdpTransportOption::FunapiUdpTransportOption ()
: impl_(std::make_shared<FunapiUdpTransportOptionImpl>()) {
}


std::shared_ptr<FunapiUdpTransportOption> FunapiUdpTransportOption::Create() {
  return std::make_shared<FunapiUdpTransportOption>();
}


void FunapiUdpTransportOption::SetEncryptionType(EncryptionType type) {
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


void FunapiHttpTransportOption::SetEncryptionType(EncryptionType type) {
  impl_->SetEncryptionType(type);
}


EncryptionType FunapiHttpTransportOption::GetEncryptionType() {
  return impl_->GetEncryptionType();
}


void FunapiHttpTransportOption::SetCACertFilePath(const std::string &path) {
  impl_->SetCACertFilePath(path);
}


const std::string& FunapiHttpTransportOption::GetCACertFilePath() {
  return impl_->GetCACertFilePath();
}


void FunapiHttpTransportOption::SetConnectTimeout(const time_t seconds) {
  impl_->SetConnectTimeout(seconds);
}


time_t FunapiHttpTransportOption::GetConnectTimeout() {
  return impl_->GetConnectTimeout();
}

}  // namespace fun
