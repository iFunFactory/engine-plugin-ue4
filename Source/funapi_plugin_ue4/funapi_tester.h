// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#pragma once

#include <memory>
#include "GameFramework/Actor.h"
#include "funapi_network.h"
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
  bool IsConnected();

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

  // callback
  void OnSessionInitiated(const std::string &session_id);
  void OnSessionClosed();
  void OnEchoJson(const fun::TransportProtocol protocol, const std::string &type, const std::vector<uint8_t> &v_body);
  void OnEchoProto(const fun::TransportProtocol protocol, const std::string &type, const std::vector<uint8_t> &v_body);

  void OnMaintenanceMessage(const fun::TransportProtocol protocol, const std::string &type, const std::vector<uint8_t> &v_body);
  void OnStoppedAllTransport();
  void OnTransportConnectFailed(const fun::TransportProtocol protocol);
  void OnTransportConnectTimeout(const fun::TransportProtocol protocol);

 private:
  void Connect(const fun::TransportProtocol protocol);
  std::shared_ptr<fun::FunapiTransport> GetNewTransport(const fun::TransportProtocol protocol);

  // Please change this address for test.
  const std::string kServerIp = "127.0.0.1";

  // member variables.
  bool with_protobuf_ = false;
  bool with_session_reliability_ = false;

  std::shared_ptr<fun::FunapiNetwork> network_ = nullptr;

  const std::string kMulticastTestChannel = "multicast";
  std::shared_ptr<fun::FunapiMulticastClient> multicast_ = nullptr;
  fun::FunEncoding multicast_encoding_;

  void OnMulticastChannelSignalle(const std::string &channel_id, const std::string &sender, const std::vector<uint8_t> &v_body);
};
