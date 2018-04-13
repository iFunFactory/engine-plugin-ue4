// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifdef FUNAPI_UE4
#include "FunapiPrivatePCH.h"
#endif

#include "funapi_downloader.h"
#include "funapi_utils.h"
#include "funapi_tasks.h"
#include "funapi_http.h"

namespace fun {

////////////////////////////////////////////////////////////////////////////////
// FunapiDownloadFileInfoImpl implementation.

class FunapiDownloadFileInfoImpl : public std::enable_shared_from_this<FunapiDownloadFileInfoImpl> {
 public:
  FunapiDownloadFileInfoImpl() = delete;
  FunapiDownloadFileInfoImpl(const std::string &url, const std::string &path, const uint64_t size, const std::string &hash, const std::string &hash_front);
  virtual ~FunapiDownloadFileInfoImpl();

  const std::string& GetUrl();
  const std::string& GetPath();
  uint64_t GetSize();
  const std::string& GetHash();
  const std::string& GetHashFront();

  const FunapiHttpDownloader::ResultCode GetResultCode();
  void SetResultCode(FunapiHttpDownloader::ResultCode r);

 private:
  std::string url_;          // file url
  std::string path_;         // save file path
  uint64_t size_;            // file size
  std::string hash_;         // md5 hash
  std::string hash_front_;   // front part of file (1MB)

