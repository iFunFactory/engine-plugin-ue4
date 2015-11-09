// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin_ue4.h"
#include "funapi_thread.h"


namespace fun {

AsyncThread::AsyncThread ()
  : running_(false) {
}

void AsyncThread::Create () {
  // Creates a thread to handle async operations.
  running_ = true;
  thread_ = std::thread(&ThreadProc, this);
}

void AsyncThread::Destroy () {
  // Terminates the thread for async operations.
  running_ = false;
  cond_.notify_all();

  if (thread_.joinable())
    thread_.join();
}

void AsyncThread::AddRequest (AsyncRequest* request) {
  std::unique_lock<std::mutex> lock(mutex_);
  queue_.push_back(request);
  cond_.notify_one();
}

int AsyncThread::ThreadProc (AsyncThread* pThis) {
  pThis->DoWork();
  return NULL;
}

void AsyncThread::DoWork () {
  Log("Thread for async operation has been created.");

  while (running_) {
    // Waits until we have something to process.
    std::list<AsyncRequest*> work_queue;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      while (running_ && queue_.empty()) {
        cond_.wait(mutex_);
      }

      // Moves element to a worker queue and releaes the mutex
      // for contention prevention.
      work_queue.swap(queue_);
    }

    for (auto it = work_queue.begin(); it != work_queue.end();) {
      AsyncRequest* r = *it;
      if (r->proc(r)) {
        delete r;
        it = work_queue.erase(it);
      }
      else {
        ++it;
      }
    }

    // Puts back requests that requires more work.
    // We should respect the order.
    {
      std::unique_lock<std::mutex> lock(mutex_);
      queue_.splice(queue_.begin(), work_queue);
    }
  }

  Log("Thread for async operation is terminating.");
}

} // namespace fun
