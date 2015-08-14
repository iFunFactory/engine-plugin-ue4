// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin_ue4.h"

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#include <process.h>
#else
#include <pthread.h>
#endif
#include <assert.h>
#include "curl/curl.h"
#include "funapi_utils.h"
#include "AsyncThread.h"

#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif


namespace Fun
{
    AsyncThread::AsyncThread ()
        : bRun(false)
    {
    }

    void AsyncThread::Begin (FOnProcess func)
    {
        bRun = true;
        OnProcess = func;

#if PLATFORM_WINDOWS
        pthread_mutex_init(&hMutex, NULL);
        hThread = (HANDLE)_beginthreadex(NULL, 0, (ProcType)&ThreadProc, (void*)this, 0, NULL);
        assert(hThread != 0);
#else
        hMutex = PTHREAD_MUTEX_INITIALIZER;
        hCond = PTHREAD_COND_INITIALIZER;
        int r = pthread_create(&hThread, NULL, (ProcType)&ThreadProc, (void*)this);
        assert(r == 0);
#endif
    }

    void AsyncThread::Stop ()
    {
        bRun = false;
#if PLATFORM_WINDOWS
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        WaitForSingleObject(hMutex, INFINITE);
        CloseHandle(hMutex);
#else
        pthread_cond_broadcast(&hCond);
        void *dummy;
        pthread_join(hThread, &dummy);
        (void)dummy;
#endif
    }

    void AsyncThread::Lock ()
    {
        pthread_mutex_lock(&hMutex);
    }

    void AsyncThread::Unlock ()
    {
        pthread_mutex_unlock(&hMutex);
    }

    void AsyncThread::AddRequest(AsyncRequest* r)
    {
        pthread_mutex_lock(&hMutex);
        queue.push_back(r);
        pthread_cond_signal(&hCond);
        pthread_mutex_unlock(&hMutex);
    }

#if PLATFORM_WINDOWS
    uint32_t WINAPI AsyncThread::ThreadProc (AsyncThread* pThis) {
#else
    void* AsyncThread::ThreadProc (AsyncThread* pThis) {
#endif
        pThis->DoWork();
        return NULL;
    }

    void AsyncThread::DoWork ()
    {
        LOG("Thread for async operation has been created.");

        while (bRun)
        {
            // Waits until we have something to process.
            pthread_mutex_lock(&hMutex);
            while (bRun && queue.empty())
            {
#if PLATFORM_WINDOWS
                ReleaseMutex(hMutex);
                Sleep(1000);
                WaitForSingleObject(hMutex, INFINITE);
#else
                pthread_cond_wait(&async_queue_cond, &hMutex);
#endif
            }

            // Moves element to a worker queue and releaes the mutex
            // for contention prevention.
            RequestQueue WorkQueue;
            WorkQueue.swap(queue);
            pthread_mutex_unlock(&hMutex);

            for (RequestQueue::iterator itr = WorkQueue.begin(); itr != WorkQueue.end();)
            {
                if (OnProcess(*itr))
                {
                    // It returns true if the performing is completed
                    itr = WorkQueue.erase(itr);
                }
                else
                {
                    ++itr;
                }
            }

            // Puts back requests that requires more work.
            // We should respect the order.
            pthread_mutex_lock(&hMutex);
            queue.splice(queue.begin(), WorkQueue);
            pthread_mutex_unlock(&hMutex);
        }

        LOG("Thread for async operation is terminating.");
    }

} // namespace Fun
