/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Source/ComponentModes/Joints/JointsSubComponentModeSnapRotation.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>

#include <Editor/Source/ComponentModes/Joints/JointsComponentModeCommon.h>
#include <PhysX/EditorJointBus.h>
#include <Source/Utils.h>

namespace PhysX
{
    AZ_CLASS_ALLOCATOR_IMPL(JointsSubComponentModeSnapRotation, AZ::SystemAllocator);

    void JointsSubComponentModeSnapRotation::Setup(const AZ::EntityComponentIdPair& idPair)
    {
        JointsSubComponentModeSnap::Setup(idPair);

        PhysX::EditorJointRequestBus::EventResult(
            m_resetRotation, m_entityComponentId, &PhysX::EditorJointRequests::GetVector3Value, JointsComponentModeCommon::ParameterNames::Rotation);

        m_manipulator->InstallLeftMouseDownCallback(
            [this]([[maybe_unused]] const AzToolsFramework::LinearManipulator::Action& action) mutable
            {
                if (!m_pickedEntity.IsValid())
                {
                    return;
                }

                AZ::EntityId leadEntityId;
                PhysX::EditorJointRequestBus::EventResult(
                    leadEntityId, m_entityComponentId, &PhysX::EditorJointRequests::GetEntityIdValue,
                    JointsComponentModeCommon::ParameterNames::LeadEntity);

                if (leadEntityId.IsValid() && m_pickedEntity == leadEntityId)
                {
                    AZ_Warning(
                        "EditorsubComponentModeSnapRotation", false,
                        "The entity %s is the lead of the joint. Please snap rotation (or orientation) of joint to another entity that is "
                        "not the lead entity.",
                        GetPickedEntityName().c_str());
                    return;
                }

                AZ::Transform worldTransform = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(worldTransform, m_entityComponentId.GetEntityId(), &AZ::TransformInterface::GetWorldTM);
                worldTransform.ExtractUniformScale();

                AZ::Transform localTransform = AZ::Transform::CreateIdentity();
                EditorJointRequestBus::EventResult(
                    localTransform, m_entityComponentId, &EditorJointRequests::GetTransformValue, JointsComponentModeCommon::ParameterNames::Transform);

                AZ::Transform pickedEntityTransform = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(pickedEntityTransform, m_pickedEntity, &AZ::TransformInterface::GetWorldTM);

                const AZ::Transform worldTransformInv = worldTransform.GetInverse();
                const AZ::Vector3 pickedLocalPosition =
                    worldTransformInv.TransformVector(pickedEntityTransform.GetTranslation()) - localTransform.GetTranslation();

                if (AZStd::abs(pickedLocalPosition.GetLength()) < FLT_EPSILON)
                {
                    AZ_Warning(
                        "EditorsubComponentModeSnapRotation", false,
                        "The entity %s is too close to the joint position. Please snap rotation to an entity that is not at the position "
                        "of the joint.",
                        GetPickedEntityName().c_str());
                    return;
                }

                const AZ::Vector3 targetDirection = pickedLocalPosition.GetNormalized();
                const AZ::Vector3 sourceDirection = AZ::Vector3::CreateAxisX();
                const AZ::Quaternion newLocalRotation = AZ::Quaternion::CreateShortestArc(sourceDirection, targetDirection);

                PhysX::EditorJointRequestBus::Event(
                    m_entityComponentId, &PhysX::EditorJointRequests::SetVector3Value,
                    JointsComponentModeCommon::ParameterNames::Rotation // using rotation parameter name to set the local rotation value
                    ,
                    newLocalRotation.GetEulerDegrees());
            });
    }

    void JointsSubComponentModeSnapRotation::ResetValues([[maybe_unused]] const AZ::EntityComponentIdPair& idPair)
    {
        PhysX::EditorJointRequestBus::Event(
            m_entityComponentId, &PhysX::EditorJointRequests::SetVector3Value, JointsComponentModeCommon::ParameterNames::Rotation, m_resetRotation);
    }

    void JointsSubComponentModeSnapRotation::DisplaySpecificSnapType(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AZ::Vector3& jointPosition,
        const AZ::Vector3& snapDirection,
        float snapLength)
    {
        const float circleRadius = 0.5f;
        const float iconGap = 1.0f;

        const AZ::Vector3 iconPosition = jointPosition + (snapDirection * (snapLength + circleRadius * 2.0f + iconGap));

        debugDisplay.SetColor(AZ::Colors::Red);
        debugDisplay.DrawCircle(iconPosition, circleRadius, 0);
        debugDisplay.SetColor(AZ::Colors::Green);
        debugDisplay.DrawCircle(iconPosition, circleRadius, 1);
        debugDisplay.SetColor(AZ::Colors::Blue);
        debugDisplay.DrawCircle(iconPosition, circleRadius, 2);
    }

} // namespace PhysX
