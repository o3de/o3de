/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PhysX_precompiled.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

#include <Source/EditorHingeJointComponent.h>
#include <Editor/EditorJointComponentMode.h>
#include <Source/HingeJointComponent.h>
#include <Source/Utils.h>

namespace PhysX
{
    void EditorHingeJointComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorHingeJointComponent, EditorJointComponent>()
                ->Version(2)
                ->Field("Angular Limit", &EditorHingeJointComponent::m_angularLimit)
                ->Field("Component Mode", &EditorHingeJointComponent::m_componentModeDelegate)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                  editContext->Class<EditorHingeJointComponent>(
                    "PhysX Hinge Joint", "The entity constrains two actors in PhysX, keeping the origins and x-axes together, and allows free rotation around this common axis")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &EditorHingeJointComponent::m_angularLimit, "Angular Limit", "Limitations for the rotation about hinge axis")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorHingeJointComponent::m_componentModeDelegate, "Component Mode", "Hinge Joint Component Mode")
                      ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    void EditorHingeJointComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("PhysXJointService", 0x0d2f906f));
    }

    void EditorHingeJointComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        required.push_back(AZ_CRC("PhysXColliderService", 0x4ff43f7c));
        required.push_back(AZ_CRC("PhysXRigidBodyService", 0x1d4c64a8));
    }

    void EditorHingeJointComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void EditorHingeJointComponent::Activate()
    {
        EditorJointComponent::Activate();

        const AZ::EntityId& entityId = GetEntityId();

        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler::BusConnect(entityId);

        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler* selection = this;
        m_componentModeDelegate.ConnectWithSingleComponentMode <
            EditorHingeJointComponent, EditorHingeJointComponentMode>(
                AZ::EntityComponentIdPair(entityId, GetId()), selection);

        PhysX::EditorJointRequestBus::Handler::BusConnect(AZ::EntityComponentIdPair(entityId, GetId()));
    }

    void EditorHingeJointComponent::Deactivate()
    {
        PhysX::EditorJointRequestBus::Handler::BusDisconnect();
        m_componentModeDelegate.Disconnect();
        AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
        EditorJointComponent::Deactivate();
    }

    void EditorHingeJointComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        m_config.m_followerEntity = GetEntityId(); // joint is always in the same entity as the follower body.
        gameEntity->CreateComponent<HingeJointComponent>(
            m_config.ToGameTimeConfig(), 
            m_angularLimit.ToGameTimeConfig());
    }

    float EditorHingeJointComponent::GetLinearValue(const AZStd::string& parameterName)
    {
        if (parameterName == PhysX::EditorJointComponentMode::s_parameterMaxForce)
        {
            return m_config.m_forceMax;
        }
        else if (parameterName == PhysX::EditorJointComponentMode::s_parameterMaxTorque)
        {
            return m_config.m_torqueMax;
        }
        else if (parameterName == PhysX::EditorJointComponentMode::s_parameterTolerance)
        {
            return m_angularLimit.m_standardLimitConfig.m_tolerance;
        }
        else if (parameterName == PhysX::EditorJointComponentMode::s_parameterDamping)
        {
            return m_angularLimit.m_standardLimitConfig.m_damping;
        }
        else if (parameterName == PhysX::EditorJointComponentMode::s_parameterStiffness)
        {
            return m_angularLimit.m_standardLimitConfig.m_stiffness;
        }

        return 0.0f;
    }

    AngleLimitsFloatPair EditorHingeJointComponent::GetLinearValuePair(const AZStd::string& parameterName)
    {
        if (parameterName == PhysX::EditorJointComponentMode::s_parameterAngularPair)
        {
            return AngleLimitsFloatPair(m_angularLimit.m_limitPositive, m_angularLimit.m_limitNegative);
        }

        return AngleLimitsFloatPair();
    }

    bool EditorHingeJointComponent::IsParameterUsed(const AZStd::string& parameterName)
    {
        if (parameterName == PhysX::EditorJointComponentMode::s_parameterMaxForce
            || parameterName == PhysX::EditorJointComponentMode::s_parameterMaxTorque
            )
        {
            return m_config.m_breakable;
        }
        else if (parameterName == PhysX::EditorJointComponentMode::s_parameterTolerance)
        {
            return !m_angularLimit.m_standardLimitConfig.m_isSoftLimit;
        }
        else if (parameterName == PhysX::EditorJointComponentMode::s_parameterDamping)
        {
            return m_angularLimit.m_standardLimitConfig.m_isSoftLimit;
        }
        else if (parameterName == PhysX::EditorJointComponentMode::s_parameterStiffness)
        {
            return m_angularLimit.m_standardLimitConfig.m_isSoftLimit;
        }

        return true; // Sub-component mode always enabled unless disabled explicitly.
    }

    void EditorHingeJointComponent::SetBoolValue(const AZStd::string& parameterName, bool value)
    {
        if (parameterName == PhysX::EditorJointComponentMode::s_parameterComponentMode)
        {
            m_angularLimit.m_standardLimitConfig.m_inComponentMode = value;
            m_config.m_inComponentMode = value;

            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay
                , AzToolsFramework::Refresh_EntireTree);
        }
        else if (parameterName == PhysX::EditorJointComponentMode::s_parameterSelectOnSnap)
        {
            m_config.m_selectLeadOnSnap = value;
        }
    }

    void EditorHingeJointComponent::SetLinearValue(const AZStd::string& parameterName, float value)
    {
        if (parameterName == PhysX::EditorJointComponentMode::s_parameterMaxForce)
        {
            m_config.m_forceMax = value;
        }
        else if (parameterName == PhysX::EditorJointComponentMode::s_parameterMaxTorque)
        {
            m_config.m_torqueMax = value;
        }
        else if (parameterName == PhysX::EditorJointComponentMode::s_parameterTolerance)
        {
            m_angularLimit.m_standardLimitConfig.m_tolerance = value;
        }
        else if (parameterName == PhysX::EditorJointComponentMode::s_parameterDamping)
        {
            m_angularLimit.m_standardLimitConfig.m_damping = value;
        }
        else if (parameterName == PhysX::EditorJointComponentMode::s_parameterStiffness)
        {
            m_angularLimit.m_standardLimitConfig.m_stiffness = value;
        }
    }

    void EditorHingeJointComponent::SetLinearValuePair(const AZStd::string& parameterName, const AngleLimitsFloatPair& valuePair)
    {
        if (parameterName == PhysX::EditorJointComponentMode::s_parameterAngularPair)
        {
            m_angularLimit.m_limitPositive = valuePair.first;
            m_angularLimit.m_limitNegative = valuePair.second;
        }
    }

    void EditorHingeJointComponent::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        EditorJointComponent::DisplayEntityViewport(viewportInfo, debugDisplay);

        if (!m_config.m_displayJointSetup && 
            !m_config.m_inComponentMode)
        {
            return;
        }

        const float s_alpha = 0.6f;
        const AZ::Color s_colorDefault = AZ::Color(1.0f, 1.0f, 1.0f, s_alpha);
        const AZ::Color s_colorFirst = AZ::Color(1.0f, 0.0f, 0.0f, s_alpha);
        const AZ::Color s_colorSecond = AZ::Color(0.0f, 1.0f, 0.0f, s_alpha);
        const AZ::Color s_colorSweepArc = AZ::Color(1.0f, 1.0f, 1.0f, s_alpha);

        AngleLimitsFloatPair currentValue(m_angularLimit.m_limitPositive, m_angularLimit.m_limitNegative);
        AZ::Vector3 axis = AZ::Vector3::CreateAxisX();

        const float size = 2.0f;
        AZ::Vector3 axisPoint = axis * size * 0.5f;

        AZ::Vector3 points[4] = {
            -axisPoint
            , axisPoint
            , axisPoint
            , -axisPoint
        };

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

        AZ::u32 stateBefore = debugDisplay.GetState();
        debugDisplay.CullOff();
        debugDisplay.SetAlpha(s_alpha);

        const AZ::EntityId& entityId = GetEntityId();

        AZ::Transform worldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(entityId);

        AZ::Transform localTransform;
        EditorJointRequestBus::EventResult(localTransform, 
            AZ::EntityComponentIdPair(entityId, GetId()),
            &EditorJointRequests::GetTransformValue, 
            PhysX::EditorJointComponentMode::s_parameterTransform);

        debugDisplay.PushMatrix(worldTransform);
        debugDisplay.PushMatrix(localTransform);

        debugDisplay.SetColor(s_colorSweepArc);
        const float sweepLineDisplaceFactor = 0.5f;
        const float sweepLineThickness = 1.0f;
        const float sweepLineGranularity = 1.0f;
        const AZ::Vector3 zeroVector = AZ::Vector3::CreateZero();
        const AZ::Vector3 posPosition = axis * sweepLineDisplaceFactor;
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

        debugDisplay.PopMatrix(); //pop local transform
        debugDisplay.PopMatrix(); //pop global transform
        debugDisplay.SetState(stateBefore);
    }
}
