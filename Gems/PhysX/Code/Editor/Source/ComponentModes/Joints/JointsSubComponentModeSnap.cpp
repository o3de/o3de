/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Editor/Source/ComponentModes/Joints/JointsSubComponentModeSnap.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

#include <Editor/Source/ComponentModes/Joints/JointsComponentModeCommon.h>
#include <PhysX/EditorJointBus.h>
#include <Source/Utils.h>

namespace PhysX
{
    AZ_CLASS_ALLOCATOR_IMPL(JointsSubComponentModeSnap, AZ::SystemAllocator);

    void JointsSubComponentModeSnap::Setup(const AZ::EntityComponentIdPair& idPair)
    {
        m_entityComponentId = idPair;

        AZ::Transform worldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(m_entityComponentId.GetEntityId());

        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EditorJointRequestBus::EventResult(
            localTransform, m_entityComponentId, &EditorJointRequests::GetTransformValue, JointsComponentModeCommon::ParameterNames::Transform);

        m_manipulator = AzToolsFramework::LinearManipulator::MakeShared(worldTransform);
        m_manipulator->AddEntityComponentIdPair(m_entityComponentId);
        m_manipulator->SetAxis(AZ::Vector3::CreateAxisX());
        m_manipulator->SetLocalTransform(localTransform);

        Refresh(idPair);

        const AZ::Color manipulatorColor(0.3f, 0.3f, 0.3f, 1.0f);
        const float manipulatorSize = 0.05f;
        AzToolsFramework::ManipulatorViews views;
        views.emplace_back(AzToolsFramework::CreateManipulatorViewQuadBillboard(manipulatorColor, manipulatorSize));
        m_manipulator->SetViews(AZStd::move(views));

        m_manipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(m_entityComponentId.GetEntityId());
    }

    void JointsSubComponentModeSnap::Refresh(const AZ::EntityComponentIdPair& idPair)
    {
        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EditorJointRequestBus::EventResult(
            localTransform, idPair, &EditorJointRequests::GetTransformValue, JointsComponentModeCommon::ParameterNames::Transform);

        m_manipulator->SetLocalTransform(localTransform);
    }

    void JointsSubComponentModeSnap::Teardown(const AZ::EntityComponentIdPair& idPair)
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();

