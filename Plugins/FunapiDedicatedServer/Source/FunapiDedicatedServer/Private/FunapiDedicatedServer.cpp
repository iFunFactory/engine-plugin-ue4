// Copyright (C) 2013-2018 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "FunapiDedicatedServerPrivatePCH.h"
#include "FunapiDedicatedServer.h"
#include "Regex.h"

namespace fun
{
  namespace FunapiDedicatedServer
  {
    typedef TFunction<void(const TSharedPtr<FJsonObject> &response_data)> ResponseDataHandler;

    static FString funapi_manager_server_;
    static FString funapi_manager_server_with_match_id_;
    static TArray<FString> user_command_options_;
    static TMap<FString, FString> auth_map_;
    static TMap<FString, FString> user_data_map_;
    static FString match_data_string_;
    static double heartbeat_seconds_ = 0;
    static double pending_users_seconds_ = 5;
    static UserDataHandler user_data_handler_;
    static MatchDataHandler match_data_handler_;
    static StartedHandler started_handler_;
    static DisconnectedHandler disconnected_handler_;
    static FString version_info_string_;
    static bool use_send_version_and_exit_ = false;
    static bool active_ = false;
    static int heartbeat_retry_count_ = 0;

    const int kHeartbeatRetryThreshold = 2;
    const FString kMatchIdField("FunapiMatchID");
    const FString kManagerServerField("FunapiManagerServer");
    const FString kHeartBeatField("FunapiHeartbeat");
    const FString kVersionField("FunapiVersion");
    const FString kPortField("port");

    // 함수 전방 선언
    bool ParseConsoleCommand(const TCHAR *cmd);
    bool ParseConsoleCommand(const TCHAR* cmd, const FString &match_id_field, const FString &manager_server_field);
    bool ParseConsoleCommand(const TCHAR* cmd, const FString &match_id_field, const FString &manager_server_field, const FString &heartbeat_field);
    void ParseUserConsoleCommand(const TCHAR *cmd);
    void SendHeartbeat();
    void Send(const FString &path, const FString &json_string, const ResponseHandler &response_handler, const ResponseDataHandler &data_handler, const bool use_match_id);
    void SetVersionInfo(const FString &version_string);


    FString GetUserDataJsonString(const FString &uid)
    {
      FString* user_data_json_string = user_data_map_.Find(uid);
      if (user_data_json_string) {
        return FString(*user_data_json_string);
      }

      return "";
    }

    FString GetMatchDataJsonString()
    {
      return match_data_string_;
    }

    TArray<FString> GetUserConsoleCommandOptions() {
      return user_command_options_;
    }

    void SetUserDataCallback(const UserDataHandler &user_data_handler)
    {
      user_data_handler_ = user_data_handler;
    }

    void SetMatchDataCallback(const MatchDataHandler &match_data_handler)
    {
      match_data_handler_ = match_data_handler;
    }

    void SetStartedCallback(const StartedHandler &started_handler)
    {
      started_handler_ = started_handler;
    }

    void SetDisconnectedCallback(const DisconnectedHandler &disconnected_handler)
    {
      disconnected_handler_ = disconnected_handler;
    }

    void SetMatchDataThenCallMathDataHandler(const FString &data_string)
    {
      if (data_string.IsEmpty())
      {
        UE_LOG(LogFunapiDedicatedServer, Error, TEXT("The 'match_data' in the HTTP response message is empty."));
      }
      else
      {
        if (match_data_string_ != data_string)
        {
          match_data_string_ = data_string;
          match_data_handler_(data_string);
        }
      }
    }

