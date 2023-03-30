/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

#include <Source/ArticulationLinkComponent.h>
#include <Source/EditorArticulationLinkComponent.h>
#include <Source/EditorColliderComponent.h>
#include <AzFramework/Physics/NameConstants.h>

namespace PhysX
{
    namespace
    {
        const float LocalRotationMax = 360.0f;
        const float LocalRotationMin = -360.0f;
    }

    void EditorArticulationLinkConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorArticulationLinkConfiguration, ArticulationLinkConfiguration>()->Version(2);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ArticulationLinkConfiguration>("PhysX Articulation Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Articulation configuration")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->UIElement(AZ::Edit::UIHandlers::Label, "<b>Root Link</b>")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::m_isRootArticulation)
                    ->UIElement(AZ::Edit::UIHandlers::Label, "<b>Child Link</b>")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::IsNotRootArticulation)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_isFixedBase,
                        "Fixed Base",
                        "When active, the root articulation is fixed.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::m_isRootArticulation)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Rigid Body configuration")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_gravityEnabled,
                        "Gravity enabled",
                        "When active, global gravity affects this rigid body.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::m_isRootArticulation)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_mass,
                        "Mass",
                        "The mass of the rigid body in kilograms. A value of 0 is treated as infinite. "
                        "The trajectory of infinite mass bodies cannot be affected by any collisions or forces other than gravity.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetMassUnit())
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_centerOfMassOffset,
                        "COM offset",
                        "Local space offset for the center of mass (COM).")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetLengthUnit())
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_linearDamping,
                        "Linear damping",
                        "The rate of decay over time for linear velocity even if no forces are acting on the rigid body.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_angularDamping,
                        "Angular damping",
                        "The rate of decay over time for angular velocity even if no forces are acting on the rigid body.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_sleepMinEnergy,
                        "Sleep threshold",
                        "The rigid body can go to sleep (settle) when kinetic energy per unit mass is persistently below this value.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetSleepThresholdUnit())
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::m_isRootArticulation)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_startAsleep,
                        "Start asleep",
                        "When active, the rigid body will be asleep when spawned, and wake when the body is disturbed.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::m_isRootArticulation)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_maxAngularVelocity,
                        "Maximum angular velocity",
                        "Clamp angular velocities to this maximum value. "
                        "This prevents rigid bodies from rotating at unrealistic velocities after collisions.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetAngularVelocityUnit())
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_solverPositionIterations,
                        "Solver Position Iterations",
                        "Higher values can improve stability at the cost of performance.")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, 255)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::m_isRootArticulation)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_solverVelocityIterations,
                        "Solver Velocity Iterations",
                        "Higher values can improve stability at the cost of performance.")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, 255)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::m_isRootArticulation)
                    ->EndGroup()

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Joint configuration")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox,
                        &ArticulationLinkConfiguration::m_articulationJointType,
                        "Joint Type",
                        "Set the type of joint for this link")
                    ->EnumAttribute(ArticulationJointType::Fix, "Fix")
                    ->EnumAttribute(ArticulationJointType::Hinge, "Hinge")
                    ->EnumAttribute(ArticulationJointType::Prismatic, "Prismatic")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::IsNotRootArticulation)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(
                        0,
                        &PhysX::ArticulationLinkConfiguration::m_localPosition,
                        "Local Position",
                        "Local Position of joint, relative to its entity.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::IsNotRootArticulation)
                    ->DataElement(
                        0,
                        &PhysX::ArticulationLinkConfiguration::m_localRotation,
                        "Local Rotation",
                        "Local Rotation of joint, relative to its entity.")
                    ->Attribute(AZ::Edit::Attributes::Min, LocalRotationMin)
                    ->Attribute(AZ::Edit::Attributes::Max, LocalRotationMax)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::IsNotRootArticulation)
                    ->DataElement(
                        0,
                        &ArticulationLinkConfiguration::m_fixJointLocation,
                        "Fix Joint Location",
                        "When enabled the joint will remain in the same location when moving the entity.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::IsNotRootArticulation)
                    ->DataElement(
                        0,
                        &ArticulationLinkConfiguration::m_selfCollide,
                        "Lead-Follower Collide",
                        "When active, the lead and follower pair will collide with each other.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::IsNotRootArticulation)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Joint limits")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        0, &ArticulationLinkConfiguration::m_isLimited, "Limit", "When active, the joint's degrees of freedom are limited.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::IsSingleDofJointType)
                    ->DataElement(
                        0, &ArticulationLinkConfiguration::m_linearLimitLower, "Lower Linear Limit", "Lower limit of linear motion.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::PrismaticPropertiesVisible)
                    ->DataElement(
                        0, &ArticulationLinkConfiguration::m_linearLimitUpper, "Upper Linear Limit", "Upper limit for linear motion.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::PrismaticPropertiesVisible)
                    ->DataElement(
                        0, &ArticulationLinkConfiguration::m_angularLimitNegative, "Lower Angular Limit", "Lower limit of angular motion..")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::HingePropertiesVisible)
                    ->DataElement(
                        0, &ArticulationLinkConfiguration::m_angularLimitPositive, "Upper Angular Limit", "Lower limit of angular motion.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::HingePropertiesVisible)
                    ->EndGroup()

                    ->DataElement(
                        0, &ArticulationLinkConfiguration::m_motorConfiguration, "Motor Configuration", "Joint's motor configuration.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::IsSingleDofJointType);
            }
        }
    }

    EditorArticulationLinkComponent::EditorArticulationLinkComponent(const EditorArticulationLinkConfiguration& configuration)
        : m_config(configuration)
    {
    }

    void EditorArticulationLinkComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorArticulationLinkConfiguration::Reflect(context);
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorArticulationLinkComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("ArticulationConfiguration", &EditorArticulationLinkComponent::m_config);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                constexpr const char* ToolTip = "Articulated rigid body.";

                editContext->Class<EditorArticulationLinkComponent>("PhysX Articulation Link", ToolTip)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/PhysXRigidBody.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/PhysXRigidBody.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "")

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorArticulationLinkComponent::m_config,
                        "Articulation Configuration",
                        "Configuration for the Articulation Link Component.")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }

    void EditorArticulationLinkComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
        provided.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
    }

    void EditorArticulationLinkComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
    }

    void EditorArticulationLinkComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void EditorArticulationLinkComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    bool EditorArticulationLinkComponent::IsRootArticulation() const
    {
        return IsRootArticulationEntity<EditorArticulationLinkComponent>(GetEntity());
    }

    void EditorArticulationLinkComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        m_config.m_isRootArticulation = IsRootArticulation();
    }

    void EditorArticulationLinkComponent::Deactivate()
    {
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
    }

    void EditorArticulationLinkComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<ArticulationLinkComponent>(m_config);
    }
} // namespace PhysX
