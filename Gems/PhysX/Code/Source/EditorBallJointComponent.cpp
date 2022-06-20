/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

#include <Source/EditorBallJointComponent.h>
#include <Editor/Source/ComponentModes/Joints/JointsComponentMode.h>
#include <Editor/Source/ComponentModes/Joints/JointsComponentModeCommon.h>
#include <Source/BallJointComponent.h>
#include <Source/Utils.h>

namespace PhysX
{
    void EditorBallJointComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorBallJointComponent, EditorJointComponent>()
                ->Version(2)
                ->Field("Swing Limit", &EditorBallJointComponent::m_swingLimit)
                ->Field("Component Mode", &EditorBallJointComponent::m_componentModeDelegate)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorBallJointComponent>(
                    "PhysX Ball Joint", "A dynamic joint constraint with swing rotation limits around the Y and Z axes of the joint.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/physx/ball-joint/")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &EditorBallJointComponent::m_swingLimit, "Swing Limit", "The rotation angle limit around the joint's Y and Z axes.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorBallJointComponent::m_componentModeDelegate, "Component Mode", "Ball Joint Component Mode.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    void EditorBallJointComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("PhysXJointService", 0x0d2f906f));
    }

    void EditorBallJointComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        required.push_back(AZ_CRC("PhysXColliderService", 0x4ff43f7c));
        required.push_back(AZ_CRC("PhysXRigidBodyService", 0x1d4c64a8));
    }

    void EditorBallJointComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void EditorBallJointComponent::Activate()
    {
        EditorJointComponent::Activate();

        const AZ::EntityId entityId = GetEntityId();

        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler::BusConnect(entityId);

        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler* selection = this;
        m_componentModeDelegate.ConnectWithSingleComponentMode<EditorBallJointComponent, JointsComponentMode>(
            AZ::EntityComponentIdPair(entityId, GetId()), selection);

        PhysX::EditorJointRequestBus::Handler::BusConnect(AZ::EntityComponentIdPair(entityId, GetId()));
    }

    void EditorBallJointComponent::Deactivate()
    {
        PhysX::EditorJointRequestBus::Handler::BusDisconnect();
        m_componentModeDelegate.Disconnect();
        AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
        EditorJointComponent::Deactivate();
    }

    void EditorBallJointComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        m_config.m_followerEntity = GetEntityId(); // joint is always in the same entity as the follower body.
        gameEntity->CreateComponent<BallJointComponent>(
            m_config.ToGameTimeConfig(), 
            m_config.ToGenericProperties(),
            m_swingLimit.ToGameTimeConfig());
    }

    float EditorBallJointComponent::GetLinearValue(const AZStd::string& parameterName)
    {
        if (parameterName == PhysX::JointsComponentModeCommon::ParamaterNames::MaxForce)
        {
            return m_config.m_forceMax;
        }
        else if (parameterName == PhysX::JointsComponentModeCommon::ParamaterNames::MaxTorque)
        {
            return m_config.m_torqueMax;
        }
        else if (parameterName == PhysX::JointsComponentModeCommon::ParamaterNames::Damping)
        {
            return m_swingLimit.m_standardLimitConfig.m_damping;
        }
        else if (parameterName == PhysX::JointsComponentModeCommon::ParamaterNames::Stiffness)
        {
            return m_swingLimit.m_standardLimitConfig.m_stiffness;
        }

        return 0.0f;
    }

    AngleLimitsFloatPair EditorBallJointComponent::GetLinearValuePair(const AZStd::string& parameterName)
    {
        if (parameterName == PhysX::JointsComponentModeCommon::ParamaterNames::SwingLimit)
        {
            return AngleLimitsFloatPair(m_swingLimit.m_limitY, m_swingLimit.m_limitZ);
        }

        return AngleLimitsFloatPair();
    }

    AZStd::vector<JointsComponentModeCommon::SubModeParamaterState> EditorBallJointComponent::GetSubComponentModesState()
    {
        AZStd::vector<JointsComponentModeCommon::SubModeParamaterState> subModes;

        subModes.emplace_back(JointsComponentModeCommon::SubModeParamaterState{
            JointsComponentModeCommon::SubComponentModes::ModeType::SnapPosition,
            JointsComponentModeCommon::ParamaterNames::SnapPosition });
        subModes.emplace_back(JointsComponentModeCommon::SubModeParamaterState{
            JointsComponentModeCommon::SubComponentModes::ModeType::SnapRotation,
            JointsComponentModeCommon::ParamaterNames::SnapRotation });

        if (AZStd::vector<JointsComponentModeCommon::SubModeParamaterState> baseSubModes =
                EditorJointComponent::GetSubComponentModesState();
            !baseSubModes.empty())
        {
            subModes.insert(subModes.end(), baseSubModes.begin(), baseSubModes.end());
        }

        if (m_swingLimit.m_standardLimitConfig.m_isLimited)
        {
            subModes.emplace_back(
                JointsComponentModeCommon::SubModeParamaterState{ JointsComponentModeCommon::SubComponentModes::ModeType::SwingLimits,
                                                                  JointsComponentModeCommon::ParamaterNames::SwingLimit });

            if (m_swingLimit.m_standardLimitConfig.m_isSoftLimit)
            {
                subModes.emplace_back(JointsComponentModeCommon::SubModeParamaterState{
                    JointsComponentModeCommon::SubComponentModes::ModeType::Damping, JointsComponentModeCommon::ParamaterNames::Damping });
                subModes.emplace_back(
                    JointsComponentModeCommon::SubModeParamaterState{ JointsComponentModeCommon::SubComponentModes::ModeType::Stiffness,
                                                                      JointsComponentModeCommon::ParamaterNames::Stiffness });
            }
        }
        
        return subModes;
    }

    void EditorBallJointComponent::SetLinearValue(const AZStd::string& parameterName, float value)
    {
        if (parameterName == PhysX::JointsComponentModeCommon::ParamaterNames::MaxForce)
        {
            m_config.m_forceMax = value;
        }
        else if (parameterName == PhysX::JointsComponentModeCommon::ParamaterNames::MaxTorque)
        {
            m_config.m_torqueMax = value;
        }
        else if (parameterName == PhysX::JointsComponentModeCommon::ParamaterNames::Damping)
        {
            m_swingLimit.m_standardLimitConfig.m_damping = value;
        }
        else if (parameterName == PhysX::JointsComponentModeCommon::ParamaterNames::Stiffness)
        {
            m_swingLimit.m_standardLimitConfig.m_stiffness = value;
        }
    }

    void EditorBallJointComponent::SetLinearValuePair(const AZStd::string& parameterName, const AngleLimitsFloatPair& valuePair)
    {
        if (parameterName == PhysX::JointsComponentModeCommon::ParamaterNames::SwingLimit)
        {
            m_swingLimit.m_limitY = valuePair.first;
            m_swingLimit.m_limitZ = valuePair.second;
        }
    }

    void EditorBallJointComponent::SetBoolValue(const AZStd::string& parameterName, bool value)
    {
        if (parameterName == PhysX::JointsComponentModeCommon::ParamaterNames::ComponentMode)
        {
            m_swingLimit.m_standardLimitConfig.m_inComponentMode = value;
            m_config.m_inComponentMode = value;

            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay
                , AzToolsFramework::Refresh_EntireTree);
        }
    }

    void EditorBallJointComponent::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        EditorJointComponent::DisplayEntityViewport(viewportInfo, debugDisplay);

        if (!m_config.ShowSetupDisplay() && 
            !m_config.m_inComponentMode)
        {
            return;
        }

        const AZ::EntityId entityId = GetEntityId();

        AZ::Transform worldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(entityId);

        AZ::Transform localTransform;
        EditorJointRequestBus::EventResult(localTransform,
            AZ::EntityComponentIdPair(entityId, GetId()),
            &EditorJointRequests::GetTransformValue, 
            PhysX::JointsComponentModeCommon::ParamaterNames::Transform);

        AZ::u32 stateBefore = debugDisplay.GetState();
        debugDisplay.CullOff();

        debugDisplay.PushMatrix(worldTransform);
        debugDisplay.PushMatrix(localTransform);

        const float xAxisArrowLength = 2.0f;
        debugDisplay.SetColor(AZ::Color(1.0f, 0.0f, 0.0f, 1.0f));
        debugDisplay.DrawArrow(AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(xAxisArrowLength, 0.0f, 0.0f));

        AngleLimitsFloatPair yzSwingAngleLimits(m_swingLimit.m_limitY, m_swingLimit.m_limitZ);

        const AZ::u32 numEllipseSamples = 16;
        AZ::Vector3 ellipseSamples[numEllipseSamples];
        float coneHeight = 3.0f;

        // Draw inverted cone if angles are larger than 90 deg.
        if (yzSwingAngleLimits.first > 90.0f || yzSwingAngleLimits.second > 90.0f)
        {
            coneHeight = -3.0f;
        }

        const float coney = tan(AZ::DegToRad(yzSwingAngleLimits.first)) * coneHeight;
        const float conez = tan(AZ::DegToRad(yzSwingAngleLimits.second)) * coneHeight;
        const float step = AZ::Constants::TwoPi / numEllipseSamples;
        for (size_t i = 0; i < numEllipseSamples; ++i)
        {
            const float angleStep = step * i;
            ellipseSamples[i].SetX(coneHeight);
            ellipseSamples[i].SetY(conez * sin(angleStep));
            ellipseSamples[i].SetZ(coney * cos(angleStep));
        }
        debugDisplay.SetColor(AZ::Color(1.0f, 1.0f, 1.0f, 0.7f));
        for (size_t i = 0; i < numEllipseSamples; ++i)
        {
            size_t nextIndex = i + 1;
            if (i == numEllipseSamples - 1)
            {
                nextIndex = 0;
            }
            debugDisplay.DrawTri(AZ::Vector3(0.0f, 0.0f, 0.0f), ellipseSamples[i], ellipseSamples[nextIndex]);
        }

        debugDisplay.PopMatrix();//pop local transform
        debugDisplay.PopMatrix();//pop world transform

        debugDisplay.SetState(stateBefore);
    }
}
