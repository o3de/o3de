/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

#include <AzFramework/Physics/NameConstants.h>
#include <AzFramework/Translation/TranslationDef.h>
#include <Editor/EditorJointCommon.h>
#include <Source/ArticulationLinkComponent.h>
#include <Source/EditorArticulationLinkComponent.h>
#include <Source/EditorColliderComponent.h>

namespace PhysX
{
    namespace
    {
        const float LocalRotationMax = 360.0f;
        const float LocalRotationMin = -360.0f;
    } // namespace

    void EditorArticulationLinkConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorArticulationLinkConfiguration, ArticulationLinkConfiguration>()->Version(2);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ArticulationLinkConfiguration>(QT_TRANSLATE_NOOP("PhysX", "PhysX Articulation Configuration"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Articulation configuration")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->UIElement(AZ::Edit::UIHandlers::Label, QT_TRANSLATE_NOOP("PhysX", "<b>Root Link</b>"))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::m_isRootArticulation)
                    ->UIElement(AZ::Edit::UIHandlers::Label, QT_TRANSLATE_NOOP("PhysX", "<b>Child Link</b>"))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::IsNotRootArticulation)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_isFixedBase,
                        QT_TRANSLATE_NOOP("PhysX", "Fixed Base"),
                        QT_TRANSLATE_NOOP("PhysX", "When active, the root articulation is fixed."))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::m_isRootArticulation)

                    ->DataElement(
                        0,
                        &ArticulationLinkConfiguration::m_selfCollide,
                        QT_TRANSLATE_NOOP("PhysX", "Self Collide"),
                        QT_TRANSLATE_NOOP("PhysX", "Enable collisions between the articulation's links (note that parent/child collisions are disabled internally in either case)."))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::m_isRootArticulation)

                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("PhysX", "Rigid Body configuration"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_gravityEnabled,
                        QT_TRANSLATE_NOOP("PhysX", "Gravity enabled"),
                        QT_TRANSLATE_NOOP("PhysX", "When active, global gravity affects this rigid body."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_mass,
                        QT_TRANSLATE_NOOP("PhysX", "Mass"),
                        QT_TRANSLATE_NOOP("PhysX", "The mass of the rigid body in kilograms. A value of 0 is treated as infinite. "
                        "The trajectory of infinite mass bodies cannot be affected by any collisions or forces other than gravity."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetMassUnit())
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_centerOfMassOffset,
                        QT_TRANSLATE_NOOP("PhysX", "COM offset"),
                        QT_TRANSLATE_NOOP("PhysX", "Local space offset for the center of mass (COM)."))
                    ->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetLengthUnit())
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_linearDamping,
                        QT_TRANSLATE_NOOP("PhysX", "Linear damping"),
                        QT_TRANSLATE_NOOP("PhysX", "The rate of decay over time for linear velocity even if no forces are acting on the rigid body."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_angularDamping,
                        QT_TRANSLATE_NOOP("PhysX", "Angular damping"),
                        QT_TRANSLATE_NOOP("PhysX", "The rate of decay over time for angular velocity even if no forces are acting on the rigid body."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_sleepMinEnergy,
                        QT_TRANSLATE_NOOP("PhysX", "Sleep threshold"),
                        QT_TRANSLATE_NOOP("PhysX", "The rigid body can go to sleep (settle) when kinetic energy per unit mass is persistently below this value."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetSleepThresholdUnit())
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::m_isRootArticulation)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_startAsleep,
                        QT_TRANSLATE_NOOP("PhysX", "Start asleep"),
                        QT_TRANSLATE_NOOP("PhysX", "When active, the rigid body will be asleep when spawned, and wake when the body is disturbed."))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::m_isRootArticulation)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_maxAngularVelocity,
                        QT_TRANSLATE_NOOP("PhysX", "Maximum angular velocity"),
                        QT_TRANSLATE_NOOP("PhysX", "Clamp angular velocities to this maximum value. "
                        "This prevents rigid bodies from rotating at unrealistic velocities after collisions."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetAngularVelocityUnit())
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_solverPositionIterations,
                        QT_TRANSLATE_NOOP("PhysX", "Solver Position Iterations"),
                        QT_TRANSLATE_NOOP("PhysX", "Higher values can improve stability at the cost of performance."))
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, 255)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::m_isRootArticulation)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &ArticulationLinkConfiguration::m_solverVelocityIterations,
                        QT_TRANSLATE_NOOP("PhysX", "Solver Velocity Iterations"),
                        QT_TRANSLATE_NOOP("PhysX", "Higher values can improve stability at the cost of performance."))
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, 255)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::m_isRootArticulation)
                    ->EndGroup()

                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("PhysX", "Joint configuration"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox,
                        &ArticulationLinkConfiguration::m_articulationJointType,
                        QT_TRANSLATE_NOOP("PhysX", "Joint Type"),
                        QT_TRANSLATE_NOOP("PhysX", "Set the type of joint for this link"))
                    ->EnumAttribute(ArticulationJointType::Fix, QT_TRANSLATE_NOOP("PhysX", "Fix"))
                    ->EnumAttribute(ArticulationJointType::Hinge, QT_TRANSLATE_NOOP("PhysX", "Hinge"))
                    ->EnumAttribute(ArticulationJointType::Prismatic, QT_TRANSLATE_NOOP("PhysX", "Prismatic"))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::IsNotRootArticulation)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(
                        0,
                        &PhysX::ArticulationLinkConfiguration::m_localPosition,
                        QT_TRANSLATE_NOOP("PhysX", "Local Position"),
                        QT_TRANSLATE_NOOP("PhysX", "Local Position of joint, relative to its entity."))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::IsNotRootArticulation)
                    ->DataElement(
                        0,
                        &PhysX::ArticulationLinkConfiguration::m_localRotation,
                        QT_TRANSLATE_NOOP("PhysX", "Local Rotation"),
                        QT_TRANSLATE_NOOP("PhysX", "Local Rotation of joint, relative to its entity."))
                    ->Attribute(AZ::Edit::Attributes::Min, LocalRotationMin)
                    ->Attribute(AZ::Edit::Attributes::Max, LocalRotationMax)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::IsNotRootArticulation)
                    ->DataElement(
                        0,
                        &ArticulationLinkConfiguration::m_fixJointLocation,
                        QT_TRANSLATE_NOOP("PhysX", "Fix Joint Location"),
                        QT_TRANSLATE_NOOP("PhysX", "When enabled the joint will remain in the same location when moving the entity."))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::IsNotRootArticulation)

                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox,
                        &ArticulationLinkConfiguration::m_displayJointSetup,
                        QT_TRANSLATE_NOOP("PhysX", "Display Setup in Viewport"),
                        QT_TRANSLATE_NOOP("PhysX", "Never = Not shown."
                        "Select = Show setup display when entity is selected."
                        "Always = Always show setup display."))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::IsNotRootArticulation)
                    ->EnumAttribute(ArticulationLinkConfiguration::DisplaySetupState::Never, QT_TRANSLATE_NOOP("PhysX", "Never"))
                    ->EnumAttribute(ArticulationLinkConfiguration::DisplaySetupState::Selected, QT_TRANSLATE_NOOP("PhysX", "Selected"))
                    ->EnumAttribute(ArticulationLinkConfiguration::DisplaySetupState::Always, QT_TRANSLATE_NOOP("PhysX", "Always"))

                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("PhysX", "Joint limits"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        0, &ArticulationLinkConfiguration::m_isLimited, QT_TRANSLATE_NOOP("PhysX", "Limit"), QT_TRANSLATE_NOOP("PhysX", "When active, the joint's degrees of freedom are limited."))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::IsSingleDofJointType)
                    ->DataElement(
                        0, &ArticulationLinkConfiguration::m_linearLimitLower, QT_TRANSLATE_NOOP("PhysX", "Lower Linear Limit"), QT_TRANSLATE_NOOP("PhysX", "Lower limit of linear motion."))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::PrismaticPropertiesVisible)
                    ->DataElement(
                        0, &ArticulationLinkConfiguration::m_linearLimitUpper, QT_TRANSLATE_NOOP("PhysX", "Upper Linear Limit"), QT_TRANSLATE_NOOP("PhysX", "Upper limit for linear motion."))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::PrismaticPropertiesVisible)
                    ->DataElement(
                        0, &ArticulationLinkConfiguration::m_angularLimitNegative, QT_TRANSLATE_NOOP("PhysX", "Lower Angular Limit"), QT_TRANSLATE_NOOP("PhysX", "Lower limit of angular motion."))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::HingePropertiesVisible)
                    ->DataElement(
                        0, &ArticulationLinkConfiguration::m_angularLimitPositive, QT_TRANSLATE_NOOP("PhysX", "Upper Angular Limit"), QT_TRANSLATE_NOOP("PhysX", "Upper limit of angular motion."))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::HingePropertiesVisible)
                    ->EndGroup()

                    ->DataElement(
                        0, &ArticulationLinkConfiguration::m_motorConfiguration, QT_TRANSLATE_NOOP("PhysX", "Motor Configuration"), QT_TRANSLATE_NOOP("PhysX", "Joint's motor configuration."))
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::IsSingleDofJointType)

                    ->DataElement(0, &ArticulationLinkConfiguration::m_jointFriction, QT_TRANSLATE_NOOP("PhysX", "Joint Friction"), QT_TRANSLATE_NOOP("PhysX", "Joint's friction coefficient."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::IsSingleDofJointType)

                    ->DataElement(0, &ArticulationLinkConfiguration::m_armature, QT_TRANSLATE_NOOP("PhysX", "Armature"), QT_TRANSLATE_NOOP("PhysX", "Mass for prismatic joints, inertia for hinge"))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::IsSingleDofJointType)

                    ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("PhysX", "Sensors"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &ArticulationLinkConfiguration::m_sensorConfigs, "Sensor Configurations", "Sensor configurations")
                    ->EndGroup()
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Joint Offset")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                           AZ::Edit::UIHandlers::Default,
                           &ArticulationLinkConfiguration::m_offset,
                           "Offset",
                           "Zero-position offset for the joint. For hinge joints the value is in degrees; "
                           "for prismatic joints the value is in meters. Limits, drive targets, and reported "
                           "positions are all expressed relative to this offset.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ArticulationLinkConfiguration::IsSingleDofJointType)
                    ->EndGroup();
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
                AZStd::vector<AZ::Crc32> componentMenus;
                if (ReducedCoordinateArticulationsEnabled())
                {
                    componentMenus.push_back(AZ::Crc32("Game"));
                }

                editContext->Class<EditorArticulationLinkComponent>(QT_TRANSLATE_NOOP("PhysX", "PhysX Articulation Link"), QT_TRANSLATE_NOOP("PhysX", "Articulated rigid body."))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/PhysXRigidBody.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/PhysXRigidBody.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, componentMenus)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "")

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorArticulationLinkComponent::m_config,
                        QT_TRANSLATE_NOOP("PhysX", "Articulation Configuration"),
                        QT_TRANSLATE_NOOP("PhysX", "Configuration for the Articulation Link Component."))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }

    void EditorArticulationLinkComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
        provided.push_back(AZ_CRC_CE("PhysicsDynamicRigidBodyService"));
        provided.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
        provided.push_back(AZ_CRC_CE("ArticulationLinkService"));
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

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
    }

    void EditorArticulationLinkComponent::Deactivate()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();

        AzToolsFramework::Components::EditorComponentBase::Deactivate();
    }

    void EditorArticulationLinkComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<ArticulationLinkComponent>(m_config);
    }

    void EditorArticulationLinkComponent::OnTransformChanged([[maybe_unused]] const AZ::Transform& localTM, const AZ::Transform& worldTM)
    {
        if (m_config.m_fixJointLocation)
        {
            const AZ::Transform localJoint = AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateFromEulerAnglesDegrees(m_config.m_localRotation), m_config.m_localPosition);
            const AZ::Transform worldJoint = m_cachedWorldTM * localJoint;

            const AZ::Transform localFromWorld = worldTM.GetInverse();
            const AZ::Transform newLocalJoint = localFromWorld * worldJoint;
            m_config.m_localPosition = newLocalJoint.GetTranslation();
            m_config.m_localRotation = newLocalJoint.GetEulerDegrees();

            InvalidatePropertyDisplay(AzToolsFramework::Refresh_Values);
        }
        m_cachedWorldTM = worldTM;
    }

    void EditorArticulationLinkComponent::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        // The root articulation doesn't have a joint.
        if (IsRootArticulation())
        {
            return;
        }

        ShowJointHierarchy(viewportInfo, debugDisplay);

        if (!ShowSetupDisplay())
        {
            return;
        }

        switch (m_config.m_articulationJointType)
        {
        case ArticulationJointType::Hinge:
            ShowHingeJoint(viewportInfo, debugDisplay);
            break;

        case ArticulationJointType::Prismatic:
            ShowPrismaticJoint(viewportInfo, debugDisplay);
            break;

        default:
            // Nothing to show
            break;
        }
    }

    bool EditorArticulationLinkComponent::ShowSetupDisplay() const
    {
        switch (m_config.m_displayJointSetup)
        {
        case ArticulationLinkConfiguration::DisplaySetupState::Always:
            return true;
        case ArticulationLinkConfiguration::DisplaySetupState::Selected:
            {
                bool showSetup = false;
                AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                    showSetup, GetEntityId(), &AzToolsFramework::EditorEntityInfoRequests::IsSelected);
                return showSetup;
            }
        }
        return false;
    }

    void EditorArticulationLinkComponent::ShowJointHierarchy(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        auto* physXDebug = AZ::Interface<Debug::PhysXDebugInterface>::Get();
        if (physXDebug == nullptr)
        {
            return;
        }

        const PhysX::Debug::DebugDisplayData& displayData = physXDebug->GetDebugDisplayData();
        if (displayData.m_showJointHierarchy)
        {
            const AZ::Color leadLineColor = displayData.GetJointLeadColor();
            const AZ::Color followerLineColor = displayData.GetJointFollowerColor();

            const AZ::Transform followerWorldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(GetEntityId());
            const AZ::Vector3 followerWorldPosition = followerWorldTransform.GetTranslation();

            const AZ::Transform jointLocalTransform = AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateFromEulerAnglesDegrees(m_config.m_localRotation),
                m_config.m_localPosition);
            const AZ::Vector3 jointWorldPosition = PhysX::Utils::ComputeJointWorldTransform(jointLocalTransform, followerWorldTransform).GetTranslation();

            const float distance = followerWorldPosition.GetDistance(jointWorldPosition);

            const float lineWidth = 4.0f;

            AZ::u32 stateBefore = debugDisplay.GetState();
            debugDisplay.DepthTestOff();
            debugDisplay.SetColor(leadLineColor);
            debugDisplay.SetLineWidth(lineWidth);

            if (distance < displayData.m_jointHierarchyDistanceThreshold)
            {
                const AZ::Transform leadWorldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(GetEntity()->GetTransform()->GetParentId());
                const AZ::Vector3 leadWorldPosition = leadWorldTransform.GetTranslation();

                const AZ::Vector3 midPoint = (jointWorldPosition + leadWorldPosition) * 0.5f;

                debugDisplay.DrawLine(jointWorldPosition, midPoint);
                debugDisplay.SetColor(followerLineColor);
                debugDisplay.DrawLine(midPoint, leadWorldPosition);
            }
            else
            {
                const AZ::Vector3 midPoint = (jointWorldPosition + followerWorldPosition) * 0.5f;

                debugDisplay.DrawLine(jointWorldPosition, midPoint);
                debugDisplay.SetColor(followerLineColor);
                debugDisplay.DrawLine(midPoint, followerWorldPosition);
            }

            debugDisplay.SetState(stateBefore);
        }
    }

    void EditorArticulationLinkComponent::ShowHingeJoint(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        const float s_alpha = 0.6f;
        const AZ::Color s_colorDefault = AZ::Color(1.0f, 1.0f, 1.0f, s_alpha);
        const AZ::Color s_colorFirst = AZ::Color(1.0f, 0.0f, 0.0f, s_alpha);
        const AZ::Color s_colorSecond = AZ::Color(0.0f, 1.0f, 0.0f, s_alpha);
        const AZ::Color s_colorSweepArc = AZ::Color(1.0f, 1.0f, 1.0f, s_alpha);

        AngleLimitsFloatPair currentValue(m_config.m_angularLimitPositive, m_config.m_angularLimitNegative);
        AZ::Vector3 axis = AZ::Vector3::CreateAxisX();

        const AZ::EntityId& entityId = GetEntityId();
        const AZ::Transform jointLocalTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateFromEulerAnglesDegrees(m_config.m_localRotation), m_config.m_localPosition);
        const AZ::Transform jointWorldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(entityId) * jointLocalTransform;
        const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);

        // scaleMultiply will represent a scale for the debug draw that makes it remain the same size on screen
        float scaleMultiply = AzToolsFramework::CalculateScreenToWorldMultiplier(jointWorldTransform.GetTranslation(), cameraState);

        const float size = 2.0f * scaleMultiply;

        AZ::u32 stateBefore = debugDisplay.GetState();
        debugDisplay.CullOff();
        debugDisplay.SetAlpha(s_alpha);

        debugDisplay.PushMatrix(jointWorldTransform);

        // draw a cylinder to indicate the axis of revolution.
        const float cylinderThickness = 0.05f * scaleMultiply;
        debugDisplay.SetColor(s_colorFirst);
        debugDisplay.DrawSolidCylinder(AZ::Vector3::CreateZero(), AZ::Vector3::CreateAxisX(), cylinderThickness, size, true);

        if (m_config.m_isLimited)
        {
            // if we are angularly limited, then show the limits, with an arc between them:
            AZ::Vector3 axisPoint = axis * size * 0.5f;

            AZ::Vector3 points[4] = { -axisPoint, axisPoint, axisPoint, -axisPoint };

            if (axis == AZ::Vector3::CreateAxisX())
            {
                points[2].SetZ(size);
                points[3].SetZ(size);
            }
            else if (axis == AZ::Vector3::CreateAxisY())
            {
                points[2].SetX(size);
                points[3].SetX(size);
            }
            else if (axis == AZ::Vector3::CreateAxisZ())
            {
                points[2].SetX(size);
                points[3].SetX(size);
            }

            debugDisplay.SetColor(s_colorSweepArc);
            const float sweepLineDisplaceFactor = 0.5f;
            const float sweepLineThickness = 1.0f * scaleMultiply;
            const float sweepLineGranularity = 1.0f;
            const AZ::Vector3 zeroVector = AZ::Vector3::CreateZero();
            const AZ::Vector3 posPosition = axis * sweepLineDisplaceFactor * scaleMultiply;
            const AZ::Vector3 negPosition = -posPosition;
            debugDisplay.DrawArc(posPosition, sweepLineThickness, -currentValue.first, currentValue.first, sweepLineGranularity, -axis);
            debugDisplay.DrawArc(zeroVector, sweepLineThickness, -currentValue.first, currentValue.first, sweepLineGranularity, -axis);
            debugDisplay.DrawArc(negPosition, sweepLineThickness, -currentValue.first, currentValue.first, sweepLineGranularity, -axis);
            debugDisplay.DrawArc(posPosition, sweepLineThickness, 0.0f, abs(currentValue.second), sweepLineGranularity, -axis);
            debugDisplay.DrawArc(zeroVector, sweepLineThickness, 0.0f, abs(currentValue.second), sweepLineGranularity, -axis);
            debugDisplay.DrawArc(negPosition, sweepLineThickness, 0.0f, abs(currentValue.second), sweepLineGranularity, -axis);

            AZ::Quaternion firstRotate = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::DegToRad(currentValue.first));
            AZ::Transform firstTM = AZ::Transform::CreateFromQuaternion(firstRotate);
            debugDisplay.PushMatrix(firstTM);
            debugDisplay.SetColor(s_colorFirst);
            debugDisplay.DrawQuad(points[0], points[1], points[2], points[3]);
            debugDisplay.PopMatrix();

            AZ::Quaternion secondRotate = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::DegToRad(currentValue.second));
            AZ::Transform secondTM = AZ::Transform::CreateFromQuaternion(secondRotate);
            debugDisplay.PushMatrix(secondTM);
            debugDisplay.SetColor(s_colorSecond);
            debugDisplay.DrawQuad(points[0], points[1], points[2], points[3]);
            debugDisplay.PopMatrix();

            debugDisplay.SetColor(s_colorDefault);
            debugDisplay.DrawQuad(points[0], points[1], points[2], points[3]);
        }
        else // if we are not limited, show direction of revolve instead
        {
            debugDisplay.SetColor(s_colorSweepArc);
            const float circleRadius = 0.6f * scaleMultiply;
            const float coneRadius = 0.05 * scaleMultiply;
            const float coneHeight = 0.2f * scaleMultiply;
            debugDisplay.DrawCircle(AZ::Vector3::CreateZero(), 1.0f * circleRadius, 0);
            // show tick-marks on the revolve axis that indicate the positive direction of revolution
            AZ::Vector3 pointOnCircle = circleRadius * AZ::Vector3::CreateAxisY();
            debugDisplay.DrawWireCone(pointOnCircle, -AZ::Vector3::CreateAxisZ(), coneRadius, coneHeight);
            pointOnCircle = -circleRadius * AZ::Vector3::CreateAxisY();
            debugDisplay.DrawWireCone(pointOnCircle, AZ::Vector3::CreateAxisZ(), coneRadius, coneHeight);

            pointOnCircle = circleRadius * AZ::Vector3::CreateAxisZ();
            debugDisplay.DrawWireCone(pointOnCircle, AZ::Vector3::CreateAxisY(), coneRadius, coneHeight);
            pointOnCircle = -circleRadius * AZ::Vector3::CreateAxisZ();
            debugDisplay.DrawWireCone(pointOnCircle, -AZ::Vector3::CreateAxisY(), coneRadius, coneHeight);
        }

        debugDisplay.PopMatrix(); // pop joint world transform
        debugDisplay.SetState(stateBefore);
    }

    void EditorArticulationLinkComponent::ShowPrismaticJoint(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        const float alpha = 0.6f;
        const AZ::Color colorDefault = AZ::Color(1.0f, 1.0f, 1.0f, alpha);
        const AZ::Color colorLimitLower = AZ::Color(1.0f, 0.0f, 0.0f, alpha);
        const AZ::Color colorLimitUpper = AZ::Color(0.0f, 1.0f, 0.0f, alpha);

        AZ::u32 stateBefore = debugDisplay.GetState();
        debugDisplay.CullOff();
        debugDisplay.SetAlpha(alpha);

        const AZ::EntityId& entityId = GetEntityId();

        const AZ::Transform jointLocalTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateFromEulerAnglesDegrees(m_config.m_localRotation), m_config.m_localPosition);
        const AZ::Transform jointWorldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(entityId) * jointLocalTransform;

        const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);
        // scaleMultiply will represent a scale for the debug draw that makes it remain the same size on screen
        float scaleMultiply = AzToolsFramework::CalculateScreenToWorldMultiplier(jointWorldTransform.GetTranslation(), cameraState);

        const float size = 1.0f * scaleMultiply;

        debugDisplay.PushMatrix(jointWorldTransform);

        debugDisplay.SetColor(colorDefault);
        debugDisplay.DrawLine(AZ::Vector3::CreateAxisX(m_config.m_linearLimitLower), AZ::Vector3::CreateAxisX(m_config.m_linearLimitUpper));

        debugDisplay.SetColor(colorLimitLower);
        debugDisplay.DrawQuad(
            AZ::Vector3(m_config.m_linearLimitLower, -size, -size),
            AZ::Vector3(m_config.m_linearLimitLower, -size, size),
            AZ::Vector3(m_config.m_linearLimitLower, size, size),
            AZ::Vector3(m_config.m_linearLimitLower, size, -size));

        debugDisplay.SetColor(colorLimitUpper);
        debugDisplay.DrawQuad(
            AZ::Vector3(m_config.m_linearLimitUpper, -size, -size),
            AZ::Vector3(m_config.m_linearLimitUpper, -size, size),
            AZ::Vector3(m_config.m_linearLimitUpper, size, size),
            AZ::Vector3(m_config.m_linearLimitUpper, size, -size));

        debugDisplay.PopMatrix(); // pop joint world transform
        debugDisplay.SetState(stateBefore);
    }

} // namespace PhysX
