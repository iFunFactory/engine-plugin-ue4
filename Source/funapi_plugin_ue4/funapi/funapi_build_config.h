// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_BUILD_CONFIG_H_
#define SRC_FUNAPI_BUILD_CONFIG_H_

// #define FUNAPI_COCOS2D
#define FUNAPI_UE4
// #define FUNAPI_STINGRAY

#ifdef FUNAPI_UE4
  #ifdef PLATFORM_WINDOWS
  #define FUNAPI_PLATFORM_WINDOWS
  #define FUNAPI_UE4_PLATFORM_WINDOWS
  #endif
#endif

#ifdef FUNAPI_COCOS2D
  #if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
  #define FUNAPI_PLATFORM_WINDOWS
  #define FUNAPI_COCOS2D_PLATFORM_WINDOWS
  #endif
#endif

#endif  // SRC_FUNAPI_BUILD_CONFIG_H_
