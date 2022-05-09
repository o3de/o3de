/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzCore/std/typetraits/remove_pointer.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <native/FileWatcher/FileWatcher.h>
#include <AzCore/PlatformIncl.h>

struct HandleDeleter
{
    void operator()(HANDLE handle)
    {
        if (handle && handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(handle);
        }
    }
};

using HandleUniquePtr = AZStd::unique_ptr<AZStd::remove_pointer_t<HANDLE>, HandleDeleter>;

class FileWatcher::PlatformImplementation
{
public:
    bool AddWatchFolder(QString folder, bool recursive);

    struct FolderRootWatch
    {
        FolderRootWatch(AZStd::unique_ptr<OVERLAPPED>&& overlapped, HandleUniquePtr&& directoryHandle, QString root, bool recursive)
            : m_overlapped(AZStd::move(overlapped))
            , m_directoryHandle(AZStd::move(directoryHandle))
            , m_directoryRoot(AZStd::move(root))
            , m_recursive(recursive)
        {
        }

        bool ReadChanges();

        AZStd::unique_ptr<OVERLAPPED> m_overlapped; // Identifies this root watch
        HandleUniquePtr m_directoryHandle;
        QString m_directoryRoot;
        bool m_recursive;
        AZStd::aligned_storage_t<64 * 1024, sizeof(DWORD)> m_fileNotifyInformationList{};
    };

    enum class EventType
    {
        FileRead,
        Shutdown
    };

    AZStd::unordered_map<LPOVERLAPPED, FolderRootWatch> m_folderRootWatches;

    HandleUniquePtr m_ioHandle{CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, /*CompletionKey =*/ static_cast<ULONG_PTR>(EventType::FileRead), 1)};
};
