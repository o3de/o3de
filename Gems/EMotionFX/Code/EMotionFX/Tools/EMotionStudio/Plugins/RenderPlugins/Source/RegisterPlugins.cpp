/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the EMotion Studio SDK
#include "RenderPluginsConfig.h"
#include "../../../EMStudioSDK/Source/EMStudioConfig.h"
#include "../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../EMStudioSDK/Source/PluginManager.h"

// include the plugin interfaces
#include "OpenGLRender/OpenGLRenderPlugin.h"

extern "C"
{
// the main register function which is executed when loading the plugin library
// we register our plugins to the plugin manager here
RENDERPLUGINS_API void MCORE_CDECL RegisterPlugins()
{
    // get the plugin manager
    EMStudio::PluginManager* pluginManager = EMStudio::GetPluginManager();

    // register the plugins
    pluginManager->RegisterPlugin(new EMStudio::OpenGLRenderPlugin());
}
}
