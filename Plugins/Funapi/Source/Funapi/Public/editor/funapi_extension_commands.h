// Copyright (C) 2019 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#pragma once

#if FUNAPI_UE4
#if WITH_EDITOR

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

#include "LevelEditor.h"
#include "Framework/Commands/UIAction.h"

class Ffunapi_Menubar {
 public:
  void MakeAndRegistFunapiMenubar();
  void UnregistFunapiMenubar();

 private:
  void CreateiFunMenuBar(FMenuBarBuilder& MenuBuilder);
  void CreateiFunSubMenu(FMenuBuilder& MenuBuilder);

 private:
  TSharedPtr<class FUICommandList> fun_command_list_;
};

class Ffunapi_extension_commands : public TCommands<Ffunapi_extension_commands>
{
 public:
  Ffunapi_extension_commands() : TCommands<Ffunapi_extension_commands>(
      TEXT("FunapiExtensionCommands"),      // The name of the context
      NSLOCTEXT("FunapiExtensionCommands",
                "FunapiExtensionCommands",
                "FunapiExtensionCommands"), // The localized description of the context
      NAME_None,                         // Optional parent context.
      TEXT("Default"))                   // The style
  {
  }

  virtual void RegisterCommands() override;

 public:
   TSharedPtr<FUICommandInfo> donwload_root_certificate_;
};


class Ffunapi_excute_actions
{
 public:
  static void DownloadRootCertificate();
};


#endif // WITH_EDITOR
#endif // FUNAPI_UE4