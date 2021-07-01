/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "FFMPEGPlugin_precompiled.h"
#include "FFMPEGPlugin.h"
#include "Include/IEditorClassFactory.h"


PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
    if (pInitParam->pluginVersion != SANDBOX_PLUGIN_SYSTEM_VERSION)
    {
        pInitParam->outErrorCode = IPlugin::eError_VersionMismatch;
        return 0;
    }

    ModuleInitISystem(GetIEditor()->GetSystem(), "FFMPEGPlugin");
    GetIEditor()->GetSystem()->GetILog()->Log("FFMPEG plugin: CreatePluginInstance");

    // Make sure the ffmpeg command can be executed before registering the command
    if (!CFFMPEGPlugin::RuntimeTest())
    {
        AZStd::string msg =
            "FFMPEG plugin: Failed to execute FFmepg. Please run Setup Assistant, "
            "go to the 'Optional software' section of the 'Install software' tab, "
            "and make sure the FFmpeg executable is correctly configured.";
        GetIEditor()->GetSystem()->GetILog()->Log(msg.c_str());
    }
    else
    {
        CFFMPEGPlugin::RegisterTheCommand();
    }

    return new CFFMPEGPlugin;
}
