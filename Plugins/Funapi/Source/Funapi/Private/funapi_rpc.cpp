// Copyright (C) 2013-2018 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifdef FUNAPI_UE4
#include "FunapiPrivatePCH.h"
#endif

#include "funapi_rpc.h"

#if FUNAPI_HAVE_RPC

#include "funapi_utils.h"
#include "funapi_tasks.h"
#include "funapi_socket.h"

#define kRpcAddMessageType "_sys_ds_add_server"
#define kRpcDelMessageType "_sys_ds_del_server"
#define kRpcInfoMessageType "_sys_ds_info"
#define kRpcMasterMessageType "_sys_ds_master"


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

  typedef std::function<void(std::shared_ptr<FunapiRpcPeer> peer,
                             const std::string &type,
                             const FunDedicatedServerRpcMessage &request_message,
                             const ResponseHandler &response_handler)> RpcSystemHandler;

  FunapiRpcImpl();
  virtual ~FunapiRpcImpl();

  void Start(std::shared_ptr<FunapiRpcOption> option);
  void SetPeerEventHandler(const EventHandler &handler);
  void SetHandler(const std::string &type, const RpcHandler &handler);
  void SetSystemHandler(const std::string &type, const RpcSystemHandler &handler);
  void Update();

  void OnHandler(std::shared_ptr<FunapiRpcPeer> peer, std::shared_ptr<FunapiRpcMessage> msg);
  std::shared_ptr<FunapiTasks> GetTasksQueue();
  std::shared_ptr<FunapiRpcOption> GetRpcOption();

  void DebugCall(const FunDedicatedServerRpcMessage &debug_request);
  void DebugMasterCall(const FunDedicatedServerRpcMessage &debug_request);
  void DebugClose();

 protected:
  void OnEvent(const EventType type, const std::string &hostname_or_ip, const int port, const std::string &peer_id);
  std::string EventTypeToString(const EventType type);
  void Connect(const std::string &peer_id, const std::string &hostname_or_ip, const int port);
  void Connect(const int index);
  void InitSystemHandlers();
  void SetPeerEventHanderReconnect(const std::string &peer_id, std::shared_ptr<FunapiRpcPeer> peer);
  void CallMasterMessage();

 private:
  std::shared_ptr<FunapiTasks> tasks_;

  std::unordered_set<std::shared_ptr<FunapiRpcPeer>> peer_set_;
  std::mutex peer_set_mutex_;

  std::unordered_map<std::string, std::shared_ptr<FunapiRpcPeer>> peer_map_;
  std::mutex peer_map_mutex_;

  std::unordered_map<std::string, std::shared_ptr<RpcHandler>> handler_map_;
  std::mutex handler_map_mutex_;

  std::unordered_map<std::string, std::shared_ptr<RpcSystemHandler>> sys_handler_map_;
  std::mutex sys_handler_map_mutex_;

  std::shared_ptr<EventHandler> event_handler_ = nullptr;
  std::mutex event_handler_mutex_;

  std::shared_ptr<FunapiRpcOption> rpc_option_ = nullptr;

  std::weak_ptr<FunapiRpcPeer> master_peer_;
  std::mutex master_peer_mutex_;
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
  void SetRpcImpl(std::weak_ptr<FunapiRpcImpl> weak);
  void SetPeerId(const std::string &id);

  void Call(std::shared_ptr<FunapiRpcMessage> message);

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
  void PushConnectThread();

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

  std::string peer_id_;
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

  while (true) {
    if (proto_length_ == 0) {
      if (recv_buffer_.size() >= 4) {
        uint32_t length;
        memcpy(&length, recv_buffer_.data(), 4);
        proto_length_ = ntohl(length);
      }
      else {
        return;
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

        auto recv_funapi_rpc_message = std::make_shared<FunapiRpcMessage>(protobuf_message);
        std::weak_ptr<FunapiRpcPeer> weak = shared_from_this();
        PushTaskQueue([this, weak, recv_funapi_rpc_message]()->bool {
          if (auto peer = weak.lock()) {
            if (auto impl = rpc_impl_.lock()) {
              impl->OnHandler(peer, recv_funapi_rpc_message);
            }
          }
          return true;
        });

        uint32_t new_length = static_cast<uint32_t>(recv_buffer_.size()) - proto_length_ - 4;
        recv_buffer_.erase(recv_buffer_.begin(), recv_buffer_.begin() + proto_length_ + 4);
        recv_buffer_.resize(new_length);

        proto_length_ = 0;
      }
      else {
        return;
      }
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
      std::stringstream ss;
      ss << "Tcp connection timed out: " << hostname_or_ip.c_str();
      if (error_code != 0) {
        ss << "(" << error_code << ") " << error_string.c_str();
      }

      DebugUtils::Log("%s", ss.str().c_str());

      OnEvent(EventType::kConnectionTimedOut);
    }
    else
    {
      std::stringstream ss;
      ss << "Failed to tcp connection: " << hostname_or_ip.c_str();
      if (error_code != 0) {
        ss << "(" << error_code << ") " << error_string.c_str();
      }

      DebugUtils::Log("%s", ss.str().c_str());

      OnEvent(EventType::kConnectionFailed);
    }
  }
  else {
    SetState(State::kConnected);
    OnEvent(EventType::kConnected);
  }
}


