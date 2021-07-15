
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <PhysX_precompiled.h>

#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>

#include <Editor/EditorJointComponentMode.h>
#include <Editor/EditorSubComponentModeAnglePair.h>
#include <Source/Utils.h>

namespace
{
    const float Alpha = 0.6f;
    const AZ::Color ColorDefault = AZ::Color(1.0f, 1.0f, 1.0f, Alpha);
    const AZ::Color ColorFirst = AZ::Color(1.0f, 0.0f, 0.0f, Alpha);
    const AZ::Color ColorSecond = AZ::Color(0.0f, 1.0f, 0.0f, Alpha);
    const AZ::Color ColorSweepArc = AZ::Color(1.0f, 1.0f, 1.0f, Alpha);

    const float SweepLineDisplaceFactor = 0.5f;
    const float SweepLineThickness = 1.0f;
    const float SweepLineGranularity = 1.0f;
}

namespace PhysX
{
    EditorSubComponentModeAnglePair::EditorSubComponentModeAnglePair(
        const AZ::EntityComponentIdPair& entityComponentIdPair
        , const AZ::Uuid& componentType
        , const AZStd::string& name
        , const AZ::Vector3& axis
        , float firstMax
        , float firstMin
        , float secondMax
        , float secondMin)
        : EditorSubComponentModeBase(entityComponentIdPair, componentType, name)
        , m_axis(axis)
        , m_firstMax(firstMax)
        , m_firstMin(firstMin)
        , m_secondMax(secondMax)
        , m_secondMin(secondMin)
    {
        AZ::Transform worldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(m_entityComponentId.GetEntityId());

        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EditorJointRequestBus::EventResult(
            localTransform, m_entityComponentId
            , &EditorJointRequests::GetTransformValue
            , PhysX::EditorJointComponentMode::s_parameterTransform);
        const AZ::Quaternion localRotation = localTransform.GetRotation();

        AZ::Vector3 displacement = m_axis;
        AZ::Transform displacementTransform = localTransform;
        AZ::Vector3 displacementTranslate = localRotation.TransformVector(displacement);
        displacementTransform.SetTranslation(localTransform.GetTranslation() + displacementTranslate);

        m_firstManipulator = AzToolsFramework::AngularManipulator::MakeShared(worldTransform);
        m_firstManipulator->AddEntityComponentIdPair(m_entityComponentId);
        m_firstManipulator->SetAxis(m_axis);
        m_firstManipulator->SetLocalTransform(displacementTransform);

        displacement = -m_axis;
        displacementTranslate = localRotation.TransformVector(displacement);
        displacementTransform.SetTranslation(localTransform.GetTranslation() + displacementTranslate);
        m_secondManipulator = AzToolsFramework::AngularManipulator::MakeShared(worldTransform);
        m_secondManipulator->AddEntityComponentIdPair(m_entityComponentId);
        m_secondManipulator->SetAxis(m_axis);
        m_secondManipulator->SetLocalTransform(displacementTransform);

        const float manipulatorRadius = 2.0f;
        const float manipulatorWidth = 0.05f;
        m_firstManipulator->SetView(AzToolsFramework::CreateManipulatorViewCircle(
            *m_firstManipulator, ColorFirst,
            manipulatorRadius, manipulatorWidth, AzToolsFramework::DrawHalfDottedCircle));

        m_secondManipulator->SetView(AzToolsFramework::CreateManipulatorViewCircle(
            *m_secondManipulator, ColorSecond,
            manipulatorRadius, manipulatorWidth, AzToolsFramework::DrawHalfDottedCircle));

        Refresh();

        AZStd::shared_ptr<SharedRotationState> sharedRotationState =
            AZStd::make_shared<SharedRotationState>();

        auto mouseDownCallback = [this, sharedRotationState](const AzToolsFramework::AngularManipulator::Action& action) mutable -> void
        {
            const AZ::Quaternion normalizedStart = action.m_start.m_rotation.GetNormalized();
            sharedRotationState->m_axis = AZ::Vector3(normalizedStart.GetX(), normalizedStart.GetY(), normalizedStart.GetZ());
            sharedRotationState->m_savedOrientation = AZ::Quaternion::CreateIdentity();

            AngleLimitsFloatPair currentValue;
            EditorJointRequestBus::EventResult(
                currentValue, m_entityComponentId
                , &EditorJointRequests::GetLinearValuePair
                , m_name);

            sharedRotationState->m_valuePair = currentValue;
        };

        m_firstManipulator->InstallLeftMouseDownCallback(mouseDownCallback);

        m_secondManipulator->InstallLeftMouseDownCallback(mouseDownCallback);

        m_firstManipulator->InstallMouseMoveCallback(
            [this, sharedRotationState]
        (const AzToolsFramework::AngularManipulator::Action& action) mutable -> void
        {
            float angleDelta;
            AZ::Quaternion manipulatorOrientation;
            const float newValue = MouseMove(sharedRotationState, action, true, angleDelta, manipulatorOrientation);
            if (newValue > m_firstMax || newValue < m_firstMin)
            {
                return;
            }
            m_firstManipulator->SetLocalOrientation(manipulatorOrientation);
            const float newFirstValue = AZ::GetClamp(sharedRotationState->m_valuePair.first + angleDelta, m_firstMin, m_firstMax);

            EditorJointRequestBus::Event(
                m_entityComponentId
                , &EditorJointRequests::SetLinearValuePair
                , m_name
                , AngleLimitsFloatPair(newFirstValue, sharedRotationState->m_valuePair.second));

            m_firstManipulator->SetBoundsDirty();
        });

        m_secondManipulator->InstallMouseMoveCallback(
            [this, sharedRotationState]
        (const AzToolsFramework::AngularManipulator::Action& action) mutable -> void
        {
            float angleDelta;
            AZ::Quaternion manipulatorOrientation;
            const float newValue = MouseMove(sharedRotationState, action, false, angleDelta, manipulatorOrientation);
            if (newValue > m_secondMax || newValue < m_secondMin)
            {
                return; //Not handling values exceeding limits
            }

            m_secondManipulator->SetLocalOrientation(manipulatorOrientation);
            float newSecondValue = AZ::GetClamp(sharedRotationState->m_valuePair.second + angleDelta, m_secondMin, m_secondMax);

            EditorJointRequestBus::Event(
                m_entityComponentId
                , &EditorJointRequests::SetLinearValuePair
                , m_name
                , AngleLimitsFloatPair(sharedRotationState->m_valuePair.first, newSecondValue));

            m_secondManipulator->SetBoundsDirty();
        });


        m_firstManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
        m_secondManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);

        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(m_entityComponentId.GetEntityId());
    }

    EditorSubComponentModeAnglePair::~EditorSubComponentModeAnglePair()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();

        m_firstManipulator->Unregister();
        m_secondManipulator->Unregister();
    }

    void EditorSubComponentModeAnglePair::Refresh()
    {
        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EditorJointRequestBus::EventResult(
            localTransform, m_entityComponentId
            , &EditorJointRequests::GetTransformValue
            , PhysX::EditorJointComponentMode::s_parameterTransform);
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

        m_firstManipulator->SetBoundsDirty();
        m_secondManipulator->SetBoundsDirty();
    }

    void EditorSubComponentModeAnglePair::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AngleLimitsFloatPair currentValue;
        EditorJointRequestBus::EventResult(
            currentValue, m_entityComponentId
            , &EditorJointRequests::GetLinearValuePair
            , m_name);

        const float size = 2.0f;
        AZ::Vector3 axisPoint = m_axis * size * 0.5f;

        AZStd::array<AZ::Vector3, 4> points = {
            -axisPoint
            , axisPoint
            , axisPoint
            , -axisPoint
        };

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
        debugDisplay.SetAlpha(Alpha);

        AZ::Transform worldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(m_entityComponentId.GetEntityId());

        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EditorJointRequestBus::EventResult(
            localTransform, m_entityComponentId
            , &EditorJointRequests::GetTransformValue
            , PhysX::EditorJointComponentMode::s_parameterTransform);
        
        debugDisplay.PushMatrix(worldTransform);
        debugDisplay.PushMatrix(localTransform);
        
        debugDisplay.SetColor(ColorSweepArc);
        
        const AZ::Vector3 zeroVector = AZ::Vector3::CreateZero();
        const AZ::Vector3 posPosition = m_axis * SweepLineDisplaceFactor;
        const AZ::Vector3 negPosition = -posPosition;
        debugDisplay.DrawArc(posPosition, SweepLineThickness, -currentValue.first, currentValue.first, SweepLineGranularity, -m_axis);
        debugDisplay.DrawArc(zeroVector, SweepLineThickness, -currentValue.first, currentValue.first, SweepLineGranularity, -m_axis);
        debugDisplay.DrawArc(negPosition, SweepLineThickness, -currentValue.first, currentValue.first, SweepLineGranularity, -m_axis);
        debugDisplay.DrawArc(posPosition, SweepLineThickness, 0.0f, abs(currentValue.second), SweepLineGranularity, -m_axis);
        debugDisplay.DrawArc(zeroVector, SweepLineThickness, 0.0f, abs(currentValue.second), SweepLineGranularity, -m_axis);
        debugDisplay.DrawArc(negPosition, SweepLineThickness, 0.0f, abs(currentValue.second), SweepLineGranularity, -m_axis);

        AZ::Quaternion firstRotate = AZ::Quaternion::CreateFromAxisAngle(m_axis, AZ::DegToRad(currentValue.first));
        AZ::Transform firstTM = AZ::Transform::CreateFromQuaternion(firstRotate);
        debugDisplay.PushMatrix(firstTM);
        debugDisplay.SetColor(ColorFirst);
        debugDisplay.DrawQuad(points[0], points[1], points[2], points[3]);
        debugDisplay.PopMatrix();

        AZ::Quaternion secondRotate = AZ::Quaternion::CreateFromAxisAngle(m_axis, AZ::DegToRad(currentValue.second));
        AZ::Transform secondTM = AZ::Transform::CreateFromQuaternion(secondRotate);
        debugDisplay.PushMatrix(secondTM);
        debugDisplay.SetColor(ColorSecond);
        debugDisplay.DrawQuad(points[0], points[1], points[2], points[3]);
        debugDisplay.PopMatrix();

        debugDisplay.SetColor(ColorDefault);
        debugDisplay.DrawQuad(points[0], points[1], points[2], points[3]);

        debugDisplay.PopMatrix(); // pop local transform
        debugDisplay.PopMatrix(); // pop global transform
        debugDisplay.SetState(stateBefore);

        // reposition and reorientate manipulators
        Refresh();
    }

    float EditorSubComponentModeAnglePair::MouseMove(AZStd::shared_ptr<SharedRotationState>& sharedRotationState
        , const AzToolsFramework::AngularManipulator::Action& action
        , bool isFirstValue
        , float& angleDelta
        , AZ::Quaternion& manipulatorOrientation)
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
        else
        {
            return sharedRotationState->m_valuePair.second + angleDelta;
        }
    }
} // namespace PhysX
