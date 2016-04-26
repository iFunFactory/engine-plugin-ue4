// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin.h"
#include "funapi_utils.h"
#include "funapi_transport.h"
#include "funapi_encryption.h"

namespace fun {

enum class EncryptionType : int {
  kNoneEncryption = 0,
  kDefaultEncryption,
};


////////////////////////////////////////////////////////////////////////////////
// FunapiEncryptionImpl implementation.

class FunapiEncryptionImpl : public std::enable_shared_from_this<FunapiEncryptionImpl> {
};


////////////////////////////////////////////////////////////////////////////////
// FunapiEncryption implementation.

FunapiEncryption::FunapiEncryption() {
}


FunapiEncryption::~FunapiEncryption() {
}


void FunapiEncryption::SetEncryptionType(EncryptionType type) {
}


bool FunapiEncryption::Encrypt(HeaderFields &header_fields, std::vector<uint8_t> &body) {
  return true;
}


bool FunapiEncryption::Decrypt(HeaderFields &header_fields, std::vector<uint8_t> &body) {
  return true;
}


void FunapiEncryption::SetHeaderFieldsForHttpSend (HeaderFields &header_fields) {
}


void FunapiEncryption::SetHeaderFieldsForHttpRecv (HeaderFields &header_fields) {
}

}  // namespace fun
