/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
