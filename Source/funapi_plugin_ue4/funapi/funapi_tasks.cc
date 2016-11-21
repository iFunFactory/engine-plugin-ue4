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
  static void Add(std::weak_ptr<FunapiTasksImpl> t);

  void Update();
  virtual void Push(const TaskHandler &task);
  virtual void Set(const TaskHandler &task);

 protected:
  std::function<bool()> function_ = nullptr;
  std::queue<std::function<bool()>> queue_;
  std::mutex mutex_;

 private:
  static std::vector<std::weak_ptr<FunapiTasksImpl>> v_tasks_;
  static std::mutex v_mutex_;
};

std::vector<std::weak_ptr<FunapiTasksImpl>> FunapiTasksImpl::v_tasks_;
std::mutex FunapiTasksImpl::v_mutex_;


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

    if (task() == false) {
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
    std::unique_lock<std::mutex> lock(v_mutex_);
    if(!v_tasks_.empty()) {
      std::vector<std::weak_ptr<FunapiTasksImpl>> weak_tasks;
      for (auto task : v_tasks_) {
        if (!task.expired()) {
          weak_tasks.push_back(task);
          v_tasks.push_back(task.lock());
        }
      }

      v_tasks_.swap(weak_tasks);
    }
  }

  for (auto task : v_tasks) {
    if (task) {
      task->Update();
    }
  }
}


void FunapiTasksImpl::Add(std::weak_ptr<FunapiTasksImpl> t) {
  std::unique_lock<std::mutex> lock(v_mutex_);
  v_tasks_.push_back(t);
}


////////////////////////////////////////////////////////////////////////////////
// FunapiTask implementation.


FunapiTasks::FunapiTasks() : impl_(std::make_shared<FunapiTasksImpl>()) {
  FunapiTasksImpl::Add(impl_);
}


FunapiTasks::~FunapiTasks() {
}


std::shared_ptr<FunapiTasks> FunapiTasks::Create() {
  return std::make_shared<FunapiTasks>();
}


void FunapiTasks::UpdateAll() {
  FunapiTasksImpl::UpdateAll();
}


void FunapiTasks::Push(const TaskHandler &task) {
  impl_->Push(task);
}


void FunapiTasks::Set(const TaskHandler &task) {
  impl_->Set(task);
}


void FunapiTasks::Update() {
  impl_->Update();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiThreadImpl implementation.

class FunapiThreadImpl : public FunapiTasksImpl {
public:
  FunapiThreadImpl();
  virtual ~FunapiThreadImpl();

  static void Add(const std::string &thread_id_string, std::weak_ptr<FunapiThread> t);
  static std::shared_ptr<FunapiThread> Get(const std::string &thread_id_string);

  void Push(const TaskHandler &task);
  void Set(const TaskHandler &task);

  void Join();

 private:
  void Initialize();
  void Finalize();
  void JoinThread();
  void Thread();

  std::thread thread_;
  bool run_ = false;
  std::condition_variable_any condition_;

  static std::unordered_map<std::string, std::weak_ptr<FunapiThread>> m_threads_;
  static std::mutex m_mutex_;
};


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


void FunapiThreadImpl::Add(const std::string &thread_id_string, std::weak_ptr<FunapiThread> t) {
  std::unique_lock<std::mutex> lock(m_mutex_);
  m_threads_[thread_id_string] = t;
}


std::shared_ptr<FunapiThread> FunapiThreadImpl::Get(const std::string &thread_id_string) {
  std::unique_lock<std::mutex> lock(m_mutex_);
  auto it = m_threads_.find(thread_id_string);
  if (it != m_threads_.end()) {
    if (!it->second.expired()) {
      if (auto t = it->second.lock()) {
        return t;
      }
      else {
        m_threads_.erase(it);
      }
    }
  }

  return nullptr;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiThread implementation.

FunapiThread::FunapiThread(const std::string &thread_id_string, const TaskHandler &task) : impl_(std::make_shared<FunapiThreadImpl>()) {
  impl_->Set(task);
}


FunapiThread::~FunapiThread() {
}


std::shared_ptr<FunapiThread> FunapiThread::Create(const std::string &thread_id_string, const TaskHandler &task) {
  auto t = std::make_shared<FunapiThread>(thread_id_string, task);
  FunapiThreadImpl::Add(thread_id_string, t);
  return t;
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
  impl_->Push(task);
}


void FunapiThread::Set(const TaskHandler &task) {
  impl_->Set(task);
}


void FunapiThread::Join() {
  impl_->Join();
}


std::shared_ptr<FunapiThread> FunapiThread::Get(const std::string &thread_id_string) {
  if (auto t = FunapiThreadImpl::Get(thread_id_string)) {
    return t;
  }

  return FunapiThread::Create(thread_id_string, nullptr);
}

}  // namespace fun
