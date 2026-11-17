/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ViewportSnapping/ViewportSnappingSystemComponent.h>
#include <AzToolsFramework/ViewportSnapping/ViewportSnapper.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    namespace ViewportSnapping
    {
        // Defined here rather than defaulted in the header: the destructor of the unique_ptr member
        // needs the complete type, which the header deliberately does not have.
        ViewportSnappingSystemComponent::ViewportSnappingSystemComponent() = default;
        ViewportSnappingSystemComponent::~ViewportSnappingSystemComponent() = default;

        void ViewportSnappingSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ViewportSnappingSystemComponent, AZ::Component>()->Version(1);
            }
        }

        void ViewportSnappingSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ViewportSnappingService"));
        }

        void ViewportSnappingSystemComponent::GetIncompatibleServices(
            AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            // Only one implementation may hold the AZ::Interface registration.
            incompatible.push_back(AZ_CRC_CE("ViewportSnappingService"));
        }

        void ViewportSnappingSystemComponent::Activate()
        {
            m_snapper = AZStd::make_unique<ViewportSnapper>();
            m_snapper->Register();
        }

        void ViewportSnappingSystemComponent::Deactivate()
        {
            if (m_snapper)
            {
                m_snapper->Unregister();
                m_snapper.reset();
            }
        }
    } // namespace ViewportSnapping
} // namespace AzToolsFramework
