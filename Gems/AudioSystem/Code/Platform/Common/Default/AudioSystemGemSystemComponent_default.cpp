/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AudioAllocators.h>
#include <SoundCVars.h>
#include <AzCore/Memory/OSAllocator.h>

namespace Audio::Platform
{
    void InitializeAudioAllocators()
    {
        if (!AZ::AllocatorInstance<AudioSystemAllocator>::IsReady())
        {
            AZ::AllocatorInstance<AudioSystemAllocator>::Create();
        }

        if (!AZ::AllocatorInstance<AudioBankAllocator>::IsReady())
        {
            AZ::AllocatorInstance<AudioBankAllocator>::Create();
        }
    }

    void ShutdownAudioAllocators()
    {
        if (AZ::AllocatorInstance<Audio::AudioBankAllocator>::IsReady())
        {
            AZ::AllocatorInstance<Audio::AudioBankAllocator>::Destroy();
        }

        if (AZ::AllocatorInstance<Audio::AudioSystemAllocator>::IsReady())
        {
            AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Destroy();
        }
    }

} // namespace Audio::Platform
