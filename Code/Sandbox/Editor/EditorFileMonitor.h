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

#ifndef CRYINCLUDE_EDITOR_EDITORFILEMONITOR_H
#define CRYINCLUDE_EDITOR_EDITORFILEMONITOR_H
#pragma once
#include "Include/IEditorFileMonitor.h"
#include "IFileChangeMonitor.h"
#include "Util/FileChangeMonitor.h"

class CEditorFileMonitor
    : public IEditorFileMonitor
    , public CFileChangeMonitorListener
    , public IEditorNotifyListener
{
public:
    CEditorFileMonitor();
    ~CEditorFileMonitor();

    bool RegisterListener(IFileChangeListener* pListener, const char* filename) override;
    bool RegisterListener(IFileChangeListener* pListener, const char* folderRelativeToGame, const char* ext) override;
    bool UnregisterListener(IFileChangeListener* pListener) override;

    // from CFileChangeMonitorListener
    void OnFileMonitorChange(const SFileChangeInfo& rChange) override;

    // from IEditorNotifyListener
    void OnEditorNotifyEvent(EEditorNotifyEvent ev) override;
private:

    void MonitorDirectories();
    // File Change Monitor stuff
    struct SFileChangeCallback
    {
        IFileChangeListener* pListener;
        QString item;
        QString extension;

        SFileChangeCallback()
            : pListener(NULL)
        {}

        SFileChangeCallback(IFileChangeListener* pListener, const char* item, const char* extension)
            : pListener(pListener)
            , item(item)
            , extension(extension)
        {}
    };

    std::vector<SFileChangeCallback> m_vecFileChangeCallbacks;
};

#endif // CRYINCLUDE_EDITOR_EDITORFILEMONITOR_H
