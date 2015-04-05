// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "funapi_tester.generated.h"

UCLASS()
class FUNAPI_PLUGIN_UE4_API Afunapi_tester : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    Afunapi_tester();

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
};
