// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin.h"
#include "funapi_utils.h"
#include "funapi_tasks.h"

namespace fun {

////////////////////////////////////////////////////////////////////////////////
// FunapiTasksImpl implementation.

class FunapiTasksImpl : public std::enable_shared_from_this<FunapiTasksImpl> {
 public:
  typedef FunapiTasks::TaskHandler TaskHandler;

  FunapiTasksImpl();
  virtual ~FunapiTasksImpl();

  static void UpdateAll();
  static void Insert(std::shared_ptr<FunapiTasksImpl> t);
  static void Erase(std::shared_ptr<FunapiTasksImpl> t);

  void Update();
  virtual void Push(const TaskHandler &task);
  virtual void Set(const TaskHandler &task);

 protected:
  std::function<bool()> function_ = nullptr;
  std::queue<std::function<bool()>> queue_;
  std::mutex mutex_;

 private:
  static std::unordered_set<std::shared_ptr<FunapiTasksImpl>> set_tasks_;
  static std::mutex set_mutex_;
};

std::unordered_set<std::shared_ptr<FunapiTasksImpl>> FunapiTasksImpl::set_tasks_;
std::mutex FunapiTasksImpl::set_mutex_;


FunapiTasksImpl::FunapiTasksImpl() {
}


FunapiTasksImpl::~FunapiTasksImpl() {
}


void FunapiTasksImpl::Update() {
  std::function<bool()> task = nullptr;

  {
    std::unique_lock<std::mutex> lock(mutex_);
    task = function_;
  }
  if (task) {
    if (task() == false) return;
  }

  while (true) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (queue_.empty()) {
        break;
      }
      else {
        task = std::move(queue_.front());
        queue_.pop();
      }
    }

    if (task) {
      if (task() == false) {
        break;
      }
    }
    else {
      break;
    }
  }
}


void FunapiTasksImpl::Push(const TaskHandler &task)
{
  std::unique_lock<std::mutex> lock(mutex_);
  queue_.push(task);
}


void FunapiTasksImpl::Set(const TaskHandler &task)
{
  std::unique_lock<std::mutex> lock(mutex_);
  function_ = task;
}


void FunapiTasksImpl::UpdateAll() {
  std::vector<std::shared_ptr<FunapiTasksImpl>> v_tasks;
  {
    std::unique_lock<std::mutex> lock(set_mutex_);

    for (auto t : set_tasks_) {
      v_tasks.push_back(t);
    }
  }

  for (auto task : v_tasks) {
    if (task) {
      task->Update();
    }
  }
}


void FunapiTasksImpl::Insert(std::shared_ptr<FunapiTasksImpl> t) {
  std::unique_lock<std::mutex> lock(set_mutex_);
  set_tasks_.insert(t);
}


void FunapiTasksImpl::Erase(std::shared_ptr<FunapiTasksImpl> t) {
  std::unique_lock<std::mutex> lock(set_mutex_);
  set_tasks_.erase(t);
}


////////////////////////////////////////////////////////////////////////////////
// FunapiTask implementation.

FunapiTasks::FunapiTasks() {
  auto impl = std::make_shared<FunapiTasksImpl>();
  FunapiTasksImpl::Insert(impl);
  impl_ = impl;
}


FunapiTasks::~FunapiTasks() {
  if (auto impl = impl_.lock()) {
    FunapiTasksImpl::Erase(impl);
  }
}


std::shared_ptr<FunapiTasks> FunapiTasks::Create() {
  return std::make_shared<FunapiTasks>();
}


void FunapiTasks::UpdateAll() {
  FunapiTasksImpl::UpdateAll();
}


void FunapiTasks::Push(const TaskHandler &task) {
  if (auto impl = impl_.lock()) {
    impl->Push(task);
  }
}


void FunapiTasks::Set(const TaskHandler &task) {
  if (auto impl = impl_.lock()) {
    impl->Set(task);
  }
}


void FunapiTasks::Update() {
  if (auto impl = impl_.lock()) {
    impl->Update();
  }
}


////////////////////////////////////////////////////////////////////////////////
// FunapiThreadImpl implementation.

class FunapiThreadImpl : public FunapiTasksImpl {
 public:
  FunapiThreadImpl();
  virtual ~FunapiThreadImpl();

  static void Insert(const std::string &thread_id_string, std::shared_ptr<FunapiThreadImpl> t);
  static void Erase(std::shared_ptr<FunapiThreadImpl> t);
  static void Add(const std::string &thread_id_string, std::weak_ptr<FunapiThread> t);
  static std::shared_ptr<FunapiThread> Get(const std::string &thread_id_string);

  void Push(const TaskHandler &task);
  void Set(const TaskHandler &task);

  void Join();

  void SetThreadId(const std::string &thread_id_string);
  std::string GetThreadId();

 private:
  void Initialize();
  void Finalize();
  void JoinThread();
  void Thread();

  std::thread thread_;
  bool run_ = false;
  std::condition_variable_any condition_;
  std::string thread_id_;

