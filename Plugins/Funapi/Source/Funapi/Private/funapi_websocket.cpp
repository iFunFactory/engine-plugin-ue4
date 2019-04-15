// Copyright (C) 2013-2018 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifdef FUNAPI_UE4
#include "FunapiPrivatePCH.h"
#endif

#include "funapi_websocket.h"
#include "funapi_utils.h"

#if FUNAPI_HAVE_WEBSOCKET
#ifdef FUNAPI_UE4
#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif
// Work around a conflict between a UI namespace defined by engine code and a typedef in OpenSSL
#define UI UI_ST
THIRD_PARTY_INCLUDES_START
#include "libwebsockets.h"
THIRD_PARTY_INCLUDES_END
#undef UI
#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif
#else // FUNAPI_UE4
#include "libwebsockets.h"
#endif // FUNAPI_UE4
#endif // FUNAPI_HAVE_WEBSOCKET

namespace fun {

#define FUNAPI_WEBSOCKET_MAX_SEND_BUFFER 65536

////////////////////////////////////////////////////////////////////////////////
// FunapiWebsocketImpl implementation.

class FunapiWebsocketImpl : public std::enable_shared_from_this<FunapiWebsocketImpl> {
 public:
  typedef FunapiWebsocket::ConnectCompletionHandler ConnectCompletionHandler;
  typedef FunapiWebsocket::RecvHandler RecvHandler;
  typedef FunapiWebsocket::SendHandler SendHandler;
  typedef FunapiWebsocket::CloseHandler CloseHandler;
  typedef FunapiWebsocket::SendCompletionHandler SendCompletionHandler;

  FunapiWebsocketImpl();
  virtual ~FunapiWebsocketImpl();

  void Connect(const char* hostname_or_ip,
               const int sever_port,
               const bool use_wss,
               const std::string &ca_file_path,
               const ConnectCompletionHandler &connect_completion_handler,
               const CloseHandler &close_handler,
               const SendHandler &send_handler,
               const RecvHandler &recv_handler);

  bool Send(const std::vector<uint8_t> &body,
            const bool is_binary,
            const SendCompletionHandler &send_completion_handler);

  void Update();
#if FUNAPI_HAVE_WEBSOCKET
  int OnCallback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);
#endif

  static void Log(int level, const char *line);
#if FUNAPI_HAVE_WEBSOCKET
  static int Callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);
#endif

 protected:
  void Init();
  void Cleanup();
  void OnSend();

  ConnectCompletionHandler connect_completion_handler_;
  CloseHandler close_handler_;
  SendHandler send_handler_;
  RecvHandler recv_handler_;
  SendCompletionHandler send_completion_handler_;

#if FUNAPI_HAVE_WEBSOCKET
  struct lws_context *context_ = nullptr;
  struct lws *web_socket_ = nullptr;
  struct lws_protocols *protocols_ = nullptr;
#endif

  uint8_t send_buffer_[FUNAPI_WEBSOCKET_MAX_SEND_BUFFER];
  size_t send_buffer_length_ = 0;
  size_t offset_ = 0;
  bool is_binary_;
};


FunapiWebsocketImpl::FunapiWebsocketImpl() {
  Init();
}


FunapiWebsocketImpl::~FunapiWebsocketImpl() {
  // DebugUtils::Log("%s", __FUNCTION__);
  Cleanup();
}


void FunapiWebsocketImpl::Log(int level, const char *line) {
  DebugUtils::Log("[libwebsocket][%d] %s", level, line);
}


#if FUNAPI_HAVE_WEBSOCKET
int FunapiWebsocketImpl::Callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
  int ret = 0;

  FunapiWebsocketImpl* impl = (FunapiWebsocketImpl*)lws_wsi_user(wsi);
  if (impl) {
    ret = impl->OnCallback(wsi, reason, user, in, len);
  }

  return ret;
}
#endif