    void AddData(const TSharedPtr<FJsonObject> json_object)
    {
      // Data json object 는 다음과 같은 형태를 가집니다.
      // "data": {
      //   ""
      //   "match_data": [ { ... } ], 또는 "match_data" : {""}
      //   "users": [
      //     {
      //       "token": "",
      //       "uid" : ""
      //     }
      //   ],
      //   "user_data": [
      //     {
      //        ...
      //     }
      //   ]
      //},

      if (json_object->HasField(FString("match_data")))
      {
        TSharedPtr<FJsonValue> match_data_json_value = json_object->GetField<EJson::None>(FString("match_data"));
        if (match_data_json_value->Type == EJson::Object)
        {
          TSharedPtr<FJsonObject> const_match_json_obj = match_data_json_value->AsObject();

          FString data_string;
          TSharedRef<TJsonWriter<TCHAR>> writer = TJsonWriterFactory<TCHAR>::Create(&data_string);
          FJsonSerializer::Serialize(const_match_json_obj.ToSharedRef(), writer);
          SetMatchDataThenCallMathDataHandler(data_string);
        }
        else if(match_data_json_value->Type == EJson::Array)
        {
          TArray<TSharedPtr<FJsonValue>> match_datas  = match_data_json_value->AsArray();
          int match_data_count = match_datas.Num();
          for (int i = 0; i < match_data_count; ++i)
          {
            TSharedPtr<FJsonObject> match_json_obj = match_datas[i]->AsObject();

            FString data_string;
            TSharedRef<TJsonWriter<TCHAR>> writer = TJsonWriterFactory<TCHAR>::Create(&data_string);
            FJsonSerializer::Serialize(match_json_obj.ToSharedRef(), writer);
            SetMatchDataThenCallMathDataHandler(data_string);
          } // match_data iterator
        }
        else // match data type is invalid
        {

          // 매치 정보의 파싱이 실패한 경우 호출된다.
          // 매치 정보를 출력 후 다른 정보들을 저장하기 위해 계속 진행된다.
          FString ouput_fstring;
          TSharedRef<TJsonWriter<TCHAR>> writer = TJsonWriterFactory<TCHAR>::Create(&ouput_fstring);
          FJsonSerializer::Serialize(json_object.ToSharedRef(), writer);

          UE_LOG(LogFunapiDedicatedServer, Error, TEXT("Invalid format of match information. Received message: %s"), *ouput_fstring);
        }
      } // end has match data

      TArray<TSharedPtr<FJsonValue>> users;
      int users_count = 0;

      if (json_object->HasField(FString("users")))
      {
        users = json_object->GetArrayField(FString("users"));
        users_count = users.Num();
      }

      TArray<TSharedPtr<FJsonValue>> user_data;
      int user_data_count = 0;

      if (json_object->HasField(FString("user_data")))
      {
        user_data = json_object->GetArrayField(FString("user_data"));
        user_data_count = user_data.Num();
      }

      for (int i = 0; i < users_count; ++i) {
        TSharedPtr<FJsonObject> o = users[i]->AsObject();

        FString uid;
        FString token;

        if (o->HasField(FString("uid"))) {
          uid = o->GetStringField(FString("uid"));
        }

        if (o->HasField(FString("token"))) {
          token = o->GetStringField(FString("token"));
        }

        UE_LOG(LogFunapiDedicatedServer, Log, TEXT("%s, %s"), *uid, *token);

        if (false == uid.IsEmpty() && false == token.IsEmpty()) {
          auth_map_.Add(uid, token);
        }

        if (i < user_data_count) {
          FString user_data_json_string;

          if (false == user_data[i]->TryGetString(user_data_json_string)) {
            TSharedPtr<FJsonObject> user_data_object = user_data[i]->AsObject();
            // Convert JSON document to string
            TSharedRef<TJsonWriter<TCHAR>> writer = TJsonWriterFactory<TCHAR>::Create(&user_data_json_string);
            FJsonSerializer::Serialize(user_data_object.ToSharedRef(), writer);
          }

          if (false == uid.IsEmpty() && false == user_data_json_string.IsEmpty()) {
            bool use_callback = true;
            if (user_data_map_.Contains(uid)) {
              if (user_data_map_[uid] == user_data_json_string) {
                use_callback = false;
              }
            }

            user_data_map_.Add(uid, user_data_json_string);

            if (use_callback) {
              user_data_handler_(uid, user_data_json_string);
            }
          }
        }
      }
    }

    void AsyncHeartbeat()
    {
      class FAsyncHeartbeatTask : public FNonAbandonableTask
      {
      public:
        /** Performs work on thread */
        void DoWork()
        {
          FPlatformProcess::Sleep(heartbeat_seconds_);
          SendHeartbeat();
        }

        /** Returns true if the task should be aborted.  Called from within the task processing code itself via delegate */
        bool ShouldAbort() const
        {
          return false;
        }

        TStatId GetStatId() const
        {
          return TStatId();
        }
      };

      (new FAutoDeleteAsyncTask<FAsyncHeartbeatTask>)->StartBackgroundTask();
    }

    void SendPendingUsers();
    void AsyncPendingUsers()
    {
      class FAsyncPendingUsersTask : public FNonAbandonableTask
      {
      public:
        /** Performs work on thread */
        void DoWork()
        {
          FPlatformProcess::Sleep(pending_users_seconds_);
          SendPendingUsers();
        }

        /** Returns true if the task should be aborted.  Called from within the task processing code itself via delegate */
        bool ShouldAbort() const
        {
          return false;
        }

        TStatId GetStatId() const
        {
          return TStatId();
        }
      };

      (new FAutoDeleteAsyncTask<FAsyncPendingUsersTask>)->StartBackgroundTask();
    }

