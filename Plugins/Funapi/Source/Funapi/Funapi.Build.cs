// Copyright (C) 2013-2018 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

using System.IO;
using UnrealBuildTool;

public class Funapi : ModuleRules
{
  // public Funapi(TargetInfo Target) // <= 4.15
  public Funapi(ReadOnlyTargetRules Target) : base(Target) // >= 4.16
  {
    Definitions.Add("WITH_FUNAPI=1");
    Definitions.Add("FUNAPI_UE4=1");

    Definitions.Add("FUNAPI_HAVE_ZLIB=1");
    Definitions.Add("FUNAPI_HAVE_DELAYED_ACK=1");
    Definitions.Add("FUNAPI_HAVE_TCP_TLS=1");
    Definitions.Add("FUNAPI_HAVE_WEBSOCKET=1");

    if (Target.Platform == UnrealTargetPlatform.PS4) {
      Definitions.Add("FUNAPI_HAVE_RPC=0");
    }
    else {
      Definitions.Add("FUNAPI_HAVE_RPC=1");
    }

    if (Target.Platform == UnrealTargetPlatform.Win32 ||
        Target.Platform == UnrealTargetPlatform.Win64) {
      Definitions.Add("FUNAPI_PLATFORM_WINDOWS=1");
      Definitions.Add("FUNAPI_UE4_PLATFORM_WINDOWS=1");
    }

    if (Target.Platform == UnrealTargetPlatform.Linux) {
      Definitions.Add("FUNAPI_HAVE_ZSTD=0");
      Definitions.Add("FUNAPI_HAVE_SODIUM=0");
      Definitions.Add("FUNAPI_HAVE_AES128=0");
    }
    else {
      Definitions.Add("FUNAPI_HAVE_ZSTD=1");
      Definitions.Add("FUNAPI_HAVE_SODIUM=1");
      if (Target.Platform == UnrealTargetPlatform.Android)
      {
        Definitions.Add("FUNAPI_HAVE_AES128=0");
      }
      else
      {
        Definitions.Add("FUNAPI_HAVE_AES128=1");
      }
    }

    PublicIncludePaths.AddRange(
      new string[] {
        // ... add public include paths required here ...
        "Funapi/Pubilc",
        "Funapi/Pubilc/funapi",
        "Funapi/Pubilc/funapi/management",
        "Funapi/Pubilc/funapi/network",
        "Funapi/Pubilc/funapi/service",
        "Funapi/Pubilc/funapi/rpc",
      }
    );

    PrivateIncludePaths.AddRange(
      new string[] {
        "Funapi/Private",
        // ... add other private include paths required here ...
      }
    );

    PublicDependencyModuleNames.AddRange(
      new string[]
      {
        "Core",
        "Engine",
        "zlib",
        // ... add other public dependencies that you statically link with here ...
      }
    );

    PrivateDependencyModuleNames.AddRange(
      new string[]
      {
        // ... add private dependencies that you statically link with here ...
      }
    );

    DynamicallyLoadedModuleNames.AddRange(
      new string[]
      {
        // ... add any modules that your module loads dynamically here ...
      }
    );

    // definitions
    Definitions.Add("RAPIDJSON_HAS_STDSTRING=0");

    // Third party library
    var ThirdPartyPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "ThirdParty/"));

    // include path
    PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "include"));

    // library
    var LibPath = ThirdPartyPath;

    if (Target.Platform == UnrealTargetPlatform.Mac)
    {
      PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "include", "Mac"));

      LibPath += "lib/Mac";
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcurl.a"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcrypto.a"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libssl.a"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libz.a"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libsodium.a"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libwebsockets.a"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libzstd.a"));
    }
    else if (Target.Platform == UnrealTargetPlatform.Win32)
    {
      PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "include", "Windows", "x86"));

      LibPath += "lib/Windows/x86";
      PublicLibraryPaths.Add(LibPath);

      Definitions.Add("CURL_STATICLIB=1");
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcurl_a.lib"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libeay32.lib"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libsodium.lib"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "websockets_static.lib"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libzstd_static.lib"));
    }
    else if (Target.Platform == UnrealTargetPlatform.Win64)
    {
      PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "include", "Windows", "x64"));

      LibPath += "lib/Windows/x64";
      PublicLibraryPaths.Add(LibPath);

      Definitions.Add("CURL_STATICLIB=1");
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcurl_a.lib"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libsodium.lib"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "websockets_static.lib"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libzstd_static.lib"));
    }
    else if (Target.Platform == UnrealTargetPlatform.Android)
    {
      Definitions.Add("FUNAPI_UE4_PLATFORM_ANDROID=1");

      // toolchain will filter properly
      PublicIncludePaths.Add(LibPath + "include/Android/ARMv7");
      PublicLibraryPaths.Add(LibPath + "lib/Android/ARMv7");
      PublicIncludePaths.Add(LibPath + "include/Android/ARM64");
      PublicLibraryPaths.Add(LibPath + "lib/Android/ARM64");

      PublicAdditionalLibraries.Add("sodium");
      PublicAdditionalLibraries.Add("curl");
      PublicAdditionalLibraries.Add("ssl");
      PublicAdditionalLibraries.Add("crypto");
      PublicAdditionalLibraries.Add("websockets");
      PublicAdditionalLibraries.Add("zstd");
    }
    else if (Target.Platform == UnrealTargetPlatform.IOS)
    {
      PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "include", "iOS"));

      LibPath += "lib/iOS";
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcurl.a"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcrypto.a"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libssl.a"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libsodium.a"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libwebsockets.a"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libzstd.a"));
    }
    else if (Target.Platform == UnrealTargetPlatform.PS4)
    {
      Definitions.Add("FUNAPI_UE4_PLATFORM_PS4=1");
      Definitions.Add("RAPIDJSON_HAS_CXX11_RVALUE_REFS=0");

      PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "include", "PS4"));
      LibPath += "lib/PS4";

      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libsodium.a"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libzstd_static.a"));

      PublicDependencyModuleNames.AddRange(new string[] { "WebSockets" });
      AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenSSL", "libWebSockets");
    }
    else if (Target.Platform == UnrealTargetPlatform.Linux)
    {
      Definitions.Add("FUNAPI_UE4_PLATFORM_LINUX=1");
      Definitions.Add("RAPIDJSON_HAS_CXX11_RVALUE_REFS=0");

      AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenSSL", "libWebSockets", "libcurl");
    }
  }
}
