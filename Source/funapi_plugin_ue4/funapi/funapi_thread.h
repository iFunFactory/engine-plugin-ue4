// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#pragma once

#include "curl/curl.h"

namespace fun {

// Request object
class AsyncRequest {
 public:
  typedef std::function<bool(AsyncRequest*)> AsyncProc;
  AsyncRequest (AsyncProc &proc) : proc(proc) {}
  virtual ~AsyncRequest () {}

  AsyncProc proc;
};

template<typename T>
class AsyncRequestEx : public AsyncRequest {
 public:
  AsyncRequestEx (AsyncProc proc) : AsyncRequest(proc) {}
  virtual ~AsyncRequestEx () {}

  T object;
};


// Thread handler
class AsyncThread {
 public:
  AsyncThread ();

  void Create ();
  void Destroy ();

  void AddRequest (AsyncRequest*);
  void CancelRequest ();

 private:
  static int ThreadProc (AsyncThread*);
  void DoWork ();

 private:
  bool running_;
  bool cancel_request_;
  std::mutex mutex_;
  std::thread thread_;
  std::condition_variable_any cond_;
  std::list<AsyncRequest*> queue_;
};

} // namespace fun
