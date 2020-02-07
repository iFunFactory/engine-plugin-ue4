// Copyright (C) 2019 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_send_flag_manager.h"

#ifdef FUNAPI_UE4
#include "FunapiPrivatePCH.h"
#endif

#include "funapi_utils.h"


namespace fun
{

static FunapiSendFlagManager* the_manager = nullptr;

#ifndef FUNAPI_PLATFORM_WINDOWS

FunapiSendFlagManager::FunapiSendFlagManager()
{
  FunapiUtil::Assert(the_manager == nullptr);

  if (pipe(pipe_fds_) < 0)
  {
    fun::stringstream ss;
    ss << "Failed to initilize FunapiSendFlagManager,";
    ss << " error code : " << errno << " error : " << strerror(errno);

    FunapiUtil::Assert(false, ss.str());
  }
}


FunapiSendFlagManager::~FunapiSendFlagManager()
{
  if (pipe_fds_[0] >= 0)
  {
    close(pipe_fds_[0]);
  }
  if (pipe_fds_[1] >= 0)
  {
    close(pipe_fds_[1]);
  }
}


int* FunapiSendFlagManager::GetPipeFds()
{
  return pipe_fds_;
}


void FunapiSendFlagManager::WakeUp()
{
  const int64_t kDummyValue = 1;
  if (not write(pipe_fds_[1], &kDummyValue, sizeof(int64_t)))
  {
    DebugUtils::Log("Failed to wakeup funapi send flag, error code: %d error : %s",
                    errno,
                    strerror(errno));
  }
}


void FunapiSendFlagManager::ResetWakeUp()
{
  int64_t value = 0;
  if (not read(pipe_fds_[0], &value, sizeof(int64_t)))
  {
    DebugUtils::Log("Failed to reset funapi send flag, error code: %d error : %s",
                    errno,
                    strerror(errno));
  }
}


#else // !FUNAPI_PLATFORM_WINDOWS


FunapiSendFlagManager::FunapiSendFlagManager()
{
  event_ = WSACreateEvent();
  if (event_ == WSA_INVALID_EVENT)
  {
    int error_cd = FunapiUtil::GetSocketErrorCode();
    fun::string error_str = FunapiUtil::GetSocketErrorString(error_cd);
    fun::stringstream ss;
    ss << "Failed to initilize FunapiSendFlagManager,";
    ss << " error code : " << error_cd << " error : " << error_str.c_str();

    FunapiUtil::Assert(false, ss.str());
  }
}


FunapiSendFlagManager::~FunapiSendFlagManager()
{
  if (event_ != WSA_INVALID_EVENT)
  {
    WSACloseEvent(event_);
  }
  event_ = WSA_INVALID_EVENT;
}


HANDLE FunapiSendFlagManager::GetEvent()
{
  return event_;
}


void FunapiSendFlagManager::WakeUp()
{
  if (SetEvent(event_) == 0)
  {
    int error_cd = FunapiUtil::GetSocketErrorCode();
    DebugUtils::Log("Failed to wakeup funapi send flag, error code: %d error : %s",
                    error_cd,
                    FunapiUtil::GetSocketErrorString(error_cd).c_str());
  }
}


void FunapiSendFlagManager::ResetWakeUp()
{
  if (ResetEvent(event_) == 0)
  {
    int error_cd = FunapiUtil::GetSocketErrorCode();
    DebugUtils::Log("Failed to reset funapi send flag, error code: %d error : %s",
                    error_cd,
                    FunapiUtil::GetSocketErrorString(error_cd).c_str());
  }
}

#endif

/////////////////////////////////////////////////////////////////////
// Not depend on platform


void FunapiSendFlagManager::Init()
{
  // Invoke when plugin module is started.
  if (the_manager == nullptr)
  {
    the_manager = new FunapiSendFlagManager();
  }
}

FunapiSendFlagManager& FunapiSendFlagManager::Get()
{
  return *the_manager;
}

} // namespace fun
