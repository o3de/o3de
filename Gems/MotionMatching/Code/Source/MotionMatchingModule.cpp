/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MotionMatchingModuleInterface.h>
#include <MotionMatchingSystemComponent.h>

namespace EMotionFX::MotionMatching
{
    class MotionMatchingModule
        : public MotionMatchingModuleInterface
    {
    public:
        AZ_RTTI(MotionMatchingModule, "{cf4381d1-0207-4ef8-85f0-6c88ec28a7b6}", MotionMatchingModuleInterface);
        AZ_CLASS_ALLOCATOR(MotionMatchingModule, AZ::SystemAllocator);
    };
}// namespace EMotionFX::MotionMatching

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), EMotionFX::MotionMatching::MotionMatchingModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_MotionMatching, EMotionFX::MotionMatching::MotionMatchingModule)
#endif
