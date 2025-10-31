/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

#include <PhysX/Configuration/PhysXConfiguration.h>
#include <Editor/Source/ComponentModes/Joints/JointsComponentModeCommon.h>
#include <Source/EditorJointComponent.h>
#include <Source/EditorRigidBodyComponent.h>
#include <Source/Utils.h>
#include <System/PhysXSystem.h>

namespace PhysX
{
    const constexpr float JointAabbHalfExtent = 0.5f;

    void EditorJointComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorJointComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("Configuration", &EditorJointComponent::m_config)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorJointComponent>(
                    "PhysX Joint", "A dynamic joint that constrains the position and orientation of one rigid body to another.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &EditorJointComponent::m_config, "Standard Joint Parameters", "Joint parameters shared by all joint types.")
                    ;
            }
        }
    }

    void EditorJointComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();

        m_config.m_followerEntity = GetEntityId();

        m_cachedWorldTM = GetWorldTM();

        AZ::TransformNotificationBus::Handler::BusConnect(m_config.m_followerEntity);
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(m_config.m_followerEntity);
        AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler::BusConnect(m_config.m_followerEntity);
        PhysX::EditorJointRequestBus::Handler::BusConnect(AZ::EntityComponentIdPair(m_config.m_followerEntity, GetId()));
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(m_config.m_followerEntity);
        AzFramework::BoundsRequestBus::Handler::BusConnect(m_config.m_followerEntity);
    }

    void EditorJointComponent::Deactivate()
    {
        AzFramework::BoundsRequestBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        PhysX::EditorJointRequestBus::Handler::BusDisconnect();

        AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void EditorJointComponent::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& localTM, const AZ::Transform& worldTM)
    {
        if (m_config.m_fixJointLocation)
        {
            const AZ::Transform localJoint = GetTransformValue(JointsComponentModeCommon::ParameterNames::Transform);
            const AZ::Transform worldJoint = m_cachedWorldTM * localJoint;

            const AZ::Transform localFromWorld = worldTM.GetInverse();
            const AZ::Transform newLocalJoint = localFromWorld * worldJoint;
            m_config.m_localPosition = newLocalJoint.GetTranslation();
            m_config.m_localRotation = newLocalJoint.GetEulerDegrees();

            InvalidatePropertyDisplay(AzToolsFramework::Refresh_Values);
        }
        m_cachedWorldTM = worldTM;
    }

    AZ::Aabb EditorJointComponent::GetEditorSelectionBoundsViewport(
        const AzFramework::ViewportInfo& viewportInfo)
    {
        AZ::Transform worldTM = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldTM, GetEntityId(), &AZ::TransformInterface::GetWorldTM);
        const AZ::Vector3 position = worldTM.GetTranslation();

        AzFramework::CameraState cameraState;
        AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::EventResult(
            cameraState, viewportInfo.m_viewportId,
            &AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Events::GetCameraState);

        const float screenToWorldScale = AzToolsFramework::CalculateScreenToWorldMultiplier(position, cameraState);

        const float scaledJointHalfAabbExtent = JointAabbHalfExtent * screenToWorldScale;
        const AZ::Vector3 selectionSize(scaledJointHalfAabbExtent);
        const AZ::Vector3 positionMin = position - selectionSize;
        const AZ::Vector3 positionMax = position + selectionSize;

        AZ::Aabb aabb = AZ::Aabb::CreateFromMinMax(positionMin, positionMax);

        return aabb;
    }

    AZ::Aabb EditorJointComponent::GetWorldBounds() const
    {
        AZ::Transform worldTM = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldTM, GetEntityId(), &AZ::TransformInterface::GetWorldTM);

        return GetLocalBounds().GetTransformedAabb(worldTM);
    }

    AZ::Aabb EditorJointComponent::GetLocalBounds() const
    {
        return AZ::Aabb::CreateFromMinMax(-AZ::Vector3(JointAabbHalfExtent), AZ::Vector3(JointAabbHalfExtent));
    }

    bool EditorJointComponent::EditorSelectionIntersectRayViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        const AZ::Vector3& src, const AZ::Vector3& dir, float& distance)
    {
        const float rayLength = 10000.0f;
        AZ::Vector3 startNormal;
        float tEnd;
        float tStart;
        const AZ::Vector3 scaledDir = dir * rayLength;
        const AZ::Aabb aabb = GetEditorSelectionBoundsViewport(viewportInfo);
        const int intersection = AZ::Intersect::IntersectRayAABB(
            src, scaledDir, scaledDir.GetReciprocal(), aabb, tStart, tEnd, startNormal);

        if (intersection == AZ::Intersect::RayAABBIsectTypes::ISECT_RAY_AABB_ISECT)
        {
            distance = tStart * rayLength;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool EditorJointComponent::GetBoolValue(const AZStd::string& parameterName)
    {
        if (parameterName == JointsComponentModeCommon::ParameterNames::ComponentMode)
        {
            return m_config.m_inComponentMode;
        }

        AZ_Error("EditorJointComponent::GetBoolValue", false, "bool parameter not recognized: %s", parameterName.c_str());
        return false;
    }

    AZ::EntityId EditorJointComponent::GetEntityIdValue(const AZStd::string& parameterName)
    {
        if (parameterName == JointsComponentModeCommon::ParameterNames::LeadEntity)
        {
            return m_config.m_leadEntity;
        }
        AZ_Error("EditorJointComponent::GetEntityIdValue", false, "EntityId parameter not recognized: %s", parameterName.c_str());
        AZ::EntityId defaultEntityId;
        defaultEntityId.SetInvalid();
        return defaultEntityId;
    }

    float EditorJointComponent::GetLinearValue(const AZStd::string& parameterName)
    {
        if (parameterName == JointsComponentModeCommon::ParameterNames::MaxForce)
        {
            return m_config.m_forceMax;
        }
        else if (parameterName == JointsComponentModeCommon::ParameterNames::MaxTorque)
        {
            return m_config.m_torqueMax;
        }
        AZ_Error("EditorJointComponent::GetLinearValue", false, "Linear value parameter not recognized: %s", parameterName.c_str());
        return 0.0f;
    }

    AngleLimitsFloatPair EditorJointComponent::GetLinearValuePair(const AZStd::string& parameterName)
    {
        AZ_UNUSED(parameterName);
        AZ_Error("EditorJointComponent::GetLinearValuePair", false, "Linear value pair parameter not recognized: %s", parameterName.c_str());
        return AngleLimitsFloatPair();
    }

    AZ::Transform EditorJointComponent::GetTransformValue(const AZStd::string& parameterName)
    {
        if (parameterName == JointsComponentModeCommon::ParameterNames::Transform)
        {
            return AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateFromEulerAnglesDegrees(m_config.m_localRotation),
                m_config.m_localPosition);
        }
        AZ_Error("EditorJointComponent::GetTransformValue", false, "Transform value parameter not recognized: %s", parameterName.c_str());
        return AZ::Transform::CreateIdentity();
    }

    AZ::Vector3 EditorJointComponent::GetVector3Value(const AZStd::string& parameterName)
    {
        if (parameterName == JointsComponentModeCommon::ParameterNames::Position)
        {
            return m_config.m_localPosition;
        }
        if (parameterName == JointsComponentModeCommon::ParameterNames::Rotation)
        {
            return m_config.m_localRotation;
        }
        AZ_Error("EditorJointComponent::GetVector3Value", false, "Vector3 value parameter not recognized: %s", parameterName.c_str());
        return AZ::Vector3::CreateZero();
    }

    AZStd::vector<JointsComponentModeCommon::SubModeParameterState> EditorJointComponent::GetSubComponentModesState()
    {
        AZStd::vector<JointsComponentModeCommon::SubModeParameterState> subModes;
        if (m_config.m_breakable)
        {
            subModes.emplace_back(JointsComponentModeCommon::SubModeParameterState{
                JointsComponentModeCommon::SubComponentModes::ModeType::MaxForce, JointsComponentModeCommon::ParameterNames::MaxForce });
            subModes.emplace_back(JointsComponentModeCommon::SubModeParameterState{
                JointsComponentModeCommon::SubComponentModes::ModeType::MaxTorque,
                JointsComponentModeCommon::ParameterNames::MaxTorque });
        }
        return subModes;
    }

    void EditorJointComponent::SetLinearValue(const AZStd::string& parameterName, float value)
    {
        if (parameterName == JointsComponentModeCommon::ParameterNames::MaxForce)
        {
            m_config.m_forceMax = value;
        }
        else if (parameterName == JointsComponentModeCommon::ParameterNames::MaxTorque)
        {
            m_config.m_torqueMax = value;
        }
    }

    void EditorJointComponent::SetLinearValuePair(const AZStd::string& parameterName, const AngleLimitsFloatPair& valuePair)
    {
        AZ_UNUSED(parameterName);
        AZ_UNUSED(valuePair);
    }

    void EditorJointComponent::SetVector3Value(const AZStd::string& parameterName, const AZ::Vector3& value)
    {
        if (parameterName == JointsComponentModeCommon::ParameterNames::Position)
        {
            m_config.m_localPosition = value;
        }
        else if (parameterName == JointsComponentModeCommon::ParameterNames::Rotation)
        {
            m_config.m_localRotation = value;
        }
    }

    void EditorJointComponent::SetBoolValue(const AZStd::string& parameterName, bool value)
    {
        if (parameterName == JointsComponentModeCommon::ParameterNames::ComponentMode)
        {
            m_config.m_inComponentMode = value;

            InvalidatePropertyDisplay(AzToolsFramework::Refresh_EntireTree);
        }
    }

    void EditorJointComponent::SetEntityIdValue(const AZStd::string& parameterName, AZ::EntityId value)
    {
        if (parameterName == JointsComponentModeCommon::ParameterNames::LeadEntity)
        {
            m_config.SetLeadEntityId(value);
        }
    }

    void EditorJointComponent::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_UNUSED(viewportInfo);

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

            const AZ::Transform followerWorldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(m_config.m_followerEntity);
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

            if (m_config.m_leadEntity.IsValid() &&
                distance < displayData.m_jointHierarchyDistanceThreshold)
            {
                const AZ::Transform leadWorldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(m_config.m_leadEntity);
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
}