#if FUNAPI_HAVE_WEBSOCKET
int FunapiWebsocketImpl::OnCallback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
  switch (reason)
  {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
      connect_completion_handler_(false, 0, "LWS_CALLBACK_CLIENT_ESTABLISHED");
      break;

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
      connect_completion_handler_(true, static_cast<int>(reason), "LWS_CALLBACK_CLIENT_CONNECTION_ERROR");
      break;

    case LWS_CALLBACK_WSI_DESTROY:
      close_handler_(static_cast<int>(reason), "LWS_CALLBACK_WSI_DESTROY");
      break;

    case LWS_CALLBACK_CLIENT_RECEIVE:
    {
      std::vector<uint8_t> receiving(len);
      memcpy (receiving.data(), in, len);
      recv_handler_(static_cast<int>(len), receiving);
    }
      break;

    case LWS_CALLBACK_CLIENT_WRITEABLE:
      web_socket_ = wsi;
      OnSend();
      break;

    default:
      break;
  }

  return 0;
}
#endif


void FunapiWebsocketImpl::Init() {
#if FUNAPI_HAVE_WEBSOCKET

#ifdef FUNAPI_UE4_PLATFORM_PS4
#if (WEBSOCKETS_PACKAGE == 0)
#error UE4 Websockets required
#endif
#endif // FUNAPI_UE4_PLATFORM_PS4

#ifdef DEBUG_LOG
  static auto lws_log_init = FunapiInit::Create([]()
  {
    int level = 0;
#ifdef FUNAPI_UE4_PLATFORM_PS4
    level = LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_HEADER | LLL_EXT | LLL_CLIENT | LLL_LATENCY;
#else
    level = LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG | LLL_HEADER | LLL_EXT | LLL_CLIENT | LLL_LATENCY;
#endif
    lws_set_log_level(level, &FunapiWebsocketImpl::Log);
  });
#endif // DEBUG_LOG

#endif // FUNAPI_HAVE_WEBSOCKET
}


void FunapiWebsocketImpl::Cleanup() {
#if FUNAPI_HAVE_WEBSOCKET
  if (context_) {
    lws_context_destroy(context_);
    context_ = nullptr;
  }
#endif
}


void FunapiWebsocketImpl::Update() {
#if FUNAPI_HAVE_WEBSOCKET
  if (context_) {
    lws_service(context_, 2);
    lws_callback_on_writable_all_protocol(context_, protocols_);
  }
#endif
}


void FunapiWebsocketImpl::Connect(const char* hostname_or_ip,
                                  const int server_port,
                                  const bool use_wss,
                                  const std::string &ca_file_path,
                                  const ConnectCompletionHandler &connect_completion_handler,
                                  const CloseHandler &close_handler,
                                  const SendHandler &send_handler,
                                  const RecvHandler &recv_handler) {
#if FUNAPI_HAVE_WEBSOCKET
  if (context_) {
    return;
  }

  connect_completion_handler_ = connect_completion_handler;
  close_handler_ = close_handler;
  send_handler_ = send_handler;
  recv_handler_ = recv_handler;

  static struct lws_protocols protocols[] =
  {
    {
      "ws",
      &FunapiWebsocketImpl::Callback,
      0,
      65536,
    },
    {
      "wss",
      &FunapiWebsocketImpl::Callback,
      0,
      65536,
    },
    { NULL, NULL, 0, 0 } /* terminator */
  };

  protocols_ = &protocols[0];

  struct lws_context_creation_info context_info;
  memset(&context_info, 0, sizeof(context_info));
  context_info.port = CONTEXT_PORT_NO_LISTEN;
  context_info.protocols = protocols;
  context_info.gid = -1;
  context_info.uid = -1;

#ifdef FUNAPI_COCOS2D
  if (use_wss) {
    context_info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

    if (ca_file_path.length() > 0) {
      context_info.ssl_ca_filepath = ca_file_path.c_str();
    }
    else {
      context_info.options |= LWS_SERVER_OPTION_PEER_CERT_NOT_REQUIRED;
    }
  }
#endif

  context_ = lws_create_context(&context_info);
  if (context_ == nullptr) {
    connect_completion_handler_(true, -1, "lws_create_context failed");
    return;
  }

  struct lws_client_connect_info connect_info;
  memset(&connect_info, 0, sizeof(connect_info));
  connect_info.context = context_;
  connect_info.address = hostname_or_ip;
  connect_info.port = server_port;
  connect_info.path = "/";
  connect_info.host = connect_info.address;
  connect_info.origin = connect_info.address;
  connect_info.ietf_version_or_minus_one = -1;
  connect_info.userdata = shared_from_this().get();

  if (use_wss) {
    connect_info.protocol = "wss";
#ifdef FUNAPI_COCOS2D
    connect_info.ssl_connection = LCCSCF_USE_SSL;
    if (ca_file_path.empty()) {
      connect_info.ssl_connection |= LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK | LCCSCF_ALLOW_SELFSIGNED;
    }
#endif
  }
  else {
    connect_info.protocol = "ws";
    connect_info.ssl_connection = 0;
  }

  web_socket_ = lws_client_connect_via_info(&connect_info);
  if (web_socket_ == nullptr) {
    connect_completion_handler_(true, -1, "lws_client_connect_via_info failed");
    return;
  }
#endif
}


