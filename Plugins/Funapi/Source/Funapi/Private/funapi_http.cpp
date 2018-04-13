// Copyright (C) 2013-2018 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef FUNAPI_UE4_PLATFORM_PS4

#ifdef FUNAPI_UE4
#include "FunapiPrivatePCH.h"
#endif

#include "funapi_http.h"
#include "funapi_utils.h"
#include "funapi_tasks.h"
#include "curl/curl.h"

namespace fun {

////////////////////////////////////////////////////////////////////////////////
// FunapiHttpImpl implementation.

class FunapiHttpImpl : public std::enable_shared_from_this<FunapiHttpImpl> {
 public:
  typedef std::function<void(void*, const size_t)> ResponseCallback;
  typedef FunapiHttp::CompletionHandler CompletionHandler;
  typedef FunapiHttp::ErrorHandler ErrorHandler;
  typedef FunapiHttp::ProgressHandler ProgressHandler;
  typedef FunapiHttp::DownloadCompletionHandler DownloadCompletionHandler;
  typedef FunapiHttp::HeaderFields HeaderFields;

  FunapiHttpImpl();
  FunapiHttpImpl(const std::string &path);
  virtual ~FunapiHttpImpl();

  void PostRequest(const std::string &url,
                   const HeaderFields &header,
                   const std::vector<uint8_t> &body,
                   const ErrorHandler &error_handler,
                   const CompletionHandler &completion_handler);

  void GetRequest(const std::string &url,
                  const HeaderFields &header,
                  const ErrorHandler &error_handler,
                  const CompletionHandler &completion_handler);

  void DownloadRequest(const std::string &url,
                       const std::string &path,
                       const HeaderFields &header,
                       const ErrorHandler &error_handler,
                       const ProgressHandler &progress_handler,
                       const DownloadCompletionHandler &download_completion_handler);

  void SetConnectTimeout(const long seconds);

 private:
  void Init();
  void Cleanup();

  CURL *curl_handle_ = NULL;

  // https://curl.haxx.se/docs/caextract.html
  // https://curl.haxx.se/ca/cacert.pem
  std::string cert_file_path_;

  static size_t OnResponse(void *data, size_t size, size_t count, void *cb);
  void OnResponseHeader(void *data, const size_t len, std::vector<std::string> &headers);
  void OnResponseBody(void *data, const size_t len, std::vector<uint8_t> &receiving);
  void OnResponseFile(void *data, const size_t len, FILE *fp);

  long connect_timeout_seconds_ = 5;
};


FunapiHttpImpl::FunapiHttpImpl() : cert_file_path_("") {
  Init();
}


FunapiHttpImpl::FunapiHttpImpl(const std::string &path) : cert_file_path_(path) {
  Init();
}


FunapiHttpImpl::~FunapiHttpImpl() {
  Cleanup();
  // DebugUtils::Log("%s", __FUNCTION__);
}


void FunapiHttpImpl::Init() {
  curl_handle_ = curl_easy_init();
  if (curl_handle_ == NULL) {
    DebugUtils::Log("%s", curl_easy_strerror(CURLE_FAILED_INIT));
  }
}


void FunapiHttpImpl::Cleanup() {
  if (curl_handle_) {
    curl_easy_cleanup(curl_handle_);
  }
}


void FunapiHttpImpl::DownloadRequest(const std::string &url,
                                     const std::string &path,
                                     const HeaderFields &header,
                                     const ErrorHandler &error_handler,
                                     const ProgressHandler &progress_handler,
                                     const DownloadCompletionHandler &download_completion_handler) {
  std::vector<std::string> header_receiving;
  std::vector<uint8_t> body_receiving;

  FILE *fp = fopen(path.c_str(), "wb");
  if (fp == NULL) {
    error_handler(errno, strerror(errno));
    return;
  }
  uint64_t bytes = 0;

  ResponseCallback receive_header_cb = [this, &header_receiving](void* data, const size_t len){
    OnResponseHeader(data, len, header_receiving);
  };
  ResponseCallback receive_body_cb = [this, &fp, &bytes, &progress_handler, &url, &path](void* data, const size_t len){
    OnResponseFile(data, len, fp);
    bytes += len;
    progress_handler(url, path, bytes);
  };

  CURL *curl = curl_easy_init();
  struct curl_slist *chunk = NULL;
  for (auto it : header) {
    std::stringstream ss;
    ss << it.first << ": " << it.second;
    chunk = curl_slist_append(chunk, ss.str().c_str());
  }

  if (chunk) {
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

  curl_easy_setopt(curl, CURLOPT_TIMEOUT, connect_timeout_seconds_);

#ifdef DEBUG_LOG
  /* Switch on full protocol/debug output while testing */
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  /* disable progress meter, set to 0L to enable and disable debug output */
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
#endif // DEBUG_LOG

  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &receive_header_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &receive_body_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &FunapiHttpImpl::OnResponse);

  if (cert_file_path_.empty()) {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
  } else {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_CAINFO, cert_file_path_.c_str());
  }

