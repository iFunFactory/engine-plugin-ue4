// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#pragma once

#include <list>


namespace Fun
{
    class AsyncRequest
    {
    public:
        AsyncRequest(string id) { this->id = id; }

    public:
        string id;
    };


    class AsyncThread
    {
    private:
        typedef bool(*FOnProcess)(const AsyncRequest*);
        typedef std::list<AsyncRequest*> RequestQueue;

        AsyncThread();

    public:
        void Begin(FOnProcess func);
        void Stop();

        void Lock();
        void Unlock();

        void AddRequest(AsyncRequest* r);

    private:
#if PLATFORM_WINDOWS
        static uint32_t WINAPI ThreadProc(AsyncThread* pThis);
        typedef uint32_t (__stdcall *ProcType)(void*);
#else
        static void* ThreadProc(AsyncThread* pThis);
        typedef void* (__stdcall *ProcType)(void*);
#endif
        void DoWork();

    private:
        bool bRun;

#if PLATFORM_WINDOWS
        HANDLE hThread;
        HANDLE hMutex;
#else
        pthread_t hThread;
        pthread_mutex_t hMutex;
        pthread_cond_t hCond;
#endif
        RequestQueue queue;
        FOnProcess OnProcess;
    };

} // namespace Fun
