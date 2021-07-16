/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <native/FileWatcher/FileWatcher.h>

#include <native/utilities/BatchApplicationManager.h>

#include <AzCore/Debug/Trace.h>
#include <CoreServices/CoreServices.h>

void FileEventStreamCallback(ConstFSEventStreamRef streamRef, void *clientCallBackInfo, size_t numEvents, void *eventPaths, const FSEventStreamEventFlags eventFlags[], const FSEventStreamEventId eventIds[]);

struct FolderRootWatch::PlatformImplementation
{
    PlatformImplementation() : m_stream(nullptr), m_runLoop(nullptr) { }

    FSEventStreamRef m_stream;
    CFRunLoopRef m_runLoop;
    QString m_renameFileDirectory;
};

//////////////////////////////////////////////////////////////////////////////
/// FolderWatchRoot
FolderRootWatch::FolderRootWatch(const QString rootFolder)
    : m_root(rootFolder)
    , m_shutdownThreadSignal(false)
    , m_fileWatcher(nullptr)
    , m_platformImpl(new PlatformImplementation())
{
}

FolderRootWatch::~FolderRootWatch()
{
    // Destructor is required in here since this file contains the definition of struct PlatformImplementation
    Stop();

    delete m_platformImpl;
}

bool FolderRootWatch::Start()
{
    m_shutdownThreadSignal = false;

    CFStringRef rootPath = CFStringCreateWithCString(kCFAllocatorDefault, m_root.toStdString().data(), kCFStringEncodingMacRoman);
    CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)&rootPath, 1, NULL);
    
    // The larger this number, the larger the delay between the kernel knowing a file changed
    // and us actually consuming the event.  It is very important for asset processor to deal with
    // file changes as fast as possible, since we use file "fencing" to control network access
    // For example, if someone asks (over the network) "does file xyz exist?" we actually put a random file on disk
    // and only answer their query when we see that file appear on our file monitor, so that we know all other
    // file creations/modifications/deletions have been seen before we answer their question.
    // as such, having a slow response time here can cause a dramatic slowdown for all other operations.
    CFAbsoluteTime timeBetweenKernelUpdateAndNotification = 0.001;

    // Set ourselves as the value for the context info field so that in the callback
    // we get passed into it and the callback can call our public API to handle
    // the file change events
    FSEventStreamContext streamContext;
    ::memset(&streamContext, 0, sizeof(streamContext));
    streamContext.info = this;

    m_platformImpl->m_stream = FSEventStreamCreate(NULL,
                                 FileEventStreamCallback,
                                 &streamContext,
                                 pathsToWatch,
                                 kFSEventStreamEventIdSinceNow,
                                 timeBetweenKernelUpdateAndNotification,
                                 kFSEventStreamCreateFlagFileEvents);

    AZ_Error("FileWatcher", (m_platformImpl->m_stream != nullptr), "FSEventStreamCreate returned a nullptr. No file events will be reported for %s", m_root.toStdString().c_str());

    m_thread = std::thread(std::bind(&FolderRootWatch::WatchFolderLoop, this));

    CFRelease(pathsToWatch);
    CFRelease(rootPath);

    return (m_platformImpl->m_stream != nullptr);
}

void FolderRootWatch::Stop()
{
    m_shutdownThreadSignal = true;

    if (m_thread.joinable())
    {
        m_thread.join(); // wait for the thread to finish
        m_thread = std::thread(); //destroy
    }

    FSEventStreamStop(m_platformImpl->m_stream);
    FSEventStreamInvalidate(m_platformImpl->m_stream);
    FSEventStreamRelease(m_platformImpl->m_stream);
}

void FolderRootWatch::WatchFolderLoop()
{
    // Use a half second timeout interval so that we can check if
    // m_shutdownThreadSignal has been changed while we were running the RunLoop
    static const CFTimeInterval secondsToProcess = 0.5;

    m_platformImpl->m_runLoop = CFRunLoopGetCurrent();
    FSEventStreamScheduleWithRunLoop(m_platformImpl->m_stream, m_platformImpl->m_runLoop, kCFRunLoopDefaultMode);
    FSEventStreamStart(m_platformImpl->m_stream);

    const bool returnAfterFirstEventHandled = false;

    while (!m_shutdownThreadSignal)
    {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, secondsToProcess, returnAfterFirstEventHandled);
    }
}

void FileEventStreamCallback(ConstFSEventStreamRef streamRef, void *clientCallBackInfo, size_t numEvents, void *eventPaths, const FSEventStreamEventFlags eventFlags[], const FSEventStreamEventId eventIds[])
{
    FolderRootWatch* watcher = reinterpret_cast<FolderRootWatch*>(clientCallBackInfo);

    const char** filePaths = reinterpret_cast<const char**>(eventPaths);

    for (int i = 0; i < numEvents; ++i)
    {
        QFileInfo fileInfo(QDir::cleanPath(filePaths[i]));
        QString fileAndPath = fileInfo.absoluteFilePath();

        if (!fileInfo.isHidden())
        {
            // Some events will be aggreated into one so it is possible we will get
            // multiple event flags set for a single file (create/modify as an example)
            // so check for all of them
            if (eventFlags[i] & kFSEventStreamEventFlagItemCreated)
            {
                watcher->ProcessNewFileEvent(fileAndPath);
            }

            if (eventFlags[i] & kFSEventStreamEventFlagItemModified)
            {
                watcher->ProcessModifyFileEvent(fileAndPath);
            }

            if (eventFlags[i] & kFSEventStreamEventFlagItemRemoved)
            {
                watcher->ProcessDeleteFileEvent(fileAndPath);
            }

            if (eventFlags[i] & kFSEventStreamEventFlagItemRenamed)
            {
                if (fileInfo.exists())
                {
                    watcher->ProcessNewFileEvent(fileAndPath);

                    // macOS does not send out an event for the directory being
                    // modified when a file has been renamed but the FileWatcher
                    // API expects it so send out the modification event ourselves.
                    watcher->ProcessModifyFileEvent(fileInfo.absolutePath());
                }
                else
                {
                    watcher->ProcessDeleteFileEvent(fileAndPath);

                    // macOS does not send out an event for the directory being
                    // modified when a file has been renamed but the FileWatcher
                    // API expects it so send out the modification event ourselves.
                    watcher->ProcessModifyFileEvent(fileInfo.absolutePath());
                }
            }
        }
    }
}
