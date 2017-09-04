// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin.h"
#include "funapi_utils.h"
#include "funapi_tasks.h"
#include "funapi_rpc.h"
#include "funapi_socket.h"
#include "funapi/rpc/fun_dedicated_server_rpc_message.pb.h"

namespace fun {

////////////////////////////////////////////////////////////////////////////////
// FunapiRpcMessage

class FunapiRpcMessage : public std::enable_shared_from_this<FunapiRpcMessage> {
 public:
  FunapiRpcMessage() = default;
  FunapiRpcMessage(const FunDedicatedServerRpcMessage &msg);
  virtual ~FunapiRpcMessage();

  void SetMessage(const FunDedicatedServerRpcMessage &msg);
  FunDedicatedServerRpcMessage& GetMessage();

 private:
  FunDedicatedServerRpcMessage protobuf_;
};


FunapiRpcMessage::FunapiRpcMessage(const FunDedicatedServerRpcMessage &msg) : protobuf_(msg) {
}


FunapiRpcMessage::~FunapiRpcMessage() {
  // DebugUtils::Log("%s", __FUNCTION__);
}


void FunapiRpcMessage::SetMessage(const FunDedicatedServerRpcMessage &msg) {
  protobuf_ = msg;
}


FunDedicatedServerRpcMessage& FunapiRpcMessage::GetMessage() {
  return protobuf_;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiRpcImpl

class FunapiRpcPeer;
class FunapiRpcImpl : public std::enable_shared_from_this<FunapiRpcImpl> {
 public:
  typedef FunapiRpc::EventType EventType;
  typedef FunapiRpc::EventHandler EventHandler;
  typedef FunapiRpc::ResponseHandler ResponseHandler;
  typedef FunapiRpc::RpcHandler RpcHandler;

  FunapiRpcImpl();
  virtual ~FunapiRpcImpl();

  void Connect(const std::string &hostname_or_ip, int port, const EventHandler &handler);
  void SetHandler(const std::string &type, const RpcHandler &handler);
  void Update();

  void OnHandler(std::shared_ptr<FunapiRpcPeer> peer, const FunDedicatedServerRpcMessage &msg);
  std::shared_ptr<FunapiTasks> GetTasksQueue();

  void DebugCall(const FunDedicatedServerRpcMessage &debug_request);
  void DebugClose();

 private:
  std::shared_ptr<FunapiTasks> tasks_;

  std::unordered_set<std::shared_ptr<FunapiRpcPeer>> peer_set_;
  std::mutex peer_set_mutex_;

  std::unordered_map<std::string, std::shared_ptr<RpcHandler>> handler_map_;
  std::mutex handler_map_mutex_;
};


////////////////////////////////////////////////////////////////////////////////
// FunapiRpcPeer implementation.

class FunapiRpcPeer : public std::enable_shared_from_this<FunapiRpcPeer> {
 public:
  enum class State : int {
    kDisconnected,
    kDisconnecting,
    kConnecting,
    kConnected
  };

  typedef FunapiRpc::EventType EventType;
  typedef FunapiRpc::EventHandler EventHandler;
  typedef FunapiRpc::ResponseHandler ResponseHandler;
  typedef FunapiRpc::RpcHandler RpcHandler;

  FunapiRpcPeer();
  virtual ~FunapiRpcPeer();

  void Connect(const std::string &hostname_or_ip, int port);
  void Close();

  State GetState();
  void Update();

  void SetEventCallback(const EventHandler &handler);
  void Call(const FunDedicatedServerRpcMessage &debug_request);

  void SetRpcImpl(std::weak_ptr<FunapiRpcImpl> weak);

 protected:
  void SetState(const State s);
  void PushNetworkThread(const FunapiThread::TaskHandler &handler);
  void PushTaskQueue(const FunapiTasks::TaskHandler &task);
  void OnConnect(std::string hostname_or_ip, int port);
  void OnClose();
  void OnEvent(const EventType type);
  void OnSend();
  void OnRecv(const int read_length, const std::vector<uint8_t> &receiving);
  void OnConnectCompletion(const bool isFailed,
                           const bool isTimedOut,
                           const int error_code,
                           const std::string &error_string,
                           std::shared_ptr<FunapiAddrInfo> addrinfo_res);
  void OnDisconnecting();
  void OnDisconnect();
  bool EmptySendQueue();
  void PushSendQueue(const FunDedicatedServerRpcMessage &message);
  void PushSendQueue(std::shared_ptr<FunapiRpcMessage> message);
  void ClearSendQueue();

 private:
  State state_ = State::kDisconnected;
  std::mutex state_mutex_;

  std::shared_ptr<EventHandler> event_handler_ = nullptr;
  std::mutex event_handler_mutex_;

  std::shared_ptr<FunapiThread> network_thread_ = nullptr;
  std::shared_ptr<FunapiTasks> tasks_ =  nullptr;
  std::shared_ptr<FunapiTcp> tcp_ = nullptr;

  std::vector<uint8_t> recv_buffer_;
  uint32_t proto_length_ = 0;

  std::string hostname_or_ip_;
  int port_ = 0;

  std::deque<std::shared_ptr<FunapiRpcMessage>> send_queue_;
  std::mutex send_queue_mutex_;

  std::weak_ptr<FunapiRpcImpl> rpc_impl_;
};


FunapiRpcPeer::FunapiRpcPeer() {
  network_thread_ = FunapiThread::Get("_network");
}


FunapiRpcPeer::~FunapiRpcPeer() {
  // DebugUtils::Log("%s", __FUNCTION__);
}


void FunapiRpcPeer::OnSend() {
  std::vector<uint8_t> buffer;
  int offset = 0;

  std::deque<std::shared_ptr<FunapiRpcMessage>> temp_queue;
  {
    std::unique_lock<std::mutex> lock(send_queue_mutex_);
    temp_queue.swap(send_queue_);
  }

  for (auto message : temp_queue) {
    buffer.resize(offset + 4 + message->GetMessage().ByteSize());

    uint8_t *data = buffer.data() + offset;

    uint32_t length = htonl(message->GetMessage().ByteSize());
    memcpy(data, &length, 4);
    message->GetMessage().SerializeToArray(data+4, message->GetMessage().ByteSize());

    offset += 4 + message->GetMessage().ByteSize();

#ifdef DEBUG_LOG
    DebugUtils::Log("[RPC:C->S] %s", message->GetMessage().ShortDebugString().c_str());
#endif
  }

  if (!buffer.empty()) {
    std::weak_ptr<FunapiRpcPeer> weak = shared_from_this();
    tcp_->Send
    (buffer,
     [weak, this]
     (const bool is_failed,
      const int error_code,
      const std::string &error_string,
      const int sent_length)
    {
      if (auto t = weak.lock()) {
        if (is_failed) {
          OnDisconnect();
        }

        if (GetState() == State::kDisconnecting) {
          OnDisconnecting();
        }
      }
    });
  }
}


void FunapiRpcPeer::OnRecv(const int read_length, const std::vector<uint8_t> &receiving) {
  recv_buffer_.insert(recv_buffer_.end(), receiving.cbegin(), receiving.cbegin() + read_length);

  if (proto_length_ == 0) {
    if (recv_buffer_.size() >= 4) {
      uint32_t length;
      memcpy(&length, recv_buffer_.data(), 4);
      proto_length_ = ntohl(length);
    }
  }

  if (proto_length_ > 0)
  {
    if (recv_buffer_.size() >= (4+proto_length_)) {
      FunDedicatedServerRpcMessage protobuf_message;
      protobuf_message.ParseFromArray(recv_buffer_.data()+4, proto_length_);

#ifdef DEBUG_LOG
      DebugUtils::Log("[RPC:S->C] %s", protobuf_message.ShortDebugString().c_str());
#endif

      std::weak_ptr<FunapiRpcPeer> weak = shared_from_this();
      PushTaskQueue([this, weak, protobuf_message]()->bool {
        if (auto peer = weak.lock()) {
          if (auto impl = rpc_impl_.lock()) {
            impl->OnHandler(peer, protobuf_message);
          }
        }
        return true;
      });

      uint32_t new_length = static_cast<uint32_t>(recv_buffer_.size()) - proto_length_ - 4;
      recv_buffer_.erase(recv_buffer_.begin(), recv_buffer_.begin() + proto_length_ + 4);
      recv_buffer_.resize(new_length);

      proto_length_ = 0;
    }
  }
}


void FunapiRpcPeer::OnConnectCompletion(const bool isFailed,
                                        const bool isTimedOut,
                                        const int error_code,
                                        const std::string &error_string,
                                        std::shared_ptr<FunapiAddrInfo> addrinfo_res) {
  std::string hostname_or_ip = addrinfo_res->GetString();

  if (isFailed) {
    SetState(State::kDisconnected);

    if (isTimedOut) {
      DebugUtils::Log("Tcp connection timed out: %s (%d) %s", hostname_or_ip.c_str(), error_code, error_string.c_str());
      OnEvent(EventType::kConnectionTimedOut);
    }
    else
    {
      DebugUtils::Log("Failed to tcp connection: %s (%d) %s", hostname_or_ip.c_str(), error_code, error_string.c_str());
      OnEvent(EventType::kConnectionFailed);
    }
  }
  else {
    SetState(State::kConnected);
    OnEvent(EventType::kConnected);
  }
}


void FunapiRpcPeer::OnConnect(std::string hostname_or_ip, int port) {
  if (GetState() == State::kDisconnecting) {
    Connect(hostname_or_ip, port);
  }
  else if (GetState() == State::kDisconnected) {
    hostname_or_ip_ = hostname_or_ip;
    port_ = port;

    PushNetworkThread ([this]()->bool
    {
      SetState(State::kConnecting);

      // Tries to connect.
      DebugUtils::Log("Try to tcp connect to server: %s %d", hostname_or_ip_.c_str(), port_);

      tcp_ = FunapiTcp::Create();
      std::weak_ptr<FunapiRpcPeer> weak = shared_from_this();
      tcp_->Connect
      (hostname_or_ip_.c_str(),
       port_,
       10,
       false,
       [weak, this]
       (const bool is_failed,
        const bool is_timed_out,
        const int error_code,
        const std::string &error_string,
        std::shared_ptr<FunapiAddrInfo> addrinfo_res)
       {
         if (auto t = weak.lock()) {
           OnConnectCompletion(is_failed, is_timed_out, error_code, error_string, addrinfo_res);
         }
       }, [weak, this]() {
         if (auto t = weak.lock()) {
           OnSend();
         }
       }, [weak, this]
       (const bool is_failed,
        const int error_code,
        const std::string &error_string,
        const int read_length, std::vector<uint8_t> &receiving)
       {
         if (auto t = weak.lock()) {
           if (is_failed) {
             OnDisconnect();
           }
           else {
             OnRecv(read_length, receiving);
           }
         }
       });

      return true;
    });
  }
}


void FunapiRpcPeer::Connect(const std::string &hostname_or_ip, const int port) {
  PushTaskQueue([this, hostname_or_ip, port]()->bool {
    OnConnect(hostname_or_ip, port);
    return false;
  });
}


void FunapiRpcPeer::OnDisconnect() {
  ClearSendQueue();
  OnDisconnecting();
}


void FunapiRpcPeer::OnDisconnecting() {
  SetState(State::kDisconnecting);
  if (EmptySendQueue()) {
    PushNetworkThread([this]()->bool {
      tcp_ = nullptr;
      SetState(State::kDisconnected);
      OnEvent(EventType::kDisconnected);
      return true;
    });
  }
}


void FunapiRpcPeer::OnClose() {
  auto state = GetState();
  if (state == State::kConnected) {
    OnDisconnecting();
  }
  else if (state == State::kConnecting) {
    Close();
  }
}


void FunapiRpcPeer::Close() {
  PushTaskQueue([this]()->bool {
    OnClose();
    return false;
  });
}


FunapiRpcPeer::State FunapiRpcPeer::GetState() {
  std::unique_lock<std::mutex> lock(state_mutex_);
  return state_;
}


void FunapiRpcPeer::SetState(const State s) {
  std::unique_lock<std::mutex> lock(state_mutex_);
  state_ = s;
}


void FunapiRpcPeer::SetEventCallback(const EventHandler &handler) {
  std::unique_lock<std::mutex> lock(event_handler_mutex_);
  event_handler_ = std::make_shared<EventHandler>(handler);
}


void FunapiRpcPeer::SetRpcImpl(std::weak_ptr<FunapiRpcImpl> weak) {
  rpc_impl_ = weak;
  if (auto impl = rpc_impl_.lock()) {
    tasks_ = impl->GetTasksQueue();
  }
}


void FunapiRpcPeer::PushNetworkThread(const FunapiThread::TaskHandler &handler) {
  if (network_thread_) {
    std::weak_ptr<FunapiRpcPeer> weak = shared_from_this();
    auto handler_ptr = std::make_shared<FunapiThread::TaskHandler>(handler);
    network_thread_->Push([weak, this, handler_ptr]()->bool{
      if (auto s = weak.lock()) {
        return (*handler_ptr)();
      }

      return true;
    });
  }
}


void FunapiRpcPeer::PushTaskQueue(const FunapiTasks::TaskHandler &task) {
  if (tasks_)
  {
    std::weak_ptr<FunapiRpcPeer> weak = shared_from_this();
    auto handler_ptr = std::make_shared<FunapiTasks::TaskHandler>(task);
    tasks_->Push([weak, handler_ptr]()->bool {
      if (auto s = weak.lock()) {
        return (*handler_ptr)();
      }

      return true;
    });
  }
}


void FunapiRpcPeer::OnEvent(const EventType type) {
  std::shared_ptr<EventHandler> handler = nullptr;
  {
    std::unique_lock<std::mutex> lock(event_handler_mutex_);
    handler = event_handler_;
  }

  if (handler) {
    (*handler)(type, hostname_or_ip_, port_);
  }
}


bool FunapiRpcPeer::EmptySendQueue() {
  std::unique_lock<std::mutex> lock(send_queue_mutex_);
  return send_queue_.empty();
}


void FunapiRpcPeer::PushSendQueue(const FunDedicatedServerRpcMessage &message) {
  std::unique_lock<std::mutex> lock(send_queue_mutex_);
  send_queue_.push_back(std::make_shared<FunapiRpcMessage>(message));
}


void FunapiRpcPeer::PushSendQueue(std::shared_ptr<FunapiRpcMessage> message) {
  std::unique_lock<std::mutex> lock(send_queue_mutex_);
  send_queue_.push_back(message);
}


void FunapiRpcPeer::ClearSendQueue() {
  std::unique_lock<std::mutex> lock(send_queue_mutex_);
  send_queue_.clear();
}


void FunapiRpcPeer::Call(const FunDedicatedServerRpcMessage &debug_request) {
  auto message = std::make_shared<FunapiRpcMessage>(debug_request);
  PushTaskQueue([this, message]()->bool {
    PushSendQueue(message);
    return true;
  });
}


////////////////////////////////////////////////////////////////////////////////
// FunapiRpcImpl implementation.


FunapiRpcImpl::FunapiRpcImpl() {
  tasks_ = FunapiTasks::Create();
}


FunapiRpcImpl::~FunapiRpcImpl() {
  // DebugUtils::Log("%s", __FUNCTION__);
}


void FunapiRpcImpl::Connect(const std::string &hostname_or_ip, const int port, const EventHandler &handler) {
  std::shared_ptr<FunapiRpcPeer> peer = std::make_shared<FunapiRpcPeer>();
  {
    std::unique_lock<std::mutex> lock(peer_set_mutex_);
    peer_set_.insert(peer);
  }

  auto old_handler_ptr = std::make_shared<EventHandler>(handler);
  std::weak_ptr<FunapiRpcPeer> peer_weak = peer;
  std::weak_ptr<FunapiRpcImpl> weak = shared_from_this();
  auto new_handler = [this, weak, peer_weak, old_handler_ptr](const EventType type, const std::string &hostname_or_ip, const int port) {
    (*old_handler_ptr)(type, hostname_or_ip, port);
    if (auto impl = weak.lock()) {
      if (type != EventType::kConnected) {
        if (auto peer = peer_weak.lock()) {
          std::unique_lock<std::mutex> lock(peer_set_mutex_);
          peer_set_.erase(peer);
        }
      }
    }
  };

  peer->SetRpcImpl(weak);
  peer->SetEventCallback(new_handler);
  peer->Connect(hostname_or_ip, port);
}


void FunapiRpcImpl::SetHandler(const std::string &type, const RpcHandler &handler) {
  {
    std::unique_lock<std::mutex> lock(handler_map_mutex_);
    handler_map_[type] = std::make_shared<RpcHandler>(handler);
  }
}


void FunapiRpcImpl::Update() {
  if (tasks_) {
    tasks_->Update();
  }

  if (auto nt = FunapiThread::Get("_network")) {
    if (nt->Size() == 0) {
      nt->Push([]()->bool {
        FunapiSocket::Select();
        return true;
      });
    }
  }
}


void FunapiRpcImpl::OnHandler(std::shared_ptr<FunapiRpcPeer> peer,
                              const FunDedicatedServerRpcMessage &msg) {
  std::string type = msg.type();
  std::shared_ptr<RpcHandler> handler = nullptr;
  {
    std::unique_lock<std::mutex> lock(handler_map_mutex_);
    if (handler_map_.find(type) != handler_map_.end()) {
      handler = handler_map_[type];
    }
  }

  if (handler) {
    std::string xid = msg.xid();
    auto response_handler = [peer, type, xid](const std::vector<uint8_t> &v_response) {
      FunDedicatedServerRpcMessage msg;
      msg.set_type(type);
      msg.set_xid(xid);
      msg.set_data(v_response.data(), v_response.size());
      peer->Call(msg);
    };

    (*handler)(type, std::vector<uint8_t>(msg.data().cbegin(), msg.data().cend()), response_handler);
  }
  else {
    DebugUtils::Log("[RPC] handler not found '%s'", type.c_str());
  }
}


std::shared_ptr<FunapiTasks> FunapiRpcImpl::GetTasksQueue() {
  return tasks_;
}


void FunapiRpcImpl::DebugCall(const FunDedicatedServerRpcMessage &debug_request) {
  std::vector<std::shared_ptr<FunapiRpcPeer>> v_peer;
  {
    std::unique_lock<std::mutex> lock(peer_set_mutex_);
    for (auto p : peer_set_) {
      v_peer.push_back(p);
    }
  }

  for (auto p : v_peer) {
    p->Call(debug_request);
  }
}


void FunapiRpcImpl::DebugClose() {
  {
    std::unique_lock<std::mutex> lock(peer_set_mutex_);
    peer_set_.clear();
  }
}


////////////////////////////////////////////////////////////////////////////////
// FunapiRpc implementation.

FunapiRpc::FunapiRpc() : impl_(std::make_shared<FunapiRpcImpl>()) {
}


std::shared_ptr<FunapiRpc> FunapiRpc::Create() {
  return std::make_shared<FunapiRpc>();
}


void FunapiRpc::Connect(const std::string &hostname_or_ip, const int port, const EventHandler &handler) {
  impl_->Connect(hostname_or_ip, port, handler);
}


void FunapiRpc::SetHandler(const std::string &type, const RpcHandler &handler) {
  impl_->SetHandler(type, handler);
}


void FunapiRpc::Update() {
  impl_->Update();
}

/*
void FunapiRpc::DebugCall(const FunDedicatedServerRpcMessage &debug_request) {
  impl_->DebugCall(debug_request);
}
*/

}  // namespace fun
