// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin_ue4.h"
#include "AutomationTest.h"
#include "funapi_session.h"
#include "test_messages.pb.h"

const std::string g_server_ip = "127.0.0.1";

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFunapiSessionTestEchoJson, "Funapi.Echo.Json", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFunapiSessionTestEchoJson::RunTest(const FString& Parameters)
{
  std::string send_string = "Json Echo Message";
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), false);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback(
    [&send_string](
      const std::shared_ptr<fun::FunapiSession> &s,
      const fun::TransportProtocol protocol,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::SessionEventType::kOpened) {
      TSharedRef<FJsonObject> json_object = MakeShareable(new FJsonObject);
      json_object->SetStringField(FString("message"), FString(send_string.c_str()));

      // Convert JSON document to string
      FString ouput_fstring;
      TSharedRef<TJsonWriter<TCHAR>> writer = TJsonWriterFactory<TCHAR>::Create(&ouput_fstring);
      FJsonSerializer::Serialize(json_object, writer);
      std::string json_stiring = TCHAR_TO_ANSI(*ouput_fstring);

      s->SendMessage("echo", json_stiring);
    }
  });

  session->AddTransportEventCallback(
    [&is_ok, &is_working](
      const std::shared_ptr<fun::FunapiSession> &s,
      const fun::TransportProtocol protocol,
      const fun::TransportEventType type,
      const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::TransportEventType::kConnectionFailed) {
      UE_LOG(LogFunapiExample, Error, TEXT("kConnectionFailed"));
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kConnectionTimedOut) {
      UE_LOG(LogFunapiExample, Error, TEXT("kConnectionTimedOut"));
      is_ok = false;
      is_working = false;
    }
  });

  session->AddJsonRecvCallback(
    [&is_working, &is_ok, &send_string](
      const std::shared_ptr<fun::FunapiSession> &s,
      const fun::TransportProtocol protocol,
      const std::string &msg_type, const std::string &json_string)
  {
    if (msg_type.compare("echo") == 0) {
      TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<>::Create(*FString(json_string.c_str()));
      TSharedPtr<FJsonObject> JsonObject;
      if (FJsonSerializer::Deserialize(Reader, JsonObject))
      {
        FString temp_fstring = JsonObject->GetStringField("message");

        if (send_string.compare(TCHAR_TO_ANSI(*temp_fstring)) == 0) {
          is_ok = true;
        }
        else {
          is_ok = false;
        }
      }
    }

    is_working = false;
  });

  session->Connect(fun::TransportProtocol::kTcp, 8012, fun::FunEncoding::kJson);

  while (is_working) {
    session->Update();
  }

  session->Close();

  return is_ok;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFunapiSessionTestEchoProtobuf, "Funapi.Echo.Protobuf", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFunapiSessionTestEchoProtobuf::RunTest(const FString& Parameters)
{
  std::string send_string = "Protobuf Echo Message";
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), false);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback(
    [&send_string](
      const std::shared_ptr<fun::FunapiSession> &s,
      const fun::TransportProtocol protocol,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::SessionEventType::kOpened) {
      // send
      FunMessage msg;
      msg.set_msgtype("pbuf_echo");
      PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
      echo->set_msg(send_string.c_str());
      s->SendMessage(msg);
    }
  });

  session->AddTransportEventCallback(
    [&is_ok, &is_working](
      const std::shared_ptr<fun::FunapiSession> &s,
      const fun::TransportProtocol protocol,
      const fun::TransportEventType type,
      const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::TransportEventType::kConnectionFailed) {
      UE_LOG(LogFunapiExample, Error, TEXT("kConnectionFailed"));
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kConnectionTimedOut) {
      UE_LOG(LogFunapiExample, Error, TEXT("kConnectionTimedOut"));
      is_ok = false;
      is_working = false;
    }
  });

  session->AddProtobufRecvCallback(
    [&is_working, &is_ok, &send_string](
      const std::shared_ptr<fun::FunapiSession> &session,
      const fun::TransportProtocol transport_protocol,
      const FunMessage &fun_message)
  {
    if (fun_message.msgtype().compare("pbuf_echo") == 0) {
      PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);

      if (send_string.compare(echo.msg()) == 0) {
        is_ok = true;
      }
      else {
        is_ok = false;
      }
    }
    else {
      is_ok = false;
    }

    is_working = false;
  });

  session->Connect(fun::TransportProtocol::kTcp, 8022, fun::FunEncoding::kProtobuf);

  while (is_working) {
    session->Update();
  }

  session->Close();

  return is_ok;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFunapiSessionTestRecvTimeout, "Funapi.RecvTimeout", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFunapiSessionTestRecvTimeout::RunTest(const FString& Parameters)
{
  std::string send_string;
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), false);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback(
    [](
      const std::shared_ptr<fun::FunapiSession> &s,
      const fun::TransportProtocol protocol,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::SessionEventType::kOpened) {
      UE_LOG(LogFunapiExample, Log, TEXT("SessionEventType::kOpened - %s"), *FString(session_id.c_str()));
      s->SetRecvTimeout("echo", 2);
    }
  });

  session->AddTransportEventCallback(
    [&is_ok, &is_working](
      const std::shared_ptr<fun::FunapiSession> &s,
      const fun::TransportProtocol protocol,
      const fun::TransportEventType type,
      const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::TransportEventType::kConnectionFailed) {
      UE_LOG(LogFunapiExample, Error, TEXT("kConnectionFailed"));
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kConnectionTimedOut) {
      UE_LOG(LogFunapiExample, Error, TEXT("kConnectionTimedOut"));
      is_ok = false;
      is_working = false;
    }
  });

  session->AddRecvTimeoutCallback(
    [&is_working, &is_ok](
      const std::shared_ptr<fun::FunapiSession> &s,
      const std::string &msg_type)
  {
    is_ok = true;
    is_working = false;
  });

  session->Connect(fun::TransportProtocol::kTcp, 8012, fun::FunEncoding::kJson);

  while (is_working) {
    session->Update();
  }

  session->Close();

  return is_ok;
}
