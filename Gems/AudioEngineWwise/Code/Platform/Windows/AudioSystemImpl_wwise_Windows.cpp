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

#include <AudioSystemImpl_wwise.h>
#include <AudioLogger.h>

#include <AK/SoundEngine/Common/AkSoundEngine.h>

namespace Audio
{
    namespace Platform
    {
        void InitializeMemory(CAudioLogger&)
        {
        }

        void SetupAkSoundEngine(AkPlatformInitSettings& platformInitSettings)
        {
            // Turn off XAudio2 output type due to rare startup crashes.  Prefers WASAPI or DirectSound.
            platformInitSettings.eAudioAPI = static_cast<AkAudioAPI>(platformInitSettings.eAudioAPI & ~AkAPI_XAudio2);
            platformInitSettings.threadBankManager.dwAffinityMask = 0;
            platformInitSettings.threadLEngine.dwAffinityMask = 0;
            platformInitSettings.threadMonitor.dwAffinityMask = 0;
        }
    }
}
