// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.IO;

public class funapi_plugin_ue4 : ModuleRules
{
	public funapi_plugin_ue4(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

        // Funapi path
        PublicIncludePaths.Add(Path.GetFullPath(Path.Combine(ModulePath, "funapi")));

        PublicIncludePaths.Add(ThirdPartyPath);
        Definitions.Add("GOOGLE_PROTOBUF_NO_RTTI=1");

        string LibPath = ThirdPartyPath;

        if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "include"));

            LibPath += "lib/";
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcurl.4.dylib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcrypto.a"));
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libprotobuf.a"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            // toolchain will filter properly
            PublicIncludePaths.Add(LibPath + "include/Android/ARMv7");
            PublicLibraryPaths.Add(LibPath + "lib/Android/ARMv7");
            PublicIncludePaths.Add(LibPath + "include/Android/ARM64");
            PublicLibraryPaths.Add(LibPath + "lib/Android/ARM64");
            PublicIncludePaths.Add(LibPath + "include/Android/x86");
            PublicLibraryPaths.Add(LibPath + "lib/Android/x86");
            PublicIncludePaths.Add(LibPath + "include/Android/x64");
            PublicLibraryPaths.Add(LibPath + "lib/Android/x64");

            //PublicAdditionalLibraries.Add("curl");
            //PublicAdditionalLibraries.Add("crypto");
            //PublicAdditionalLibraries.Add("protobuf");

            LibPath += "lib/Android/ARMv7/";
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcurl.a"));
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcrypto.a"));
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libprotobuf.a"));

        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "include"));

            LibPath += "lib/";
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcurl.4.dylib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcrypto.1.0.0.dylib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libprotobuf.9.dylib"));
        }
        //Target.Platform == UnrealTargetPlatform.Win32
        //Target.Platform == UnrealTargetPlatform.Win64
    }

    private string ModulePath
    {
        get { return Path.GetDirectoryName(RulesCompiler.GetModuleFilename(this.GetType().Name)); }
    }

    private string ThirdPartyPath
    {
        get { return Path.GetFullPath(Path.Combine(ModulePath, "../../ThirdParty/")); }
    }
}
