/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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