// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin_ue4.h"

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif

#include "test_messages.pb.h"
#include "funapi/management/maintenance_message.pb.h"
#include "funapi/service/multicast_message.pb.h"
#include "funapi/funapi_utils.h"
#include "funapi/funapi_tasks.h"
#include "funapi/funapi_announcement.h"
#include "funapi_session.h"
#include "funapi_multicasting.h"
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

  fun::FunapiTasks::UpdateAll();

  UpdateUI();
}

void Afunapi_tester::UpdateUI()
{
  bool is_connected_session = false;
  if (session_) {
    if (session_->IsConnected()) {
      is_connected_session = true;
    }
  }

  if (is_connected_session) {
    EnableButtonConnectTcp = false;
    EnableButtonConnectUdp = false;
    EnableButtonConnectHttp = false;
    EnableButtonDisconnect = true;
    EnableButtonSendEchoMessage = true;
  }
  else {
    EnableButtonConnectTcp = true;
    EnableButtonConnectUdp = true;
    EnableButtonConnectHttp = true;
    EnableButtonDisconnect = false;
    EnableButtonSendEchoMessage = false;
  }

  bool is_connected_multicast = false;
  if (multicast_) {
    if (multicast_->IsConnected()) {
      is_connected_multicast = true;
    }
  }

  if (is_connected_multicast) {
    EnableButtonCreateMulticast = false;
    if (multicast_->IsInChannel(kMulticastTestChannel)) {
      EnableButtonJoinChannel = false;
      EnableButtonSendMulticastMessage = true;
      EnableButtonLeaveChannel = true;
      EnableButtonLeaveAllChannels = true;
    }
    else {
      EnableButtonJoinChannel = true;
      EnableButtonSendMulticastMessage = false;
      EnableButtonLeaveChannel = false;
      EnableButtonLeaveAllChannels = false;
    }
    EnableButtonRequestList = true;
  }
  else {
      EnableButtonCreateMulticast = true;
      EnableButtonJoinChannel = false;
      EnableButtonSendMulticastMessage = false;
      EnableButtonLeaveChannel = false;
      EnableButtonRequestList = false;
      EnableButtonLeaveAllChannels = false;
  }

  if (is_connected_session || is_connected_multicast) {
    EnableTextboxServerIP = false;
    EnableCheckboxProtobuf = false;
    EnableCheckboxSessionReliability = false;
  }
}

