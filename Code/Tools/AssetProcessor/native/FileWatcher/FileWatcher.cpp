/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "FileWatcher.h"
#include <native/assetprocessor.h>

//////////////////////////////////////////////////////////////////////////////
/// FolderWatchRoot
void FolderRootWatch::ProcessNewFileEvent(const QString& file)
{
    FileChangeInfo info;
    info.m_action = FileAction::FileAction_Added;
    info.m_filePath = file;
    const bool invoked = QMetaObject::invokeMethod(m_fileWatcher, "AnyFileChange", Qt::QueuedConnection, Q_ARG(FileChangeInfo, info));
    Q_ASSERT(invoked);
}

void FolderRootWatch::ProcessDeleteFileEvent(const QString& file)
{
    FileChangeInfo info;
    info.m_action = FileAction::FileAction_Removed;
    info.m_filePath = file;
    const bool invoked = QMetaObject::invokeMethod(m_fileWatcher, "AnyFileChange", Qt::QueuedConnection, Q_ARG(FileChangeInfo, info));
    Q_ASSERT(invoked);
}

void FolderRootWatch::ProcessModifyFileEvent(const QString& file)
{
    FileChangeInfo info;
    info.m_action = FileAction::FileAction_Modified;
    info.m_filePath = file;
    const bool invoked = QMetaObject::invokeMethod(m_fileWatcher, "AnyFileChange", Qt::QueuedConnection, Q_ARG(FileChangeInfo, info));
    Q_ASSERT(invoked);
}

//////////////////////////////////////////////////////////////////////////
/// FileWatcher
FileWatcher::FileWatcher()
    : m_nextHandle(0)
{
    qRegisterMetaType<FileChangeInfo>("FileChangeInfo");
}

FileWatcher::~FileWatcher()
{
}

int FileWatcher::AddFolderWatch(FolderWatchBase* pFolderWatch)
{
    if (!pFolderWatch)
    {
        return -1;
    }

    FolderRootWatch* pFolderRootWatch = nullptr;

    //see if this a sub folder of an already watched root
    for (auto rootsIter = m_folderWatchRoots.begin(); !pFolderRootWatch && rootsIter != m_folderWatchRoots.end(); ++rootsIter)
    {
        if (FolderWatchBase::IsSubfolder(pFolderWatch->m_folder, (*rootsIter)->m_root))
        {
            pFolderRootWatch = *rootsIter;
        }
    }

    bool bCreatedNewRoot = false;
    //if its not a sub folder
    if (!pFolderRootWatch)
    {
        //create a new root and start listening for changes
        pFolderRootWatch = new FolderRootWatch(pFolderWatch->m_folder);

        //make sure the folder watcher(s) get deleted before this
        pFolderRootWatch->setParent(this);
        bCreatedNewRoot = true;
    }

    pFolderRootWatch->m_fileWatcher = this;
    QObject::connect(this, &FileWatcher::AnyFileChange, pFolderWatch, &FolderWatchBase::OnAnyFileChange);

    if (bCreatedNewRoot)
    {
        if (m_startedWatching)
        {
            pFolderRootWatch->Start();
        }

        //since we created a new root, see if the new root is a super folder
        //of other roots, if it is then then fold those roots into the new super root
        for (auto rootsIter = m_folderWatchRoots.begin(); rootsIter != m_folderWatchRoots.end(); )
        {
            if (FolderWatchBase::IsSubfolder((*rootsIter)->m_root, pFolderWatch->m_folder))
            {
                //union the sub folder map over to the new root
                pFolderRootWatch->m_subFolderWatchesMap.insert((*rootsIter)->m_subFolderWatchesMap);

                //clear the old root sub folders map so they don't get deleted when we
                //delete the old root as they are now pointed to by the new root
                (*rootsIter)->m_subFolderWatchesMap.clear();

                //delete the empty old root, deleting a root will call Stop()
                //automatically which kills the thread
                delete *rootsIter;

                //remove the old root pointer form the watched list
                rootsIter = m_folderWatchRoots.erase(rootsIter);
            }
            else
            {
                ++rootsIter;
            }
        }

        //add the new root to the watched roots
        m_folderWatchRoots.push_back(pFolderRootWatch);
    }

    //add to the root
    pFolderRootWatch->m_subFolderWatchesMap.insert(m_nextHandle, pFolderWatch);

    m_nextHandle++;

    return m_nextHandle - 1;
}

void FileWatcher::RemoveFolderWatch(int handle)
{
    for (auto rootsIter = m_folderWatchRoots.begin(); rootsIter != m_folderWatchRoots.end(); )
    {
        //find an element by the handle
        auto foundIter = (*rootsIter)->m_subFolderWatchesMap.find(handle);
        if (foundIter != (*rootsIter)->m_subFolderWatchesMap.end())
        {
            //remove the element
            (*rootsIter)->m_subFolderWatchesMap.erase(foundIter);

            //we removed a folder watch, if it's empty then there is no reason to keep watching it.
            if ((*rootsIter)->m_subFolderWatchesMap.empty())
            {
                delete(*rootsIter);
                rootsIter = m_folderWatchRoots.erase(rootsIter);
            }
            else
            {
                ++rootsIter;
            }
        }
        else
        {
            ++rootsIter;
        }
    }
}

void FileWatcher::StartWatching()
{
    if (m_startedWatching)
    {
        AZ_Warning("FileWatcher", false, "StartWatching() called when already watching for file changes.");
        return;
    }

    for (FolderRootWatch* root : m_folderWatchRoots)
    {
        root->Start();
    }

    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "File Change Monitoring started.\n");
    m_startedWatching = true;
}

void FileWatcher::StopWatching()
{
    if (!m_startedWatching)
    {
        AZ_Warning("FileWatcher", false, "StartWatching() called when is not watching for file changes.");
        return;
    }

    for (FolderRootWatch* root : m_folderWatchRoots)
    {
        root->Stop();
    }

    m_startedWatching = false;
}

#include "native/FileWatcher/moc_FileWatcher.cpp"
#include "native/FileWatcher/moc_FileWatcherAPI.cpp"
