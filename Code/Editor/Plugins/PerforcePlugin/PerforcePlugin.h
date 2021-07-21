/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_PERFORCEPLUGIN_PERFORCEPLUGIN_H
#define CRYINCLUDE_PERFORCEPLUGIN_PERFORCEPLUGIN_H
#pragma once


#include <Include/IPlugin.h>

class CPerforcePlugin
    : public IPlugin
{
public:
    void Release();
    void ShowAbout();
    const char* GetPluginGUID();
    DWORD GetPluginVersion();
    const char* GetPluginName();
    bool CanExitNow();
    void Serialize(FILE* hFile, bool bIsStoring);
    void ResetContent();
    bool CreateUIElements();
    void OnEditorNotify(EEditorNotifyEvent aEventId);
};

#endif // CRYINCLUDE_PERFORCEPLUGIN_PERFORCEPLUGIN_H
