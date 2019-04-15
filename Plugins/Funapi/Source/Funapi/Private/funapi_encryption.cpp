// Copyright (C) 2013-2018 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifdef FUNAPI_UE4
#include "FunapiPrivatePCH.h"
#endif

#include "funapi_encryption.h"
#include "funapi_utils.h"
#include "funapi_session.h"

#if FUNAPI_HAVE_SODIUM
#define SODIUM_STATIC
#include "sodium.h"
#include "sodium/crypto_hash_sha512.h"
#include "sodium/crypto_scalarmult.h"
#include "sodium/crypto_stream_aes128ctr.h"
#include "sodium/crypto_stream_chacha20.h"
#include "sodium/randombytes.h"
#include "sodium/utils.h"
#endif // FUNAPI_HAVE_SODIUM

// http header
#define kProtocolHttpEncryptionField "X-iFun-Enc"

// Funapi header-related constants.
#define kEncryptionHeaderField "ENC"

// Encryption-releated constants.
#define kEncryptionHandshakeBegin "HELLO!"
#define kDelim1 "-"
#define kDelim2 ","


namespace fun {

////////////////////////////////////////////////////////////////////////////////
// Encryptor implementation.
class Encryptor : public std::enable_shared_from_this<Encryptor> {
 public:
  Encryptor();
  virtual ~Encryptor();

  static std::shared_ptr<Encryptor> Create(EncryptionType type);
  static std::shared_ptr<Encryptor> Create(std::string name);

  virtual EncryptionType GetEncryptionType() = 0;

  virtual void HandShake(const std::string &key);
  virtual const std::string& GetHandShakeString();

  virtual bool Encrypt(std::vector<uint8_t> &body) = 0;
  virtual bool Decrypt(std::vector<uint8_t> &body) = 0;

  bool IsHandShakeCompleted();

 protected:
  bool handshake_completed_ = false;
};


Encryptor::Encryptor() {
}


Encryptor::~Encryptor() {
}


void Encryptor::HandShake(const std::string &key) {
}


const std::string& Encryptor::GetHandShakeString() {
  static std::string empty_string("");
  return empty_string;
}


bool Encryptor::IsHandShakeCompleted() {
  return handshake_completed_;
}

////////////////////////////////////////////////////////////////////////////////
// Encryptor0 implementation.
class Encryptor0 : public Encryptor {
public:
  Encryptor0();
  virtual ~Encryptor0();

  EncryptionType GetEncryptionType();
  static std::string GetEncryptionName();

  bool Encrypt(std::vector<uint8_t> &body);
  bool Decrypt(std::vector<uint8_t> &body);
};


Encryptor0::Encryptor0() {
}


Encryptor0::~Encryptor0() {
}


EncryptionType Encryptor0::GetEncryptionType() {
  return EncryptionType::kDummyEncryption;
}


std::string Encryptor0::GetEncryptionName() {
  return "dummy";
}


bool Encryptor0::Encrypt(std::vector<uint8_t> &body) {
  return true;
}


bool Encryptor0::Decrypt(std::vector<uint8_t> &body) {
  return true;
}

////////////////////////////////////////////////////////////////////////////////
// Encryptor Legacy
#include "funapi_encryption_legacy.h"


#if FUNAPI_HAVE_SODIUM
////////////////////////////////////////////////////////////////////////////////
// EncryptorSodium implementation.
class EncryptorSodium : public Encryptor {
 public:
  EncryptorSodium();
  virtual ~EncryptorSodium() = default;

  virtual const std::string& GetHandShakeString();

 protected:
  static inline bool Unhexify(std::array<uint8_t, 32> *out,
                              const std::string &hex_str);
  static inline std::string Hexify(const uint8_t *v, size_t len);
  std::string GenerateSharedSecret(std::array<uint8_t, 64> *secret,
                                   const std::string &_server_pubkey);

