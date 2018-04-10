// Copyright (C) 2013-2018 iFunFactory Inc. All Rights Reserved.
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

#include "funapi_session.h"

namespace fun {

template <typename T> class FunapiEvent
{
 public:
  void operator+= (const T &handler) {
    std::unique_lock<std::mutex> lock(mutex_);
    vector_.push_back(handler);
  }

  template <typename... ARGS>
  void operator() (const ARGS&... args) {
    std::unique_lock<std::mutex> lock(mutex_);
    for (const auto &f : vector_) f(args...);
  }

  bool empty() {
    std::unique_lock<std::mutex> lock(mutex_);
    return vector_.empty();
  }

 private:
  std::vector<T> vector_;
  std::mutex mutex_;
};


class FunapiTimer
{
 public:
  FunapiTimer(time_t seconds = 0);
  bool IsExpired() const;
  void SetTimer(const time_t seconds);

 private:
  time_t time_ = 0;
};


class DebugUtils
{
 public:
  static void Log(const char* fmt, ...);
};


class FunapiUtil
{
 public:
  static bool SeqLess(const uint32_t x, const uint32_t y);
  static bool IsFileExists(const std::string &file_name);
  static long GetFileSize(const std::string &file_name);

  static std::string MD5String(const std::string &file_name, bool use_front);

  static std::string StringFromBytes(const std::string &uuid_str);
  static std::string BytesFromString(const std::string &uuid);

  static bool DecodeBase64(const std::string &in, std::vector<uint8_t> &out);

  static int GetSocketErrorCode();
  static std::string GetSocketErrorString(const int code);
};


class FunapiInitImpl;
class FunapiInit : public std::enable_shared_from_this<FunapiInit> {
 public:
  typedef std::function<void()> InitHandler;
  typedef std::function<void()> DestroyHandler;

  FunapiInit() = delete;
  FunapiInit(const InitHandler &init_handler,
             const DestroyHandler &destroy_handler);
  virtual ~FunapiInit();

  static std::shared_ptr<FunapiInit> Create(const InitHandler &init_handler,
                                            const DestroyHandler &destroy_handler = [](){});

 private:
  std::shared_ptr<FunapiInitImpl> impl_;
};

}  // namespace fun


#endif  // SRC_FUNAPI_UTILS_H_
