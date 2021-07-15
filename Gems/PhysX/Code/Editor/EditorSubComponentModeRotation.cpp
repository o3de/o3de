
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <PhysX_precompiled.h>

#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/Manipulators/AngularManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>

#include <Editor/EditorJointComponentMode.h>
#include <PhysX/EditorJointBus.h>
#include <Editor/EditorSubComponentModeRotation.h>
#include <Source/Utils.h>

namespace PhysX
{
    EditorSubComponentModeRotation::EditorSubComponentModeRotation(
        const AZ::EntityComponentIdPair& entityComponentIdPair
        , const AZ::Uuid& componentType
        , const AZStd::string& name)
        : EditorSubComponentModeBase(entityComponentIdPair, componentType, name)
    {
        CreateManipulators();
        RegisterManipulators();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(m_entityComponentId.GetEntityId());
    }

    EditorSubComponentModeRotation::~EditorSubComponentModeRotation()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        UnregisterManipulators();
    }

    void EditorSubComponentModeRotation::Refresh()
    {
        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EditorJointRequestBus::EventResult(
            localTransform, m_entityComponentId
            , &EditorJointRequests::GetTransformValue
            , PhysX::EditorJointComponentMode::s_parameterTransform);

        for (auto rotationManipulator : m_rotationManipulators)
        {
            rotationManipulator->SetLocalTransform(localTransform);
            rotationManipulator->SetBoundsDirty();
        }
    }

    void EditorSubComponentModeRotation::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        [[maybe_unused]] AzFramework::DebugDisplayRequests& debugDisplay)
    {
        Refresh(); //Update position and orientation of manipulators.
    }

    void EditorSubComponentModeRotation::CreateManipulators()
    {
        AZ::Transform worldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(m_entityComponentId.GetEntityId());

        const AZ::Quaternion worldRotation = worldTransform.GetRotation();

        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EditorJointRequestBus::EventResult(
            localTransform, m_entityComponentId
            , &EditorJointRequests::GetTransformValue
            , PhysX::EditorJointComponentMode::s_parameterTransform);

        const AZStd::array<AZ::Vector3, 3> axes = { AZ::Vector3::CreateAxisX()
            , AZ::Vector3::CreateAxisY()
            , AZ::Vector3::CreateAxisZ()};

        const AZStd::array <AZ::Color, 3> colors = { AZ::Color(1.0f, 0.0f, 0.0f, 1.0f)
            , AZ::Color(0.0f, 1.0f, 0.0f, 1.0f)
            , AZ::Color(0.0f, 0.0f, 1.0f, 1.0f)};

        for (AZ::u32 i = 0; i < 3; ++i)
        {
            m_rotationManipulators[i] = AzToolsFramework::AngularManipulator::MakeShared(worldTransform);
            m_rotationManipulators[i]->AddEntityComponentIdPair(m_entityComponentId);
            m_rotationManipulators[i]->SetAxis(axes[i]);
            m_rotationManipulators[i]->SetLocalTransform(localTransform);
            const float manipulatorRadius = 2.0f;
            const float manipulatorWidth = 0.05f;
            m_rotationManipulators[i]->SetView(AzToolsFramework::CreateManipulatorViewCircle(
                *m_rotationManipulators[i], colors[i],
                manipulatorRadius, manipulatorWidth, AzToolsFramework::DrawHalfDottedCircle));
        }
        
        Refresh();
        InstallManipulatorMouseCallbacks();
    }

    void EditorSubComponentModeRotation::InstallManipulatorMouseCallbacks()
    {
        struct SharedState
        {
            AZ::Transform m_startTM;
        };
        auto sharedState = AZStd::make_shared<SharedState>();

        auto mouseDownRotateXCallback = [this, sharedState]([[maybe_unused]] const AzToolsFramework::AngularManipulator::Action& action) mutable -> void
        {
            PhysX::EditorJointRequestBus::EventResult(sharedState->m_startTM
                , m_entityComponentId
                , &PhysX::EditorJointRequests::GetTransformValue
                , PhysX::EditorJointComponentMode::s_parameterTransform);
        };

        for (AZ::u32 index = 0; index < 3; ++index)
        {
            m_rotationManipulators[index]->InstallLeftMouseDownCallback(mouseDownRotateXCallback);

            m_rotationManipulators[index]->InstallMouseMoveCallback(
                [this, index, sharedState]
            (const AzToolsFramework::AngularManipulator::Action& action) mutable -> void
            {
                const AZ::Quaternion manipulatorOrientation = action.m_start.m_rotation * action.m_current.m_delta;

                AZ::Transform newTransform = AZ::Transform::CreateIdentity();
                newTransform = sharedState->m_startTM * AZ::Transform::CreateFromQuaternion(action.m_current.m_delta);

                PhysX::EditorJointRequestBus::Event(m_entityComponentId
                    , &PhysX::EditorJointRequests::SetVector3Value
                    , PhysX::EditorJointComponentMode::s_parameterRotation
                    , newTransform.GetRotation().GetEulerDegrees());

                m_rotationManipulators[index]->SetLocalOrientation(manipulatorOrientation);
                m_rotationManipulators[index]->SetBoundsDirty();
            });
        }
    }

    void EditorSubComponentModeRotation::RegisterManipulators()
    {
        for (auto rotationManipulator : m_rotationManipulators)
        {
            rotationManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
        }
    }

    void EditorSubComponentModeRotation::UnregisterManipulators()
    {
        for (auto rotationManipulator : m_rotationManipulators)
        {
            rotationManipulator->Unregister();
        }
    }
}
