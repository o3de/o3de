/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_EDITORFILEMONITOR_H
#define CRYINCLUDE_EDITOR_EDITORFILEMONITOR_H
#pragma once
#include "Include/IEditorFileMonitor.h"
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
