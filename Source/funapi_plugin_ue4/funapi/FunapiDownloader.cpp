// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin_ue4.h"
#include "FunapiDownloader.h"
#include "AsyncThread.h"

#include <assert.h>
#include <io.h>
#include <openssl/md5.h>
#include "./funapi_utils.h"

using namespace Fun;


namespace
{
    struct DownloadFileInfo
    {
        string path;
        uint64_t size;
        string hash;
        string hash_front;
    };

    const int kBlockSize = 1024 * 1024; // 1MB
    const int kMaxSleepCount = 5;

    typedef void(*OnResult)(const string&, const DownloadFileInfo*, bool);


    struct Md5Request : public AsyncRequest
    {
    public:
        Md5Request(const string& path, const DownloadFileInfo* file, OnResult callback)
            : AsyncRequest(typeid(this).name())
        {
            this->path = path;
            this->file = file;
            this->callback = callback;
        }

    public:
        string path;
        const DownloadFileInfo* file;
        OnResult callback;
    };


    string MakeHashString (const unsigned char* digest)
    {
        char md5hash[MD5_DIGEST_LENGTH * 2 + 1];
        for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
            sprintf(md5hash + (i * 2), "%02x", digest[i]);
        }
        md5hash[MD5_DIGEST_LENGTH * 2] = '\0';

        return string(md5hash);
    }

    bool Calc (const AsyncRequest* request)
    {
        const Md5Request* req = (const Md5Request*)request;

        FILE* fp;
        if (fopen_s(&fp, req->path.c_str(), "rb") != 0)
        {
            LOG1("MD5Async - Can't find '%s' file.", req->path.c_str());
            return true;
        }

        assert(fp != NULL);
        fseek(fp, 0, SEEK_END);
        int length = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        MD5_CTX lctx;
        const DownloadFileInfo* file = req->file;

        uint8_t *buffer = new uint8_t[kBlockSize];
        unsigned char digest[MD5_DIGEST_LENGTH];
        int read_len = 0, offset = 0;
        bool check_front = true;

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

            if (check_front && file->hash_front.length() > 0)
            {
                check_front = false;
                MD5_Final(digest, &lctx);

                string md5hash = MakeHashString(digest);
                if (md5hash != file->hash_front || offset == length)
                {
                    fclose(fp);

                    req->callback(req->path, file,
                        file->hash_front == md5hash && file->hash == md5hash);

                    delete request;
                    return true;
                }

                fseek(fp, 0, SEEK_SET);
                offset = 0;
                MD5_Init(&lctx);
            }

#if PLATFORM_WINDOWS
            Sleep(10);
#else
            nanosleep(10000000L);
#endif
        }

        fclose(fp);
        MD5_Final(digest, &lctx);

        string md5hash = MakeHashString(digest);
        req->callback(req->path, file, file->hash == md5hash);

        delete request;
        return true;
    }

    void OnMd5Result (const string& path, const DownloadFileInfo* file, bool match)
    {
        LOG2("%s >> %d", *FString(file->path.c_str()), match);
    }


    // FOR TEST ////////////////////////////////////////////////////
    DownloadFileInfo file;

} // unnamed namespace


namespace Fun
{
    FunapiDownloader::FunapiDownloader()
    {
        // FOR TEST ////////////////////////////////////////////////////
        Thread = new AsyncThread();
        Thread->Begin(&Calc);

        string path = "C:\\Users\\arin\\work\\funapi-plugin-unity-4.6\\funapi-plugin-unity\\client_data\\images\\pizza.jpg";

        if (access(path.c_str(), F_OK) == -1)
        {
            LOG1("MD5Async.Compute - Can't find a file.\npath: %s", path.c_str());
            OnMd5Result(path, &file, false);
            return;
        }

        file.path = "images/pizza.jpg";
        file.hash = "9ab9626a301c2bf1444893a69a1bfac7";
        file.hash_front = "8fdd96188cadababf2fbcb812b43e6fb";

        Thread->AddRequest(new Md5Request(path, &file, OnMd5Result));
        // FOR TEST ////////////////////////////////////////////////////
    }

    FunapiDownloader::~FunapiDownloader()
    {
        // FOR TEST ////////////////////////////////////////////////////
        Thread->Stop();
        delete Thread;
        // FOR TEST ////////////////////////////////////////////////////
    }

} // namespace Fun
