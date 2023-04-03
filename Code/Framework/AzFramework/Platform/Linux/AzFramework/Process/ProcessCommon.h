/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

        [[nodiscard]] bool IsValid() const;
        [[nodiscard]] bool IsBroken() const;
        [[nodiscard]] int GetHandle() const;
        [[nodiscard]] int GetEPollHandle() const;

        void Break();
        void Close();
        [[nodiscard]] bool SetHandle(int handle, bool createEPollHandle);

    protected:
        int m_handle = -1;
        int m_epollHandle = -1;
        bool m_broken = false;
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

