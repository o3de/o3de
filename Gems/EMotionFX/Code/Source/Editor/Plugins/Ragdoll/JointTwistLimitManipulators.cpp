/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Configuration/JointConfiguration.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <EMotionFX/CommandSystem/Source/JointLimitCommands.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Editor/Plugins/Ragdoll/JointLimitRotationManipulators.h>
#include <Editor/Plugins/Ragdoll/JointTwistLimitManipulators.h>
#include <Editor/Plugins/Ragdoll/RagdollJointLimitWidget.h>

namespace EMotionFX
{
    static const float ManipulatorAxisLength = 0.2f;
    static const float ManipulatorRadius = 0.5f;
    static const float ManipulatorWidth = 0.05f;
    static const float ManipulatorQuadWidth = 0.1f;

    float GetAngleDeltaDegrees(const AzToolsFramework::AngularManipulator::Action& action)
    {
        float angleDeltaRadians = 0.0f;
        AZ::Vector3 axis = AZ::Vector3::CreateZero();
        action.m_current.m_delta.ConvertToAxisAngle(axis, angleDeltaRadians);

        // reverse the direction of the angle if the axis has been flipped by ConvertToAxisAngle
        if (axis.GetX() < 0.0f)
        {
            angleDeltaRadians = -angleDeltaRadians;
        }
        return AZ::RadToDeg(angleDeltaRadians);
    }