bool FunapiWebsocketImpl::Send(const std::vector<uint8_t> &body,
                               const bool is_binary,
                               const SendCompletionHandler &send_completion_handler) {
#if FUNAPI_HAVE_WEBSOCKET
  if (send_buffer_length_ == 0 && offset_ == 0) {
    is_binary_ = is_binary;
    send_completion_handler_ = send_completion_handler;

    send_buffer_length_ = body.size();
    if (send_buffer_length_ > FUNAPI_WEBSOCKET_MAX_SEND_BUFFER - LWS_PRE) {
      send_buffer_length_ = FUNAPI_WEBSOCKET_MAX_SEND_BUFFER - LWS_PRE;
    }

    memset(send_buffer_, 0x00, LWS_PRE);
    memcpy(send_buffer_ + LWS_PRE, body.data(), send_buffer_length_);

    return true;
  }
#endif

  return false;
}


void FunapiWebsocketImpl::OnSend() {
#if FUNAPI_HAVE_WEBSOCKET
  if (send_buffer_length_ == 0 && offset_ == 0) {
    send_handler_();
  }

  if (send_buffer_length_ > 0 && web_socket_) {
    enum lws_write_protocol write_protocol = LWS_WRITE_CONTINUATION;

    if (offset_ == 0) {
      if (is_binary_) {
        write_protocol = LWS_WRITE_BINARY;
      }
      else {
        write_protocol = LWS_WRITE_TEXT;
      }
    }

    int sent_length = 0;
    uint8_t *buf = send_buffer_ + LWS_PRE + offset_;
    sent_length = lws_write(web_socket_, (unsigned char*)buf, send_buffer_length_ - offset_, write_protocol);

    if (sent_length < 0) {
      send_completion_handler_(true, -1, "lws_write failed", sent_length);
    }
    else {
      offset_ += sent_length;
    }

    if (offset_ == send_buffer_length_) {
      offset_ = 0;
      send_buffer_length_ = 0;

      send_completion_handler_(false, 0, "", sent_length);
    }
  }
#endif
}


////////////////////////////////////////////////////////////////////////////////
// FunapiWebsocket implementation.

FunapiWebsocket::FunapiWebsocket()
: impl_(std::make_shared<FunapiWebsocketImpl>()) {
}


FunapiWebsocket::~FunapiWebsocket() {
  // DebugUtils::Log("%s", __FUNCTION__);
}


std::shared_ptr<FunapiWebsocket> FunapiWebsocket::Create() {
  return std::make_shared<FunapiWebsocket>();
}


void FunapiWebsocket::Connect(const char* hostname_or_ip,
                              const int port,
                              const ConnectCompletionHandler &connect_completion_handler,
                              const CloseHandler &close_handler,
                              const SendHandler &send_handler,
                              const RecvHandler &recv_handler) {
  impl_->Connect(hostname_or_ip,
                 port,
                 false,
                 "",
                 connect_completion_handler,
                 close_handler,
                 send_handler,
                 recv_handler);
}


void FunapiWebsocket::Connect(const char* hostname_or_ip,
                              const int port,
                              const bool use_wss,
                              const std::string &ca_file_path,
                              const ConnectCompletionHandler &connect_completion_handler,
                              const CloseHandler &close_handler,
                              const SendHandler &send_handler,
                              const RecvHandler &recv_handler) {
  impl_->Connect(hostname_or_ip,
                 port,
                 use_wss,
                 ca_file_path,
                 connect_completion_handler,
                 close_handler,
                 send_handler,
                 recv_handler);
}


bool FunapiWebsocket::Send(const std::vector<uint8_t> &body,
                           const bool is_binary,
                           const SendCompletionHandler &send_completion_handler) {
  return impl_->Send(body, is_binary, send_completion_handler);
}


void FunapiWebsocket::Update() {
  return impl_->Update();
}

}  // namespace fun
