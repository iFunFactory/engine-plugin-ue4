// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_PLUGIN_H_
#define SRC_FUNAPI_PLUGIN_H_

#ifndef FUNAPI_UE4
#include "funapi_build_config.h"
#include "cocos2d.h"
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
#ifndef FUNAPI_UE4_PLATFORM_PS4
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#endif // FUNAPI_UE4_PLATFORM_PS4
#endif // FUNAPI_PLATFORM_WINDOWS

#include <array>
#include <map>
#include <string>
#include <vector>
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
#include <unordered_set>
#include <unordered_map>
#include <assert.h>

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

#endif  // SRC_FUNAPI_PLUGIN_H_
