// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.IO;

public class funapi_plugin_ue4 : ModuleRules
{
	public funapi_plugin_ue4(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

        var ModulePath = Path.GetDirectoryName(RulesCompiler.GetModuleFilename(this.GetType().Name));
        var ThirdPartyPath = Path.GetFullPath(Path.Combine(ModulePath, "..", "..", "ThirdParty/"));

        // include path
        PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "include"));
        PublicIncludePaths.Add(Path.GetFullPath(Path.Combine(ModulePath, "funapi")));

        Definitions.Add("GOOGLE_PROTOBUF_NO_RTTI=1");

        var LibPath = ThirdPartyPath;

        if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "include", "Mac"));

            LibPath += "lib/Mac";
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcurl.a"));
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcrypto.a"));
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libprotobuf.a"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Win32)
        {
            PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "include", "Windows", "x86"));

            LibPath += "lib/Windows/x86";
            PublicLibraryPaths.Add(LibPath);

            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcurl_a.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libeay32.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libprotobuf.lib"));
            Definitions.Add("CURL_STATICLIB=1");
        }
        else if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "include", "Windows", "x64"));

            LibPath += "lib/Windows/x64";
            PublicLibraryPaths.Add(LibPath);

            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcurl_a.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libeay32.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libprotobuf.lib"));
            Definitions.Add("CURL_STATICLIB=1");
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            // toolchain will filter properly
            PublicIncludePaths.Add(LibPath + "include/Android/ARMv7");
            PublicLibraryPaths.Add(LibPath + "lib/Android/ARMv7");
            //PublicIncludePaths.Add(LibPath + "include/Android/ARM64");
            //PublicLibraryPaths.Add(LibPath + "lib/Android/ARM64");
            //PublicIncludePaths.Add(LibPath + "include/Android/x86");
            //PublicLibraryPaths.Add(LibPath + "lib/Android/x86");
            //PublicIncludePaths.Add(LibPath + "include/Android/x64");
            //PublicLibraryPaths.Add(LibPath + "lib/Android/x64");

            LibPath += "lib/Android/ARMv7";
            //PublicAdditionalLibraries.Add("curl");
            //PublicAdditionalLibraries.Add("crypto");
            //PublicAdditionalLibraries.Add("protobuf");

            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcurl.a"));
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcrypto.a"));
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libprotobuf.a"));

        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "include", "iOS"));

            LibPath += "lib/iOS";
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcurl.4.dylib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcrypto.1.0.0.dylib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libprotobuf.9.dylib"));
        }
    }
}