    bool ParseConsoleCommand(const TCHAR* cmd, const FString &match_id_field, const FString &manager_server_field, const FString &heartbeat_field, const FString &version_field)
    {
      // 호스 매니저로 받은 명령줄 실행인자 값을 출력합니다.
      UE_LOG(LogFunapiDedicatedServer, Log, TEXT("%s"), cmd);

      if (FParse::Param(cmd, *version_field))
      {
        use_send_version_and_exit_ = true;
        UE_LOG(LogFunapiDedicatedServer, Log, TEXT("Need to send version imformation"));
      }
      else {
        use_send_version_and_exit_ = false;
      }


      if (ParseConsoleCommand(cmd, match_id_field, manager_server_field, heartbeat_field))
      {
        return true;
      }

      return false;
    }

    bool ParseConsoleCommand(const TCHAR* cmd, const FString &match_id_field, const FString &manager_server_field, const FString &heartbeat_field)
    {
      if (ParseConsoleCommand(cmd, match_id_field, manager_server_field))
      {
        int32 seconds = 0;

        FString heartbeat_field_string(heartbeat_field);
        heartbeat_field_string.Append("=");

        if (FParse::Value(cmd, *heartbeat_field_string, seconds))
        {
          // timeout은 플러그인 내부 규칙은 다음과 같다.
          // FunapiHeartbeat 는 10초 이상이여야하고 최대 60초를 넘을수 없다.
          // - 주기      : `FunapiHeartbeat`
          // - 타임 아웃 : `FunapiHeartbeat` / 2
          if (seconds >= 10 && seconds <= 60)
          {
            heartbeat_seconds_ = static_cast<double>(seconds);
            FHttpModule::Get().SetHttpTimeout(heartbeat_seconds_ / 2.0f);

            AsyncHeartbeat();
            return true;
          }
          else {
            UE_LOG(LogFunapiDedicatedServer, Error, TEXT("'heartbeat' value must be greater than zero and lesser than 60 seconds"));
          }
        }
      }

      return false;
    }

    bool ParseConsoleCommand(const TCHAR* cmd, const FString &match_id_field, const FString &manager_server_field)
    {
      bool ret = true;

      FString match_id;
      FString manager_server;
      FString server_port;

      if (FParse::Value(cmd, *FString(match_id_field), match_id))
      {
        match_id = match_id.Replace(TEXT("="), TEXT("")).Replace(TEXT("\""), TEXT("")); // replace quotes
        UE_LOG(LogFunapiDedicatedServer, Log, TEXT("match_id is '%s'"), *match_id);
      }
      else {
        ret = false;
      }

      if (FParse::Value(cmd, *FString(manager_server_field), manager_server))
      {
        manager_server = manager_server.Replace(TEXT("="), TEXT("")).Replace(TEXT("\""), TEXT("")); // replace quotes
        UE_LOG(LogFunapiDedicatedServer, Log, TEXT("manager_server is '%s'"), *manager_server);
      }
      else {
        ret = false;
      }

      if (FParse::Value(cmd, *kPortField, server_port))
      {
        server_port = server_port.Replace(TEXT("="), TEXT("")).Replace(TEXT("\""), TEXT(""));
        UE_LOG(LogFunapiDedicatedServer, Log, TEXT("Spawning a dedicated server: port=%s"), *server_port);
      }
      else {
        ret = false;
      }

      if (ret) {
        funapi_manager_server_ = manager_server + "/";
        funapi_manager_server_with_match_id_ = manager_server + "/match/" + match_id + "/";
        UE_LOG(LogFunapiDedicatedServer, Log, TEXT("Dedicated server manager server : %s"), *funapi_manager_server_with_match_id_);

        AsyncPendingUsers();
      }

      return ret;
    }

    bool ParseConsoleCommand(const TCHAR *cmd)
    {

      // 서버와 통신을 위한 초기화 진행 및 서버의 버전정보를 확인한다.
      if (ParseConsoleCommand(cmd, kMatchIdField, kManagerServerField, kHeartBeatField, kVersionField)) {
        // 사용자의 추가 옵션값들을 저장한다.
        ParseUserConsoleCommand(cmd);
        return true;
      }

      return false;
    }

    void ParseUserConsoleCommand(const TCHAR *cmd)
    {
      int32 user_options_count = user_command_options_.Num();
      if (user_options_count != 0) {
        UE_LOG(LogFunapiDedicatedServer, Warning, TEXT("User options have already been saved. The deleted the old user options and save the new user options."));
        user_command_options_.Empty();
      }

      int32 length = TCString<TCHAR>::Strlen(cmd);
      TCHAR* copyed_cmd = new TCHAR[length + 1];
      TCString<TCHAR>::Strcpy(copyed_cmd, length, cmd);

      FString delim(" ");
      TCHAR* context = NULL;

      FString token = TCString<TCHAR>::Strtok(copyed_cmd, *delim, &context);

      while (!token.IsEmpty()) {
        if (!token.Contains(kMatchIdField) &&
          !token.Contains(kManagerServerField) &&
          !token.Contains(kHeartBeatField) &&
          !token.Contains(kVersionField) &&
          !token.Contains(kPortField)) {
          user_command_options_.Add(token);
        }

        token = TCString<TCHAR>::Strtok(NULL, *delim, &context);
      }

      delete[] copyed_cmd;

      for (FString& user_option : user_command_options_)
      {
        UE_LOG(LogFunapiDedicatedServer, Log, TEXT("'%s' user options saved"), *user_option);
      }
    }

