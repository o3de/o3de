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

namespace PhysX
{
    void EditorArticulatedBodyComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorArticulatedBodyComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                constexpr const char* ToolTip = "The entity behaves as a non-movable rigid body in PhysX.";

                editContext
                    ->Class<EditorArticulatedBodyComponent>("PhysX Articulated Rigid Body", ToolTip)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/PhysXStaticRigidBody.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/PhysXStaticRigidBody.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(
                        AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/physx/static-rigid-body/")
                    ->UIElement(AZ::Edit::UIHandlers::Label, "", ToolTip)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->Attribute(AZ::Edit::Attributes::ValueText, "<i>Component properties not required</i><br>Non-movable rigid body in PhysX")
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

    void EditorArticulatedBodyComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        AZ::TransformInterface* thisTransform = GetEntity()->GetTransform();
        if (thisTransform)
        {
            bool isRootArticulation = true;
            {
                AZ::EntityId parentId = thisTransform->GetParentId();
                if (parentId.IsValid())
                {
                    AZ::Entity* parentEntity = nullptr;

                    AZ::ComponentApplicationBus::BroadcastResult(
                        parentEntity, &AZ::ComponentApplicationBus::Events::FindEntity, parentId);

                    if (parentEntity && parentEntity->FindComponent<EditorArticulatedBodyComponent>())
                    {
                        isRootArticulation = false;
                    }
                }
            }

            AZStd::vector<AZ::EntityId> children = GetEntity()->GetTransform()->GetChildren();


            EditorColliderComponent* collider = GetEntity()->FindComponent<EditorColliderComponent>();

            const EditorProxyShapeConfig& shapeConfigProxy = collider->GetShapeConfiguration();
            const Physics::ColliderConfiguration& colliderConfig = collider->GetColliderConfiguration();

            ArticulationLinkData thisLink;
            thisLink.m_colliderConfiguration = colliderConfig;
            thisLink.m_shapeConfiguration = shapeConfigProxy.CloneCurrent();

            ArticulatedBodyComponent* component = gameEntity->CreateComponent<ArticulatedBodyComponent>();

            component->m_articulationLinkData.m_colliderConfiguration;
            component->m_articulationLinkData.m_shapeConfiguration;

            component->m_articulationLinkData.m_childLinks.emplace_back(thisLink);
        }
        else
        {
            gameEntity->CreateComponent<ArticulatedBodyComponent>();
        }
    }
} // namespace PhysX
