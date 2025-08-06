/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AFR/AFRTypeIds.h>
#include <AFRModuleInterface.h>
#include "AFREditorSystemComponent.h"

namespace AFR
{
    class AFREditorModule
        : public AFRModuleInterface
    {
    public:
        AZ_RTTI(AFREditorModule, AFREditorModuleTypeId, AFRModuleInterface);
        AZ_CLASS_ALLOCATOR(AFREditorModule, AZ::SystemAllocator);

        AFREditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                AFREditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<AFREditorSystemComponent>(),
            };
        }
    };
}// namespace AFR

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), AFR::AFREditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_AFR_Editor, AFR::AFREditorModule)
#endif
