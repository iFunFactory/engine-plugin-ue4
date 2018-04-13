// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifdef FUNAPI_UE4
#include "FunapiPrivatePCH.h"
#endif

#include "funapi_announcement.h"
#include "funapi_utils.h"
#include "funapi_tasks.h"
#include "funapi_http.h"

namespace fun {

////////////////////////////////////////////////////////////////////////////////
// FunapiAnnouncementInfoImpl implementation.

class FunapiAnnouncementInfoImpl : public std::enable_shared_from_this<FunapiAnnouncementInfoImpl> {
 public:
  FunapiAnnouncementInfoImpl() = delete;
  FunapiAnnouncementInfoImpl(const std::string &date,
                             const std::string &message,
                             const std::string &subject,
                             const std::string &image_md5,
                             const std::string &image_url,
                             const std::string &link_url,
                             const std::string &file_path);
  virtual ~FunapiAnnouncementInfoImpl();

  const std::string& GetDate();
  const std::string& GetMessageText();
  const std::string& GetSubject();
  const std::string& GetImageMd5();
  const std::string& GetImageUrl();
  const std::string& GetLinkUrl();
  const std::string& GetFilePath();

 private:
  std::string date_;
  std::string message_;
  std::string subject_;
  std::string image_md5_;
  std::string image_url_;
  std::string link_url_;
  std::string file_path_;
};


FunapiAnnouncementInfoImpl::FunapiAnnouncementInfoImpl(const std::string &date,
                                                       const std::string &message,
                                                       const std::string &subject,
                                                       const std::string &image_md5,
                                                       const std::string &image_url,
                                                       const std::string &link_url,
                                                       const std::string &file_path)
: date_(date),
  message_(message),
  subject_(subject),
  image_md5_(image_md5),
  image_url_(image_url),
  link_url_(link_url),
  file_path_(file_path)
{
}


FunapiAnnouncementInfoImpl::~FunapiAnnouncementInfoImpl() {
}


const std::string& FunapiAnnouncementInfoImpl::GetDate() {
  return date_;
}


const std::string& FunapiAnnouncementInfoImpl::GetMessageText() {
  return message_;
}


const std::string& FunapiAnnouncementInfoImpl::GetSubject() {
  return subject_;
}


const std::string& FunapiAnnouncementInfoImpl::GetImageMd5() {
  return image_md5_;
}


const std::string& FunapiAnnouncementInfoImpl::GetImageUrl() {
  return image_url_;
}


const std::string& FunapiAnnouncementInfoImpl::GetLinkUrl() {
  return link_url_;
}


const std::string& FunapiAnnouncementInfoImpl::GetFilePath() {
  return file_path_;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiAnnouncementInfo implementation.

FunapiAnnouncementInfo::FunapiAnnouncementInfo(const std::string &date,
                                   const std::string &message,
                                   const std::string &subject,
                                   const std::string &image_md5,
                                   const std::string &image_url,
                                   const std::string &link_url,
                                   const std::string &file_path)
: impl_(std::make_shared<FunapiAnnouncementInfoImpl>(date, message, subject, image_md5, image_url, link_url, file_path)) {
}


FunapiAnnouncementInfo::~FunapiAnnouncementInfo() {
}


const std::string& FunapiAnnouncementInfo::GetDate() {
  return impl_->GetDate();
}


const std::string& FunapiAnnouncementInfo::GetMessageText() {
  return impl_->GetMessageText();
}


const std::string& FunapiAnnouncementInfo::GetSubject() {
  return impl_->GetSubject();
}


const std::string& FunapiAnnouncementInfo::GetImageMd5() {
  return impl_->GetImageMd5();
}


const std::string& FunapiAnnouncementInfo::GetImageUrl() {
  return impl_->GetImageUrl();
}


const std::string& FunapiAnnouncementInfo::GetLinkUrl() {
  return impl_->GetLinkUrl();
}


const std::string& FunapiAnnouncementInfo::GetFilePath() {
  return impl_->GetFilePath();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiAnnouncement implementation.

class FunapiAnnouncementImpl : public std::enable_shared_from_this<FunapiAnnouncementImpl> {
 public:
  typedef FunapiAnnouncement::CompletionHandler CompletionHandler;

  FunapiAnnouncementImpl() = delete;
  FunapiAnnouncementImpl(const std::string &url, const std::string &path);
  virtual ~FunapiAnnouncementImpl();

  void AddCompletionCallback(const CompletionHandler &handler);
  void RequestList(std::weak_ptr<FunapiAnnouncement> a, int max_count);
  void Update();

  static std::shared_ptr<FunapiTasks> GetFunapiTasks();

 private:
  void OnAnnouncementInfoList(const std::string &json_string);

  void DownloadFiles();
  bool DownloadFile(const std::string &url, const std::string &path);
  bool IsDownloadFile(std::shared_ptr<FunapiAnnouncementInfo> info);
  bool MD5Compare(std::shared_ptr<FunapiAnnouncementInfo> info);

  void OnCompletion(const FunapiAnnouncement::ResultCode result);

  std::string url_;
  std::string path_;

  FunapiEvent<CompletionHandler> on_completion_;

  std::weak_ptr<FunapiAnnouncement> announcement_;
  std::vector<std::shared_ptr<FunapiAnnouncementInfo>> info_list_;

  std::shared_ptr<FunapiTasks> tasks_;
  std::shared_ptr<FunapiThread> thread_;
};


FunapiAnnouncementImpl::FunapiAnnouncementImpl(const std::string &url, const std::string &path)
: url_(url), path_(path) {
  tasks_ = FunapiAnnouncementImpl::GetFunapiTasks();
  thread_ = FunapiThread::Get("_file");
}


FunapiAnnouncementImpl::~FunapiAnnouncementImpl() {
}


void FunapiAnnouncementImpl::AddCompletionCallback(const CompletionHandler &handler) {
  on_completion_ += handler;
}


void FunapiAnnouncementImpl::OnCompletion(const FunapiAnnouncement::ResultCode result) {
  std::weak_ptr<FunapiAnnouncementImpl> weak = shared_from_this();
  tasks_->Push([weak, this, result]()->bool {
    if (auto impl = weak.lock()) {
      if (auto a = announcement_.lock()) {
        on_completion_(a, info_list_, result);
      }
    }

    return true;
  });
}


void FunapiAnnouncementImpl::OnAnnouncementInfoList(const std::string &json_string) {
  rapidjson::Document document;
  document.Parse<0>(json_string.c_str());

  if (document.HasParseError())
  {
    OnCompletion(FunapiAnnouncement::ResultCode::kInvalidUrl);
  }
  else {
    if (document.HasMember("list")) {
      rapidjson::Value &d = document["list"];
      int total_count = d.Size();

      DebugUtils::Log("total_count = %d", total_count);

      for (int i=0;i<total_count;++i) {
        rapidjson::Value &v = d[i];

        std::string date;
        std::string message;
        std::string subject;
        std::string image_md5;
        std::string image_url;
        std::string link_url;
        std::string path;

        if (v.HasMember("date")) {
          date = v["date"].GetString();
        }

        if (v.HasMember("message")) {
          message = v["message"].GetString();
        }

        if (v.HasMember("subject")) {
          subject = v["subject"].GetString();
        }

        if (v.HasMember("image_md5")) {
          image_md5 = v["image_md5"].GetString();
        }

        if (v.HasMember("image_url")) {
          image_url = v["image_url"].GetString();
          std::string file_name = image_url.substr(1);
          image_url = url_ + "/images" + image_url;
          path = path_ + file_name;
        }

        if (v.HasMember("link_url")) {
          link_url = v["link_url"].GetString();
          int index = static_cast<int>(link_url.rfind("/"));
          if (index != std::string::npos) {
            std::string file_name = link_url.substr(index+1);
            path = path_ + file_name;
          }
        }

        // fun::DebugUtils::Log("index=%d date=%s message=%s subject=%s image_md5=%s image_url=%s link_url= %s path=%s", i, date.c_str(), message.c_str(), subject.c_str(), image_md5.c_str(), image_url.c_str(), link_url.c_str(), path.c_str());

        info_list_.push_back(std::make_shared<FunapiAnnouncementInfo>(date, message, subject, image_md5, image_url, link_url, path));
      }

      DownloadFiles();
    }
  }
}


void FunapiAnnouncementImpl::RequestList(std::weak_ptr<FunapiAnnouncement> a, int max_count) {
  announcement_ = a;

  std::weak_ptr<FunapiAnnouncementImpl> weak = shared_from_this();
  thread_->Push([weak, this, max_count]()->bool
  {
    if (auto impl = weak.lock()) {
      std::stringstream ss_url;
      ss_url << url_ << "/announcements/?count=" << max_count;

      DebugUtils::Log("RequestList - url = %s", ss_url.str().c_str());

      auto http = FunapiHttp::Create();
      http->GetRequest
      (ss_url.str(),
       FunapiHttp::HeaderFields(),
       [this]
       (const int error_code,
        const std::string error_string)
      {
        std::stringstream ss_temp;
        ss_temp << error_code << " " << error_string;
        DebugUtils::Log ("%s\n", ss_temp.str().c_str());

        OnCompletion(FunapiAnnouncement::ResultCode::kInvalidUrl);
      },
       [this]
       (const std::vector<std::string> &headers,
        const std::vector<uint8_t> &v_recv)
      {
        std::string temp(v_recv.begin(), v_recv.end());
        DebugUtils::Log ("%s\n", temp.c_str());

        OnAnnouncementInfoList(std::string(v_recv.begin(), v_recv.end()));
      });
    }

    return true;
  });
}


void FunapiAnnouncementImpl::DownloadFiles() {
  if (info_list_.empty()) {
    OnCompletion(fun::FunapiAnnouncement::ResultCode::kListIsNullOrEmpty);
    return;
  }

  for (size_t i=0;i<info_list_.size();++i) {
    auto &info = info_list_[i];
    std::string url;
    if (info->GetLinkUrl().length() > 0) {
      url = info->GetLinkUrl();
    }
    else if (info->GetImageUrl().length() > 0) {
      if (IsDownloadFile(info)) {
        url = info->GetImageUrl();
      }
      else {
        continue;
      }
    }

    if (url.length() > 0 && info->GetFilePath().length() > 0) {
      if (!DownloadFile(url, info->GetFilePath())) {
        OnCompletion(fun::FunapiAnnouncement::ResultCode::kExceptionError);
        return;
      }
    }
  }

  OnCompletion(fun::FunapiAnnouncement::ResultCode::kSucceed);
}


bool FunapiAnnouncementImpl::DownloadFile(const std::string &url, const std::string &path) {
  bool is_ok = true;

  auto http = FunapiHttp::Create();
  http->DownloadRequest(url, path, FunapiHttp::HeaderFields(),[&is_ok](const int error_code, const std::string error_string)
  {
    is_ok = false;
  }, [](const std::string &request_url, const std::string &target_path, const uint64_t recv_bytes)
  {
  }, [](const std::string &request_url, const std::string &target_path, const std::vector<std::string> &headers)
  {
  });

  return is_ok;
}


bool FunapiAnnouncementImpl::IsDownloadFile(std::shared_ptr<FunapiAnnouncementInfo> info) {
  if (!FunapiUtil::IsFileExists(info->GetFilePath())) {
    return true;
  }

  if (!MD5Compare(info)) {
    return true;
  }

  return false;
}


bool FunapiAnnouncementImpl::MD5Compare(std::shared_ptr<FunapiAnnouncementInfo> info) {
  if (info->GetImageMd5().length() > 0) {
    std::string md5_string = FunapiUtil::MD5String(info->GetFilePath(), false);
    if (info->GetImageMd5().compare(md5_string) == 0) {
      return true;
    }
  }

  return false;
}


void FunapiAnnouncementImpl::Update() {
  tasks_->Update();
}


std::shared_ptr<FunapiTasks> FunapiAnnouncementImpl::GetFunapiTasks() {
  static std::shared_ptr<FunapiTasks> tasks = FunapiTasks::Create();
  return tasks;
}

////////////////////////////////////////////////////////////////////////////////
// FunapiAnnouncement implementation.

FunapiAnnouncement::FunapiAnnouncement(const std::string &url, const std::string &path)
  : impl_(std::make_shared<FunapiAnnouncementImpl>(url, path)) {
}


FunapiAnnouncement::~FunapiAnnouncement() {
}


std::shared_ptr<FunapiAnnouncement> FunapiAnnouncement::Create(const std::string &url, const std::string &path) {
  return std::make_shared<FunapiAnnouncement>(url, path);
}


void FunapiAnnouncement::RequestList(const int max_count) {
  impl_->RequestList(shared_from_this(), max_count);
}


void FunapiAnnouncement::AddCompletionCallback(const CompletionHandler &handler) {
  impl_->AddCompletionCallback(handler);
}


void FunapiAnnouncement::Update() {
  return impl_->Update();
}


void FunapiAnnouncement::UpdateAll() {
  if (auto t = FunapiAnnouncementImpl::GetFunapiTasks()) {
    t->Update();
  }
}

}  // namespace fun
