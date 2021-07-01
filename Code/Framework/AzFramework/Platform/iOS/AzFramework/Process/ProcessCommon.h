/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

