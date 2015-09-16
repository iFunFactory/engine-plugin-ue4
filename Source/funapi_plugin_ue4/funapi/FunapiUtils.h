// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#pragma once

#if PLATFORM_WINDOWS

#pragma warning(disable:4996)

namespace fun
{
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
} // namespace fun

#endif // PLATFORM_WINDOWS
