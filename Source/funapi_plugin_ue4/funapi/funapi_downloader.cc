// Copyright (C) 2014 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "Engine.h"
#include "UnrealString.h"

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#include <process.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <io.h>
#include <direct.h>
#include <functional>
#else
#include <netinet/in.h>
#include <algorithm>
#endif

#include <list>

#include <sys/stat.h>

#include <openssl/md5.h>
#include "curl/curl.h"
#include "rapidjson/stringbuffer.h"

#include "./funapi_downloader.h"
#include "./funapi_utils.h"
#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif


namespace fun {

using std::map;


namespace {

////////////////////////////////////////////////////////////////////////////////
// Types.

typedef helper::Binder1<int, void *> AsyncRequestCallback;
typedef helper::Binder1<string, void *> AsyncRequestMd5Callback;
typedef helper::Binder2<void *, int, void *> AsyncResponseCallback;


enum DownloadState {
  kDownloadReady,
  kDownloadStart,
  kDownloading,
  kDownloadComplete,
};


enum RequestState {
  kWebRequestStart = 0,
  kWebRequestInProgress,
  kWebRequestEnd,
};


struct DownloadUrl {
  string host;
  string url;
};


struct DownloadFile {
  string path;
  string md5;
};


struct AsyncRequest {
  enum RequestType {
    kRequestList = 0,
    kRequestFile,
    kRequestMD5,
  };

  enum AsyncState {
    kStateStart = 0,
    kStateUpdate,
    kStateFinish,
  };

  RequestType type_;
  AsyncState state_;

  struct {
    string url_;
    AsyncRequestCallback callback_;
    AsyncResponseCallback response_header_;
    AsyncResponseCallback response_body_;
  } web_request_;

