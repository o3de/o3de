/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <native/FileWatcher/FileWatcher.h>
#include <QSet>
#include <QString>
#include <CoreServices/CoreServices.h>

class FileWatcher::PlatformImplementation
{
public:
    FSEventStreamRef m_stream = nullptr;
    CFRunLoopRef m_runLoop = nullptr;
    
    // until a file is removed, the mac FSStream will send
    // all changes done to that file every time the file changes (including
    // when it is deleted).  To avoid double creates, we don't send any more creates
    
    QSet<QString> m_sentCreateAlready;
    FSEventStreamEventId m_lastSeenEventId = 0;
    
    // static member that gets the raw file callback from the OS
    static void FileEventStreamCallback(ConstFSEventStreamRef streamRef, void *clientCallBackInfo, size_t numEvents, void *eventPaths, const FSEventStreamEventFlags eventFlags[], const FSEventStreamEventId eventIds[]);
    
    // non-static friendlier member interface.
    void ConsumeEvents(size_t numEvents, const char **filePaths, const FSEventStreamEventFlags eventFlags[], const FSEventStreamEventId eventIds[]);
    
    FileWatcher* m_watcher = nullptr;
    dispatch_queue_t m_dispatchQueue;
};

