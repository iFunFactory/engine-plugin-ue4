// Copyright (C) 2013-2018 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_SESSION_H_
#define SRC_FUNAPI_SESSION_H_

#include "funapi_plugin.h"
#include "funapi_option.h"
#include "funapi_encryption.h"
#include "funapi_compression.h"
#include "funapi/network/ping_message.pb.h"
#include "funapi/service/redirect_message.pb.h"
#include "funapi/management/maintenance_message.pb.h"

#define kLengthHeaderField "LEN"
#define kMaintenanceMessageType "_maintenance"


namespace fun
{

enum class FUNAPI_API TransportState : int
{
    kDisconnected = 0,
    kDisconnecting,
    kConnecting,
    kConnected
};

// Funapi transport protocol
enum class FUNAPI_API TransportProtocol : int
{
    kTcp = 0,
    kUdp,
    kHttp,
#if FUNAPI_HAVE_WEBSOCKET
    kWebsocket,
#endif
    kDefault,
};

// Message encoding type
enum class FUNAPI_API FunEncoding
{
    kNone,
    kJson,
    kProtobuf
};

enum class FUNAPI_API SessionEventType : int
{
    kOpened,
    kClosed,
    kChanged,
    kRedirectStarted,
    kRedirectSucceeded,
    kRedirectFailed,
};

enum class FUNAPI_API TransportEventType : int
{
    kStarted,
    kStopped,
    kReconnecting,
    kConnectionFailed,
    kConnectionTimedOut,
    kDisconnected,
};


extern FUNAPI_API fun::string TransportProtocolToString(TransportProtocol protocol);


class FunapiSessionOption;
class FunapiSessionImpl;
class FunapiUnsentMessage;
class FUNAPI_API FunapiSession : public std::enable_shared_from_this<FunapiSession>
{
public:
    typedef std::function<void(const std::shared_ptr<FunapiSession>&,
                               const TransportProtocol,
                               const fun::string&,
                               const fun::string&)> JsonRecvHandler;

    typedef std::function<void(const std::shared_ptr<FunapiSession>&,
                               const TransportProtocol,
                               const FunMessage&)> ProtobufRecvHandler;

    typedef std::function<void(const std::shared_ptr<FunapiSession>&,
                               const TransportProtocol,
                               const SessionEventType,
                               const fun::string&,
                               const std::shared_ptr<FunapiError>&)> SessionEventHandler;

    typedef std::function<void(const std::shared_ptr<FunapiSession>&,
                               const TransportProtocol,
                               const TransportEventType,
                               const std::shared_ptr<FunapiError>&)> TransportEventHandler;

    typedef std::function<void(const std::shared_ptr<FunapiSession>&,
                               const fun::string&)> RecvTimeoutHandler;

    typedef std::function<void(const std::shared_ptr<FunapiSession>&,
                               const int32_t)> RecvTimeoutIntHandler;

    typedef std::function<std::shared_ptr<FunapiTransportOption>(const TransportProtocol,
                                                                 const fun::string&)> TransportOptionHandler;

    typedef std::function<std::shared_ptr<FunapiSessionOption>(const fun::string&)> SessionOptionHandler;

    typedef std::function<void(const TransportProtocol,
                               const fun::vector<fun::string>&, const fun::vector<fun::string>&,
                               const fun::deque<std::shared_ptr<FunapiUnsentMessage>>&)> RedirectQueueHandler;

    FunapiSession() = delete;
    FunapiSession(const char* hostname_or_ip, std::shared_ptr<FunapiSessionOption> option);
    virtual ~FunapiSession();

    static std::shared_ptr<FunapiSession> Create(const char* hostname_or_ip,
                                                 bool reliability = false);
    static std::shared_ptr<FunapiSession> Create(const char* hostname_or_ip,
                                                 std::shared_ptr<FunapiSessionOption> option);

    void Connect(const TransportProtocol protocol,
                 int port,
                 FunEncoding encoding,
                 std::shared_ptr<FunapiTransportOption> option = nullptr);
    void Connect(const TransportProtocol protocol);

    void Close(const TransportProtocol protocol);
    void Close();

    void SendMessage(const fun::string &msg_type,
                     const fun::string &json_string,
                     const TransportProtocol protocol = TransportProtocol::kDefault,
                     const EncryptionType encryption_type = EncryptionType::kDefaultEncryption);

    void SendMessage(const FunMessage &message,
                     const TransportProtocol protocol = TransportProtocol::kDefault,
                     const EncryptionType encryption_type = EncryptionType::kDefaultEncryption);

    bool IsConnected(const TransportProtocol protocol) const;
    bool IsConnected() const;
    bool IsReliableSession() const;

    FunEncoding GetEncoding(const TransportProtocol protocol) const;
    bool HasTransport(const TransportProtocol protocol) const;
    void Update();

    void SetDefaultProtocol(const TransportProtocol protocol);
    TransportProtocol GetDefaultProtocol() const;

    int64_t GetPingTime();

    void AddSessionEventCallback(const SessionEventHandler &handler);
    void AddTransportEventCallback(const TransportEventHandler &handler);
    void AddProtobufRecvCallback(const ProtobufRecvHandler &handler);
    void AddJsonRecvCallback(const JsonRecvHandler &handler);
    void AddRecvTimeoutCallback(const RecvTimeoutHandler &handler);
    void AddRecvTimeoutCallback(const RecvTimeoutIntHandler &handler);

    void SetSessionOptionCallback(const SessionOptionHandler &handler);
    void SetTransportOptionCallback(const TransportOptionHandler &handler);
    void SetRedirectQueueCallback(const RedirectQueueHandler &handler);

    void RemoveSessionEventCallback();
    void RemoveTransportEventCallback();
    void RemoveProtobufRecvCallback();
    void RemoveJsonRecvCallback();
    void RemoveRecvTimeoutCallback();
    void RemoveRecvTimeoutIntCallback();
    void RemoveSessionOptionCallback();
    void RemoveTransportOptionCallback();
    void RemoveRedirectQueueCallback();

    void RemoveAllCallbacks();

    void SetRecvTimeout(const fun::string &msg_type, const int seconds);
    void EraseRecvTimeout(const fun::string &msg_type);

    void SetRecvTimeout(const int32_t msg_type, const int seconds);
    void EraseRecvTimeout(const int32_t msg_type);

    static void UpdateAll();

private:
    std::shared_ptr<FunapiSessionImpl> impl_;
};


class FUNAPI_API FunapiUnsentMessage : public std::enable_shared_from_this<FunapiUnsentMessage>
{
public:
    FunapiUnsentMessage() = default;
    virtual ~ FunapiUnsentMessage() = default;

    virtual const fun::string& GetMsgType() = 0;
    virtual int32_t GetMsgType2() = 0;

    virtual void SetDiscard(const bool discard) = 0;
};

}  // namespace fun

#endif  // SRC_FUNAPI_SESSION_H_
