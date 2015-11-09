// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin_ue4.h"

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif
#include "funapi_downloader.h"
#include "async_delegates.h"

#include <ctime>
#include <fstream>
#include <future>
#include <sys/stat.h>

#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif


namespace fun {

using namespace std::chrono;


namespace {

////////////////////////////////////////////////////////////////////////////////
const int kCurrentFunapiDownloaderProtocolVersion = 1;

const string kRootPath = "client_data";
const string kCachedFilename = "cached_files";
const int kDownloadUnitBufferSize = 65536;
const int kBlockSize = 1024 * 1024; // 1M

#if PLATFORM_WINDOWS
#define PATH_DELIMITER        '\\'
#else
#define PATH_DELIMITER        '/'
#endif

enum class State : short {
  None,
  Ready,
  Start,
  Downloading,
  Completed
};


struct DownloadUrl {
  string host;
  string url;
};


struct DownloadFileInfo {
  string path;          // save path
  uint32 size;          // file size
  string hash;          // md5 hash
  string hash_front;    // front part of file (1MB)
};


////////////////////////////////////////////////////////////////////////////////
// Helper functions.

string GetDirectory (const string& path) {
  auto offset = path.find_last_of("\\/");
  if (offset == string::npos)
    return "";

  return path.substr(0, offset);
}

string GetFilename (const string& path) {
  auto offset = path.find_last_of("\\/");
  if (offset == string::npos)
    return path;

  return path.substr(offset + 1);
}

string MakeHashString (uint8* digest) {
  char md5msg[32 + 1];
  for (int i = 0; i < 16; ++i) {
    sprintf(md5msg + (i * 2), "%02x", digest[i]);
  }
  md5msg[32] = '\0';

  return md5msg;
}

}  // unnamed namespace



////////////////////////////////////////////////////////////////////////////////
// FunapiHttpDownloaderImpl implementation.

class FunapiHttpDownloaderImpl {
 public:
  typedef FunapiHttpDownloader::OnUpdate OnUpdate;
  typedef FunapiHttpDownloader::OnFinished OnFinished;

  FunapiHttpDownloaderImpl (FunapiHttpDownloader*);
  ~FunapiHttpDownloaderImpl ();

  bool IsDownloading () const;

  void GetDownloadList (const char *url, const char *target_path);
  void StartDownload ();
  void Stop ();

 private:
   typedef std::list<DownloadFileInfo*> FileList;
   typedef std::list<string> PathList;

   void LoadCachedList ();
   void UpdateCachedList ();

   void CheckFileList (FileList&);
   bool CheckMd5 (const string&, const DownloadFileInfo*);
   void RemoveCachedList (PathList&);
   void DeleteLocalFiles ();

   void DownloadResourceFile ();

   void DownloadListCb (const string&, bool);
   void DownloadFileCb (const DownloadFileInfo*, bool);

 private:
  typedef std::list<DownloadUrl*> UrlList;

  void ClearDownloadList ();
  void ClearCachedFileList ();

private:
  std::unique_ptr<HttpFileDownloader> http_;
  FunapiHttpDownloader *caller_;

  State state_;
  std::mutex mutex_;
  time_point<system_clock> check_time_;

  // Url-related.
  string host_url_;
  string target_path_;
  UrlList url_list_;

  FileList cached_list_;
  FileList download_list_;
  PathList delete_file_list_;

