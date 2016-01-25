// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin_ue4.h"

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif

// #include <functional>

// #include "funapi_tester.h"
// #include "funapi/funapi_network.h"
// #include "funapi/test_messages.pb.h"
// #include "Funapi/funapi_downloader.h"

#include "funapi/funapi_network.h"
#include "funapi/pb/test_messages.pb.h"
#include "funapi/pb/management/maintenance_message.pb.h"
#include "funapi/pb/service/multicast_message.pb.h"
#include "funapi/funapi_utils.h"

#include "funapi_tester.h"

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
  return network_ != nullptr && network_->IsConnected();
}

void Afunapi_tester::Disconnect()
{
  if (network_ == nullptr || network_->IsStarted() == false)
  {
    fun::DebugUtils::Log("You should connect first.");
    return;
  }

  network_->Stop();
}

bool Afunapi_tester::SendEchoMessage()
{
  if (network_ == nullptr || (network_->IsStarted() == false && network_->IsSessionReliability()))
  {
    fun::DebugUtils::Log("You should connect first.");
  }
  else {
    fun::FunEncoding encoding = network_->GetEncoding(network_->GetDefaultProtocol());
    if (encoding == fun::FunEncoding::kNone)
    {
      fun::DebugUtils::Log("You should attach transport first.");
      return false;
    }

    if (encoding == fun::FunEncoding::kProtobuf)
    {
      for (int i = 1; i < 100; ++i) {
        // std::to_string is not supported on android, using std::stringstream instead.
        std::stringstream ss_temp;
        ss_temp << "hello proto - " << static_cast<int>(i);
        std::string temp_string = ss_temp.str();

        FunMessage msg;
        msg.set_msgtype("pbuf_echo");
        PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
        echo->set_msg(temp_string.c_str());
        network_->SendMessage(msg);
      }
    }

    if (encoding == fun::FunEncoding::kJson)
    {
      for (int i = 1; i < 100; ++i) {
        // std::to_string is not supported on android, using std::stringstream instead.
        std::stringstream ss_temp;
        ss_temp <<  "hello world - " << static_cast<int>(i);
        std::string temp_string = ss_temp.str();

        rapidjson::Document msg;
        msg.SetObject();
        rapidjson::Value message_node(temp_string.c_str(), msg.GetAllocator());
        msg.AddMember("message", message_node, msg.GetAllocator());

        // Convert JSON document to string
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        msg.Accept(writer);
        std::string json_string = buffer.GetString();

        network_->SendMessage("echo", json_string);
      }
    }
  }

  return true;
}

bool Afunapi_tester::CreateMulticast()
{
  UE_LOG(LogClass, Warning, TEXT("CreateMulticast button clicked."));

  /*
  if (multicast_) {
    return true;
  }
  */

  if (network_) {
    auto transport = network_->GetTransport(fun::TransportProtocol::kTcp);
    if (transport) {
      multicast_ = std::make_shared<fun::FunapiMulticastClient>(network_, transport->GetEncoding());

      std::stringstream ss_temp;
      srand(time(NULL));
      ss_temp << "player" << static_cast<int>(rand() % 100 + 1);
      std::string sender = ss_temp.str();

      fun::DebugUtils::Log("sender = %s", sender.c_str());

      multicast_encoding_ = transport->GetEncoding();

      multicast_->SetSender(sender);
      // multicast_->SetEncoding(multicast_encoding_);

      multicast_->AddJoinedCallback([](const std::string &channel_id, const std::string &sender) {
        fun::DebugUtils::Log("JoinedCallback called. channel_id:%s player:%s", channel_id.c_str(), sender.c_str());
      });
      multicast_->AddLeftCallback([](const std::string &channel_id, const std::string &sender) {
        fun::DebugUtils::Log("LeftCallback called. channel_id:%s player:%s", channel_id.c_str(), sender.c_str());
      });
      multicast_->AddErrorCallback([](int error) {
        /*
        EC_ALREADY_JOINED = 1,
        EC_ALREADY_LEFT,
        EC_FULL_MEMBER
        */
      });

      return true;
    }
  }

  fun::DebugUtils::Log("You should connect to tcp transport first.");

  return true;
}

bool Afunapi_tester::JoinMulticastChannel()
{
  UE_LOG(LogClass, Warning, TEXT("JoinMulticastChannel button clicked."));

  if (multicast_) {
    if (multicast_->IsConnected() && !multicast_->IsInChannel(kMulticastTestChannel)) {
      multicast_->JoinChannel(kMulticastTestChannel, [this](const std::string &channel_id, const std::string &sender, const std::vector<uint8_t> &v_body) {
        OnMulticastChannelSignalle(channel_id, sender, v_body);
      });
    }
  }

  return true;
}

