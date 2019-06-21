// Copyright (C) 2019 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#if FUNAPI_UE4
#if WITH_EDITOR

#include "funapi_plugin.h"

#include "curl/curl.h"


#include "Commands/InputChord.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/MessageDialog.h"

#include "funapi_utils.h"
#include "funapi_extension_commands.h"


// Avoid name collisions with the rest of engine code and avoid typing namespace
#define LOCTEXT_NAMESPACE "iFun"

void Ffunapi_Menubar::MakeAndRegistFunapiMenubar()
{
  // Editor 메뉴 UI에서 사용되는 커맨드 등록.
  Ffunapi_extension_commands::Register();

  // 생선된 커맨드에 행동을 부여.
  fun_command_list_ = MakeShareable(new FUICommandList());
  fun_command_list_->MapAction(
    Ffunapi_extension_commands::Get().donwload_root_certificate_,
    FExecuteAction::CreateStatic(&Ffunapi_excute_actions::DownloadRootCertificate),
    FCanExecuteAction());

  // 추가할 Editor 메뉴의 정보를 생성한다.
  TSharedPtr<FExtender> extender = MakeShareable(new FExtender);
  extender->AddMenuBarExtension(
    "Help",
    EExtensionHook::After,
    fun_command_list_,
    FMenuBarExtensionDelegate::CreateRaw(this, &Ffunapi_Menubar::CreateiFunMenuBar));

  // Editor 메뉴 등록
  FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
  LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(extender);
}


void Ffunapi_Menubar::CreateiFunSubMenu(FMenuBuilder& MenuBuilder)
{
  MenuBuilder.AddMenuEntry(Ffunapi_extension_commands::Get().donwload_root_certificate_);
  // 이후로 추가되는 메뉴 기능은 여기서 등록을 한다.
}


void Ffunapi_Menubar::CreateiFunMenuBar(FMenuBarBuilder& MenuBuilder)
{
  MenuBuilder.AddPullDownMenu(
    LOCTEXT("iFun Plugin Menu", "iFun Plugin"),
    LOCTEXT("iFun Plugin Tool tip", ""),
    FNewMenuDelegate::CreateRaw(this, &Ffunapi_Menubar::CreateiFunSubMenu),
    FName(TEXT("iFun Plugin")),
    FName(TEXT("iFun Plugin")));
}


void Ffunapi_Menubar::UnregistFunapiMenubar()
{
  Ffunapi_extension_commands::Unregister();
}


void Ffunapi_extension_commands::RegisterCommands()
{
  UI_COMMAND(donwload_root_certificate_,
             "Download root certificate",       // button name
             "Requires HTTPS protocol",      // tool tip
             EUserInterfaceActionType::Button,
             FInputGesture());

}


void Ffunapi_excute_actions::DownloadRootCertificate()
{
  fun::string url = "https://curl.haxx.se/ca/cacert.pem";
  FString directory_path = FPaths::ProjectSavedDir() + "Certificate/";
  FString file_path = directory_path + "cacert.pem";

  // 인증서를 저장할 디렉토리 확인.
  IPlatformFile& platform_file = FPlatformFileManager::Get().GetPlatformFile();
  if (!platform_file.DirectoryExists(*directory_path))
  {
    if (!platform_file.CreateDirectory(*directory_path))
    {
      fun::DebugUtils::Log("Error: Failed to create a directory to save certificates. path : %s", TCHAR_TO_UTF8(*directory_path));
      FMessageDialog::Open(
        EAppMsgType::Ok,
        LOCTEXT("DownloadCertificateFail", "Failed to download root certificates. Please check the log message"));
      return;
    }
  }

  CURL *curl = curl_easy_init();
  if (!curl)
  {
    fun::DebugUtils::Log("Failed to download root certificates. Error : curl initialize failed");
    FMessageDialog::Open(
      EAppMsgType::Ok,
      LOCTEXT("DownloadCertificateFail", "Failed to download root certificates. Please check the log message"));
    return;
  }

  // 인증서를 저장할 파일 스트림을 생성합니다.
  FILE *fp = fopen(TCHAR_TO_UTF8(*file_path), "wb");
  if (fp == nullptr)
  {
    fun::DebugUtils::Log("Failed to download root certificates. Error : file wirte failed.");
    FMessageDialog::Open(
      EAppMsgType::Ok,
      LOCTEXT("DownloadCertificateFail", "Failed to download root certificates. Please check the log message"));
    return;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

  // Preform the request
  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  fclose(fp);

  // 결과를 확인한다.
  if (res != CURLcode::CURLE_OK)
  {
    fun::DebugUtils::Log("Failed to download root certificates. Error CURLcode : %d", res);
    FMessageDialog::Open(
        EAppMsgType::Ok,
        LOCTEXT("DownloadCertificateFail", "Failed to download root certificates. Please check the log message"));
    return;
  }

  // 성공 메세지 출력.
  fun::DebugUtils::Log("Succeed to download root certificate. Path : %s", TCHAR_TO_UTF8(*file_path));
  FMessageDialog::Open(EAppMsgType::Ok,
                       LOCTEXT("DownloadCertificateSuccess", "Succeed to download root certificate."));


}


#undef LOCTEXT_NAMESPACE

#endif // WITH_EDITOR
#endif //FUNAPI_UE4
