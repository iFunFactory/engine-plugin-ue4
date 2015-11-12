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
#include "funapi/funapi_network.h"
#include "funapi/test_messages.pb.h"
#include "Funapi/funapi_downloader.h"

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
}

void Afunapi_tester::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  Super::EndPlay(EndPlayReason);
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
    network_ = std::make_shared<fun::FunapiNetwork>(msg_type_,
      [this](const std::string &session_id){ OnSessionInitiated(session_id); },
      [this]{ OnSessionClosed(); });

    network_->RegisterHandler("echo", [this](const std::string &type, const std::vector<uint8_t> &v_body){ OnEchoJson(type, v_body); });
    network_->RegisterHandler("pbuf_echo", [this](const std::string &type, const std::vector<uint8_t> &v_body){ OnEchoProto(type, v_body); });

    network_->AttachTransport(GetNewTransport(protocol));
    network_->Start();
  }
  else {
    network_->AttachTransport(GetNewTransport(protocol));
    network_->Start();
  }

  protocol_ = protocol;
}

std::shared_ptr<fun::FunapiTransport> Afunapi_tester::GetNewTransport(fun::TransportProtocol protocol)
{
  std::shared_ptr<fun::FunapiTransport> transport;

  if (protocol == fun::TransportProtocol::kTcp)
    transport = std::make_shared<fun::FunapiTcpTransport>(kServerIp, static_cast<uint16_t>(msg_type_ == fun::kProtobufEncoding ? 8022 : 8012));
  else if (protocol == fun::TransportProtocol::kUdp)
    transport = std::make_shared<fun::FunapiUdpTransport>(kServerIp, static_cast<uint16_t>(msg_type_ == fun::kProtobufEncoding ? 8023 : 8013));
  else if (protocol == fun::TransportProtocol::kHttp)
    transport = std::make_shared<fun::FunapiHttpTransport>(kServerIp, static_cast<uint16_t>(msg_type_ == fun::kProtobufEncoding ? 8028 : 8018), false);

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
  fun::FunapiHttpDownloader downloader;
  downloader.ReadyCallback += [&downloader](int count, uint64 size) {
    downloader.StartDownload();
  };

  downloader.GetDownloadList("http://127.0.0.1:8020", "C:\\download_test");

  while (downloader.IsDownloading()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  return true;
}