bool Afunapi_tester::SendMulticastMessage()
{
  UE_LOG(LogClass, Warning, TEXT("SendMulticastMessage button clicked."));

  if (multicast_) {
    if (multicast_->IsConnected() && multicast_->IsInChannel(kMulticastTestChannel)) {
      if (multicast_encoding_ == fun::FunEncoding::kJson) {
        rapidjson::Document msg;
        msg.SetObject();

        rapidjson::Value channel_id_node(kMulticastTestChannel.c_str(), msg.GetAllocator());
        // msg.AddMember(rapidjson::StringRef("_channel"), channel_id_node, msg.GetAllocator());
        msg.AddMember("_channel", channel_id_node, msg.GetAllocator());

        rapidjson::Value bounce_node(true);
        // msg.AddMember(rapidjson::StringRef("_bounce"), bounce_node, msg.GetAllocator());
        msg.AddMember("_bounce", bounce_node, msg.GetAllocator());

        std::string temp_messsage = "multicast test message";
        rapidjson::Value message_node(temp_messsage.c_str(), msg.GetAllocator());
        // msg.AddMember(rapidjson::StringRef("message"), message_node, msg.GetAllocator());
        msg.AddMember("message", message_node, msg.GetAllocator());

        // Convert JSON document to string
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        msg.Accept(writer);
        std::string json_string = buffer.GetString();

        multicast_->SendToChannel(json_string);
      }

      if (multicast_encoding_ == fun::FunEncoding::kProtobuf) {

      }
    }
  }

  return true;
}

bool Afunapi_tester::LeaveMulticastChannel()
{
  UE_LOG(LogClass, Warning, TEXT("LeaveMulticastChannel button clicked."));
  
  if (multicast_) {
    if (multicast_->IsConnected() && multicast_->IsInChannel(kMulticastTestChannel)) {
      multicast_->LeaveChannel(kMulticastTestChannel);
      // multicast_ = nullptr;
    }
  }
  
  return true;
}


void Afunapi_tester::Connect(const fun::TransportProtocol protocol)
{
  if (!network_ || !network_->IsSessionReliability()) {
    network_ = std::make_shared<fun::FunapiNetwork>(with_session_reliability_);

    network_->AddSessionInitiatedCallback([this](const std::string &session_id) { OnSessionInitiated(session_id); });
    network_->AddSessionClosedCallback([this]() { OnSessionClosed(); });

    network_->AddStoppedAllTransportCallback([this]() { OnStoppedAllTransport(); });
    network_->AddTransportConnectFailedCallback([this](const fun::TransportProtocol p) { OnTransportConnectFailed(p); });
    network_->AddTransportConnectTimeoutCallback([this](const fun::TransportProtocol p) { OnTransportConnectTimeout(p); });

    network_->AddMaintenanceCallback([this](const fun::TransportProtocol p, const std::string &type, const std::vector<uint8_t> &v_body) { OnMaintenanceMessage(p, type, v_body); });

    network_->RegisterHandler("echo", [this](const fun::TransportProtocol p, const std::string &type, const std::vector<uint8_t> &v_body) { OnEchoJson(p, type, v_body); });
    network_->RegisterHandler("pbuf_echo", [this](const fun::TransportProtocol p, const std::string &type, const std::vector<uint8_t> &v_body) { OnEchoProto(p, type, v_body); });

    network_->AttachTransport(GetNewTransport(protocol));
  }
  else {
    if (!network_->HasTransport(protocol))
    {
      network_->AttachTransport(GetNewTransport(protocol));
    }

    network_->SetDefaultProtocol(protocol);
  }

  network_->Start();
}

std::shared_ptr<fun::FunapiTransport> Afunapi_tester::GetNewTransport(fun::TransportProtocol protocol)
{
  std::shared_ptr<fun::FunapiTransport> transport = nullptr;
  fun::FunEncoding encoding = with_protobuf_ ? fun::FunEncoding::kProtobuf : fun::FunEncoding::kJson;

  if (protocol == fun::TransportProtocol::kTcp) {
    transport = fun::FunapiTcpTransport::create(kServerIp, static_cast<uint16_t>(with_protobuf_ ? 8022 : 8012), encoding);

    // transport->SetAutoReconnect(true);
    // transport->SetEnablePing(true);
    // transport->SetDisableNagle(true);
    // transport->SetConnectTimeout(100);
  }
  else if (protocol == fun::TransportProtocol::kUdp) {
    transport = fun::FunapiUdpTransport::create(kServerIp, static_cast<uint16_t>(with_protobuf_ ? 8023 : 8013), encoding);
  }
  else if (protocol == fun::TransportProtocol::kHttp) {
    transport = fun::FunapiHttpTransport::create(kServerIp, static_cast<uint16_t>(with_protobuf_ ? 8028 : 8018), false, encoding);
  }

  return transport;
}

