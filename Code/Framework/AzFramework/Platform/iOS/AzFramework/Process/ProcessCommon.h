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

namespace AzFramework
{
    class CommunicatorHandleImpl
    {
    public:
        ~CommunicatorHandleImpl() = default;

        bool IsValid() const { return false; }
        bool IsBroken() const { return false; }
        int GetHandle() const { return -1; }

        void Break() {}
        void Close() {}
        void SetHandle([[maybe_unused]] int handle) {}

    };

    struct StartupInfo
    {
        ~StartupInfo();

        void SetupHandlesForChildProcess();
        void CloseAllHandles();

        int m_inputHandleForChild = -1;
        int m_outputHandleForChild = -1;
        int m_errorHandleForChild = -1;
    };

    struct ProcessData
    {
        StartupInfo m_startupInfo;
        int m_childProcessId = 0;
        bool m_childProcessIsDone = false;
    };
} // namespace AzFramework

