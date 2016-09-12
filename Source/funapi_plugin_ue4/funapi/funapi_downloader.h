// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_DOWNLOADER_H_
#define SRC_FUNAPI_DOWNLOADER_H_

namespace fun {

class FunapiDownloadFileInfo;
class FunapiHttpDownloaderImpl;
class FunapiHttpDownloader : public std::enable_shared_from_this<FunapiHttpDownloader> {
 public:
  enum class ResultCode : int {
    kNone,
    kSucceed,
    kFailed,
  };

  typedef std::function<void(const std::shared_ptr<FunapiHttpDownloader>&,
                             const std::vector<std::shared_ptr<FunapiDownloadFileInfo>>&)> ReadyHandler;

  typedef std::function<void(const std::shared_ptr<FunapiHttpDownloader>&,
                             const std::vector<std::shared_ptr<FunapiDownloadFileInfo>>&,
                             const int index,
                             const int max_index,
                             const uint64_t received_bytes,
                             const uint64_t expected_bytes)> ProgressHandler;

  typedef std::function<void(const std::shared_ptr<FunapiHttpDownloader>&,
                             const std::vector<std::shared_ptr<FunapiDownloadFileInfo>>&,
                             const ResultCode)> CompletionHandler;

  FunapiHttpDownloader() = delete;
  FunapiHttpDownloader(const std::string &url, const std::string &path);
  ~FunapiHttpDownloader();

  static std::shared_ptr<FunapiHttpDownloader> Create(const std::string &url, const std::string &path);

  void AddReadyCallback(const ReadyHandler &handler);
  void AddProgressCallback(const ProgressHandler &handler);
  void AddCompletionCallback(const CompletionHandler &handler);

  void Start();
  void Update();

 private:
  std::shared_ptr<FunapiHttpDownloaderImpl> impl_;
};


class FunapiDownloadFileInfoImpl;
class FunapiDownloadFileInfo : public std::enable_shared_from_this<FunapiDownloadFileInfo> {
 public:
  FunapiDownloadFileInfo() = delete;
  FunapiDownloadFileInfo(const std::string &url,
                         const std::string &path,
                         const uint64_t size,
                         const std::string &hash,
                         const std::string &hash_front);
  ~FunapiDownloadFileInfo();

  const std::string& GetUrl();
  const std::string& GetPath();
  uint64_t GetSize();
  const std::string& GetHash();
  const std::string& GetHashFront();

  const FunapiHttpDownloader::ResultCode GetResultCode();
  void SetResultCode(FunapiHttpDownloader::ResultCode r);

 private:
  std::shared_ptr<FunapiDownloadFileInfoImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_DOWNLOADER_H_
