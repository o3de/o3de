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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include <AzCore/Console/IConsole.h>

namespace Audio::Wwise::Cvars
{
    AZ_CVAR_EXTERNED(AZ::u64, s_PrimaryMemorySize);
    AZ_CVAR_EXTERNED(AZ::u64, s_SecondaryMemorySize);
    AZ_CVAR_EXTERNED(AZ::u64, s_StreamDeviceMemorySize);
    AZ_CVAR_EXTERNED(AZ::u64, s_CommandQueueMemorySize);

#if !defined(WWISE_RELEASE)
    AZ_CVAR_EXTERNED(AZ::u64, s_MonitorQueueMemorySize);
    AZ_CVAR_EXTERNED(bool, s_EnableCommSystem);
    AZ_CVAR_EXTERNED(bool, s_EnableOutputCapture);
#endif // !WWISE_RELEASE

} // namespace Audio::Wwise::Cvars
