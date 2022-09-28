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

#include <Source/EditorPrismaticJointComponent.h>
#include <Source/PrismaticJointComponent.h>
#include <Source/Utils.h>

namespace PhysX
{
    void EditorPrismaticJointComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorPrismaticJointComponent, EditorJointComponent>()
                ->Version(2)
                ->Field("Linear Limit", &EditorPrismaticJointComponent::m_linearLimit)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<EditorPrismaticJointComponent>(
                        "PhysX Prismatic Joint", "A dynamic joint that constrains a rigid body with linear limits along a single axis.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        0,
                        &EditorPrismaticJointComponent::m_linearLimit,
                        "Linear Limit",
                        "The limit of linear motion along the joint's axis.");
            }
        }
    }

    void EditorPrismaticJointComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsJointService"));
    }

    void EditorPrismaticJointComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
        required.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
    }

    void EditorPrismaticJointComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void EditorPrismaticJointComponent::Activate()
    {
        EditorJointComponent::Activate();

        const AZ::EntityId& entityId = GetEntityId();

        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler::BusConnect(entityId);
        PhysX::EditorJointRequestBus::Handler::BusConnect(AZ::EntityComponentIdPair(entityId, GetId()));
    }

    void EditorPrismaticJointComponent::Deactivate()
    {
        PhysX::EditorJointRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
        EditorJointComponent::Deactivate();
    }

    void EditorPrismaticJointComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        m_config.m_followerEntity = GetEntityId(); // joint is always in the same entity as the follower body.
        gameEntity->CreateComponent<PrismaticJointComponent>(
            m_config.ToGameTimeConfig(),
            m_config.ToGenericProperties(),
            m_linearLimit.ToGameTimeConfig());
    }

    float EditorPrismaticJointComponent::GetLinearValue(const AZStd::string& parameterName)
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
            return m_linearLimit.m_standardLimitConfig.m_damping;
        }
        else if (parameterName == PhysX::JointsComponentModeCommon::ParamaterNames::Stiffness)
        {
            return m_linearLimit.m_standardLimitConfig.m_stiffness;
        }

        return 0.0f;
    }

    LinearLimitsFloatPair EditorPrismaticJointComponent::GetLinearValuePair(const AZStd::string& parameterName)
    {
        if (parameterName == PhysX::JointsComponentModeCommon::ParamaterNames::LinearLimits)
        {
            return LinearLimitsFloatPair(m_linearLimit.m_limitUpper, m_linearLimit.m_limitLower);
        }

        return LinearLimitsFloatPair();
    }

    AZStd::vector<JointsComponentModeCommon::SubModeParamaterState> EditorPrismaticJointComponent::GetSubComponentModesState()
    {
        return {};
    }

    void EditorPrismaticJointComponent::SetBoolValue(const AZStd::string& parameterName, bool value)
    {
        if (parameterName == PhysX::JointsComponentModeCommon::ParamaterNames::EnableLimits)
        {
            m_linearLimit.m_standardLimitConfig.m_isLimited = value;
        }
        else if (parameterName == PhysX::JointsComponentModeCommon::ParamaterNames::EnableSoftLimits)
        {
            m_linearLimit.m_standardLimitConfig.m_isSoftLimit = value;
        }
    }

    void EditorPrismaticJointComponent::SetLinearValue(const AZStd::string& parameterName, float value)
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
            m_linearLimit.m_standardLimitConfig.m_damping = value;
        }
        else if (parameterName == PhysX::JointsComponentModeCommon::ParamaterNames::Stiffness)
        {
            m_linearLimit.m_standardLimitConfig.m_stiffness = value;
        }
    }

    void EditorPrismaticJointComponent::SetLinearValuePair(const AZStd::string& parameterName, const LinearLimitsFloatPair& valuePair)
    {
        if (parameterName == PhysX::JointsComponentModeCommon::ParamaterNames::LinearLimits)
        {
            m_linearLimit.m_limitUpper = valuePair.first;
            m_linearLimit.m_limitLower = valuePair.second;
        }
    }

    void EditorPrismaticJointComponent::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        EditorJointComponent::DisplayEntityViewport(viewportInfo, debugDisplay);

        if (!m_config.ShowSetupDisplay() &&
            !m_config.m_inComponentMode)
        {
            return;
        }

        const float size = 1.0f;
        const float alpha = 0.6f;
        const AZ::Color colorDefault = AZ::Color(1.0f, 1.0f, 1.0f, alpha);
        const AZ::Color colorLimitLower = AZ::Color(1.0f, 0.0f, 0.0f, alpha);
        const AZ::Color colorLimitUpper = AZ::Color(0.0f, 1.0f, 0.0f, alpha);

        AZ::u32 stateBefore = debugDisplay.GetState();
        debugDisplay.CullOff();
        debugDisplay.SetAlpha(alpha);

        const AZ::EntityId& entityId = GetEntityId();

        AZ::Transform worldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(entityId);

        AZ::Transform localTransform;
        EditorJointRequestBus::EventResult(localTransform,
            AZ::EntityComponentIdPair(entityId, GetId()),
            &EditorJointRequests::GetTransformValue,
            PhysX::JointsComponentModeCommon::ParamaterNames::Transform);

        debugDisplay.PushMatrix(worldTransform);
        debugDisplay.PushMatrix(localTransform);

        debugDisplay.SetColor(colorDefault);
        debugDisplay.DrawLine(AZ::Vector3::CreateAxisX(m_linearLimit.m_limitLower), AZ::Vector3::CreateAxisX(m_linearLimit.m_limitUpper));

        debugDisplay.SetColor(colorLimitLower);
        debugDisplay.DrawQuad(
            AZ::Vector3(m_linearLimit.m_limitLower, -size, -size),
            AZ::Vector3(m_linearLimit.m_limitLower, -size, size),
            AZ::Vector3(m_linearLimit.m_limitLower, size, size),
            AZ::Vector3(m_linearLimit.m_limitLower, size, -size));

        debugDisplay.SetColor(colorLimitUpper);
        debugDisplay.DrawQuad(
            AZ::Vector3(m_linearLimit.m_limitUpper, -size, -size),
            AZ::Vector3(m_linearLimit.m_limitUpper, -size, size),
            AZ::Vector3(m_linearLimit.m_limitUpper, size, size),
            AZ::Vector3(m_linearLimit.m_limitUpper, size, -size));

        debugDisplay.PopMatrix(); // pop local transform
        debugDisplay.PopMatrix(); // pop world transform
        debugDisplay.SetState(stateBefore);
    }
} // namespace PhysX
