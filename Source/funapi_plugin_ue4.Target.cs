// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

using UnrealBuildTool;
using System.Collections.Generic;

public class funapi_plugin_ue4Target : TargetRules
{
    public funapi_plugin_ue4Target(TargetInfo Target)
    {
        Type = TargetType.Game;
    }

    //
    // TargetRules interface.
    //

    public override void SetupBinaries(
        TargetInfo Target,
        ref List<UEBuildBinaryConfiguration> OutBuildBinaryConfigurations,
        ref List<string> OutExtraModuleNames
        )
    {
        OutExtraModuleNames.AddRange( new string[] { "funapi_plugin_ue4" } );
    }
}
