// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_ANNOUNCEMENT_H_
#define SRC_FUNAPI_ANNOUNCEMENT_H_

namespace fun {

enum class AnnouncementResult
{
  kSuccess,
  kInvalidUrl,
  kInvalidJson,
  kListIsNullOrEmpty,
  kExceptionError
};


class AnnouncementInfo : public std::enable_shared_from_this<AnnouncementInfo> {
public:
  AnnouncementInfo() = delete;
  AnnouncementInfo(const std::string &date,
                   const std::string &message,
                   const std::string &subject,
                   const std::string &image_md5,
                   const std::string &image_url,
                   const std::string &link_url,
                   const std::string &file_path);
  ~AnnouncementInfo();

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


class FunapiAnnouncementImpl;
class FunapiAnnouncement : public std::enable_shared_from_this<FunapiAnnouncement> {
 public:
  typedef std::function<void(const std::shared_ptr<FunapiAnnouncement>&,
                             const AnnouncementResult,
                             const std::vector<std::shared_ptr<AnnouncementInfo>>&)> CompletionHandler;

  FunapiAnnouncement() = delete;
  FunapiAnnouncement(const char* url, const char* path);
  ~FunapiAnnouncement();

  static std::shared_ptr<FunapiAnnouncement> Create(const char* url, const char* path);

  void AddCompletionCallback(const CompletionHandler &handler);
  void RequestList(int max_count);
  void Update();

 private:
  std::shared_ptr<FunapiAnnouncementImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_ANNOUNCEMENT_H_
