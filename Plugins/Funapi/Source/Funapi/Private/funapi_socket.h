// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_SOCKET_H_
#define SRC_FUNAPI_SOCKET_H_

#include "funapi_plugin.h"

namespace fun {

class FunapiSocket {
 public:
  static bool Select();
};


class FunapiAddrInfoImpl;
class FunapiAddrInfo : public std::enable_shared_from_this<FunapiAddrInfo> {
 public:
  FunapiAddrInfo();
  virtual ~FunapiAddrInfo();

  static std::shared_ptr<FunapiAddrInfo> Create();

  fun::string GetString();
  std::shared_ptr<FunapiAddrInfoImpl> GetImpl();

 private:
  std::shared_ptr<FunapiAddrInfoImpl> impl_;
};


class FunapiTcpImpl;
class FunapiTcp : public std::enable_shared_from_this<FunapiTcp> {
 public:
  typedef std::function<void(const bool is_failed,
                             const bool is_timed_out,
                             const int error_code,
                             const fun::string &error_string,
                             std::shared_ptr<FunapiAddrInfo> addrinfo_res)> ConnectCompletionHandler;

  typedef std::function<void(const bool is_failed,
                             const int error_code,
                             const fun::string &error_string,
                             const int read_length,
                             fun::vector<uint8_t> &receiving)> RecvHandler;

  typedef std::function<void()> SendHandler;

  typedef std::function<void(const bool is_failed,
                             const int error_code,
                             const fun::string &error_string,
                             const int sent_length)> SendCompletionHandler;

  FunapiTcp();
  virtual ~FunapiTcp();

  static std::shared_ptr<FunapiTcp> Create();

  void Connect(const char* hostname_or_ip,
               const int sever_port,
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

  void Connect(std::shared_ptr<FunapiAddrInfo> addrinfo,
               const ConnectCompletionHandler &connect_completion_handler);

  bool Send(const fun::vector<uint8_t> &body,
            const SendCompletionHandler &send_completion_handler);

  int GetSocket();

#ifdef FUNAPI_PLATFORM_WINDOWS
  void OnSelect(HANDLE handle);
#else
  void OnSelect(const fd_set rset, const fd_set wset, const fd_set eset);
#endif

 private:
  std::shared_ptr<FunapiTcpImpl> impl_;
};


class FunapiUdpImpl;
class FunapiUdp : public std::enable_shared_from_this<FunapiUdp> {
 public:
  typedef std::function<void(const bool is_failed,
                             const int error_code,
                             const fun::string &error_string)> InitHandler;

  typedef std::function<void(const bool is_failed,
                             const int error_code,
                             const fun::string &error_string,
                             const int read_length,
                             fun::vector<uint8_t> &receiving)> RecvHandler;

  typedef std::function<void()> SendHandler;

  typedef std::function<void(const bool is_failed,
                             const int error_code,
                             const fun::string &error_string,
                             const int sent_length)> SendCompletionHandler;

  FunapiUdp() = delete;
  FunapiUdp(const char* hostname_or_ip,
            const int port,
            const InitHandler &init_handler,
            const SendHandler &send_handler,
            const RecvHandler &recv_handler);
  virtual ~FunapiUdp();

  static std::shared_ptr<FunapiUdp> Create(const char* hostname_or_ip,
                                           const int port,
                                           const InitHandler &init_handler,
                                           const SendHandler &send_handler,
                                           const RecvHandler &recv_handler);

  bool Send(const fun::vector<uint8_t> &body, const SendCompletionHandler &send_completion_handler);

  int GetSocket();
#ifdef FUNAPI_PLATFORM_WINDOWS
  void OnSelect(HANDLE handle);
#else
  void OnSelect(const fd_set rset, const fd_set wset, const fd_set eset);
#endif

 private:
  std::shared_ptr<FunapiUdpImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_SOCKET_H_
