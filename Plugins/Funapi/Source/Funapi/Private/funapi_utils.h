// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_UTILS_H_
#define SRC_FUNAPI_UTILS_H_

#include "funapi_plugin.h"

#ifdef FUNAPI_PLATFORM_WINDOWS
#include <stdint.h>
#pragma warning(disable:4996)
#endif

namespace fun {

#ifdef FUNAPI_PLATFORM_WINDOWS
#define ssize_t   size_t
#define access    _access
#define snprintf  _snprintf
#define F_OK      0
#define mkdir(path, mode)   _mkdir(path)
#endif

// Function event
template <typename T>
class FunapiEvent
{
 public:
  void operator+= (const T &handler) { std::unique_lock<std::mutex> lock(mutex_); vector_.push_back(handler); }
  template <typename... ARGS>
  void operator() (const ARGS&... args) { std::unique_lock<std::mutex> lock(mutex_); for (const auto &f : vector_) f(args...); }
  bool empty() { return vector_.empty(); }

 private:
  std::vector<T> vector_;
  std::mutex mutex_;
};


class FunapiTimer
{
 public:
  FunapiTimer(time_t seconds = 0) {
    SetTimer(seconds);
  }

  bool IsExpired() const {
    if (time_ == 0)
      return false;

    if (time(NULL) > time_)
      return true;

    return false;
  };

  void SetTimer(const time_t seconds) {
    time_ = time(NULL) + seconds;
  };

 private:
  time_t time_ = 0;
};


class DebugUtils
{
 public:
   static void Log(std::string fmt, ...) {
#ifdef DEBUG_LOG
    const int MAX_LENGTH = 2048;

    va_list args;
    char buffer[MAX_LENGTH];

    va_start(args, fmt);
    vsnprintf(buffer, MAX_LENGTH, fmt.c_str(), args);
    va_end(args);

#ifdef FUNAPI_COCOS2D
    CCLOG("%s", buffer);
#endif

#ifdef FUNAPI_UE4
    UE_LOG(LogFunapi, Log, TEXT("%s"), *FString(buffer));
#endif

#endif // DEBUG_LOG
  };
};


class FunapiUtil
{
 public:
  static bool SeqLess(const uint32_t x, const uint32_t y) {
    // 아래 참고
    //  - http://en.wikipedia.org/wiki/Serial_number_arithmetic
    //  - RFC 1982
    return (int32_t)(y - x) > 0;
  }

  static bool IsFileExists(const std::string &file_name) {
#ifdef FUNAPI_COCOS2D
    return cocos2d::FileUtils::getInstance()->isFileExist(file_name.c_str());
#endif

#ifdef FUNAPI_UE4
    return FPlatformFileManager::Get().GetPlatformFile().FileExists(ANSI_TO_TCHAR(file_name.c_str()));
#endif

    return false;
  };

  static long GetFileSize(const std::string &file_name) {
#ifdef FUNAPI_COCOS2D
    return cocos2d::FileUtils::getInstance()->getFileSize(file_name.c_str());
#endif

#ifdef FUNAPI_UE4
    return FPlatformFileManager::Get().GetPlatformFile().FileSize(ANSI_TO_TCHAR(file_name.c_str()));
#endif

    return 0;
  };

  static std::string MD5String(const std::string &file_name, bool use_front);

  static std::string StringFromBytes(const std::string &uuid_str);
  static std::string BytesFromString(const std::string &uuid);
};

}  // namespace fun


#endif  // SRC_FUNAPI_UTILS_H_