  std::string handshake_string_;
};


EncryptorSodium::EncryptorSodium() {
  static auto sodium_init = FunapiInit::Create([](){
    if (::sodium_init() != 0) {
      // error
    }
  });
}


const std::string &EncryptorSodium::GetHandShakeString() {
  handshake_completed_ = true;
  return handshake_string_;
}


bool EncryptorSodium::Unhexify(std::array<uint8_t, 32> *out,
                               const std::string &hex_str) {
  if (hex_str.size() != 64) {
    return false;
  }
  return 0 == ::sodium_hex2bin(&(*out)[0],
                               out->size(),
                               hex_str.data(),
                               hex_str.size(),
                               nullptr, nullptr, nullptr);
}


std::string EncryptorSodium::Hexify(const uint8_t *v, size_t len) {
  std::string s;
  s.resize(len * 2 + 1);
  ::sodium_bin2hex(&s[0], s.size(), v, len);
  s.resize(len*2);
  return s;
}


std::string EncryptorSodium::GenerateSharedSecret(std::array<uint8_t, 64> *secret,
                                                  const std::string &_server_pubkey) {
  std::array<uint8_t, 32> server_pubkey;
  if (!Unhexify(&server_pubkey, _server_pubkey)) {
    return "";  // error
  }

  std::array<uint8_t, 32> my_seckey, my_pubkey;
  ::randombytes_buf(&my_seckey[0], 32);
  ::crypto_scalarmult_base(&my_pubkey[0], &my_seckey[0]);

  std::array<uint8_t, 32> q;
  if (0 != ::crypto_scalarmult(&q[0], &my_seckey[0], &server_pubkey[0])) {
    return "";  // error
  }

  ::crypto_hash_sha512_state h;
  ::crypto_hash_sha512_init(&h);
  ::crypto_hash_sha512_update(&h, &q[0], 32);
  ::crypto_hash_sha512_update(&h, &my_pubkey[0], 32);
  ::crypto_hash_sha512_update(&h, &server_pubkey[0], 32);
  ::crypto_hash_sha512_final(&h, &(*secret)[0]);

  return Hexify(&my_pubkey[0], 32);
}


////////////////////////////////////////////////////////////////////////////////
// EncryptorChacha20 implementation.
class EncryptorChacha20 : public EncryptorSodium {
 public:
  EncryptorChacha20();
  virtual ~EncryptorChacha20();

  EncryptionType GetEncryptionType();
  static std::string GetEncryptionName();

  void HandShake(const std::string &key);

  bool Encrypt(std::vector<uint8_t> &body);
  bool Decrypt(std::vector<uint8_t> &body);

 private:
  void GenerateChacha20Secret(std::array<uint8_t, 32> *key,
                              std::array<uint8_t, 8> *enc_nonce,
                              std::array<uint8_t, 8> *dec_nonce,
                              const std::string &server_pubkey);

  std::array<uint8_t, 8> enc_nonce_;
  uint64_t enc_count_ = 0;

  std::array<uint8_t, 8> dec_nonce_;
  uint64_t dec_count_ = 0;

  std::array<uint8_t, 32> key_;
};


EncryptorChacha20::EncryptorChacha20() {
}


EncryptorChacha20::~EncryptorChacha20() {
}


EncryptionType EncryptorChacha20::GetEncryptionType() {
  return EncryptionType::kChacha20Encryption;
}


std::string EncryptorChacha20::GetEncryptionName() {
  return "chacha20";
}


bool EncryptorChacha20::Encrypt(std::vector<uint8_t> &body) {
  if (handshake_completed_ == false)
    return false;

  if (!body.empty()) {
    ::crypto_stream_chacha20_xor_ic(body.data(),
                                    body.data(),
                                    body.size(),
                                    &enc_nonce_[0],
                                    enc_count_,
                                    &key_[0]);
    enc_count_ += body.size();
  }

  return true;
}


bool EncryptorChacha20::Decrypt(std::vector<uint8_t> &body) {
  if (!body.empty()) {
    if (0 != ::crypto_stream_chacha20_xor_ic(body.data(),
                                             body.data(),
                                             body.size(),
                                             &dec_nonce_[0],
                                             dec_count_,
                                             &key_[0])) {
      return false;
    }

    dec_count_ += body.size();
  }

  return true;
}


void EncryptorChacha20::HandShake(const std::string &key) {
  if (key.length() > 0) {
    GenerateChacha20Secret(&key_, &enc_nonce_, &dec_nonce_, key);
  }
}


void EncryptorChacha20::GenerateChacha20Secret(std::array<uint8_t, 32> *key,
                                   std::array<uint8_t, 8> *enc_nonce,
                                   std::array<uint8_t, 8> *dec_nonce,
                                   const std::string &server_pubkey) {
  std::array<uint8_t, 64> secret;

  handshake_string_ = GenerateSharedSecret(&secret, server_pubkey);

  ::memcpy(&(*key)[0], &secret[0], 32);
  ::memcpy(&(*dec_nonce)[0], &secret[0] + 32, 8);
  ::memcpy(&(*enc_nonce)[0], &secret[0] + 40, 8);
}
#else // FUNAPI_HAVE_SODIUM
class EncryptorSodium : public Encryptor {
public:
  EncryptorSodium();
  virtual ~EncryptorSodium() = default;

