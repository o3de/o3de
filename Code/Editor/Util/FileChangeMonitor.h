/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_UTIL_FILECHANGEMONITOR_H
#define CRYINCLUDE_EDITOR_UTIL_FILECHANGEMONITOR_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/containers/set.h>

#include <QFileInfoList>
#include <QFileSystemWatcher>

#include <QObject>
#include <QQueue>
#include <QScopedPointer>
#endif

class CFileChangeMonitorListener;

struct SFileChangeInfo
{
    enum EChangeType
    {
        //! error or unknown change type
        eChangeType_Unknown,
        //! the file was created
        eChangeType_Created,
        //! the file was deleted
        eChangeType_Deleted,
        //! the file was modified (size changed,write)
        eChangeType_Modified,
        //! this is the old name of a renamed file
        eChangeType_RenamedOldName,
        //! this is the new name of a renamed file
        eChangeType_RenamedNewName
    };

    SFileChangeInfo()
        : changeType(eChangeType_Unknown)
    {
    }

    bool operator==(const SFileChangeInfo& rhs) const
    {
        return changeType == rhs.changeType && filename == rhs.filename;
    }

    QString filename;
    EChangeType changeType;
};

// Monitors directory for any changed files
class CFileChangeMonitor : public QObject
{
public:
    friend class CEditorFileMonitor;
    typedef AZStd::set<CFileChangeMonitorListener*> TListeners;

protected:
    explicit CFileChangeMonitor(QObject* parent = nullptr);
    ~CFileChangeMonitor();

    void Initialize();
    static void DeleteInstance();

    static CFileChangeMonitor* s_pFileMonitorInstance;

public:

    static CFileChangeMonitor* Instance();

    bool MonitorItem(const QString& sItem);
    void StopMonitor();
    void SetEnabled(bool bEnable);
    bool IsEnabled();
    //! get next modified file, this file will be delete from list after calling this function,
    //! call it until HaveModifiedFiles return true or this function returns false
    void Subscribe(CFileChangeMonitorListener* pListener);
    void Unsubscribe(CFileChangeMonitorListener* pListener);
    bool IsDirectory(const char* pFilename);
    bool IsFile(const char* pFilename);
    bool IsLoggingChanges() const
    {
        return ed_logFileChanges != 0;
    }
    void AddIgnoreFileMask(const char* pMask);
    void RemoveIgnoreFileMask(const char* pMask, int aAfterDelayMsec = 1000);

private:
    void OnDirectoryChange(const QString &path);
    void OnFileChange(const QString &path);
    void NotifyListeners(const QString &path, SFileChangeInfo::EChangeType changeType);

    int ed_logFileChanges;
    QScopedPointer<QFileSystemWatcher> m_watcher;
    TListeners m_listeners;
    QQueue<SFileChangeInfo> m_changes;
    QStringList m_ignoreMasks;
    QHash<QString, QFileInfoList> m_entries;
};

// Used as base class (aka interface) to subscribe for file change events
class CFileChangeMonitorListener
{
public:
    CFileChangeMonitorListener()
        : m_pMonitor(nullptr)
    {
    }

    virtual ~CFileChangeMonitorListener()
    {
        if (m_pMonitor)
        {
            m_pMonitor->Unsubscribe(this);
        }
    }

    virtual void OnFileMonitorChange(const SFileChangeInfo& rChange) = 0;

    void SetMonitor(CFileChangeMonitor* pMonitor)
    {
        m_pMonitor = pMonitor;
    }

private:
    CFileChangeMonitor* m_pMonitor;
};

#endif // CRYINCLUDE_EDITOR_UTIL_FILECHANGEMONITOR_H