  FunapiHttpDownloader::ResultCode result_code_;
};


FunapiDownloadFileInfoImpl::FunapiDownloadFileInfoImpl(const std::string &url, const std::string &path, const uint64_t size, const std::string &hash, const std::string &hash_front)
: url_(url), path_(path), size_(size), hash_(hash), hash_front_(hash_front), result_code_(FunapiHttpDownloader::ResultCode::kNone)
{
}


FunapiDownloadFileInfoImpl::~FunapiDownloadFileInfoImpl() {
}


const std::string& FunapiDownloadFileInfoImpl::GetUrl() {
  return url_;
}


const std::string& FunapiDownloadFileInfoImpl::GetPath() {
  return path_;
}


uint64_t FunapiDownloadFileInfoImpl::GetSize() {
  return size_;
}


const std::string& FunapiDownloadFileInfoImpl::GetHash() {
  return hash_;
}


const std::string& FunapiDownloadFileInfoImpl::GetHashFront() {
  return hash_front_;
}


const FunapiHttpDownloader::ResultCode FunapiDownloadFileInfoImpl::GetResultCode() {
  return result_code_;
}


void FunapiDownloadFileInfoImpl::SetResultCode(FunapiHttpDownloader::ResultCode r) {
  result_code_ = r;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiDownloadFileInfo implementation.

FunapiDownloadFileInfo::FunapiDownloadFileInfo(const std::string &url, const std::string &path, const uint64_t size, const std::string &hash, const std::string &hash_front)
: impl_(std::make_shared<FunapiDownloadFileInfoImpl>(url, path, size, hash, hash_front))
{
}

FunapiDownloadFileInfo::~FunapiDownloadFileInfo() {
}


const std::string& FunapiDownloadFileInfo::GetUrl() {
  return impl_->GetUrl();
}


const std::string& FunapiDownloadFileInfo::GetPath() {
  return impl_->GetPath();
}


uint64_t FunapiDownloadFileInfo::GetSize() {
  return impl_->GetSize();
}


const std::string& FunapiDownloadFileInfo::GetHash() {
  return impl_->GetHash();
}


const std::string& FunapiDownloadFileInfo::GetHashFront() {
  return impl_->GetHashFront();
}


const FunapiHttpDownloader::ResultCode FunapiDownloadFileInfo::GetResultCode() {
  return impl_->GetResultCode();
}


void FunapiDownloadFileInfo::SetResultCode(FunapiHttpDownloader::ResultCode r) {
  impl_->SetResultCode(r);
}


////////////////////////////////////////////////////////////////////////////////
// FunapiHttpDownloaderImpl implementation.

class FunapiHttpDownloaderImpl : public std::enable_shared_from_this<FunapiHttpDownloaderImpl> {
 public:
  typedef FunapiHttpDownloader::ResultCode ResultCode;
  typedef FunapiHttpDownloader::ReadyHandler ReadyHandler;
  typedef FunapiHttpDownloader::ProgressHandler ProgressHandler;
  typedef FunapiHttpDownloader::CompletionHandler CompletionHandler;

  FunapiHttpDownloaderImpl() = delete;
  FunapiHttpDownloaderImpl(const std::string &url, const std::string &path);
  virtual ~FunapiHttpDownloaderImpl();

  void AddReadyCallback(const ReadyHandler &handler);
  void AddProgressCallback(const ProgressHandler &handler);
  void AddCompletionCallback(const CompletionHandler &handler);

  void Start(std::weak_ptr<FunapiHttpDownloader> d);
  void Update();

  static std::shared_ptr<FunapiTasks> GetFunapiTasks();

 private:
  void GetDownloadList(const std::string &download_url, const std::string &target_path);
  void OnDownloadInfoList(const std::string &json_string);
  void DownloadFiles();
  bool DownloadFile(int index, std::shared_ptr<FunapiDownloadFileInfo> &info);

  FunapiEvent<ReadyHandler> on_ready_;
  FunapiEvent<ProgressHandler> on_progress_;
  FunapiEvent<CompletionHandler> on_completion_;

  void OnReady();
  void OnProgress(const int index, const uint64_t bytes);
  void OnCompletion(ResultCode code);

  std::weak_ptr<FunapiHttpDownloader> downloader_;

  std::shared_ptr<FunapiTasks> tasks_;

  std::vector<std::shared_ptr<FunapiDownloadFileInfo>> info_list_;

  bool IsDownloadFile(std::shared_ptr<FunapiDownloadFileInfo> info);
  bool MD5Compare(std::shared_ptr<FunapiDownloadFileInfo> info);

  std::string url_;
  std::string path_;

  std::shared_ptr<FunapiThread> thread_;

  int max_index_;
};


FunapiHttpDownloaderImpl::FunapiHttpDownloaderImpl(const std::string &url, const std::string &path)
: url_(url), path_(path) {
  tasks_ = FunapiHttpDownloaderImpl::GetFunapiTasks();
  thread_ = FunapiThread::Get("_file");
}


FunapiHttpDownloaderImpl::~FunapiHttpDownloaderImpl() {
}


void FunapiHttpDownloaderImpl::Start(std::weak_ptr<FunapiHttpDownloader> d) {
  downloader_ = d;

  std::weak_ptr<FunapiHttpDownloaderImpl> weak = shared_from_this();
  thread_->Push([weak, this]()->bool {
    if (auto impl = weak.lock()) {
      GetDownloadList(url_, path_);
    }

    return true;
  });
}


bool FunapiHttpDownloaderImpl::DownloadFile(int index, std::shared_ptr<FunapiDownloadFileInfo> &info) {
  bool is_ok = true;
  auto http = FunapiHttp::Create();
  http->DownloadRequest(info->GetUrl(),
                        info->GetPath(),
                        FunapiHttp::HeaderFields(),[&is_ok,&info](const int error_code, const std::string error_string)
  {
    info->SetResultCode(FunapiHttpDownloader::ResultCode::kFailed);
    is_ok = false;
  }, [this, index](const std::string &url, const std::string &path, const uint64_t recv_bytes)
  {
    OnProgress(index, recv_bytes);
  }, [&info](const std::string &url, const std::string &path, const std::vector<std::string> &headers)
  {
    info->SetResultCode(FunapiHttpDownloader::ResultCode::kSucceed);
  });

  return is_ok;
}


void FunapiHttpDownloaderImpl::DownloadFiles() {
  if (info_list_.empty()) {
    OnCompletion(fun::FunapiHttpDownloader::ResultCode::kFailed);
    return;
  }

  OnReady();

  for (size_t i=0;i<info_list_.size();++i) {
    auto &info = info_list_[i];
    if (IsDownloadFile(info)) {
      if (DownloadFile(static_cast<int>(i), info) == false) {
        OnCompletion(FunapiHttpDownloader::ResultCode::kFailed);
        return;
      }
    }
    else {
      info->SetResultCode(FunapiHttpDownloader::ResultCode::kSucceed);
    }
  }

  OnCompletion(FunapiHttpDownloader::ResultCode::kSucceed);
}


void FunapiHttpDownloaderImpl::OnDownloadInfoList(const std::string &json_string) {
  rapidjson::Document document;
  document.Parse<0>(json_string.c_str());

  if (document.HasParseError())
  {
    OnCompletion(FunapiHttpDownloader::ResultCode::kFailed);
  }
  else {
    if (document.HasMember("data")) {
      rapidjson::Value &d = document["data"];
      max_index_ = d.Size() - 1;

      for (int i=0;i<=max_index_;++i) {
        rapidjson::Value &v = d[i];

        std::string url;
        std::string path;
        std::string md5;
        std::string md5_front;

        uint64_t size = 0;

        if (v.HasMember("path")) {
          url = url_ + "/" + v["path"].GetString();
          path = path_ + v["path"].GetString();
        }

        if (v.HasMember("md5")) {
          md5 = v["md5"].GetString();
        }

        if (v.HasMember("md5_front")) {
          md5_front = v["md5_front"].GetString();
        }

        if (v.HasMember("size")) {
          size = v["size"].GetUint64();
        }

//        fun::DebugUtils::Log("index=%d path=%s size=%llu md5=%s md5_front=%s", i, path.c_str(), size, md5.c_str(), md5_front.c_str());

        info_list_.push_back(std::make_shared<FunapiDownloadFileInfo>(url, path, size, md5, md5_front));
      }

      DownloadFiles();
    }
  }
}


void FunapiHttpDownloaderImpl::GetDownloadList(const std::string &download_url, const std::string &target_path) {
  auto http = FunapiHttp::Create();
  http->PostRequest(download_url, FunapiHttp::HeaderFields(), std::vector<uint8_t>(), [this](int code, const std::string error_string)
  {
    DebugUtils::Log("Error from cURL: %d, %s", code, error_string.c_str());
    OnCompletion(FunapiHttpDownloader::ResultCode::kFailed);
  }, [this](const std::vector<std::string> &v_header, const std::vector<uint8_t> &v_body)
  {
    OnDownloadInfoList(std::string(v_body.begin(), v_body.end()));
  });
}


bool FunapiHttpDownloaderImpl::IsDownloadFile(std::shared_ptr<FunapiDownloadFileInfo> info) {
  if (!FunapiUtil::IsFileExists(info->GetPath())) {
    return true;
  }

  if (FunapiUtil::GetFileSize(info->GetPath()) != info->GetSize()) {
    return true;
  }

  if (!MD5Compare(info)) {
    return true;
  }

  return false;
}


bool FunapiHttpDownloaderImpl::MD5Compare(std::shared_ptr<FunapiDownloadFileInfo> info) {
  if (info->GetHashFront().length() > 0) {
    std::string md5_string = FunapiUtil::MD5String(info->GetPath(), true);
    if (info->GetHashFront().compare(md5_string) == 0) {
      return true;
    }
  }

  if (info->GetHash().length() > 0) {
    std::string md5_string = FunapiUtil::MD5String(info->GetPath(), false);
    if (info->GetHash().compare(md5_string) == 0) {
      return true;
    }
  }

  return false;
}


void FunapiHttpDownloaderImpl::AddReadyCallback(const ReadyHandler &handler) {
  on_ready_ += handler;
}


void FunapiHttpDownloaderImpl::AddProgressCallback(const ProgressHandler &handler) {
  on_progress_ += handler;
}


void FunapiHttpDownloaderImpl::AddCompletionCallback(const CompletionHandler &handler) {
  on_completion_ += handler;
}


void FunapiHttpDownloaderImpl::OnReady() {
  std::weak_ptr<FunapiHttpDownloaderImpl> weak = shared_from_this();
  tasks_->Push([weak, this]()->bool {
    if (auto impl = weak.lock()) {
      if (auto d = downloader_.lock()) {
        on_ready_(d, info_list_);
      }
    }

    return true;
  });
}


void FunapiHttpDownloaderImpl::OnProgress(const int index, const uint64_t bytes) {
  std::weak_ptr<FunapiHttpDownloaderImpl> weak = shared_from_this();
  tasks_->Push([weak, this, index, bytes]()->bool {
    if (auto impl = weak.lock()) {
      if (auto d = downloader_.lock()) {
        auto info = info_list_[index];
        on_progress_(d, info_list_, index, max_index_, bytes, info->GetSize());
      }
    }

    return true;
  });
}


void FunapiHttpDownloaderImpl::OnCompletion(const ResultCode result) {
  std::weak_ptr<FunapiHttpDownloaderImpl> weak = shared_from_this();
  tasks_->Push([weak, this, result]()->bool {
    if (auto impl = weak.lock()) {
      if (auto d = downloader_.lock()) {
        on_completion_(d, info_list_, result);
      }
    }

    return true;
  });
}


void FunapiHttpDownloaderImpl::Update() {
  tasks_->Update();
}


std::shared_ptr<FunapiTasks> FunapiHttpDownloaderImpl::GetFunapiTasks() {
  static std::shared_ptr<FunapiTasks> tasks = FunapiTasks::Create();
  return tasks;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiHttpDownloader implementation.

FunapiHttpDownloader::FunapiHttpDownloader(const std::string &url, const std::string &path)
  : impl_(std::make_shared<FunapiHttpDownloaderImpl>(url, path)) {
}


FunapiHttpDownloader::~FunapiHttpDownloader() {
}


std::shared_ptr<FunapiHttpDownloader> FunapiHttpDownloader::Create(const std::string &url, const std::string &path) {
  return std::make_shared<FunapiHttpDownloader>(url, path);
}


void FunapiHttpDownloader::AddReadyCallback(const ReadyHandler &handler) {
  impl_->AddReadyCallback(handler);
}


void FunapiHttpDownloader::AddProgressCallback(const ProgressHandler &handler) {
  impl_->AddProgressCallback(handler);
}


void FunapiHttpDownloader::AddCompletionCallback(const CompletionHandler &handler) {
  impl_->AddCompletionCallback(handler);
}


void FunapiHttpDownloader::Start() {
  return impl_->Start(shared_from_this());
}


void FunapiHttpDownloader::Update() {
  return impl_->Update();
}


void FunapiHttpDownloader::UpdateAll() {
  if (auto t = FunapiHttpDownloaderImpl::GetFunapiTasks()) {
    t->Update();
  }
}

}  // namespace fun
