/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <native/FileWatcher/FileWatcher.h>
#include <native/FileWatcher/FileWatcher_platform.h>

#include <native/utilities/BatchApplicationManager.h>

#include <AzCore/Debug/Trace.h>

// set this to 1 if you find yourself debugging this code and want to see the debug spam:
#define ALLOW_FILEWATCHER_DEBUG_SPAM 0

#if ALLOW_FILEWATCHER_DEBUG_SPAM
#define DEBUG_FILEWATCHER(...) printf(__VA_ARGS__)
#else
#define DEBUG_FILEWATCHER(...)
#endif

bool FileWatcher::PlatformStart()
{
    // by the time this function exits, we must already have established the watch
    // and no events may be dropped
    
    CFMutableArrayRef pathsToWatch = CFArrayCreateMutable(nullptr, this->m_folderWatchRoots.size(), nullptr);
    for (const auto& root : this->m_folderWatchRoots)
    {
        CFArrayAppendValue(pathsToWatch, root.m_directory.toCFString());
    }
    
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
    FSEventStreamContext streamContext{
        /*.version =*/ 0,
        /*.info =*/ m_platformImpl.get(),
    };

    m_platformImpl->m_watcher = this;
    m_platformImpl->m_stream = FSEventStreamCreate(nullptr,
                                 FileWatcher::PlatformImplementation::FileEventStreamCallback,
                                 &streamContext,
                                 pathsToWatch,
                                 kFSEventStreamEventIdSinceNow,
                                 timeBetweenKernelUpdateAndNotification,
                                 kFSEventStreamCreateFlagFileEvents);

    AZ_Error("FileWatcher", (m_platformImpl->m_stream != nullptr), "FSEventStreamCreate returned a nullptr. No file events will be reported.");

    m_platformImpl->m_dispatchQueue = dispatch_queue_create("EventStreamQueue", DISPATCH_QUEUE_CONCURRENT);
    
    const CFIndex pathCount = CFArrayGetCount(pathsToWatch);
    for(CFIndex i = 0; i < pathCount; ++i)
    {
        CFRelease(CFArrayGetValueAtIndex(pathsToWatch, i));
    }
    CFRelease(pathsToWatch);

    return m_platformImpl->m_stream != nullptr;
    
    DEBUG_FILEWATCHER("Started watching for file events\n");
}

void FileWatcher::PlatformStop()
{
    DEBUG_FILEWATCHER("Stopped watching for file events\n");
    if (m_thread.joinable())
    {
        m_thread.join(); // wait for the thread to finish
    }

    FSEventStreamStop(m_platformImpl->m_stream);
    FSEventStreamInvalidate(m_platformImpl->m_stream);
    FSEventStreamRelease(m_platformImpl->m_stream);
}

void FileWatcher::WatchFolderLoop()
{
    DEBUG_FILEWATCHER("Watch loop entry\n");
    // Use a half second timeout interval so that we can check if
    // m_shutdownThreadSignal has been changed while we were running the RunLoop
    static const CFTimeInterval secondsToProcess = 0.5;

    m_platformImpl->m_runLoop = CFRunLoopGetCurrent();
    FSEventStreamSetDispatchQueue(m_platformImpl->m_stream, m_platformImpl->m_dispatchQueue);
    FSEventStreamStart(m_platformImpl->m_stream);

    const bool returnAfterFirstEventHandled = false;

    DEBUG_FILEWATCHER("Watch loop begins\n");
    m_startedSignal = true; // signal that we are no longer going to drop any events.``
    while (!m_shutdownThreadSignal)
    {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, secondsToProcess, returnAfterFirstEventHandled);
    }
}

void FileWatcher::PlatformImplementation::FileEventStreamCallback([[maybe_unused]] ConstFSEventStreamRef streamRef, void *clientCallBackInfo, size_t numEvents, void *eventPaths, const FSEventStreamEventFlags eventFlags[], const FSEventStreamEventId eventIds[])
{
    auto* platformImpl = reinterpret_cast<FileWatcher::PlatformImplementation*>(clientCallBackInfo);
    const char** filePaths = reinterpret_cast<const char**>(eventPaths);
    platformImpl->ConsumeEvents(numEvents, filePaths, eventFlags, eventIds);
}

void FileWatcher::PlatformImplementation::ConsumeEvents(size_t numEvents, const char **filePaths, const FSEventStreamEventFlags eventFlags[], const FSEventStreamEventId eventIds[])
{
    for (int i = 0; i < numEvents; ++i)
    {
        const QFileInfo fileInfo(QDir::cleanPath(filePaths[i]));
        const QString fileAndPath = fileInfo.absoluteFilePath();
        
        // avoid repeats
        if (m_lastSeenEventId >= eventIds[i])
        {
            DEBUG_FILEWATCHER("File monitor: eventId %" PRIu64 " was repeated, ignoring\n", eventIds[i]);
            continue;
        }

        m_lastSeenEventId = eventIds[i];
        
        DEBUG_FILEWATCHER("File monitor: %s eventflags %x eventId %" PRIu64 "\n", fileAndPath.toUtf8().constData(), eventFlags[i], eventIds[i]);
        if (!fileInfo.isHidden())
        {
            // Some events will be aggregated into one so it is possible we will get
            // multiple event flags set for a single file (create/modify/delete all in one as an example)
            // so check for all of them!
            
            // one tricky caveat is that deletion will usually include created (even if the file
            // was already created).  So you can expect to get a 'create' for every delete.
            if (eventFlags[i] & kFSEventStreamEventFlagItemCreated)
            {
                if (!m_sentCreateAlready.contains(fileAndPath))
                {
                    DEBUG_FILEWATCHER("    - sending rawFileAdded\n");
                    m_watcher->rawFileAdded(fileAndPath, {});
                    m_sentCreateAlready.insert(fileAndPath);
                }
                
            }

            if (eventFlags[i] & kFSEventStreamEventFlagItemModified)
            {
                DEBUG_FILEWATCHER("    - sending rawFileModified\n");
                m_watcher->rawFileModified(fileAndPath, {});
            }

            if (eventFlags[i] & kFSEventStreamEventFlagItemRemoved)
            {
                DEBUG_FILEWATCHER("    - sending rawFileRemoved\n");
                m_watcher->rawFileRemoved(fileAndPath, {});
                m_sentCreateAlready.remove(fileAndPath);
            }

            if (eventFlags[i] & kFSEventStreamEventFlagItemRenamed)
            {
                if (fileInfo.exists())
                {
                    if (!m_sentCreateAlready.contains(fileAndPath))
                    {
                        DEBUG_FILEWATCHER("    - renamed sending rawFileAdded\n");
                        m_watcher->rawFileAdded(fileAndPath, {});
                        m_sentCreateAlready.insert(fileAndPath);
                    }

                    // macOS does not send out an event for the directory being
                    // modified when a file has been renamed but the FileWatcher
                    // API expects it so send out the modification event ourselves.
                    DEBUG_FILEWATCHER("    - renamed - sending rawFileModified for parent dir\n");
                    m_watcher->rawFileModified(fileInfo.absolutePath(), {});
                }
                else
                {
                    m_watcher->rawFileRemoved(fileAndPath, {});
                    m_sentCreateAlready.remove(fileAndPath);

                    // macOS does not send out an event for the directory being
                    // modified when a file has been renamed but the FileWatcher
                    // API expects it so send out the modification event ourselves.
                    DEBUG_FILEWATCHER("    - renamed - sending rawFileModified for parent dir\n");
                    m_watcher->rawFileModified(fileInfo.absolutePath(), {});
                    
                }
            }
        }
    }
}
