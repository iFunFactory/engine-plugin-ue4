// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin.h"
#include "funapi_utils.h"
#include "funapi_announcement.h"
#include "md5/md5.h"

namespace fun {

////////////////////////////////////////////////////////////////////////////////
// AnnouncementInfo implementation.

AnnouncementInfo::AnnouncementInfo(const std::string &date,
                                   const std::string &message,
                                   const std::string &subject,
                                   const std::string &image_md5,
                                   const std::string &image_url,
                                   const std::string &link_url,
                                   const std::string &file_path)
: date_(date), message_(message), subject_(subject), image_md5_(image_md5), image_url_(image_url), link_url_(link_url), file_path_(file_path)
{
}


AnnouncementInfo::~AnnouncementInfo() {
}


const std::string& AnnouncementInfo::GetDate() {
  return date_;
}


const std::string& AnnouncementInfo::GetMessageText() {
  return message_;
}


const std::string& AnnouncementInfo::GetSubject() {
  return subject_;
}


const std::string& AnnouncementInfo::GetImageMd5() {
  return image_md5_;
}


const std::string& AnnouncementInfo::GetImageUrl() {
  return image_url_;
}


const std::string& AnnouncementInfo::GetLinkUrl() {
  return link_url_;
}


const std::string& AnnouncementInfo::GetFilePath() {
  return file_path_;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiAnnouncement implementation.

class FunapiAnnouncementImpl : public std::enable_shared_from_this<FunapiAnnouncementImpl> {
 public:
  typedef FunapiAnnouncement::CompletionHandler CompletionHandler;

  FunapiAnnouncementImpl() = delete;
  FunapiAnnouncementImpl(const char* url, const char* path);
  ~FunapiAnnouncementImpl();

  void AddCompletionCallback(const CompletionHandler &handler);
  void RequestList(std::weak_ptr<FunapiAnnouncement> a, int max_count);
  void Update();

 private:
  void DownloadFile(int index);
  void DownloadFile(int index, const std::string &url, const std::string &path);
  bool IsDownloadFile(std::shared_ptr<AnnouncementInfo> info);
  std::string GetMD5String(std::shared_ptr<AnnouncementInfo> info, bool use_front);
  bool MD5Compare(std::shared_ptr<AnnouncementInfo> info);

  std::string url_;
  std::string path_;

  void OnCompletion(const AnnouncementResult result);
  FunapiEvent<CompletionHandler> on_completion_;

  std::queue<std::function<void()>> tasks_queue_;
  std::mutex tasks_queue_mutex_;

  void PushTaskQueue(const std::function<void()> &task);

  std::weak_ptr<FunapiAnnouncement> announcement_;
  std::vector<std::shared_ptr<AnnouncementInfo>> info_list_;
};


FunapiAnnouncementImpl::FunapiAnnouncementImpl(const char *url, const char *path)
  : url_(url), path_(path) {
}


FunapiAnnouncementImpl::~FunapiAnnouncementImpl() {
}


void FunapiAnnouncementImpl::AddCompletionCallback(const CompletionHandler &handler) {
  on_completion_ += handler;
}


void FunapiAnnouncementImpl::OnCompletion(const AnnouncementResult result) {
  PushTaskQueue([this, result]() {
    if (auto a = announcement_.lock()) {
      on_completion_(a, result, info_list_);
    }
  });
}

void FunapiAnnouncementImpl::RequestList(std::weak_ptr<FunapiAnnouncement> a, int max_count) {
  announcement_ = a;

  std::stringstream ss_url;
  ss_url << url_ << "/announcements/?count=" << max_count;

  // DebugUtils::Log("RequestList - url = %s", ss_url.str().c_str());

  auto http_request = FHttpModule::Get().CreateRequest();
  http_request->SetURL(FString(ss_url.str().c_str()));
  http_request->SetVerb(FString("GET"));

  http_request->OnProcessRequestComplete().BindLambda(
    [this](FHttpRequestPtr request, FHttpResponsePtr response, bool succeed) {
    if (!succeed) {
      DebugUtils::Log("Response was invalid!");
      OnCompletion(fun::AnnouncementResult::kInvalidUrl);
    }
    else {
      FString json_fstring = response->GetContentAsString();
      std::string json_string = TCHAR_TO_UTF8(*(json_fstring));

      DebugUtils::Log("%s", json_string.c_str());

      rapidjson::Document document;
      document.Parse<0>(json_string.c_str());

      if (document.HasParseError())
      {
        OnCompletion(fun::AnnouncementResult::kInvalidUrl);
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
              // image_url = url_ + "/images" + v["image_url"].GetString();
              image_url = v["image_url"].GetString();
              std::string file_name = image_url.substr(1);
              image_url = url_ + "/images" + image_url;
              path = path_ + file_name;
            }

            if (v.HasMember("link_url")) {
              link_url = v["link_url"].GetString();
              int index = link_url.rfind("/");
              if (index != std::string::npos) {
                std::string file_name = link_url.substr(index+1);
                path = path_ + file_name;
              }
            }

            fun::DebugUtils::Log("index=%d date=%s message=%s subject=%s image_md5=%s image_url=%s link_url= %s path=%s", i, date.c_str(), message.c_str(), subject.c_str(), image_md5.c_str(), image_url.c_str(), link_url.c_str(), path.c_str());

            info_list_.push_back(std::make_shared<AnnouncementInfo>(date, message, subject, image_md5, image_url, link_url, path));
          }

          DownloadFile(0);
        }

      }
    }
  });
  http_request->ProcessRequest();
}


