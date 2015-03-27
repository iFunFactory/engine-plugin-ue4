// Fill out your copyright notice in the Description page of Project Settings.

#include "funapi_plugin_ue4.h"
#include "funapi_tester.h"

#include "funapi/funapi_network.h"
#include "funapi/funapi_downloader.h"
#include "funapi/pbuf_echo.pb.h"

#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"


namespace
{
    const char kServerIp[] = "127.0.0.1";
    
    fun::FunapiNetwork* network = NULL;
    int8 msg_type = fun::kJsonEncoding;
    bool is_downloading = false;

    
    void on_session_initiated (const std::string &session_id, void *ctxt)
    {
        LOG1("session initiated: %s", *FString(session_id.c_str()));
    }
    
    void on_session_closed (void *ctxt)
    {
        LOG("session closed");
    }
    
    void on_echo_json (const std::string &type, const std::string &body, void *ctxt)
    {
        LOG1("msg '%s' arrived.", *FString(type.c_str()));
        LOG1("json: %s", *FString(body.c_str()));
    }
    
    void on_echo_proto (const std::string &type, const std::string &body, void *ctxt)
    {
        LOG1("msg '%s' arrived.", *FString(type.c_str()));

        FunMessage msg;
        msg.ParseFromString(body);
        PbufEchoMessage echo = msg.GetExtension(pbuf_echo);
        LOG1("proto: %s", *FString(echo.message().c_str()));
    }

    void on_download_update (const std::string &path, long receive, long total, int percent, void *ctxt)
    {
    }
    
    void on_download_finished (int result, void *ctxt)
    {
        is_downloading = false;
    }
    
    bool connect_to_server (fun::FunapiTransport *transport)
    {
        if (transport == NULL)
            return false;
        
        network = new fun::FunapiNetwork(transport, msg_type,
                                         fun::FunapiNetwork::OnSessionInitiated(on_session_initiated, NULL),
                                         fun::FunapiNetwork::OnSessionClosed(on_session_closed, NULL));
        
        if (msg_type == fun::kJsonEncoding)
            network->RegisterHandler("echo", fun::FunapiNetwork::MessageHandler(on_echo_json, NULL));
        else if (msg_type == fun::kProtobufEncoding)
            network->RegisterHandler("pbuf_echo", fun::FunapiNetwork::MessageHandler(on_echo_proto, NULL));
        
        network->Start();
        return true;
    }
}


// Sets default values
Afunapi_tester::Afunapi_tester()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void Afunapi_tester::BeginPlay()
{
	Super::BeginPlay();
    
    fun::FunapiNetwork::Initialize();
}

void Afunapi_tester::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    fun::FunapiNetwork::Finalize();
}

bool Afunapi_tester::ConnectTcp()
{
    connect_to_server(new fun::FunapiTcpTransport(kServerIp, 8012));
    return true;
}

bool Afunapi_tester::ConnectUdp()
{
    connect_to_server(new fun::FunapiUdpTransport(kServerIp, 8013));
    return true;
}

bool Afunapi_tester::ConnectHttp()
{
    connect_to_server(new fun::FunapiHttpTransport(kServerIp, 8018));
    return true;
}

bool Afunapi_tester::IsConnected()
{
    return network != NULL && network->Connected();
}

void Afunapi_tester::Disconnect()
{
    if (network == NULL || network->Started() == false)
    {
        LOG("You should connect first.");
        return;
    }

    network->Stop();
}

bool Afunapi_tester::SendEchoMessage()
{
    if (network == NULL || network->Started() == false)
    {
        LOG("You should connect first.");
        return false;
    }
    
    if (msg_type == fun::kJsonEncoding)
    {
        fun::Json msg;
        msg.SetObject();
        rapidjson::Value message_node("hello world", msg.GetAllocator());
        msg.AddMember("message", message_node, msg.GetAllocator());
        network->SendMessage("echo", msg);
        return true;
    }
    else if (msg_type == fun::kProtobufEncoding)
    {
        FunMessage msg;
        msg.set_msgtype("pbuf_echo");
        PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
        echo->set_message("hello proto");
        network->SendMessage(msg);
        return true;
    }
    
    return false;
}

bool Afunapi_tester::FileDownload()
{
    fun::FunapiHttpDownloader downloader("/Users/arin/work/resource_test",//"/home/anonymous/resource",
                                               fun::FunapiHttpDownloader::OnUpdate(on_download_update, this),
                                               fun::FunapiHttpDownloader::OnFinished(on_download_finished, this));
    
    is_downloading = true;
    downloader.StartDownload(kServerIp, 8020, "list");
    
    while (is_downloading) {
        sleep(1);
    }
    
    return true;
}
