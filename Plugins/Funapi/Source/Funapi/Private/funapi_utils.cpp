// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifdef FUNAPI_UE4
#include "FunapiPrivatePCH.h"
#else
#include "funapi_build_config.h"
#endif

#include "funapi_utils.h"
#include "funapi_plugin.h"

#ifdef FUNAPI_COCOS2D
#include "md5/md5.h"
#endif

#ifdef FUNAPI_UE4
#include "SecureHash.h"
#endif

namespace fun {

#ifdef FUNAPI_COCOS2D
std::string FunapiUtil::MD5String(const std::string &file_name, bool use_front) {
  const size_t read_buffer_size = 1048576; // 1024*1024
  const size_t md5_buffer_size = 16;
  std::vector<unsigned char> buffer(read_buffer_size);
  std::vector<unsigned char> md5(md5_buffer_size);
  size_t length;

  FILE *fp = fopen(file_name.c_str(), "rb");
  if (!fp) {
    return std::string("");
  }

  md5_state_t ctx;
  md5_init(&ctx);
  if (use_front) {
    length = fread(buffer.data(), 1, read_buffer_size, fp);
    md5_append(&ctx, buffer.data(), static_cast<int>(length));
  }
  else {
    while ((length = fread(buffer.data(), 1, read_buffer_size, fp)) != 0) {
      md5_append(&ctx, buffer.data(), static_cast<int>(length));
    }
  }
  md5_finish(&ctx, md5.data());
  fclose(fp);

  std::string ret(md5_buffer_size*2, 0);
  char* c = const_cast<char*>(ret.data());
  for (int i = 0; i<md5_buffer_size; ++i, c+=2) {
    sprintf(c, "%02x", md5[i]);
  }

  return ret;
}
#endif // FUNAPI_COCOS2D


#ifdef FUNAPI_UE4
std::string FunapiUtil::MD5String(const std::string &file_name, bool use_front) {
  const size_t read_buffer_size = 1048576; // 1024*1024
  const size_t md5_buffer_size = 16;
  std::vector<unsigned char> buffer(read_buffer_size);
  std::vector<unsigned char> md5_buffer(read_buffer_size);
  size_t length;
  FMD5 md5;

  FILE *fp = fopen(file_name.c_str(), "rb");
  if (!fp) {
    return std::string("");
  }

  if (use_front) {
    length = fread(buffer.data(), 1, read_buffer_size, fp);
    md5.Update(buffer.data(), length);
  }
  else {
    while ((length = fread(buffer.data(), 1, read_buffer_size, fp)) != 0) {
      md5.Update(buffer.data(), length);
    }
  }

  md5.Final(md5_buffer.data());

  fclose(fp);

  std::string ret(md5_buffer_size*2, 0);
  char* c = const_cast<char*>(ret.data());
  for (int i = 0; i<md5_buffer_size; ++i, c+=2) {
    sprintf(c, "%02x", md5_buffer[i]);
  }

  return ret;
}
#endif // FUNAPI_UE4


std::string FunapiUtil::StringFromBytes(const std::string &uuid_str) {
  if (uuid_str.length() >= 24) {
    if (uuid_str[8] == '-' &&
        uuid_str[13] == '-' &&
        uuid_str[18] == '-' &&
        uuid_str[23] == '-') {
      return uuid_str;
    }
  }

  std::stringstream ss;
  ss << std::hex << std::right;
  ss.fill(ss.widen('0'));

  const size_t length = uuid_str.length();
  uint8_t *data = reinterpret_cast<uint8_t*>(const_cast<char*>(uuid_str.c_str()));

  for (size_t i = 0; i < length; ++i) {
    ss.width(2);
    ss << static_cast<uint16_t>(data[i]);
    if (i == 3 || i == 5 || i == 7 || i == 9) {
      ss << ss.widen('-');
    }
  }

  return ss.str();
}


std::string FunapiUtil::BytesFromString(const std::string &uuid) {
  std::string uuid_str;
  uuid_str.resize(16);
  uint8_t *dest = reinterpret_cast<uint8_t*>(const_cast<char*>(uuid_str.c_str()));

  const char *src = uuid.c_str();
  char temp_buffer[3];
  temp_buffer[2] = 0;
  char *ptr;

  for (int i=0;i<16;++i) {
    memcpy (temp_buffer, src, 2);
    dest[i] = strtoul(temp_buffer, &ptr, 16);

    if (i == 3 || i == 5 || i == 7 || i == 9) {
      src += 3;
    }
    else {
      src += 2;
    }
  }

  return uuid_str;
}

}  // namespace fun
