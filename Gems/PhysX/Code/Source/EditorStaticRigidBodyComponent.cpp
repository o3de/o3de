/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>

#include <Source/StaticRigidBodyComponent.h>
#include <Source/EditorStaticRigidBodyComponent.h>

namespace PhysX
{
    void EditorStaticRigidBodyComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorStaticRigidBodyComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                constexpr const char* ToolTip = "The entity behaves as a non-movable rigid body in PhysX.";

                editContext
                    ->Class<EditorStaticRigidBodyComponent>("PhysX Static Rigid Body", ToolTip)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/PhysXStaticRigidBody.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/PhysXStaticRigidBody.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(
                        AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/physx/static-rigid-body/")
                    ->UIElement(AZ::Edit::UIHandlers::MultiLineEdit, "", ToolTip)
                    ->Attribute(AZ::Edit::Attributes::ValueText,"<i>Component properties not required</i>")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                    ->UIElement(AZ::Edit::UIHandlers::MultiLineEdit, "", ToolTip)
                    ->Attribute(AZ::Edit::Attributes::ValueText, "Non-movable rigid body in PhysX")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, true);
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
