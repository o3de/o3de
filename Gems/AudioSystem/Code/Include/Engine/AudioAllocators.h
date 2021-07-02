/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/Trace.h>
#include <AzCore/Memory/SystemAllocator.h>
#define AUDIO_MEMORY_ALIGNMENT  AZCORE_GLOBAL_NEW_ALIGNMENT

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////
    class AudioSystemAllocator final
        : public AZ::SystemAllocator
    {
    public:
        AZ_TYPE_INFO(AudioSystemAllocator, "{AE15F55D-BD65-4666-B18B-9ED81999A85B}");

        ///////////////////////////////////////////////////////////////////////////////////////////
        // IAllocator
        const char* GetName() const override
        {
            return "AudioSystemAllocator";
        }

        const char* GetDescription() const override
        {
            return "Generic allocator for use in the Audio System module";
        }
        ///////////////////////////////////////////////////////////////////////////////////////////
    };

    using AudioSystemStdAllocator = AZ::AZStdAlloc<AZ::SystemAllocator>;


    ///////////////////////////////////////////////////////////////////////////////////////////////
    class AudioImplAllocator final
        : public AZ::SystemAllocator
    {
    public:
        AZ_TYPE_INFO(AudioImplAllocator, "{197D999F-3093-4F9D-A9A0-BA9E2AAA11DC}");

        ///////////////////////////////////////////////////////////////////////////////////////////
        // IAllocator
        const char* GetName() const override
        {
            return "AudioImplAllocator";
        }

        const char* GetDescription() const override
        {
            return "Generic allocator for use in the Audio Engine Implementation module";
        }
        ///////////////////////////////////////////////////////////////////////////////////////////
    };

    using AudioImplStdAllocator = AZ::AZStdAlloc<AZ::SystemAllocator>;


    ///////////////////////////////////////////////////////////////////////////////////////////////
    class AudioBankAllocator final
        : public AZ::SystemAllocator
    {
    public:
        AZ_TYPE_INFO(AudioBankAllocator, "{19E89718-400F-42F9-92C3-E7F0DC1CCC1F}");

        ///////////////////////////////////////////////////////////////////////////////////////////
        // IAllocator
        const char* GetName() const override
        {
            return "AudioBankAllocator";
        }

        const char* GetDescription() const override
        {
            return "Generic allocator for use by the Audio File Cache Manager for sound banks";
        }
        ///////////////////////////////////////////////////////////////////////////////////////////
    };

} // namespace Audio


#define AUDIO_SYSTEM_CLASS_ALLOCATOR(type)      AZ_CLASS_ALLOCATOR(type, Audio::AudioSystemAllocator, 0)
#define AUDIO_IMPL_CLASS_ALLOCATOR(type)        AZ_CLASS_ALLOCATOR(type, Audio::AudioImplAllocator, 0)