  int cur_download_count_ = 0;
  int total_download_count_ = 0;
  uint64 cur_download_size_ = 0;
  uint64 total_download_size_ = 0;
};


FunapiHttpDownloaderImpl::FunapiHttpDownloaderImpl (FunapiHttpDownloader* caller)
  : state_(State::None), caller_(caller) {
  http_ = std::make_unique<HttpFileDownloader>();
}


FunapiHttpDownloaderImpl::~FunapiHttpDownloaderImpl () {
}

bool FunapiHttpDownloaderImpl::IsDownloading () const {
  return state_ == State::Start || state_ == State::Ready || state_ == State::Downloading;
}

void FunapiHttpDownloaderImpl::GetDownloadList (const char *url, const char *target_path) {
  std::unique_lock<std::mutex> lock(mutex_);

  if (caller_->ReadyCallback.empty()) {
    LogError("You must register the ReadyCallback first.");
    return;
  }

  if (IsDownloading()) {
    LogWarning("The resource file is being downloaded, from %s.\
 Please try again after the download is completed.", *FString(url));
    return;
  }

  state_ = State::Start;

  string host_url = url;
  if (host_url.back() != '/')
    host_url.append("/");
  host_url_ = host_url;
  Log("Download url: %s", *FString(host_url_.c_str()));

  target_path_ = target_path;
  if (target_path_.back() != PATH_DELIMITER)
    target_path_.append(1, PATH_DELIMITER);
  target_path_.append(kRootPath);
  target_path_.append(1, PATH_DELIMITER);
  Log("Download path: %s", *FString(target_path_.c_str()));

  cur_download_count_ = 0;
  cur_download_size_ = 0;
  total_download_count_ = 0;
  total_download_size_ = 0;

  LoadCachedList();

  // Requests a list file.
  http_->AddRequest(host_url_, [this](const string& data, bool success) {
                      DownloadListCb(data, success); });
}

void FunapiHttpDownloaderImpl::ClearDownloadList () {
  std::unique_lock<std::mutex> lock(mutex_);
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
}


void FunapiHttpDownloaderImpl::ClearCachedFileList () {
  if (cached_list_.empty())
    return;

  {
    std::unique_lock<std::mutex> lock(mutex_);
    FileList::iterator itr = cached_list_.begin();
    for (; itr != cached_list_.end(); ++itr) {
      delete (*itr);
    }

    cached_list_.clear();
  }
}


void FunapiHttpDownloaderImpl::StartDownload () {
  if (state_ != State::Ready) {
    LogError("You must call GetDownloadList function first.");
    return;
  }

  //std::unique_lock<std::mutex> lock(mutex_);

  if (total_download_count_ > 0) {
    float size;
    string unit;
    if (total_download_size_ < 1024 * 1024) {
      size = total_download_size_ / float(1024);
      unit = "K";
    }
    else {
      size = total_download_size_ / float(1024 * 1024);
      unit = "M";
    }
    size = int(size * pow(10, 3)) / pow(10, 3);
    Log("Ready to download %d files (%s%s)", total_download_count_,
        *FString::SanitizeFloat(size), *FString(unit.c_str()));
  }

  state_ = State::Downloading;
  check_time_ = system_clock::now();

  // Deletes files
  DeleteLocalFiles();

  // Starts download
  DownloadResourceFile();
}


void FunapiHttpDownloaderImpl::Stop () {
  if (state_ == State::Start || state_ == State::Downloading) {
    caller_->FinishCallback(kDownloadFailure);
  }

  ClearDownloadList();
}

void FunapiHttpDownloaderImpl::LoadCachedList () {
  cached_list_.clear();

  string path = target_path_ + kCachedFilename;
  std::ifstream fs(path.c_str(), std::ios::in | std::ios::ate);
  if (!fs.is_open())
    return;

  int length = fs.tellg();
  char *buffer = new char[length + 1];
  fs.seekg(0, std::ios::beg);
  fs.read(buffer, length);
  buffer[length] = '\0';
  fs.close();

  rapidjson::Document json;
  json.Parse<0>(buffer);
  assert(json.IsObject());
  delete[] buffer;

  rapidjson::Value &list = json["list"];
  assert(list.IsArray());

  int count = (int)list.Size();
  for (int i = 0; i < count; ++i) {
    DownloadFileInfo *info = new DownloadFileInfo();
    info->path = list[i]["path"].GetString();
    info->size = list[i]["size"].GetUint();
    info->hash = list[i]["hash"].GetString();
    if (list[i].HasMember("front"))
      info->hash_front = list[i]["front"].GetString();
    else
      info->hash_front = "";

    cached_list_.push_back(info);
  }
  
  Log("Loads cached list : %d", cached_list_.size());
}

void FunapiHttpDownloaderImpl::UpdateCachedList () {
  std::stringstream data;
  int count = cached_list_.size();

  data << "{ \"list\": [ ";
  for (auto info : cached_list_) {
    data << "{ \"path\":\"" << info->path << "\", ";
    data << "\"size\":" << info->size << ", ";
    if (info->hash_front.length() > 0)
      data << "\"front\":\"" << info->hash_front << "\", ";
    data << "\"hash\":\"" << info->hash << "\" }";
    if (--count > 0)
      data << ", ";
  }
  data << " ] }";

  string path = target_path_ + kCachedFilename;
  std::ofstream fs(path.c_str(), std::ios::out | std::ios::trunc);
  if (!fs.is_open()) {
    LogWarning("Failed to update cached list. can't open the file.");
    return;
  }

  string buf = data.str();
  int length = data.tellg();
  fs.write(buf.c_str(), buf.length());
  fs.close();
  
  Log("Updates cached list : %d", cached_list_.size());
}

void FunapiHttpDownloaderImpl::CheckFileList (FileList &list) {
  std::unique_lock<std::mutex> lock(mutex_);
  std::mutex list_lock;

  time_point<system_clock> start, end;
  start = system_clock::now();

  FileList tmp_list(list);
  PathList verify_file_list;
  PathList remove_list;
  std::list<int> rnd_list;
  bool verify_success = true;
  int rnd_index = -1;

  string file_path = target_path_ + kCachedFilename;
  struct stat file_stat;
  stat(file_path.c_str(), &file_stat);

  delete_file_list_.clear();

  // Randomly check list
  if (!cached_list_.empty()) {
    int max_count = cached_list_.size();
    int count = std::min(std::max(1, max_count / 10), 10);
    std::srand(std::time(0));

    while (rnd_list.size() < count) {
      rnd_index = std::rand() % max_count;
      if (std::find(rnd_list.begin(), rnd_list.end(), rnd_index) == rnd_list.end())
        rnd_list.push_back(rnd_index);
    }
    Log("Random check file count is %d", rnd_list.size());

    rnd_index = -1;
    if (!rnd_list.empty()) {
      rnd_index = rnd_list.front();
      rnd_list.pop_front();
    }
  }

  // Checks local files
  try {
    int index = 0;
    for (auto file : cached_list_) {
      DownloadFileInfo* item = nullptr;
      {
        std::unique_lock<std::mutex> lock(list_lock);
        auto it = std::find_if(list.begin(), list.end(), [&file](const DownloadFileInfo* f) {
                                 return file->path == f->path; });
          if (it != list.end())
            item = *it;
      }

      if (item != nullptr) {
        string path = target_path_ + file->path;
        struct stat st;
        stat(path.c_str(), &st);

        if (!std::ifstream(path) || item->size != st.st_size || item->hash != file->hash) {
          std::unique_lock<std::mutex> lock(list_lock);
          remove_list.push_back(file->path);
        }
        else {
          string filename = GetFilename(item->path);

          if (filename.front() == '_' || index == rnd_index ||
#if PLATFORM_WINDOWS || PLATFORM_ANDROID
              st.st_mtime > file_stat.st_mtime) {
#elif PLATFORM_MAC || PLATFORM_IOS
              st.st_mtimespec.tv_sec > file_stat.st_mtimespec.tv_sec) {
#endif
            if (index == rnd_index) {
              rnd_index = -1;
              if (!rnd_list.empty()) {
                rnd_index = rnd_list.front();
                rnd_list.pop_front();
              }
            }

            {
              std::unique_lock<std::mutex> lock(list_lock);
              verify_file_list.push_back(file->path);
            }

            // Checks md5 hash
            auto handle = std::async(std::launch::async, [&, path, item]() {
              bool is_match = CheckMd5(path, item);

              caller_->VerifyCallback(path);

              {
                std::unique_lock<std::mutex> lock(list_lock);
                verify_file_list.remove(item->path);

                if (is_match) {
                  list.remove(item);
                }
                else {
                  remove_list.push_back(item->path);
                  verify_success = false;
                }
              }
            });
          }
          else {
            std::unique_lock<std::mutex> lock(list_lock);
            list.remove(item);
          }
        }
      }
      else {
        std::unique_lock<std::mutex> lock(list_lock);
        remove_list.push_back(file->path);
      }

      ++index;
    }
  }
  catch (std::system_error &e) {
    LogError("Failed to random check files. %s", *FString(e.what()));
    return;
  }

  while (!verify_file_list.empty()) {
    std::this_thread::sleep_for(milliseconds(100));
  }

  RemoveCachedList(remove_list);

  string ret = verify_success ? "succeeded" : "failed";
  Log("Random validation has %s", *FString(ret.c_str()));

  // Checks all local files
  if (!verify_success) {
    try {
      for (auto file : cached_list_) {
        auto it = std::find_if(tmp_list.begin(), tmp_list.end(), [&file](const DownloadFileInfo* f) {
                                 return file->path == f->path; });
        if (it != tmp_list.end()) {
          DownloadFileInfo* item = *it;
          string path = target_path_ + file->path;

          {
            std::unique_lock<std::mutex> lock(list_lock);
            verify_file_list.push_back(file->path);
          }

          // Checks md5 hash
          auto handle = std::async(std::launch::async, [&, path, item]() {
            bool is_match = CheckMd5(path, item);

            caller_->VerifyCallback(path);

            {
              std::unique_lock<std::mutex> lock(list_lock);
              verify_file_list.remove(item->path);

              if (!is_match) {
                remove_list.push_back(item->path);

                it = std::find(list.begin(), list.end(), item);
                if (it == list.end())
                  list.push_back(item);
              }
            }
          });
        }
      }
    }
    catch (std::system_error &e) {
      LogError("Failed to check all files. %s", *FString(e.what()));
      return;
    }

    while (!verify_file_list.empty()) {
      std::this_thread::sleep_for(milliseconds(100));
    }

    RemoveCachedList(remove_list);
  }

  end = system_clock::now();
  auto elapsed = duration_cast<milliseconds>(end - start);
  Log("File check total time - %ss", *FString::SanitizeFloat(elapsed.count() / (float)1000));

  total_download_count_ = list.size();

  for (auto item : list) {
    total_download_size_ += item->size;
  }

  if (total_download_count_ > 0) {
    state_ = State::Ready;
    caller_->ReadyCallback(total_download_count_, total_download_size_);
  }
  else {
    DeleteLocalFiles();
    UpdateCachedList();

    state_ = State::Completed;
    Log("All resources are up to date.");
    caller_->FinishCallback(kDownloadSuccess);
  }
}

bool FunapiHttpDownloaderImpl::CheckMd5 (const string &path, const DownloadFileInfo* info) {
  std::ifstream fs(path.c_str(), std::ios::in | std::ios::binary);
  if (!fs.is_open())
    return false;

  int buf_size = info->size;
  std::unique_ptr<char[]> buffer(new char[buf_size]);
  fs.read(buffer.get(), buf_size);
  fs.close();

  uint8 digest[16];

  if (info->hash_front.length() > 0) {
    FMD5 md5;
    int len = buf_size < kBlockSize ? buf_size : kBlockSize;
    md5.Update((uint8*)buffer.get(), len);
    md5.Final(digest);
    
    string hash = MakeHashString(digest);
    if (hash != info->hash_front)
      return false;
  }

  if (buf_size > 0) {
    FMD5 md5;
    md5.Update((uint8*)buffer.get(), buf_size);
    md5.Final(digest);
  }
  else {
    FMD5 md5;
    md5.Final(digest);
  }

  string hash = MakeHashString(digest);
  return hash == info->hash;
}

void FunapiHttpDownloaderImpl::RemoveCachedList (PathList &remove_list) {
  if (remove_list.empty())
    return;

  cached_list_.remove_if([&remove_list](const DownloadFileInfo* info) {
    return std::find(remove_list.begin(), remove_list.end(), info->path) != remove_list.end();
  });

  for (auto path : remove_list) {
    delete_file_list_.push_back(target_path_ + path);
  }

  remove_list.clear();
}

void FunapiHttpDownloaderImpl::DeleteLocalFiles () {
  if (delete_file_list_.empty())
    return;

  for (string path : delete_file_list_) {
    if (std::ifstream(path)) {
      std::remove(path.c_str());
      Log("Deleted resource file. path: %s", *FString(path.c_str()));
    }
  }

  delete_file_list_.clear();
}

void FunapiHttpDownloaderImpl::DownloadResourceFile () {
  if (download_list_.empty()) {
    UpdateCachedList();

    time_point<system_clock> end = system_clock::now();
    auto elapsed = duration_cast<milliseconds>(end - check_time_);
    Log("File download total time - %ss",
        *FString::SanitizeFloat(elapsed.count() / (float)1000));

    state_ = State::Completed;
    Log("Download completed.");
    caller_->FinishCallback(kDownloadSuccess);
  }
  else {
    const DownloadFileInfo* info = download_list_.front();

    // Checks directory
    string path = GetDirectory(info->path);
    if (!path.empty()) {
      path = target_path_ + path;
      if (access(path.c_str(), F_OK) == -1) {
        if (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
          LogError("Failed to create folder. path: %s", *FString(path.c_str()));
          return;
        }
      }
    }

    string file_path = target_path_ + info->path;
    if (std::ifstream(file_path)) {
      std::remove(file_path.c_str());
    }

    // Requests a resource file
    Log("Download file - %s", *FString(file_path.c_str()));
    string url = host_url_ + info->path;
    http_->AddRequest(url, file_path, [this, info](const string& data, bool success) {
                        DownloadFileCb(info, success);
                      }, [this](const string& path, uint64 received, uint64 total, int percent) {
                        caller_->UpdateCallback(path, received, total, percent);
                      }
    );
  }
}

void FunapiHttpDownloaderImpl::DownloadListCb (const string& data, bool success) {
  Log("DownloadListCb called. Download list %s.", *FString(success ? "succeed" : "failed"));
  std::unique_lock<std::mutex> lock(mutex_);

  if (!success) {
    Stop();
    return;
  }

  bool failed = false;
  //Log("Json data >> %s", *FString(data.c_str()));

  // Parse json
  rapidjson::Document json;
  json.Parse<0>(data.c_str());
  assert(json.IsObject());

  // Redirect url
  if (json.HasMember("url")) {
    string url = json["url"].GetString();
    if (url.back() != '/')
      url.append("/");

    host_url_ = url;
    Log("Redirect download url: %s", *FString(url.c_str()));
  }

  const rapidjson::Value& list = json["data"];
  assert(list.IsArray());
  if (list.Empty()) {
    Log("Invalid list data. List count is 0.");
    assert(false);
    failed = true;
  }
  else {
    download_list_.clear();

    int count = (int)list.Size();
    for (int i = 0; i < count; ++i) {
      DownloadFileInfo *info = new DownloadFileInfo();
      info->path = list[i]["path"].GetString();
      info->size = list[i]["size"].GetUint();
      info->hash = list[i]["md5"].GetString();
      if (list[i].HasMember("md5_front"))
        info->hash_front = list[i]["md5_front"].GetString();
      else
        info->hash_front = "";

      download_list_.push_back(info);
    }

    // Check files
    AsyncEvent::instance().AddEvent(
      [this](){ CheckFileList(download_list_); });
  }

  if (failed)
    Stop();
}

void FunapiHttpDownloaderImpl::DownloadFileCb (const DownloadFileInfo* info, bool success) {
  Log("DownloadFileCb called. Download list %s.", *FString(success ? "succeed" : "failed"));
  std::unique_lock<std::mutex> lock(mutex_);

  if (!success) {
    Stop();
    return;
  }

  ++cur_download_count_;
  cur_download_size_ += info->size;

  DownloadFileInfo* item = const_cast<DownloadFileInfo*>(info);
  download_list_.remove(item);
  cached_list_.push_back(item);

  DownloadResourceFile();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiHttpDownloader implementation.

FunapiHttpDownloader::FunapiHttpDownloader ()
  : impl_(new FunapiHttpDownloaderImpl(this)) {
}

FunapiHttpDownloader::~FunapiHttpDownloader() {
  delete impl_;
}

void FunapiHttpDownloader::GetDownloadList (const char *hostname_or_ip, uint16_t port,
                                            bool https, const char *target_path) {
  string url = FormatString("%s://%s:%d",
                            (https ? "https" : "http"), hostname_or_ip, port);

  impl_->GetDownloadList(url.c_str(), target_path);
}

void FunapiHttpDownloader::GetDownloadList (const char *url, const char *target_path) {
  impl_->GetDownloadList(url, target_path);
}

void FunapiHttpDownloader::StartDownload () {
  impl_->StartDownload();
}


void FunapiHttpDownloader::Stop () {
  impl_->Stop();
}

bool FunapiHttpDownloader::IsDownloading() const {
  return impl_->IsDownloading();
}

}  // namespace fun
