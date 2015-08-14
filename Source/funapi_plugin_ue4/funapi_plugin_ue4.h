// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#pragma once

#include "Engine.h"
#include "UnrealString.h"

#include <string>


namespace Fun
{
#define LOG(x) UE_LOG(LogClass, Log, TEXT(x));
#define LOG1(x, a1) UE_LOG(LogClass, Log, TEXT(x), a1);
#define LOG2(x, a1, a2) UE_LOG(LogClass, Log, TEXT(x), a1, a2);

typedef std::string string;

} // namespace Fun
