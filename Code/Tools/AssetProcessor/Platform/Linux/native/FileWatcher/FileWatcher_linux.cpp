/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/string/fixed_string.h>
#include <native/FileWatcher/FileWatcher.h>
#include <native/FileWatcher/FileWatcher_platform.h>

#include <QDirIterator>
#include <QHash>
#include <QFileInfo>

// set this to 1 if you find yourself debugging this code and want to see the debug spam:
#define ALLOW_FILEWATCHER_DEBUG_SPAM 0

#if ALLOW_FILEWATCHER_DEBUG_SPAM
#define DEBUG_FILEWATCHER(...) printf("FileWatcher:" __VA_ARGS__)
#else
#define DEBUG_FILEWATCHER(...) 
#endif

// There is a classic race for linux inotify file watching.  
// inotify file watching on Linux requires you to establish a watch handle on every object you want to watch and does not recurse.
// these notifies are generated when the actual file is created/modified/deleted/etc, and only for the notifies already present
// on the object at that exact moment.

// This means that if you want to know about file creation or modification, you have to be watching the directory the file is 
// created in before the file is created or modified or you will miss the event.

// A race condition occurs in the case where you are interested in all file notifications and don't want to miss any, but
// someone creates a new directory and immediately creates a new file in that directory.  If nobody has a watch handle established
// on the new directory by the time the file is created, then nobody will be told about it.  Because files and directories
// are created by kernel threads in response to other threads, you cannot control whether any inotify-based watcher gets
// told about the new directory before or after the new file is created in it.  And even if you process the new directory
// event very quickly, there is no guarantee that you are able to establish a watch on it before whatever other thread is working
// on creating the new file finishes doing so, since various parts of these operations are asynchronous.

// Thus, if you want to guarantee that you don't miss anything you have to emit synthetic 'file was created' events by crawling
// the directory of any new directories which show up.  However, this causes potentially duplicate events to be emitted in the case
// where the watch IS established on that directory before the file is created.   Thus if you also want to guarantee no duplicates
// you have to come up with ways to cache the create events sent and not re-send them.

// Note that this same problem can occur for Create->modify, Create->Delete, although Create->Delete tends to be ignorable
// and Create->modify is not usually a problem since apps are usually reacting to the creation the same way as they react to modify
// so in this implementation we will at least attempt to guarantee
//  - you don't miss creation events for anything
//  - you don't get duplicate creation events for anything

#include <unistd.h>
#include <sys/inotify.h>
#include <sys/eventfd.h>
#include <sys/poll.h>

static constexpr size_t s_inotifyMaxEntries = 1024 * 16;         // Control the maximum number of entries (from inotify) that can be read at one time
static constexpr size_t s_inotifyEventSize = sizeof(struct inotify_event);
static constexpr size_t s_inotifyReadBufferSize = s_inotifyMaxEntries * s_inotifyEventSize;

bool FileWatcher::PlatformImplementation::Initialize()
{
    if (m_inotifyHandle < 0)
    {
        // The CLOEXEC flag prevents the inotify watchers from copying on fork/exec
        m_inotifyHandle = inotify_init1(IN_CLOEXEC);

        [[maybe_unused]] const auto err = errno;
        [[maybe_unused]] AZStd::fixed_string<255> errorString;
        AZ_Warning("FileWatcher", (m_inotifyHandle >= 0), "Unable to initialize inotify, file monitoring will not be available: %s\n", strerror_r(err, errorString.data(), errorString.capacity()));
    }

    // create a handle that you can write to, in order to wake up the listening thread immediately.
    m_wakeThreadHandle = eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE);

    return (m_inotifyHandle >= 0);
}

