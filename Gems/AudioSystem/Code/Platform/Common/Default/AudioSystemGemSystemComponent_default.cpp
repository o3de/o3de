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
        // Create audio system memory pool
        if (!AZ::AllocatorInstance<AudioSystemAllocator>::IsReady())
        {
            AudioSystemAllocator::Descriptor allocDesc;

            // Generic Allocator:
            allocDesc.m_allocationRecords = true;
            AZ::AllocatorInstance<AudioSystemAllocator>::Create(allocDesc);
        }

        // Create the Bank allocator...
        if (!AZ::AllocatorInstance<AudioBankAllocator>::IsReady())
        {
            AudioBankAllocator::Descriptor allocDesc;
            allocDesc.m_allocationRecords = true;
            AZ::AllocatorInstance<AudioBankAllocator>::Create(allocDesc);
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