  CURLcode res = curl_easy_perform(curl);

  fclose(fp);

  if (res != CURLE_OK) {
    error_handler(res, curl_easy_strerror(res));
  }
  else {
    download_completion_handler(url, path, header_receiving);
  }

  if (chunk) {
    curl_slist_free_all(chunk);
  }

  curl_easy_cleanup(curl);
}


void FunapiHttpImpl::GetRequest(const std::string &url,
                                            const HeaderFields &header,
                                            const ErrorHandler &error_handler,
                                            const CompletionHandler &completion_handler) {
  std::vector<std::string> header_receiving;
  std::vector<uint8_t> body_receiving;

  ResponseCallback receive_header_cb = [this, &header_receiving](void* data, const size_t len){
    OnResponseHeader(data, len, header_receiving);
  };
  ResponseCallback receive_body_cb = [this, &body_receiving](void* data, const size_t len){
    OnResponseBody(data, len, body_receiving);
  };

  CURL *curl = curl_easy_init();
  struct curl_slist *chunk = NULL;
  for (auto it : header) {
    std::stringstream ss;
    ss << it.first << ": " << it.second;
    chunk = curl_slist_append(chunk, ss.str().c_str());
  }
  if (chunk) {
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

  curl_easy_setopt(curl, CURLOPT_TIMEOUT, connect_timeout_seconds_);

  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &receive_header_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &receive_body_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &FunapiHttpImpl::OnResponse);

  if (cert_file_path_.empty()) {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
  } else {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_CAINFO, cert_file_path_.c_str());
  }

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    error_handler(res, curl_easy_strerror(res));
  }
  else {
    completion_handler(header_receiving, body_receiving);
  }

  if (chunk) {
    curl_slist_free_all(chunk);
  }

  curl_easy_cleanup(curl);
}


void FunapiHttpImpl::PostRequest(const std::string &url,
                                 const HeaderFields &header,
                                 const std::vector<uint8_t> &body,
                                 const ErrorHandler &error_handler,
                                 const CompletionHandler &completion_handler) {
  std::vector<std::string> header_receiving;
  std::vector<uint8_t> body_receiving;

  ResponseCallback receive_header_cb = [this, &header_receiving](void* data, const size_t len){
    OnResponseHeader(data, len, header_receiving);
  };
  ResponseCallback receive_body_cb = [this, &body_receiving](void* data, const size_t len){
    OnResponseBody(data, len, body_receiving);
  };

  // set http header
  struct curl_slist *chunk = NULL;
  for (auto it : header) {
    std::stringstream ss;
    ss << it.first << ": " << it.second;

    // DebugUtils::Log("ss.str() = %s", ss.str().c_str());

    chunk = curl_slist_append(chunk, ss.str().c_str());
  }
  if (chunk) {
    curl_easy_setopt(curl_handle_, CURLOPT_HTTPHEADER, chunk);
  }

  curl_easy_setopt(curl_handle_, CURLOPT_URL, url.c_str());

  curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, connect_timeout_seconds_);

  /* enable TCP keep-alive for this transfer */
  curl_easy_setopt(curl_handle_, CURLOPT_TCP_KEEPALIVE, 1L);
  /* keep-alive idle time to 120 seconds */
  curl_easy_setopt(curl_handle_, CURLOPT_TCP_KEEPIDLE, 120L);
  /* interval time between keep-alive probes: 60 seconds */
  curl_easy_setopt(curl_handle_, CURLOPT_TCP_KEEPINTVL, 60L);

  curl_easy_setopt(curl_handle_, CURLOPT_POST, 1L);
  curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDS, body.data());
  curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDSIZE, body.size());

  curl_easy_setopt(curl_handle_, CURLOPT_HEADERDATA, &receive_header_cb);
  curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &receive_body_cb);
  curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, &FunapiHttpImpl::OnResponse);

  if (cert_file_path_.empty()) {
    curl_easy_setopt(curl_handle_, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle_, CURLOPT_SSL_VERIFYHOST, 0L);
  } else {
    curl_easy_setopt(curl_handle_, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl_handle_, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl_handle_, CURLOPT_CAINFO, cert_file_path_.c_str());
  }

  CURLcode res = curl_easy_perform(curl_handle_);
  if (res != CURLE_OK) {
    error_handler(res, curl_easy_strerror(res));
  }
  else {
    long response_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &response_code);
    if (response_code == 200) {
      completion_handler(header_receiving, body_receiving);
    }
    else {
      std::stringstream ss;
      ss << "http response code " << response_code;
      error_handler(static_cast<int>(response_code), ss.str());
    }
  }

  if (chunk) {
    curl_slist_free_all(chunk);
  }
}


