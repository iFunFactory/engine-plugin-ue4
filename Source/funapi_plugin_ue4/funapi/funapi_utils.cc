// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_utils.h"
#include "md5/md5.h"

namespace fun {

std::string FunapiUtil::MD5String(const std::string &file_name, bool use_front) {
  const size_t read_buffer_size = 1048576; // 1024*1024
  const size_t md5_buffer_size = 16;
  std::vector<unsigned char> buffer(read_buffer_size);
  std::vector<unsigned char> md5(md5_buffer_size);
  std::string ret;
  size_t length;

  FILE *fp = fopen(file_name.c_str(), "rb");
  if (!fp) {
    return std::string("");
  }

  MD5_CTX ctx;
  MD5_Init(&ctx);
  if (use_front) {
    length = fread(buffer.data(), 1, read_buffer_size, fp);
    MD5_Update(&ctx, buffer.data(), length);
  }
  else {
    while ((length = fread(buffer.data(), 1, read_buffer_size, fp)) != 0) {
      MD5_Update(&ctx, buffer.data(), length);
    }
  }
  MD5_Final(md5.data(), &ctx);
  fclose(fp);

  char c[3];
  for (int i = 0; i<md5_buffer_size; ++i) {
    sprintf(c, "%02x", md5[i]);
    ret.append(c);
  }

  return ret;
}

}  // namespace fun
