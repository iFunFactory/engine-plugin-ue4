// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin_ue4.h"
#include "async_delegates.h"
#include "curl/curl.h"

#include <fstream>


namespace fun {

namespace {
// Web header-related constants
const char kDownloaderHeaderDelimeter[] = "\n";
const char kDownloaderHeaderFieldDelimeter[] = ":";
const char kDownloaderHeaderFieldContentLength[] = "content-length";
const int kDownloadUnitBufferSize = 65536;

} // unnamed namespace


// Async event
AsyncEvent& AsyncEvent::instance() {
  static AsyncEvent inst;
  return inst;
}

AsyncEvent::AsyncEvent () {
  thread_.Create();
}

AsyncEvent::~AsyncEvent() {
  thread_.Destroy();
}

void AsyncEvent::AddEvent (EventCallback cb) {
  thread_.AddRequest(
    new EventInfo([this](AsyncRequest* r)->bool{ return AsyncRequestCb(r); }, cb));
}

bool AsyncEvent::AsyncRequestCb (AsyncRequest* r) {
  EventInfo* info = (EventInfo*)r;
  info->callback();
  return true;
}


// Http file request
class HttpFileRequest {
public:
  HttpFileRequest ();
  ~HttpFileRequest ();

private:
  static size_t WriteCb (void*, size_t, size_t, void*);
  bool AsyncRequestCb (AsyncRequest*);
  void HeaderDataCb (void*, const int);
  void BodyDataCb (void*, const int);

private:
  friend class HttpFileDownloader;
  typedef std::function<void(void*, const int)> DataCallback;

  string url_;
  string target_;   // text or save path

  bool is_file_ = false;
  std::ofstream file_;
  int file_length_;

  // Curl-related
  CURL *ctx_ = NULL;
  CURLM *mctx_ = NULL;
  int maxfd_, running_ = 0;
  struct timeval timeout_;
  fd_set fdread_, fdwrite_, fdexcep_;
  int recv_size_;
  DataCallback header_callback_;
  DataCallback body_callback_;

