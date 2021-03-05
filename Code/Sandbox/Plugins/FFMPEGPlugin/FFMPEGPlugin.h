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

#pragma once
//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
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
