/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FFMPEGPlugin.h"
#include "Include/IEditorClassFactory.h"


PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
    if (pInitParam->pluginVersion != SANDBOX_PLUGIN_SYSTEM_VERSION)
    {
        pInitParam->outErrorCode = IPlugin::eError_VersionMismatch;
        return nullptr;
    }

    ModuleInitISystem(GetIEditor()->GetSystem(), "FFMPEGPlugin");
    GetIEditor()->GetSystem()->GetILog()->Log("FFMPEG plugin: CreatePluginInstance");

    // Make sure the ffmpeg command can be executed before registering the command
    if (!CFFMPEGPlugin::RuntimeTest())
    {
        GetIEditor()->GetSystem()->GetILog()->Log("FFMPEG plugin: Failed to execute FFmpeg. Please install FFmpeg.");
    }
    else
    {
        CFFMPEGPlugin::RegisterTheCommand();
    }

    return new CFFMPEGPlugin;
}
