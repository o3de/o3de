
/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */
#include <PhysX_precompiled.h>

#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>

#include <Editor/EditorSubComponentModeVec3.h>
#include <PhysX/EditorJointBus.h>
#include <Source/Utils.h>

namespace PhysX
{
    EditorSubComponentModeVec3::EditorSubComponentModeVec3(
        const AZ::EntityComponentIdPair& entityComponentIdPair
        , const AZ::Uuid& componentType
        , const AZStd::string& name)
        : EditorSubComponentModeBase(entityComponentIdPair, componentType, name)
        , m_translationManipulators(AzToolsFramework::TranslationManipulators::Dimensions::Three, AZ::Transform::Identity())
    {
        AZ::Transform worldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(m_entityComponentId.GetEntityId());

        AZ::Vector3 localTranslation;
        EditorJointRequestBus::EventResult(
            localTranslation, m_entityComponentId
            , &EditorJointRequests::GetVector3Value
            , m_name);

        m_translationManipulators.SetSpace(worldTransform);
        m_translationManipulators.SetLocalPosition(localTranslation);
        m_translationManipulators.AddEntityComponentIdPair(m_entityComponentId);

        m_translationManipulators.Register(AzToolsFramework::g_mainManipulatorManagerId);

        AzToolsFramework::ConfigureTranslationManipulatorAppearance3d(&m_translationManipulators);

        m_translationManipulators.InstallLinearManipulatorMouseMoveCallback([this](
            const AzToolsFramework::LinearManipulator::Action& action)
        {
            OnManipulatorMoved(action.LocalPosition());
        });

        m_translationManipulators.InstallPlanarManipulatorMouseMoveCallback([this](
            const AzToolsFramework::PlanarManipulator::Action& action)
        {
            OnManipulatorMoved(action.LocalPosition());
        });

        m_translationManipulators.InstallSurfaceManipulatorMouseMoveCallback([this](
            const AzToolsFramework::SurfaceManipulator::Action& action)
        {
            OnManipulatorMoved(action.LocalPosition());
        });
    }

    EditorSubComponentModeVec3::~EditorSubComponentModeVec3()
    {
        m_translationManipulators.Unregister();
    }

    void EditorSubComponentModeVec3::Refresh()
    {
        AZ::Transform worldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(m_entityComponentId.GetEntityId());

        AZ::Vector3 localTranslation;
        EditorJointRequestBus::EventResult(
            localTranslation, m_entityComponentId
            , &EditorJointRequests::GetVector3Value
            , m_name);

        m_translationManipulators.SetSpace(worldTransform);
        m_translationManipulators.SetLocalPosition(localTranslation);
        m_translationManipulators.SetBoundsDirty();
    }

    void EditorSubComponentModeVec3::OnManipulatorMoved(const AZ::Vector3& position)
    {
        m_translationManipulators.SetLocalPosition(position);
        EditorJointRequestBus::Event(
            m_entityComponentId
            , &EditorJointRequests::SetVector3Value
            , m_name
            , position);
    }
} // namespace PhysX
