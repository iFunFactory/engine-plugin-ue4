// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_BUILD_CONFIG_H_
#define SRC_FUNAPI_BUILD_CONFIG_H_

// #define FUNAPI_COCOS2D
#define FUNAPI_UE4

#ifdef FUNAPI_UE4
  #if PLATFORM_WINDOWS
  #define FUNAPI_PLATFORM_WINDOWS
  #define FUNAPI_UE4_PLATFORM_WINDOWS
  #endif
#endif

#ifdef FUNAPI_COCOS2D
#include "cocos2d.h"
  #if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
  #define FUNAPI_PLATFORM_WINDOWS
  #define FUNAPI_COCOS2D_PLATFORM_WINDOWS
  #endif
#endif

#define DEBUG_LOG

#ifdef FUNAPI_PLATFORM_WINDOWS
#define NOMINMAX
#endif

#endif  // SRC_FUNAPI_BUILD_CONFIG_H_