    bool ProcessHttpResponse(const TSharedPtr<FJsonObject>& json_object, HttpResponse &http_response)
    {
      // 호스트 매니저와의 통신은 다음과 같은 형태를 가집니다.
      // 성공시
      // {
      //   "error": {
      //     "desc" : "",
      //     "code" : 0
      //   },
      //   "data" : {
      //     // 기존 내용
      //   }
      // }

      // 실패시
      // {
      //   "error": {
      //     "desc" : "",
      //     "code" : 0
      //   }
      // }

      // 필수 field 가 있는지 확인한다.
      if (!json_object->HasField(FString("error")))
      {
        UE_LOG(LogFunapiDedicatedServer, Error, TEXT("HTTP response message does not have error field. Please check host manager."));
        http_response = { 22  /* Curl 22 = HTTP returned error */,
                          "HTTP response message does not have error field. Please check host manager." };
        return false;
      }

      const TSharedPtr<FJsonObject>& error_json_object = json_object->GetObjectField(FString("error"));
      if (!error_json_object->HasField(FString("desc")))
      {
        UE_LOG(LogFunapiDedicatedServer, Error, TEXT("HTTP response message does not have desc field. Please check host manager."));
        http_response = { 22  /* Curl 22 = HTTP returned error */,
                          "HTTP response message does not have desc field. Please check host manager." };
        return false;
      }

      if (!error_json_object->HasField(FString("code")))
      {
        UE_LOG(LogFunapiDedicatedServer, Error, TEXT("HTTP response message does not have code field. Please check host manager."));
        http_response = { 22  /* Curl 22 = HTTP returned error */,
                          "HTTP response message does not have code field. Please check host manager." };
        return false;
      }


      // 코드 정보의 규칙은 다음과 같습니다. 0(성공) 나머지(실패)
      int code = error_json_object->GetIntegerField("code");
      if (code == 0)
      {
        if (!json_object->HasField(FString("data")))
        {
          UE_LOG(LogFunapiDedicatedServer, Error, TEXT("HTTP response message does not have data field. Please check host manager."));
          http_response = { 22  /* Curl 22 = HTTP returned error */,
                            "HTTP response message does not have data field. Please check host manager." };
          return false;
        }

        // 응답 메세지의 형태가 올바르고 응답메세지 내용이 성공일때 실행됩니다.
        http_response = { code, "" };
        return true;
      }
      else
      {
        FString error = *error_json_object->GetStringField(FString("desc"));
        UE_LOG(LogFunapiDedicatedServer, Error, TEXT("Error Code : %d, Error Message : %s"), code, *error)
          http_response = { code, error };
        return true;
      }
    }

    bool ProcessOldHttpResponse(const TSharedPtr<FJsonObject>& json_object, HttpResponse &http_response)
    {
      // 호스트 매니저와의 통신은 다음과 같은 형태를 가집니다.
      // 성공시
      //{
      //  "status" : "ok or OK"
      //  "key1" : value
      //  "key2" : value
      //   ...
      //}

      // 실패시
      //{
      //  "status" : "error"
      //  "error" : error message
      //}

      if (!json_object->HasField("status")) {
        UE_LOG(LogFunapiDedicatedServer, Error, TEXT("HTTP response message does not have status field. Please check host manager."));
        http_response = { 22  /* Curl 22 = HTTP returned error */,
                          "HTTP response message does not have status field. Please check host manager." };
        return false;
      }

      // 코드 정보의 규칙은 다음과 같습니다. 0(성공) 나머지(실패)
      int code = 0;
      FString status = json_object->GetStringField(FString("status"));
      if (status.Compare("ok", ESearchCase::IgnoreCase) == 0)
      {
        http_response = { code, "" };
        return true;
      }
      else
      {
        if (!json_object->HasField(FString("error"))) {
          UE_LOG(LogFunapiDedicatedServer, Error, TEXT("HTTP response message does not have error field. Please check host manager."));
          http_response = { 22  /* Curl 22 = Http returned error */,
                          "HTTP response message does not have error field. Please check host manager." };
          return false;
        }
        http_response = { code, json_object->GetStringField(FString("error")) };
        return true;
      }
    }

