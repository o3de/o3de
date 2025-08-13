/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MiniAudioModuleInterface.h>

namespace MiniAudio
{
    class MiniAudioModule : public MiniAudioModuleInterface
    {
    public:
        AZ_RTTI(MiniAudioModule, "{501C94A1-993A-4203-9720-D43D6C1DDB7A}", MiniAudioModuleInterface);
        AZ_CLASS_ALLOCATOR(MiniAudioModule, AZ::SystemAllocator, 0);
    };
} // namespace MiniAudio

// this must match the actual name of the gem module target from CMakeLists.txt
AZ_DECLARE_MODULE_CLASS(Gem_O3DEMiniAudio, MiniAudio::MiniAudioModule)
