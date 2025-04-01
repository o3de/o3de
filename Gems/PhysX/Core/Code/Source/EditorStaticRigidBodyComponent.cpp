/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>

#include <AzFramework/Physics/NameConstants.h>

#include <Source/EditorStaticRigidBodyComponent.h>
#include <Source/StaticRigidBodyComponent.h>

namespace PhysX
{
    void EditorStaticRigidBodyComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorStaticRigidBodyComponent, AzToolsFramework::Components::EditorComponentBase>()->Version(1);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                constexpr const char* ToolTip = "The entity behaves as a non-movable rigid body in PhysX.";
                constexpr const char* HelpPageURL = "https://o3de.org/docs/user-guide/components/reference/physx/static-rigid-body/";

                editContext->Class<EditorStaticRigidBodyComponent>("PhysX Static Rigid Body", ToolTip)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/PhysXStaticRigidBody.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/PhysXStaticRigidBody.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, HelpPageURL)
                    ->UIElement(AZ::Edit::UIHandlers::Label, "", ToolTip)
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(
                        AZ::Edit::Attributes::ValueText,
                        Physics::NameConstants::GetBulletPoint() + " This component is empty and has no modifiable properties.<br>" +
                        Physics::NameConstants::GetBulletPoint() + " Its purpose is to be a non-movable rigid body in PhysX. <a href=\"" + HelpPageURL + "\">Read more</a>");
            }
        }
    }

    void EditorStaticRigidBodyComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
        provided.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
        provided.push_back(AZ_CRC_CE("PhysicsStaticRigidBodyService"));
    }

    void EditorStaticRigidBodyComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
    }

    void EditorStaticRigidBodyComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void EditorStaticRigidBodyComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void EditorStaticRigidBodyComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<StaticRigidBodyComponent>();
    }
} // namespace PhysX