  virtual const std::string& GetHandShakeString();

protected:
  std::string handshake_string_;
};


EncryptorSodium::EncryptorSodium() {
}


const std::string &EncryptorSodium::GetHandShakeString() {
  handshake_completed_ = true;
  return handshake_string_;
}

////////////////////////////////////////////////////////////////////////////////
// EncryptorChacha20 implementation.
class EncryptorChacha20 : public EncryptorSodium {
public:
  EncryptorChacha20();
  virtual ~EncryptorChacha20();

  EncryptionType GetEncryptionType();
  static std::string GetEncryptionName();

  void HandShake(const std::string &key);

  bool Encrypt(std::vector<uint8_t> &body);
  bool Decrypt(std::vector<uint8_t> &body);
};


EncryptorChacha20::EncryptorChacha20() {
}


EncryptorChacha20::~EncryptorChacha20() {
}


EncryptionType EncryptorChacha20::GetEncryptionType() {
  return EncryptionType::kChacha20Encryption;
}


std::string EncryptorChacha20::GetEncryptionName() {
  return "chacha20";
}


bool EncryptorChacha20::Encrypt(std::vector<uint8_t> &body) {
  return true;
}


bool EncryptorChacha20::Decrypt(std::vector<uint8_t> &body) {
  return true;
}


void EncryptorChacha20::HandShake(const std::string &key) {
}
#endif // FUNAPI_HAVE_SODIUM

#if (FUNAPI_HAVE_SODIUM && FUNAPI_HAVE_AES128)
////////////////////////////////////////////////////////////////////////////////
// EncryptorAes128 implementation.
class EncryptorAes128 : public EncryptorSodium {
 public:
  EncryptorAes128();
  virtual ~EncryptorAes128();

  EncryptionType GetEncryptionType();
  static std::string GetEncryptionName();

  void HandShake(const std::string &key);

  bool Encrypt(std::vector<uint8_t> &body);
  bool Decrypt(std::vector<uint8_t> &body);

 private:
  void GenerateAes128Secret(std::array<uint8_t, 1408> *key,
                            std::array<uint8_t, 16> *enc_nonce,
                            std::array<uint8_t, 16> *dec_nonce,
                            const std::string &server_pubkey);

  std::array<uint8_t, 16> enc_nonce_;
  std::array<uint8_t, 16> dec_nonce_;

  // AES key expansion table
  std::array<uint8_t, 1408> table_;
};


EncryptorAes128::EncryptorAes128() {
}


EncryptorAes128::~EncryptorAes128() {
}


EncryptionType EncryptorAes128::GetEncryptionType() {
  return EncryptionType::kAes128Encryption;
}


std::string EncryptorAes128::GetEncryptionName() {
  return "aes128";
}


bool EncryptorAes128::Encrypt(std::vector<uint8_t> &body) {
  if (handshake_completed_ == false)
    return false;

  if (!body.empty()) {
    ::crypto_stream_aes128ctr_xor_afternm(body.data(),
                                          body.data(),
                                          body.size(),
                                          &enc_nonce_[0],
                                          &table_[0]);
    ::sodium_increment(&enc_nonce_[0], 16);
  }

  return true;
}


bool EncryptorAes128::Decrypt(std::vector<uint8_t> &body) {
  if (!body.empty()) {
    if (0 != ::crypto_stream_aes128ctr_xor_afternm(body.data(),
                                                   body.data(),
                                                   body.size(),
                                                   &dec_nonce_[0],
                                                   &table_[0])) {
      return false;
    }

    ::sodium_increment(&dec_nonce_[0], 16);
  }

  return true;
}


void EncryptorAes128::HandShake(const std::string &key) {
  if (key.length() > 0) {
    GenerateAes128Secret(&table_, &enc_nonce_, &dec_nonce_, key);
  }
}


void EncryptorAes128::GenerateAes128Secret(std::array<uint8_t, 1408> *key,
                                           std::array<uint8_t, 16> *enc_nonce,
                                           std::array<uint8_t, 16> *dec_nonce,
                                           const std::string &server_pubkey) {
  std::array<uint8_t, 64> secret;
  std::array<uint8_t, 16> _key;

  handshake_string_ = GenerateSharedSecret(&secret, server_pubkey);

  ::memcpy(&_key[0], &secret[0], 16);
  ::memcpy(&(*dec_nonce)[0], &secret[0] + 16, 16);
  ::memcpy(&(*enc_nonce)[0], &secret[0] + 32, 16);

  ::crypto_stream_aes128ctr_beforenm(&(*key)[0], &_key[0]);
}
#else
////////////////////////////////////////////////////////////////////////////////
// EncryptorAes128 implementation.
class EncryptorAes128 : public EncryptorSodium {
 public:
  EncryptorAes128();
  virtual ~EncryptorAes128();