void FunapiRpcPeer::PushConnectThread() {
  SetState(State::kConnecting);

  // Tries to connect.
  DebugUtils::Log("Try to tcp connect to server: %s %d", hostname_or_ip_.c_str(), port_);

  tcp_ = FunapiTcp::Create();
  std::weak_ptr<FunapiRpcPeer> weak = shared_from_this();

  if (auto ct = FunapiThread::Get("_connect")) {
    ct->Push([this, weak](){
      if (auto peer = weak.lock()) {
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
      }

      return true;
    });
  }
}


void FunapiRpcPeer::OnConnect(std::string hostname_or_ip, int port) {
  if (GetState() == State::kDisconnecting) {
    Connect(hostname_or_ip, port);
  }
  else if (GetState() == State::kDisconnected) {
    hostname_or_ip_ = hostname_or_ip;
    port_ = port;

    PushConnectThread();
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
  PushTaskQueue([this, type]()->bool{
    std::shared_ptr<EventHandler> handler = nullptr;
    {
      std::unique_lock<std::mutex> lock(event_handler_mutex_);
      handler = event_handler_;
    }

    if (handler) {
      (*handler)(type, hostname_or_ip_, port_, peer_id_);
    }

    return true;
  });
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


void FunapiRpcPeer::Call(std::shared_ptr<FunapiRpcMessage> message) {
  PushTaskQueue([this, message]()->bool {
    PushSendQueue(message);
    return true;
  });
}


void FunapiRpcPeer::SetPeerId(const std::string &id) {
  peer_id_ = id;
  OnEvent(EventType::kPeerId);
}


////////////////////////////////////////////////////////////////////////////////
// FunapiRpcImpl implementation.


FunapiRpcImpl::FunapiRpcImpl() {
  tasks_ = FunapiTasks::Create();
}


FunapiRpcImpl::~FunapiRpcImpl() {
  // DebugUtils::Log("%s", __FUNCTION__);
}


void FunapiRpcImpl::CallMasterMessage() {
  std::shared_ptr<FunapiRpcPeer> master_peer = nullptr;
  {
    std::unique_lock<std::mutex> lock(master_peer_mutex_);
    master_peer = master_peer_.lock();
  }

  if (master_peer) {
    auto master_message = std::make_shared<FunapiRpcMessage>();

    std::stringstream ss_xid;
    std::random_device rd;
    std::default_random_engine re(rd());
    std::uniform_int_distribution<int> dist(1,0xffff);
    ss_xid << dist(re) << "-" << dist(re);

    master_message->GetMessage().set_xid(ss_xid.str());
    master_message->GetMessage().set_is_request(true);
    master_message->GetMessage().set_type(kRpcMasterMessageType);

    master_peer->Call(master_message);
  }
}

void FunapiRpcImpl::InitSystemHandlers() {
  SetSystemHandler(kRpcInfoMessageType,
                   [this]
                   (std::shared_ptr<FunapiRpcPeer> peer,
                    const std::string &type,
                    const FunDedicatedServerRpcMessage &request_message,
                    const fun::FunapiRpc::ResponseHandler &response_handler)
  {
    {
      FunDedicatedServerRpcMessage response_message;
      FunDedicatedServerRpcSystemMessage *sys_response_message = response_message.MutableExtension(ds_rpc_sys);

      std::string tag = rpc_option_->GetTag();
      if (tag.length() > 0) {
        std::stringstream ss;
        ss << "{ \"tag\" : \"" << rpc_option_->GetTag() << "\" }";
        sys_response_message->set_data(ss.str());
      }

      response_handler(response_message);
    }

    FunDedicatedServerRpcSystemMessage sys_message = request_message.GetExtension(ds_rpc_sys);

    rapidjson::Document json_document;
    json_document.Parse<0>(sys_message.data().c_str());

    std::string peer_id;
    const char* kPeerId = "id";
    if (json_document.HasMember(kPeerId)) {
      peer_id = json_document[kPeerId].GetString();
    }

    if (peer_id.length() > 0) {
      {
        std::unique_lock<std::mutex> lock(peer_map_mutex_);
        peer_map_[peer_id] = peer;
      }
      {
        std::unique_lock<std::mutex> lock(peer_set_mutex_);
        peer_set_.erase(peer);
      }

      SetPeerEventHanderReconnect(peer_id, peer);
      peer->SetPeerId(peer_id);

      std::shared_ptr<FunapiRpcPeer> master_peer = nullptr;
      {
        std::unique_lock<std::mutex> lock(master_peer_mutex_);
        master_peer = master_peer_.lock();
      }
      if (master_peer == nullptr) {
        {
          std::unique_lock<std::mutex> lock(master_peer_mutex_);
          master_peer_ = peer;
        }

        CallMasterMessage();
      }
    }
  });

  SetSystemHandler(kRpcAddMessageType,
                   [this]
                   (std::shared_ptr<FunapiRpcPeer> peer,
                    const std::string &type,
                    const FunDedicatedServerRpcMessage &request_message,
                    const fun::FunapiRpc::ResponseHandler &response_handler)
  {
    FunDedicatedServerRpcMessage response_message;
    response_handler(response_message);

    FunDedicatedServerRpcSystemMessage sys_message = request_message.GetExtension(ds_rpc_sys);

    rapidjson::Document json_document;
    json_document.Parse<0>(sys_message.data().c_str());

    const char* kPeerId = "id";
    const char* kPeerIp = "ip";
    const char* kPeerPort = "port";
    if (json_document.HasMember(kPeerId) &&
        json_document.HasMember(kPeerIp) &&
        json_document.HasMember(kPeerPort))
    {
      std::string peer_id = json_document[kPeerId].GetString();
      std::string peer_ip = json_document[kPeerIp].GetString();
      int peer_port = json_document[kPeerPort].GetInt();

      Connect(peer_id, peer_ip, peer_port);
    }
  });

  SetSystemHandler(kRpcDelMessageType,
                   [this]
                   (std::shared_ptr<FunapiRpcPeer> peer,
                    const std::string &type,
                    const FunDedicatedServerRpcMessage &request_message,
                    const fun::FunapiRpc::ResponseHandler &response_handler)
  {
    FunDedicatedServerRpcMessage response_message;
    response_handler(response_message);

    FunDedicatedServerRpcSystemMessage sys_message = request_message.GetExtension(ds_rpc_sys);

    rapidjson::Document json_document;
    json_document.Parse<0>(sys_message.data().c_str());

    const char* kPeerId = "id";
    if (json_document.HasMember(kPeerId))
    {
      std::string peer_id = json_document[kPeerId].GetString();

      if (peer_id.length() > 0) {
        std::shared_ptr<FunapiRpcPeer> del_peer = nullptr;
        {
          std::unique_lock<std::mutex> lock(peer_map_mutex_);
          if (peer_map_.find(peer_id) != peer_map_.end()) {
            del_peer = peer_map_[peer_id];
            peer_map_.erase(peer_id);
          }
        }

        std::shared_ptr<FunapiRpcPeer> master_peer = nullptr;
        {
          std::unique_lock<std::mutex> lock(master_peer_mutex_);
          master_peer = master_peer_.lock();
        }

        if (master_peer == del_peer) {
          std::shared_ptr<FunapiRpcPeer> new_master_peer = nullptr;
          {
            std::unique_lock<std::mutex> lock(peer_map_mutex_);
            if (!peer_map_.empty()) {
              new_master_peer = peer_map_.cbegin()->second;
            }
          }

          if (new_master_peer)
          {
            {
              std::unique_lock<std::mutex> lock(master_peer_mutex_);
              master_peer_ = new_master_peer;
            }

            CallMasterMessage();
          }
          else {
            Connect(0);
          }
        }
      }
    }
  });
}

void FunapiRpcImpl::Start(std::shared_ptr<FunapiRpcOption> option) {
  InitSystemHandlers();

  rpc_option_ = option;

  Connect(0);
}


void FunapiRpcImpl::Connect(const int index) {
  if (index >= static_cast<int>(rpc_option_->GetInitializers().size())) {
    return;
  }

  std::shared_ptr<FunapiRpcPeer> peer = std::make_shared<FunapiRpcPeer>();
  {
    std::unique_lock<std::mutex> lock(peer_set_mutex_);
    peer_set_.insert(peer);
  }

  std::weak_ptr<FunapiRpcPeer> peer_weak = peer;
  std::weak_ptr<FunapiRpcImpl> weak = shared_from_this();
  auto new_handler = [this, index, weak, peer_weak](const EventType type,
                                             const std::string &hostname_or_ip,
                                             const int port,
                                             const std::string &peer_id)
  {
    if (auto impl = weak.lock()) {
      OnEvent(type, hostname_or_ip, port, peer_id);

      if (type == EventType::kDisconnected || type == EventType::kConnectionFailed || type == EventType::kConnectionTimedOut) {
        if (auto this_peer = peer_weak.lock()) {
          std::unique_lock<std::mutex> lock(peer_set_mutex_);
          peer_set_.erase(this_peer);
        }

        if (type == EventType::kConnectionFailed) {
          if (auto ct = FunapiThread::Get("_connect")) {
            auto option = rpc_option_;
            ct->Push([option]()->bool{
              std::this_thread::sleep_for(std::chrono::seconds(option->GetConnectTimeout()));
              return true;
            });
          }
        }

        int new_index = 0;
        auto &v = rpc_option_->GetInitializers();
        if ((index+1) < static_cast<int>(v.size())) {
          new_index = index + 1;
        }

        tasks_->Push([this, new_index]()->bool{
          Connect(new_index);
          return true;
        });
      }
    }
  };

  peer->SetRpcImpl(weak);
  peer->SetEventCallback(new_handler);

  auto &v = rpc_option_->GetInitializers();
  peer->Connect(std::get<0>(v[index]), std::get<1>(v[index]));
}


void FunapiRpcImpl::SetPeerEventHanderReconnect(const std::string &peer_id, std::shared_ptr<FunapiRpcPeer> peer) {
  std::weak_ptr<FunapiRpcPeer> peer_weak = peer;
  std::weak_ptr<FunapiRpcImpl> weak = shared_from_this();
  auto new_handler = [this, peer_id, weak, peer_weak](const EventType type,
                                                      const std::string &peer_hostname_or_ip,
                                                      const int peer_port,
                                                      const std::string &peer_id_empty)
  {
    if (auto impl = weak.lock()) {
      OnEvent(type, peer_hostname_or_ip, peer_port, peer_id);

      if (type == EventType::kDisconnected) {
        if (auto this_peer = peer_weak.lock()) {
          bool del_server = false;
          {
            std::unique_lock<std::mutex> lock(master_peer_mutex_);
            if (auto master_peer = master_peer_.lock()) {
              if (master_peer == this_peer) {
                del_server = true;
              }
            }
          }

          if (del_server) {
            std::shared_ptr<FunapiRpcPeer> new_master_peer = nullptr;
            {
              std::unique_lock<std::mutex> lock(peer_map_mutex_);
              peer_map_.erase(peer_id);
              if (!peer_map_.empty()) {
                new_master_peer = peer_map_.cbegin()->second;
              }
            }

            if (new_master_peer)
            {
              {
                std::unique_lock<std::mutex> lock(master_peer_mutex_);
                master_peer_ = new_master_peer;
              }

              CallMasterMessage();
            }
            else {
              Connect(0);
            }

            return;
          }
        }
      }

      if (type == EventType::kDisconnected || type == EventType::kConnectionFailed || type == EventType::kConnectionTimedOut) {
        bool reconnect = false;
        {
          std::unique_lock<std::mutex> lock(peer_map_mutex_);
          if (peer_map_.find(peer_id) != peer_map_.end()) {
            reconnect = true;
          }
        }

        if (reconnect) {
          if (type == EventType::kConnectionFailed) {
            if (auto ct = FunapiThread::Get("_connect")) {
              auto option = rpc_option_;
              ct->Push([option]()->bool {
                std::this_thread::sleep_for(std::chrono::seconds(option->GetConnectTimeout()));
                return true;
              });
            }
          }

          Connect(peer_id, peer_hostname_or_ip, peer_port);
        }
      }
    }
  };

  peer->SetEventCallback(new_handler);
}


void FunapiRpcImpl::Connect(const std::string &peer_id, const std::string &hostname_or_ip, const int port) {
  std::shared_ptr<FunapiRpcPeer> peer = nullptr;
  {
    std::unique_lock<std::mutex> lock(peer_map_mutex_);
    if (peer_map_.find(peer_id) != peer_map_.end()) {
      peer = peer_map_[peer_id];
      auto state = peer->GetState();
      if (state == FunapiRpcPeer::State::kConnecting || state == FunapiRpcPeer::State::kConnected) {
        return;
      }
    }
  }

  peer = std::make_shared<FunapiRpcPeer>();
  {
    std::unique_lock<std::mutex> lock(peer_map_mutex_);
    peer_map_[peer_id] = peer;
  }

  SetPeerEventHanderReconnect(peer_id, peer);

  peer->SetRpcImpl(shared_from_this());
  peer->Connect(hostname_or_ip, port);
}


void FunapiRpcImpl::SetPeerEventHandler(const EventHandler &handler) {
  std::unique_lock<std::mutex> lock(event_handler_mutex_);
  event_handler_ = std::make_shared<EventHandler>(handler);
}


void FunapiRpcImpl::SetHandler(const std::string &type, const RpcHandler &handler) {
  std::unique_lock<std::mutex> lock(handler_map_mutex_);
  handler_map_[type] = std::make_shared<RpcHandler>(handler);
}


void FunapiRpcImpl::SetSystemHandler(const std::string &type, const RpcSystemHandler &handler) {
  std::unique_lock<std::mutex> lock(sys_handler_map_mutex_);
  sys_handler_map_[type] = std::make_shared<RpcSystemHandler>(handler);
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


std::string FunapiRpcImpl::EventTypeToString(const EventType type) {
  if (type == EventType::kConnected) {
    return "Connected";
  }
  else if (type == EventType::kDisconnected) {
    return "Disconnected";
  }
  else if (type == EventType::kConnectionFailed) {
    return "ConnectionFailed";
  }
  else if (type == EventType::kConnectionTimedOut) {
    return "ConnectionTimedOut";
  }
  else if (type == EventType::kPeerId) {
    return "PeerId";
  }

  return "";
}


void FunapiRpcImpl::OnEvent(const EventType type,
                            const std::string &hostname_or_ip,
                            const int port,
                            const std::string &peer_id) {
  std::stringstream ss;
  ss << "RPC peer event: ";
  if (peer_id.length() > 0) {
    ss << "'" << peer_id << "' ";
  }
  ss << hostname_or_ip << ":" << port << " " << EventTypeToString(type);
  DebugUtils::Log("%s", ss.str().c_str());

  std::shared_ptr<EventHandler> handler = nullptr;
  {
    std::unique_lock<std::mutex> lock(event_handler_mutex_);
    handler = event_handler_;
  }

  if (handler) {
    (*handler)(type, hostname_or_ip, port, peer_id);
  }
}


void FunapiRpcImpl::OnHandler(std::shared_ptr<FunapiRpcPeer> peer,
                              std::shared_ptr<FunapiRpcMessage> request_message) {
  if (request_message->GetMessage().is_request()) {
    std::string type = request_message->GetMessage().type();

    std::shared_ptr<RpcHandler> handler = nullptr;
    {
      std::unique_lock<std::mutex> lock(handler_map_mutex_);
      if (handler_map_.find(type) != handler_map_.end()) {
        handler = handler_map_[type];
      }
    }

    std::shared_ptr<RpcSystemHandler> system_handler = nullptr;
    {
      std::unique_lock<std::mutex> lock(sys_handler_map_mutex_);
      if (sys_handler_map_.find(type) != sys_handler_map_.end()) {
        system_handler = sys_handler_map_[type];
      }
    }

    auto response_handler = [peer, request_message](const FunDedicatedServerRpcMessage &msg) {
      auto response_msg = std::make_shared<FunapiRpcMessage>(msg);
      response_msg->GetMessage().set_type(request_message->GetMessage().type());
      response_msg->GetMessage().set_xid(request_message->GetMessage().xid());
      response_msg->GetMessage().set_is_request(false);
      peer->Call(response_msg);
    };

    if (handler) {
      (*handler)(type, request_message->GetMessage(), response_handler);
    }
    else if (system_handler) {
      (*system_handler)(peer, type, request_message->GetMessage(), response_handler);
    }
    else {
      DebugUtils::Log("[RPC] handler not found '%s'", type.c_str());
    }
  }
}


std::shared_ptr<FunapiTasks> FunapiRpcImpl::GetTasksQueue() {
  return tasks_;
}


std::shared_ptr<FunapiRpcOption> FunapiRpcImpl::GetRpcOption() {
  return rpc_option_;
}


void FunapiRpcImpl::DebugCall(const FunDedicatedServerRpcMessage &debug_request) {
  std::vector<std::shared_ptr<FunapiRpcPeer>> v_peer;
  {
    std::unique_lock<std::mutex> lock(peer_set_mutex_);
    for (auto p : peer_set_) {
      v_peer.push_back(p);
    }
  }

  {
    std::unique_lock<std::mutex> lock(peer_map_mutex_);
    for (auto i : peer_map_) {
      v_peer.push_back(i.second);
    }
  }

  auto request_message = std::make_shared<FunapiRpcMessage>(debug_request);
  for (auto p : v_peer) {
    p->Call(request_message);
  }
}


void FunapiRpcImpl::DebugMasterCall(const FunDedicatedServerRpcMessage &debug_request) {
  std::shared_ptr<FunapiRpcPeer> p = nullptr;
  {
    std::unique_lock<std::mutex> lock(master_peer_mutex_);
    p = master_peer_.lock();
  }

  auto request_message = std::make_shared<FunapiRpcMessage>(debug_request);
  if (p) {
    p->Call(request_message);
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


void FunapiRpc::Start(std::shared_ptr<FunapiRpcOption> option) {
  impl_->Start(option);
}


void FunapiRpc::SetPeerEventHandler(const EventHandler &handler) {
  impl_->SetPeerEventHandler(handler);
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


void FunapiRpc::DebugMasterCall(const FunDedicatedServerRpcMessage &debug_request) {
  impl_->DebugMasterCall(debug_request);
}
*/

////////////////////////////////////////////////////////////////////////////////
// FunapiRpcOptionImpl implementation.

class FunapiRpcOptionImpl : public std::enable_shared_from_this<FunapiRpcOptionImpl> {
 public:
  FunapiRpcOptionImpl() = default;
  virtual ~FunapiRpcOptionImpl() = default;

  void AddInitializer(const std::string &hostname_or_ip, const int port);
  std::vector<std::tuple<std::string, int>>& GetInitializers();

  void SetDisableNagle(const bool disable_nagle);
  bool GetDisableNagle();

  void SetConnectTimeout(const int seconds);
  int GetConnectTimeout();

  void SetTag(const std::string &tag);
  std::string GetTag();

 private:
  std::vector<std::tuple<std::string, int>> initializers_;
  bool disable_nagle_ = true;
  int timeout_seconds_ = 5;
  std::string tag_;
};


void FunapiRpcOptionImpl::AddInitializer(const std::string &hostname_or_ip, const int port) {
  initializers_.push_back(std::make_tuple(hostname_or_ip, port));
}


std::vector<std::tuple<std::string, int>>& FunapiRpcOptionImpl::GetInitializers() {
  return initializers_;
}


void FunapiRpcOptionImpl::SetDisableNagle(const bool disable_nagle) {
  disable_nagle_ = disable_nagle;
}


bool FunapiRpcOptionImpl::GetDisableNagle() {
  return disable_nagle_;
}


void FunapiRpcOptionImpl::SetConnectTimeout(const int seconds) {
  timeout_seconds_ = seconds;
}


int FunapiRpcOptionImpl::GetConnectTimeout() {
  return timeout_seconds_;
}


void FunapiRpcOptionImpl::SetTag(const std::string &tag) {
  tag_ = tag;
}


std::string FunapiRpcOptionImpl::GetTag() {
  return tag_;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiRpcOption implementation.

FunapiRpcOption::FunapiRpcOption() : impl_(std::make_shared<FunapiRpcOptionImpl>()) {
}


std::shared_ptr<FunapiRpcOption> FunapiRpcOption::Create() {
  return std::make_shared<FunapiRpcOption>();
}


void FunapiRpcOption::AddInitializer(const std::string &hostname_or_ip, const int port) {
  impl_->AddInitializer(hostname_or_ip, port);
}


std::vector<std::tuple<std::string, int>>& FunapiRpcOption::GetInitializers() {
  return impl_->GetInitializers();
}


void FunapiRpcOption::SetDisableNagle(const bool disable_nagle) {
  impl_->SetDisableNagle(disable_nagle);
}


bool FunapiRpcOption::GetDisableNagle() {
  return impl_->GetDisableNagle();
}


void FunapiRpcOption::SetConnectTimeout(const int seconds) {
  impl_->SetConnectTimeout(seconds);
}


int FunapiRpcOption::GetConnectTimeout() {
  return impl_->GetConnectTimeout();
}


void FunapiRpcOption::SetTag(const std::string &tag) {
  impl_->SetTag(tag);
}


std::string FunapiRpcOption::GetTag() {
  return impl_->GetTag();
}

}  // namespace fun

#endif
