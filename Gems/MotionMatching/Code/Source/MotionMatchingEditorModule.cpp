/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MotionMatchingModuleInterface.h>
#include <MotionMatchingEditorSystemComponent.h>

namespace EMotionFX::MotionMatching
{
    class MotionMatchingEditorModule
        : public MotionMatchingModuleInterface
    {
    public:
        AZ_RTTI(MotionMatchingEditorModule, "{cf4381d1-0207-4ef8-85f0-6c88ec28a7b6}", MotionMatchingModuleInterface);
        AZ_CLASS_ALLOCATOR(MotionMatchingEditorModule, AZ::SystemAllocator);

        MotionMatchingEditorModule()
        {
            m_descriptors.insert(m_descriptors.end(),
                {
                    MotionMatchingEditorSystemComponent::CreateDescriptor(),
                });
        }

        /// Add required SystemComponents to the SystemEntity. Non-SystemComponents should not be added here.
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList
                {
                    azrtti_typeid<MotionMatchingEditorSystemComponent>(),
                };
        }
    };
}// namespace EMotionFX::MotionMatching

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), EMotionFX::MotionMatching::MotionMatchingEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_MotionMatching_Editor, EMotionFX::MotionMatching::MotionMatchingEditorModule)
#endif
