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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : To add plug-in to the Editor create a new DLL with class implementation derived from IPlugin


#ifndef CRYINCLUDE_EDITOR_INCLUDE_IPLUGIN_H
#define CRYINCLUDE_EDITOR_INCLUDE_IPLUGIN_H
#pragma once

#include <IEditor.h>

// forbid plugins from loading across debug and release:

#define SANDBOX_PLUGIN_SYSTEM_BASE_VERSION 1

#if defined(_DEBUG)
    #define SANDBOX_PLUGIN_SYSTEM_VERSION (100000 + SANDBOX_PLUGIN_SYSTEM_BASE_VERSION)
#else
    #define SANDBOX_PLUGIN_SYSTEM_VERSION SANDBOX_PLUGIN_SYSTEM_BASE_VERSION
#endif

// Interface for instantiating the plugin for the editor
struct IPlugin
{
    enum EError
    {
        eError_None = 0,
        eError_VersionMismatch = 1
    };

    virtual ~IPlugin() = default;
    
    // Releases plugin.
    virtual void Release() = 0;
    //! Show a modal about dialog / message box for the plugin
    virtual void ShowAbout() = 0;
    //! Return the GUID of the plugin
    virtual const char* GetPluginGUID() = 0;
    virtual DWORD GetPluginVersion() = 0;
    //! Return the human readable name of the plugin
    virtual const char* GetPluginName() = 0;
    //! Asks if the plugin can exit now. This might involve asking the user if he wants to save
    //! data. The plugin is only supposed to ask for unsaved data which is not serialize into
    //! the editor project file. When data is modified which is saved into the project file, the
    //! plugin should call IEditor::SetDataModified() to make the editor ask
    virtual bool CanExitNow() = 0;
    //! this method is called when there is an event triggered inside the editor
    virtual void OnEditorNotify(EEditorNotifyEvent aEventId) = 0;
};

// Initialization structure
struct PLUGIN_INIT_PARAM
{
    IEditor* pIEditorInterface;
    int pluginVersion;
    IPlugin::EError outErrorCode;
};

// Plugin Settings structure
struct SPluginSettings
{
    // note: the pluginVersion in PLUGIN_INIT_PARAM denotes the version of the plugin manager
    // whereas this denotes the version of the individual plugin.
    // future: manage plugin versions.
    int pluginVersion;
    bool autoLoad;
};

// Factory API
extern "C"
{
PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam);
PLUGIN_API void QueryPluginSettings(SPluginSettings& settings);
}
#endif // CRYINCLUDE_EDITOR_INCLUDE_IPLUGIN_H
