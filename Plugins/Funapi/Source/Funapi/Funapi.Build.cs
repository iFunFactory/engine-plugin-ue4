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
    PublicDefinitions.Add("WITH_FUNAPI=1");
    PublicDefinitions.Add("FUNAPI_UE4=1");

    PublicDefinitions.Add("FUNAPI_HAVE_ZLIB=1");
    PublicDefinitions.Add("FUNAPI_HAVE_DELAYED_ACK=1");
    PublicDefinitions.Add("FUNAPI_HAVE_TCP_TLS=1");
    PublicDefinitions.Add("FUNAPI_HAVE_WEBSOCKET=1");

    if (Target.Platform == UnrealTargetPlatform.PS4) {
      PublicDefinitions.Add("FUNAPI_HAVE_RPC=0");
    }
    else {
      PublicDefinitions.Add("FUNAPI_HAVE_RPC=1");
    }

    if (Target.Platform == UnrealTargetPlatform.Win32 ||
        Target.Platform == UnrealTargetPlatform.Win64) {
      PublicDefinitions.Add("FUNAPI_PLATFORM_WINDOWS=1");
      PublicDefinitions.Add("FUNAPI_UE4_PLATFORM_WINDOWS=1");
    }

    if (Target.Platform == UnrealTargetPlatform.Linux) {
      PublicDefinitions.Add("FUNAPI_HAVE_ZSTD=0");
      PublicDefinitions.Add("FUNAPI_HAVE_SODIUM=0");
      PublicDefinitions.Add("FUNAPI_HAVE_AES128=0");
    }
    else {
      PublicDefinitions.Add("FUNAPI_HAVE_ZSTD=1");
      PublicDefinitions.Add("FUNAPI_HAVE_SODIUM=1");
      if (Target.Platform == UnrealTargetPlatform.Android)
      {
        PublicDefinitions.Add("FUNAPI_HAVE_AES128=0");
      }
      else
      {
        PublicDefinitions.Add("FUNAPI_HAVE_AES128=1");
      }
    }

    PublicIncludePaths.AddRange(
      new string[] {
        // NOTE(sungjin): Path를 문자열로 넘겨주지 않고
        // Path.Combine(ModuleDirectory,"Path") 을 사용한다.
        // 엔진이 플러그인의 Public, Private 폴더를 자동으로 추가해 주지만
        // 잘못된 경로를 추가하는 현상이 있어
        // Path.Combine 함수를 통해 명시적으로 표현하도록 하여 우회한다.
        Path.Combine(ModuleDirectory, "Public"),
        Path.Combine(ModuleDirectory, "Public/funapi"),
        Path.Combine(ModuleDirectory, "Public/funapi/management"),
        Path.Combine(ModuleDirectory, "Public/funapi/network"),
        Path.Combine(ModuleDirectory, "Public/funapi/service"),
        //... add public include paths required here...
      }
    );

    PrivateIncludePaths.AddRange(
      new string[] {
        //ModuleDirectory = real dicractory path where Funapi.Build.cs
        Path.Combine(ModuleDirectory, "Private"),
        //... add other private include paths required here ...
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
    PublicDefinitions.Add("RAPIDJSON_HAS_STDSTRING=0");
    PublicDefinitions.Add("RAPIDJSON_HAS_CXX11_RVALUE_REFS=0");
    // definitions for zlib
    PublicDefinitions.Add("_LARGEFILE64_SOURCE=0");
    PublicDefinitions.Add("_FILE_OFFSET_BITS=0");

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

      PublicDefinitions.Add("CURL_STATICLIB=1");
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

      PublicDefinitions.Add("CURL_STATICLIB=1");
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcurl_a.lib"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libsodium.lib"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "websockets_static.lib"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libzstd_static.lib"));
    }
    else if (Target.Platform == UnrealTargetPlatform.Android)
    {
      PublicDefinitions.Add("FUNAPI_UE4_PLATFORM_ANDROID=1");

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
      PublicDefinitions.Add("WITH_HOT_RELOAD=0");

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
      PublicDefinitions.Add("FUNAPI_UE4_PLATFORM_PS4=1");

      PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "include", "PS4"));
      LibPath += "lib/PS4";

      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libsodium.a"));
      PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libzstd_static.a"));

      PublicDependencyModuleNames.AddRange(new string[] { "WebSockets" });
      AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenSSL", "libWebSockets");
    }
    else if (Target.Platform == UnrealTargetPlatform.Linux)
    {
      PublicDefinitions.Add("FUNAPI_UE4_PLATFORM_LINUX=1");

      AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenSSL", "libWebSockets", "libcurl");
    }
  }
}
