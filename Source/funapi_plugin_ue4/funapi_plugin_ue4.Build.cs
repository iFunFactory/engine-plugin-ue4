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
            PublicIncludePaths.Add(LibPath + "include/Android/ARMv7");

            LibPath += "lib/Android/ARMv7";
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