void Afunapi_tester::SendRedirectTestMessage()
{
  if (session_ == nullptr)
  {
    fun::DebugUtils::Log("You should connect first.");
  }
  else {
    fun::FunEncoding encoding = session_->GetEncoding(session_->GetDefaultProtocol());
    if (encoding == fun::FunEncoding::kNone)
    {
      fun::DebugUtils::Log("You should attach transport first.");
      return;
    }

    std::stringstream ss_temp;
    std::random_device rd;
    std::default_random_engine re(rd());
    std::uniform_int_distribution<int> dist(1,0xffff);
    ss_temp << "name" << dist(re);
    std::string name = ss_temp.str();

    if (encoding == fun::FunEncoding::kProtobuf)
    {
      /*
       FunMessage msg;

       msg.set_msgtype("cs_hello");
       Hello *hello = msg.MutableExtension(cs_hello);
       // hello->set_name("hello-name");
       hello->set_name(name.c_str());

       session_->SendMessage(msg);
       */
    }

    if (encoding == fun::FunEncoding::kJson)
    {
      // json 메시지를 생성합니다.
      TSharedRef<FJsonObject> json_object = MakeShareable(new FJsonObject);
      json_object->SetStringField(FString("name"), FString(name.c_str()));

      // Convert JSON document to string
      FString ouput_fstring;
      TSharedRef<TJsonWriter<TCHAR>> writer = TJsonWriterFactory<TCHAR>::Create(&ouput_fstring);
      FJsonSerializer::Serialize(json_object, writer);
      std::string json_stiring = TCHAR_TO_ANSI(*ouput_fstring);

      // 메시지 전송
      session_->SendMessage("hello", json_stiring);
    }
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

// 서버와의 연결을 끊습니다.
void Afunapi_tester::Disconnect()
{
  if (session_) {
    session_->Close();
    return;
  }

  fun::DebugUtils::Log("You should connect first.");
}

// 테스트용 에코 메시지를 서버로 전송합니다.
// 이 메시지를 받으면 서버에서는 같은 메시지를 응답으로 보내줍니다.
bool Afunapi_tester::SendEchoMessage()
{
  if (session_ == nullptr)
  {
    fun::DebugUtils::Log("You should connect first.");
  }
  else
  {
    // 기본 프로토콜 타입을 가져옵니다.
    fun::FunEncoding encoding = session_->GetEncoding(session_->GetDefaultProtocol());
    if (encoding == fun::FunEncoding::kNone)
    {
      fun::DebugUtils::Log("You should attach transport first.");
      return false;
    }

    // encoding 타입이 protobuf일 경우 protobuf 메시지를 보냅니다.
    if (encoding == fun::FunEncoding::kProtobuf)
    {
      // protobuf 메시지를 10개 보냅니다.
      for (int i = 0; i < 10; ++i) {
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
        session_->SendMessage(msg);
      }
    }

    // encoding 타입이 json일 경우 json 메시지를 보냅니다.
    if (encoding == fun::FunEncoding::kJson)
    {
      // json 메시지를 10개 보냅니다.
      for (int i = 0; i < 10; ++i) {
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
        session_->SendMessage("echo", json_stiring);
      }
    }
  }

  return true;
}

// 멀티캐스팅 객체를 생성합니다.
bool Afunapi_tester::CreateMulticast()
{
  UE_LOG(LogClass, Warning, TEXT("CreateMulticast button clicked."));

  if (!multicast_) {
    // random sender id 만들기
    std::stringstream ss_temp;
    std::random_device rd;
    std::default_random_engine re(rd());
    std::uniform_int_distribution<int> dist(1, 100);
    ss_temp << "player" << dist(re);
    std::string sender = ss_temp.str();

    fun::DebugUtils::Log("sender = %s", sender.c_str());

    fun::FunEncoding encoding = with_protobuf_ ? fun::FunEncoding::kProtobuf : fun::FunEncoding::kJson;
    uint16_t port = with_protobuf_ ? 8022 : 8012;

    // 멀티캐스팅 객체 생성
    multicast_ = fun::FunapiMulticast::create(sender.c_str(), kServerIp.c_str(), port, encoding);

    // 채널 입장 콜백 등록
    multicast_->AddJoinedCallback([](const std::shared_ptr<fun::FunapiMulticast>& funapi_multicast,
      const std::string &channel_id, const std::string &multicast_sender) {
      fun::DebugUtils::Log("JoinedCallback called. channel_id:%s player:%s", channel_id.c_str(), multicast_sender.c_str());
    });
    // 채널 퇴장 콜백 등록
    multicast_->AddLeftCallback([](const std::shared_ptr<fun::FunapiMulticast>& funapi_multicast,
                                   const std::string &channel_id,
                                   const std::string &multicast_sender) {
      fun::DebugUtils::Log("LeftCallback called. channel_id:%s player:%s", channel_id.c_str(), multicast_sender.c_str());
    });
    // 에러 콜백 등록
    multicast_->AddErrorCallback([](const std::shared_ptr<fun::FunapiMulticast>& funapi_multicast,
                                    int error) {
      // EC_ALREADY_JOINED = 1,
      // EC_ALREADY_LEFT,
      // EC_FULL_MEMBER
      // EC_CLOSED
    });
    multicast_->AddChannelListCallback([](const std::shared_ptr<fun::FunapiMulticast>& funapi_multicast,
                                          const std::map<std::string, int> &cl) {
      // fun::DebugUtils::Log("[channel list]");
      for (auto i : cl) {
        fun::DebugUtils::Log("%s - %d", i.first.c_str(), i.second);
      }
    });
    multicast_->AddSessionEventCallback([](const std::shared_ptr<fun::FunapiMulticast>& funapi_multicast,
                                           const fun::SessionEventType type,
                                           const std::string &session_id,
                                           const std::shared_ptr<fun::FunapiError> &error) {
     /*
     if (type == fun::SessionEventType::kOpened) {
     }
     else if (type == fun::SessionEventType::kChanged) {
     // session id changed
     }
     else if (type == fun::SessionEventType::kClosed) {
     }
     */
    });
    multicast_->AddTransportEventCallback([this](const std::shared_ptr<fun::FunapiMulticast>& funapi_multicast,
                                                 const fun::TransportEventType type,
                                                 const std::shared_ptr<fun::FunapiError> &error) {
      if (type == fun::TransportEventType::kStarted) {
        fun::DebugUtils::Log("Transport Started called.");
      }
      else if (type == fun::TransportEventType::kStopped) {
        fun::DebugUtils::Log("Transport Stopped called.");
      }
      else if (type == fun::TransportEventType::kConnectionFailed) {
        fun::DebugUtils::Log("Transport Connection Failed");
        multicast_ = nullptr;
      }
      else if (type == fun::TransportEventType::kConnectionTimedOut) {
        fun::DebugUtils::Log("Transport Connection Timedout called");
        multicast_ = nullptr;
      }
      else if (type == fun::TransportEventType::kDisconnected) {
        fun::DebugUtils::Log("Transport Disconnected called");
      }
    });

    multicast_->Connect();
  }

  return true;
}

// 멀티캐스팅 채널에 입장합니다.
bool Afunapi_tester::JoinMulticastChannel()
{
  UE_LOG(LogClass, Warning, TEXT("JoinMulticastChannel button clicked."));

  if (multicast_) {
    if (!multicast_->IsInChannel(kMulticastTestChannel)) {
      // add callback
      multicast_->AddJsonChannelMessageCallback(kMulticastTestChannel,
        [this](const std::shared_ptr<fun::FunapiMulticast>& funapi_multicast,
          const std::string &channel_id,
          const std::string &sender,
          const std::string &json_string)
      {
        fun::DebugUtils::Log("channel_id=%s, sender=%s, body=%s", channel_id.c_str(), sender.c_str(), json_string.c_str());
      });

      multicast_->AddProtobufChannelMessageCallback(kMulticastTestChannel,
        [this](const std::shared_ptr<fun::FunapiMulticast> &funapi_multicast,
          const std::string &channel_id,
          const std::string &sender,
          const FunMessage& message)
      {
        FunMulticastMessage mcast_msg = message.GetExtension(multicast);
        FunChatMessage chat_msg = mcast_msg.GetExtension(chat);
        std::string text = chat_msg.text();

        fun::DebugUtils::Log("channel_id=%s, sender=%s, message=%s", channel_id.c_str(), sender.c_str(), text.c_str());
      });

      // join
      multicast_->JoinChannel(kMulticastTestChannel);
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
      if (multicast_->GetEncoding() == fun::FunEncoding::kJson) {
        rapidjson::Document msg;
        msg.SetObject();

        std::string temp_messsage = "multicast test message";
        rapidjson::Value message_node(temp_messsage.c_str(), msg.GetAllocator());
        msg.AddMember(rapidjson::StringRef("message"), message_node, msg.GetAllocator());

        // Convert JSON document to string
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        msg.Accept(writer);
        std::string json_string = buffer.GetString();

        multicast_->SendToChannel(kMulticastTestChannel, json_string);
      }

      // encoding 타입이 protobuf일 경우
      if (multicast_->GetEncoding() == fun::FunEncoding::kProtobuf) {
        FunMessage msg;

        FunMulticastMessage* mcast_msg = msg.MutableExtension(multicast);
        mcast_msg->set_channel(kMulticastTestChannel.c_str());
        mcast_msg->set_bounce(true);

        FunChatMessage *chat_msg = mcast_msg->MutableExtension(chat);
        chat_msg->set_text("multicast test message");

        multicast_->SendToChannel(kMulticastTestChannel, msg);
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
    }
  }

  return true;
}

bool Afunapi_tester::RequestMulticastChannelList()
{
  UE_LOG(LogClass, Warning, TEXT("RequestMulticastChannelList button clicked."));

  if (multicast_) {
    if (multicast_->IsConnected()) {
      multicast_->RequestChannelList();
    }
  }

  return true;
}

bool Afunapi_tester::LeaveMulticastAllChannels()
{
  UE_LOG(LogClass, Warning, TEXT("Leave all channels button clicked."));

  if (multicast_) {
    multicast_->LeaveAllChannels();
  }

  return true;
}

bool Afunapi_tester::RequestAnnouncements()
{
  UE_LOG(LogClass, Warning, TEXT("Request announcements button clicked."));

  if (announcement_ == nullptr) {
    std::stringstream ss_url;
    ss_url << "http://" << kAnnouncementServer << ":" << kAnnouncementPort;

    std::string path = fun::FunapiUtil::GetWritablePath();

    fun::DebugUtils::Log("path = %s", path.c_str());

    announcement_ = fun::FunapiAnnouncement::create(ss_url.str().c_str(), path.c_str());

    announcement_->AddCompletionCallback([](const std::shared_ptr<fun::FunapiAnnouncement> &announcement,
                                            const fun::AnnouncementResult result,
                                            const std::vector<std::shared_ptr<fun::AnnouncementInfo>>&info){
      if (result == fun::AnnouncementResult::kSuccess) {
        for (auto i : info) {
          fun::DebugUtils::Log("date=%s message=%s subject=%s file_path=%s", i->GetDate().c_str(), i->GetMessageText().c_str(), i->GetSubject().c_str(), i->GetFilePath().c_str());
        }
      }
    });
  }

  announcement_->RequestList(5);

  return true;
}

FText Afunapi_tester::GetServerIP() {
  return FText::FromString(FString(kServerIp.c_str()));
}

void Afunapi_tester::SetServerIP(FText server_ip) {
  kServerIp = TCHAR_TO_UTF8(*(server_ip.ToString()));
  // fun::DebugUtils::Log("SetServerIP - %s", kServerIp.c_str());
}

bool Afunapi_tester::GetIsProtobuf() {
  return with_protobuf_;
}

void Afunapi_tester::SetIsProtobuf(bool check) {
  with_protobuf_ = check;
  // fun::DebugUtils::Log("SetIsProtobuf - %d", with_protobuf_);
}

bool Afunapi_tester::GetIsSessionReliability() {
  return with_session_reliability_;
}

void Afunapi_tester::SetIsSessionReliability(bool check) {
  with_session_reliability_ = check;
  // fun::DebugUtils::Log("SetIsSessionReliability - %d", with_session_reliability_);
}

bool Afunapi_tester::GetIsEnableTextboxServerIP() {
  return EnableTextboxServerIP;
}

bool Afunapi_tester::GetIsEnableCheckboxProtobuf() {
  return EnableCheckboxProtobuf;
}

bool Afunapi_tester::GetIsEnableCheckboxSessionReliability() {
  return EnableCheckboxSessionReliability;
}

bool Afunapi_tester::GetIsEnableButtonConnectTcp() {
  return EnableButtonConnectTcp;
}

bool Afunapi_tester::GetIsEnableButtonConnectUdp() {
  return EnableButtonConnectUdp;
}

bool Afunapi_tester::GetIsEnableButtonConnectHttp() {
  return EnableButtonConnectHttp;
}

bool Afunapi_tester::GetIsEnableButtonSendEchoMessage() {
  return EnableButtonSendEchoMessage;
}

bool Afunapi_tester::GetIsEnableButtonDisconnect() {
  return EnableButtonDisconnect;
}

bool Afunapi_tester::GetIsEnableButtonCreateMulticast() {
  return EnableButtonCreateMulticast;
}

bool Afunapi_tester::GetIsEnableButtonJoinChannel() {
  return EnableButtonJoinChannel;
}

bool Afunapi_tester::GetIsEnableButtonSendMulticastMessage() {
  return EnableButtonSendMulticastMessage;
}

bool Afunapi_tester::GetIsEnableButtonLeaveChannel() {
  return EnableButtonLeaveChannel;
}

bool Afunapi_tester::GetIsEnableButtonRequestList() {
  return EnableButtonRequestList;
}

bool Afunapi_tester::GetIsEnableButtonLeaveAllChannels() {
  return EnableButtonLeaveAllChannels;
}

// FunapiSession 객체를 생성하고 연결을 시작합니다.
void Afunapi_tester::Connect(const fun::TransportProtocol protocol)
{
  if (!session_) {
    // create
    session_ = fun::FunapiSession::create(kServerIp.c_str(), with_session_reliability_);

    // add callback
    session_->AddSessionEventCallback([this](const std::shared_ptr<fun::FunapiSession> &session,
                                             const fun::TransportProtocol transport_protocol,
                                             const fun::SessionEventType type,
                                             const std::string &session_id,
                                             const std::shared_ptr<fun::FunapiError> &error) {
      if (type == fun::SessionEventType::kOpened) {
        OnSessionInitiated(session_id);
      }
      else if (type == fun::SessionEventType::kChanged) {
        // session id changed
      }
      else if (type == fun::SessionEventType::kClosed) {
        OnSessionClosed();
      }
    });

    session_->AddTransportEventCallback([this](const std::shared_ptr<fun::FunapiSession> &session,
                                               const fun::TransportProtocol transport_protocol,
                                               const fun::TransportEventType type,
                                               const std::shared_ptr<fun::FunapiError> &error) {
      if (type == fun::TransportEventType::kStarted) {
        fun::DebugUtils::Log("Transport Started called.");
      }
      else if (type == fun::TransportEventType::kStopped) {
        fun::DebugUtils::Log("Transport Stopped called.");
      }
      else if (type == fun::TransportEventType::kConnectionFailed) {
        fun::DebugUtils::Log("Transport Connection Failed(%d)", (int)transport_protocol);
        session_ = nullptr;
      }
      else if (type == fun::TransportEventType::kConnectionTimedOut) {
        fun::DebugUtils::Log("Transport Connection Timedout called");
        session_ = nullptr;
      }
      else if (type == fun::TransportEventType::kDisconnected) {
        fun::DebugUtils::Log("Transport Disconnected called (%d)", (int)transport_protocol);
      }
    });

    session_->AddJsonRecvCallback([](const std::shared_ptr<fun::FunapiSession> &session,
                                     const fun::TransportProtocol transport_protocol,
                                     const std::string &msg_type,
                                     const std::string &json_string) {
      if (msg_type.compare("echo") == 0) {
        fun::DebugUtils::Log("msg '%s' arrived.", msg_type.c_str());
        fun::DebugUtils::Log("json: %s", json_string.c_str());
      }

      if (msg_type.compare("_maintenance") == 0) {
        fun::DebugUtils::Log("Maintenance message : %s", json_string.c_str());
      }
    });

    session_->AddProtobufRecvCallback([](const std::shared_ptr<fun::FunapiSession> &session,
                                         const fun::TransportProtocol transport_protocol,
                                         const FunMessage &fun_message) {
      if (fun_message.msgtype().compare("pbuf_echo") == 0) {
        fun::DebugUtils::Log("msg '%s' arrived.", fun_message.msgtype().c_str());

        PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);
        fun::DebugUtils::Log("proto: %s", echo.msg().c_str());
      }

      if (fun_message.msgtype().compare("_maintenance") == 0) {
        fun::DebugUtils::Log("msg '%s' arrived.", fun_message.msgtype().c_str());

        PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);
        fun::DebugUtils::Log("proto: %s", echo.msg().c_str());

        MaintenanceMessage maintenance = fun_message.GetExtension(pbuf_maintenance);
        std::string date_start = maintenance.date_start();
        std::string date_end = maintenance.date_end();
        std::string message_text = maintenance.messages();

        fun::DebugUtils::Log("Maintenance message:\nstart: %s\nend: %s\nmessage: %s", date_start.c_str(), date_end.c_str(), message_text.c_str());
      }
    });

    /*
    session_->SetTransportOptionCallback([](const fun::TransportProtocol protocol,
                                            const std::string &flavor) -> std::shared_ptr<fun::FunapiTransportOption> {
      if (protocol == fun::TransportProtocol::kTcp) {
        auto option = fun::FunapiTcpTransportOption::create();
        option->SetDisableNagle(true);
        return option;
      }

      return nullptr;
    });
    */
  }

  // connect
  if (session_->HasTransport(protocol)) {
    session_->Connect(protocol);
  }
  else {
    fun::FunEncoding encoding = with_protobuf_ ? fun::FunEncoding::kProtobuf : fun::FunEncoding::kJson;
    uint16_t port = 0;

    if (protocol == fun::TransportProtocol::kTcp) {
      port = with_protobuf_ ? 8022 : 8012;

      /*
      auto option = fun::FunapiTcpTransportOption::create();
      option->SetDisableNagle(true);
      option->SetEnablePing(true);
      session_->Connect(protocol, port, encoding, option);
      */
    }
    else if (protocol == fun::TransportProtocol::kUdp) {
      port = with_protobuf_ ? 8023 : 8013;

      /*
      auto option = fun::FunapiUdpTransportOption::create();
      session_->Connect(protocol, port, encoding, option);
      */
    }
    else if (protocol == fun::TransportProtocol::kHttp) {
      port = with_protobuf_ ? 8028 : 8018;

      /*
      auto option = fun::FunapiHttpTransportOption::create();
      option->SetSequenceNumberValidation(true);
      option->SetUseHttps(false);
      session_->Connect(protocol, port, encoding, option);
      */
    }

    session_->Connect(protocol, port, encoding);
  }

  session_->SetDefaultProtocol(protocol);
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
