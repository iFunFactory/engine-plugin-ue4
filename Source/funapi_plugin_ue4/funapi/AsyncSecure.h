// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#pragma once

#include "AsyncThread.h"


namespace Fun
{
    struct DownloadFileInfo
    {
        string path;
        uint64_t size;
        string hash;
        string hash_front;
    };

    class AsyncSecure
    {
    private:
        typedef void(*OnResult)(string, DownloadFileInfo*, bool);

    public:
        static void Get(string Path, OnResult OnFinished);

    private:
        class Md5Request : public AsyncRequest
        {
        public:
            Md5Request(string path, OnResult callback)
                : AsyncRequest(typeid(this).name())
            {
                this->path = path;
                this->callback = callback;
            }

        public:
            string path;
            OnResult callback;
        };

        static bool Calc(const AsyncRequest* request);
    };

} // namespace Fun
