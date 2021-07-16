/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once
////////////////////////////////////////////////////////////////////////////
// -------------------------------------------------------------------------
//  File Name        : FFMPEGPlugin.h
//  Author           : Jaewon Jung
//  Time of creation : 8/10/2011   16:33
//  Compilers        : VS2008
//  Description      :
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////
#ifndef CRYINCLUDE_FFMPEGPLUGIN_FFMPEGPLUGIN_H
#define CRYINCLUDE_FFMPEGPLUGIN_FFMPEGPLUGIN_H
#include <Include/IPlugin.h>

#define COMMAND_MODULE "plugin"
#define COMMAND_NAME "ffmpeg_encode"

class CFFMPEGPlugin
    : public IPlugin
{
public:
    void Release();
    void ShowAbout();
    const char* GetPluginGUID();
    DWORD GetPluginVersion();
    const char* GetPluginName();
    bool CanExitNow();
    static QString GetFFMPEGExectablePath();
    static bool RuntimeTest();
    static void RegisterTheCommand();
    void OnEditorNotify(EEditorNotifyEvent aEventId);
};
#endif // CRYINCLUDE_FFMPEGPLUGIN_FFMPEGPLUGIN_H
