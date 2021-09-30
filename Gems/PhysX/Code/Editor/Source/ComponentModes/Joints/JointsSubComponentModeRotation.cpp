/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Source/ComponentModes/Joints/JointsSubComponentModeRotation.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/Manipulators/AngularManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

#include <Editor/Source/ComponentModes/Joints/JointsComponentModeCommon.h>
#include <PhysX/EditorColliderComponentRequestBus.h>
#include <PhysX/EditorJointBus.h>
#include <Source/Utils.h>

namespace PhysX
{
    AZ_CLASS_ALLOCATOR_IMPL(JointsSubComponentModeRotation, AZ::SystemAllocator, 0);

    void JointsSubComponentModeRotation::Setup(const AZ::EntityComponentIdPair& idPair)
    {
        AZ::Transform worldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(idPair.GetEntityId());

        const AZ::Quaternion worldRotation = worldTransform.GetRotation();

        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EditorJointRequestBus::EventResult(
            localTransform, idPair, &EditorJointRequests::GetTransformValue, JointsComponentModeCommon::ParamaterNames::Transform);

        EditorJointRequestBus::EventResult(
            m_resetValue, idPair, &PhysX::EditorJointRequests::GetVector3Value, JointsComponentModeCommon::ParamaterNames::Rotation);

        const AZStd::array<AZ::Vector3, 3> axes = { AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ() };

        const AZStd::array<AZ::Color, 3> colors = { AZ::Color(1.0f, 0.0f, 0.0f, 1.0f), AZ::Color(0.0f, 1.0f, 0.0f, 1.0f),
                                                    AZ::Color(0.0f, 0.0f, 1.0f, 1.0f) };

        for (AZ::u32 i = 0; i < 3; ++i)
        {
            m_manipulators[i] = AzToolsFramework::AngularManipulator::MakeShared(worldTransform);
            m_manipulators[i]->AddEntityComponentIdPair(idPair);
            m_manipulators[i]->SetAxis(axes[i]);
            m_manipulators[i]->SetLocalTransform(localTransform);
            const float manipulatorRadius = 2.0f;
            m_manipulators[i]->SetView(AzToolsFramework::CreateManipulatorViewCircle(
                *m_manipulators[i], colors[i], manipulatorRadius,
                AzToolsFramework::ManipulatorCicleBoundWidth(), AzToolsFramework::DrawHalfDottedCircle));

            m_manipulators[i]->Register(AzToolsFramework::g_mainManipulatorManagerId);
        }
        InstallManipulatorMouseCallbacks(idPair);
    }

    void JointsSubComponentModeRotation::Refresh(const AZ::EntityComponentIdPair& idPair)
    {
        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EditorJointRequestBus::EventResult(
            localTransform, idPair, &EditorJointRequests::GetTransformValue, JointsComponentModeCommon::ParamaterNames::Transform);

        for (auto rotationManipulator : m_manipulators)
        {
            rotationManipulator->SetLocalTransform(localTransform);
        }
    }

    void JointsSubComponentModeRotation::Teardown(const AZ::EntityComponentIdPair& idPair)
    {
        for (auto rotationManipulator : m_manipulators)
        {
            rotationManipulator->RemoveEntityComponentIdPair(idPair);
            rotationManipulator->Unregister();
        }
    }

    void JointsSubComponentModeRotation::ResetValues(const AZ::EntityComponentIdPair& idPair)
    {
        EditorJointRequestBus::Event(
            idPair, &PhysX::EditorJointRequests::SetVector3Value, JointsComponentModeCommon::ParamaterNames::Rotation, m_resetValue);

        const AZ::Quaternion reset = AZ::Quaternion::CreateFromEulerAnglesDegrees(m_resetValue);
        for (auto manipulator : m_manipulators)
        {
            manipulator->SetLocalOrientation(reset);
        }
    }

    void JointsSubComponentModeRotation::InstallManipulatorMouseCallbacks(const AZ::EntityComponentIdPair& idPair)
    {
        struct SharedState
        {
            AZ::Transform m_startTM;
        };
        auto sharedState = AZStd::make_shared<SharedState>();

        auto mouseDownRotateXCallback =
            [sharedState, idPair]([[maybe_unused]] const AzToolsFramework::AngularManipulator::Action& action) mutable -> void
        {
            EditorJointRequestBus::EventResult(
                sharedState->m_startTM, idPair, &PhysX::EditorJointRequests::GetTransformValue, JointsComponentModeCommon::ParamaterNames::Transform);
        };

        for (AZ::u32 index = 0; index < 3; ++index)
        {
            m_manipulators[index]->InstallLeftMouseDownCallback(mouseDownRotateXCallback);

            m_manipulators[index]->InstallMouseMoveCallback(
                [this, index, sharedState, idPair](const AzToolsFramework::AngularManipulator::Action& action) mutable -> void
                {
                    const AZ::Quaternion manipulatorOrientation = action.m_start.m_rotation * action.m_current.m_delta;

                    AZ::Transform newTransform = AZ::Transform::CreateIdentity();
                    newTransform = sharedState->m_startTM * AZ::Transform::CreateFromQuaternion(action.m_current.m_delta);

                    EditorJointRequestBus::Event(
                        idPair, &PhysX::EditorJointRequests::SetVector3Value, JointsComponentModeCommon::ParamaterNames::Rotation,
                        newTransform.GetRotation().GetEulerDegrees());

                    m_manipulators[index]->SetLocalOrientation(manipulatorOrientation);
                });
        }
    }
} // namespace PhysX
