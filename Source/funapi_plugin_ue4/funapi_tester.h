// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#pragma once

#include <memory>
#include "GameFramework/Actor.h"
#include "funapi_tester.generated.h"

namespace fun {
  class FunapiAnnouncement;
  class FunapiSession;
  class FunapiMulticast;
  enum class TransportProtocol : int;
}

UCLASS()
class FUNAPI_PLUGIN_UE4_API Afunapi_tester : public AActor
{
  GENERATED_BODY()
 public:
  // Sets default values for this actor's properties
  Afunapi_tester();
  virtual ~Afunapi_tester();

  // Called when the game starts or when spawned
  virtual void BeginPlay() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
  virtual void Tick(float DeltaTime);

  UFUNCTION(BlueprintCallable, Category="Funapi")
  bool ConnectTcp();

  UFUNCTION(BlueprintCallable, Category="Funapi")
  bool ConnectUdp();

  UFUNCTION(BlueprintCallable, Category="Funapi")
  bool ConnectHttp();

  UFUNCTION(BlueprintCallable, Category="Funapi")
  void Disconnect();

  UFUNCTION(BlueprintCallable, Category="Funapi")
  bool SendEchoMessage();

  UFUNCTION(BlueprintCallable, Category="Funapi")
  bool FileDownload();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool CreateMulticast();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool JoinMulticastChannel();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool SendMulticastMessage();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool LeaveMulticastChannel();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool RequestMulticastChannelList();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool LeaveMulticastAllChannels();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool RequestAnnouncements();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  FText GetServerIP();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  void SetServerIP(FText server_ip);

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool GetIsEnableTextboxServerIP();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool GetIsProtobuf();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  void SetIsProtobuf(bool check);

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool GetIsEnableCheckboxProtobuf();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool GetIsSessionReliability();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  void SetIsSessionReliability(bool check);

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool GetIsEnableCheckboxSessionReliability();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool GetIsEnableButtonConnectTcp();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool GetIsEnableButtonConnectUdp();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool GetIsEnableButtonConnectHttp();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool GetIsEnableButtonSendEchoMessage();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool GetIsEnableButtonDisconnect();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool GetIsEnableButtonCreateMulticast();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool GetIsEnableButtonJoinChannel();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool GetIsEnableButtonSendMulticastMessage();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool GetIsEnableButtonLeaveChannel();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool GetIsEnableButtonRequestList();

  UFUNCTION(BlueprintCallable, Category = "Funapi")
  bool GetIsEnableButtonLeaveAllChannels();

  // callback
  void OnSessionInitiated(const std::string &session_id);
  void OnSessionClosed();

 private:
  void Connect(const fun::TransportProtocol protocol);
  void UpdateUI();
  void SendRedirectTestMessage();

  // Please change this address for test.
  std::string kServerIp = "127.0.0.1";

  // member variables.
  bool with_protobuf_ = false;
  bool with_session_reliability_ = false;

  std::shared_ptr<fun::FunapiSession> session_ = nullptr;

  const std::string kMulticastTestChannel = "multicast";
  std::shared_ptr<fun::FunapiMulticast> multicast_ = nullptr;

  // Please change this address for test.
  const std::string kAnnouncementServer = "127.0.0.1";
  const int kAnnouncementPort = 8080;
  std::shared_ptr<fun::FunapiAnnouncement> announcement_ = nullptr;

  bool EnableTextboxServerIP = true;
  bool EnableCheckboxProtobuf = true;
  bool EnableCheckboxSessionReliability = true;
  bool EnableButtonConnectTcp = true;
  bool EnableButtonConnectUdp = true;
  bool EnableButtonConnectHttp = true;
  bool EnableButtonSendEchoMessage = true;
  bool EnableButtonDisconnect = true;
  bool EnableButtonCreateMulticast = true;
  bool EnableButtonJoinChannel = true;
  bool EnableButtonSendMulticastMessage = true;
  bool EnableButtonLeaveChannel = true;
  bool EnableButtonRequestList = true;
  bool EnableButtonLeaveAllChannels = true;
};
