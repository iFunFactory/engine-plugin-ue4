// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#pragma once

#include <memory>
#include <string>

#if PLATFORM_WINDOWS
#include <stdint.h>

#pragma warning(disable:4996)
#endif

namespace fun {

#if PLATFORM_WINDOWS

struct IOVec
{
    uint8_t* iov_base;
    size_t iov_len;
};

#define iovec     IOVec
#define ssize_t   size_t

#define access    _access
#define snprintf  _snprintf

#define F_OK      0

#define mkdir(path, mode)    _mkdir(path)

#endif // PLATFORM_WINDOWS


// Format string
typedef std::string string;

template <typename ... Args>
string str_fmt(const char* fmt, Args... args)
{
  size_t length = snprintf(nullptr, 0, fmt, args...) + 1;
  std::unique_ptr<char[]> buf(new char[length]);
  snprintf(buf.get(), length, fmt, args...);
  return string(buf.get(), buf.get() + length - 1);
}

} // namespace fun