void FileWatcher::PlatformImplementation::CloseMainWatchHandle()
{
    if (m_inotifyHandle < 0)
    {
        return;
    }
      
    ::close(m_inotifyHandle);
    m_inotifyHandle = -1;

    // signal the thread to awaken in case its blocked on read.
    AZ::u64 flagValue = 1;
    if (write(m_wakeThreadHandle, &flagValue, sizeof(flagValue)) != sizeof(flagValue))
    {
        AZ_ErrorOnce("FileWatcher", false, "Was unable to write to the wake-up-the-thread handle, this may cause timeouts and deadlocks.\n");
    }
}

void FileWatcher::PlatformImplementation::Finalize()
{
    // This happens after the watch thread has already joined.
    
    // note from the man-page of inotify " all associated watches are automatically freed." - no need to call inotify_rm_watch on each handle.
    if (m_wakeThreadHandle >= 0)
    {
        close(m_wakeThreadHandle);
        m_wakeThreadHandle = -1;
    }
    m_handleToFolderMap.clear();
    m_alreadyNotifiedCreate.clear();
}

bool FileWatcher::PlatformImplementation::TryToWatch(const QString &pathStr, int* errnoPtr)
{
    AZ::IO::FixedMaxPathString path(pathStr.toUtf8().constData());
    // note:  IN_MASK_CREATE will set EEXIST if the directory already has a watch established on it.
    // note:  IN_ONLYDIR will set ENOTDIR if the watch was attempted to establish on a file instead of a dir.
    int watchHandle = inotify_add_watch(
        m_inotifyHandle, path.c_str(), IN_CREATE | IN_CLOSE_WRITE | IN_DELETE | IN_DELETE_SELF | IN_MODIFY | IN_MOVE | IN_MOVE_SELF | IN_MASK_CREATE | IN_ONLYDIR); 
    const auto err = errno; // Only contains relevant information if we actually failed

    if (watchHandle < 0)
    {
        if (err == ENOTDIR)
        {
            // the dir being watched was deleted and replaced by a file before we managed to watch it.
            // this is okay, and absorbing this removes a race condition.
            DEBUG_FILEWATCHER("Not adding an additional file watch for %s - it is not a directory\n", pathStr.toUtf8().constData());
            return false;
        }
        if (err == EEXIST)
        {
            // the dir already has a watch handle on it that belongs to m_inotifyHandle.
            // avoid duplicating watches.  Return false here, to stop the caller from recursing into the folder
            // since it indicates it has already previously recursed.
            DEBUG_FILEWATCHER("Not adding an additional file watch for %s - already exists\n", pathStr.toUtf8().constData());
            return false;
        }
        [[maybe_unused]] const char* extraStr = (err == ENOSPC ? " (try increasing fs.inotify.max_user_watches with sysctl)" : "");
        [[maybe_unused]] AZStd::fixed_string<255> errorString;
        AZ_Warning(
            "FileWatcher", false,
            "inotify_add_watch failed for path %s with error %d: %s%s",
            path.c_str(), err, strerror_r(err, errorString.data(), errorString.capacity()), extraStr);
        if (errnoPtr)
        {
            *errnoPtr = err;
        }
        return false;
    }

    m_handleToFolderMap[watchHandle] = pathStr;
    DEBUG_FILEWATCHER("added actual watch to (%s) - handle is %i\n", pathStr.toUtf8().constData(), watchHandle);
    return true;
}

