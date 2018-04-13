// Copyright (C) 2013-2018 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifdef FUNAPI_UE4
#include "FunapiPrivatePCH.h"
#endif

#include "funapi_compression.h"
#include "funapi_utils.h"
#include "funapi_session.h"

#if FUNAPI_HAVE_ZLIB
extern "C" {
#include <zlib.h>
}
#endif

#if FUNAPI_HAVE_ZSTD
// zstandard
#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>
#endif

#define kProtocolHttpCompressionField "X-iFun-C" // http header

namespace fun {

//////////////////////////////////////////////////////////////////////////////
// Compressor implementation.
class Compressor : public std::enable_shared_from_this<Compressor>
{
 public:
  Compressor() = default;
  virtual ~Compressor() = default;

  static std::shared_ptr<Compressor> Create(CompressionType type);

  virtual CompressionType GetCompressionType() = 0;

  virtual bool Compress(const std::vector<uint8_t> &in, std::vector<uint8_t> &out) = 0;
  virtual bool Decompress(const std::vector<uint8_t> &in, std::vector<uint8_t> &out) = 0;
};


#define ZLIB_WS (-15)


////////////////////////////////////////////////////////////////////////////////
// CompressorZlib implementation.
class CompressorZlib : public Compressor
{
 public:
  CompressorZlib() = default;
  virtual ~CompressorZlib() = default;

  CompressionType GetCompressionType();

  bool Compress(const std::vector<uint8_t> &in, std::vector<uint8_t> &out);
  bool Decompress(const std::vector<uint8_t> &in, std::vector<uint8_t> &out);

 private:
  size_t GetMaximumLength(size_t in_size);
};


size_t CompressorZlib::GetMaximumLength(size_t in_size) {
  return 18 + in_size + 5 * (in_size / 16383 + 1);
}


CompressionType CompressorZlib::GetCompressionType()
{
#if FUNAPI_HAVE_ZLIB
  return CompressionType::kDeflate;
#else
  return CompressionType::kNone;
#endif
}


bool CompressorZlib::Compress(const std::vector<uint8_t> &in, std::vector<uint8_t> &out)
{
#if FUNAPI_HAVE_ZLIB
  const size_t in_size = in.size();
  const size_t max_size = GetMaximumLength(in_size);
  out.resize(max_size);

  z_stream zstr;
  zstr.zalloc = Z_NULL;
  zstr.zfree = Z_NULL;
  zstr.opaque = Z_NULL;

  zstr.next_in = const_cast<uint8_t*>(in.data());
  zstr.avail_in = static_cast<unsigned int>(in_size);
  zstr.next_out = out.data();
  zstr.avail_out = static_cast<unsigned int>(max_size);

  int result = ::deflateInit2(&zstr, 3, Z_DEFLATED, ZLIB_WS, 9, Z_DEFAULT_STRATEGY);
  if (result != Z_OK) {
    return false;
  }

  result = deflate(&zstr, Z_FINISH);
  if (result != Z_STREAM_END) {
    deflateEnd(&zstr);
    return false;
  }

  const size_t out_size = max_size - zstr.avail_out;
  if (out_size >= in_size) {
    return false;
  }

  out.resize(out_size);
  result = deflateEnd(&zstr);
  if (result != Z_OK) {
    return false;
  }
#endif

  return true;
}


bool CompressorZlib::Decompress(const std::vector<uint8_t> &in, std::vector<uint8_t> &out)
{
#if FUNAPI_HAVE_ZLIB
  z_stream zstr;
  zstr.zalloc = Z_NULL;
  zstr.zfree = Z_NULL;
  zstr.opaque = Z_NULL;

  zstr.next_in = const_cast<uint8_t*>(in.data());
  zstr.avail_in = static_cast<unsigned int>(in.size());
  zstr.next_out = out.data();
  zstr.avail_out = static_cast<unsigned int>(out.size());

  int result = ::inflateInit2(&zstr, ZLIB_WS);
  if (result != Z_OK) {
    DebugUtils::Log("Failed to initialize zlib stream.");
    return false;
  }

  result = inflate(&zstr, Z_FINISH);
  if (result != Z_STREAM_END) {
    ::inflateEnd(&zstr);
    DebugUtils::Log("Failed to compress (zlib).");
    return false;
  }

  result = ::inflateEnd(&zstr);
  if (result != Z_OK || zstr.avail_out != 0) {
    DebugUtils::Log("Failed to finish compression (zlib).");
    return false;
  }
#endif

  return true;
}


////////////////////////////////////////////////////////////////////////////////
// CompressorZstd implementation.
class CompressorZstd : public Compressor
{
 public:
  CompressorZstd() = default;
  virtual ~CompressorZstd();

