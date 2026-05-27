/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkyAtmosphere/SkyAtmosphereTypeIds.h>
#include <SkyAtmosphereModuleInterface.h>
#include "SkyAtmosphereEditorSystemComponent.h"
#include "Components/EditorSkyAtmosphereComponent.h"

namespace SkyAtmosphere
{
    class SkyAtmosphereEditorModule
        : public SkyAtmosphereModuleInterface
    {
    public:
        AZ_RTTI(SkyAtmosphereEditorModule, SkyAtmosphereEditorModuleTypeId, SkyAtmosphereModuleInterface);
        AZ_CLASS_ALLOCATOR(SkyAtmosphereEditorModule, AZ::SystemAllocator);

        SkyAtmosphereEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                SkyAtmosphereEditorSystemComponent::CreateDescriptor(),
                SkyAtmosphereComponent::CreateDescriptor(),
                EditorSkyAtmosphereComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<SkyAtmosphereEditorSystemComponent>(),
            };
        }
    };
}// namespace SkyAtmosphere

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), SkyAtmosphere::SkyAtmosphereEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_SkyAtmosphere_Editor, SkyAtmosphere::SkyAtmosphereEditorModule)
#endif