  struct {
    string path_;
    AsyncRequestMd5Callback callback_;
  } md5_request_;
};


////////////////////////////////////////////////////////////////////////////////
// Constants.

// Funapi version
const int kCurrentFunapiProtocolVersion = 1;

// Funapi header-related constants.
const char kHeaderDelimeter[] = "\n";
const char kHeaderFieldDelimeter[] = ":";
const char kHeaderFieldContentLength[] = "content-length";

// File-related constants.
const int kMd5DivideFileLength = 1024 * 1024;  // 1Mb
const char kCacheFileName[] = "cached_files_list";


////////////////////////////////////////////////////////////////////////////////
// Global variables.

typedef std::list<AsyncRequest> AsyncQueue;
AsyncQueue async_queue;

bool async_thread_run = false;
#if PLATFORM_WINDOWS
HANDLE async_queue_thread;
HANDLE async_queue_mutex;

#define PATH_DELIMITER        '\\'
#else
pthread_t async_queue_thread;
pthread_mutex_t async_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t async_queue_cond = PTHREAD_COND_INITIALIZER;

#define PATH_DELIMITER        '/'
#endif


////////////////////////////////////////////////////////////////////////////////
// Helper functions.

struct compare_path : public std::binary_function<DownloadFile*, string, bool> {
  bool operator() (const DownloadFile *lhs, const string &path) const {
    return lhs->path == path;
  }
};


size_t HttpResponseCb(void *data, size_t size, size_t count, void *cb) {
  AsyncResponseCallback *callback = (AsyncResponseCallback*)(cb);
  if (callback != NULL)
    (*callback)(data, size * count);
  return size * count;
}



////////////////////////////////////////////////////////////////////////////////
// Asynchronous operation related..

#if PLATFORM_WINDOWS
uint32_t WINAPI AsyncQueueThreadProc(LPVOID /*arg*/) {
#else
void *AsyncQueueThreadProc(void *arg) {
#endif
  LOG("Thread for async operation has been created.");

  CURL *ctx = NULL;
  CURLM *mctx = NULL;
  int maxfd, running = 0;
  struct timeval timeout;
  fd_set fdread, fdwrite, fdexcep;
  FILE *fp = NULL;
  MD5_CTX lctx;
  uint8_t *buffer = new uint8_t [kMd5DivideFileLength];
  int length = 0, read_len = 0, offset = 0;

  while (async_thread_run) {
    // Waits until we have something to process.
    pthread_mutex_lock(&async_queue_mutex);
    while (async_thread_run && async_queue.empty()) {
#if PLATFORM_WINDOWS
      ReleaseMutex(async_queue_mutex);
      Sleep(1000);
      WaitForSingleObject(async_queue_mutex, INFINITE);
#else
      pthread_cond_wait(&async_queue_cond, &async_queue_mutex);
#endif
    }

    // Moves element to a worker queue and releaes the mutex
    // for contention prevention.
    AsyncQueue work_queue;
    work_queue.swap(async_queue);
    pthread_mutex_unlock(&async_queue_mutex);

    for (AsyncQueue::iterator itr = work_queue.begin(); itr != work_queue.end(); ) {
      switch (itr->type_) {
      case AsyncRequest::kRequestList:
      case AsyncRequest::kRequestFile: {
          if (itr->state_ == AsyncRequest::kStateStart) {
            running = 0;
            ctx = curl_easy_init();

            curl_easy_setopt(ctx, CURLOPT_URL, itr->web_request_.url_.c_str());
            curl_easy_setopt(ctx, CURLOPT_HEADERDATA, &itr->web_request_.response_header_);
            curl_easy_setopt(ctx, CURLOPT_WRITEDATA, &itr->web_request_.response_body_);
            curl_easy_setopt(ctx, CURLOPT_WRITEFUNCTION, &HttpResponseCb);

            mctx = curl_multi_init();
            curl_multi_add_handle(mctx, ctx);

            itr->web_request_.callback_(kWebRequestStart);

            curl_multi_perform(mctx, &running);
            itr->state_ = AsyncRequest::kStateUpdate;
          } else if (itr->state_ == AsyncRequest::kStateUpdate) {
            FD_ZERO(&fdread);
            FD_ZERO(&fdwrite);
            FD_ZERO(&fdexcep);

            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            long curl_timeo = -1;
            curl_multi_timeout(mctx, &curl_timeo);
            if (curl_timeo >= 0) {
              timeout.tv_sec = curl_timeo / 1000;
              if (timeout.tv_sec > 1)
                timeout.tv_sec = 1;
              else
                timeout.tv_usec = (curl_timeo % 1000) * 1000;
            }

            maxfd = -1;
            curl_multi_fdset(mctx, &fdread, &fdwrite, &fdexcep, &maxfd);

            if (maxfd != -1) {
              int rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
              assert(rc >= 0);

              if (rc == -1) {
                running = 0;
                LOG("select() returns error.");
              } else {
                curl_multi_perform(mctx, &running);
              }
            }

            if (!running)
              itr->state_ = AsyncRequest::kStateFinish;
          } else if (itr->state_ == AsyncRequest::kStateFinish) {
            itr->web_request_.callback_(kWebRequestEnd);
            itr = work_queue.erase(itr);

            curl_multi_remove_handle(mctx, ctx);
            curl_easy_cleanup(ctx);
            curl_multi_cleanup(mctx);
          } else {
            LOG1("Invalid state value. state: %d", (int)itr->state_);
            assert(false);
          }
        }
        break;

      case AsyncRequest::kRequestMD5: {
          if (itr->state_ == AsyncRequest::kStateStart) {
            MD5_Init(&lctx);

            fp = fopen(itr->md5_request_.path_.c_str(), "rb");
            assert(fp != NULL);
            fseek(fp, 0, SEEK_END);
            length = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            offset = 0;
            itr->state_ = AsyncRequest::kStateUpdate;
          } else if (itr->state_ == AsyncRequest::kStateUpdate) {
            if (offset + kMd5DivideFileLength > length)
              read_len = length - offset;
            else
              read_len = kMd5DivideFileLength;

            fread(buffer, sizeof(uint8_t), read_len, fp);

            MD5_Update(&lctx, buffer, read_len);
            offset += read_len;

            if (offset >= length)
              itr->state_ = AsyncRequest::kStateFinish;
          } else if (itr->state_ == AsyncRequest::kStateFinish) {
            char md5msg[MD5_DIGEST_LENGTH * 2 + 1];
            unsigned char digest[MD5_DIGEST_LENGTH];

            MD5_Final(digest, &lctx);
            fclose(fp);

            for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
                sprintf(md5msg + (i * 2), "%02x", digest[i]);
            }
            md5msg[MD5_DIGEST_LENGTH * 2] = '\0';

            LOG2("MD5(%s) >> %s", *FString(itr->md5_request_.path_.c_str()), *FString(md5msg));

            itr->md5_request_.callback_(md5msg);
            itr = work_queue.erase(itr);
          } else {
            LOG1("Invalid state value. state: %d", (int)itr->state_);
            assert(false);
          }
        }
        break;

      default:
        LOG1("Invalid request type. type: %d", (int)itr->type_);
        ++itr;
        break;
      }
    }

    // Puts back requests that requires more work.
    // We should respect the order.
    pthread_mutex_lock(&async_queue_mutex);
    async_queue.splice(async_queue.begin(), work_queue);
    pthread_mutex_unlock(&async_queue_mutex);
  }

  LOG("Thread for async operation is terminating.");
  return NULL;
}


void AsyncRequestList(const string &url, AsyncRequestCallback cb_request,
    AsyncResponseCallback cb_header, AsyncResponseCallback cb_body) {
  LOG("Queueing async request list file.");

  AsyncRequest r;
  r.type_ = AsyncRequest::kRequestList;
  r.state_ = AsyncRequest::kStateStart;
  r.web_request_.callback_ = cb_request;
  r.web_request_.response_header_ = cb_header;
  r.web_request_.response_body_ = cb_body;
  r.web_request_.url_ = url;

  pthread_mutex_lock(&async_queue_mutex);
  async_queue.push_back(r);
  pthread_cond_signal(&async_queue_cond);
  pthread_mutex_unlock(&async_queue_mutex);
}


void AsyncRequestFile (const string &url, AsyncRequestCallback cb_request,
    AsyncResponseCallback cb_header, AsyncResponseCallback cb_body) {
  LOG("Queueing async request resource file.");

  AsyncRequest r;
  r.type_ = AsyncRequest::kRequestFile;
  r.state_ = AsyncRequest::kStateStart;
  r.web_request_.callback_ = cb_request;
  r.web_request_.response_header_ = cb_header;
  r.web_request_.response_body_ = cb_body;
  r.web_request_.url_ = url;

  pthread_mutex_lock(&async_queue_mutex);
  async_queue.push_back(r);
  pthread_cond_signal(&async_queue_cond);
  pthread_mutex_unlock(&async_queue_mutex);
}


void AsyncRequestMd5 (const string &path, AsyncRequestMd5Callback cb_request) {
  LOG("Queueing async request MD5.");

  AsyncRequest r;
  r.type_ = AsyncRequest::kRequestMD5;
  r.state_ = AsyncRequest::kStateStart;
  r.md5_request_.path_ = path;
  r.md5_request_.callback_ = cb_request;

  pthread_mutex_lock(&async_queue_mutex);
  async_queue.push_back(r);
  pthread_cond_signal(&async_queue_cond);
  pthread_mutex_unlock(&async_queue_mutex);
}

}  // unnamed namespace



////////////////////////////////////////////////////////////////////////////////
// FunapiHttpDownloaderImpl implementation.

class FunapiHttpDownloaderImpl {
 public:
  typedef FunapiHttpDownloader::OnUpdate OnUpdate;
  typedef FunapiHttpDownloader::OnFinished OnFinished;

