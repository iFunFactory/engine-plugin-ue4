// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_PLUGIN_H_
#define SRC_FUNAPI_PLUGIN_H_

#include "funapi_build_config.h"

#ifdef FUNAPI_UE4
#include "Engine.h"
#include "UnrealString.h"
#include "Json.h"
#endif

#ifdef FUNAPI_UE4_PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif

#ifdef FUNAPI_PLATFORM_WINDOWS
#include <process.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <io.h>
#include <direct.h>
#else
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
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
#include <chrono>
#include <algorithm>
#include <random>

#include "curl/curl.h"

#ifdef FUNAPI_COCOS2D
#include "json/stringbuffer.h"
#include "json/writer.h"
#include "json/document.h"
#endif

#ifdef FUNAPI_UE4
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#endif

#ifdef FUNAPI_UE4_PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif

#ifdef FUNAPI_UE4
DECLARE_LOG_CATEGORY_EXTERN(LogFunapi, Log, All);
#endif

#endif  // SRC_FUNAPI_PLUGIN_H_
