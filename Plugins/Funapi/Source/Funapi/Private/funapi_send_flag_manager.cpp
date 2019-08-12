// Copyright (C) 2019 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_send_flag_manager.h"
#include "funapi_utils.h"


namespace fun
{

#ifndef FUNAPI_PLATFORM_WINDOWS

bool the_initialized = false;
FunapiSendFlagManager::FunapiSendFlagManager()
{
  Initialize();
}


FunapiSendFlagManager::~FunapiSendFlagManager()
{
  the_initialized = false;
  close(pipe_fds[0]);
  close(pipe_fds[1]);
}


void FunapiSendFlagManager::Initialize()
{
 if (pipe(pipe_fds) == -1)
 {
    DebugUtils::Log("Failed to initilize send flag manger");
    the_initialized = false;
    return;
 }

  the_initialized = true;
}


bool FunapiSendFlagManager::IsInitialized()
{
  return the_initialized;
}


int* FunapiSendFlagManager::GetPipFds()
{
  if (!the_initialized)
  {
    return nullptr;
  }

  return pipe_fds;
}


void FunapiSendFlagManager::WakeUp()
{
  const int64_t kDummyValue = 1;
  if (not write(pipe_fds[1], &kDummyValue, sizeof(int64_t)))
  {
    DebugUtils::Log("Failed to wakeup funapi send flag");
  }
}


void FunapiSendFlagManager::ResetWakeUp()
{
  int64_t value = 0;
  if (not read(pipe_fds[0], &value, sizeof(int64_t)))
  {
    DebugUtils::Log("Failed to reset wakeup funapi send flag");
  }
}


#else // !FUNAPI_PLATFORM_WINDOWS


FunapiSendFlagManager::FunapiSendFlagManager()
{
  Initialize();
}


FunapiSendFlagManager::~FunapiSendFlagManager()
{
  if (IsInitialized())
  {
    WSACloseEvent(event_);
    event_ = nullptr;
  }
}


void FunapiSendFlagManager::Initialize()
{
  event_ = WSACreateEvent();
  if (event_ == WSA_INVALID_EVENT)
  {
    DebugUtils::Log("Failed to initilize send flag manager");
    event_ = nullptr;
  }
}


bool FunapiSendFlagManager::IsInitialized()
{
  if (event_)
  {
    return true;
  }

  return false;
}


HANDLE FunapiSendFlagManager::GetEvent()
{
  if (event_ == nullptr)
  {
    DebugUtils::Log("Failed to event to send flag manager, Retry Initialize");
    Initialize();
    return nullptr;
  }

  return event_;
}


void FunapiSendFlagManager::WakeUp()
{
  const int64_t kDummyValue = 1;
  if (SetEvent(event_) == 0)
  {
    DebugUtils::Log("Failed to wakeup funapi send flag : %d", WSAGetLastError());
  }
}


void FunapiSendFlagManager::ResetWakeUp()
{
  int64_t value = 0;
  if (ResetEvent(event_) == 0)
  {
    DebugUtils::Log("Failed to reset wakeup funapi send flag : %d", WSAGetLastError());
  }
}

#endif

/////////////////////////////////////////////////////////////////////
// Not depend on platform

std::shared_ptr<FunapiSendFlagManager> FunapiSendFlagManager::Get()
{
  static std::weak_ptr<FunapiSendFlagManager> manager;
  static std::mutex send_flag_manager_mutex;

  std::unique_lock<std::mutex> lock(send_flag_manager_mutex);
  if(!manager.expired())
  {
    return manager.lock();
  }

  std::shared_ptr<FunapiSendFlagManager> instance =
      std::make_shared<FunapiSendFlagManager>();
  manager = instance;
  return instance;
}

} // namespace fun
