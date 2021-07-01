/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LmbrCentral_precompiled.h"
#include "EntityDebugDisplayComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <Shape/ShapeDisplay.h>

namespace LmbrCentral
{
    void EntityDebugDisplayComponent::Activate()
    {
        m_currentEntityTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentEntityTransform, GetEntityId(), &AZ::TransformInterface::GetWorldTM);
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
    }

    void EntityDebugDisplayComponent::Deactivate()
    {
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
    }

    void EntityDebugDisplayComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentEntityTransform = world;
    }

    void EntityDebugDisplayComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        DisplayShape(
            debugDisplay,
            []() { return true; }, // canDraw function - in game mode/view we always want to draw, so just return true
            [this](AzFramework::DebugDisplayRequests& debugDisplay)
            {
                Draw(debugDisplay);
            },
            m_currentEntityTransform);
    }

    void EntityDebugDisplayComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EntityDebugDisplayComponent, AZ::Component>()
                ->Version(1);
        }
    }
}