    void RequestHttp(
      const FString &verb,
      const FString &path,
      const FString &json_string,
      const ResponseHandler &response_handler,
      const ResponseDataHandler &response_data_handler,
      const bool use_match_id = true) {

      // 데디케이티드 서버 플러그인의 초기화전에 호출되면 호스트 매니저의 주소를 찾을 수 없다.
      // 호스트 매니저의 초기화 과정에서 확인되고 문제가 있다면 종료 처리를 한다.
      if (funapi_manager_server_.IsEmpty()) {
        UE_LOG(LogFunapiDedicatedServer, Error,
          TEXT("FunapiManagerServer option is not initialized. Please check the host manager."));
        FGenericPlatformMisc::RequestExit(false);
        return;
      }

      // 호스트 매니저와 접속 상태를 나타냅니다.
      if (!active_) {
        // 사용자에게 후처리를 맡겨놓은 상태입니다.
        return;
      }

      // 응답 핸들러는 항상 존재해야 한다.
      if (!response_handler) {
        UE_LOG(LogFunapiDedicatedServer, Warning, TEXT("The HTTP request is aborted. Please check if the callback is registered: request='%s'"), *path);
        return;
      }

      FString server_url;
      if (use_match_id) {
        server_url = funapi_manager_server_with_match_id_ + path;
      }
      else {
        server_url = funapi_manager_server_ + path;
      }

      auto http_request = FHttpModule::Get().CreateRequest();
      http_request->SetURL(server_url);
      http_request->SetVerb(verb);
      http_request->SetHeader(FString("Content-Type"), FString("application/json; charset=utf-8"));
      http_request->SetHeader(FString("Content-Length"), FString::FromInt(json_string.Len()));
      http_request->SetContentAsString(json_string);

      http_request->OnProcessRequestComplete().BindLambda(
          [verb, path, response_handler, response_data_handler](FHttpRequestPtr request, FHttpResponsePtr response, bool succeed) {

            // 콜백으로 넘겨줄 정보.
            HttpResponse processed_response;
            if (!succeed)
            {
              // 내부통신으로 실패가 발생하지 않는다.
              // 만약 실패가 발생한다면 호스트 매니저에서 문제가 발생한것으로
              // heart beat 에서 호스트 매니저와의 연결을 확인 후 사용자에게 Disconnected Callback을 호출합니다.
              FString error_log = FString::Printf(TEXT("%s %s :  Failed to get a response from HostManager."), *verb, *path);
              UE_LOG(LogFunapiDedicatedServer, Error, TEXT("%s"), *error_log);
              // 로그 메세지 수정
              processed_response = { 28  /* Curl 28 = Operation timeout */,
                                     error_log };
              response_handler(processed_response);
              return;
            }

            FString json_fstring;
            json_fstring = response->GetContentAsString();
            UE_LOG(LogFunapiDedicatedServer, Log, TEXT("%s %s : Response = %s"), *verb, *path, *json_fstring);

            TSharedRef< TJsonReader<> > reader = TJsonReaderFactory<>::Create(json_fstring);
            TSharedPtr<FJsonObject> json_object;

            if (!FJsonSerializer::Deserialize(reader, json_object))
            {
              FString error_log = FString::Printf(TEXT("Failed to parse the response message. Please check host manager. Requset : %s, %s"), *verb, *path);
              UE_LOG(LogFunapiDedicatedServer, Error, TEXT("%s"), *error_log);
              processed_response = { 22  /* Curl 22 = Http returned error */,
                                     error_log };
              response_handler(processed_response);
              return;
            }


            bool old_http_response = json_object->HasField(FString("status"));
            // 호스트 매니저 응답의 호환성을 갖춥니다.
            if (old_http_response)
            {
              if (ProcessOldHttpResponse(json_object, processed_response)) {
                if (response_data_handler) {
                  response_data_handler(json_object);
                }
              }
              else {
                UE_LOG(LogFunapiDedicatedServer, Error, TEXT("The message format is invalied. Please check host manager. Requset : %s, %s"), *verb, *path);
              }
              response_handler(processed_response);
            }
            else
            {
              if (ProcessHttpResponse(json_object, processed_response)) {
                if (response_data_handler) {
                  response_data_handler(json_object->GetObjectField(FString("data")));
                }
              }
              else {
                UE_LOG(LogFunapiDedicatedServer, Error, TEXT("The message format is invalied. Please check host manager. Requset : %s, %s"), *verb, *path);
              }
              response_handler(processed_response);
            } // else /* old_http_response */
          }); // bind lambda
      http_request->ProcessRequest();
    }

