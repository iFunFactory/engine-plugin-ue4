// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

////////////////////////////////////////////////////////////////////////////////
// Encryptor1 implementation.
class Encryptor1 : public Encryptor {
 public:
  Encryptor1();
  virtual ~Encryptor1();

  EncryptionType GetEncryptionType();
  static fun::string GetEncryptionName();

  bool Encrypt(fun::vector<uint8_t> &body);
  bool Decrypt(fun::vector<uint8_t> &body);
};


Encryptor1::Encryptor1() {
}


Encryptor1::~Encryptor1() {
}


EncryptionType Encryptor1::GetEncryptionType() {
  return EncryptionType::kIFunEngine1Encryption;
}


fun::string Encryptor1::GetEncryptionName() {
  return "ife1";
}


bool Encryptor1::Encrypt(fun::vector<uint8_t> &body) {
  return true;
}


bool Encryptor1::Decrypt(fun::vector<uint8_t> &body) {
  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Encryptor2 implementation.
class Encryptor2 : public Encryptor {
 public:
  Encryptor2();
  virtual ~Encryptor2();

  EncryptionType GetEncryptionType();
  static fun::string GetEncryptionName();

  bool Encrypt(fun::vector<uint8_t> &body);
  bool Decrypt(fun::vector<uint8_t> &body);
};


Encryptor2::Encryptor2() {
}


Encryptor2::~Encryptor2() {
}


EncryptionType Encryptor2::GetEncryptionType() {
  return EncryptionType::kIFunEngine2Encryption;
}


fun::string Encryptor2::GetEncryptionName() {
  return "ife2";
}


bool Encryptor2::Encrypt(fun::vector<uint8_t> &body) {
  return true;
}


bool Encryptor2::Decrypt(fun::vector<uint8_t> &body) {
  return true;
}
