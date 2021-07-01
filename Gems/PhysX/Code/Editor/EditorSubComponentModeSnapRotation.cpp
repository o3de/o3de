
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <PhysX_precompiled.h>

#include <Editor/EditorJointComponentMode.h>
#include <Editor/EditorSubComponentModeSnapRotation.h>
#include <PhysX/EditorJointBus.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Color.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>

namespace PhysX
{
    EditorSubComponentModeSnapRotation::EditorSubComponentModeSnapRotation(
        const AZ::EntityComponentIdPair& entityComponentIdPair
        , const AZ::Uuid& componentType
        , const AZStd::string& name)
        : EditorSubComponentModeSnap(entityComponentIdPair, componentType, name)
    {
        InitMouseDownCallBack();
        m_manipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(m_entityComponentId.GetEntityId());
    }

    EditorSubComponentModeSnapRotation::~EditorSubComponentModeSnapRotation()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        m_manipulator->Unregister();
    }

    void EditorSubComponentModeSnapRotation::DisplaySpecificSnapType(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AZ::Vector3& jointPosition,
        const AZ::Vector3& snapDirection,
        const float snapLength)
    {
        const float circleRadius = 0.5f;
        const float iconGap = 1.0f;

        const AZ::Vector3 iconPosition = jointPosition +
            (snapDirection * (snapLength + circleRadius * 2.0f + iconGap));

        debugDisplay.SetColor(AZ::Colors::Red);
        debugDisplay.DrawCircle(iconPosition, circleRadius, 0);
        debugDisplay.SetColor(AZ::Colors::Green);
        debugDisplay.DrawCircle(iconPosition, circleRadius, 1);
        debugDisplay.SetColor(AZ::Colors::Blue);
        debugDisplay.DrawCircle(iconPosition, circleRadius, 2);
    }

    void EditorSubComponentModeSnapRotation::InitMouseDownCallBack()
    {
        m_manipulator->InstallLeftMouseDownCallback(
            [this]([[maybe_unused]] const AzToolsFramework::LinearManipulator::Action& action) mutable
        {
            if (!m_pickedEntity.IsValid())
            {
                return;
            }

            AZ::EntityId leadEntityId;
            PhysX::EditorJointRequestBus::EventResult(leadEntityId,
                m_entityComponentId, 
                &PhysX::EditorJointRequests::GetEntityIdValue, 
                PhysX::EditorJointComponentMode::s_parameterLeadEntity);

            if (leadEntityId.IsValid() && m_pickedEntity == leadEntityId)
            {
                AZ_Warning("EditorsubComponentModeSnapRotation", 
                    false, 
                    "The entity %s is the lead of the joint. Please snap rotation (or orientation) of joint to another entity that is not the lead entity.",
                    GetPickedEntityName().c_str());
                return;
            }

            AZ::Transform worldTransform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(
                worldTransform, m_entityComponentId.GetEntityId(), &AZ::TransformInterface::GetWorldTM);
            worldTransform.ExtractUniformScale();

            AZ::Transform localTransform = AZ::Transform::CreateIdentity();
            EditorJointRequestBus::EventResult(
                localTransform, m_entityComponentId
                , &EditorJointRequests::GetTransformValue
                , PhysX::EditorJointComponentMode::s_parameterTransform);

            AZ::Transform pickedEntityTransform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(
                pickedEntityTransform, m_pickedEntity, &AZ::TransformInterface::GetWorldTM);

            const AZ::Transform worldTransformInv = worldTransform.GetInverse();
            const AZ::Vector3 pickedLocalPosition = worldTransformInv.TransformVector(pickedEntityTransform.GetTranslation()) - localTransform.GetTranslation();

            if (abs(pickedLocalPosition.GetLength()) < FLT_EPSILON)
            {
                AZ_Warning("EditorsubComponentModeSnapRotation",
                    false,
                    "The entity %s is too close to the joint position. Please snap rotation to an entity that is not at the position of the joint.",
                    GetPickedEntityName().c_str());
                return;
            }

            const AZ::Vector3 targetDirection = pickedLocalPosition.GetNormalized();
            const AZ::Vector3 sourceDirection = AZ::Vector3::CreateAxisX();
            const AZ::Quaternion newLocalRotation = AZ::Quaternion::CreateShortestArc(sourceDirection, targetDirection);

            PhysX::EditorJointRequestBus::Event(m_entityComponentId
                , &PhysX::EditorJointRequests::SetVector3Value
                , PhysX::EditorJointComponentMode::s_parameterRotation //using rotation parameter name to set the local rotation value
                , newLocalRotation.GetEulerDegrees());

            m_manipulator->SetBoundsDirty();
        });
    }
} // namespace PhysX
