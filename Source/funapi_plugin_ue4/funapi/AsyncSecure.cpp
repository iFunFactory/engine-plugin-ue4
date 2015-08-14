// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin_ue4.h"
#include "AsyncSecure.h"
#include "SecureHash.h"

#include <assert.h>
#include <stdio.h>
#include <openssl/md5.h>


namespace
{
    const int kBlockSize = 1024 * 1024; // 1Mb
    const int kMaxSleepCount = 5;
}

namespace Fun
{
    void AsyncSecure::Get(string path, OnResult callback)
    {
        //AsyncWorker.AddRequest(new Md5Request(path, callback));
        //AsyncWorker.Begin(&Calc);
    }

    bool AsyncSecure::Calc (const AsyncRequest* request)
    {
        const Md5Request* r = (const Md5Request*)request;

        FILE* fp;
        if (fopen_s(&fp, r->path.c_str(), "rb") != 0)
        {
            LOG1("AsyncSecure - Can't find '%s' file.", r->path.c_str());
            return true;
        }

        assert(fp != NULL);
        fseek(fp, 0, SEEK_END);
        int length = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        MD5_CTX lctx;
        uint8_t *buffer = new uint8_t[kBlockSize];
        int read_len = 0, offset = 0;

        MD5_Init(&lctx);

        while (offset < length)
        {
            if (offset + kBlockSize > length)
                read_len = length - offset;
            else
                read_len = kBlockSize;

            fread(buffer, sizeof(uint8_t), read_len, fp);

            MD5_Update(&lctx, buffer, read_len);
            offset += read_len;
#if PLATFORM_WINDOWS
            Sleep(10);
#else
            nanosleep(10000000L);
#endif
        }

        fclose(fp);

        char md5hash[MD5_DIGEST_LENGTH * 2 + 1];
        unsigned char digest[MD5_DIGEST_LENGTH];

        MD5_Final(digest, &lctx);

        for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
            sprintf_s(md5hash + (i * 2), 2, "%02x", digest[i]);
        }
        md5hash[MD5_DIGEST_LENGTH * 2] = '\0';

        //r->callback(md5hash);
        delete r;

        return true;
    }

} // namespace Fun
