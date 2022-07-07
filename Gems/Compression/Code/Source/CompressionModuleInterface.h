/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <Clients/CompressionSystemComponent.h>

namespace Compression
{
    class CompressionModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(CompressionModuleInterface, "{89158BD2-EAC1-4CBF-9F05-9237034FF28E}", AZ::Module);
        AZ_CLASS_ALLOCATOR(CompressionModuleInterface, AZ::SystemAllocator, 0);

        CompressionModuleInterface()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                CompressionSystemComponent::CreateDescriptor(),
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CompressionSystemComponent>(),
            };
        }
    };
}// namespace Compression
