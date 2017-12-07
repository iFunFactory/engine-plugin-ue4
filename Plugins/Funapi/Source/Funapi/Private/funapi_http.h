// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_HTTP_H_
#define SRC_FUNAPI_HTTP_H_

#include "funapi_plugin.h"

namespace fun {

class FunapiHttpImpl;
class FunapiHttp : public std::enable_shared_from_this<FunapiHttp> {
 public:
  typedef std::function<void(const int,
                             const std::string&)> ErrorHandler;

  typedef std::function<void(const std::vector<std::string>&,
                             const std::vector<uint8_t>&)> CompletionHandler;

  typedef std::function<void(const std::string&,
                             const std::string&,
                             const uint64_t)> ProgressHandler;

  typedef std::function<void(const std::string&,
                             const std::string&,
                             const std::vector<std::string>&)> DownloadCompletionHandler;

  typedef std::map<std::string, std::string> HeaderFields;

  FunapiHttp();
  FunapiHttp(const std::string &path);
  virtual ~FunapiHttp();

  static std::shared_ptr<FunapiHttp> Create();
  static std::shared_ptr<FunapiHttp> Create(const std::string &path);

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
  std::shared_ptr<FunapiHttpImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_HTTP_H_
