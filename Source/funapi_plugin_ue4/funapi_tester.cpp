// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin_ue4.h"

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif

#include "funapi/funapi_network.h"
#include "funapi/pb/test_messages.pb.h"
#include "funapi/pb/management/maintenance_message.pb.h"
#include "funapi/pb/service/multicast_message.pb.h"
#include "funapi/funapi_utils.h"
#include "funapi_tester.h"

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

// Tcp Transport로 서버에 연결합니다.
bool Afunapi_tester::ConnectTcp()
{
  Connect(fun::TransportProtocol::kTcp);
  return true;
}

// Udp Transport로 서버에 연결합니다.
bool Afunapi_tester::ConnectUdp()
{
  Connect(fun::TransportProtocol::kUdp);
  return true;
}

// Http Transport로 서버에 연결합니다.
bool Afunapi_tester::ConnectHttp()
{
  Connect(fun::TransportProtocol::kHttp);
  return true;
}

// 서버와 연결이 된 상태인지 체크합니다.
bool Afunapi_tester::IsConnected()
{
  return network_ != nullptr && network_->IsConnected();
}

// 서버와의 연결을 끊습니다.
void Afunapi_tester::Disconnect()
{
  if (network_ == nullptr || network_->IsStarted() == false)
  {
    fun::DebugUtils::Log("You should connect first.");
    return;
  }

  network_->Stop();
}

// 테스트용 에코 메시지를 서버로 전송합니다.
// 이 메시지를 받으면 서버에서는 같은 메시지를 응답으로 보내줍니다.
bool Afunapi_tester::SendEchoMessage()
{
  // 연결 여부를 확인합니다.
  // 연결이 끊긴 상태에서도 Session reliability 옵션이 켜져 있다면 메시지를 보낼 수 있습니다. (나중에 연결되면 전송됨)
  if (network_ == nullptr || (network_->IsStarted() == false && network_->IsSessionReliability()))
  {
    fun::DebugUtils::Log("You should connect first.");
  }
  else
  {
    // 기본 프로토콜 타입을 가져옵니다.
    fun::FunEncoding encoding = network_->GetEncoding(network_->GetDefaultProtocol());
    if (encoding == fun::FunEncoding::kNone)
    {
      fun::DebugUtils::Log("You should attach transport first.");
      return false;
    }

    // encoding 타입이 protobuf일 경우 protobuf 메시지를 보냅니다.
    if (encoding == fun::FunEncoding::kProtobuf)
    {
      // protobuf 메시지를 100개 보냅니다.
      for (int i = 1; i < 100; ++i) {
        // std::to_string is not supported on android, using std::stringstream instead.
        std::stringstream ss_temp;
        ss_temp << "hello proto - " << static_cast<int>(i);
        std::string temp_string = ss_temp.str();

        // protobuf 메시지를 생성합니다.
        FunMessage msg;
        msg.set_msgtype("pbuf_echo");
        PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
        echo->set_msg(temp_string.c_str());

        // 메시지 전송
        network_->SendMessage(msg);
      }
    }

    // encoding 타입이 json일 경우 json 메시지를 보냅니다.
    if (encoding == fun::FunEncoding::kJson)
    {
      // json 메시지를 100개 보냅니다.
      for (int i = 1; i < 100; ++i) {
        // std::to_string is not supported on android, using std::stringstream instead.
        std::stringstream ss_temp;
        ss_temp <<  "hello world - " << static_cast<int>(i);
        std::string temp_string = ss_temp.str();

        // json 메시지를 생성합니다.
        TSharedRef<FJsonObject> json_object = MakeShareable(new FJsonObject);
        json_object->SetStringField(FString("message"), FString(temp_string.c_str()));

        // Convert JSON document to string
        FString ouput_fstring;
        TSharedRef<TJsonWriter<TCHAR>> writer = TJsonWriterFactory<TCHAR>::Create(&ouput_fstring);
        FJsonSerializer::Serialize(json_object, writer);
        std::string json_stiring = TCHAR_TO_ANSI(*ouput_fstring);

        // 메시지 전송
        network_->SendMessage("echo", json_stiring);
      }
    }
  }

  return true;
}

// 멀티캐스팅 객체를 생성합니다. 멀티캐스팅은 Tcp 프로토콜을 사용합니다.
// 멀티캐스팅 객체를 생성하려면 먼저 FunapiNetwork가 생성되어 있어야 합니다.
// 먼저 Tcp Transport 연결을 하고 멀티캐스팅 객체를 생성해주세요.
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
      // 멀티캐스팅 객체 생성
      multicast_ = std::make_shared<fun::FunapiMulticastClient>(network_, transport->GetEncoding());

      // random sender id 만들기
      std::stringstream ss_temp;
      srand(time(NULL));
      ss_temp << "player" << static_cast<int>(rand() % 100 + 1);
      std::string sender = ss_temp.str();

      fun::DebugUtils::Log("sender = %s", sender.c_str());

      // Tcp transport의 인코딩 타입을 멀티캐스팅의 인코딩 타입으로 세팅
      multicast_encoding_ = transport->GetEncoding();

      // sender id 세팅
      multicast_->SetSender(sender);
      // multicast_->SetEncoding(multicast_encoding_);

      // 채널 입장 콜백 등록
      multicast_->AddJoinedCallback([](const std::string &channel_id, const std::string &s) {
        fun::DebugUtils::Log("JoinedCallback called. channel_id:%s player:%s", channel_id.c_str(), s.c_str());
      });
      // 채널 퇴장 콜백 등록
      multicast_->AddLeftCallback([](const std::string &channel_id, const std::string &s) {
        fun::DebugUtils::Log("LeftCallback called. channel_id:%s player:%s", channel_id.c_str(), s.c_str());
      });
      // 에러 콜백 등록
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