  CompressionType GetCompressionType();

  bool Compress(const std::vector<uint8_t> &in, std::vector<uint8_t> &out);
  bool Decompress(const std::vector<uint8_t> &in, std::vector<uint8_t> &out);

  void SetDictBase64String(const std::string &zstd_dict_base64string);

 private:
#if FUNAPI_HAVE_ZSTD
  size_t DoCompressSmall(::ZSTD_CCtx *ctxt, const void *src, size_t src_size, void *dst, size_t dst_size);
  size_t DoCompress(::ZSTD_CCtx *ctxt, const void *src, size_t src_size, void *dst, size_t dst_size);
  size_t DoDecompressSmall(::ZSTD_DCtx *ctxt, void *dst, size_t dst_size, const void *src, size_t src_size);
  size_t DoDecompress(::ZSTD_DCtx *ctxt, void *dst, size_t dst_size, const void *src, size_t src_size);
#endif

  void Cleanup();

#if FUNAPI_HAVE_ZSTD
  ::ZSTD_CDict *cdict_ = NULL;
  ::ZSTD_DDict *ddict_ = NULL;
#endif
};


#if FUNAPI_HAVE_ZSTD
#define kZstdCompressionLevel (1)
#define kZstdMinBlock (1UL << ZSTD_BLOCKSIZELOG_MAX)
#define kZstdNoFrameParam {0, 0, 1}
#endif

CompressorZstd::~CompressorZstd() {
  Cleanup();
}


void CompressorZstd::Cleanup() {
#if FUNAPI_HAVE_ZSTD
  if (ddict_) {
    ::ZSTD_freeDDict(ddict_);
    ddict_ = NULL;
  }

  if (cdict_) {
    ::ZSTD_freeCDict(cdict_);
    cdict_ = NULL;
  }
#endif
}


#if FUNAPI_HAVE_ZSTD
size_t CompressorZstd::DoCompressSmall(::ZSTD_CCtx *ctxt, const void *src, size_t src_size, void *dst, size_t dst_size) {
  size_t rv = 0;

  if (cdict_) {
    ::ZSTD_compressBegin_usingCDict_advanced(ctxt, cdict_, kZstdNoFrameParam, src_size);
  }
  else {
    ::ZSTD_parameters param = {
      ::ZSTD_getCParams(kZstdCompressionLevel, src_size, 0),
      kZstdNoFrameParam
    };
    ::ZSTD_compressBegin_advanced(ctxt, NULL, 0, param, src_size);
  }
  rv = ::ZSTD_compressBlock(ctxt, dst, dst_size, src, src_size);
  if (::ZSTD_isError(rv)) {
    std::stringstream ss;
    ss << __func__ << ": " << ::ZSTD_getErrorName(rv);
    DebugUtils::Log("%s", ss.str().c_str());
    return 0;
  }

  return rv;
}
#endif


#if FUNAPI_HAVE_ZSTD
size_t CompressorZstd::DoCompress(::ZSTD_CCtx *ctxt, const void *src, size_t src_size, void *dst, size_t dst_size) {
  size_t rv =0;
  if (cdict_) {
    rv = ::ZSTD_compress_usingCDict(ctxt, dst, dst_size, src, src_size, cdict_);
  }
  else {
    rv = ::ZSTD_compressCCtx(ctxt, dst, dst_size, src, src_size, kZstdCompressionLevel);
  }

  if (::ZSTD_isError(rv)) {
    return 0;
  }

  return rv;
}
#endif


#if FUNAPI_HAVE_ZSTD
size_t CompressorZstd::DoDecompressSmall(::ZSTD_DCtx *ctxt, void *dst, size_t dst_size, const void *src, size_t src_size) {
  size_t rv = 0;
  if (ddict_) {
    ::ZSTD_decompressBegin_usingDDict(ctxt, ddict_);
  }
  else {
    ::ZSTD_decompressBegin(ctxt);
  }

  rv = ::ZSTD_decompressBlock(ctxt, dst, dst_size, src, src_size);
  if (::ZSTD_isError(rv)) {
    std::stringstream ss;
    ss << __func__ << ": " << ::ZSTD_getErrorName(rv);
    DebugUtils::Log("%s", ss.str().c_str());
    return 0;
  }

  return rv;
}
#endif


#if FUNAPI_HAVE_ZSTD
size_t CompressorZstd::DoDecompress(::ZSTD_DCtx *ctxt, void *dst, size_t dst_size, const void *src, size_t src_size) {
  size_t rv = 0;
  if (ddict_) {
    rv = ::ZSTD_decompress_usingDDict(ctxt, dst, dst_size, src, src_size, ddict_);
  }
  else {
    rv = ::ZSTD_decompressDCtx(ctxt, dst, dst_size, src, src_size);
  }

  if (::ZSTD_isError(rv)) {
    std::stringstream ss;
    ss << __func__ << ": " << ::ZSTD_getErrorName(rv);
    DebugUtils::Log("%s", ss.str().c_str());
    return 0;
  }

  return rv;
}
#endif


CompressionType CompressorZstd::GetCompressionType()
{
#if FUNAPI_HAVE_ZSTD
  return CompressionType::kZstd;
#else
  return CompressionType::kNone;
#endif
}


bool CompressorZstd::Compress(const std::vector<uint8_t> &in, std::vector<uint8_t> &out)
{
#if FUNAPI_HAVE_ZSTD
  ::ZSTD_CCtx *ctxt = ::ZSTD_createCCtx();
  if (!ctxt) {
    DebugUtils::Log("Cannot allocate ZSTD_CCtx.");
    return false;
  }

  const size_t in_size = in.size();
  const size_t max_size = ZSTD_compressBound(in_size);
  out.resize(max_size);
  size_t compressed_size = 0;

  if (in_size >= kZstdMinBlock || in_size == 0) {
    compressed_size = DoCompress(ctxt,
                                 in.data(),
                                 in_size,
                                 out.data(),
                                 max_size);
  } else {
    compressed_size = DoCompressSmall(ctxt,
                                      in.data(),
                                      in_size,
                                      out.data(),
                                      max_size);
  }
  ::ZSTD_freeCCtx(ctxt);

  if (in_size < compressed_size || compressed_size == 0) {
    return false;
  }

  out.resize(compressed_size);
#endif

  return true;
}


bool CompressorZstd::Decompress(const std::vector<uint8_t> &in, std::vector<uint8_t> &out)
{
#if FUNAPI_HAVE_ZSTD
  size_t expected_size = out.size();

  size_t out_size = 0;
  ::ZSTD_DCtx *ctxt = ::ZSTD_createDCtx();
  if (!ctxt) {
    DebugUtils::Log("Cannot allocate ZSTD_DCtx");
    return false;
  }

  if (expected_size == 0 || expected_size >= kZstdMinBlock) {
    out_size = DoDecompress(ctxt,
                            out.data(),
                            expected_size,
                            in.data(),
                            in.size());
  } else {
    out_size = DoDecompressSmall(ctxt,
                                 out.data(),
                                 expected_size,
                                 in.data(),
                                 in.size());
  }
  ::ZSTD_freeDCtx(ctxt);

  return (out.size() == expected_size);
#else
  return true;
#endif
}


void CompressorZstd::SetDictBase64String(const std::string &zstd_dict_base64string) {
#if FUNAPI_HAVE_ZSTD
  std::vector<uint8_t> dict_buf;
  if (false == FunapiUtil::DecodeBase64(zstd_dict_base64string, dict_buf)) {
    DebugUtils::Log("Cannot decode zstd_dict");
  }

  cdict_ = ::ZSTD_createCDict(dict_buf.data(), dict_buf.size(), kZstdCompressionLevel);
  ddict_ = ::ZSTD_createDDict(dict_buf.data(), dict_buf.size());

  if (cdict_ == NULL || ddict_ == NULL) {
    std::stringstream ss;
    ss << "Failed to load zstd dictionary object.";
    DebugUtils::Log("%s", ss.str().c_str());
  }
#endif
}


////////////////////////////////////////////////////////////////////////////////
// Compressor implementation.
// Factory

std::shared_ptr<Compressor> Compressor::Create(CompressionType type) {
#if FUNAPI_HAVE_ZLIB
  if (type == CompressionType::kDeflate)
    return std::make_shared<CompressorZlib>();
#endif

#if FUNAPI_HAVE_ZSTD
  if (type == CompressionType::kZstd)
    return std::make_shared<CompressorZstd>();
#endif

  return nullptr;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiCompressionImpl implementation.

class FunapiCompressionImpl : public std::enable_shared_from_this<FunapiCompressionImpl> {
 public:
  typedef FunapiCompression::HeaderFields HeaderFields;

  FunapiCompressionImpl();
  virtual ~FunapiCompressionImpl();

  void SetCompressionType(const CompressionType type);
  void SetZstdDictBase64String(const std::string &zstd_dict_base64string);

  bool Compress(HeaderFields &header_fields, std::vector<uint8_t> &body);
  bool Decompress(HeaderFields &header_fields, std::vector<uint8_t> &body);

  void SetHeaderFieldsForHttpSend (HeaderFields &header_fields);
  void SetHeaderFieldsForHttpRecv (HeaderFields &header_fields);

  bool HasCompression(const CompressionType type);

 private:
  bool CreateCompressor(const CompressionType type);
  std::shared_ptr<Compressor> GetCompressor(CompressionType type);

  std::map<CompressionType, std::shared_ptr<Compressor>> compressors_;
  std::shared_ptr<Compressor> default_compressor_ = nullptr;
};


FunapiCompressionImpl::FunapiCompressionImpl() {
}


FunapiCompressionImpl::~FunapiCompressionImpl() {
}


std::shared_ptr<Compressor> FunapiCompressionImpl::GetCompressor(CompressionType type) {
  if (type == CompressionType::kDefault) {
    return default_compressor_;
  }

  auto iter = compressors_.find(type);
  if (iter != compressors_.cend())
    return iter->second;

  return nullptr;
}


bool FunapiCompressionImpl::CreateCompressor(const CompressionType type) {
  if (GetCompressor(type)) {
    return false;
  }

  std::shared_ptr<Compressor> c = Compressor::Create(type);
  if (c) {
    compressors_[type] = c;
    default_compressor_ = c;
  }

  return true;
}


bool FunapiCompressionImpl::Decompress(HeaderFields &header_fields,
                                       std::vector<uint8_t> &body) {
  if (default_compressor_) {
    HeaderFields::iterator it;
    it = header_fields.find(kProtocolCompressionField);
    if (it != header_fields.end()) {
      size_t body_length = atoi(it->second.c_str());
      if (body_length > 0) {
        std::vector<uint8_t> in(body.cbegin(), body.cend());
        body.resize(body_length);
        return default_compressor_->Decompress(in, body);
      }
    }
  }

  return true;
}


bool FunapiCompressionImpl::Compress(HeaderFields &header_fields, std::vector<uint8_t> &body) {
  if (default_compressor_ && !body.empty()) {
    std::vector<uint8_t> in(body.cbegin(), body.cend());
    if (default_compressor_->Compress(in, body)) {
      HeaderFields::iterator it = header_fields.find(kLengthHeaderField);
      if (it != header_fields.end()) {
        header_fields[kProtocolCompressionField] = it->second.c_str();
      }

      std::stringstream ss;
      ss << body.size();
      header_fields[kLengthHeaderField] = ss.str();
    }
    else {
      body.assign(in.begin(), in.end());
      return false;
    }
  }

  return true;
}


void FunapiCompressionImpl::SetHeaderFieldsForHttpSend (HeaderFields &header_fields) {
  HeaderFields::const_iterator it = header_fields.find(kProtocolCompressionField);
  if (it != header_fields.end()) {
    header_fields[kProtocolHttpCompressionField] = it->second;
  }
}


void FunapiCompressionImpl::SetHeaderFieldsForHttpRecv (HeaderFields &header_fields) {
  HeaderFields::const_iterator it = header_fields.find(kProtocolHttpCompressionField);
  if (it != header_fields.end()) {
    header_fields[kProtocolCompressionField] = it->second;
  }
}


void FunapiCompressionImpl::SetCompressionType(const CompressionType type) {
  std::shared_ptr<Compressor> c = Compressor::Create(type);
  if (c) {
    compressors_[type] = c;
    default_compressor_ = c;
  }
}


bool FunapiCompressionImpl::HasCompression(const CompressionType type) {
  if (GetCompressor(type)) {
    return true;
  }

  return false;
}


void FunapiCompressionImpl::SetZstdDictBase64String(const std::string &zstd_dict_base64string) {
#if FUNAPI_HAVE_ZSTD
  if (zstd_dict_base64string.length() > 0) {
    if (false == HasCompression(CompressionType::kZstd)) {
      SetCompressionType(CompressionType::kZstd);
    }

    std::shared_ptr<Compressor> c = GetCompressor(CompressionType::kZstd);
    if (c) {
      std::shared_ptr<CompressorZstd> zstd_compressor = std::static_pointer_cast<CompressorZstd>(c);
      zstd_compressor->SetDictBase64String(zstd_dict_base64string);
    }
  }
#endif
}


////////////////////////////////////////////////////////////////////////////////
// FunapiCompression implementation.

FunapiCompression::FunapiCompression()
  : impl_(std::make_shared<FunapiCompressionImpl>()) {
}


FunapiCompression::~FunapiCompression() {
}


void FunapiCompression::SetCompressionType(const CompressionType type) {
  return impl_->SetCompressionType(type);
}


#if FUNAPI_HAVE_ZSTD
void FunapiCompression::SetZstdDictBase64String(const std::string &zstd_dict_base64string) {
  return impl_->SetZstdDictBase64String(zstd_dict_base64string);
}
#endif


bool FunapiCompression::Compress(HeaderFields &header_fields, std::vector<uint8_t> &body) {
  return impl_->Compress(header_fields, body);
}


bool FunapiCompression::Decompress(HeaderFields &header_fields, std::vector<uint8_t> &body) {
  return impl_->Decompress(header_fields, body);
}


void FunapiCompression::SetHeaderFieldsForHttpSend (HeaderFields &header_fields) {
  return impl_->SetHeaderFieldsForHttpSend(header_fields);
}


void FunapiCompression::SetHeaderFieldsForHttpRecv (HeaderFields &header_fields) {
  return impl_->SetHeaderFieldsForHttpRecv(header_fields);
}


bool FunapiCompression::HasCompression(const CompressionType type) {
  return impl_->HasCompression(type);
}

}  // namespace fun
