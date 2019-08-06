// Copyright (C) 2019 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_SEND_FLAG_MANAGER_H_
#define SRC_FUNAPI_SEND_FLAG_MANAGER_H_

#include "funapi_plugin.h"

namespace fun {

class FUNAPI_API FunapiSendFlagManager :
    public std::enable_shared_from_this<FunapiSendFlagManager>
{
 public:
  static std::shared_ptr<FunapiSendFlagManager> Get();

  FunapiSendFlagManager();
  virtual ~FunapiSendFlagManager();

  bool IsInitialized();

  void WakeUp();
  void ResetWakeUp();

 private:
  void Initialize();

#ifdef FUNAPI_PLATFORM_WINDOWS

 public:
  HANDLE GetEvent();

 private:
  HANDLE event_ = nullptr;

#else

 public:
  int* GetPipFds();

 private:
  int pipe_fds[2];

#endif

};

} // namespace fun

#endif // SRC_FUNAPI_SEND_FLAG_MANAGER_H_
