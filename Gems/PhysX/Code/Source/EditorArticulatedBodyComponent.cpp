/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>

#include <Source/ArticulatedBodyComponent.h>
#include <Source/EditorArticulatedBodyComponent.h>
#include "EditorColliderComponent.h"
#include "ToolsComponents/TransformComponent.h"

namespace PhysX
{
    void EditorArticulatedBodyComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorArticulatedBodyComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Field("Configuration", &EditorArticulatedBodyComponent::m_config)
                ->Field("PhysXSpecificConfiguration", &EditorArticulatedBodyComponent::m_physxSpecificConfig)
                ->Field("JointConfig", &EditorArticulatedBodyComponent::m_jointConfig)
                ->Field("LinkData", &EditorArticulatedBodyComponent::m_articulationLinkData)
                ->Version(2)
            ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                constexpr const char* ToolTip = "Articulated rigid body.";

                editContext
                    ->Class<EditorArticulatedBodyComponent>("PhysX Articulated Rigid Body", ToolTip)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/PhysXRigidBody.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/PhysXRigidBody.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(
                        AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/physx/rigid-body/")

                    ->DataElement(0, &EditorArticulatedBodyComponent::m_config, "Configuration", "Configuration for rigid body physics.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorArticulatedBodyComponent::m_physxSpecificConfig,
                        "PhysX-Specific Configuration",
                        "Settings which are specific to PhysX, rather than generic.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorArticulatedBodyComponent::m_jointConfig,
                        "Joint Configuration",
                        "Joint configuration for the articulation link.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ;
            }
        }
    }

    void EditorArticulatedBodyComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
        provided.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
        provided.push_back(AZ_CRC_CE("PhysicsStaticRigidBodyService"));
    }

    void EditorArticulatedBodyComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
    }

    void EditorArticulatedBodyComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void EditorArticulatedBodyComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    bool EditorArticulatedBodyComponent::IsRootArticulation() const
    {
        AzToolsFramework::Components::TransformComponent* thisTransform =
            GetEntity()->FindComponent<AzToolsFramework::Components::TransformComponent>();

        AZ::EntityId parentId = thisTransform->GetParentId();
        if (parentId.IsValid())
        {
            AZ::Entity* parentEntity = nullptr;

            AZ::ComponentApplicationBus::BroadcastResult(parentEntity, &AZ::ComponentApplicationBus::Events::FindEntity, parentId);

            if (parentEntity && parentEntity->FindComponent<EditorArticulatedBodyComponent>())
            {
                return false;
            }
        }

        return true;
    }

    void EditorArticulatedBodyComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        AZ::TickBus::Handler::BusConnect();
    }

    void EditorArticulatedBodyComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
    }

    void EditorArticulatedBodyComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        ArticulatedBodyComponent* component = gameEntity->CreateComponent<ArticulatedBodyComponent>();
        component->m_articulationLinkData = m_articulationLinkData;
    }

    void EditorArticulatedBodyComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        if (IsRootArticulation())
        {
            UpdateArticulationHierarchy();
        }
    }

    void EditorArticulatedBodyComponent::UpdateArticulationHierarchy()
    {
        m_articulationLinkData.Reset();

        AZStd::vector<AZ::EntityId> children = GetEntity()->GetTransform()->GetChildren();
        for (auto childId : children)
        {
            AZ::Entity* childEntity = nullptr;

            AZ::ComponentApplicationBus::BroadcastResult(childEntity, &AZ::ComponentApplicationBus::Events::FindEntity, childId);

            if (!childEntity)
            {
                continue;
            }

            if (auto* articulatedComponent = childEntity->FindComponent<EditorArticulatedBodyComponent>())
            {
                articulatedComponent->UpdateArticulationHierarchy();
                m_articulationLinkData.m_childLinks.emplace_back(
                    AZStd::make_shared<ArticulationLinkData>(articulatedComponent->m_articulationLinkData));
            }
        }

        EditorColliderComponent* collider = GetEntity()->FindComponent<EditorColliderComponent>();
        if (collider)
        {
            const EditorProxyShapeConfig& shapeConfigProxy = collider->GetShapeConfiguration();
            const Physics::ColliderConfiguration& colliderConfig = collider->GetColliderConfiguration();

            m_articulationLinkData.m_colliderConfiguration = colliderConfig;
            m_articulationLinkData.m_shapeConfiguration = shapeConfigProxy.CloneCurrent();
            m_articulationLinkData.m_entityId = GetEntity()->GetId();

            m_articulationLinkData.m_config = m_config; //!< Generic properties from AzPhysics.
            m_articulationLinkData.m_physxSpecificConfig = m_physxSpecificConfig; 
            // m_linkData.m_genericProperties = m_jointConfig;
            // m_linkData.m_limits;
            // m_linkData.m_motor;
        }
    }

} // namespace PhysX
