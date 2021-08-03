
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <PhysX_precompiled.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Color.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>

#include <Editor/EditorJointComponentMode.h>
#include <Editor/EditorSubComponentModeSnapPosition.h>
#include <PhysX/EditorJointBus.h>
#include <Source/Utils.h>

namespace PhysX
{
    EditorSubComponentModeSnapPosition::EditorSubComponentModeSnapPosition(
        const AZ::EntityComponentIdPair& entityComponentIdPair
        , const AZ::Uuid& componentType
        , const AZStd::string& name
        , bool selectLeadOnSnap)
        : EditorSubComponentModeSnap(entityComponentIdPair, componentType, name)
        , m_selectLeadOnSnap(selectLeadOnSnap)
    {
        InitMouseDownCallBack();
        m_manipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(m_entityComponentId.GetEntityId());
    }

    EditorSubComponentModeSnapPosition::~EditorSubComponentModeSnapPosition()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        m_manipulator->Unregister();
    }

    void EditorSubComponentModeSnapPosition::DisplaySpecificSnapType(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AZ::Vector3& jointPosition,
        const AZ::Vector3& snapDirection,
        const float snapLength)
    {
        const float arrowLength = 1.0f;
        const float iconGap = 1.0f;
        const AZ::Vector3 iconPosition = jointPosition + 
            (snapDirection * (snapLength + arrowLength + iconGap));

        debugDisplay.SetColor(AZ::Colors::Red);
        debugDisplay.DrawArrow(iconPosition, iconPosition + AZ::Vector3(arrowLength, 0.0f, 0.0f));
        debugDisplay.SetColor(AZ::Colors::Green);
        debugDisplay.DrawArrow(iconPosition, iconPosition + AZ::Vector3(0.2f, arrowLength, 0.2f));
        debugDisplay.SetColor(AZ::Colors::Blue);
        debugDisplay.DrawArrow(iconPosition, iconPosition + AZ::Vector3(0.0f, 0.0f, arrowLength));
    }

    void EditorSubComponentModeSnapPosition::InitMouseDownCallBack()
    {
        m_manipulator->InstallLeftMouseDownCallback(
            [this](const AzToolsFramework::LinearManipulator::Action& /*action*/) mutable
        {
            if (!m_pickedEntity.IsValid())
            {
                return;
            }

            const AZ::Vector3 newLocalPosition = PhysX::Utils::ComputeJointLocalTransform(
                PhysX::Utils::GetEntityWorldTransformWithScale(m_pickedEntity),
                PhysX::Utils::GetEntityWorldTransformWithScale(m_entityComponentId.GetEntityId())).GetTranslation();

            PhysX::EditorJointRequestBus::Event(m_entityComponentId
                , &PhysX::EditorJointRequests::SetVector3Value
                , PhysX::EditorJointComponentMode::s_parameterPosition
                , newLocalPosition);

            const bool selectedEntityIsNotJointEntity = m_pickedEntity != m_entityComponentId.GetEntityId();

            AZ_Error("EditorSubComponentModeSnapPosition",
                selectedEntityIsNotJointEntity,
                "Joint's lead entity cannot be the same as the entity in which the joint resides. Select lead entity on snap failed.");

            if (m_selectLeadOnSnap && 
                selectedEntityIsNotJointEntity)
            {
                PhysX::EditorJointRequestBus::Event(m_entityComponentId
                    , &PhysX::EditorJointRequests::SetEntityIdValue
                    , PhysX::EditorJointComponentMode::s_parameterLeadEntity
                    , m_pickedEntity);
            }

            m_manipulator->SetBoundsDirty();
        });
    }
} // namespace PhysX