  FunapiHttpDownloaderImpl(const char *target_path,
      const OnUpdate &cb1, const OnFinished &cb2);
  ~FunapiHttpDownloaderImpl();

  bool StartDownload(const string url);
  void Stop();
  bool Connected() const;

 private:
  typedef std::list<DownloadUrl*> UrlList;
  typedef std::list<DownloadFile*> FileList;
  typedef std::map<string, string> HeaderFields;

  void ClearDownloadList();
  void ClearCachedFileList();

  void LoadCachedFileList();
  void SaveCachedFileList();

  void CheckFileList(FileList &list);
  void DownloadListFile(const string &url);
  void DownloadResourceFile();

  static void DownloadListCbWrapper(int state, void *arg);
  static void DownloadFileCbWrapper(int state, void *arg);
  static void WebResponseHeaderCbWrapper(void *data, int len, void *arg);
  static void WebResponseBodyCbWrapper(void *data, int len, void *arg);
  static void ComputeMd5CbWrapper(string md5hashs, void *arg);

  void DownloadListCb(int state);
  void DownloadFileCb(int state);
  void WebResponseHeaderCb(void *data, int len);
  void WebResponseBodyCb(void *data, int len);
  void ComputeMd5Cb(const string &md5hash);

  static const int kUnitBufferSize = 65536;

