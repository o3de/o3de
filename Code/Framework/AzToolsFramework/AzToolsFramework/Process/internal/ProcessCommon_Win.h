/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/PlatformIncl.h>

namespace AzToolsFramework
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