    void JointTwistLimitManipulators::Setup(const PhysicsSetupManipulatorData& physicsSetupManipulatorData)
    {
        m_physicsSetupManipulatorData = physicsSetupManipulatorData;

        if (!m_physicsSetupManipulatorData.HasJointLimit())
        {
            return;
        }

        AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
        AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, m_viewportId);
        m_debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);

        const AZ::Transform parentWorldTransform = m_physicsSetupManipulatorData.GetJointParentFrameWorld();

        // lower limit manipulator
        m_twistLimitLowerManipulator = AzToolsFramework::AngularManipulator::MakeShared(parentWorldTransform);
        m_twistLimitLowerManipulator->SetAxis(AZ::Vector3::CreateAxisX());
        m_twistLimitLowerManipulator->SetLocalPosition(AZ::Vector3::CreateAxisX(0.5f * ManipulatorAxisLength));
        m_twistLimitLowerManipulator->SetView(AzToolsFramework::CreateManipulatorViewCircle(
            *m_twistLimitLowerManipulator, AzPhysics::JointVisualizationDefaults::ColorFirst, ManipulatorRadius, ManipulatorWidth,
            AzToolsFramework::DrawHalfDottedCircle));

        m_twistLimitLowerManipulator->InstallLeftMouseDownCallback(
            [this]([[maybe_unused]] const AzToolsFramework::AngularManipulator::Action& action)
            {
                BeginEditing();
            });

        m_twistLimitLowerManipulator->InstallMouseMoveCallback(
            [this](const AzToolsFramework::AngularManipulator::Action& action)
            {
                if (!m_jointTwistLimitState.m_twistLimitLower.has_value())
                {
                    return;
                }

                float angleDeltaDegrees = GetAngleDeltaDegrees(action);
                float newTwistLimitLower = m_jointTwistLimitState.m_twistLimitLower.value() + angleDeltaDegrees;
                m_physicsSetupManipulatorData.m_jointConfiguration->SetPropertyValue(AZ::Name("TwistLimitLower"), newTwistLimitLower);

                InvalidateEditorValues();
            });

        m_twistLimitLowerManipulator->InstallLeftMouseUpCallback(
            [this]([[maybe_unused]] const AzToolsFramework::AngularManipulator::Action& action)
            {
                EndEditing();
            });

        m_twistLimitLowerManipulator->Register(EMStudio::g_animManipulatorManagerId);

        // upper limit manipulator
        m_twistLimitUpperManipulator = AzToolsFramework::AngularManipulator::MakeShared(parentWorldTransform);
        m_twistLimitUpperManipulator->SetAxis(AZ::Vector3::CreateAxisX());
        m_twistLimitUpperManipulator->SetLocalPosition(AZ::Vector3::CreateAxisX(-0.5f * ManipulatorAxisLength));
        m_twistLimitUpperManipulator->SetView(AzToolsFramework::CreateManipulatorViewCircle(
            *m_twistLimitUpperManipulator, AzPhysics::JointVisualizationDefaults::ColorSecond, ManipulatorRadius, ManipulatorWidth,
            AzToolsFramework::DrawHalfDottedCircle));

        m_twistLimitUpperManipulator->InstallLeftMouseDownCallback(
            [this]([[maybe_unused]] const AzToolsFramework::AngularManipulator::Action& action)
            {
                BeginEditing();
            });

        m_twistLimitUpperManipulator->InstallMouseMoveCallback(
            [this](const AzToolsFramework::AngularManipulator::Action& action)
            {
                if (!m_jointTwistLimitState.m_twistLimitUpper.has_value())
                {
                    return;
                }

                float angleDeltaDegrees = GetAngleDeltaDegrees(action);
                float newTwistLimitUpper = m_jointTwistLimitState.m_twistLimitUpper.value() + angleDeltaDegrees;
                m_physicsSetupManipulatorData.m_jointConfiguration->SetPropertyValue(AZ::Name("TwistLimitUpper"), newTwistLimitUpper);

                InvalidateEditorValues();
            });

        m_twistLimitUpperManipulator->InstallLeftMouseUpCallback(
            [this]([[maybe_unused]] const AzToolsFramework::AngularManipulator::Action& action)
            {
                EndEditing();
            });

        m_twistLimitUpperManipulator->Register(EMStudio::g_animManipulatorManagerId);

        AZ::TickBus::Handler::BusConnect();
        PhysicsSetupManipulatorRequestBus::Handler::BusConnect();
        m_adjustJointLimitCallback = AZStd::make_unique<PhysicsSetupManipulatorCommandCallback>(this, false);
        EMStudio::GetCommandManager()->RegisterCommandCallback("AdjustJointLimit", m_adjustJointLimitCallback.get());
    }

    void JointTwistLimitManipulators::Refresh()
    {
        if (m_physicsSetupManipulatorData.HasJointLimit())
        {
            const AZ::Transform parentWorldTransform = m_physicsSetupManipulatorData.GetJointParentFrameWorld();
            m_twistLimitLowerManipulator->SetSpace(parentWorldTransform);
            m_twistLimitUpperManipulator->SetSpace(parentWorldTransform);
        }
    }

    void JointTwistLimitManipulators::Teardown()
    {
        EMStudio::GetCommandManager()->RemoveCommandCallback(m_adjustJointLimitCallback.get(), false);
        m_adjustJointLimitCallback.reset();
        PhysicsSetupManipulatorRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        if (m_twistLimitLowerManipulator)
        {
            m_twistLimitLowerManipulator->Unregister();
        }
        if (m_twistLimitUpperManipulator)
        {
            m_twistLimitUpperManipulator->Unregister();
        }
        m_twistLimitLowerManipulator.reset();
        m_twistLimitUpperManipulator.reset();
        m_debugDisplay = nullptr;
    }

    void JointTwistLimitManipulators::ResetValues()
    {
        if (m_physicsSetupManipulatorData.HasJointLimit())
        {
            BeginEditing();
            m_physicsSetupManipulatorData.m_jointConfiguration->SetPropertyValue(AZ::Name("TwistLimitLower"), -45.0f);
            m_physicsSetupManipulatorData.m_jointConfiguration->SetPropertyValue(AZ::Name("TwistLimitUpper"), 45.0f);
            EndEditing();
            Refresh();
        }
    }

    void JointTwistLimitManipulators::OnTick([[maybe_unused]] float delta, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        if (!m_debugDisplay || !m_physicsSetupManipulatorData.HasJointLimit())
        {
            return;
        }

        AZStd::optional<float> twistLimitLower =
            m_physicsSetupManipulatorData.m_jointConfiguration->GetPropertyValue(AZ::Name("TwistLimitLower"));
        AZStd::optional<float> twistLimitUpper =
            m_physicsSetupManipulatorData.m_jointConfiguration->GetPropertyValue(AZ::Name("TwistLimitUpper"));

        if (!twistLimitLower.has_value() && !twistLimitUpper.has_value())
        {
            return;
        }

        const AZStd::array<AZ::Vector3, 4> points = {
            AZ::Vector3(-0.5f * ManipulatorAxisLength, 0.0f, 0.0f),
            AZ::Vector3(0.5f * ManipulatorAxisLength, 0.0f, 0.0f),
            AZ::Vector3(0.5f * ManipulatorAxisLength, ManipulatorQuadWidth, 0.0f),
            AZ::Vector3(-0.5f * ManipulatorAxisLength, ManipulatorQuadWidth, 0.0f)
        };

        AZ::u32 previousState = m_debugDisplay->GetState();
        m_debugDisplay->CullOff();
        m_debugDisplay->SetAlpha(AzPhysics::JointVisualizationDefaults::Alpha);
        m_debugDisplay->PushMatrix(m_physicsSetupManipulatorData.GetJointParentFrameWorld());

        if (twistLimitLower.has_value())
        {
            AZ::Quaternion twistLimitLowerRotation =
                AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::DegToRad(twistLimitLower.value()));
            AZ::Transform twistLimitLowerTM = AZ::Transform::CreateFromQuaternion(twistLimitLowerRotation);
            m_debugDisplay->PushMatrix(twistLimitLowerTM);
            m_debugDisplay->SetColor(AzPhysics::JointVisualizationDefaults::ColorFirst);
            m_debugDisplay->DrawQuad(points[0], points[1], points[2], points[3]);
            m_debugDisplay->PopMatrix();
        }

        if (twistLimitUpper.has_value())
        {
            AZ::Quaternion twistLimitUpperRotation =
                AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::DegToRad(twistLimitUpper.value()));
            AZ::Transform twistLimitUpperTM = AZ::Transform::CreateFromQuaternion(twistLimitUpperRotation);
            m_debugDisplay->PushMatrix(twistLimitUpperTM);
            m_debugDisplay->SetColor(AzPhysics::JointVisualizationDefaults::ColorSecond);
            m_debugDisplay->DrawQuad(points[0], points[1], points[2], points[3]);
            m_debugDisplay->PopMatrix();
        }

        m_debugDisplay->PopMatrix();
        m_debugDisplay->SetState(previousState);
    }

    void JointTwistLimitManipulators::OnUnderlyingPropertiesChanged()
    {
        Refresh();
    }

    void JointTwistLimitManipulators::BeginEditing()
    {
        m_jointTwistLimitState.m_twistLimitLower =
            m_physicsSetupManipulatorData.m_jointConfiguration->GetPropertyValue(AZ::Name("TwistLimitLower"));
        m_jointTwistLimitState.m_twistLimitUpper =
            m_physicsSetupManipulatorData.m_jointConfiguration->GetPropertyValue(AZ::Name("TwistLimitUpper"));

        CreateCommandAdjustJointLimit(m_commandGroup, m_physicsSetupManipulatorData);
    }

    void JointTwistLimitManipulators::EndEditing()
    {
        ExecuteCommandAdjustJointLimit(m_commandGroup, m_physicsSetupManipulatorData);
    }

    void JointTwistLimitManipulators::InvalidateEditorValues()
    {
        if (m_physicsSetupManipulatorData.m_jointLimitWidget)
        {
            m_physicsSetupManipulatorData.m_jointLimitWidget->InvalidateValues();
        }
    }
} // namespace EMotionFX
