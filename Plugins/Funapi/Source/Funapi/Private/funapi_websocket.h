// Copyright (C) 2013-2018 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_WEBSOCKET_H_
#define SRC_FUNAPI_WEBSOCKET_H_

#include "funapi_plugin.h"

namespace fun {

class FunapiWebsocketImpl;
class FunapiWebsocket : public std::enable_shared_from_this<FunapiWebsocket> {
 public:
  typedef std::function<void(const bool is_failed,
                             const int error_code,
                             const std::string &error_string)> ConnectCompletionHandler;

  typedef std::function<void(const int error_code,
                             const std::string &error_string)> CloseHandler;

  typedef std::function<void(const int read_length,
                             std::vector<uint8_t> &receiving)> RecvHandler;

  typedef std::function<void()> SendHandler;

  typedef std::function<void(const bool is_failed,
                             const int error_code,
                             const std::string &error_string,
                             const int sent_length)> SendCompletionHandler;

  FunapiWebsocket();
  virtual ~FunapiWebsocket();

  static std::shared_ptr<FunapiWebsocket> Create();

  void Connect(const char* hostname_or_ip,
               const int sever_port,
               const ConnectCompletionHandler &connect_completion_handler,
               const CloseHandler &close_handler,
               const SendHandler &send_handler,
               const RecvHandler &recv_handler);

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

 private:
  std::shared_ptr<FunapiWebsocketImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_SOCKET_H_
