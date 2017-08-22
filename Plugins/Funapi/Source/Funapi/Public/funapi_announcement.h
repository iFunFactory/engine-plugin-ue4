// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_ANNOUNCEMENT_H_
#define SRC_FUNAPI_ANNOUNCEMENT_H_

#include "funapi_plugin.h"

namespace fun {

class FunapiAnnouncementInfoImpl;
class FUNAPI_API FunapiAnnouncementInfo : public std::enable_shared_from_this<FunapiAnnouncementInfo> {
 public:
  FunapiAnnouncementInfo() = delete;
  FunapiAnnouncementInfo(const std::string &date,
                   const std::string &message,
                   const std::string &subject,
                   const std::string &image_md5,
                   const std::string &image_url,
                   const std::string &link_url,
                   const std::string &file_path);
  virtual ~FunapiAnnouncementInfo();

  const std::string& GetDate();
  const std::string& GetMessageText();
  const std::string& GetSubject();
  const std::string& GetImageMd5();
  const std::string& GetImageUrl();
  const std::string& GetLinkUrl();
  const std::string& GetFilePath();

 private:
  std::shared_ptr<FunapiAnnouncementInfoImpl> impl_;
};


class FunapiAnnouncementImpl;
class FUNAPI_API FunapiAnnouncement : public std::enable_shared_from_this<FunapiAnnouncement> {
 public:
  enum class ResultCode : int {
    kSucceed,
    kInvalidUrl,
    kInvalidJson,
    kListIsNullOrEmpty,
    kExceptionError
  };

  typedef std::function<void(const std::shared_ptr<FunapiAnnouncement>&,
                             const std::vector<std::shared_ptr<FunapiAnnouncementInfo>>&,
                             const ResultCode)> CompletionHandler;

  FunapiAnnouncement() = delete;
  FunapiAnnouncement(const std::string &url, const std::string &path);
  virtual ~FunapiAnnouncement();

  static std::shared_ptr<FunapiAnnouncement> Create(const std::string &url, const std::string &path);

  void AddCompletionCallback(const CompletionHandler &handler);
  void RequestList(int max_count);
  void Update();
  static void UpdateAll();

 private:
  std::shared_ptr<FunapiAnnouncementImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_ANNOUNCEMENT_H_
