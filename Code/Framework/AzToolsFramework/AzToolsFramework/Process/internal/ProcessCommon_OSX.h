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

#if AZ_TRAIT_OS_PLATFORM_APPLE

namespace AzToolsFramework
{
    class CommunicatorHandleImpl
    {
    public:
        ~CommunicatorHandleImpl() = default;

        bool IsValid() const;
        bool IsBroken() const;
        int GetHandle() const;

        void Break();
        void Close();
        void SetHandle(int handle);

    protected:
        int m_handle = -1;
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
} // namespace AzToolsFramework

#endif // AZ_TRAIT_OS_PLATFORM_APPLE
