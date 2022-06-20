/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Source/ComponentModes/Joints/JointsSubComponentModeSnapPosition.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <Editor/Source/ComponentModes/Joints/JointsComponentModeCommon.h>
#include <PhysX/EditorJointBus.h>
#include <Source/Utils.h>

namespace PhysX
{
    AZ_CLASS_ALLOCATOR_IMPL(JointsSubComponentModeSnapPosition, AZ::SystemAllocator, 0);

    void JointsSubComponentModeSnapPosition::Setup(const AZ::EntityComponentIdPair& idPair)
    {
        JointsSubComponentModeSnap::Setup(idPair);

        PhysX::EditorJointRequestBus::EventResult(
            m_resetPosition, m_entityComponentId, &PhysX::EditorJointRequests::GetVector3Value, JointsComponentModeCommon::ParamaterNames::Position);

        PhysX::EditorJointRequestBus::EventResult(
            m_resetLeadEntity, m_entityComponentId, &PhysX::EditorJointRequests::GetEntityIdValue,
            JointsComponentModeCommon::ParamaterNames::LeadEntity);

        m_manipulator->InstallLeftMouseDownCallback(
            [this](const AzToolsFramework::LinearManipulator::Action& /*action*/) mutable
            {
                if (!m_pickedEntity.IsValid())
                {
                    return;
                }

                const AZ::Vector3 newLocalPosition = PhysX::Utils::ComputeJointLocalTransform(
                                                         PhysX::Utils::GetEntityWorldTransformWithScale(m_pickedEntity),
                                                         PhysX::Utils::GetEntityWorldTransformWithScale(m_entityComponentId.GetEntityId()))
                                                         .GetTranslation();

                PhysX::EditorJointRequestBus::Event(
                    m_entityComponentId, &PhysX::EditorJointRequests::SetVector3Value, JointsComponentModeCommon::ParamaterNames::Position,
                    newLocalPosition);

                const bool selectedEntityIsNotJointEntity = m_pickedEntity != m_entityComponentId.GetEntityId();

                AZ_Error(
                    "EditorSubComponentModeSnapPosition", selectedEntityIsNotJointEntity,
                    "Joint's lead entity cannot be the same as the entity in which the joint resides. Select lead entity on snap failed.");

                if (selectedEntityIsNotJointEntity)
                {
                    PhysX::EditorJointRequestBus::Event(
                        m_entityComponentId, &PhysX::EditorJointRequests::SetEntityIdValue, JointsComponentModeCommon::ParamaterNames::LeadEntity,
                        m_pickedEntity);
                }
            });
    }

    void JointsSubComponentModeSnapPosition::ResetValues([[maybe_unused]]const AZ::EntityComponentIdPair& idPair)
    {
        PhysX::EditorJointRequestBus::Event(
            m_entityComponentId, &PhysX::EditorJointRequests::SetVector3Value, JointsComponentModeCommon::ParamaterNames::Position, m_resetPosition);
        PhysX::EditorJointRequestBus::Event(
            m_entityComponentId, &PhysX::EditorJointRequests::SetEntityIdValue, JointsComponentModeCommon::ParamaterNames::LeadEntity, m_resetLeadEntity);
    }

    void JointsSubComponentModeSnapPosition::DisplaySpecificSnapType(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AZ::Vector3& jointPosition,
        const AZ::Vector3& snapDirection,
        float snapLength)
    {
        const float arrowLength = 1.0f;
        const float iconGap = 1.0f;
        const AZ::Vector3 iconPosition = jointPosition + (snapDirection * (snapLength + arrowLength + iconGap));

        debugDisplay.SetColor(AZ::Colors::Red);
        debugDisplay.DrawArrow(iconPosition, iconPosition + AZ::Vector3(arrowLength, 0.0f, 0.0f));
        debugDisplay.SetColor(AZ::Colors::Green);
        debugDisplay.DrawArrow(iconPosition, iconPosition + AZ::Vector3(0.2f, arrowLength, 0.2f));
        debugDisplay.SetColor(AZ::Colors::Blue);
        debugDisplay.DrawArrow(iconPosition, iconPosition + AZ::Vector3(0.0f, 0.0f, arrowLength));
    }

} // namespace PhysX
