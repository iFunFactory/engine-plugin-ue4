// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_UTILS_H_
#define SRC_FUNAPI_UTILS_H_

#include "funapi_plugin.h"
//#include <memory>
//#include <string>
//#include <vector>

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


// Format string
typedef std::string string;

template <typename ... Args>
string FormatString (const char* fmt, Args... args)
{
  size_t length = snprintf(nullptr, 0, fmt, args...) + 1;
  std::unique_ptr<char[]> buf(new char[length]);
  snprintf(buf.get(), length, fmt, args...);
  return string(buf.get(), buf.get() + length - 1);
}


// Function event
template <typename T>
class FunapiEvent
{
 public:
  // void operator+= (const T &handler) { std::unique_lock<std::mutex> lock(mutex_); vector_.push_back(handler); }
  void operator+= (const T &handler) { vector_.push_back(handler); }
  template <typename... ARGS>
  // void operator() (const ARGS&... args) { std::unique_lock<std::mutex> lock(mutex_); for (const auto &f : vector_) f(args...); }
  void operator() (const ARGS&... args) { for (const auto &f : vector_) f(args...); }
  bool empty() { return vector_.empty(); }

 private:
  std::vector<T> vector_;
//  std::mutex mutex_;
};


class FunapiTimer
{
 public:
  FunapiTimer(time_t seconds = 0) {
    SetTimer(seconds);
  }

  bool IsExpired() const {
    // UE_LOG(LogClass, Warning, TEXT("IsExpired - return false"));
    // return false; 
    /*
    {
      time_t now = time(NULL);
      UE_LOG(LogClass, Warning, TEXT("now = %ld, time= %ld"), now, time_);
    }
    */

    // if (time_miliseconds_ == 0)
    if (time_ == 0)
      return false;

    // if ((std::chrono::system_clock::now().time_since_epoch().count()/1000) > time_miliseconds_)
    if (time(NULL) > time_)
      return true;

    return false;
  };

  void SetTimer(const time_t seconds) {
    // time_miliseconds_ = static_cast<int64_t>((std::chrono::system_clock::now().time_since_epoch().count()/1000) + (seconds*1000));
    time_ = time(NULL) + seconds;
  };

 private:
  // int64_t time_miliseconds_ = 0;
   time_t time_ = 0;
};


class DebugUtils
{
 public:
   static void Log(std::string fmt, ...) {
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
    UE_LOG(LogClass, Warning, TEXT("%s"), *FString(buffer));
#endif
  };

  /*
  template <>
  static void Log(const char* fmt) {
#ifdef FUNAPI_COCOS2D
//    std::string temp_string = fun::FormatString(fmt, args...);
//    CCLOG("%s", temp_string.c_str());
#endif

#ifdef FUNAPI_UE4
    // std::string temp_string = fun::FormatString(fmt, args...);
    UE_LOG(LogClass, Warning, TEXT("%s"), *FString(fmt));
#endif
  };
  */
};

}  // namespace fun

#endif  // SRC_FUNAPI_UTILS_H_
