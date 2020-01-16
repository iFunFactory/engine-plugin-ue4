// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_ANNOUNCEMENT_H_
#define SRC_FUNAPI_ANNOUNCEMENT_H_

#include "funapi_plugin.h"

namespace fun {

class FunapiExtraImageInfoImpl;
class FUNAPI_API FunapiExtraImageInfo : public std::enable_shared_from_this<FunapiExtraImageInfo>
{
 public:
  FunapiExtraImageInfo() = delete;
  FunapiExtraImageInfo(const fun::string &image_url,
                       const fun::string &image_md5,
                       const fun::string &file_path);
  virtual ~FunapiExtraImageInfo();

  const fun::string& GetImageUrl();
  const fun::string& GetImageMd5();
  const fun::string& GetFilePath();

 private:
  std::shared_ptr<FunapiExtraImageInfoImpl> impl_;
};


class FunapiAnnouncementInfoImpl;
class FUNAPI_API FunapiAnnouncementInfo : public std::enable_shared_from_this<FunapiAnnouncementInfo> {
 public:
  FunapiAnnouncementInfo() = delete;
  FunapiAnnouncementInfo(const fun::string &date,
                         const fun::string &message,
                         const fun::string &subject,
                         const fun::string &image_md5,
                         const fun::string &image_url,
                         const fun::string &link_url,
                         const fun::string &file_path,
                         const fun::string &kind,
                         const fun::vector<std::shared_ptr<FunapiExtraImageInfo>> &extra_image_infos);
  virtual ~FunapiAnnouncementInfo();

  const fun::string& GetDate();
  const fun::string& GetMessageText();
  const fun::string& GetSubject();
  const fun::string& GetImageMd5();
  const fun::string& GetImageUrl();
  const fun::string& GetLinkUrl();
  const fun::string& GetFilePath();
  const fun::string& GetKind();
  const fun::vector<std::shared_ptr<FunapiExtraImageInfo>>& GetExtraImageInfos();

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
                             const fun::vector<std::shared_ptr<FunapiAnnouncementInfo>>&,
                             const ResultCode)> CompletionHandler;

  FunapiAnnouncement() = delete;
  FunapiAnnouncement(const fun::string &url, const fun::string &path);
  virtual ~FunapiAnnouncement();

  static std::shared_ptr<FunapiAnnouncement> Create(const fun::string &url, const fun::string &path);

  void AddCompletionCallback(const CompletionHandler &handler);
  void RequestList(int max_count, int page = 0, const fun::string& category = "");
  void Update();
  static void UpdateAll();

 private:
  std::shared_ptr<FunapiAnnouncementImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_ANNOUNCEMENT_H_