    void Get(const FString &path, const FString &json_string, const ResponseHandler &response_handler, const ResponseDataHandler &data_handler)
    {
      if (use_send_version_and_exit_) {
        return;
      }

      RequestHttp("GET", path, json_string, response_handler, data_handler);
    }

    void GetGameInfo(const ResponseHandler &response_handler) {
      const ResponseDataHandler data_handler =
        [](const TSharedPtr<FJsonObject> &response_data)
      {
        auth_map_.Empty();

        TSharedPtr<FJsonObject> data = response_data->GetObjectField(FString("data"));
        AddData(data);
      };

      Get("", "", response_handler, data_handler);
    }

    void Start(const FString &version_string)
    {
      const ResponseHandler handler =
        [](HttpResponse http_response)
      {
        if (http_response.code != 0) {
          UE_LOG(LogFunapiDedicatedServer, Error, TEXT("Failed to get game info. Please check the host manager."));
          FGenericPlatformMisc::RequestExit(false);
          return;
        }

        if (started_handler_) {
          started_handler_();
        }
      };

      active_ = true;

      // 명령줄 인자 값을 확인한다. 필수 명령줄 인자값이 존재하지 않는다면 프로그램을 종료한다.
      if (!ParseConsoleCommand(FCommandLine::Get())) {
        UE_LOG(LogFunapiDedicatedServer, Error, TEXT("Failed to parse console command. Please check the host manager."));
        FGenericPlatformMisc::RequestExit(false);
        return;
      }

      SetVersionInfo(version_string);
      GetGameInfo(handler);
    }

    DEPRECATED(1.14 /* plugin version */, "This function is deprecated. Please use FunapiDedicatedServer::SendReady function")
      void PostReady()
    {
      UE_LOG(LogFunapiDedicatedServer, Warning, TEXT("This function is deprecated. Please use FunapiDedicatedServer::SendReady function"));
      Send("ready", "", [](HttpResponse http_response) {}, nullptr, true);
    }

    DEPRECATED(1.14 /* plugin version */, "This function is deprecated. Please use FunapiDedicatedServer::SendResult function")
      void PostResult(const FString &json_string, const bool use_exit)
    {
      UE_LOG(LogFunapiDedicatedServer, Warning, TEXT("This function is deprecated. Please use FunapiDedicatedServer::SendResult function"));
      Send("result", json_string,
        [use_exit](HttpResponse http_response)
      {
        // 결과에 대한 콜백이 존재 하지 않기 때문에 실패 성공여부에 상관없이 종료합니다.
        if (use_exit) {
          FGenericPlatformMisc::RequestExit(false);
        }
      },
        nullptr,
        true);
    }

    void SendVersion()
    {
      // 호스트 매니저의 초기화 단계에서만 호출됩니다.
      // version_info_string_ 는 항상 Json 형태를 유지 합니다.
      if (use_send_version_and_exit_ && version_info_string_.IsEmpty() == false) {
        Send("server/version", version_info_string_,
          [](HttpResponse http_response)
        {
          if (http_response.code == 0) {
            // 호스트 매니저에서 초기화 확인 후 데디케이티드 서버를 종료시키기 때문에 호출되지 않을 수 있습니다.
            UE_LOG(LogFunapiDedicatedServer, Log, TEXT("Succeed to verify the dedicated server version: %s"), *version_info_string_);
            FGenericPlatformMisc::RequestExit(false);
            return;
          }

          // SendVersion 는 내부통신으로 실패가 발생하지 않는다.
          // 만약 실패가 발생한다면 호스트 매니저에 문제가 생긴것으로
          // 호스트 매니저를 데디케이티드 서버가 조작할 수 없기 떄문에 종료처리한다.
          UE_LOG(LogFunapiDedicatedServer, Error, TEXT("Failed to check a dedicated server version. Please check the host manager."));
          FGenericPlatformMisc::RequestExit(false);
        },
          nullptr,
          false);
      }
    }

    void SendHeartbeat()
    {
      if (!active_)
      {
        return;
      }

      Send("heartbeat", "",
        [](HttpResponse http_response)
      {

        bool succeed = (http_response.code == 0);
        heartbeat_retry_count_ = succeed ? 0 : heartbeat_retry_count_ + 1;

        if (!succeed) {
          // heartbeat 는 내부통신으로 실패가 발생하지 않는다.
          // 만약 실패가 발생한다면 호스트 매니저에 문제가 생긴것으로
          // disconnected_handler_ 를 호출하여 사용자에게 추후 처리를 맡긴다.
          if (heartbeat_retry_count_ > kHeartbeatRetryThreshold)
          {
            // Disconnect handler 가 호출은 단 한번만 실행이 된다.
            if (disconnected_handler_ && active_ == true) {
              // 개발자는 이 시점에서 사용자를 내보내고 프로세스를 종료해야 합니다.
              disconnected_handler_();
            }

            // 더 이상 호스트 매니저와의 통신이 진행되지 않습니다.
            // 호스트 매니저와 다시 연결 가능한 상태가 되어도 이 데디케이티드 서버 데이터를 업데이트할 수 없습니다.
            active_ = false;
            return;
          }
        }
        AsyncHeartbeat();
      },
        nullptr,
        true);
    }

