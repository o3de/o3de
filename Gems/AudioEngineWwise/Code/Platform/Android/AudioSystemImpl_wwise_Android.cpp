/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AudioSystemImpl_wwise.h>

#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AzCore/Android/AndroidEnv.h>

namespace Audio
{
    namespace Platform
    {
        void InitializeMemory()
        {
        }

        void SetupAkSoundEngine(AkPlatformInitSettings& platformInitSettings)
        {
            JNIEnv* jniEnv = AZ::Android::AndroidEnv::Get()->GetJniEnv();
            jobject javaActivity = AZ::Android::AndroidEnv::Get()->GetActivityRef();
            JavaVM* javaVM = nullptr;
            if (jniEnv)
            {
               jniEnv->GetJavaVM(&javaVM);
            }

            platformInitSettings.pJavaVM = javaVM;
            platformInitSettings.jActivity = javaActivity;
        }
    }
}
