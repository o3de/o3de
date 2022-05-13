/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/RecastNavigationSystemComponent.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace RecastNavigation
{
    void RecastNavigationSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<RecastNavigationSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void RecastNavigationSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RecastNavigationSystemService"));
    }

    void RecastNavigationSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RecastNavigationSystemService"));
    }

    void RecastNavigationSystemComponent::Activate()
    {
        m_navigationMeshAssetHandler = AZStd::make_unique<NavigationMeshAssetHandler>();
        m_navigationMeshAssetHandler->Register();
    }

    void RecastNavigationSystemComponent::Deactivate()
    {
        m_navigationMeshAssetHandler->Unregister();
        m_navigationMeshAssetHandler = {};
    }

} // namespace RecastNavigation