  static std::unordered_map<std::string, std::shared_ptr<FunapiThreadImpl>> m_thread_impls_;
  static std::unordered_map<std::string, std::weak_ptr<FunapiThread>> m_threads_;
  static std::mutex m_mutex_;
};


std::unordered_map<std::string, std::shared_ptr<FunapiThreadImpl>> FunapiThreadImpl::m_thread_impls_;
std::unordered_map<std::string, std::weak_ptr<FunapiThread>> FunapiThreadImpl::m_threads_;
std::mutex FunapiThreadImpl::m_mutex_;


FunapiThreadImpl::FunapiThreadImpl() {
  Initialize();
}


FunapiThreadImpl::~FunapiThreadImpl() {
  Finalize();
}


void FunapiThreadImpl::Initialize() {
  run_ = true;
  thread_ = std::thread([this]{ Thread(); });
}


void FunapiThreadImpl::Finalize() {
  JoinThread();
}


void FunapiThreadImpl::Push(const TaskHandler &task)
{
  FunapiTasksImpl::Push(task);
  condition_.notify_one();
}


void FunapiThreadImpl::Set(const TaskHandler &task)
{
  FunapiTasksImpl::Set(task);
  condition_.notify_one();
}


void FunapiThreadImpl::JoinThread() {
  run_ = false;
  condition_.notify_all();
  if (thread_.joinable())
    thread_.join();
}


void FunapiThreadImpl::Thread() {
  while (run_) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (function_ == nullptr && queue_.empty()) {
        condition_.wait(mutex_);
      }
    }

    Update();
  }
}


void FunapiThreadImpl::Join() {
  JoinThread();
}


void FunapiThreadImpl::SetThreadId(const std::string &thread_id_string) {
  thread_id_ = thread_id_string;
}


std::string FunapiThreadImpl::GetThreadId() {
  return thread_id_;
}


void FunapiThreadImpl::Insert(const std::string &thread_id_string, std::shared_ptr<FunapiThreadImpl> t) {
  std::unique_lock<std::mutex> lock(m_mutex_);
  m_thread_impls_[thread_id_string] = t;
}


void FunapiThreadImpl::Add(const std::string &thread_id_string, std::weak_ptr<FunapiThread> t) {
  std::unique_lock<std::mutex> lock(m_mutex_);
  m_threads_[thread_id_string] = t;
}


void FunapiThreadImpl::Erase(std::shared_ptr<FunapiThreadImpl> t) {
  std::unique_lock<std::mutex> lock(m_mutex_);
  std::string thread_id_string = t->GetThreadId();
  m_threads_.erase(thread_id_string);
  m_thread_impls_.erase(thread_id_string);
}


std::shared_ptr<FunapiThread> FunapiThreadImpl::Get(const std::string &thread_id_string) {
  std::unique_lock<std::mutex> lock(m_mutex_);
  auto it = m_threads_.find(thread_id_string);
  if (it != m_threads_.end()) {
    if (!it->second.expired()) {
      if (auto t = it->second.lock()) {
        return t;
      }
    }
  }

  return nullptr;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiThread implementation.

FunapiThread::FunapiThread(const std::string &thread_id_string, const TaskHandler &task) {
  auto impl = std::make_shared<FunapiThreadImpl>();
  impl->Set(task);
  impl->SetThreadId(thread_id_string);
  FunapiThreadImpl::Insert(thread_id_string, impl);
  impl_ = impl;
}


FunapiThread::~FunapiThread() {
  if (auto impl = impl_.lock()) {
    FunapiThreadImpl::Erase(impl);
  }
}


std::shared_ptr<FunapiThread> FunapiThread::Create(const std::string &thread_id_string, const TaskHandler &task) {
  if (auto t = std::make_shared<FunapiThread>(thread_id_string, task)) {
    FunapiThreadImpl::Add(thread_id_string, t);
    return t;
  }

  return nullptr;
}


bool FunapiThread::Set(const std::string &thread_id_string, const TaskHandler &task) {
  if (auto t = FunapiThreadImpl::Get(thread_id_string)) {
    t->Set(task);
    return true;
  }

  return false;
}


bool FunapiThread::Push(const std::string &thread_id_string, const TaskHandler &task) {
  if (auto t = FunapiThreadImpl::Get(thread_id_string)) {
    t->Push(task);
    return true;
  }

  return false;
}


bool FunapiThread::Join(const std::string &thread_id_string) {
  if (auto t = FunapiThreadImpl::Get(thread_id_string)) {
    t->Join();
    return true;
  }

  return false;
}


void FunapiThread::Push(const TaskHandler &task) {
  if (auto impl = impl_.lock()) {
    impl->Push(task);
  }
}


void FunapiThread::Set(const TaskHandler &task) {
  if (auto impl = impl_.lock()) {
    impl->Set(task);
  }
}


void FunapiThread::Join() {
  if (auto impl = impl_.lock()) {
    impl->Join();
  }
}


std::shared_ptr<FunapiThread> FunapiThread::Get(const std::string &thread_id_string) {
  if (auto t = FunapiThreadImpl::Get(thread_id_string)) {
    return t;
  }

  return FunapiThread::Create(thread_id_string, nullptr);
}

}  // namespace fun