void FunapiAnnouncementImpl::DownloadFile(int index) {
  DebugUtils::Log("DownloadFile - index = %d", index);

  if (index >= info_list_.size())
  {
    OnCompletion(fun::AnnouncementResult::kSuccess);
  }
  else {
    auto &info = info_list_[index];
    bool bNext = false;

    if (info->GetLinkUrl().length() > 0) {
      DownloadFile(index, info->GetLinkUrl(), info->GetFilePath());
    }
    else if (info->GetImageUrl().length() > 0) {
      if (IsDownloadFile(info)) {
        DownloadFile(index, info->GetImageUrl(), info->GetFilePath());
      }
      else {
        bNext = true;
      }
    }
    else {
      bNext = true;
    }

    if (bNext) {
      DownloadFile(index + 1);
    }
  }
}


void FunapiAnnouncementImpl::DownloadFile(const int index, const std::string &url, const std::string &path) {
  // DebugUtils::Log("RequestList - url = %s", url.c_str());

  auto http_request = FHttpModule::Get().CreateRequest();
  http_request->SetURL(FString(url.c_str()));
  http_request->SetVerb(FString("GET"));

  http_request->OnProcessRequestComplete().BindLambda([this, index, path](FHttpRequestPtr request, FHttpResponsePtr response, bool succeed)
  {
    if (!succeed) {
      DebugUtils::Log("Response was invalid!");
      OnCompletion(fun::AnnouncementResult::kInvalidJson);
    }
    else {
      fun::DebugUtils::Log("path = %s, size = %d", path.c_str(), response->GetContent().Num());

      FFileHelper::SaveArrayToFile(response->GetContent(), ANSI_TO_TCHAR(path.c_str()));

      DownloadFile(index + 1);
    }
  });
  http_request->ProcessRequest();
}


bool FunapiAnnouncementImpl::IsDownloadFile(std::shared_ptr<AnnouncementInfo> info) {
  if (!FPaths::FileExists(ANSI_TO_TCHAR(info->GetFilePath().c_str()))) {
    return true;
  }

  if (!MD5Compare(info)) {
    return true;
  }

  return false;
}


std::string FunapiAnnouncementImpl::GetMD5String(std::shared_ptr<AnnouncementInfo> info, bool use_front) {
  const size_t read_buffer_size = 1048576; // 1024*1024
  const size_t md5_buffer_size = 16;
  std::vector<unsigned char> buffer(read_buffer_size);
  std::vector<unsigned char> md5(md5_buffer_size);
  std::string ret;
  size_t length;

  FILE *fp = fopen(info->GetFilePath().c_str(), "rb");
  if (!fp) {
    return std::string("");
  }

  MD5_CTX ctx;
  MD5_Init(&ctx);
  if (use_front) {
    length = fread(buffer.data(), 1, read_buffer_size, fp);
    MD5_Update(&ctx, buffer.data(), length);
  }
  else {
    while ((length = fread(buffer.data(), 1, read_buffer_size, fp)) != 0) {
      MD5_Update(&ctx, buffer.data(), length);
    }
  }
  MD5_Final(md5.data(), &ctx);
  fclose(fp);

  char c[3];
  for (int i = 0; i<md5_buffer_size; ++i) {
    sprintf(c, "%02x", md5[i]);
    ret.append(c);
  }

  return ret;
}


bool FunapiAnnouncementImpl::MD5Compare(std::shared_ptr<AnnouncementInfo> info) {
  if (info->GetImageMd5().length() > 0) {
    std::string md5_string = GetMD5String(info, false);
    if (info->GetImageMd5().compare(md5_string) == 0) {
      return true;
    }
  }

  return false;
}


void FunapiAnnouncementImpl::Update() {
  std::function<void()> task = nullptr;
  while (true) {
    {
      std::unique_lock<std::mutex> lock(tasks_queue_mutex_);
      if (tasks_queue_.empty()) {
        break;
      }
      else {
        task = std::move(tasks_queue_.front());
        tasks_queue_.pop();
      }
    }

    task();
  }
}


void FunapiAnnouncementImpl::PushTaskQueue(const std::function<void()> &task)
{
  std::unique_lock<std::mutex> lock(tasks_queue_mutex_);
  tasks_queue_.push(task);
}


////////////////////////////////////////////////////////////////////////////////
// FunapiAnnouncement implementation.

FunapiAnnouncement::FunapiAnnouncement(const char* url, const char *path)
  : impl_(std::make_shared<FunapiAnnouncementImpl>(url, path)) {
}


FunapiAnnouncement::~FunapiAnnouncement() {
}


std::shared_ptr<FunapiAnnouncement> FunapiAnnouncement::create(const char* url, const char *path) {
  return std::make_shared<FunapiAnnouncement>(url, path);
}


void FunapiAnnouncement::RequestList(const int max_count) {
  impl_->RequestList(shared_from_this(), max_count);
}


void FunapiAnnouncement::AddCompletionCallback(const CompletionHandler &handler) {
  impl_->AddCompletionCallback(handler);
}


void FunapiAnnouncement::Update() {
  auto self = shared_from_this();
  return impl_->Update();
}


}  // namespace fun