void FunapiHttpImpl::OnResponseHeader(void *data, const size_t len, std::vector<std::string> &headers) {
  headers.push_back(std::string((char*)data, (char*)data+len));
}


void FunapiHttpImpl::OnResponseBody(void *data, const size_t len, std::vector<uint8_t> &receiving) {
  receiving.insert(receiving.end(), (uint8_t*)data, (uint8_t*)data + len);
}


void FunapiHttpImpl::OnResponseFile(void *data, const size_t len, FILE *fp) {
  ::fwrite(data, len, 1, (FILE*)fp);
}


size_t FunapiHttpImpl::OnResponse(void *data, size_t size, size_t count, void *cb) {
  ResponseCallback *callback = (ResponseCallback*)(cb);
  size_t len = size * count;
  if (callback != NULL) {
    (*callback)(data, len);
  }

  return len;
}


void FunapiHttpImpl::SetConnectTimeout(const long seconds) {
  connect_timeout_seconds_ = seconds;
}

////////////////////////////////////////////////////////////////////////////////
// FunapiHttp implementation.


FunapiHttp::FunapiHttp() : impl_(std::make_shared<FunapiHttpImpl>()) {
}


FunapiHttp::FunapiHttp(const std::string &path) : impl_(std::make_shared<FunapiHttpImpl>(path)) {
}


FunapiHttp::~FunapiHttp() {
  // DebugUtils::Log("%s", __FUNCTION__);
}


std::shared_ptr<FunapiHttp> FunapiHttp::Create() {
  return std::make_shared<FunapiHttp>();
}


std::shared_ptr<FunapiHttp> FunapiHttp::Create(const std::string &path) {
  return std::make_shared<FunapiHttp>(path);
}


void FunapiHttp::PostRequest(const std::string &url,
                             const HeaderFields &header,
                             const std::vector<uint8_t> &body,
                             const ErrorHandler &error_handler,
                             const CompletionHandler &completion_handler) {
  impl_->PostRequest(url, header, body, error_handler, completion_handler);
}


void FunapiHttp::GetRequest(const std::string &url,
                const HeaderFields &header,
                const ErrorHandler &error_handler,
                            const CompletionHandler &completion_handler) {
  impl_->GetRequest(url, header, error_handler, completion_handler);
}


void FunapiHttp::DownloadRequest(const std::string &url,
                                 const std::string &path,
                                 const HeaderFields &header,
                                 const ErrorHandler &error_handler,
                                 const ProgressHandler &progress_handler,
                                 const DownloadCompletionHandler &download_completion_handler) {
  impl_->DownloadRequest(url, path, header, error_handler, progress_handler, download_completion_handler);
}


void FunapiHttp::SetConnectTimeout(const long seconds) {
  impl_->SetConnectTimeout(seconds);
}

}  // namespace fun

#endif
