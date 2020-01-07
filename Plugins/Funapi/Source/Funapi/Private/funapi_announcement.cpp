// Copyright (C) 2013-2020 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_announcement.h"

#ifdef FUNAPI_UE4
#include "FunapiPrivatePCH.h"
#endif

#include "funapi_utils.h"
#include "funapi_tasks.h"
#include "funapi_http.h"

namespace fun {

////////////////////////////////////////////////////////////////////////////////
// FunapiAnnouncementInfoImpl implementation.

class FunapiExtraImageInfoImpl : public std::enable_shared_from_this<FunapiExtraImageInfoImpl> {
 public:
  FunapiExtraImageInfoImpl() = delete;
  FunapiExtraImageInfoImpl(const fun::string &image_url,
                           const fun::string &image_md5,
                           const fun::string &file_path);
  virtual ~FunapiExtraImageInfoImpl();

  const fun::string& GetImageUrl();
  const fun::string& GetImageMd5();
  const fun::string& GetFilePath();

 private:
   fun::string image_url_;
   fun::string image_md5_;
   fun::string file_path_;
};


FunapiExtraImageInfoImpl::FunapiExtraImageInfoImpl(const fun::string &image_url,
                                                   const fun::string &image_md5,
                                                   const fun::string &file_path)
  : image_url_(image_url), image_md5_(image_md5), file_path_(file_path)
{
}


FunapiExtraImageInfoImpl::~FunapiExtraImageInfoImpl()
{
}


const fun::string& FunapiExtraImageInfoImpl::GetImageUrl()
{
  return image_url_;
}


const fun::string& FunapiExtraImageInfoImpl::GetImageMd5()
{
  return image_md5_;
}


const fun::string& FunapiExtraImageInfoImpl::GetFilePath()
{
  return file_path_;
}

////////////////////////////////////////////////////////////////////////////////
// FunapiAnnouncementInfoImpl implementation.

class FunapiAnnouncementInfoImpl : public std::enable_shared_from_this<FunapiAnnouncementInfoImpl> {
 public:
  FunapiAnnouncementInfoImpl() = delete;
  FunapiAnnouncementInfoImpl(const fun::string &date,
                             const fun::string &message,
                             const fun::string &subject,
                             const fun::string &image_md5,
                             const fun::string &image_url,
                             const fun::string &link_url,
                             const fun::string &file_path,
                             const fun::string &kind,
                             const std::vector<std::shared_ptr<FunapiExtraImageInfo>> &extra_image_info);
  virtual ~FunapiAnnouncementInfoImpl();

  const fun::string& GetDate();
  const fun::string& GetMessageText();
  const fun::string& GetSubject();
  const fun::string& GetImageMd5();
  const fun::string& GetImageUrl();
  const fun::string& GetLinkUrl();
  const fun::string& GetFilePath();
  const fun::string& GetKind();
  const std::vector<std::shared_ptr<FunapiExtraImageInfo>>& GetExtraImageInfos();

