// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#pragma once

#include <memory>
#include "GameFramework/Actor.h"
#include "funapi_session.h"
#include "funapi_multicasting.h"
#include "funapi_tester.generated.h"

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

  // callback
  void OnSessionInitiated(const std::string &session_id);
  void OnSessionClosed();

 private:
  void Connect(const fun::TransportProtocol protocol);

  // Please change this address for test.
  const std::string kServerIp = "127.0.0.1";

  // member variables.
  bool with_protobuf_ = false;
  bool with_session_reliability_ = false;

  std::shared_ptr<fun::FunapiSession> session_ = nullptr;

  const std::string kMulticastTestChannel = "multicast";
  std::shared_ptr<fun::FunapiMulticast> multicast_ = nullptr;
};