    DEPRECATED(1.14 /* plugin version */, "This function is deprecated. Please use FunapiDedicatedServer::SendGameState function")
      void PostGameState(const FString &json_string)
    {
      UE_LOG(LogFunapiDedicatedServer, Warning, TEXT("This function is deprecated. Please use FunapiDedicatedServer::SendGameState function"));
      Send("state", json_string, [](HttpResponse http_response) {}, nullptr, true);
    }

    void SendPendingUsers()
    {
      if (!active_)
      {
        return;
      }

      Send("pending_users", "",
        [](HttpResponse http_response)
      {
        if (http_response.code != 0)
        {
          UE_LOG(LogFunapiDedicatedServer, Error, TEXT("Failed to send pending users to HostManager"));
        }

        // 실패, 성공 여부에 상관없이 지속적으로 업데이트 됩니다.
        AsyncPendingUsers();
      },
        // 성공시에만 호출이 됩니다.
        [](TSharedPtr<FJsonObject> response_data)
      {
        AddData(response_data);
      },
        true);
    }

    FString JsonFStringWithUID(const FString &uid) {
      TSharedRef<FJsonObject> json_object = MakeShareable(new FJsonObject);
      json_object->SetStringField(FString("uid"), uid);

      // Convert JSON document to string
      FString ouput_fstring;
      TSharedRef<TJsonWriter<TCHAR>> writer = TJsonWriterFactory<TCHAR>::Create(&ouput_fstring);
      FJsonSerializer::Serialize(json_object, writer);

      // UE_LOG(LogFunapiDedicatedServer, Log, TEXT("JsonFStringWithUID = %s"), *ouput_fstring);

      return ouput_fstring;
    }

    DEPRECATED(1.14 /* plugin version */, "This function is deprecated. Please use FunapiDedicatedServer::SendJoined function")
      void PostJoined(const FString &uid)
    {
      UE_LOG(LogFunapiDedicatedServer, Warning, TEXT("This function is deprecated. Please use FunapiDedicatedServer::SendJoined function"));
      Send("joined", JsonFStringWithUID(uid), [](HttpResponse http_response) {}, nullptr, true);
    }

    DEPRECATED(1.14 /* plugin version */, "This function is deprecated. Please use FunapiDedicatedServer::SendLeft function")
      void PostLeft(const FString &uid)
    {
      UE_LOG(LogFunapiDedicatedServer, Warning, TEXT("This function is deprecated. Please use FunapiDedicatedServer::SendLeft function"));
      Send("left", JsonFStringWithUID(uid), [](HttpResponse http_response) {}, nullptr, true);
    }

    bool AuthUser(const FString &uid, const FString &token, FString &error_message)
    {
      if (funapi_manager_server_.IsEmpty()) {
        return true;
      }

      if (uid.Len() <= 0)
      {
        error_message = FString("User id does not set.");
        UE_LOG(LogFunapiDedicatedServer, Error, TEXT("[Auth] ERROR: %s"), *error_message);
        return false;
      }
      else {
        UE_LOG(LogFunapiDedicatedServer, Log, TEXT("[Auth] uid: %s"), *uid);
      }

      if (token.Len() <= 0)
      {
        error_message = FString("User token does not set.");
        UE_LOG(LogFunapiDedicatedServer, Error, TEXT("[Auth] ERROR: %s"), *error_message);
        return false;
      }
      else {
        UE_LOG(LogFunapiDedicatedServer, Log, TEXT("[Auth] token: %s"), *token);
      }

      bool ret = false;
      FString* t = auth_map_.Find(uid);
      if (t) {
        if (token.Compare(*t) == 0) {
          ret = true;
        }
      }

      if (ret == false) {
        error_message = FString("There is no valid user id & token in this dedicated server.");
        UE_LOG(LogFunapiDedicatedServer, Error, TEXT("[Auth] ERROR: %s"), *error_message);
      }

      return ret;
    }

    DEPRECATED(1.14 /* plugin version */, "This function is deprecated. Please use FunapiDedicatedServer::AuthUser(uid, token) function")
      bool AuthUser(const FString& options, FString &error_message)
    {
      return FunapiDedicatedServer::AuthUser(options, "FunapiUID", "FunapiToken", error_message);
    }

