// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_SOCKET_H_
#define SRC_FUNAPI_SOCKET_H_

namespace fun {

class FunapiSocket {
 public:
  static std::string GetStringFromAddrInfo(struct addrinfo *info);
};


class FunapiTcpImpl;
class FunapiTcp : public std::enable_shared_from_this<FunapiTcp> {
 public:
  typedef std::function<void(const bool is_failed,
                             const bool is_timed_out,
                             const int error_code,
                             const std::string &error_string,
                             struct addrinfo *addrinfo_res)> ConnectCompletionHandler;

  typedef std::function<void(const bool is_failed,
                             const int error_code,
                             const std::string &error_string,
                             const int read_length,
                             std::vector<uint8_t> &receiving)> RecvHandler;

  typedef std::function<void()> SendHandler;

  typedef std::function<void(const bool is_failed,
                             const int error_code,
                             const std::string &error_string,
                             const int sent_length)> SendCompletionHandler;

  FunapiTcp();
  virtual ~FunapiTcp();

  static std::shared_ptr<FunapiTcp> Create();

  void Connect(const char* hostname_or_ip,
               const int sever_port,
               const time_t connect_timeout_seconds,
               const bool disable_nagle,
               ConnectCompletionHandler connect_completion_handler,
               SendHandler send_handler,
               RecvHandler recv_handler);

  void Connect(struct addrinfo *addrinfo_res,
               ConnectCompletionHandler connect_completion_handler);

  bool Send(const std::vector<uint8_t> &body,
            SendCompletionHandler send_completion_handler);

  int GetSocket();
  void OnSelect(const fd_set rset, const fd_set wset, const fd_set eset);

 private:
  std::shared_ptr<FunapiTcpImpl> impl_;
};


class FunapiUdpImpl;
class FunapiUdp : public std::enable_shared_from_this<FunapiUdp> {
 public:
  typedef std::function<void(const bool is_failed,
                             const int error_code,
                             const std::string &error_string)> InitHandler;

  typedef std::function<void(const bool is_failed,
                             const int error_code,
                             const std::string &error_string,
                             const int read_length,
                             std::vector<uint8_t> &receiving)> RecvHandler;

  typedef std::function<void()> SendHandler;

  typedef std::function<void(const bool is_failed,
                             const int error_code,
                             const std::string &error_string,
                             const int sent_length)> SendCompletionHandler;

  FunapiUdp() = delete;
  FunapiUdp(const char* hostname_or_ip,
            const int port,
            InitHandler init_handler,
            SendHandler send_handler,
            RecvHandler recv_handler);
  virtual ~FunapiUdp();

  static std::shared_ptr<FunapiUdp> Create(const char* hostname_or_ip,
                                           const int port,
                                           InitHandler init_handler,
                                           SendHandler send_handler,
                                           RecvHandler recv_handler);

  bool Send(const std::vector<uint8_t> &body, SendCompletionHandler send_completion_handler);

  int GetSocket();
  void OnSelect(const fd_set rset, const fd_set wset, const fd_set eset);

 private:
  std::shared_ptr<FunapiUdpImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_SOCKET_H_
