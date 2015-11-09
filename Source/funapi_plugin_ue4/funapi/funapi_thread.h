// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#pragma once

#include "curl/curl.h"

namespace fun {

// State
enum ThreadState {
  kThreadStart,
  kThreadUpdate,
  kThreadFinish,
  kThreadError
};

// Request object
struct AsyncRequest {
  typedef std::function<bool(AsyncRequest*)> AsyncProc;
  AsyncRequest (AsyncProc &proc) : state(kThreadStart), proc(proc) {}

  ThreadState state;
  AsyncProc proc;
};

template<typename T>
struct AsyncRequestEx : public AsyncRequest {
  AsyncRequestEx (AsyncProc proc) : AsyncRequest(proc) {}

  T object;
};


// Thread handler
class AsyncThread {
 public:
  AsyncThread ();

  void Create ();
  void Destroy ();

  void AddRequest (AsyncRequest*);

 private:
  static int ThreadProc (AsyncThread*);
  void DoWork ();

 private:
  bool running_;
  std::mutex mutex_;
  std::thread thread_;
  std::condition_variable_any cond_;
  std::list<AsyncRequest*> queue_;
};

} // namespace fun
