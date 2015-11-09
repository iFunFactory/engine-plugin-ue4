// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#pragma once

#include "funapi_utils.h"

namespace fun {

enum DownloadResult {
  kDownloadSuccess,
  kDownloadFailure
};


class FunapiHttpDownloaderImpl;
class FunapiHttpDownloader {
 public:
  typedef std::function<void(const std::string&,const long,const long,const int)> OnUpdate;
  typedef std::function<void(const int)> OnFinished;

  FunapiHttpDownloader ();
  ~FunapiHttpDownloader ();

  void GetDownloadList (const char *hostname_or_ip, uint16_t port,
                        bool https, const char *target_path);
  void GetDownloadList (const char *url, const char *target_path);
  void StartDownload ();
  void Stop ();

  bool IsDownloading () const;

 public:
  FEvent<string> VerifyCallback;
  FEvent<int, uint64> ReadyCallback;
  FEvent<string, uint64, uint64, int> UpdateCallback;
  FEvent<DownloadResult> FinishCallback;

 private:
  FunapiHttpDownloaderImpl *impl_;
};

}  // namespace fun
