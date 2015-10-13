// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#pragma once

#include "Engine.h"
#include "UnrealString.h"
#include "Http.h"
#include "Json.h"

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#include <process.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <io.h>
#include <direct.h>
#else
#include <netdb.h>
#include <netinet/in.h>
#endif

#include <map>
#include <string>
#include <list>
#include <sstream>
#include <functional>
#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <thread>

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"

#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif

namespace fun {
  namespace {
    typedef rapidjson::Document Json;
  }

  #define LOG(x) UE_LOG(LogClass, Log, TEXT(x));
  #define LOG1(x, a1) UE_LOG(LogClass, Log, TEXT(x), a1);
  #define LOG2(x, a1, a2) UE_LOG(LogClass, Log, TEXT(x), a1, a2);
} // namespace Fun