  // State-related.
#if PLATFORM_WINDOWS
  HANDLE mutex_;
#else
  mutable pthread_mutex_t mutex_;
#endif
  DownloadState state_;

  // Url-related.
  string host_url_;
  string target_path_;
  UrlList url_list_;
  FileList download_list_;
  FileList cached_files_list_;
  DownloadFile *cur_download_;

  // Callback functions
  OnUpdate on_update_;
  OnFinished on_finished_;

  // Buffer-related.
  HeaderFields header_fields_;
  struct iovec buffer_;
  int recv_offset_;
  int file_length_;
};


FunapiHttpDownloaderImpl::FunapiHttpDownloaderImpl(
    const char *target_path, const OnUpdate &cb1, const OnFinished &cb2)
    : state_(kDownloadReady), on_update_(cb1), on_finished_(cb2) {
  target_path_ = target_path;
  if (target_path_.at(target_path_.length() - 1) != PATH_DELIMITER)
    target_path_.append(1, PATH_DELIMITER);

  header_fields_.clear();

  buffer_.iov_len = kUnitBufferSize;
  buffer_.iov_base = new uint8_t[buffer_.iov_len + 1];
  assert(buffer_.iov_base);

  LoadCachedFileList();
}


FunapiHttpDownloaderImpl::~FunapiHttpDownloaderImpl() {
  ClearDownloadList();
  ClearCachedFileList();
}


void FunapiHttpDownloaderImpl::ClearDownloadList() {
  pthread_mutex_lock(&mutex_);
  if (!url_list_.empty()) {
    UrlList::iterator itr = url_list_.begin();
    for (; itr != url_list_.end(); ++itr) {
      delete (*itr);
    }
    url_list_.clear();
  }

  if (!download_list_.empty()) {
    FileList::iterator itr = download_list_.begin();
    for (; itr != download_list_.end(); ++itr) {
      delete (*itr);
    }
    download_list_.clear();
  }
  pthread_mutex_unlock(&mutex_);
}


void FunapiHttpDownloaderImpl::ClearCachedFileList() {
  if (cached_files_list_.empty())
    return;

  pthread_mutex_lock(&mutex_);
  FileList::iterator itr = cached_files_list_.begin();
  for (; itr != cached_files_list_.end(); ++itr) {
    delete (*itr);
  }

  cached_files_list_.clear();
  pthread_mutex_unlock(&mutex_);
}


bool FunapiHttpDownloaderImpl::StartDownload(const string url) {
  char ver[128];
  sprintf(ver, "/v%d/", kCurrentFunapiProtocolVersion);
  int index = url.find(ver);
  if (index <= 0) {
    LOG1("Invalid request url : %s", *FString(url.c_str()));
    assert(false);
    return false;
  }

  pthread_mutex_lock(&mutex_);
  string host_url = url.substr(0, index + strlen(ver));
  DownloadUrl *info = new DownloadUrl();
  info->host = host_url;
  info->url = url;

  if (state_ == kDownloading) {
    url_list_.push_back(info);
  } else {
    state_ = kDownloadStart;
    host_url_ = host_url;
    LOG("Start Download.");

    DownloadListFile(url);
  }
  pthread_mutex_unlock(&mutex_);
  return true;
}


void FunapiHttpDownloaderImpl::Stop() {
  if (state_ == kDownloadStart || state_ == kDownloading) {
    on_finished_(kDownloadFailed);
  }

  ClearDownloadList();
}


bool FunapiHttpDownloaderImpl::Connected() const {
  return state_ == kDownloading || state_ == kDownloadComplete;
}


void FunapiHttpDownloaderImpl::LoadCachedFileList() {
  ClearCachedFileList();

  string path = target_path_ + kCacheFileName;
  LOG1("Cached file path: %s", *FString(path.c_str()));
  if (access(path.c_str(), F_OK) == -1)
    return;

  FILE *fp = fopen(path.c_str(), "r");
  assert(fp != NULL);
  fseek(fp,0,SEEK_END);
  ssize_t size = ftell(fp);
  fseek(fp,0,SEEK_SET);

  char *buffer = new char [size + 1];
  size_t readsize = fread(buffer, sizeof(unsigned char), size, fp);
  buffer[readsize] = '\0';
  fclose(fp);

  if (readsize <= 0) {
    LOG1("Get data from file '%s' failed!", *FString(path.c_str()));
    assert(false);
    return;
  }

  Json json;
  json.Parse<0>(buffer);
  assert(json.IsObject());

  rapidjson::Value &list = json["data"];
  assert(list.IsArray());

  int count = (int)list.Size();
  for (int i = 0; i < count; ++i) {
    DownloadFile *info = new DownloadFile();
    info->path = list[i]["path"].GetString();
    info->md5 = list[i]["md5"].GetString();
    cached_files_list_.push_back(info);
  }

  LOG1("Load file's information : %d", cached_files_list_.size());
}


void FunapiHttpDownloaderImpl::SaveCachedFileList() {
  string data = "{ \"data\": [ ";
  int index = 0;

  FileList::const_iterator itr = cached_files_list_.begin();
  for (; itr != cached_files_list_.end(); ++itr, ++index) {
    const DownloadFile *info = *itr;
    data.append("{ \"path\":\"");
    data.append(info->path);
    data.append("\", \"md5\":\"");
    data.append(info->md5);
    data.append("\" }");
    if (index < cached_files_list_.size() - 1)
      data.append(", ");
  }
  data.append(" ] }");
  LOG1("SAVE DATA : %s", *FString(data.c_str()));

  string path = target_path_ + kCacheFileName;
  if (access(path.c_str(), F_OK) == -1)
    remove(path.c_str());

  FILE *fp = fopen(path.c_str(), "w");
  assert(fp != NULL);
  fwrite(data.c_str(), sizeof(char), data.length(), fp);
  fclose(fp);

  LOG1("Save file's information : %d", cached_files_list_.size());
}


// Check MD5
void FunapiHttpDownloaderImpl::CheckFileList(FileList &list) {
  if (list.empty() || cached_files_list_.empty())
    return;

  pthread_mutex_lock(&mutex_);
  FileList remove_list;

  // Deletes local files
  FileList::const_iterator itr = cached_files_list_.begin();
  for (; itr != cached_files_list_.end(); ++itr) {
    DownloadFile *info = *itr;
    FileList::iterator itr_find = std::find_if(list.begin(), list.end(),
        std::bind2nd<compare_path>(compare_path(), info->path));
    if (itr_find == list.end()) {
      string path = target_path_ + info->path;
      remove(path.c_str());
      remove_list.push_back(info);
      LOG1("Deleted resource file. path: %s", *FString(path.c_str()));
    }
  }

  if (!remove_list.empty()) {
    for (itr = remove_list.begin(); itr != remove_list.end(); ++itr) {
      DownloadFile *info = *itr;
      delete info;
      cached_files_list_.remove(info);
    }
    remove_list.clear();
  }

  // Check download files
  string path = target_path_ + kCacheFileName;
  struct tm *tm;
  struct stat st;
  stat(path.c_str(), &st);
#if PLATFORM_ANDROID
  tm = gmtime((time_t*)&(st.st_mtime_nsec));
#elif PLATFORM_WINDOWS
  tm = gmtime(&st.st_mtime);
#else
  tm = gmtime(&st.st_mtimespec.tv_sec);
#endif

  for (itr = list.begin(); itr != list.end(); ++itr) {
    DownloadFile *info = *itr;
    FileList::iterator itr_find = std::find_if(
        cached_files_list_.begin(), cached_files_list_.end(),
        std::bind2nd<compare_path>(compare_path(), info->path));
    if (itr_find != cached_files_list_.end()) {
      bool is_same = true;
      const DownloadFile *find_info = *itr_find;
      if (info->md5 != find_info->md5) {
        is_same = false;
      } else {
        string path2 = target_path_ + find_info->path;
        struct tm *tm2;
        struct stat st2;
        stat(path2.c_str(), &st2);
#if PLATFORM_ANDROID
        tm2 = gmtime((time_t*)&(st2.st_mtime_nsec));
#elif PLATFORM_WINDOWS
        tm2 = gmtime(&st.st_mtime);
#else
        tm2 = gmtime(&(st2.st_mtimespec.tv_sec));
#endif
        if (tm->tm_year != tm2->tm_year || tm->tm_yday != tm2->tm_yday ||
            tm->tm_hour != tm2->tm_hour || tm->tm_min != tm2->tm_min ||
            tm->tm_sec != tm2->tm_sec)
          is_same = false;
      }

      if (is_same) {
        remove_list.push_back(info);
      } else {
        cached_files_list_.remove(info);
      }
    }
  }

  if (!remove_list.empty()) {
    for (itr = remove_list.begin(); itr != remove_list.end(); ++itr) {
      DownloadFile *info = *itr;
      delete info;
      list.remove(info);
    }
    remove_list.clear();
  }
  pthread_mutex_unlock(&mutex_);
}


void FunapiHttpDownloaderImpl::DownloadListFile(const string &url) {
  cur_download_ = NULL;

  // Request a list of download files.
  LOG1("List file url : %s", *FString(url.c_str()));
  AsyncRequestList(url,
      AsyncRequestCallback(&FunapiHttpDownloaderImpl::DownloadListCbWrapper, (void*)this),
      AsyncResponseCallback(&FunapiHttpDownloaderImpl::WebResponseHeaderCbWrapper, (void*)this),
      AsyncResponseCallback(&FunapiHttpDownloaderImpl::WebResponseBodyCbWrapper, (void*)this));
}


void FunapiHttpDownloaderImpl::DownloadResourceFile() {
  if (download_list_.size() <= 0) {
    if (url_list_.size() > 0) {
      DownloadUrl *info = *url_list_.begin();
      url_list_.erase(url_list_.begin());

      host_url_ = info->host;
      DownloadListFile(info->url);
    } else {
      state_ = kDownloadComplete;
      LOG("Download completed.");

      on_finished_(kDownloadSuccess);

      SaveCachedFileList();
    }
  } else {
    cur_download_ = *download_list_.begin();

    // Check directory
    string path = target_path_;
    int offset = cur_download_->path.find_last_of('/');
    if (offset >= 0)
      path.append(cur_download_->path.substr(0, offset));

    if (access(path.c_str(), F_OK) == -1) {
      if (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
        LOG1("Create folder failed. path: %s", *FString(path.c_str()));
        assert(false);
        return;
      }
    }

    string url = host_url_ + cur_download_->path;
    AsyncRequestFile(url,
        AsyncRequestCallback(&FunapiHttpDownloaderImpl::DownloadFileCbWrapper, (void*)this),
        AsyncResponseCallback(&FunapiHttpDownloaderImpl::WebResponseHeaderCbWrapper, (void*)this),
        AsyncResponseCallback(&FunapiHttpDownloaderImpl::WebResponseBodyCbWrapper, (void*)this));
  }
}


void FunapiHttpDownloaderImpl::DownloadListCbWrapper(int state, void *arg) {
  FunapiHttpDownloaderImpl *obj = reinterpret_cast<FunapiHttpDownloaderImpl*>(arg);
  obj->DownloadListCb(state);
}


void FunapiHttpDownloaderImpl::DownloadFileCbWrapper(int state, void *arg) {
  FunapiHttpDownloaderImpl *obj = reinterpret_cast<FunapiHttpDownloaderImpl*>(arg);
  obj->DownloadFileCb(state);
}


void FunapiHttpDownloaderImpl::WebResponseHeaderCbWrapper(void *data, int len, void *arg) {
  FunapiHttpDownloaderImpl *obj = reinterpret_cast<FunapiHttpDownloaderImpl*>(arg);
  obj->WebResponseHeaderCb(data, len);
}


void FunapiHttpDownloaderImpl::WebResponseBodyCbWrapper(void *data, int len, void *arg) {
  FunapiHttpDownloaderImpl *obj = reinterpret_cast<FunapiHttpDownloaderImpl*>(arg);
  obj->WebResponseBodyCb(data, len);
}


void FunapiHttpDownloaderImpl::ComputeMd5CbWrapper(string md5hash, void *arg) {
  FunapiHttpDownloaderImpl *obj = reinterpret_cast<FunapiHttpDownloaderImpl*>(arg);
  obj->ComputeMd5Cb(md5hash);
}


void FunapiHttpDownloaderImpl::DownloadListCb(int state) {
  LOG1("DownloadListCb called. state: %d", state);
  if (state == kWebRequestStart) {
    recv_offset_ = 0;
    file_length_ = 0;
    header_fields_.clear();
  } else if (state == kWebRequestEnd) {
    state_ = kDownloading;

    char *data = reinterpret_cast<char *>(buffer_.iov_base);
    bool failed = false;

    data[recv_offset_] = '\0';
    LOG1("Json data >> %s", *FString(data));

    Json json;
    json.Parse<0>(data);
    assert(json.IsObject());

    const rapidjson::Value& list = json["data"];
    assert(list.IsArray());
    if (list.Size() <= 0) {
      LOG("Invalid list data. List count is 0.");
      assert(false);
      failed = true;
    } else {
      int count = (int)list.Size();
      for (int i = 0; i < count; ++i) {
        DownloadFile *info = new DownloadFile();
        info->path = list[i]["path"].GetString();
        info->md5 = list[i]["md5"].GetString();

        download_list_.push_back(info);
      }

      // Check files
      CheckFileList(download_list_);

      // Downlaod file
      DownloadResourceFile();
    }

    if (failed)
      Stop();
  }
}


void FunapiHttpDownloaderImpl::DownloadFileCb(int state) {
  LOG1("DownloadFileCb called. state: %d", state);
  if (state == kWebRequestStart) {
    recv_offset_ = 0;
    file_length_ = 0;
    header_fields_.clear();
  } else if (state == kWebRequestEnd) {
    if (download_list_.size() > 0) {
      cached_files_list_.push_back(cur_download_);
      download_list_.erase(download_list_.begin());

      LOG1("Download complete - %s", *FString(cur_download_->path.c_str()));
      string target = target_path_ + cur_download_->path;
      LOG1("Save file : %s", *FString(target.c_str()));
      FILE *fp = fopen(target.c_str(), "wb");
      assert(fp != NULL);
      fwrite(buffer_.iov_base, sizeof(uint8_t), recv_offset_, fp);
      fclose(fp);

      AsyncRequestMd5(target,
          AsyncRequestMd5Callback(&FunapiHttpDownloaderImpl::ComputeMd5CbWrapper, (void*)this));
    }
  }
}


void FunapiHttpDownloaderImpl::WebResponseHeaderCb(void *data, int len) {
  char buf[1024];
  memcpy(buf, data, len);
  buf[len-2] = '\0';

  char *ptr = std::search(buf, buf + len, kHeaderFieldDelimeter,
      kHeaderFieldDelimeter + sizeof(kHeaderFieldDelimeter) - 1);
  ssize_t eol_offset = ptr - buf;
  if (eol_offset >= len)
    return;

  // Generates null-terminated string by replacing the delimeter with \0.
  *ptr = '\0';
  const char *e1 = buf, *e2 = ptr + 1;
  while (*e2 == ' ' || *e2 == '\t') ++e2;
  if (std::strcmp(e1, kHeaderFieldContentLength) == 0) {
    file_length_ = atoi(e2);

    if (file_length_ >= buffer_.iov_len) {
      size_t new_size = file_length_ + 1;
      LOG1("Resizing a buffer to %d bytes.", new_size);
      uint8_t *new_buffer = new uint8_t[new_size + 1];
      memmove(new_buffer, buffer_.iov_base, recv_offset_);
      delete [] reinterpret_cast<uint8_t *>(buffer_.iov_base);
      buffer_.iov_base = new_buffer;
      buffer_.iov_len = new_size;
    }
  }
  LOG2("Decoded header field '%s' => '%s'", *FString(e1), *FString(e2));
  header_fields_[e1] = e2;
}


void FunapiHttpDownloaderImpl::WebResponseBodyCb(void *data, int len) {
  int offset = recv_offset_;
  uint8_t *buf = reinterpret_cast<uint8_t*>(buffer_.iov_base);
  memcpy(buf + offset, data, len);
  recv_offset_ += len;

  //LOG("path: " << cur_download_->path << " total: " << file_length_ << " receive: " << len);
}


void FunapiHttpDownloaderImpl::ComputeMd5Cb(const string &md5hash) {
  cur_download_->md5 = md5hash;

  DownloadResourceFile();
}



////////////////////////////////////////////////////////////////////////////////
// FunapiHttpDownloader implementation.

FunapiHttpDownloader::FunapiHttpDownloader(
    const char *target_path, const OnUpdate &cb1, const OnFinished &cb2)
    : impl_(new FunapiHttpDownloaderImpl(target_path, cb1, cb2)) {

  curl_global_init(CURL_GLOBAL_ALL);

  // Creates a thread to handle async operations.
  async_thread_run = true;
#if PLATFORM_WINDOWS
  pthread_mutex_init(&async_queue_mutex, NULL);
  async_queue_thread = (HANDLE)_beginthreadex(NULL, 0, AsyncQueueThreadProc, NULL, 0, NULL);
  assert(async_queue_thread != 0);
#else
  int r = pthread_create(&async_queue_thread, NULL, AsyncQueueThreadProc, NULL);
  assert(r == 0);
#endif
}

FunapiHttpDownloader::~FunapiHttpDownloader() {
  curl_global_cleanup();

  // Terminates the thread for async operations.
  async_thread_run = false;
#if PLATFORM_WINDOWS
  WaitForSingleObject(async_queue_thread, INFINITE);
  CloseHandle(async_queue_thread);

  WaitForSingleObject(async_queue_mutex, INFINITE);
  CloseHandle(async_queue_mutex);
#else
  pthread_cond_broadcast(&async_queue_cond);
  void *dummy;
  pthread_join(async_queue_thread, &dummy);
  (void) dummy;
#endif
}

bool FunapiHttpDownloader::StartDownload(const char *hostname_or_ip,
    uint16_t port, const char *list_filename, bool https) {
  char url[1024];
  sprintf(url, "%s://%s:%d/v%d/%s",
      https ? "https" : "http", hostname_or_ip, port,
      kCurrentFunapiProtocolVersion, list_filename);

  return impl_->StartDownload(url);
}


bool FunapiHttpDownloader::StartDownload(const char *url) {
  return impl_->StartDownload(url);
}


void FunapiHttpDownloader::Stop() {
  impl_->Stop();
}


bool FunapiHttpDownloader::Connected() const {
  return impl_->Connected();
}

}  // namespace fun
