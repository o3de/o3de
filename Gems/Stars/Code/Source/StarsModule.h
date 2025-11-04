/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <StarsSystemComponent.h>
#include <StarsComponent.h>

namespace AZ::Render
{
    class StarsModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(StarsModule, "{1C13B38B-BAD5-4C42-AB75-9038596CBF3E}", AZ::Module);
        AZ_CLASS_ALLOCATOR(StarsModule, AZ::SystemAllocator);

        StarsModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                StarsSystemComponent::CreateDescriptor(),
                StarsComponent::CreateDescriptor(),
                });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<StarsSystemComponent>()
            };
        }
    };
}// namespace AZ::Render
