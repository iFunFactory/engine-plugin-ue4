// Copyright (C) 2013-2019 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef FUNAPI_UE4_PLATFORM_PS4
#ifndef FUNAPI_PLATFORM_WINDOWS

#include "funapi_socket.h"

#ifdef FUNAPI_UE4
#include "FunapiPrivatePCH.h"
#endif

#include "funapi_send_flag_manager.h"
#include "funapi_utils.h"

#ifdef FUNAPI_UE4
// Work around a conflict between a UI namespace defined by engine code and a typedef in OpenSSL
#define UI UI_ST
THIRD_PARTY_INCLUDES_START
#include "openssl/ssl.h"
#include "openssl/err.h"
#ifdef UI
#undef UI
#endif
#else // FUNAPI_UE4
#include "openssl/ssl.h"
#include "openssl/err.h"
#endif // FUNAPI_UE4

#include <poll.h>

namespace fun {

////////////////////////////////////////////////////////////////////////////////
// FunapiAddrInfoImpl implementation.

class FunapiAddrInfoImpl : public std::enable_shared_from_this<FunapiAddrInfoImpl> {
 public:
   FunapiAddrInfoImpl();
  virtual ~FunapiAddrInfoImpl();

  fun::string GetString();

  struct addrinfo* GetAddrInfo();
  void SetAddrInfo(struct addrinfo* info);

 private:
  struct addrinfo *addrinfo_res_ = nullptr;
};

FunapiAddrInfoImpl::FunapiAddrInfoImpl() {
}


FunapiAddrInfoImpl::~FunapiAddrInfoImpl() {
  // DebugUtils::Log("%s", __FUNCTION__);
}


void FunapiAddrInfoImpl::SetAddrInfo(struct addrinfo* info) {
  addrinfo_res_ = info;
}


struct addrinfo* FunapiAddrInfoImpl::GetAddrInfo() {
  return addrinfo_res_;
}


fun::string FunapiAddrInfoImpl::GetString() {
  auto info = addrinfo_res_;

  if (info) {
    char addrStr[INET6_ADDRSTRLEN];
    if (info->ai_family == AF_INET)
    {
      struct sockaddr_in *sin = (struct sockaddr_in*) info->ai_addr;
      inet_ntop(info->ai_family, (void*)&sin->sin_addr, addrStr, sizeof(addrStr));
    }
    else if (info->ai_family == AF_INET6)
    {
      struct sockaddr_in6 *sin = (struct sockaddr_in6*) info->ai_addr;
      inet_ntop(info->ai_family, (void*)&sin->sin6_addr, addrStr, sizeof(addrStr));
    }

    return fun::string(addrStr);
  }

  return "NULL";
}


////////////////////////////////////////////////////////////////////////////////
// FunapiSocketImpl implementation.

class FunapiSocketImpl : public std::enable_shared_from_this<FunapiSocketImpl> {
 public:
  FunapiSocketImpl();
  virtual ~FunapiSocketImpl();

  static fun::string GetStringFromAddrInfo(struct addrinfo *info);

  static fun::vector<std::weak_ptr<FunapiSocketImpl>> vec_sockets_;
  static std::mutex vec_sockets_mutex_;

  static void Add(std::shared_ptr<FunapiSocketImpl> s);
  static fun::vector<std::shared_ptr<FunapiSocketImpl>> GetSocketImpls();
  static bool Select();

  static const int kBufferSize = 65536;

  int GetSocket();
  virtual bool IsReadySelect();
  virtual void OnPoll(short poll_revents) = 0;

 protected:
  bool InitAddrInfo(int socktype,
                    const char* hostname_or_ip,
                    const int port,
                    int &error_code,
                    fun::string &error_string);
  void FreeAddrInfo();

  bool InitSocket(struct addrinfo *info,
                  int &error_code,
                  fun::string &error_string);
  void CloseSocket();

  void SocketPoll(short poll_revents);

  virtual void OnSend() = 0;
  virtual void OnRecv() = 0;