 private:
  fun::string date_;
  fun::string message_;
  fun::string subject_;
  fun::string image_md5_;
  fun::string image_url_;
  fun::string link_url_;
  fun::string file_path_;
  fun::string kind_;
  std::vector<std::shared_ptr<FunapiExtraImageInfo>> extra_image_infos_;
};


FunapiAnnouncementInfoImpl::FunapiAnnouncementInfoImpl(const fun::string &date,
                                                       const fun::string &message,
                                                       const fun::string &subject,
                                                       const fun::string &image_md5,
                                                       const fun::string &image_url,
                                                       const fun::string &link_url,
                                                       const fun::string &file_path,
                                                       const fun::string &kind,
                                                       const std::vector<std::shared_ptr<FunapiExtraImageInfo>> &extra_image_infos)
: date_(date),
  message_(message),
  subject_(subject),
  image_md5_(image_md5),
  image_url_(image_url),
  link_url_(link_url),
  file_path_(file_path),
  kind_(kind),
  extra_image_infos_(extra_image_infos)
{
}


FunapiAnnouncementInfoImpl::~FunapiAnnouncementInfoImpl() {
}


const fun::string& FunapiAnnouncementInfoImpl::GetDate() {
  return date_;
}


const fun::string& FunapiAnnouncementInfoImpl::GetMessageText() {
  return message_;
}


const fun::string& FunapiAnnouncementInfoImpl::GetSubject() {
  return subject_;
}


const fun::string& FunapiAnnouncementInfoImpl::GetImageMd5() {
  return image_md5_;
}


const fun::string& FunapiAnnouncementInfoImpl::GetImageUrl() {
  return image_url_;
}


const fun::string& FunapiAnnouncementInfoImpl::GetLinkUrl() {
  return link_url_;
}


const fun::string& FunapiAnnouncementInfoImpl::GetFilePath() {
  return file_path_;
}

const fun::string& FunapiAnnouncementInfoImpl::GetKind() {
  return kind_;
}


const std::vector<std::shared_ptr<FunapiExtraImageInfo>>& FunapiAnnouncementInfoImpl::GetExtraImageInfos() {
  return extra_image_infos_;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiExtraImageInfo implementation.

FunapiExtraImageInfo::FunapiExtraImageInfo(const fun::string &image_url,
                                           const fun::string &image_md5,
                                           const fun::string &file_path)
  : impl_(std::make_shared<FunapiExtraImageInfoImpl>(image_url, image_md5, file_path))
{
}


FunapiExtraImageInfo::~FunapiExtraImageInfo()
{
}


const fun::string& FunapiExtraImageInfo::GetImageUrl() {
  return impl_->GetImageUrl();
}


const fun::string& FunapiExtraImageInfo::GetImageMd5() {
  return impl_->GetImageMd5();
}

const fun::string& FunapiExtraImageInfo::GetFilePath() {
  return impl_->GetFilePath();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiAnnouncementInfo implementation.

FunapiAnnouncementInfo::FunapiAnnouncementInfo(const fun::string &date,
                                               const fun::string &message,
                                               const fun::string &subject,
                                               const fun::string &image_md5,
                                               const fun::string &image_url,
                                               const fun::string &link_url,
                                               const fun::string &file_path,
                                               const fun::string &kind,
                                               const std::vector<std::shared_ptr<FunapiExtraImageInfo>> &extra_image_infos)
: impl_(std::make_shared<FunapiAnnouncementInfoImpl>(date, message, subject, image_md5, image_url, link_url, file_path, kind, extra_image_infos)) {
}


FunapiAnnouncementInfo::~FunapiAnnouncementInfo() {
}


const fun::string& FunapiAnnouncementInfo::GetDate() {
  return impl_->GetDate();
}


const fun::string& FunapiAnnouncementInfo::GetMessageText() {
  return impl_->GetMessageText();
}


const fun::string& FunapiAnnouncementInfo::GetSubject() {
  return impl_->GetSubject();
}


const fun::string& FunapiAnnouncementInfo::GetImageMd5() {
  return impl_->GetImageMd5();
}


const fun::string& FunapiAnnouncementInfo::GetImageUrl() {
  return impl_->GetImageUrl();
}


const fun::string& FunapiAnnouncementInfo::GetLinkUrl() {
  return impl_->GetLinkUrl();
}


const fun::string& FunapiAnnouncementInfo::GetFilePath() {
  return impl_->GetFilePath();
}


const fun::string& FunapiAnnouncementInfo::GetKind() {
  return impl_->GetKind();
}


const std::vector<std::shared_ptr<FunapiExtraImageInfo>>& FunapiAnnouncementInfo::GetExtraImageInfos() {
  return impl_->GetExtraImageInfos();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiAnnouncement implementation.

class FunapiAnnouncementImpl : public std::enable_shared_from_this<FunapiAnnouncementImpl> {
 public:
  typedef FunapiAnnouncement::CompletionHandler CompletionHandler;

  FunapiAnnouncementImpl() = delete;
  FunapiAnnouncementImpl(const fun::string &url, const fun::string &path);
  virtual ~FunapiAnnouncementImpl();

  void AddCompletionCallback(const CompletionHandler &handler);
  void RequestList(std::weak_ptr<FunapiAnnouncement> a, int max_count, int page, const fun::string& category);
  void Update();

  static std::shared_ptr<FunapiTasks> GetFunapiTasks();

 private:
  void OnAnnouncementInfoList(const fun::string &json_string);

  void DownloadFiles();
  bool DownloadFile(const fun::string &url, const fun::string &path);
  bool IsDownloadFile(const fun::string &file_path, const fun::string &md5_str);
  bool MD5Compare(const fun::string &file_path, const fun::string &md5_str);

  void OnCompletion(const FunapiAnnouncement::ResultCode result);

  fun::string url_;
  fun::string path_;

  FunapiEvent<CompletionHandler> on_completion_;

  std::weak_ptr<FunapiAnnouncement> announcement_;
  fun::vector<std::shared_ptr<FunapiAnnouncementInfo>> info_list_;

  std::shared_ptr<FunapiTasks> tasks_;
  std::shared_ptr<FunapiThread> thread_;
};


FunapiAnnouncementImpl::FunapiAnnouncementImpl(const fun::string &url, const fun::string &path)
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


void FunapiAnnouncementImpl::OnAnnouncementInfoList(const fun::string &json_string) {
  rapidjson::Document document;
  document.Parse<0>(json_string.c_str());

  if (document.HasParseError())
  {
    OnCompletion(FunapiAnnouncement::ResultCode::kInvalidUrl);
    return;
  }

  if (!info_list_.empty())
  {
    info_list_.clear();
  }

  if (document.HasMember("list"))
  {
    rapidjson::Value &d = document["list"];
    int total_count = d.Size();

    DebugUtils::Log("total_count = %d", total_count);

    for (int i=0;i<total_count;++i) {
      rapidjson::Value &v = d[i];

      fun::string date;
      fun::string message;
      fun::string subject;
      fun::string image_md5;
      fun::string image_url;
      fun::string link_url;
      fun::string path;
      fun::string kind;
      fun::vector<std::shared_ptr<FunapiExtraImageInfo>> extra_images;

      if (v.HasMember("date"))
      {
        date = v["date"].GetString();
      }

      if (v.HasMember("message"))
      {
        message = v["message"].GetString();
      }

      if (v.HasMember("subject"))
      {
        subject = v["subject"].GetString();
      }

      if (v.HasMember("image_md5"))
      {
        image_md5 = v["image_md5"].GetString();
      }

      if (v.HasMember("image_url"))
      {
        image_url = v["image_url"].GetString();
        fun::string file_name = image_url.substr(1);
        image_url = url_ + "/images" + image_url;
        path = path_ + file_name;
      }

      if (v.HasMember("link_url"))
      {
        link_url = v["link_url"].GetString();
        int index = static_cast<int>(link_url.rfind("/"));
        if (index != fun::string::npos)
        {
          fun::string file_name = link_url.substr(index+1);
          path = path_ + file_name;
        }
      }

      if (v.HasMember("kind"))
      {
        kind = v["kind"].GetString();
      }

      if (v.HasMember("extra_images"))
      {
        int extra_image_count = v["extra_images"].Size();
        for (int i = 0; i < extra_image_count; ++i)
        {
          rapidjson::Value &extra = v["extra_images"][i];
          if (extra.HasMember("url") && extra.HasMember("md5"))
          {
            std::string extra_image_url = extra["url"].GetString();
            fun::string file_name = extra_image_url.substr(1);
            extra_image_url = url_ + "/images" + extra_image_url;
            std::string file_path = path_ + file_name;

            std::string extra_imge_md5 = extra["md5"].GetString();

            extra_images.push_back(
                std::make_shared<FunapiExtraImageInfo>(extra_image_url, extra_imge_md5, file_path));
          }
        }

      }

      // fun::DebugUtils::Log("index=%d date=%s message=%s subject=%s image_md5=%s image_url=%s link_url= %s path=%s", i, date.c_str(), message.c_str(), subject.c_str(), image_md5.c_str(), image_url.c_str(), link_url.c_str(), path.c_str());

      info_list_.push_back(std::make_shared<FunapiAnnouncementInfo>(date, message, subject, image_md5, image_url, link_url, path, kind, extra_images));
    }

    DownloadFiles();
  }

}


void FunapiAnnouncementImpl::RequestList(std::weak_ptr<FunapiAnnouncement> a, int max_count, int page, const fun::string& category) {
  announcement_ = a;

  std::weak_ptr<FunapiAnnouncementImpl> weak = shared_from_this();
  thread_->Push([weak, this, max_count, page, category]()->bool
  {
    if (auto impl = weak.lock()) {
      fun::stringstream ss_url;
      ss_url << url_ << "/announcements/?count=" << max_count;
      if (page > 0)
      {
        ss_url << "&page=" << page;
      }
      if (!category.empty())
      {
        ss_url << "&kind=" << FunapiUtil::EncodeUrl(category.c_str());
      }

      DebugUtils::Log("RequestList - url = %s", ss_url.str().c_str());

      auto http = FunapiHttp::Create();
      http->GetRequest
      (ss_url.str(),
       FunapiHttp::HeaderFields(),
       [this]
       (const int error_code,
        const fun::string error_string)
      {
        fun::stringstream ss_temp;
        ss_temp << error_code << " " << error_string;
        DebugUtils::Log ("%s\n", ss_temp.str().c_str());

        OnCompletion(FunapiAnnouncement::ResultCode::kInvalidUrl);
      },
       [this]
       (const fun::vector<fun::string> &headers,
        const fun::vector<uint8_t> &v_recv)
      {
        fun::string temp(v_recv.begin(), v_recv.end());
        DebugUtils::Log ("%s\n", temp.c_str());

        OnAnnouncementInfoList(fun::string(v_recv.begin(), v_recv.end()));
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
    // pair url, file path
    fun::vector<std::pair<std::string, fun::string>> download_list;

    if (info->GetLinkUrl().length() > 0 && info->GetFilePath().length() > 0)
    {
      download_list.push_back(std::pair<std::string, fun::string>(info->GetLinkUrl(), info->GetFilePath()));
    }

    if (info->GetImageUrl().length() > 0)
    {
      if (IsDownloadFile(info->GetFilePath(), info->GetImageMd5()))
      {
        download_list.push_back(std::pair<std::string, fun::string>(info->GetImageUrl(), info->GetFilePath()));
      }
    }

    if (info->GetExtraImageInfos().size() > 0)
    {
      auto extra_infos = info->GetExtraImageInfos();
      for (auto &extra_info : extra_infos)
      {
        if (IsDownloadFile(extra_info->GetFilePath(), extra_info->GetImageMd5()))
        {
          download_list.push_back(std::pair<std::string, fun::string>(extra_info->GetImageUrl(), extra_info->GetFilePath()));
        }
      }
    }

    for (auto &i : download_list)
    {
      if (!DownloadFile(i.first, i.second))
      {
        OnCompletion(fun::FunapiAnnouncement::ResultCode::kExceptionError);
        return;
      }
    }
  }

  OnCompletion(fun::FunapiAnnouncement::ResultCode::kSucceed);
}


bool FunapiAnnouncementImpl::DownloadFile(const fun::string &url, const fun::string &path) {
  bool is_ok = true;

  auto http = FunapiHttp::Create();
  http->DownloadRequest(url, path, FunapiHttp::HeaderFields(),[&is_ok](const int error_code, const fun::string error_string)
  {
    is_ok = false;
  }, [](const fun::string &request_url, const fun::string &target_path, const uint64_t recv_bytes)
  {
  }, [](const fun::string &request_url, const fun::string &target_path, const fun::vector<fun::string> &headers)
  {
  });

  return is_ok;
}


bool FunapiAnnouncementImpl::IsDownloadFile(const std::string &file_path, const std::string &md5_str) {
  if (!FunapiUtil::IsFileExists(file_path)) {
    return true;
  }

  if (!MD5Compare(file_path, md5_str)) {
    return true;
  }

  return false;
}


bool FunapiAnnouncementImpl::MD5Compare(const std::string &file_path, const std::string &md5_str) {
  if (md5_str.length() > 0) {
    fun::string md5_string = FunapiUtil::MD5String(file_path, false);
    if (md5_str.compare(md5_string) == 0) {
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

FunapiAnnouncement::FunapiAnnouncement(const fun::string &url, const fun::string &path)
  : impl_(std::make_shared<FunapiAnnouncementImpl>(url, path)) {
}


FunapiAnnouncement::~FunapiAnnouncement() {
}


std::shared_ptr<FunapiAnnouncement> FunapiAnnouncement::Create(const fun::string &url, const fun::string &path) {
  return std::make_shared<FunapiAnnouncement>(url, path);
}


void FunapiAnnouncement::RequestList(const int max_count, int page, const fun::string& category) {
  impl_->RequestList(shared_from_this(), max_count, page , category);
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
