// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#pragma once

#include "funapi_thread.h"
#include "funapi_utils.h"

namespace fun {

// Async event
class AsyncEvent {
 typedef std::function<void()> EventCallback;

 public:
  static AsyncEvent& instance();

  void AddEvent (EventCallback);

 private:
  AsyncEvent ();
  ~AsyncEvent ();

  bool AsyncRequestCb (AsyncRequest*);

 private:
  class EventInfo : public AsyncRequest {
   public:
    EventInfo (AsyncProc proc, EventCallback cb)
      : AsyncRequest(proc), callback(cb) {}

    EventCallback callback;
  };

  AsyncThread thread_;
};


typedef std::function<void(const string&, uint64, uint64, int)> UpdateCallback;
typedef std::function<void(const string&, bool)> FinishCallback;

// Http file downloader
class HttpFileDownloader {
 public:
  HttpFileDownloader ();
  ~HttpFileDownloader ();

  void AddRequest (const string& url, FinishCallback finish);
  void AddRequest (const string& url, const string& target_path,
                   FinishCallback finish, UpdateCallback update = nullptr);
  void CancelRequest ();

 private:
  static bool AsyncRequestCb (AsyncRequest*);

 private:
  AsyncThread thread_;
};

} // namespace fun
