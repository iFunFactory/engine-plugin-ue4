// Copyright (C) 2013-2018 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_COMPRESSION_H_
#define SRC_FUNAPI_COMPRESSION_H_

#include "funapi_plugin.h"

#define kProtocolCompressionField "C"

namespace fun {

enum class FUNAPI_API CompressionType : int {
#if FUNAPI_HAVE_ZSTD
  kZstd,     // Zstandard
#endif
#if FUNAPI_HAVE_ZLIB
  kDeflate,  // DEFLATE
#endif
  kDefault,
  kNone,
};


class FunapiCompressionImpl;
class FUNAPI_API FunapiCompression : public std::enable_shared_from_this<FunapiCompression> {
 public:
  typedef std::map<std::string, std::string> HeaderFields;

  FunapiCompression();
  virtual ~FunapiCompression();

  void SetCompressionType(const CompressionType type);
#if FUNAPI_HAVE_ZSTD
  void SetZstdDictBase64String(const std::string &zstd_dict_base64string);
#endif

  bool Compress(HeaderFields &header_fields, std::vector<uint8_t> &body);
  bool Decompress(HeaderFields &header_fields, std::vector<uint8_t> &body);

  void SetHeaderFieldsForHttpSend (HeaderFields &header_fields);
  void SetHeaderFieldsForHttpRecv (HeaderFields &header_fields);

  bool HasCompression(const CompressionType type);

 private:
  std::shared_ptr<FunapiCompressionImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_COMPRESSION_H_