void FileWatcher::PlatformImplementation::AddWatchFolder(QString folder, bool recursive, FileWatcher& source, bool notifyFiles)
{
    DEBUG_FILEWATCHER("AddWatchFolder(%s) - notify: %s\n", folder.toUtf8().constData(), notifyFiles ? "True" : "False");
    if (m_inotifyHandle < 0)
    {
        return;
    }

    // Clean up the path before accepting it as a watch folder
    QString cleanPath = QDir::cleanPath(folder);

    if (source.IsExcluded(cleanPath))
    {
        DEBUG_FILEWATCHER("'%s' matches an exclusion rule.  Not watching.\n", cleanPath.toUtf8().constData());
        return; // don't watch excluded paths and don't recurse into them.
    }

    if (!TryToWatch(cleanPath, nullptr))
    {
        return;
    }
    
    // Each watch costs linux a file handle from a limited (default 8k) set of handles.
    // as such, we don't want to establish a watch on any excluded dirs.
    // Its safer (and more efficient) to first establish the watches, then check for files.
    QList<QString> dirsAdded;
    dirsAdded.push_back(cleanPath);

    QDirIterator dirIter(
        folder, QDir::NoDotAndDotDot | QDir::Dirs,
        (recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags) | QDirIterator::FollowSymlinks);

    while (dirIter.hasNext())
    {
        QString dirPath = dirIter.next();

        if (source.IsExcluded(dirPath))
        {
            DEBUG_FILEWATCHER("'%s' matches an exclusion rule during subtree traversal.  Not watching.\n", dirPath.toUtf8().constData());
            continue; // do not "see" excluded dirs at all.
        }

        if (notifyFiles)
        {
            if (!m_alreadyNotifiedCreate.contains(dirPath))
            {
                DEBUG_FILEWATCHER("%s rawFileAdded for root AddWatchFolder\n", dirPath.toUtf8().constData());
                source.rawFileAdded(dirPath, {});
                m_alreadyNotifiedCreate.insert(dirPath);
            }
        }

        // we do not want to establish a watch on dirs that are children of non-recursive dirs.
        if (!recursive)
        {
            continue;
        }

        int theErrno = 0;
        if (!TryToWatch(dirPath, &theErrno))
        {
            switch (theErrno)
            {
            case EACCES:
            case EBADF:
            case ENOENT:
            case ENOTDIR:
            case EEXIST:
                // Errors specific to the directory: try next one
                continue;
            default:
                // Other errors are usually non-recoverable: bail out to avoid warning spam
                AZ_Warning(
                    "FileWatcher", false,
                    "Giving up on watching %s (ErrNo: %i)", dirPath.toUtf8().constData(), theErrno);
                return;
            }
        }
        else
        {
            dirsAdded.push_back(dirPath);
        }
    }

    // we only need to check files if we've been asked to notify when they change.
    if (notifyFiles)
    {
        for (const QString& addedDir : dirsAdded)
        {
            QFileInfoList filesInDir = QDir(addedDir).entryInfoList(QDir::NoDotAndDotDot | QDir::Files);

            for (const QFileInfo& fileInfo : filesInDir)
            {
                QString filePath = fileInfo.absoluteFilePath();
                if (source.IsExcluded(filePath))
                {
                    DEBUG_FILEWATCHER("%s matches an exclusion rule during file traversal.  Not Notifying.\n", filePath.toUtf8().constData());
                    continue; // do not "see" excluded files at all.
                }

                if (!m_alreadyNotifiedCreate.contains(filePath))
                {
                    DEBUG_FILEWATCHER("%s rawFileAdded via recursive directory crawl for file\n", filePath.toUtf8().constData());
                    source.rawFileAdded(filePath, {});
                    m_alreadyNotifiedCreate.insert(filePath);
                }
            }
        }
    }
}

void FileWatcher::PlatformImplementation::RemoveWatchFolder(int watchHandle)
{
    if (m_inotifyHandle < 0)
    {
        return;
    }

    if (m_handleToFolderMap.remove(watchHandle))
    {
        inotify_rm_watch(m_inotifyHandle, watchHandle);
    }
}

bool FileWatcher::PlatformStart()
{
    // inotify will be used by linux to monitor file changes within directories under the root folder
    if (!m_platformImpl->Initialize())
    {
        return false;
    }
    for (const auto& [directory, recursive] : m_folderWatchRoots)
    {
        if (QDir(directory).exists())
        {
            // this happens BEFORE we start the thread that listens to the file queue, so there is no need for a lock here.
            m_platformImpl->AddWatchFolder(directory, recursive, *this, false);
        }
    }

    AZ_TracePrintf("FileWatcher", "Using %i file watch handles.\n", static_cast<int>(m_platformImpl->m_handleToFolderMap.size()));

    return true;
}

