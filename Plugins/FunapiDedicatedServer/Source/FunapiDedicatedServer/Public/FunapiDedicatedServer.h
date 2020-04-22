// Copyright (C) 2013-2020 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#pragma once

#include "FunapiDedicatedServerPrivatePCH.h"

namespace fun {
  namespace FunapiDedicatedServer {
    struct HttpResponse {
      int64_t code;
      FString desc;
    };

    typedef TFunction<void(const HttpResponse &response)> ResponseHandler;
    typedef TFunction<void()> StartedHandler;
    typedef TFunction<void()> DisconnectedHandler;
    typedef TFunction<void(const FString &uid, const FString &json_string)> UserDataHandler;
    typedef TFunction<void(const FString &json_string)> MatchDataHandler;

    extern FUNAPIDEDICATEDSERVER_API FString GetUserDataJsonString(const FString &uid);
    extern FUNAPIDEDICATEDSERVER_API FString GetMatchDataJsonString();
    extern FUNAPIDEDICATEDSERVER_API TArray<FString> GetUserConsoleCommandOptions();
    extern FUNAPIDEDICATEDSERVER_API void SetUserDataCallback(const UserDataHandler &user_data_handler);
    extern FUNAPIDEDICATEDSERVER_API void SetStartedCallback(const StartedHandler &started_handler);
    extern FUNAPIDEDICATEDSERVER_API void SetDisconnectedCallback(const DisconnectedHandler &disconnected_handler);
    extern FUNAPIDEDICATEDSERVER_API void SetMatchDataCallback(const MatchDataHandler &match_data_handler);

    extern FUNAPIDEDICATEDSERVER_API void PostReady();
    extern FUNAPIDEDICATEDSERVER_API void PostResult(const FString &json_string, const bool use_exit);
    extern FUNAPIDEDICATEDSERVER_API void PostGameState(const FString &json_string);
    extern FUNAPIDEDICATEDSERVER_API void PostJoined(const FString &uid);
    extern FUNAPIDEDICATEDSERVER_API void PostLeft(const FString &uid);
    extern FUNAPIDEDICATEDSERVER_API bool AuthUser(const FString& options, const FString& uid_field, const FString& token_field, FString &error_message);
    extern FUNAPIDEDICATEDSERVER_API bool AuthUser(const FString& options, FString &error_message);
    extern FUNAPIDEDICATEDSERVER_API void PostCustomCallback(const FString &json_string);

    extern FUNAPIDEDICATEDSERVER_API void Start(const FString &version_string);
    extern FUNAPIDEDICATEDSERVER_API void SendReady(const ResponseHandler &response_handler);
    extern FUNAPIDEDICATEDSERVER_API void SendResult(const FString &json_string, const ResponseHandler &response_handler);
    extern FUNAPIDEDICATEDSERVER_API void SendGameState(const FString &json_string, const ResponseHandler &response_handler);
    extern FUNAPIDEDICATEDSERVER_API void SendJoined(const FString &uid, const ResponseHandler &response_handler);
    extern FUNAPIDEDICATEDSERVER_API void SendLeft(const FString &uid, const ResponseHandler &response_handler);
    extern FUNAPIDEDICATEDSERVER_API void SendCustomCallback(const FString &json_string, const ResponseHandler &response_handler);
    extern FUNAPIDEDICATEDSERVER_API bool AuthUser(const FString &uid, const FString &token, FString &error_message);
  }
}
