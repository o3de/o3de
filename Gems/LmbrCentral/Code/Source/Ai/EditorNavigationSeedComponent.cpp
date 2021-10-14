/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorNavigationSeedComponent.h"
#include "EditorNavigationUtil.h"

// Cry pathfinding system
#include <INavigationSystem.h>
#include <MathConversion.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace LmbrCentral
{
    void EditorNavigationSeedComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorNavigationSeedComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("Agent Type", &EditorNavigationSeedComponent::m_agentType);

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorNavigationSeedComponent>("Navigation Seed", "Determines reachable navigation nodes")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::Category, "AI")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/NavigationSeed.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/NavigationSeed.svg")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/ai/nav-seed/")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorNavigationSeedComponent::m_agentType, "Agent Type", "Describes the type of the Entity for navigation purposes.")
                        ->Attribute(AZ::Edit::Attributes::StringList, &PopulateAgentTypeList)
                        ->Attribute("ChangeNotify", &EditorNavigationSeedComponent::OnAgentTypeChanged);
            }
        }
    }

    void EditorNavigationSeedComponent::TriggerReachaibilityRecalculation() const
    {
        // We have to call the legacy method to account for Legacy seeds that might be out there
        INavigationSystem* navigationSystem = nullptr; // INavigationSystem will be converted to an AZInterface (LY-111343)
        if (navigationSystem)
        {
            navigationSystem->CalculateAccessibility();
        }
    }

    AZ::u32 EditorNavigationSeedComponent::OnAgentTypeChanged() const
    {
        TriggerReachaibilityRecalculation();
        return AZ_CRC("RefreshNone", 0x98a5045b);
    }

    void EditorNavigationSeedComponent::RecalculateReachabilityAroundSelf()
    {
        AZ::Vector3 translation = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(translation, m_entity->GetId(), &AZ::TransformInterface::GetWorldTranslation);
        
        INavigationSystem* navigationSystem = nullptr; // INavigationSystem will be converted to an AZInterface (LY-111343)
        if (navigationSystem)
        {
            auto agentType = navigationSystem->GetAgentTypeID(m_agentType.c_str());
            navigationSystem->ComputeAccessibility(AZVec3ToLYVec3(translation), agentType);
        }
    }

    void EditorNavigationSeedComponent::OnTransformChanged(const AZ::Transform&, const AZ::Transform&)
    {
        TriggerReachaibilityRecalculation();
    }

    void EditorNavigationSeedComponent::Activate()
    {
        Base::Activate();

        NavigationSeedRequestsBus::Handler::BusConnect();
        AZ::TransformNotificationBus::Handler::BusConnect(m_entity->GetId());

        TriggerReachaibilityRecalculation();
    }

    void EditorNavigationSeedComponent::Deactivate()
    {
        Base::Deactivate();

        NavigationSeedRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect(m_entity->GetId());
    }
} // LmbrCentral