  EncryptionType GetEncryptionType();
  static std::string GetEncryptionName();

  void HandShake(const std::string &key);

  bool Encrypt(std::vector<uint8_t> &body);
  bool Decrypt(std::vector<uint8_t> &body);
};


EncryptorAes128::EncryptorAes128() {
}


EncryptorAes128::~EncryptorAes128() {
}


EncryptionType EncryptorAes128::GetEncryptionType() {
  return EncryptionType::kAes128Encryption;
}


std::string EncryptorAes128::GetEncryptionName() {
  return "aes128";
}


bool EncryptorAes128::Encrypt(std::vector<uint8_t> &body) {
  return true;
}


bool EncryptorAes128::Decrypt(std::vector<uint8_t> &body) {
  return true;
}


void EncryptorAes128::HandShake(const std::string &key) {
}
#endif

////////////////////////////////////////////////////////////////////////////////
// Encryptor implementation.
// Factory


std::shared_ptr<Encryptor> Encryptor::Create(EncryptionType type) {
  if (type == EncryptionType::kDummyEncryption)
    return std::make_shared<Encryptor0>();
  else if (type == EncryptionType::kIFunEngine1Encryption)
    return std::make_shared<Encryptor1>();
  else if (type == EncryptionType::kIFunEngine2Encryption)
    return std::make_shared<Encryptor2>();
  else if (type == EncryptionType::kChacha20Encryption)
    return std::make_shared<EncryptorChacha20>();
  else if (type == EncryptionType::kAes128Encryption)
    return std::make_shared<EncryptorAes128>();

  return nullptr;
}


std::shared_ptr<Encryptor> Encryptor::Create(std::string name) {
  if (name.compare(Encryptor0::GetEncryptionName()) == 0)
    return std::make_shared<Encryptor0>();
  else if (name.compare(Encryptor1::GetEncryptionName()) == 0)
    return std::make_shared<Encryptor1>();
  else if (name.compare(Encryptor2::GetEncryptionName()) == 0)
    return std::make_shared<Encryptor2>();
  else if (name.compare(EncryptorChacha20::GetEncryptionName()) == 0)
    return std::make_shared<EncryptorChacha20>();
  else if (name.compare(EncryptorAes128::GetEncryptionName()) == 0)
    return std::make_shared<EncryptorAes128>();

  return nullptr;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiEncryptionImpl implementation.

class FunapiEncryptionImpl : public std::enable_shared_from_this<FunapiEncryptionImpl> {
 public:
  typedef FunapiEncryption::HeaderFields HeaderFields;

  FunapiEncryptionImpl();
  ~FunapiEncryptionImpl();

  void SetEncryptionType(EncryptionType type);
  void SetEncryptionType(EncryptionType type, const std::string &key);

  bool Encrypt(HeaderFields &header_fields,
               std::vector<uint8_t> &body,
               const EncryptionType encryption_type);
  bool Decrypt(HeaderFields &header_fields,
               std::vector<uint8_t> &body,
               std::vector<EncryptionType>& encryption_types);

  void SetHeaderFieldsForHttpSend (HeaderFields &header_fields);
  void SetHeaderFieldsForHttpRecv (HeaderFields &header_fields);

  bool UseSodium();
  bool HasEncryption(const EncryptionType type);
  bool IsHandShakeCompleted(const EncryptionType type);
  bool IsHandShakeCompleted();

 private:
  bool CreateEncryptor(const EncryptionType type);

  std::shared_ptr<Encryptor> GetEncryptor(EncryptionType type);
  std::map<EncryptionType, std::shared_ptr<Encryptor>> encryptors_;
  EncryptionType encryptor_type_ = EncryptionType::kDefaultEncryption;
};


FunapiEncryptionImpl::FunapiEncryptionImpl() {
}


FunapiEncryptionImpl::~FunapiEncryptionImpl() {
}


std::shared_ptr<Encryptor> FunapiEncryptionImpl::GetEncryptor(EncryptionType type) {
  if (type == EncryptionType::kDefaultEncryption) {
    if (!encryptors_.empty()) {
      return encryptors_.cbegin()->second;
    }
    else {
      return nullptr;
    }
  }

  auto iter = encryptors_.find(type);
  if (iter != encryptors_.cend())
    return iter->second;

  return nullptr;
}


bool FunapiEncryptionImpl::CreateEncryptor(const EncryptionType type) {
  if (GetEncryptor(type)) {
    return false;
  }

  std::shared_ptr<Encryptor> e = Encryptor::Create(type);
  if (e) {
    encryptors_[type] = e;
  }

  return true;
}


bool FunapiEncryptionImpl::Decrypt(HeaderFields &header_fields,
                                   std::vector<uint8_t> &body,
                                   std::vector<EncryptionType>& encryption_types) {
  std::string encryption_header;
  std::string encryption_str("");

  HeaderFields::iterator it;
  it = header_fields.find(kLengthHeaderField);
  size_t body_length = atoi(it->second.c_str());

  it = header_fields.find(kEncryptionHeaderField);
  if (it != header_fields.end()) {
    size_t index = it->second.find(kDelim1);
    encryption_header = it->second;

    if (index != std::string::npos) {
      encryption_str = encryption_header.substr(0, index);
      encryption_header = encryption_header.substr(index+1);
    }
    else if (encryption_header.compare(" ") != 0) {
      encryption_str = encryption_header;
    }
  }

  if (encryption_str.compare(kEncryptionHandshakeBegin) == 0) {
    if (encryption_header.length() > 0) {
      size_t begin = 0;
      size_t end = encryption_header.find(kDelim2);
      EncryptionType type;

      while (end != std::string::npos) {
        type = static_cast<EncryptionType>(atoi(encryption_header.substr(begin, end-begin).c_str()));
        CreateEncryptor(type);

        begin = end + 1;
        end = encryption_header.find(kDelim2, begin);
      }

      type = static_cast<EncryptionType>(atoi(encryption_header.substr(begin).c_str()));
      CreateEncryptor(type);

      if (HasEncryption(EncryptionType::kChacha20Encryption)) {
        encryption_types.push_back(EncryptionType::kChacha20Encryption);
      }
      if (HasEncryption(EncryptionType::kAes128Encryption)) {
        encryption_types.push_back(EncryptionType::kAes128Encryption);
      }
      if (false == HasEncryption(EncryptionType::kIFunEngine1Encryption)) {
        encryption_types.push_back(EncryptionType::kDefaultEncryption);
      }

    }
    else {
      encryption_types.push_back(EncryptionType::kDefaultEncryption);
    }
  }
  else if (encryption_str.length() > 0) {
    EncryptionType type = static_cast<EncryptionType>(atoi(encryption_str.c_str()));
    if (type == EncryptionType::kIFunEngine1Encryption) {
      std::shared_ptr<Encryptor1> e1 = std::static_pointer_cast<Encryptor1>(GetEncryptor(type));

      if (e1) {
        if (false == e1->IsHandShakeCompleted()) {
          encryption_types.push_back(EncryptionType::kIFunEngine1Encryption);
          e1->HandShake(encryption_header);
        }
      }
    }

    if (body_length > 0) {
      std::shared_ptr<Encryptor> e = GetEncryptor(type);
      if (e) {
        e->Decrypt(body);
      }
    }
  }

  return true;
}


bool FunapiEncryptionImpl::Encrypt(HeaderFields &header_fields,
                                   std::vector<uint8_t> &body,
                                   const EncryptionType encryption_type) {
  std::shared_ptr<Encryptor> e = GetEncryptor(encryption_type);

  if (e) {
    std::stringstream ss_encryption_type;
    ss_encryption_type << static_cast<int>(e->GetEncryptionType());

    bool use_encrypt = true;
    if (e->IsHandShakeCompleted() == false) {
      if (e->GetEncryptionType() == EncryptionType::kIFunEngine1Encryption) {
        return false;
      }

      ss_encryption_type << "-" << e->GetHandShakeString();
      use_encrypt = false;

      body.resize(0);
      header_fields[kLengthHeaderField] = "0";
    }

    header_fields[kEncryptionHeaderField] = ss_encryption_type.str();

    if (use_encrypt) {
      return e->Encrypt(body);
    }
  }

  return true;
}


void FunapiEncryptionImpl::SetHeaderFieldsForHttpSend (HeaderFields &header_fields) {
  HeaderFields::const_iterator it = header_fields.find(kEncryptionHeaderField);
  if (it != header_fields.end()) {
    header_fields[kProtocolHttpEncryptionField] = it->second;
  }
}


void FunapiEncryptionImpl::SetHeaderFieldsForHttpRecv (HeaderFields &header_fields) {
  HeaderFields::const_iterator it = header_fields.find(kProtocolHttpEncryptionField);
  if (it != header_fields.end()) {
    header_fields[kEncryptionHeaderField] = it->second;
  }
}


void FunapiEncryptionImpl::SetEncryptionType(EncryptionType type) {
  std::shared_ptr<Encryptor> e = Encryptor::Create(type);
  if (e) {
    encryptors_[type] = e;
    encryptor_type_ = type;
  }
}


void FunapiEncryptionImpl::SetEncryptionType(EncryptionType type, const std::string &key) {
  std::shared_ptr<Encryptor> e = Encryptor::Create(type);
  if (e) {
    encryptors_[type] = e;
    encryptor_type_ = type;

    if (key.length() > 0) {
      if (e->GetEncryptionType() == EncryptionType::kChacha20Encryption ||
          e->GetEncryptionType() == EncryptionType::kAes128Encryption) {
        e->HandShake(key);
      }
    }
  }
}


bool FunapiEncryptionImpl::UseSodium() {
  if (GetEncryptor(EncryptionType::kChacha20Encryption)) {
    return true;
  }

  if (GetEncryptor(EncryptionType::kAes128Encryption)) {
    return true;
  }

  return false;
}


bool FunapiEncryptionImpl::IsHandShakeCompleted(const EncryptionType type) {
  if (auto e = GetEncryptor(type)) {
    return e->IsHandShakeCompleted();
  }

  return false;
}


bool FunapiEncryptionImpl::IsHandShakeCompleted() {
  for (auto iter : encryptors_) {
    if (false == iter.second->IsHandShakeCompleted()) {
      return false;
    }
  }

  return true;
}


bool FunapiEncryptionImpl::HasEncryption(const EncryptionType type) {
  if (auto e = GetEncryptor(type)) {
    return true;
  }

  return false;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiEncryption implementation.

FunapiEncryption::FunapiEncryption()
  : impl_(std::make_shared<FunapiEncryptionImpl>()) {
}


FunapiEncryption::~FunapiEncryption() {
}


void FunapiEncryption::SetEncryptionType(EncryptionType type) {
  return impl_->SetEncryptionType(type);
}


void FunapiEncryption::SetEncryptionType(EncryptionType type, const std::string &key) {
  return impl_->SetEncryptionType(type, key);
}


bool FunapiEncryption::Encrypt(HeaderFields &header_fields,
                               std::vector<uint8_t> &body,
                               const EncryptionType encryption_type) {
  return impl_->Encrypt(header_fields, body, encryption_type);
}


bool FunapiEncryption::Decrypt(HeaderFields &header_fields,
                               std::vector<uint8_t> &body,
                               std::vector<EncryptionType>& encryption_types) {
  return impl_->Decrypt(header_fields, body, encryption_types);
}


void FunapiEncryption::SetHeaderFieldsForHttpSend (HeaderFields &header_fields) {
  return impl_->SetHeaderFieldsForHttpSend(header_fields);
}


void FunapiEncryption::SetHeaderFieldsForHttpRecv (HeaderFields &header_fields) {
  return impl_->SetHeaderFieldsForHttpRecv(header_fields);
}


bool FunapiEncryption::UseSodium() {
  return impl_->UseSodium();
}


bool FunapiEncryption::IsHandShakeCompleted(const EncryptionType type) {
  return impl_->IsHandShakeCompleted(type);
}


bool FunapiEncryption::IsHandShakeCompleted() {
  return impl_->IsHandShakeCompleted();
}


bool FunapiEncryption::HasEncryption(const EncryptionType type) {
  return impl_->HasEncryption(type);
}

}  // namespace fun
