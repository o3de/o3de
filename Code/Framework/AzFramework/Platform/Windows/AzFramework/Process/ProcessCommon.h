/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/PlatformIncl.h>

namespace AzFramework
{
    class CommunicatorHandleImpl
    {
    public:
        ~CommunicatorHandleImpl() = default;

        bool IsPipe() const;
        bool IsValid() const;
        bool IsBroken() const;
        const HANDLE& GetHandle() const;

        void Break();
        void Close();
        void SetHandle(const HANDLE& handle, bool isPipe);

    protected:
        HANDLE m_handle = INVALID_HANDLE_VALUE;
        bool m_pipe = false;
        bool m_broken = false;
    };

    struct ProcessData
    {
        ProcessData();

        void Init(bool stdCommunication);

        DWORD WaitForJobOrProcess(AZ::u32 waitTimeInMilliseconds) const;

        PROCESS_INFORMATION processInformation;
        BOOL inheritHandles;
        STARTUPINFOW startupInfo;

        HANDLE jobHandle;
        JOBOBJECT_ASSOCIATE_COMPLETION_PORT jobCompletionPort;
    };
} // namespace AzToolsFramework

