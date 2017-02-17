// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin_ue4.h"

#include <sstream>
#include <random>

#include "funapi_tester.h"
#include "test_messages.pb.h"
#include "funapi_tasks.h"
#include "funapi_encryption.h"


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
    UE_LOG(LogFunapiExample, Log, TEXT("You should connect first."));
  }
  else {
    fun::FunEncoding encoding = session_->GetEncoding(session_->GetDefaultProtocol());
    if (encoding == fun::FunEncoding::kNone)
    {
      UE_LOG(LogFunapiExample, Log, TEXT("You should attach transport first."));
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
      TSharedRef<FJsonObject> json_object = MakeShareable(new FJsonObject);
      json_object->SetStringField(FString("name"), FString(name.c_str()));

      // Convert JSON document to string
      FString ouput_fstring;
      TSharedRef<TJsonWriter<TCHAR>> writer = TJsonWriterFactory<TCHAR>::Create(&ouput_fstring);
      FJsonSerializer::Serialize(json_object, writer);
      std::string json_string = TCHAR_TO_ANSI(*ouput_fstring);

      session_->SendMessage("hello", json_string);
    }
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

void Afunapi_tester::Disconnect()
{
  if (session_) {
    session_->Close();
    return;
  }

  UE_LOG(LogFunapiExample, Log, TEXT("You should connect first."));
}

bool Afunapi_tester::SendEchoMessage()
{
  if (session_ == nullptr)
  {
    UE_LOG(LogFunapiExample, Log, TEXT("You should connect first."));
  }
  else
  {
    fun::FunEncoding encoding = session_->GetEncoding(session_->GetDefaultProtocol());
    if (encoding == fun::FunEncoding::kNone)
    {
      UE_LOG(LogFunapiExample, Log, TEXT("You should attach transport first."));
      return false;
    }

    if (encoding == fun::FunEncoding::kProtobuf)
    {
      for (int i = 0; i < 10; ++i) {
        // std::to_string is not supported on android, using std::stringstream instead.
        std::stringstream ss_temp;
        ss_temp << "hello proto - " << static_cast<int>(i);
        std::string temp_string = ss_temp.str();

        FunMessage msg;
        msg.set_msgtype("pbuf_echo");
        PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
        echo->set_msg(temp_string.c_str());

        session_->SendMessage(msg);
      }
    }

    if (encoding == fun::FunEncoding::kJson)
    {
      for (int i = 0; i < 10; ++i) {
        // std::to_string is not supported on android, using std::stringstream instead.
        std::stringstream ss_temp;
        ss_temp <<  "hello world - " << static_cast<int>(i);
        std::string temp_string = ss_temp.str();

        TSharedRef<FJsonObject> json_object = MakeShareable(new FJsonObject);
        json_object->SetStringField(FString("message"), FString(temp_string.c_str()));

        // Convert JSON document to string
        FString ouput_fstring;
        TSharedRef<TJsonWriter<TCHAR>> writer = TJsonWriterFactory<TCHAR>::Create(&ouput_fstring);
        FJsonSerializer::Serialize(json_object, writer);
        std::string json_stiring = TCHAR_TO_ANSI(*ouput_fstring);

        session_->SendMessage("echo", json_stiring);
      }
    }
  }

  return true;
}

bool Afunapi_tester::CreateMulticast()
{
  UE_LOG(LogFunapiExample, Warning, TEXT("CreateMulticast button clicked."));

  if (!multicast_) {
    // random sender id
    std::stringstream ss_temp;
    std::random_device rd;
    std::default_random_engine re(rd());
    std::uniform_int_distribution<int> dist(1, 100);
    ss_temp << "player" << dist(re);
    std::string sender = ss_temp.str();

    UE_LOG(LogFunapiExample, Log, TEXT("sender = %s"), *FString(sender.c_str()));

    fun::FunEncoding encoding = with_protobuf_ ? fun::FunEncoding::kProtobuf : fun::FunEncoding::kJson;
    uint16_t port = with_protobuf_ ? 8122 : 8112;

    multicast_ = fun::FunapiMulticast::Create(sender.c_str(), kServer.c_str(), port, encoding, with_session_reliability_);

    multicast_->AddJoinedCallback([](const std::shared_ptr<fun::FunapiMulticast>& funapi_multicast,
      const std::string &channel_id, const std::string &multicast_sender) {
      UE_LOG(LogFunapiExample, Log, TEXT("JoinedCallback called. channel_id:%s player:%s"), *FString(channel_id.c_str()), *FString(multicast_sender.c_str()));
    });
    multicast_->AddLeftCallback([](const std::shared_ptr<fun::FunapiMulticast>& funapi_multicast,
                                   const std::string &channel_id,
                                   const std::string &multicast_sender) {
      UE_LOG(LogFunapiExample, Log, TEXT("LeftCallback called. channel_id:%s player:%s"), *FString(channel_id.c_str()), *FString(multicast_sender.c_str()));
    });
    multicast_->AddErrorCallback([](const std::shared_ptr<fun::FunapiMulticast>& funapi_multicast,
                                    int error) {
      // EC_ALREADY_JOINED = 1,
      // EC_ALREADY_LEFT,
      // EC_FULL_MEMBER
      // EC_CLOSED
    });
    multicast_->AddChannelListCallback([](const std::shared_ptr<fun::FunapiMulticast>& funapi_multicast,
                                          const std::map<std::string, int> &cl) {
      for (auto i : cl) {
        UE_LOG(LogFunapiExample, Log, TEXT("%s - %d"), *FString(i.first.c_str()), i.second);
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
        UE_LOG(LogFunapiExample, Log, TEXT("Transport Started called."));
      }
      else if (type == fun::TransportEventType::kStopped) {
        UE_LOG(LogFunapiExample, Log, TEXT("Transport Stopped called."));
        multicast_ = nullptr;
      }
      else if (type == fun::TransportEventType::kConnectionFailed) {
        UE_LOG(LogFunapiExample, Log, TEXT("Transport Connection Failed"));
        multicast_ = nullptr;
      }
      else if (type == fun::TransportEventType::kConnectionTimedOut) {
        UE_LOG(LogFunapiExample, Log, TEXT("Transport Connection Timedout called"));
        multicast_ = nullptr;
      }
      else if (type == fun::TransportEventType::kDisconnected) {
        UE_LOG(LogFunapiExample, Log, TEXT("Transport Disconnected called"));
        multicast_ = nullptr;
      }
    });

    multicast_->AddJsonChannelMessageCallback(kMulticastTestChannel,
      [this](
        const std::shared_ptr<fun::FunapiMulticast>& funapi_multicast,
        const std::string &channel_id,
        const std::string &sender,
        const std::string &json_string)
    {
      UE_LOG(LogFunapiExample, Log, TEXT("channel_id = %s, sender = %s, body = %s"), *FString(channel_id.c_str()), *FString(sender.c_str()), *FString(json_string.c_str()));
    });

    multicast_->AddProtobufChannelMessageCallback(kMulticastTestChannel,
      [this](
        const std::shared_ptr<fun::FunapiMulticast> &funapi_multicast,
        const std::string &channel_id,
        const std::string &sender,
        const FunMessage& message)
    {
      FunMulticastMessage mcast_msg = message.GetExtension(multicast);
      FunChatMessage chat_msg = mcast_msg.GetExtension(chat);
      std::string text = chat_msg.text();

      UE_LOG(LogFunapiExample, Log, TEXT("channel_id = %s, sender = %s, message = %s"), *FString(channel_id.c_str()), *FString(sender.c_str()), *FString(text.c_str()));
    });

    multicast_->Connect();
  }

  return true;
}

bool Afunapi_tester::JoinMulticastChannel()
{
  UE_LOG(LogFunapiExample, Log, TEXT("JoinMulticastChannel button clicked."));

  if (multicast_) {
    if (!multicast_->IsInChannel(kMulticastTestChannel)) {
      multicast_->JoinChannel(kMulticastTestChannel);
    }
  }

  return true;
}

bool Afunapi_tester::SendMulticastMessage()
{
  UE_LOG(LogFunapiExample, Log, TEXT("SendMulticastMessage button clicked."));

  if (multicast_) {
    if (multicast_->IsConnected() && multicast_->IsInChannel(kMulticastTestChannel)) {
      if (multicast_->GetEncoding() == fun::FunEncoding::kJson) {
        std::string temp_messsage = "multicast test message";

        TSharedRef<FJsonObject> json_object = MakeShareable(new FJsonObject);
        json_object->SetStringField(FString("message"), FString(temp_messsage.c_str()));

        // Convert JSON document to string
        FString ouput_fstring;
        TSharedRef<TJsonWriter<TCHAR>> writer = TJsonWriterFactory<TCHAR>::Create(&ouput_fstring);
        FJsonSerializer::Serialize(json_object, writer);
        std::string json_string = TCHAR_TO_ANSI(*ouput_fstring);

        multicast_->SendToChannel(kMulticastTestChannel, json_string);
      }

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

bool Afunapi_tester::LeaveMulticastChannel()
{
  UE_LOG(LogFunapiExample, Log, TEXT("LeaveMulticastChannel button clicked."));

  if (multicast_) {
    if (multicast_->IsConnected() && multicast_->IsInChannel(kMulticastTestChannel)) {
      multicast_->LeaveChannel(kMulticastTestChannel);
    }
  }

  return true;
}

bool Afunapi_tester::RequestMulticastChannelList()
{
  UE_LOG(LogFunapiExample, Log, TEXT("RequestMulticastChannelList button clicked."));

  if (multicast_) {
    if (multicast_->IsConnected()) {
      multicast_->RequestChannelList();
    }
  }

  return true;
}

bool Afunapi_tester::LeaveMulticastAllChannels()
{
  UE_LOG(LogFunapiExample, Log, TEXT("Leave all channels button clicked."));

  if (multicast_) {
    multicast_->LeaveAllChannels();
  }

  return true;
}

bool Afunapi_tester::DownloaderTest()
{
  UE_LOG(LogFunapiExample, Log, TEXT("Download button clicked."));

  if (!downloader_) {
    std::stringstream ss_download_url;
    ss_download_url << "http://" << kDownloadServer << ":" << kDownloadServerPort;

    downloader_ = fun::FunapiHttpDownloader::Create(ss_download_url.str(),
                                                    TCHAR_TO_UTF8(*(FPaths::GameSavedDir())));

    downloader_->AddReadyCallback([](const std::shared_ptr<fun::FunapiHttpDownloader>&downloader,
                                     const std::vector<std::shared_ptr<fun::FunapiDownloadFileInfo>>&info) {
       for (auto i : info) {
         std::stringstream ss_temp;
         ss_temp << i->GetUrl() << std::endl;
         UE_LOG(LogFunapiExample, Log, TEXT("%s"), *FString(ss_temp.str().c_str()));
       }
    });

    downloader_->AddProgressCallback([](const std::shared_ptr<fun::FunapiHttpDownloader> &downloader,
                                        const std::vector<std::shared_ptr<fun::FunapiDownloadFileInfo>>&info,
                                        const int index,
                                        const int max_index,
                                        const uint64_t received_bytes,
                                        const uint64_t expected_bytes) {
      auto i = info[index];

      std::stringstream ss_temp;
      ss_temp << index << "/" << max_index << " " << received_bytes << "/" << expected_bytes << " " << i->GetUrl() << std::endl;
      UE_LOG(LogFunapiExample, Log, TEXT("%s"), *FString(ss_temp.str().c_str()));
    });

    downloader_->AddCompletionCallback([this](const std::shared_ptr<fun::FunapiHttpDownloader>&downloader,
                                              const std::vector<std::shared_ptr<fun::FunapiDownloadFileInfo>>&info,
                                              const fun::FunapiHttpDownloader::ResultCode result_code) {
      if (result_code == fun::FunapiHttpDownloader::ResultCode::kSucceed) {
        for (auto i : info) {
          UE_LOG(LogFunapiExample, Log, TEXT("file_path=%s"), *FString(i->GetPath().c_str()));
        }
      }

      downloader_ = nullptr;
    });

    downloader_->Start();
  }

  return true;
}

bool Afunapi_tester::RequestAnnouncements()
{
  UE_LOG(LogFunapiExample, Log, TEXT("Request announcements button clicked."));

  if (announcement_ == nullptr) {
    std::stringstream ss_url;
    ss_url << "http://" << kAnnouncementServer << ":" << kAnnouncementServerPort;

    announcement_ = fun::FunapiAnnouncement::Create(ss_url.str(),
                                                    TCHAR_TO_UTF8(*(FPaths::GameSavedDir())));

    announcement_->AddCompletionCallback([this](const std::shared_ptr<fun::FunapiAnnouncement> &announcement,
                                                const std::vector<std::shared_ptr<fun::FunapiAnnouncementInfo>>&info,
                                                const fun::FunapiAnnouncement::ResultCode result){
      if (result == fun::FunapiAnnouncement::ResultCode::kSucceed) {
        for (auto i : info) {
          UE_LOG(LogFunapiExample, Log, TEXT("date=%s message=%s subject=%s file_path=%s"), *FString(i->GetDate().c_str()), *FString(i->GetMessageText().c_str()), *FString(i->GetSubject().c_str()), *FString(i->GetFilePath().c_str()));
        }
      }

      announcement_ = nullptr;
    });
  }

  announcement_->RequestList(5);

  return true;
}

FText Afunapi_tester::GetServerIP() {
  return FText::FromString(FString(kServer.c_str()));
}

void Afunapi_tester::SetServerIP(FText server_ip) {
  kServer = TCHAR_TO_UTF8(*(server_ip.ToString()));
}

bool Afunapi_tester::GetIsProtobuf() {
  return with_protobuf_;
}

void Afunapi_tester::SetIsProtobuf(bool check) {
  with_protobuf_ = check;
}

bool Afunapi_tester::GetIsSessionReliability() {
  return with_session_reliability_;
}

void Afunapi_tester::SetIsSessionReliability(bool check) {
  with_session_reliability_ = check;
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

void Afunapi_tester::Connect(const fun::TransportProtocol protocol)
{
  if (!session_) {
    // create
    session_ = fun::FunapiSession::Create(kServer.c_str(), with_session_reliability_);

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
        UE_LOG(LogFunapiExample, Log, TEXT("Transport Started called."));
      }
      else if (type == fun::TransportEventType::kStopped) {
        UE_LOG(LogFunapiExample, Log, TEXT("Transport Stopped called."));
      }
      else if (type == fun::TransportEventType::kConnectionFailed) {
        UE_LOG(LogFunapiExample, Log, TEXT("Transport Connection Failed (%s)"), *FString(fun::TransportProtocolToString(transport_protocol).c_str()));
        session_ = nullptr;
      }
      else if (type == fun::TransportEventType::kConnectionTimedOut) {
        UE_LOG(LogFunapiExample, Log, TEXT("Transport Connection Timedout called"));
        session_ = nullptr;
      }
      else if (type == fun::TransportEventType::kDisconnected) {
        UE_LOG(LogFunapiExample, Log, TEXT("Transport Disconnected called (%s)"), *FString(fun::TransportProtocolToString(transport_protocol).c_str()));
      }
    });

    session_->AddJsonRecvCallback([](const std::shared_ptr<fun::FunapiSession> &session,
                                     const fun::TransportProtocol transport_protocol,
                                     const std::string &msg_type,
                                     const std::string &json_string) {
      if (msg_type.compare("echo") == 0) {
        UE_LOG(LogFunapiExample, Log, TEXT("msg '%s' arrived."), *FString(msg_type.c_str()));
        UE_LOG(LogFunapiExample, Log, TEXT("json: %s"), *FString(json_string.c_str()));
      }

      if (msg_type.compare("_maintenance") == 0) {
        UE_LOG(LogFunapiExample, Log, TEXT("Maintenance message : %s"), *FString(json_string.c_str()));
      }
    });

    session_->AddProtobufRecvCallback([](const std::shared_ptr<fun::FunapiSession> &session,
                                         const fun::TransportProtocol transport_protocol,
                                         const FunMessage &fun_message) {
      if (fun_message.msgtype().compare("pbuf_echo") == 0) {
        UE_LOG(LogFunapiExample, Log, TEXT("msg '%s' arrived."), *FString(fun_message.msgtype().c_str()));

        PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);
        UE_LOG(LogFunapiExample, Log, TEXT("proto: %s"), *FString(echo.msg().c_str()));
      }

      if (fun_message.msgtype().compare("_maintenance") == 0) {
        UE_LOG(LogFunapiExample, Log, TEXT("msg '%s' arrived."), *FString(fun_message.msgtype().c_str()));

        PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);
        UE_LOG(LogFunapiExample, Log, TEXT("proto: %s"), *FString(echo.msg().c_str()));

        MaintenanceMessage maintenance = fun_message.GetExtension(pbuf_maintenance);
        std::string date_start = maintenance.date_start();
        std::string date_end = maintenance.date_end();
        std::string message_text = maintenance.messages();

        UE_LOG(LogFunapiExample, Log, TEXT("Maintenance message:\nstart: %s\nend: %s\nmessage: %s"), *FString(date_start.c_str()), *FString(date_end.c_str()), *FString(message_text.c_str()));
      }
    });

    /*
    session_->SetTransportOptionCallback([](const fun::TransportProtocol protocol,
                                            const std::string &flavor) -> std::shared_ptr<fun::FunapiTransportOption> {
      if (protocol == fun::TransportProtocol::kTcp) {
        auto option = fun::FunapiTcpTransportOption::Create();
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
      auto option = fun::FunapiTcpTransportOption::Create();
      option->SetDisableNagle(true);
      option->SetEnablePing(true);
      option->SetEncryptionType(fun::EncryptionType::kChacha20Encryption, "0b8504a9c1108584f4f0a631ead8dd548c0101287b91736566e13ead3f008f5d");
      session_->Connect(protocol, port, encoding, option);
      */
    }
    else if (protocol == fun::TransportProtocol::kUdp) {
      port = with_protobuf_ ? 8023 : 8013;

      /*
      auto option = fun::FunapiUdpTransportOption::Create();
      session_->Connect(protocol, port, encoding, option);
      */
    }
    else if (protocol == fun::TransportProtocol::kHttp) {
      port = with_protobuf_ ? 8028 : 8018;

      /*
      auto option = fun::FunapiHttpTransportOption::Create();
      option->SetSequenceNumberValidation(true);
      option->SetUseHttps(false);
      session_->Connect(protocol, port, encoding, option);
      */
    }

    session_->Connect(protocol, port, encoding);
  }

  session_->SetDefaultProtocol(protocol);
}

void Afunapi_tester::OnSessionInitiated(const std::string &session_id)
{
  UE_LOG(LogFunapiExample, Log, TEXT("Session initiated: %s"), *FString(session_id.c_str()));
}

void Afunapi_tester::OnSessionClosed()
{
  UE_LOG(LogFunapiExample, Log, TEXT("Session closed"));
}
