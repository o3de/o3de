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

namespace PhysX
{
    void EditorArticulationLinkConfiguration::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorArticulationLinkConfiguration, ArticulationLinkConfiguration>()->Version(1);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<ArticulationLinkConfiguration>("PhysX Articulation Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Articulation")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    //->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_initialLinearVelocity,
                        "Initial linear velocity",
                        "Linear velocity applied when the rigid body is activated.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::GetInitialVelocitiesVisibility)
                    //->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetSpeedUnit())
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_initialAngularVelocity,
                        "Initial angular velocity",
                        "Angular velocity applied when the rigid body is activated (limited by maximum angular velocity).")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::GetInitialVelocitiesVisibility)
                    //->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetAngularVelocityUnit())

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_linearDamping,
                        "Linear damping",
                        "The rate of decay over time for linear velocity even if no forces are acting on the rigid body.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::GetDampingVisibility)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_angularDamping,
                        "Angular damping",
                        "The rate of decay over time for angular velocity even if no forces are acting on the rigid body.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::GetDampingVisibility)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_sleepMinEnergy,
                        "Sleep threshold",
                        "The rigid body can go to sleep (settle) when kinetic energy per unit mass is persistently below this value.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::GetSleepOptionsVisibility)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    //->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetSleepThresholdUnit())
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_startAsleep,
                        "Start asleep",
                        "When active, the rigid body will be asleep when spawned, and wake when the body is disturbed.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::GetSleepOptionsVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_interpolateMotion,
                        "Interpolate motion",
                        "When active, simulation results are interpolated resulting in smoother motion.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::GetInterpolationVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_gravityEnabled,
                        "Gravity enabled",
                        "When active, global gravity affects this rigid body.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::GetGravityVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox,
                        &ArticulationLinkConfiguration::m_kinematic,
                        "Type",
                        "Determines how the movement/position of the rigid body is controlled.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::GetKinematicVisibility)
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &ArticulationLinkConfiguration::m_ccdEnabled)
                    //->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &ArticulationLinkConfiguration::GetKinematicTooltip)
                    ->Attribute(AZ_CRC_CE("EditButtonVisible"), true)
                    ->Attribute(AZ_CRC_CE("SetTrueLabel"), "Kinematic")
                    ->Attribute(AZ_CRC_CE("SetFalseLabel"), "Simulated")
                    //->Attribute(AZ_CRC_CE("EditButtonCallback"), AzToolsFramework::GenericEditButtonCallback<bool>(&OnEditButtonClicked))
                    ->Attribute(AZ_CRC_CE("EditButtonToolTip"), "Open Type dialog for a detailed description on the motion types")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)

                    // Linear axis locking properties
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Linear Axis Locking")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_lockLinearX,
                        "Lock X",
                        "When active, forces won't create translation on the X axis of the rigid body.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_lockLinearY,
                        "Lock Y",
                        "When active, forces won't create translation on the Y axis of the rigid body.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_lockLinearZ,
                        "Lock Z",
                        "When active, forces won't create translation on the Z axis of the rigid body.")

                    // Angular axis locking properties
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Angular Axis Locking")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_lockAngularX,
                        "Lock X",
                        "When active, forces won't create rotation on the X axis of the rigid body.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_lockAngularY,
                        "Lock Y",
                        "When active, forces won't create rotation on the Y axis of the rigid body.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_lockAngularZ,
                        "Lock Z",
                        "When active, forces won't create rotation on the Z axis of the rigid body.")

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_maxAngularVelocity,
                        "Maximum angular velocity",
                        "Clamp angular velocities to this maximum value. "
                        "This prevents rigid bodies from rotating at unrealistic velocities after collisions.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::GetMaxVelocitiesVisibility)
                    //->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetAngularVelocityUnit())

                    // Mass properties
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_computeCenterOfMass,
                        "Compute COM",
                        "Compute the center of mass (COM) for this rigid body.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::GetInertiaSettingsVisibility)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_centerOfMassOffset,
                        "COM offset",
                        "Local space offset for the center of mass (COM).")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::GetCoMVisibility)
                    //->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetLengthUnit())

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_computeMass,
                        "Compute Mass",
                        "When active, the mass of the rigid body is computed based on the volume and density values of its colliders.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::GetInertiaSettingsVisibility)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_mass,
                        "Mass",
                        "The mass of the rigid body in kilograms. A value of 0 is treated as infinite. "
                        "The trajectory of infinite mass bodies cannot be affected by any collisions or forces other than gravity.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    //->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetMassUnit())
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::GetMassVisibility)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_computeInertiaTensor,
                        "Compute inertia",
                        "When active, inertia is computed based on the mass and shape of the rigid body.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::GetInertiaSettingsVisibility)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_includeAllShapesInMassCalculation,
                        "Include non-simulated shapes in Mass",
                        "When active, non-simulated shapes are included in the center of mass, inertia, and mass calculations.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::GetInertiaSettingsVisibility)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree);

                    editContext->Class<ArticulationLinkConfiguration>("PhysX Articulation Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Joint Configuration")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        0,
                        &PhysX::ArticulationLinkConfiguration::m_localPosition,
                        "Local Position",
                        "Local Position of joint, relative to its entity.")
                    ->DataElement(
                        0,
                        &PhysX::ArticulationLinkConfiguration::m_localRotation,
                        "Local Rotation",
                        "Local Rotation of joint, relative to its entity.")
                    /*                   ->Attribute(AZ::Edit::Attributes::Min, LocalRotationMin)
                                       ->Attribute(AZ::Edit::Attributes::Max, LocalRotationMax)*/
                    ->DataElement(
                        0, &PhysX::ArticulationLinkConfiguration::m_leadEntity, "Lead Entity", "Parent entity associated with joint.")
                    //->Attribute(AZ::Edit::Attributes::ChangeNotify, &ArticulationLinkConfiguration::ValidateLeadEntityId)
                    ->DataElement(
                        0,
                        &PhysX::ArticulationLinkConfiguration::m_selfCollide,
                        "Lead-Follower Collide",
                        "When active, the lead and follower pair will collide with each other.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox,
                        &PhysX::ArticulationLinkConfiguration::m_displayJointSetup,
                        "Display Setup in Viewport",
                        "Never = Not shown."
                        "Select = Show setup display when entity is selected."
                        "Always = Always show setup display.")
                    ->EnumAttribute(ArticulationLinkConfiguration::DisplaySetupState::Selected, "Selected")
                    ->EnumAttribute(ArticulationLinkConfiguration::DisplaySetupState::Never, "Never")
                    ->EnumAttribute(ArticulationLinkConfiguration::DisplaySetupState::Always, "Always")
                    //->Att(ribute(AZ::Edit::Attributes::ReadOnly, &ArticulationLinkConfiguration::IsInComponentMode)
                    ->DataElement(
                        0,
                        &PhysX::ArticulationLinkConfiguration::m_selectLeadOnSnap,
                        "Select Lead on Snap",
                        "Select lead entity on snap to position in component mode.")
                    ->DataElement(
                        0,
                        &PhysX::ArticulationLinkConfiguration::m_breakable,
                        "Breakable",
                        "Joint is breakable when force or torque exceeds limit.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    //->Attribute(AZ::Edit::Attributes::ReadOnly, &ArticulationLinkConfiguration::IsInComponentMode)
                    ->DataElement(
                        0,
                        &PhysX::ArticulationLinkConfiguration::m_forceMax,
                        "Maximum Force",
                        "Amount of force joint can withstand before breakage.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::m_breakable)
                    ->Attribute(AZ::Edit::Attributes::Max, s_breakageMax)
                    ->Attribute(AZ::Edit::Attributes::Min, s_breakageMin)
                    ->DataElement(
                        0,
                        &PhysX::ArticulationLinkConfiguration::m_torqueMax,
                        "Maximum Torque",
                        "Amount of torque joint can withstand before breakage.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::m_breakable)
                    ->Attribute(AZ::Edit::Attributes::Max, s_breakageMax)
                    ->Attribute(AZ::Edit::Attributes::Min, s_breakageMin)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_solverPositionIterations,
                        "Solver Position Iterations",
                        "Higher values can improve stability at the cost of performance.")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, 255)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_solverVelocityIterations,
                        "Solver Velocity Iterations",
                        "Higher values can improve stability at the cost of performance.")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, 255)

                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox,
                        &ArticulationLinkConfiguration::m_articulationJointType,
                        "Articulation LinkJoint Type",
                        "Set the type of joint for this link")
                    ->EnumAttribute(physx::PxArticulationJointType::Enum::eFIX, "Fix")
                    ->EnumAttribute(physx::PxArticulationJointType::Enum::ePRISMATIC, "Prismatic")
                    ->EnumAttribute(physx::PxArticulationJointType::Enum::eSPHERICAL, "Spherical")
                    ->EnumAttribute(physx::PxArticulationJointType::Enum::eREVOLUTE, "Revolute")
                    ->EnumAttribute(physx::PxArticulationJointType::Enum::eREVOLUTE_UNWRAPPED, "Revolute Unrwrapped")

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_isRootArticulation,
                        "Root articulation",
                        "Higher values can improve stability at the cost of performance.");
            }
        }
    }
    EditorArticulationLinkComponent::EditorArticulationLinkComponent(const EditorArticulationLinkConfiguration& configuration)
        : m_config(configuration)
    {
        SetIsRootArticulation();
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
                    //->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "")

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorArticulationLinkComponent::m_config,
                        "Articulation Configuration",
                        "Configuration for the Articulation Link Component.")
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

    void EditorArticulationLinkComponent::SetIsRootArticulation()
    {
        m_config.SetIsRootArticulation(IsRootArticulation());
    }

    void EditorArticulationLinkComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
    }

    void EditorArticulationLinkComponent::Deactivate()
    {
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
    }

    void EditorArticulationLinkComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<ArticulationLinkComponent>(m_config, AzPhysics::InvalidSceneHandle);
    }
} // namespace PhysX