void Afunapi_tester::OnSessionInitiated(const std::string &session_id)
{
  fun::DebugUtils::Log("session initiated: %s", session_id.c_str());
}

void Afunapi_tester::OnSessionClosed()
{
  fun::DebugUtils::Log("session closed");
}

void Afunapi_tester::OnEchoJson(const fun::TransportProtocol protocol, const std::string &type, const std::vector<uint8_t> &v_body)
{
  std::string body(v_body.begin(), v_body.end());

  fun::DebugUtils::Log("msg '%s' arrived.", type.c_str());
  fun::DebugUtils::Log("json: %s", body.c_str());
}

void Afunapi_tester::OnEchoProto(const fun::TransportProtocol protocol, const std::string &type, const std::vector<uint8_t> &v_body)
{
  fun::DebugUtils::Log("msg '%s' arrived.", type.c_str());

  std::string body(v_body.begin(), v_body.end());

  FunMessage msg;
  msg.ParseFromString(body);
  PbufEchoMessage echo = msg.GetExtension(pbuf_echo);
  fun::DebugUtils::Log("proto: %s", echo.msg().c_str());
}

bool Afunapi_tester::FileDownload()
{
  UE_LOG(LogClass, Warning, TEXT("FileDownload button clicked."));
  /*
  fun::FunapiHttpDownloader downloader;
  downloader.ReadyCallback += [&downloader](int count, uint64 size) {
    downloader.StartDownload();
  };

  downloader.GetDownloadList("http://127.0.0.1:8020", "C:\\download_test");

  while (downloader.IsDownloading()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  */

  return true;
}

void Afunapi_tester::OnMaintenanceMessage(const fun::TransportProtocol protocol, const std::string &type, const std::vector<uint8_t> &v_body)
{
  fun::DebugUtils::Log("OnMaintenanceMessage");

  fun::FunEncoding encoding = with_protobuf_ ? fun::FunEncoding::kProtobuf : fun::FunEncoding::kJson;

  if (encoding == fun::FunEncoding::kJson) {
    std::string body(v_body.cbegin(), v_body.cend());
    fun::DebugUtils::Log("Maintenance message\n%s", body.c_str());
  }

  if (encoding == fun::FunEncoding::kProtobuf) {
    std::string body(v_body.cbegin(), v_body.cend());

    FunMessage msg;
    msg.ParseFromString(body);

    MaintenanceMessage maintenance = msg.GetExtension(pbuf_maintenance);
    std::string date_start = maintenance.date_start();
    std::string date_end = maintenance.date_end();
    std::string message = maintenance.messages();

    fun::DebugUtils::Log("Maintenance message\nstart: %s\nend: %s\nmessage: %s", date_start.c_str(), date_end.c_str(), message.c_str());
  }
}

void Afunapi_tester::OnStoppedAllTransport()
{
  fun::DebugUtils::Log("OnStoppedAllTransport called.");
}

void Afunapi_tester::OnTransportConnectFailed(const fun::TransportProtocol protocol)
{
  fun::DebugUtils::Log("OnTransportConnectFailed(%d)", (int)protocol);
}

void Afunapi_tester::OnTransportConnectTimeout(const fun::TransportProtocol protocol)
{
  fun::DebugUtils::Log("OnTransportConnectTimeout called.");
}

void Afunapi_tester::OnMulticastChannelSignalle(const std::string &channel_id, const std::string &sender, const std::vector<uint8_t> &v_body)
{
  if (multicast_encoding_ == fun::FunEncoding::kJson) {
    std::string body(v_body.cbegin(), v_body.cend());
    fun::DebugUtils::Log("channel_id=%s, sender=%s, body=%s", channel_id.c_str(), sender.c_str(), body.c_str());
  }

  if (multicast_encoding_ == fun::FunEncoding::kProtobuf) {
    std::string body(v_body.cbegin(), v_body.cend());

    FunMessage msg;
    msg.ParseFromString(body);

    FunMulticastMessage multicast_msg = msg.GetExtension(multicast);
    // multicast_msg.channel();
  }
}
