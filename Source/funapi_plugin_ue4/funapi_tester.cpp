// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin_ue4.h"

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif

#include <functional>

#include "funapi_tester.h"
#include "funapi/FunapiNetwork.h"
#include "funapi/test_messages.pb.h"
// #include "Funapi/FunapiDownloader.h"

#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif




// Sets default values
Afunapi_tester::Afunapi_tester()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;
}

Afunapi_tester::~Afunapi_tester()
{
}

// Called when the game starts or when spawned
void Afunapi_tester::BeginPlay()
{
    Super::BeginPlay();

    // FOR TEST ////////////////////////////////////////////////////
    // downloader = new Fun::FunapiDownloader();
}

void Afunapi_tester::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  Super::EndPlay(EndPlayReason);

  if (network_) {
    delete network_;
    network_ = nullptr;
  }
}

void Afunapi_tester::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);

  // LOG1("Tick - %f", DeltaTime);

  if (network_)
  {
    network_->Update();
  }
}

bool Afunapi_tester::ConnectTcp()
{
  Connect(fun::TransportProtocol::kTcp);
  return true;
}

bool Afunapi_tester::ConnectUdp()
{
  Connect(fun::TransportProtocol::kUdp);
  return true;
}

bool Afunapi_tester::ConnectHttp()
{
  Connect(fun::TransportProtocol::kHttp);
  return true;
}

bool Afunapi_tester::IsConnected()
{
    return network_ != nullptr && network_->Connected();
}

void Afunapi_tester::Disconnect()
{
    if (network_ == nullptr || network_->Started() == false)
    {
        LOG("You should connect first.");
        return;
    }

    network_->Stop();
}

bool Afunapi_tester::SendEchoMessage()
{
    if (network_ == NULL || network_->Started() == false)
    {
        LOG("You should connect first.");
        return false;
    }

    if (msg_type_ == fun::kJsonEncoding)
    {
      FJsonObject json_object;
      json_object.SetStringField(FString("message"), FString("hello world"));
      network_->SendMessage("echo", json_object, protocol_);

      return true;
    }
    else if (msg_type_ == fun::kProtobufEncoding)
    {
        FunMessage msg;
        msg.set_msgtype("pbuf_echo");
        PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
        echo->set_msg("hello proto");
        network_->SendMessage(msg, protocol_);
        return true;
    }

    return false;
}

void Afunapi_tester::Connect(const fun::TransportProtocol protocol)
{
  if (!network_) {
    fun::FunapiTransport *transport = GetNewTransport(protocol);

    network_ = new fun::FunapiNetwork(transport, msg_type_,
      [this](const std::string &session_id){ OnSessionInitiated(session_id); },
      [this]{ OnSessionClosed(); });

    if (msg_type_ == fun::kJsonEncoding)
      network_->RegisterHandler("echo", [this](const std::string &type, const std::vector<uint8_t> &v_body){ OnEchoJson(type, v_body); });
    else if (msg_type_ == fun::kProtobufEncoding)
      network_->RegisterHandler("pbuf_echo", [this](const std::string &type, const std::vector<uint8_t> &v_body){ OnEchoProto(type, v_body); });

    network_->Start();
  }
  else {
    fun::FunapiTransport *transport = GetNewTransport(protocol);
    network_->AttachTransport(transport);
    network_->Start();
  }

  protocol_ = protocol;
}

fun::FunapiTransport* Afunapi_tester::GetNewTransport(fun::TransportProtocol protocol)
{
  fun::FunapiTransport *transport = nullptr;

  if (protocol == fun::TransportProtocol::kTcp)
    transport = new fun::FunapiTcpTransport(kServerIp, (uint16_t)(msg_type_ == fun::kProtobufEncoding ? 8022 : 8012));
  else if (protocol == fun::TransportProtocol::kUdp)
    transport = new fun::FunapiUdpTransport(kServerIp, (uint16_t)(msg_type_ == fun::kProtobufEncoding ? 8023 : 8013));
  else if (protocol == fun::TransportProtocol::kHttp)
    transport = new fun::FunapiHttpTransport(kServerIp, (uint16_t)(msg_type_ == fun::kProtobufEncoding ? 8028 : 8018), false);

  return transport;
}

void Afunapi_tester::OnSessionInitiated(const std::string &session_id)
{
  LOG1("session initiated: %s", *FString(session_id.c_str()));
}

void Afunapi_tester::OnSessionClosed()
{
  LOG("session closed");
}

void Afunapi_tester::OnEchoJson(const std::string &type, const std::vector<uint8_t> &v_body)
{
  std::string body(v_body.begin(), v_body.end());

  LOG1("msg '%s' arrived.", *FString(type.c_str()));
  LOG1("json: %s", *FString(body.c_str()));
}

void Afunapi_tester::OnEchoProto(const std::string &type, const std::vector<uint8_t> &v_body)
{
  LOG1("msg '%s' arrived.", *FString(type.c_str()));

  std::string body(v_body.begin(), v_body.end());

  FunMessage msg;
  msg.ParseFromString(body);
  PbufEchoMessage echo = msg.GetExtension(pbuf_echo);
  LOG1("proto: %s", *FString(echo.msg().c_str()));
}

bool Afunapi_tester::FileDownload()
{
    return true;
}