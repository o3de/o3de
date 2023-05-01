/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Source/ComponentModes/Joints/JointsSubComponentModeTranslate.h>

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>

#include <Editor/Source/ComponentModes/Joints/JointsComponentModeCommon.h>
#include <PhysX/EditorJointBus.h>
#include <Source/Utils.h>

namespace PhysX
{
    AZ_CLASS_ALLOCATOR_IMPL(JointsSubComponentModeTranslation, AZ::SystemAllocator);

    JointsSubComponentModeTranslation::JointsSubComponentModeTranslation()
        : m_manipulator(
              AzToolsFramework::TranslationManipulators::Dimensions::Three, AZ::Transform::Identity(), AZ::Vector3::CreateOne())
    {
    }

    void JointsSubComponentModeTranslation::Setup(const AZ::EntityComponentIdPair& idPair)
    {
        AZ::Transform worldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(idPair.GetEntityId());

        EditorJointRequestBus::EventResult(
            m_resetValue, idPair, &EditorJointRequests::GetVector3Value, JointsComponentModeCommon::ParameterNames::Position);

        m_manipulator.SetSpace(worldTransform);
        m_manipulator.SetLocalPosition(m_resetValue);

        m_manipulator.AddEntityComponentIdPair(idPair);
        m_manipulator.Register(AzToolsFramework::g_mainManipulatorManagerId);
        AzToolsFramework::ConfigureTranslationManipulatorAppearance3d(&m_manipulator);
        m_manipulator.InstallLinearManipulatorMouseMoveCallback(
            [this, idPair](const AzToolsFramework::LinearManipulator::Action& action)
            {
                OnManipulatorMoved(action.LocalPosition(), idPair);
            });

        m_manipulator.InstallPlanarManipulatorMouseMoveCallback(
            [this, idPair](const AzToolsFramework::PlanarManipulator::Action& action)
            {
                OnManipulatorMoved(action.LocalPosition(), idPair);
            });

        m_manipulator.InstallSurfaceManipulatorMouseMoveCallback(
            [this, idPair](const AzToolsFramework::SurfaceManipulator::Action& action)
            {
                OnManipulatorMoved(action.LocalPosition(), idPair);
            });
    }

    void JointsSubComponentModeTranslation::Refresh(const AZ::EntityComponentIdPair& idPair)
    {
        AZ::Vector3 localTranslation;
        EditorJointRequestBus::EventResult(
            localTranslation, idPair, &EditorJointRequests::GetVector3Value, JointsComponentModeCommon::ParameterNames::Position);
        m_manipulator.SetLocalPosition(localTranslation);
    }

    void JointsSubComponentModeTranslation::Teardown(const AZ::EntityComponentIdPair& idPair)
    {
        m_manipulator.RemoveEntityComponentIdPair(idPair);
        m_manipulator.Unregister();
    }

    void JointsSubComponentModeTranslation::ResetValues(const AZ::EntityComponentIdPair& idPair)
    {
        PhysX::EditorJointRequestBus::Event(
            idPair, &EditorJointRequests::SetVector3Value, JointsComponentModeCommon::ParameterNames::Position, m_resetValue);
        m_manipulator.SetLocalPosition(m_resetValue);
    }

    void JointsSubComponentModeTranslation::OnManipulatorMoved(const AZ::Vector3& position, const AZ::EntityComponentIdPair& idPair)
    {
        m_manipulator.SetLocalPosition(position);
        PhysX::EditorJointRequestBus::Event(
            idPair, &EditorJointRequests::SetVector3Value, JointsComponentModeCommon::ParameterNames::Position, position);
    }

} // namespace PhysX
