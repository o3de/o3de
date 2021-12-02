/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/string/string.h>

namespace AZ::RHI::Platform
{
    bool IsPixDllInjected([[maybe_unused]] const char* dllName)
    {
        return false;
    }

    AZStd::wstring GetLatestWinPixGpuCapturerPath()
    {
        return L"";
    }
} 
