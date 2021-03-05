/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
