// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#pragma once

#include <memory>
#include "GameFramework/Actor.h"
#include "funapi_network.h"
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

    // callback
    void OnSessionInitiated(const std::string &session_id);
    void OnSessionClosed();
    void OnEchoJson(const std::string &type, const std::vector<uint8_t> &v_body);
    void OnEchoProto(const std::string &type, const std::vector<uint8_t> &v_body);

private:
  void Connect(const fun::TransportProtocol protocol);
  std::shared_ptr<fun::FunapiTransport> GetNewTransport(fun::TransportProtocol protocol);

  const std::string kServerIp = "127.0.0.1";
  std::unique_ptr<fun::FunapiNetwork> network_;
  int8 msg_type_ = fun::kJsonEncoding;
  fun::TransportProtocol protocol_ = fun::TransportProtocol::kDefault;
};