// 멀티캐스팅 채널에 입장합니다.
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

// 멀티캐스팅 채널에 메시지를 전송합니다.
bool Afunapi_tester::SendMulticastMessage()
{
  UE_LOG(LogClass, Warning, TEXT("SendMulticastMessage button clicked."));

  if (multicast_) {
    // 채널에 입장된 상태인지 검사
    if (multicast_->IsConnected() && multicast_->IsInChannel(kMulticastTestChannel)) {
      // encoding 타입이 json일 경우
      if (multicast_encoding_ == fun::FunEncoding::kJson) {
        rapidjson::Document msg;
        msg.SetObject();

        // 채널 Id
        rapidjson::Value channel_id_node(kMulticastTestChannel.c_str(), msg.GetAllocator());
        // msg.AddMember(rapidjson::StringRef("_channel"), channel_id_node, msg.GetAllocator());
        msg.AddMember("_channel", channel_id_node, msg.GetAllocator());

        // 이 값을 true로 주면 보내는 사람도 메시지를 받을 수 있습니다.
        rapidjson::Value bounce_node(true);
        // msg.AddMember(rapidjson::StringRef("_bounce"), bounce_node, msg.GetAllocator());
        msg.AddMember("_bounce", bounce_node, msg.GetAllocator());

        // 보내는 메시지
        std::string temp_messsage = "multicast test message";
        rapidjson::Value message_node(temp_messsage.c_str(), msg.GetAllocator());
        // msg.AddMember(rapidjson::StringRef("message"), message_node, msg.GetAllocator());
        msg.AddMember("message", message_node, msg.GetAllocator());

        // Convert JSON document to string
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        msg.Accept(writer);
        std::string json_string = buffer.GetString();

        // 메시지 전송
        multicast_->SendToChannel(json_string);
      }

      // encoding 타입이 protobuf일 경우
      if (multicast_encoding_ == fun::FunEncoding::kProtobuf) {
        FunMessage msg;

        FunMulticastMessage* mcast_msg = msg.MutableExtension(multicast);
        mcast_msg->set_channel(kMulticastTestChannel.c_str());
        mcast_msg->set_bounce(true);

        FunChatMessage *chat_msg = mcast_msg->MutableExtension(chat);
        chat_msg->set_text("multicast test message");

        multicast_->SendToChannel(msg);
      }
    }
  }

  return true;
}

// 멀티캐스팅 채널에서 퇴장합니다.
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

// FunapiNetwork 객체를 생성하고 연결을 시작합니다.
void Afunapi_tester::Connect(const fun::TransportProtocol protocol)
{
  if (!network_ || !network_->IsSessionReliability()) {
    // FunapiNetwork 객체 생성
    network_ = std::make_shared<fun::FunapiNetwork>(with_session_reliability_);

    // 세션 관련 콜백 등록
    network_->AddSessionInitiatedCallback([this](const std::string &session_id) { OnSessionInitiated(session_id); });
    network_->AddSessionClosedCallback([this]() { OnSessionClosed(); });

    // Transport 관련 콜백 등록
    network_->AddStoppedAllTransportCallback([this]() { OnStoppedAllTransport(); });
    network_->AddTransportConnectFailedCallback([this](const fun::TransportProtocol p) { OnTransportConnectFailed(p); });
    network_->AddTransportConnectTimeoutCallback([this](const fun::TransportProtocol p) { OnTransportConnectTimeout(p); });

    // 정기점검 관련 콜백 등록
    network_->AddMaintenanceCallback([this](const fun::TransportProtocol p, const std::string &type, const std::vector<uint8_t> &v_body) { OnMaintenanceMessage(p, type, v_body); });

    // 메시지 핸들러 등록
    // 각각의 메시지마다 핸들러를 등록해야 메시지에 대한 응답을 받을 수 있습니다.
    network_->RegisterHandler("echo", [this](const fun::TransportProtocol p, const std::string &type, const std::vector<uint8_t> &v_body) { OnEchoJson(p, type, v_body); });
    network_->RegisterHandler("pbuf_echo", [this](const fun::TransportProtocol p, const std::string &type, const std::vector<uint8_t> &v_body) { OnEchoProto(p, type, v_body); });

    network_->AttachTransport(GetNewTransport(protocol));
  }
  else {
    if (!network_->HasTransport(protocol))
    {
      network_->AttachTransport(GetNewTransport(protocol));
    }

    // 기본 프로토콜 지정
    // Transport가 2개 이상일 경우 기본 프로토콜을 지정하지 않으면 제일 처음 등록한 Transport의 프로토콜이 기본 프로토콜이 됩니다.
    network_->SetDefaultProtocol(protocol);
  }

  // 서버에 연결을 시작합니다.
  network_->Start();
}

