/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "FileChangeMonitor.h"

// Qt
#include <QDateTime>
#include <QTimer>


CFileChangeMonitor* CFileChangeMonitor::s_pFileMonitorInstance = nullptr;

//////////////////////////////////////////////////////////////////////////
CFileChangeMonitor::CFileChangeMonitor(QObject* parent)
    : QObject(parent)
{
    ed_logFileChanges = 0;
}

//////////////////////////////////////////////////////////////////////////
CFileChangeMonitor::~CFileChangeMonitor()
{
    for (TListeners::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
    {
        CFileChangeMonitorListener* pListener = *it;

        if (pListener)
        {
            pListener->SetMonitor(nullptr);
        }
    }

    // Send to thread a kill event.
    StopMonitor();
}

CFileChangeMonitor* CFileChangeMonitor::Instance()
{
    if (!s_pFileMonitorInstance)
    {
        s_pFileMonitorInstance = new CFileChangeMonitor();
        s_pFileMonitorInstance->Initialize();
    }

    return s_pFileMonitorInstance;
}

void CFileChangeMonitor::DeleteInstance()
{
    SAFE_DELETE(s_pFileMonitorInstance);
}

void CFileChangeMonitor::Initialize()
{
    REGISTER_CVAR(ed_logFileChanges, 0, VF_NULL, "If its 1, then enable the logging of file monitor file changes");

    m_watcher.reset(new QFileSystemWatcher);
    connect(m_watcher.data(), &QFileSystemWatcher::fileChanged,
            this, &CFileChangeMonitor::OnFileChange);
    connect(m_watcher.data(), &QFileSystemWatcher::directoryChanged,
            this, &CFileChangeMonitor::OnDirectoryChange);

    AddIgnoreFileMask("*$tmp*");
}

bool CFileChangeMonitor::IsDirectory(const char* sFileName)
{
    QFileInfo finfo(sFileName);
    return finfo.isDir();
}

//////////////////////////////////////////////////////////////////////////
bool CFileChangeMonitor::IsFile(const char* sFileName)
{
    QFileInfo finfo(sFileName);
    return finfo.isFile();
}

void CFileChangeMonitor::AddIgnoreFileMask(const char* pMask)
{
    Log("Adding '%s' to ignore masks for changed files.", pMask);
    m_ignoreMasks.append(QString::fromLatin1(pMask));
}

void CFileChangeMonitor::RemoveIgnoreFileMask(const char* pMask, int aAfterDelayMsec)
{
    QTimer::singleShot(aAfterDelayMsec, [=]() {
        m_ignoreMasks.removeAll(QString::fromLatin1(pMask));
    });
}

//////////////////////////////////////////////////////////////////////////
bool CFileChangeMonitor::MonitorItem(const QString& sItem)
{
    QFileInfo finfo(sItem);

    if (finfo.isDir())
    {
        QDir dir(sItem);
        m_entries.insert(sItem, std::move(dir.entryInfoList(QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot)));
    }

    return m_watcher->addPath(sItem);
}

void CFileChangeMonitor::StopMonitor()
{
    if (m_watcher)
    {
        disconnect(m_watcher.data(), &QFileSystemWatcher::fileChanged,
                   this, &CFileChangeMonitor::OnFileChange);
        disconnect(m_watcher.data(), &QFileSystemWatcher::directoryChanged,
                   this, &CFileChangeMonitor::OnDirectoryChange);
    }
}

void CFileChangeMonitor::SetEnabled(bool bEnable)
{
    m_watcher->blockSignals(!bEnable);
}

bool CFileChangeMonitor::IsEnabled()
{
    return !m_watcher->signalsBlocked();
}

void CFileChangeMonitor::Subscribe(CFileChangeMonitorListener* pListener)
{
    assert(pListener);
    pListener->SetMonitor(this);
    m_listeners.insert(pListener);
}

void CFileChangeMonitor::Unsubscribe(CFileChangeMonitorListener* pListener)
{
    assert(pListener);
    m_listeners.erase(pListener);
    pListener->SetMonitor(nullptr);
}

void CFileChangeMonitor::OnDirectoryChange(const QString &path)
{
    QDir dir(path);

    auto entries = dir.entryInfoList(QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot);
    const auto prev = m_entries.value(path);

    for (const auto &fi : prev)
    {
        int eindex = entries.indexOf(fi);
        if (eindex >= 0)
        {
            if (fi.lastModified() != entries.at(eindex).lastModified())
            {
                NotifyListeners(fi.canonicalFilePath(), SFileChangeInfo::eChangeType_Modified);
            }
        }
        else
        {
            NotifyListeners(fi.canonicalFilePath(), SFileChangeInfo::eChangeType_Deleted);
        }
    }

    for (const auto &fi : entries)
    {
        if (!prev.contains(fi))
        {
            NotifyListeners(fi.canonicalFilePath(), SFileChangeInfo::eChangeType_Created);
        }
    }

    m_entries.insert(path, std::move(entries));

    NotifyListeners(path, SFileChangeInfo::eChangeType_Modified);
}

void CFileChangeMonitor::OnFileChange(const QString &path)
{
    QFileInfo finfo(path);
    NotifyListeners(path, finfo.exists() ? SFileChangeInfo::eChangeType_Modified : SFileChangeInfo::eChangeType_Deleted);
}

void CFileChangeMonitor::NotifyListeners(const QString &path, SFileChangeInfo::EChangeType changeType)
{
    for (const auto &glob : m_ignoreMasks)
    {
        QRegExp exp(glob, Qt::CaseInsensitive, QRegExp::Wildcard);
        if (path.contains(exp))
        {
            return; // mask matches, ignore event
        }
    }

    SFileChangeInfo change;
    change.filename = path;
    change.changeType = changeType;

    for (auto it = m_listeners.begin(); it != m_listeners.end(); ++it)
    {
        CFileChangeMonitorListener* pListener = *it;

        if (pListener)
        {
            pListener->OnFileMonitorChange(change);
        }
    }
}
