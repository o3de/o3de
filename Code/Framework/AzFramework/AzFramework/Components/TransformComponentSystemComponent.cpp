/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/EntityActiveSystemBus.h>
#include <AzFramework/Components/TransformComponentSystemComponent.h>

namespace AzFramework
{
    void TransformComponentSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TransformComponentSystemComponent, AZ::Component>()->Version(1);
        }
    }

    void TransformComponentSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("TransformSystemService"));
    }

    void TransformComponentSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("TransformSystemService"));
    }

    void TransformComponentSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("EntityActivateSystemService"));
    }

    void TransformComponentSystemComponent::Activate()
    {
        AZ::EntityActiveSystemRequestBus::Broadcast(&AZ::EntityActiveSystemRequests::RegisterEntityActiveType, PARENT_ACTIVE_TYPE_NAME);
    }

    void TransformComponentSystemComponent::Deactivate()
    {
    }

} // namespace AzFramework