// 새 Transport 객체를 생성합니다.
std::shared_ptr<fun::FunapiTransport> Afunapi_tester::GetNewTransport(fun::TransportProtocol protocol)
{
  std::shared_ptr<fun::FunapiTransport> transport = nullptr;
  fun::FunEncoding encoding = with_protobuf_ ? fun::FunEncoding::kProtobuf : fun::FunEncoding::kJson;

  if (protocol == fun::TransportProtocol::kTcp) {
    // Tcp transport 객체 생성
    transport = fun::FunapiTcpTransport::create(kServerIp, static_cast<uint16_t>(with_protobuf_ ? 8022 : 8012), encoding);

    // transport->SetAutoReconnect(true);
    // transport->SetEnablePing(true);
    // transport->SetDisableNagle(true);
    // transport->SetConnectTimeout(100);
  }
  else if (protocol == fun::TransportProtocol::kUdp) {
    // Udp transport 객체 생성
    transport = fun::FunapiUdpTransport::create(kServerIp, static_cast<uint16_t>(with_protobuf_ ? 8023 : 8013), encoding);
  }
  else if (protocol == fun::TransportProtocol::kHttp) {
    // Http transport 객체 생성
    transport = fun::FunapiHttpTransport::create(kServerIp, static_cast<uint16_t>(with_protobuf_ ? 8028 : 8018), false, encoding);
  }

  return transport;
}

// 세션이 초기화되면 호출되는 콜백 함수입니다.
void Afunapi_tester::OnSessionInitiated(const std::string &session_id)
{
  fun::DebugUtils::Log("session initiated: %s", session_id.c_str());
}

// 세션 연결이 종료되면 호출되는 콜백 함수입니다.
void Afunapi_tester::OnSessionClosed()
{
  fun::DebugUtils::Log("session closed");
}

// Json "echo" 메시지에 대한 응답을 받으면 호출되는 콜백 함수입니다.
void Afunapi_tester::OnEchoJson(const fun::TransportProtocol protocol, const std::string &type, const std::vector<uint8_t> &v_body)
{
  std::string body(v_body.begin(), v_body.end());

  fun::DebugUtils::Log("msg '%s' arrived.", type.c_str());
  fun::DebugUtils::Log("json: %s", body.c_str());
}

// Protobuf "pbuf_echo" 메시지에 대한 응답을 받으면 호출되는 콜백 함수입니다.
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

// 서버가 정기점검 중일 때 받게 되는 메시지 콜백 함수입니다.
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

// 모든 Transport의 연결이 종료되면 호출되는 콜백 함수입니다.
void Afunapi_tester::OnStoppedAllTransport()
{
  fun::DebugUtils::Log("OnStoppedAllTransport called.");
}

// Transport 연결이 실패하면 호출되는 콜백 함수입니다.
void Afunapi_tester::OnTransportConnectFailed(const fun::TransportProtocol protocol)
{
  fun::DebugUtils::Log("OnTransportConnectFailed(%d)", (int)protocol);
}

// Transport 연결 대기 시간이 초과되면 호출되는 콜백 함수입니다.
void Afunapi_tester::OnTransportConnectTimeout(const fun::TransportProtocol protocol)
{
  fun::DebugUtils::Log("OnTransportConnectTimeout called.");
}

// 멀티캐스팅 메시지를 받으면 호출되는 콜백 함수입니다.
void Afunapi_tester::OnMulticastChannelSignalle(const std::string &channel_id, const std::string &sender, const std::vector<uint8_t> &v_body)
{
  if (multicast_encoding_ == fun::FunEncoding::kJson) {
    std::string body(v_body.cbegin(), v_body.cend());
    fun::DebugUtils::Log("channel_id=%s, sender=%s, body=%s", channel_id.c_str(), sender.c_str(), body.c_str());
  }

  if (multicast_encoding_ == fun::FunEncoding::kProtobuf) {
    std::string body(v_body.cbegin(), v_body.cend());

    // 모든 protobuf 메시지는 FunMessage로 되어 있습니다.
    // FunMessage에 extend로 추가된 메시지를 꺼내서 사용해야합니다.
    FunMessage msg;
    msg.ParseFromString(body);

    // 멀티캐스팅 메시지입니다.
    FunMulticastMessage* mcast_msg = msg.MutableExtension(multicast);
    FunChatMessage *chat_msg = mcast_msg->MutableExtension(chat);
    std::string message = chat_msg->text();

    fun::DebugUtils::Log("channel_id=%s, sender=%s, message=%s", channel_id.c_str(), sender.c_str(), message.c_str());
  }
}