  // Callback-related
  UpdateCallback update_callback_;
  FinishCallback finish_callback_;
};


HttpFileRequest::HttpFileRequest () {
  // curl callbacks
  header_callback_ = [this](void *data, const int len){ HeaderDataCb(data, len); };
  body_callback_ = [this](void *data, const int len){ BodyDataCb(data, len); };
}

HttpFileRequest::~HttpFileRequest () {
}

size_t HttpFileRequest::WriteCb (void *data, size_t size, size_t count, void *cb) {
  DataCallback *callback = (DataCallback*)cb;
  if (callback != NULL)
    (*callback)(data, size * count);

  return size * count;
}

bool HttpFileRequest::AsyncRequestCb (AsyncRequest* request) {
  if (request->state == kThreadStart) {
    running_ = 0;
    ctx_ = curl_easy_init();

    curl_easy_setopt(ctx_, CURLOPT_URL, url_.c_str());
    curl_easy_setopt(ctx_, CURLOPT_HEADERDATA, &header_callback_);
    curl_easy_setopt(ctx_, CURLOPT_WRITEDATA, &body_callback_);
    curl_easy_setopt(ctx_, CURLOPT_WRITEFUNCTION, &WriteCb);

    mctx_ = curl_multi_init();
    curl_multi_add_handle(mctx_, ctx_);
    curl_multi_perform(mctx_, &running_);

    recv_size_ = 0;
    file_length_ = 0;

    if (!target_.empty()) {
      is_file_ = true;
      file_.open(target_.c_str(), std::ios::out | std::ios::binary);
      if (!file_.is_open()) {
        LogError("Failed to create file for downloading. path:%s", *FString(target_.c_str()));
        request->state = kThreadError;
        return true;
      }
    }

    request->state = kThreadUpdate;
  }
  else if (request->state == kThreadUpdate) {
    FD_ZERO(&fdread_);
    FD_ZERO(&fdwrite_);
    FD_ZERO(&fdexcep_);

    timeout_.tv_sec = 1;
    timeout_.tv_usec = 0;

    long curl_timeo = -1;
    curl_multi_timeout(mctx_, &curl_timeo);
    if (curl_timeo >= 0) {
      timeout_.tv_sec = curl_timeo / 1000;
      if (timeout_.tv_sec > 1)
        timeout_.tv_sec = 1;
      else
        timeout_.tv_usec = (curl_timeo % 1000) * 1000;
    }

    maxfd_ = -1;
    curl_multi_fdset(mctx_, &fdread_, &fdwrite_, &fdexcep_, &maxfd_);

    if (maxfd_ != -1) {
      int rc = select(maxfd_ + 1, &fdread_, &fdwrite_, &fdexcep_, &timeout_);
      assert(rc >= 0);

      if (rc == -1) {
        running_ = 0;
        request->state = kThreadError;
        LogError("select() returns error.");
        return false;
      }
      else {
        curl_multi_perform(mctx_, &running_);
      }
    }

    if (!running_)
      request->state = kThreadFinish;
  }
  else if (request->state == kThreadFinish || request->state == kThreadError) {
    curl_multi_remove_handle(mctx_, ctx_);
    curl_easy_cleanup(ctx_);
    curl_multi_cleanup(mctx_);

    if (file_.is_open())
      file_.close();
    else
      target_.append(1, '\0');

    bool succeed = request->state == kThreadFinish;
    finish_callback_(target_, succeed);
    return true;
  }
  else {
    Log("Invalid state value. state: %d", (int)request->state);
    assert(false);
  }

  return false;
}

void HttpFileRequest::HeaderDataCb (void *data, const int len) {
  char buf[1024];
  memcpy(buf, data, len);
  buf[len - 2] = '\0';

  char *ptr = std::search(buf, buf + len, kDownloaderHeaderFieldDelimeter,
    kDownloaderHeaderFieldDelimeter + sizeof(kDownloaderHeaderFieldDelimeter) - 1);
  ssize_t eol_offset = ptr - buf;
  if (eol_offset >= len)
    return;

  // Generates null-terminated string by replacing the delimeter with \0.
  *ptr = '\0';
  const char *e1 = buf, *e2 = ptr + 1;
  while (*e2 == ' ' || *e2 == '\t') ++e2;
  if (std::strcmp(e1, kDownloaderHeaderFieldContentLength) == 0) {
    file_length_ = atoi(e2);
  }

  Log("Decoded header field '%s' => '%s'", *FString(e1), *FString(e2));
}

void HttpFileRequest::BodyDataCb (void *data, const int len) {
  char *buf = reinterpret_cast<char*>(data);
  if (file_.is_open()) {
    file_.write(buf, len);
  }
  else {
    target_.append(buf, len);
  }

  if (is_file_) {
    recv_size_ += len;
    int percentage = float(recv_size_) / float(file_length_) * 100;
    update_callback_(target_, len, recv_size_, percentage);
  }
}


// Http file downloader
HttpFileDownloader::HttpFileDownloader () {
  curl_global_init(CURL_GLOBAL_ALL);
  thread_.Create();
}

HttpFileDownloader::~HttpFileDownloader () {
  thread_.Destroy();
  curl_global_cleanup();
}

void HttpFileDownloader::AddRequest (const string& url, FinishCallback finish) {
  AddRequest(url, "", finish);
}

void HttpFileDownloader::AddRequest (const std::string& url, const string& target_path,
                                     FinishCallback finish, UpdateCallback update /*= nullptr*/) {
  auto request = new AsyncRequestEx<HttpFileRequest>(AsyncRequestCb);
  request->object.url_ = url;
  request->object.target_ = target_path;
  request->object.update_callback_ = update;
  request->object.finish_callback_ = finish;

  thread_.AddRequest(request);
}

bool HttpFileDownloader::AsyncRequestCb (AsyncRequest *request) {
  AsyncRequestEx<HttpFileRequest> *r = (AsyncRequestEx<HttpFileRequest>*)request;
  return r->object.AsyncRequestCb(r);
}

} // namespace fun
