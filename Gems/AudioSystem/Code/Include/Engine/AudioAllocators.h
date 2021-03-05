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
        AZ_CLASS_ALLOCATOR(AudioSystemAllocator, AZ::SystemAllocator, 0)
        AZ_TYPE_INFO(AudioSystemAllocator, "{AE15F55D-BD65-4666-B18B-9ED81999A85B}")

        ///////////////////////////////////////////////////////////////////////////////////////////
        // IAllocator
        const char* GetName() const override
        {
            return "AudioSystemAllocator";
        }

        const char* GetDescription() const override
        {
            return "Generic allocator for use in the Audio System Module.";
        }
        ///////////////////////////////////////////////////////////////////////////////////////////
    };

    using AudioSystemStdAllocator = AZ::AZStdAlloc<AZ::SystemAllocator>;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    class AudioImplAllocator final
        : public AZ::SystemAllocator
    {
    public:
        AZ_CLASS_ALLOCATOR(AudioImplAllocator, AZ::SystemAllocator, 0)
        AZ_TYPE_INFO(AudioImplAllocator, "{197D999F-3093-4F9D-A9A0-BA9E2AAA11DC}")

        ///////////////////////////////////////////////////////////////////////////////////////////
        // IAllocator
        const char* GetName() const override
        {
            return "AudioImplAllocator";
        }

        const char* GetDescription() const override
        {
            return "Generic allocator for use in the Audio Implementation Module.";
        }
        ///////////////////////////////////////////////////////////////////////////////////////////
    };

    using AudioImplStdAllocator = AZ::AZStdAlloc<AZ::SystemAllocator>;

} // namespace Audio


#define AUDIO_SYSTEM_CLASS_ALLOCATOR(type)      AZ_CLASS_ALLOCATOR(type, Audio::AudioSystemAllocator, 0)
#define AUDIO_IMPL_CLASS_ALLOCATOR(type)        AZ_CLASS_ALLOCATOR(type, Audio::AudioImplAllocator, 0)
