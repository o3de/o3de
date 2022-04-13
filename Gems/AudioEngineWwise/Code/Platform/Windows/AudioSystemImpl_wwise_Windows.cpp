/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AudioSystemImpl_wwise.h>

#include <AK/SoundEngine/Common/AkSoundEngine.h>

namespace Audio
{
    namespace Platform
    {
        void InitializeMemory()
        {
        }

        void SetupAkSoundEngine(AkPlatformInitSettings& platformInitSettings)
        {
            platformInitSettings.threadBankManager.dwAffinityMask = 0;
            platformInitSettings.threadLEngine.dwAffinityMask = 0;
            platformInitSettings.threadMonitor.dwAffinityMask = 0;
        }
    }
}