    DEPRECATED(1.14 /* plugin version */, "This function is deprecated. Please use FunapiDedicatedServer::AuthUser(uid, token) function")
      bool AuthUser(const FString& options, const FString& uid_field, const FString& token_field, FString &error_message)
    {
      UE_LOG(LogFunapiDedicatedServer, Warning, TEXT("This function is deprecated. Please use FunapiDedicatedServer::AuthUser(uid, token) function"));

      if (funapi_manager_server_.IsEmpty())
      {
        return true;
      }

      FString uid = UGameplayStatics::ParseOption(options, *uid_field);
      if (uid.Len() <= 0)
      {
        error_message = FString("User id does not set.");
        UE_LOG(LogFunapiDedicatedServer, Error, TEXT("[Auth] ERROR: %s"), *error_message);
        return false;
      }
      else
      {
        UE_LOG(LogFunapiDedicatedServer, Log, TEXT("[Auth] uid: %s"), *uid);
      }

      FString token = UGameplayStatics::ParseOption(options, token_field);
      if (token.Len() <= 0)
      {
        error_message = FString("User token does not set.");
        UE_LOG(LogFunapiDedicatedServer, Error, TEXT("[Auth] ERROR: %s"), *error_message);
        return false;
      }
      else
      {
        UE_LOG(LogFunapiDedicatedServer, Log, TEXT("[Auth] token: %s"), *token);
      }

      bool ret = false;
      FString* t = auth_map_.Find(uid);
      if (t) {
        if (token.Compare(*t) == 0) {
          ret = true;
        }
      }

      if (ret == false) {
        error_message = FString("There is no valid user id & token in this dedicated server.");
        UE_LOG(LogFunapiDedicatedServer, Error, TEXT("[Auth] ERROR: %s"), *error_message);
      }

      return ret;
    }

    DEPRECATED(1.14 /* plugin version */, "This function is deprecated. Please use FunapiDedicatedServer::SendCustomCallback function")
      void PostCustomCallback(const FString &json_string)
    {
      UE_LOG(LogFunapiDedicatedServer, Warning, TEXT("This function is deprecated. Please use FunapiDedicatedServer::SendCustomCallback function"));
      Send("callback", json_string, [](HttpResponse http_response) {}, nullptr, true);
    }

    bool CheckServerVersion(const FString &version_in) {
      // 데디케이티드 버전 형식은 'x.x.x.x' 이다. 여기서 'x' 는 Number Field 를 의미한다.
      const FRegexPattern regex_pattern(TEXT("^[0-9]+[.][0-9]+[.][0-9]+[.][0-9]+$"));
      FRegexMatcher regex_matcher(regex_pattern, version_in);
      if (regex_matcher.FindNext()) {
        return true;
      }
      return false;
    }

    void SetVersionInfo(const FString &version_string)
    {
      // 버전 정보의 형식이 올바른지 확인한다.
      if (!CheckServerVersion(version_string))
      {
        UE_LOG(LogFunapiDedicatedServer, Error, TEXT("The dedicated server version must be `x.y.z.r` format."));
        FGenericPlatformMisc::RequestExit(false);
        return;
      }

      TSharedRef<FJsonObject> json_object = MakeShareable(new FJsonObject);
      json_object->SetStringField(FString("version"), version_string);
      TSharedRef<TJsonWriter<TCHAR>> writer = TJsonWriterFactory<TCHAR>::Create(&version_info_string_);
      FJsonSerializer::Serialize(json_object, writer);

      if (use_send_version_and_exit_) {
        SendVersion();
      }
    }

    void Send(const FString &path, const FString &json_string, const ResponseHandler &response_handler, const ResponseDataHandler &data_handler, const bool use_match_id)
    {
      if (use_send_version_and_exit_ && use_match_id) {
        return;
      }

      RequestHttp("POST", path, json_string, response_handler, data_handler, use_match_id);
    }

    void SendReady(const ResponseHandler &response_handler)
    {
      Send("ready", "", response_handler, nullptr, true);
    }

    void SendResult(const FString &json_string, const ResponseHandler &response_handler)
    {
      Send("result", json_string, response_handler, nullptr, true);
    }

    void SendGameState(const FString &json_string, const ResponseHandler &response_handler)
    {
      Send("state", json_string, response_handler, nullptr, true);
    }

    void SendJoined(const FString &uid, const ResponseHandler &response_handler)
    {
      Send("joined", JsonFStringWithUID(uid), response_handler, nullptr, true);
    }

    void SendLeft(const FString &uid, const ResponseHandler &response_handler)
    {
      Send("left", JsonFStringWithUID(uid), response_handler, nullptr, true);
    }

    void SendCustomCallback(const FString &json_string, const ResponseHandler &response_handler)
    {
      Send("callback", json_string, response_handler, nullptr, true);
    }
  }
}