  int socket_ = -1;
  struct addrinfo *addrinfo_ = nullptr;
  struct addrinfo *addrinfo_res_ = nullptr;
};


fun::string FunapiSocketImpl::GetStringFromAddrInfo(struct addrinfo *info) {
  if (info) {
    char addrStr[INET6_ADDRSTRLEN];
    if (info->ai_family == AF_INET)
    {
      struct sockaddr_in *sin = (struct sockaddr_in*) info->ai_addr;
      inet_ntop(info->ai_family, (void*)&sin->sin_addr, addrStr, sizeof(addrStr));
    }
    else if (info->ai_family == AF_INET6)
    {
      struct sockaddr_in6 *sin = (struct sockaddr_in6*) info->ai_addr;
      inet_ntop(info->ai_family, (void*)&sin->sin6_addr, addrStr, sizeof(addrStr));
    }

    return fun::string(addrStr);
  }

  return "NULL";
}


fun::vector<std::weak_ptr<FunapiSocketImpl>> FunapiSocketImpl::vec_sockets_;
std::mutex FunapiSocketImpl::vec_sockets_mutex_;


void FunapiSocketImpl::Add(std::shared_ptr<FunapiSocketImpl> s) {
  std::unique_lock<std::mutex> lock(vec_sockets_mutex_);
  vec_sockets_.push_back(s);
}


fun::vector<std::shared_ptr<FunapiSocketImpl>> FunapiSocketImpl::GetSocketImpls() {
  fun::vector<std::shared_ptr<FunapiSocketImpl>> v_sockets;
  fun::vector<std::weak_ptr<FunapiSocketImpl>> v_weak_sockets;
  {
    std::unique_lock<std::mutex> lock(vec_sockets_mutex_);
    if (!vec_sockets_.empty()) {
      for (auto i : vec_sockets_) {
        if (auto s = i.lock()) {
          v_sockets.push_back(s);
          v_weak_sockets.push_back(i);
        }
      }

      vec_sockets_.swap(v_weak_sockets);
    }
  }

  return v_sockets;
}


// extern function in funapi_session.cpp
void OnSessionTicked();
bool FunapiSocketImpl::Select()
{
  static const int MAX_POLLFDS = 1024;
  static FunapiTimer session_tick_timer;

  struct pollfd pollfds[MAX_POLLFDS];
  int num_pollfds = 0;

  auto v_sockets = FunapiSocketImpl::GetSocketImpls();

  if (!v_sockets.empty())
  {
    int* pipe_fds = FunapiSendFlagManager::Get().GetPipeFds();
    // pipe_fds 는 int[2] 크기를 가진다.
    for (int i = 0; i < 2; ++i)
    {
      int fd = pipe_fds[i];
      pollfds[num_pollfds].fd = fd;
      pollfds[num_pollfds].events = POLLIN | POLLPRI | POLLOUT;
      pollfds[num_pollfds].revents = 0;
      ++num_pollfds;
    }

    fun::vector<std::shared_ptr<FunapiSocketImpl>> v_select_sockets;
    for (auto& s : v_sockets)
    {
      if (s->IsReadySelect())
      {
        int fd = s->GetSocket();
        if (fd > 0)
        {
          assert(num_pollfds + 1 < MAX_POLLFDS);
          pollfds[num_pollfds].fd = fd;
          pollfds[num_pollfds].events = POLLIN | POLLPRI | POLLOUT;
          pollfds[num_pollfds].revents = 0;
          ++num_pollfds;
          v_select_sockets.push_back(s);
        }
      }
    }

    if (!v_select_sockets.empty())
    {
      int result = poll(pollfds, num_pollfds, 1);

      // ERROR
      if (result == -1)
      {
        int error_cd = FunapiUtil::GetSocketErrorCode();
        auto error_str = FunapiUtil::GetSocketErrorString(error_cd);
        DebugUtils::Log(
          "Socket select failed, error_code : %d error : %s",
          error_cd,
          error_str.c_str());
        return false;
      }

      // PING
      if (session_tick_timer.IsExpired())
      {
        // Update 는 1초 간격을 유지.
        // Ping TimeOut 은 OnSessionTicked 함수 안에서 확인.
        OnSessionTicked();
        session_tick_timer.SetTimer(1);
      }

      // TIME OUT
      if (result == 0)
      {
        return true;
      }

      // SEND
      if ((pollfds[0].revents & POLLIN) ||
          (pollfds[0].revents & POLLPRI) ||
          (pollfds[0].revents & POLLHUP))
      {
        FunapiSendFlagManager::Get().ResetWakeUp();
        for (auto& s : v_select_sockets)
        {
          s->OnSend();
        }
        return true;
      }

      // RECV
      int index = 0;
      for (auto& s : v_select_sockets) {
        while (index < num_pollfds) {
          if (pollfds[index].fd == s->GetSocket()) {
            break;
          }
          ++index;
        }
        if (index < num_pollfds) {
          assert(pollfds[index].fd == s->GetSocket());
          if (pollfds[index].revents) {
            s->OnPoll(pollfds[index].revents);
          }
        }
      }

      return true;
    }
  }

  return false;
}


FunapiSocketImpl::FunapiSocketImpl() {
}


FunapiSocketImpl::~FunapiSocketImpl() {
  CloseSocket();
  FreeAddrInfo();
  // DebugUtils::Log("%s", __FUNCTION__);
}


int FunapiSocketImpl::GetSocket() {
  return socket_;
}


bool FunapiSocketImpl::IsReadySelect() {
  return true;
}


void FunapiSocketImpl::FreeAddrInfo() {
  if (addrinfo_) {
    freeaddrinfo(addrinfo_);
  }
}


bool FunapiSocketImpl::InitAddrInfo(int socktype,
                                    const char* hostname_or_ip,
                                    const int port,
                                    int &error_code,
                                    fun::string &error_string) {
#ifdef FUNAPI_COCOS2D_PLATFORM_WINDOWS
  static auto wsa_init = FunapiInit::Create([](){
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
  }, [](){
    WSACleanup();
  });
#endif

  struct addrinfo hints;

  fun::stringstream ss_port;
  ss_port << static_cast<int>(port);

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = socktype;
  error_code = getaddrinfo(hostname_or_ip, ss_port.str().c_str(), &hints, &addrinfo_);
  if (error_code) {
    error_string = (char*)gai_strerror(error_code);
    return false;
  }

  return true;
}


void FunapiSocketImpl::CloseSocket() {
  // DebugUtils::Log("%s", __FUNCTION__);

  if (socket_ >= 0) {
    // DebugUtils::Log("Socket [%d] closed.", socket_);

#ifdef FUNAPI_PLATFORM_WINDOWS
    closesocket(socket_);
#else
    close(socket_);
#endif
    socket_ = -1;
  }
}


bool FunapiSocketImpl::InitSocket(struct addrinfo *info,
                                  int &error_code,
                                  fun::string &error_string) {
  if (!info) {
    return false;
  }

  int fd = -1;

  for (addrinfo_res_ = info;addrinfo_res_;addrinfo_res_ = addrinfo_res_->ai_next) {
    fd = socket(addrinfo_res_->ai_family,
                addrinfo_res_->ai_socktype,
                addrinfo_res_->ai_protocol);
    if (fd < 0) {
      switch errno {
      case EAFNOSUPPORT:
      case EPROTONOSUPPORT:
        if (addrinfo_res_->ai_next)
          continue;
        else {
          break;
        }

      default:
        /* handle other socket errors */
        ;
      }
    }
    else {
      break;
    }
  }

  if (fd < 0) {
    error_code = FunapiUtil::GetSocketErrorCode();
    error_string = FunapiUtil::GetSocketErrorString(error_code);
    return false;
  }

  socket_ = fd;

  return true;
}


void FunapiSocketImpl::SocketPoll(short poll_revents)
{
  if (socket_ > 0)
  {
    if ((poll_revents & POLLIN) ||
        (poll_revents & POLLHUP) ||
        (poll_revents & POLLERR))
    {
      OnRecv();
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
// FunapiTcpImpl implementation.

class FunapiTcpImpl : public FunapiSocketImpl {
 public:
  typedef FunapiTcp::ConnectCompletionHandler ConnectCompletionHandler;
  typedef FunapiTcp::RecvHandler RecvHandler;
  typedef FunapiTcp::SendHandler SendHandler;
  typedef FunapiTcp::SendCompletionHandler SendCompletionHandler;

  FunapiTcpImpl();
  virtual ~FunapiTcpImpl();

  void OnPoll(short poll_revents);

  void Connect(const char* hostname_or_ip,
               const int port,
               const time_t connect_timeout_seconds,
               const bool disable_nagle,
               const ConnectCompletionHandler &connect_completion_handler,
               const SendHandler &send_handler,
               const RecvHandler &recv_handler);

  void Connect(const char* hostname_or_ip,
               const int sever_port,
               const time_t connect_timeout_seconds,
               const bool disable_nagle,
               const bool use_tls,
               const fun::string &cert_file_path,
               const ConnectCompletionHandler &connect_completion_handler,
               const SendHandler &send_handler,
               const RecvHandler &recv_handler);

  void Connect(struct addrinfo *addrinfo_res,
               const ConnectCompletionHandler &connect_completion_handler);

  void Connect(struct addrinfo *addrinfo_res);

  bool Send(const fun::vector<uint8_t> &body, const SendCompletionHandler &send_handler);

  bool IsReadySelect();

 protected:
  bool InitTcpSocketOption(bool disable_nagle,
                           int &error_code,
                           fun::string &error_string);

  void SocketPoll(short poll_revents);

  void OnConnectCompletion(const bool is_failed,
                           const bool is_timed_out,
                           const int error_code,
                           const fun::string &error_string);

  void OnConnectCompletion(const bool is_failed,
                           const bool is_timed_out);

  bool ConnectTLS();
  void CleanupSSL();

  void OnSend();
  void OnRecv();

  enum class SocketSelectState : int {
    kNone = 0,
    kSelect,
  };
  SocketSelectState socket_select_state_ = SocketSelectState::kNone;

  ConnectCompletionHandler completion_handler_ = nullptr;

  SendHandler send_handler_;
  RecvHandler recv_handler_;
  SendCompletionHandler send_completion_handler_;

  fun::vector<uint8_t> body_;
  int offset_ = 0;
  time_t connect_timeout_seconds_ = 5;

  // https://curl.haxx.se/docs/caextract.html
  // https://curl.haxx.se/ca/cacert.pem
  fun::string cert_file_path_;
  bool use_tls_ = false;

  SSL_CTX *ctx_ = nullptr;
  SSL *ssl_ = nullptr;
};


FunapiTcpImpl::FunapiTcpImpl() {
}


FunapiTcpImpl::~FunapiTcpImpl() {
  // DebugUtils::Log("%s", __FUNCTION__);
  CleanupSSL();
}


void FunapiTcpImpl::CleanupSSL() {
  // DebugUtils::Log("%s", __FUNCTION__);

  if (ssl_) {
    SSL_shutdown(ssl_);
    SSL_free(ssl_);
    ssl_ = nullptr;
  }

  if (ctx_) {
    SSL_CTX_free(ctx_);
    ctx_ = nullptr;
  }
}


void FunapiTcpImpl::OnPoll(short poll_revents) {
  if (socket_select_state_ == SocketSelectState::kSelect) {
    FunapiSocketImpl::SocketPoll(poll_revents);
  }
}


bool FunapiTcpImpl::IsReadySelect() {
  if (socket_select_state_ == SocketSelectState::kSelect) {
    return true;
  }

  return false;
}


void FunapiTcpImpl::Connect(struct addrinfo *addrinfo_res) {
  addrinfo_res_ = addrinfo_res;

  socket_select_state_ = SocketSelectState::kNone;

  int rc = connect(socket_,addrinfo_res_->ai_addr, addrinfo_res_->ai_addrlen);
  if (rc != 0 && errno != EINPROGRESS) {
    OnConnectCompletion(true, false);
    return;
  }
#ifndef FUNAPI_PLATFORM_WINDOWS
  if (rc == 0) {
    OnConnectCompletion(true, true);
    return;
  }
#endif

  struct pollfd pollfds[1];
  pollfds[0].fd = socket_;
  pollfds[0].events = POLLIN | POLLPRI | POLLOUT;
  pollfds[0].revents = 0;

  struct timeval timeout = { static_cast<long>(connect_timeout_seconds_), 0 };
  rc = poll(pollfds, sizeof(pollfds) / sizeof(pollfds[0]), connect_timeout_seconds_ * 1000);
  if (rc < 0) {
    // select failed
    OnConnectCompletion(true, false);
  }
  else if (rc == 0) {
    // connect timed out
    OnConnectCompletion(true, true);
  }
  else {
#ifdef FUNAPI_PLATFORM_WINDOWS
    if (!FD_ISSET(socket_, &rset) && !FD_ISSET(socket_, &wset)) {
      OnConnectCompletion(true, false, 0, "");
      return;
    }

    if (FD_ISSET(socket_, &eset)) {
      OnConnectCompletion(true, false, 0, "");
      return;
    }

    OnConnectCompletion(false, false, 0, "");
#else
    int e = 0;
    socklen_t e_size = sizeof(e);

    short revents = pollfds[0].revents;

    if (!(revents & POLLIN) && !(revents & POLLOUT)) {
      OnConnectCompletion(true, false, 0, "");
      return;
    }

    if (getsockopt(socket_, SOL_SOCKET, SO_ERROR, &e, &e_size) < 0) {
      OnConnectCompletion(true, false, e, strerror(e));
      return;
    }

    if (e == 0) {
      OnConnectCompletion(false, false, 0, "");
      return;
    }

    if (e == 60) {
      OnConnectCompletion(true, true, e, strerror(e));
      return;
    }

    OnConnectCompletion(true, false, e, strerror(e));
#endif // FUNAPI_PLATFORM_WINDOWS
  }
}


void FunapiTcpImpl::Connect(struct addrinfo *addrinfo_res,
                            const ConnectCompletionHandler &connect_completion_handler) {
  completion_handler_ = connect_completion_handler;

  Connect(addrinfo_res);
}


void FunapiTcpImpl::Connect(const char* hostname_or_ip,
                            const int port,
                            const time_t connect_timeout_seconds,
                            const bool disable_nagle,
                            const ConnectCompletionHandler &connect_completion_handler,
                            const SendHandler &send_handler,
                            const RecvHandler &recv_handler) {
  Connect(hostname_or_ip,
          port,
          connect_timeout_seconds,
          disable_nagle,
          false, "",
          connect_completion_handler,
          send_handler,
          recv_handler);
}


void FunapiTcpImpl::Connect(const char* hostname_or_ip,
                            const int port,
                            const time_t connect_timeout_seconds,
                            const bool disable_nagle,
                            const bool use_tls,
                            const fun::string &cert_file_path,
                            const ConnectCompletionHandler &connect_completion_handler,
                            const SendHandler &send_handler,
                            const RecvHandler &recv_handler) {
  completion_handler_ = connect_completion_handler;

  if (socket_ != -1) {
    OnConnectCompletion(true, false, 0, "");
    return;
  }

  use_tls_ = use_tls;
  cert_file_path_ = cert_file_path;

  send_handler_ = send_handler;
  recv_handler_ = recv_handler;
  connect_timeout_seconds_ = connect_timeout_seconds;

  int error_code = 0;
  fun::string error_string;

  if (!InitAddrInfo(SOCK_STREAM, hostname_or_ip, port, error_code, error_string)) {
    OnConnectCompletion(true, false, error_code, error_string);
    return;
  }

  addrinfo_res_ = addrinfo_;

  if (!InitSocket(addrinfo_res_, error_code, error_string)) {
    OnConnectCompletion(true, false, error_code, error_string);
    return;
  }

  if (!InitTcpSocketOption(disable_nagle, error_code, error_string)) {
    OnConnectCompletion(true, false, error_code, error_string);
    return;
  }

  // log
  {
    fun::string hostname = FunapiSocketImpl::GetStringFromAddrInfo(addrinfo_res_);
    DebugUtils::Log("Address Info: %s -> %s", hostname_or_ip, hostname.c_str());
  }
  // //

  Connect(addrinfo_res_);
}


bool FunapiTcpImpl::InitTcpSocketOption(bool disable_nagle, int &error_code, fun::string &error_string) {
  // non-blocking.
#ifdef FUNAPI_PLATFORM_WINDOWS
  u_long argp = 0;
  int flag = ioctlsocket(socket_, FIONBIO, &argp);
  assert(flag >= 0);
#else
  int flag = fcntl(socket_, F_GETFL);
  assert(flag >= 0);
  int rc = fcntl(socket_, F_SETFL, O_NONBLOCK | flag);
  assert(rc >= 0);
#endif

  // Disable nagle
  if (disable_nagle) {
    int nagle_flag = 1;
    int result = setsockopt(socket_,
                            IPPROTO_TCP,
                            TCP_NODELAY,
                            reinterpret_cast<char*>(&nagle_flag),
                            sizeof(int));
    if (result < 0) {
      // Error - TCP_NODELAY
      error_code = FunapiUtil::GetSocketErrorCode();
      error_string = FunapiUtil::GetSocketErrorString(error_code);
      return false;
    }
  }

  return true;
}


bool FunapiTcpImpl::ConnectTLS() {
  auto on_ssl_error_completion = [this]() {
    SSL_load_error_strings();

    unsigned long error_code = ERR_get_error();
    char error_buffer[1024];
    ERR_error_string(error_code, (char *)error_buffer);

    OnConnectCompletion(true, false, static_cast<int>(error_code), error_buffer);
  };

  static auto ssl_init = FunapiInit::Create([](){
    SSL_library_init();
  });

  CleanupSSL();

  SSL_METHOD *method = (SSL_METHOD *)SSLv23_client_method();

  if (!method) {
    on_ssl_error_completion();
    return false;
  }

  ctx_ = SSL_CTX_new(method);

  if (!ctx_) {
    on_ssl_error_completion();
    return false;
  }

  bool use_verify = false;

  if (!cert_file_path_.empty()) {
    if (!SSL_CTX_load_verify_locations(ctx_, cert_file_path_.c_str(), NULL)) {
      on_ssl_error_completion();
      return false;
    }
    else {
      SSL_CTX_set_verify(ctx_, SSL_VERIFY_PEER, NULL);
      SSL_CTX_set_verify_depth(ctx_, 1);

      use_verify = true;
    }
  }

  ssl_ = SSL_new(ctx_);

  if (!ssl_) {
    on_ssl_error_completion();
    return false;
  }

  int ret = SSL_set_fd(ssl_, socket_);
  if (ret !=  1) {
    on_ssl_error_completion();
    return false;
  }

  while (true)
  {
    ret = SSL_connect(ssl_);
    if (ret != 1) {
      int n = SSL_get_error(ssl_, ret);

      if (n == SSL_ERROR_WANT_READ || n == SSL_ERROR_WANT_READ) {
        continue;
      }
      else {
        on_ssl_error_completion();
        return false;
      }
    }
    else {
      break;
    }
  }

  if (use_verify) {
    if (SSL_get_peer_certificate(ssl_) != NULL)
    {
      if (SSL_get_verify_result(ssl_) != X509_V_OK) {
        on_ssl_error_completion();
        return false;
      }
    }
    else {
      on_ssl_error_completion();
      return false;
    }
  }

  return true;
}


void FunapiTcpImpl::OnConnectCompletion(const bool is_failed,
                                        const bool is_timed_out) {
  int error_code = FunapiUtil::GetSocketErrorCode();
  fun::string error_string = FunapiUtil::GetSocketErrorString(error_code);

  OnConnectCompletion(is_failed, is_timed_out, error_code, error_string);
}


void FunapiTcpImpl::OnConnectCompletion(const bool is_failed,
                                        const bool is_timed_out,
                                        const int error_code,
                                        const fun::string &error_string) {
  if (false == is_failed && use_tls_) {
    if (false == ConnectTLS()) {
      return;
    }
  }

  if (completion_handler_) {
    if (!is_failed) {
      socket_select_state_ = SocketSelectState::kSelect;
    }
    else {
      socket_select_state_ = SocketSelectState::kNone;
    }

    auto addrinfo = FunapiAddrInfo::Create();
    addrinfo->GetImpl()->SetAddrInfo(addrinfo_res_);

    completion_handler_(is_failed, is_timed_out, error_code, error_string, addrinfo);
  }
}


void FunapiTcpImpl::OnSend() {
  if (body_.empty() && offset_ == 0) {
    send_handler_();
  }

  if (!body_.empty()) {
    int nSent = 0;

    if (use_tls_) {
      nSent = static_cast<int>(SSL_write(ssl_,
        reinterpret_cast<char*>(body_.data()) + offset_,
        static_cast<int>(body_.size() - offset_)));
    }
    else {
      nSent = static_cast<int>(send(socket_,
        reinterpret_cast<char*>(body_.data()) + offset_,
        body_.size() - offset_,
        0));
    }

    /*
    if (nSent == 0) {
      DebugUtils::Log("Socket [%d] closed.", socket_);
    }
    */

    if (nSent <= 0) {
      int error_code = FunapiUtil::GetSocketErrorCode();
      fun::string error_string = FunapiUtil::GetSocketErrorString(error_code);
      send_completion_handler_(true, error_code, error_string, nSent);
      CloseSocket();
    }
    else {
      offset_ += nSent;
    }

    if (offset_ == body_.size()) {
      offset_ = 0;
      body_.resize(0);

      send_completion_handler_(false, 0, "", nSent);
    }
  }
}


void FunapiTcpImpl::OnRecv() {
  fun::vector<uint8_t> buffer(kBufferSize);

  int nRead = 0;

  if (use_tls_) {
    nRead = static_cast<int>(SSL_read(ssl_, reinterpret_cast<char*>(buffer.data()), kBufferSize));
  }
  else {
    nRead = static_cast<int>(recv(socket_, reinterpret_cast<char*>(buffer.data()), kBufferSize, 0));
  }

  /*
  if (nRead == 0) {
    DebugUtils::Log("Socket [%d] closed.", socket_);
  }
  */

  if (nRead <= 0) {
    int error_code = FunapiUtil::GetSocketErrorCode();
    fun::string error_string = FunapiUtil::GetSocketErrorString(error_code);
    recv_handler_(true, error_code, error_string, nRead, buffer);
    CloseSocket();
  }
  else {
    recv_handler_(false, 0, "", nRead, buffer);
  }
}


bool FunapiTcpImpl::Send(const fun::vector<uint8_t> &body, const SendCompletionHandler &send_completion_handler) {
  send_completion_handler_ = send_completion_handler;

  body_.insert(body_.end(), body.cbegin(), body.cend());

//  // log
//  {
//    fun::string temp_string(body.cbegin(), body.cend());
//    printf("\"%s\"\n", temp_string.c_str());
//  }
//  //

  return true;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiUdpImpl implementation.

class FunapiUdpImpl : public FunapiSocketImpl {
 public:
  typedef FunapiUdp::InitHandler InitHandler;
  typedef FunapiUdp::RecvHandler RecvHandler;
  typedef FunapiUdp::SendHandler SendHandler;
  typedef FunapiUdp::SendCompletionHandler SendCompletionHandler;

  FunapiUdpImpl() = delete;
  FunapiUdpImpl(const char* hostname_or_ip,
                const int port,
                const InitHandler &init_handler,
                const SendHandler &send_handler,
                const RecvHandler &recv_handler);
  virtual ~FunapiUdpImpl();

  void OnPoll(short poll_revents);
  bool Send(const fun::vector<uint8_t> &body, const SendCompletionHandler &send_handler);

 private:
  void Finalize();
  void OnSend();
  void OnRecv();

  SendHandler send_handler_;
  RecvHandler recv_handler_;
};


FunapiUdpImpl::FunapiUdpImpl(const char* hostname_or_ip,
                             const int port,
                             const InitHandler &init_handler,
                             const SendHandler &send_handler,
                             const RecvHandler &recv_handler)
: send_handler_(send_handler), recv_handler_(recv_handler) {
  int error_code = 0;
  fun::string error_string;

  if (!InitAddrInfo(SOCK_DGRAM, hostname_or_ip, port, error_code, error_string)) {
    init_handler(true, error_code, error_string);
    return;
  }

  addrinfo_res_ = addrinfo_;

  if (!InitSocket(addrinfo_res_, error_code, error_string)) {
    init_handler(true, error_code, error_string);
    return;
  }

  init_handler(false, 0, "");
}


FunapiUdpImpl::~FunapiUdpImpl() {
}


void FunapiUdpImpl::OnPoll(short poll_revents) {
  FunapiSocketImpl::SocketPoll(poll_revents);
}


void FunapiUdpImpl::OnSend() {
  send_handler_();
}


void FunapiUdpImpl::OnRecv() {
  fun::vector<uint8_t> receiving_vector(kBufferSize);

#ifdef FUNAPI_PLATFORM_WINDOWS
  int nRead = static_cast<int>(recvfrom(socket_,
                                        reinterpret_cast<char*>(receiving_vector.data()),
                                        receiving_vector.size(), 0, addrinfo_res_->ai_addr,
                                        reinterpret_cast<int*>(&addrinfo_res_->ai_addrlen)));
#else
  int nRead = static_cast<int>(recvfrom(socket_,
                                        reinterpret_cast<char*>(receiving_vector.data()),
                                        receiving_vector.size(), 0, addrinfo_res_->ai_addr,
                                        (&addrinfo_res_->ai_addrlen)));
#endif // FUNAPI_PLATFORM_WINDOWS

  /*
  if (nRead == 0) {
    DebugUtils::Log("Socket [%d] closed.", socket_);
  }
  */

  if (nRead <= 0) {
    int error_code = FunapiUtil::GetSocketErrorCode();
    fun::string error_string = FunapiUtil::GetSocketErrorString(error_code);
    recv_handler_(true, error_code, error_string, nRead, receiving_vector);
    CloseSocket();
  }
  else {
    recv_handler_(false, 0, "", nRead, receiving_vector);
  }
}


bool FunapiUdpImpl::Send(const fun::vector<uint8_t> &body, const SendCompletionHandler &send_completion_handler) {
  uint8_t *buf = const_cast<uint8_t*>(body.data());

  int nSent = static_cast<int>(sendto(socket_, reinterpret_cast<char*>(buf), body.size(), 0, addrinfo_res_->ai_addr, addrinfo_res_->ai_addrlen));

  /*
  if (nSent == 0) {
    DebugUtils::Log("Socket [%d] closed.", socket_);
  }
  */

  if (nSent <= 0) {
    int error_code = FunapiUtil::GetSocketErrorCode();
    fun::string error_string = FunapiUtil::GetSocketErrorString(error_code);
    send_completion_handler(true, error_code, error_string, nSent);
    CloseSocket();
  }
  else {
    send_completion_handler(false, 0, "", nSent);
  }

  return true;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiSocket implementation.

bool FunapiSocket::Select() {
  return FunapiSocketImpl::Select();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiAddrInfo implementation.

FunapiAddrInfo::FunapiAddrInfo()
  : impl_(std::make_shared<FunapiAddrInfoImpl>()) {
}


FunapiAddrInfo::~FunapiAddrInfo() {
  // DebugUtils::Log("%s", __FUNCTION__);
}


std::shared_ptr<FunapiAddrInfo> FunapiAddrInfo::Create() {
  return std::make_shared<FunapiAddrInfo>();
}


fun::string FunapiAddrInfo::GetString() {
  return impl_->GetString();
}


std::shared_ptr<FunapiAddrInfoImpl> FunapiAddrInfo::GetImpl() {
  return impl_;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiTcp implementation.

FunapiTcp::FunapiTcp()
: impl_(std::make_shared<FunapiTcpImpl>()) {
  FunapiSocketImpl::Add(impl_);
}


FunapiTcp::~FunapiTcp() {
  // DebugUtils::Log("%s", __FUNCTION__);
}


std::shared_ptr<FunapiTcp> FunapiTcp::Create() {
  return std::make_shared<FunapiTcp>();
}


void FunapiTcp::Connect(const char* hostname_or_ip,
                        const int port,
                        const time_t connect_timeout_seconds,
                        const bool disable_nagle,
                        const ConnectCompletionHandler &connect_completion_handler,
                        const SendHandler &send_handler,
                        const RecvHandler &recv_handler) {
  impl_->Connect(hostname_or_ip,
                port,
                connect_timeout_seconds,
                disable_nagle,
                connect_completion_handler,
                send_handler,
                recv_handler);
}


void FunapiTcp::Connect(const char* hostname_or_ip,
                        const int port,
                        const time_t connect_timeout_seconds,
                        const bool disable_nagle,
                        const bool use_tls,
                        const fun::string &cert_file_path,
                        const ConnectCompletionHandler &connect_completion_handler,
                        const SendHandler &send_handler,
                        const RecvHandler &recv_handler) {
  impl_->Connect(hostname_or_ip,
                 port,
                 connect_timeout_seconds,
                 disable_nagle,
                 use_tls,
                 cert_file_path,
                 connect_completion_handler,
                 send_handler,
                 recv_handler);
}


void FunapiTcp::Connect(std::shared_ptr<FunapiAddrInfo> info,
                        const ConnectCompletionHandler &connect_completion_handler) {
  // TODO(sungjin): PS4 기능 테스트 시 아래 코드가 영향을 주는 지 확인
  // auto addr_info_next = info->GetImpl()->GetAddrInfo()->ai_next;
  auto addr_info = info->GetImpl()->GetAddrInfo();
  impl_->Connect(addr_info, connect_completion_handler);
}


bool FunapiTcp::Send(const fun::vector<uint8_t> &body, const SendCompletionHandler &send_handler) {
  return impl_->Send(body, send_handler);
}


int FunapiTcp::GetSocket() {
  return impl_->GetSocket();
}


void FunapiTcp::OnPoll(short poll_revents) {
  impl_->OnPoll(poll_revents);
}


////////////////////////////////////////////////////////////////////////////////
// FunapiUdp implementation.

FunapiUdp::FunapiUdp(const char* hostname_or_ip,
                     const int port,
                     const InitHandler &init_handler,
                     const SendHandler &send_handler,
                     const RecvHandler &recv_handler)
: impl_(std::make_shared<FunapiUdpImpl>(hostname_or_ip,
                                        port,
                                        init_handler,
                                        send_handler,
                                        recv_handler)) {
  FunapiSocketImpl::Add(impl_);
}


FunapiUdp::~FunapiUdp() {
}


std::shared_ptr<FunapiUdp> FunapiUdp::Create(const char* hostname_or_ip,
                                             const int port,
                                             const InitHandler &init_handler,
                                             const SendHandler &send_handler,
                                             const RecvHandler &recv_handler) {
  return std::make_shared<FunapiUdp>(hostname_or_ip,
                                     port,
                                     init_handler,
                                     send_handler,
                                     recv_handler);
}


bool FunapiUdp::Send(const fun::vector<uint8_t> &body, const SendCompletionHandler &send_handler) {
  return impl_->Send(body, send_handler);
}


int FunapiUdp::GetSocket() {
  return impl_->GetSocket();
}


void FunapiUdp::OnPoll(short poll_revents) {
  impl_->OnPoll(poll_revents);
}

}  // namespace fun

#endif // FUNAPI_PLATFORM_WINDOWS
#endif // FUNAPI_UE4_PLATFORM_PS4