void FileWatcher::PlatformStop()
{
    // close the handle and signal the thread to wake up
    m_platformImpl->CloseMainWatchHandle();
  
    if (m_thread.joinable()) // it may have already finished.
    {
        m_thread.join(); // wait for the thread to finish
    }

    m_platformImpl->Finalize();
}

void FileWatcher::WatchFolderLoop()
{
    char eventBuffer[s_inotifyReadBufferSize];

    [[maybe_unused]] int cycleCount = 0;

    constexpr const nfds_t nfds = 2;
    struct pollfd fds[nfds];

    m_startedSignal = true; // signal that we are no longer going to drop any events.

    while (!m_shutdownThreadSignal)
    {
        memset(eventBuffer, 0, s_inotifyReadBufferSize);

        fds[0].fd = m_platformImpl->m_wakeThreadHandle; 
        fds[0].events = POLLIN;
        fds[1].fd = m_platformImpl->m_inotifyHandle; 
        fds[1].events = POLLIN;

        int numPollEvents = poll(fds, nfds, -1);
        if (numPollEvents == -1) 
        {
            break; // error polling.
        }

        // were we woken up by the wake thread event?
        if (fds[0].revents & POLLIN)
        {
            break;
        }

        ssize_t bytesRead = 0;
        if ((fds[1].revents & POLLIN) && (!m_shutdownThreadSignal))
        {
            bytesRead = ::read(m_platformImpl->m_inotifyHandle, eventBuffer, s_inotifyReadBufferSize);
            if (bytesRead < 0)
            {
                // Break out of the loop when the notify handle was closed (outside of this thread)
                break;
            }
            
            if (!bytesRead)
            {
                continue;
            }
        }

        if (m_shutdownThreadSignal)
        {
            break;
        }

        cycleCount++;

        for (size_t index=0; index<bytesRead;)
        {
            const auto* event = reinterpret_cast<inotify_event*>(&eventBuffer[index]);

            if (event->mask & (IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVE | IN_DELETE_SELF | IN_MOVE_SELF ))
            {
                // note that the event->name coming in is relative to the thing being watched.  Since we watch folders,
                // for the folder itself, this will be blank, for files in it, it will be the file name.
                QDir watchedDir(m_platformImpl->m_handleToFolderMap[event->wd]);
                const QString pathStr = watchedDir.absoluteFilePath(event->name);

                if (event->mask & (IN_CREATE | IN_MOVED_TO))
                {
                    DEBUG_FILEWATCHER("notify event is IN_CREATE | IN_MOVED_TO (flags 0x%08x) %s (from '%s') cycle: %i \n", event->mask, pathStr.toUtf8().constData(), event->name, cycleCount);
                    const auto found = AZStd::find_if(begin(m_folderWatchRoots), end(m_folderWatchRoots), [this, event](const WatchRoot& watchRoot)
                        {
                            return watchRoot.m_directory == m_platformImpl->m_handleToFolderMap[event->wd];
                        });

                    bool isChildOfRootFolder = found != end(m_folderWatchRoots);
                    bool isChildOfRecursiveRootFolder = isChildOfRootFolder && found->m_recursive;
                    
                    if (event->mask & IN_ISDIR)
                    {
                        // for directories, we only care about create or delete, not modify
                        // so we only need to add a watch to them if they may have children we're interested in.
                        // this is only the case if they are either a child of a recursive root folder, or if they are a child
                        // of some non-root folder (because that infers that their parent is recursive)
                        if ((isChildOfRecursiveRootFolder) || (!isChildOfRootFolder))
                        {
                            // note that we check IsExcluded inside AddWatchFolder, but its also checked inside all the
                            // raw*** functions like rawFileAdded.  Since IsExcluded is expensive, we check it once here
                            // and if so, we don't send it to AddWatchFolder and we don't send it to the raw file queue either,
                            // avoiding multiple redundant IsExcluded calls.
                            if (!IsExcluded(pathStr))
                            {
                                // first, notify about the folder itself, to keep things in order (ie, parent folders then child folders, then files):
                                if (!m_platformImpl->m_alreadyNotifiedCreate.remove(pathStr))
                                {
                                    DEBUG_FILEWATCHER("sending rawFileAdded(%s) from file monitor cycle: %i \n", pathStr.toUtf8().constData(), cycleCount);
                                    rawFileAdded(pathStr, {});
                                }

                                // when a folder is MOVED, we don't notify for all the files inside that folder, only
                                // the folder itself, so as to be consistent with other implementations and the API Contract:
                                bool shouldNotifyAllFilesInFolder = (event->mask & IN_MOVED_TO) == 0;
                                m_platformImpl->AddWatchFolder(pathStr, true, *this, shouldNotifyAllFilesInFolder);
                            }
                            else
                            {
                                DEBUG_FILEWATCHER("'%s'excluded during notify event - dropping \n", pathStr.toUtf8().constData());
                            }
                        }
                    }
                    else
                    {
                        // if we get here, we're looking at a file create/move.  In that case, we always send the notify.  
                        // note that rawFileAdded will eventually check it for ignore anyway.
                        if (!m_platformImpl->m_alreadyNotifiedCreate.remove(pathStr))
                        {
                            DEBUG_FILEWATCHER("sending rawFileAdded(%s) from file monitor cycle: %i \n", pathStr.toUtf8().constData(), cycleCount);
                            rawFileAdded(pathStr, {});
                        }
                        else
                        {
                            DEBUG_FILEWATCHER("SKIPPING sending rawFileAdded(%s) from file monitor cycle: %i \n", pathStr.toUtf8().constData(), cycleCount);
                        }
                    }
                }

                if (event->mask & (IN_DELETE | IN_MOVED_FROM))
                {
                    DEBUG_FILEWATCHER("notify event is IN_DELETE | IN_MOVED_FROM: %s (from '%s' - handle %i) cycle: %i\n", pathStr.toUtf8().constData(), event->name, event->wd, cycleCount);
                    DEBUG_FILEWATCHER("sending rawFileRemoved(%s)\n", pathStr.toUtf8().constData());
                    m_platformImpl->m_alreadyNotifiedCreate.remove(pathStr);
                    rawFileRemoved(pathStr, {});
                }

                if (event->mask & (IN_DELETE_SELF | IN_MOVE_SELF))
                {
                    // This is called on the actual watched folder being moved out, or deleted.
                    // Because we only watch folders, not files, we can assume that the object that this event is coming from is a folder.

                    // If its a move as opposed to delete, it will also be accompanied by a IN_MOVED_FROM and IN_MOVED_TO, so we should 
                    // uninstall any watch here, since IN_MOVED_TO will install a new one if necessary.
                    DEBUG_FILEWATCHER("notify event is IN_MOVE_SELF | IN_DELETE_SELF: %s (from '%s' handle %i ) cycle: %i \n", pathStr.toUtf8().constData(), event->name, event->wd, cycleCount);
                    DEBUG_FILEWATCHER("removing watch dir (%s)\n", pathStr.toUtf8().constData());
                    m_platformImpl->m_alreadyNotifiedCreate.remove(pathStr);
                    m_platformImpl->RemoveWatchFolder(event->wd);
                }
                if (event->mask & IN_MODIFY)
                {
                    DEBUG_FILEWATCHER("notify event is modify, sending rawFileModified: '%s' (from event-Name '%s') cycle: %i mask 0x%08x\n", pathStr.toUtf8().constData(), event->name, cycleCount, event->mask);
                    m_platformImpl->m_alreadyNotifiedCreate.remove(pathStr);
                    rawFileModified(pathStr, {});
                }
            }
            index += s_inotifyEventSize + event->len;
        }
    }
}
