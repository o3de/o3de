/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/Trace.h>
#include <AzCore/Memory/ChildAllocatorSchema.h>
#include <AzCore/Memory/SystemAllocator.h>
#define AUDIO_MEMORY_ALIGNMENT  AZCORE_GLOBAL_NEW_ALIGNMENT

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////
    AZ_CHILD_ALLOCATOR_WITH_NAME(AudioSystemAllocator, "AudioSystemAllocator", "{AE15F55D-BD65-4666-B18B-9ED81999A85B}", AZ::SystemAllocator);
    using AudioSystemStdAllocator = AudioSystemAllocator_for_std_t;


    ///////////////////////////////////////////////////////////////////////////////////////////////
    AZ_CHILD_ALLOCATOR_WITH_NAME(AudioImplAllocator, "AudioImplAllocator", "{197D999F-3093-4F9D-A9A0-BA9E2AAA11DC}", AZ::SystemAllocator);
    using AudioImplStdAllocator = AudioImplAllocator_for_std_t;


    ///////////////////////////////////////////////////////////////////////////////////////////////
    AZ_CHILD_ALLOCATOR_WITH_NAME(AudioBankAllocator, "AudioBankAllocator", "{19E89718-400F-42F9-92C3-E7F0DC1CCC1F}", AZ::SystemAllocator);

} // namespace Audio


#define AUDIO_SYSTEM_CLASS_ALLOCATOR(type)      AZ_CLASS_ALLOCATOR(type, Audio::AudioSystemAllocator)
#define AUDIO_IMPL_CLASS_ALLOCATOR(type)        AZ_CLASS_ALLOCATOR(type, Audio::AudioImplAllocator)
