// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_TASKS_H_
#define SRC_FUNAPI_TASKS_H_

#include "funapi_plugin.h"

namespace fun {

class FunapiTasksImpl;
class FUNAPI_API FunapiTasks : public std::enable_shared_from_this<FunapiTasks> {
 public:
  typedef std::function<bool()> TaskHandler;

  FunapiTasks();
  virtual ~FunapiTasks();

  static std::shared_ptr<FunapiTasks> Create();
  static void UpdateAll();

  void Push(const TaskHandler &task);
  int Size();
  void Update();

 private:
  std::shared_ptr<FunapiTasksImpl> impl_;
};


class FunapiThreadImpl;
class FUNAPI_API FunapiThread : public std::enable_shared_from_this<FunapiThread> {
 public:
  typedef FunapiTasks::TaskHandler TaskHandler;

  FunapiThread() = delete;
  FunapiThread(const std::string &thread_id);
  virtual ~FunapiThread();

  static std::shared_ptr<FunapiThread> Create(const std::string &thread_id);
  static std::shared_ptr<FunapiThread> Get(const std::string &thread_id);

  void Push(const TaskHandler &task);
  int Size();
  void Join();

 private:
  std::shared_ptr<FunapiThreadImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_TASKS_H_