        m_manipulator->RemoveEntityComponentIdPair(idPair);
        m_manipulator->Unregister();
        m_manipulator.reset();
    }

    void JointsSubComponentModeSnap::HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        if (mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Move)
        {
            const int viewportId = mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId;
            const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportId);
            m_pickedEntity = m_picker.PickEntity(cameraState, mouseInteraction, m_pickedPosition, m_pickedEntityAabb);

            if (m_pickedEntity.IsValid())
            {
                AZ::Transform worldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(m_entityComponentId.GetEntityId());
                const AZ::Quaternion worldRotate = worldTransform.GetRotation();
                const AZ::Quaternion worldRotateInv = worldRotate.GetInverseFull();

                m_manipulator->SetLocalPosition(worldRotateInv.TransformVector(m_pickedPosition - worldTransform.GetTranslation()));
            }
        }
    }

    AZStd::string JointsSubComponentModeSnap::GetPickedEntityName()
    {
        AZStd::string pickedEntityName;
        if (m_pickedEntity.IsValid())
        {
            AZ::ComponentApplicationBus::BroadcastResult(
                pickedEntityName, &AZ::ComponentApplicationRequests::GetEntityName, m_pickedEntity);
        }
        return pickedEntityName;
    }

    AZ::Vector3 JointsSubComponentModeSnap::GetPosition() const
    {
        AZ::Transform worldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(m_entityComponentId.GetEntityId());

        AZ::Quaternion worldRotate = worldTransform.GetRotation();

        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EditorJointRequestBus::EventResult(
            localTransform, m_entityComponentId, &EditorJointRequests::GetTransformValue, JointsComponentModeCommon::ParameterNames::Transform);

        return worldTransform.GetTranslation() + worldRotate.TransformVector(localTransform.GetTranslation());
    }

    void JointsSubComponentModeSnap::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ::u32 stateBefore = debugDisplay.GetState();

        AZ::Vector3 position = GetPosition();

        AZ::Transform worldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(m_entityComponentId.GetEntityId());

        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EditorJointRequestBus::EventResult(
            localTransform, m_entityComponentId, &EditorJointRequests::GetTransformValue, JointsComponentModeCommon::ParameterNames::Transform);

        debugDisplay.PushMatrix(worldTransform);
        debugDisplay.PushMatrix(localTransform);

        const float xAxisLineLength = 15.0f;
        debugDisplay.SetColor(AZ::Color(1.0f, 0.0f, 0.0f, 1.0f));
        debugDisplay.DrawLine(AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(xAxisLineLength, 0.0f, 0.0f));

        AngleLimitsFloatPair yzSwingAngleLimits;
        EditorJointRequestBus::EventResult(
            yzSwingAngleLimits, m_entityComponentId, &EditorJointRequests::GetLinearValuePair, JointsComponentModeCommon::ParameterNames::SwingLimit);

        const AZ::u32 numEllipseSamples = 16;
        AZStd::array<AZ::Vector3, numEllipseSamples> ellipseSamples;
        float coneHeight = 3.0f;

        // Draw inverted cone if angles are larger than 90 deg.
        if (yzSwingAngleLimits.first > 90.0f || yzSwingAngleLimits.second > 90.0f)
        {
            coneHeight = -3.0f;
        }

        // Compute points along perimeter of cone base
        const float coney = tan(AZ::DegToRad(yzSwingAngleLimits.first)) * coneHeight;
        const float conez = tan(AZ::DegToRad(yzSwingAngleLimits.second)) * coneHeight;
        const float step = AZ::Constants::TwoPi / numEllipseSamples;
        for (size_t i = 0; i < numEllipseSamples; ++i)
        {
            const float angleStep = step * i;
            ellipseSamples[i].SetX(coneHeight);
            ellipseSamples[i].SetY(conez * sin(angleStep));
            ellipseSamples[i].SetZ(coney * cos(angleStep));
        }

        // draw cone
        for (size_t i = 0; i < numEllipseSamples; ++i)
        {
            size_t nextIndex = i + 1;
            if (i == numEllipseSamples - 1)
            {
                nextIndex = 0;
            }

            // draw cone sides
            debugDisplay.SetColor(AZ::Color(1.0f, 1.0f, 1.0f, 0.2f));
            debugDisplay.DrawTri(AZ::Vector3(0.0f, 0.0f, 0.0f), ellipseSamples[i], ellipseSamples[nextIndex]);

            // draw parameter of cone base
            debugDisplay.SetColor(AZ::Color(0.4f, 0.4f, 0.4f, 0.4f));
            debugDisplay.DrawLine(ellipseSamples[i], ellipseSamples[nextIndex]);
        }

        // draw axis lines at base of cone, and from tip to base.
        debugDisplay.SetColor(AZ::Color(0.5f, 0.5f, 0.5f, 0.6f));
        debugDisplay.DrawLine(ellipseSamples[0], ellipseSamples[numEllipseSamples / 2]);
        debugDisplay.DrawLine(ellipseSamples[numEllipseSamples * 3 / 4], ellipseSamples[numEllipseSamples / 4]);
        debugDisplay.DrawLine(AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(coneHeight, 0.0f, 0.0f));

        debugDisplay.PopMatrix(); // pop local transform
        debugDisplay.PopMatrix(); // pop world transform

        // draw line from joint to mouse-over entity
        if (m_pickedEntity.IsValid())
        {
            const AZ::Vector3 direction = (m_pickedPosition - position);
            const float directionLength = direction.GetLength();
            const AZ::Vector3 directionNorm = direction.GetNormalized();

            const float LineExtend = 1.0f; ///< Distance snap line extends beyond the snapped entity when drawn.

            const AZ::Color lineColor(0.0f, 1.0f, 0.0f, 1.0f);
            debugDisplay.SetColor(lineColor);

            debugDisplay.DrawLine(position, position + (directionNorm * (directionLength + LineExtend)));
            debugDisplay.DrawWireBox(m_pickedEntityAabb.GetMin(), m_pickedEntityAabb.GetMax());

            // draw something, e.g. an icon, to indicate type of snapping
            DisplaySpecificSnapType(viewportInfo, debugDisplay, position, directionNorm, directionLength);
        }

        debugDisplay.SetState(stateBefore);
    }
} // namespace PhysX
