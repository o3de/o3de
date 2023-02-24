/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Source/ComponentModes/Joints/JointsSubComponentModeAnglePair.h>

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzFramework/Physics/Configuration/JointConfiguration.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>

#include <Editor/Source/ComponentModes/Joints/JointsComponentModeCommon.h>
#include <PhysX/EditorJointBus.h>
#include <Source/Utils.h>

namespace PhysX
{
    AZ_CLASS_ALLOCATOR_IMPL(JointsSubComponentModeAnglePair, AZ::SystemAllocator);

    JointsSubComponentModeAnglePair::JointsSubComponentModeAnglePair(
        const AZStd::string& propertyName, const AZ::Vector3& axis, float max, float min)
        : m_propertyName(propertyName)
        , m_axis(axis)
        , m_firstMax(max)
        , m_firstMin(min)
        , m_secondMax(min)
        , m_secondMin(-max)
    {

    }

    void JointsSubComponentModeAnglePair::Setup(const AZ::EntityComponentIdPair& idPair)
    {
        m_entityComponentIdPair = idPair;
        EditorJointRequestBus::EventResult(m_resetValue, idPair, &EditorJointRequests::GetLinearValuePair, m_propertyName);

        const AZ::Transform worldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(idPair.GetEntityId());

        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EditorJointRequestBus::EventResult(
            localTransform, idPair, &EditorJointRequests::GetTransformValue, JointsComponentModeCommon::ParameterNames::Transform);
        const AZ::Quaternion localRotation = localTransform.GetRotation();

        AZ::Vector3 displacement = m_axis;
        AZ::Transform displacementTransform = localTransform;
        AZ::Vector3 displacementTranslate = localRotation.TransformVector(displacement);
        displacementTransform.SetTranslation(localTransform.GetTranslation() + displacementTranslate);

        m_firstManipulator = AzToolsFramework::AngularManipulator::MakeShared(worldTransform);
        m_firstManipulator->AddEntityComponentIdPair(idPair);
        m_firstManipulator->SetAxis(m_axis);
        m_firstManipulator->SetLocalTransform(displacementTransform);

        displacement = -m_axis;
        displacementTranslate = localRotation.TransformVector(displacement);
        displacementTransform.SetTranslation(localTransform.GetTranslation() + displacementTranslate);
        m_secondManipulator = AzToolsFramework::AngularManipulator::MakeShared(worldTransform);
        m_secondManipulator->AddEntityComponentIdPair(idPair);
        m_secondManipulator->SetAxis(m_axis);
        m_secondManipulator->SetLocalTransform(displacementTransform);

        const float manipulatorRadius = 2.0f;
        const float manipulatorWidth = 0.05f;
        m_firstManipulator->SetView(AzToolsFramework::CreateManipulatorViewCircle(
            *m_firstManipulator, AzPhysics::JointVisualizationDefaults::ColorFirst, manipulatorRadius, manipulatorWidth,
            AzToolsFramework::DrawHalfDottedCircle));

        m_secondManipulator->SetView(AzToolsFramework::CreateManipulatorViewCircle(
            *m_secondManipulator, AzPhysics::JointVisualizationDefaults::ColorSecond, manipulatorRadius, manipulatorWidth,
            AzToolsFramework::DrawHalfDottedCircle));

        Refresh(idPair);

        m_sharedRotationState = AZStd::make_shared<JointsComponentModeCommon::SubComponentModes::AngleModesSharedRotationState>();

        auto mouseDownCallback = [this,
                                  idPair](const AzToolsFramework::AngularManipulator::Action& action) mutable -> void
        {
            const AZ::Quaternion normalizedStart = action.m_start.m_rotation.GetNormalized();
            m_sharedRotationState->m_axis = AZ::Vector3(normalizedStart.GetX(), normalizedStart.GetY(), normalizedStart.GetZ());
            m_sharedRotationState->m_savedOrientation = AZ::Quaternion::CreateIdentity();

            AngleLimitsFloatPair currentValue;
            EditorJointRequestBus::EventResult(currentValue, idPair, &EditorJointRequests::GetLinearValuePair, m_propertyName);

            m_sharedRotationState->m_valuePair = currentValue;
        };

        m_firstManipulator->InstallLeftMouseDownCallback(mouseDownCallback);

        m_secondManipulator->InstallLeftMouseDownCallback(mouseDownCallback);

        m_firstManipulator->InstallMouseMoveCallback(
            [this, idPair](const AzToolsFramework::AngularManipulator::Action& action) mutable -> void
            {
                float angleDelta;
                AZ::Quaternion manipulatorOrientation;
                const float newValue = MouseMove(m_sharedRotationState, action, true, angleDelta, manipulatorOrientation);
                if (newValue > m_firstMax || newValue < m_firstMin)
                {
                    return;
                }
                m_firstManipulator->SetLocalOrientation(manipulatorOrientation);
                const float newFirstValue = AZ::GetClamp(m_sharedRotationState->m_valuePair.first + angleDelta, m_firstMin, m_firstMax);

                EditorJointRequestBus::Event(
                    idPair, &EditorJointRequests::SetLinearValuePair, m_propertyName,
                    AngleLimitsFloatPair(newFirstValue, m_sharedRotationState->m_valuePair.second));
            });

        m_secondManipulator->InstallMouseMoveCallback(
            [this, idPair](const AzToolsFramework::AngularManipulator::Action& action) mutable -> void
            {
                float angleDelta;
                AZ::Quaternion manipulatorOrientation;
                const float newValue = MouseMove(m_sharedRotationState, action, false, angleDelta, manipulatorOrientation);
                if (newValue > m_secondMax || newValue < m_secondMin)
                {
                    return; // Not handling values exceeding limits
                }

                m_secondManipulator->SetLocalOrientation(manipulatorOrientation);
                float newSecondValue = AZ::GetClamp(m_sharedRotationState->m_valuePair.second + angleDelta, m_secondMin, m_secondMax);

                EditorJointRequestBus::Event(
                    idPair, &EditorJointRequests::SetLinearValuePair, m_propertyName,
                    AngleLimitsFloatPair(m_sharedRotationState->m_valuePair.first, newSecondValue));
            });

        m_firstManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
        m_secondManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);

        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(idPair.GetEntityId());
    }

    void JointsSubComponentModeAnglePair::Refresh(const AZ::EntityComponentIdPair& idPair)
    {
        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EditorJointRequestBus::EventResult(
            localTransform, idPair, &EditorJointRequests::GetTransformValue, JointsComponentModeCommon::ParameterNames::Transform);
        const AZ::Quaternion localRotation = localTransform.GetRotation();

        AZ::Transform displacementTransform = localTransform;
        AZ::Vector3 displacement = m_axis;
        AZ::Vector3 displacementTranslate = localRotation.TransformVector(displacement);
        displacementTransform.SetTranslation(localTransform.GetTranslation() + displacementTranslate);
        m_firstManipulator->SetLocalTransform(displacementTransform);

        displacement = -m_axis;
        displacementTranslate = localRotation.TransformVector(displacement);
        displacementTransform.SetTranslation(localTransform.GetTranslation() + displacementTranslate);
        m_secondManipulator->SetLocalTransform(displacementTransform);
    }

    void JointsSubComponentModeAnglePair::Teardown(const AZ::EntityComponentIdPair& idPair)
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        m_firstManipulator->RemoveEntityComponentIdPair(idPair);
        m_firstManipulator->Unregister();
        m_secondManipulator->RemoveEntityComponentIdPair(idPair);
        m_secondManipulator->Unregister();
    }

    void JointsSubComponentModeAnglePair::ResetValues(const AZ::EntityComponentIdPair& idPair)
    {
        EditorJointRequestBus::Event(idPair, &EditorJointRequests::SetLinearValuePair, m_propertyName, m_resetValue);

        m_firstManipulator->SetLocalOrientation(AZ::Quaternion::CreateFromAxisAngle(m_axis, m_resetValue.first));
        m_secondManipulator->SetLocalOrientation(AZ::Quaternion::CreateFromAxisAngle(m_axis, m_resetValue.first));
    }

    float JointsSubComponentModeAnglePair::MouseMove(
        AZStd::shared_ptr<JointsComponentModeCommon::SubComponentModes::AngleModesSharedRotationState>& sharedRotationState,
        const AzToolsFramework::AngularManipulator::Action& action,
        bool isFirstValue,
        float& angleDelta,
        AZ::Quaternion& manipulatorOrientation)
    {
        sharedRotationState->m_savedOrientation = action.m_current.m_delta.GetInverseFull();
        angleDelta = 0.0f;
        AZ::Vector3 axis = m_axis;
        sharedRotationState->m_savedOrientation.ConvertToAxisAngle(axis, angleDelta);
        // Polarity of axis is switched by ConvertToAxisAngle call depending on direction of rotation
        if (abs(m_axis.GetX() - 1.0f) < FLT_EPSILON)
        {
            angleDelta = AZ::RadToDeg(angleDelta) * axis.GetX();
        }
        else if (abs(m_axis.GetY() - 1.0f) < FLT_EPSILON)
        {
            angleDelta = AZ::RadToDeg(angleDelta) * axis.GetY();
        }
        else if (abs(m_axis.GetZ() - 1.0f) < FLT_EPSILON)
        {
            angleDelta = AZ::RadToDeg(angleDelta) * axis.GetZ();
        }

        manipulatorOrientation = action.m_start.m_rotation * action.m_current.m_delta;

        if (isFirstValue)
        {
            return sharedRotationState->m_valuePair.first + angleDelta;
        }
        return sharedRotationState->m_valuePair.second + angleDelta;
    }

    void JointsSubComponentModeAnglePair::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AngleLimitsFloatPair currentValue;
        EditorJointRequestBus::EventResult(currentValue, m_entityComponentIdPair, &EditorJointRequests::GetLinearValuePair, m_propertyName);

        const float size = 2.0f;
        AZ::Vector3 axisPoint = m_axis * size * 0.5f;

        AZStd::array<AZ::Vector3, 4> points = { -axisPoint, axisPoint, axisPoint, -axisPoint };

        if (abs(m_axis.GetX() - 1.0f) < FLT_EPSILON)
        {
            points[2].SetZ(size);
            points[3].SetZ(size);
        }
        else if (abs(m_axis.GetY() - 1.0f) < FLT_EPSILON)
        {
            points[2].SetX(size);
            points[3].SetX(size);
        }
        else if (abs(m_axis.GetZ() - 1.0f) < FLT_EPSILON)
        {
            points[2].SetX(size);
            points[3].SetX(size);
        }

        AZ::u32 stateBefore = debugDisplay.GetState();
        debugDisplay.CullOff();
        debugDisplay.SetAlpha(AzPhysics::JointVisualizationDefaults::Alpha);

        AZ::Transform worldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(m_entityComponentIdPair.GetEntityId());

        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EditorJointRequestBus::EventResult(
            localTransform, m_entityComponentIdPair, &EditorJointRequests::GetTransformValue, JointsComponentModeCommon::ParameterNames::Transform);

        debugDisplay.PushMatrix(worldTransform);
        debugDisplay.PushMatrix(localTransform);

        debugDisplay.SetColor(AzPhysics::JointVisualizationDefaults::ColorSweepArc);

        const AZ::Vector3 zeroVector = AZ::Vector3::CreateZero();
        const AZ::Vector3 posPosition = m_axis * AzPhysics::JointVisualizationDefaults::SweepLineDisplaceFactor;
        const AZ::Vector3 negPosition = -posPosition;
        debugDisplay.DrawArc(
            posPosition, AzPhysics::JointVisualizationDefaults::SweepLineThickness, -currentValue.first, currentValue.first,
            AzPhysics::JointVisualizationDefaults::SweepLineGranularity, -m_axis);
        debugDisplay.DrawArc(
            zeroVector, AzPhysics::JointVisualizationDefaults::SweepLineThickness, -currentValue.first, currentValue.first,
            AzPhysics::JointVisualizationDefaults::SweepLineGranularity, -m_axis);
        debugDisplay.DrawArc(
            negPosition, AzPhysics::JointVisualizationDefaults::SweepLineThickness, -currentValue.first, currentValue.first,
            AzPhysics::JointVisualizationDefaults::SweepLineGranularity, -m_axis);
        debugDisplay.DrawArc(
            posPosition, AzPhysics::JointVisualizationDefaults::SweepLineThickness, 0.0f, abs(currentValue.second),
            AzPhysics::JointVisualizationDefaults::SweepLineGranularity, -m_axis);
        debugDisplay.DrawArc(
            zeroVector, AzPhysics::JointVisualizationDefaults::SweepLineThickness, 0.0f, abs(currentValue.second),
            AzPhysics::JointVisualizationDefaults::SweepLineGranularity, -m_axis);
        debugDisplay.DrawArc(
            negPosition, AzPhysics::JointVisualizationDefaults::SweepLineThickness, 0.0f, abs(currentValue.second),
            AzPhysics::JointVisualizationDefaults::SweepLineGranularity, -m_axis);

        AZ::Quaternion firstRotate = AZ::Quaternion::CreateFromAxisAngle(m_axis, AZ::DegToRad(currentValue.first));
        AZ::Transform firstTM = AZ::Transform::CreateFromQuaternion(firstRotate);
        debugDisplay.PushMatrix(firstTM);
        debugDisplay.SetColor(AzPhysics::JointVisualizationDefaults::ColorFirst);
        debugDisplay.DrawQuad(points[0], points[1], points[2], points[3]);
        debugDisplay.PopMatrix();

        AZ::Quaternion secondRotate = AZ::Quaternion::CreateFromAxisAngle(m_axis, AZ::DegToRad(currentValue.second));
        AZ::Transform secondTM = AZ::Transform::CreateFromQuaternion(secondRotate);
        debugDisplay.PushMatrix(secondTM);
        debugDisplay.SetColor(AzPhysics::JointVisualizationDefaults::ColorSecond);
        debugDisplay.DrawQuad(points[0], points[1], points[2], points[3]);
        debugDisplay.PopMatrix();

        debugDisplay.SetColor(AzPhysics::JointVisualizationDefaults::ColorDefault);
        debugDisplay.DrawQuad(points[0], points[1], points[2], points[3]);

        debugDisplay.PopMatrix(); // pop local transform
        debugDisplay.PopMatrix(); // pop global transform
        debugDisplay.SetState(stateBefore);

        // reposition and reorientate manipulators
        Refresh(m_entityComponentIdPair);
    }

} // namespace PhysX
