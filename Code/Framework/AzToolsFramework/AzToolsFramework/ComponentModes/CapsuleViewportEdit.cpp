/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Viewport/ViewportColors.h>
#include <AzFramework/Viewport/ViewportConstants.h>
#include <AzToolsFramework/ComponentModes/BoxViewportEdit.h>
#include <AzToolsFramework/ComponentModes/CapsuleViewportEdit.h>

namespace AzToolsFramework
{
    namespace
    {
        const AZ::Vector3 RadiusManipulatorAxis = AZ::Vector3::CreateAxisX();
        const AZ::Vector3 HeightManipulatorAxis = AZ::Vector3::CreateAxisZ();
        const float MinCapsuleRadius = 0.001f;
        const float MinCapsuleHeight = 0.002f;
        const float HalfHeight = 0.5f;
        const float ResetCapsuleHeight = 1.0f;
        const float ResetCapsuleRadius = 0.25f;
    } // namespace

    AZ::Vector3 CapsuleViewportEdit::GetCapsuleNonUniformScale() const
    {
        return AZ::Vector3::CreateOne();
    }

    void CapsuleViewportEdit::OnCameraStateChanged(const AzFramework::CameraState& cameraState)
    {
        const float radius = GetCapsuleRadius();
        const AZ::Transform capsuleLocalTransform = GetCapsuleLocalTransform();
        m_radiusManipulator->SetAxis(cameraState.m_side);
        m_radiusManipulator->SetLocalTransform(capsuleLocalTransform * AZ::Transform::CreateTranslation(cameraState.m_side * radius));
    }

    void CapsuleViewportEdit::SetupCapsuleManipulators(const ManipulatorManagerId manipulatorManagerId)
    {
        const AZ::Transform& worldTransform = GetCapsuleWorldTransform();
        const AZ::Vector3& nonUniformScale = GetCapsuleNonUniformScale();
        const AZ::Transform& localTransform = GetCapsuleLocalTransform();
        m_manipulatorManagerId = manipulatorManagerId;

        SetupRadiusManipulator(worldTransform, localTransform, nonUniformScale);
        SetupHeightManipulator(worldTransform, localTransform, nonUniformScale);
    }

    void CapsuleViewportEdit::UpdateCapsuleManipulators()
    {
        AZ::Transform worldTransform = GetCapsuleWorldTransform();
        AZ::Vector3 nonUniformScale = GetCapsuleNonUniformScale();
        AZ::Transform localTransform = GetCapsuleLocalTransform();

        const float capsuleHeight = GetCapsuleHeight();
        const float capsuleRadius = GetCapsuleRadius();

        m_radiusManipulator->SetSpace(worldTransform);
        m_radiusManipulator->SetLocalTransform(
            localTransform * AZ::Transform::CreateTranslation(m_radiusManipulator->GetAxis() * capsuleRadius));
        m_radiusManipulator->SetNonUniformScale(nonUniformScale);
        m_heightManipulator->SetSpace(worldTransform);
        m_heightManipulator->SetLocalTransform(
            localTransform * AZ::Transform::CreateTranslation(m_heightManipulator->GetAxis() * capsuleHeight * HalfHeight));
        m_heightManipulator->SetNonUniformScale(nonUniformScale);
    }

    void CapsuleViewportEdit::ResetCapsuleManipulators()
    {
        SetCapsuleHeight(ResetCapsuleHeight);
        SetCapsuleRadius(ResetCapsuleRadius);
    }

    void CapsuleViewportEdit::TeardownCapsuleManipulators()
    {
        m_radiusManipulator->Unregister();
        m_heightManipulator->Unregister();
    }

    void CapsuleViewportEdit::SetupRadiusManipulator(
        const AZ::Transform& worldTransform, const AZ::Transform& localTransform, const AZ::Vector3& nonUniformScale)
    {
        float capsuleRadius = GetCapsuleRadius();
        m_radiusManipulator = AzToolsFramework::LinearManipulator::MakeShared(worldTransform);
        m_radiusManipulator->SetAxis(RadiusManipulatorAxis);
        m_radiusManipulator->Register(m_manipulatorManagerId);
        m_radiusManipulator->SetLocalTransform(localTransform * AZ::Transform::CreateTranslation(RadiusManipulatorAxis * capsuleRadius));
        m_radiusManipulator->SetNonUniformScale(nonUniformScale);
        {
            AzToolsFramework::ManipulatorViews views;
            views.emplace_back(AzToolsFramework::CreateManipulatorViewQuadBillboard(
                AzFramework::ViewportColors::DefaultManipulatorHandleColor, AzFramework::ViewportConstants::DefaultManipulatorHandleSize));
            m_radiusManipulator->SetViews(AZStd::move(views));
        }
        m_radiusManipulator->InstallLeftMouseDownCallback(
            [this]([[maybe_unused]] const AzToolsFramework::LinearManipulator::Action& action)
            {
                BeginEditing();
            });
        m_radiusManipulator->InstallMouseMoveCallback(
            [this](const AzToolsFramework::LinearManipulator::Action& action)
            {
                OnRadiusManipulatorMoved(action);
            });
        m_radiusManipulator->InstallLeftMouseUpCallback(
            [this]([[maybe_unused]] const AzToolsFramework::LinearManipulator::Action& action)
            {
                FinishEditing();
            });
    }

    void CapsuleViewportEdit::SetupHeightManipulator(
        const AZ::Transform& worldTransform, const AZ::Transform& localTransform, const AZ::Vector3& nonUniformScale)
    {
        float capsuleHeight = GetCapsuleHeight();
        m_heightManipulator = AzToolsFramework::LinearManipulator::MakeShared(worldTransform);
        m_heightManipulator->SetAxis(HeightManipulatorAxis);
        m_heightManipulator->Register(m_manipulatorManagerId);
        m_heightManipulator->SetLocalTransform(
            localTransform * AZ::Transform::CreateTranslation(HeightManipulatorAxis * capsuleHeight * HalfHeight));
        m_heightManipulator->SetNonUniformScale(nonUniformScale);
        {
            AzToolsFramework::ManipulatorViews views;
            views.emplace_back(AzToolsFramework::CreateManipulatorViewQuadBillboard(
                AzFramework::ViewportColors::DefaultManipulatorHandleColor, AzFramework::ViewportConstants::DefaultManipulatorHandleSize));
            m_heightManipulator->SetViews(AZStd::move(views));
        }
        m_heightManipulator->InstallLeftMouseDownCallback(
            [this]([[maybe_unused]] const AzToolsFramework::LinearManipulator::Action& action)
            {
                BeginEditing();
            });
        m_heightManipulator->InstallMouseMoveCallback(
            [this](const AzToolsFramework::LinearManipulator::Action& action)
            {
                OnHeightManipulatorMoved(action);
            });
        m_heightManipulator->InstallLeftMouseUpCallback(
            [this]([[maybe_unused]] const AzToolsFramework::LinearManipulator::Action& action)
            {
                FinishEditing();
            });
    }

    void CapsuleViewportEdit::OnRadiusManipulatorMoved(const AzToolsFramework::LinearManipulator::Action& action)
    {
        // manipulator action offsets do not take entity transform scale into account, so need to apply it here
        const AZ::Transform capsuleLocalTransform = GetCapsuleLocalTransform();
        const AZ::Vector3 manipulatorPosition = AzToolsFramework::GetPositionInManipulatorFrame(
            m_radiusManipulator->GetSpace().GetUniformScale(), capsuleLocalTransform, action);

        // Get the distance the manipulator has moved along the axis.
        float extent = manipulatorPosition.Dot(action.m_fixed.m_axis);

        // Clamp radius to a small value.
        extent = AZ::GetMax(extent, MinCapsuleRadius);

        // Update the manipulator and capsule radius.
        m_radiusManipulator->SetLocalTransform(capsuleLocalTransform * AZ::Transform::CreateTranslation(extent * action.m_fixed.m_axis));

        // Adjust the height manipulator so it is always clamped to twice the radius.
        AdjustHeightManipulator(extent);

        // The final radius of the capsule is the manipulator's extent.
        SetCapsuleRadius(extent);
    }

    void CapsuleViewportEdit::OnHeightManipulatorMoved(const AzToolsFramework::LinearManipulator::Action& action)
    {
        // manipulator action offsets do not take entity transform scale into account, so need to apply it here
        const AZ::Transform localTransform = GetCapsuleLocalTransform();
        const AZ::Vector3 manipulatorPosition =
            AzToolsFramework::GetPositionInManipulatorFrame(m_heightManipulator->GetSpace().GetUniformScale(), localTransform, action);

        // Get the distance the manipulator has moved along the axis.
        float extent = manipulatorPosition.Dot(action.m_fixed.m_axis);

        // Ensure capsule's half height is always greater than the radius.
        extent = AZ::GetMax(extent, MinCapsuleHeight);

        // Update the manipulator and capsule height.
        m_heightManipulator->SetLocalTransform(localTransform * AZ::Transform::CreateTranslation(extent * action.m_fixed.m_axis));

        // The final height of the capsule is twice the manipulator's extent.
        float capsuleHeight = extent / HalfHeight;

        // Adjust the radius manipulator so it is always clamped to half the capsule height.
        AdjustRadiusManipulator(capsuleHeight);

        // Finally adjust the capsule height
        SetCapsuleHeight(capsuleHeight);
    }

    void CapsuleViewportEdit::AdjustRadiusManipulator(float capsuleHeight)
    {
        float capsuleRadius = GetCapsuleRadius();

        // Clamp the radius to half height.
        capsuleRadius = AZ::GetMin(capsuleRadius, capsuleHeight * HalfHeight);

        // Update manipulator and the capsule radius.
        const AZ::Transform localTransform = GetCapsuleLocalTransform();
        m_radiusManipulator->SetLocalTransform(
            localTransform * AZ::Transform::CreateTranslation(capsuleRadius * m_radiusManipulator->GetAxis()));
        SetCapsuleRadius(capsuleRadius);
    }

    void CapsuleViewportEdit::AdjustHeightManipulator(float capsuleRadius)
    {
        float capsuleHeight = GetCapsuleHeight();

        // Clamp the height to twice the radius.
        capsuleHeight = AZ::GetMax(capsuleHeight, capsuleRadius / HalfHeight);

        // Update the manipulator and capsule height.
        const AZ::Transform localTransform = GetCapsuleLocalTransform();
        m_heightManipulator->SetLocalTransform(
            localTransform * AZ::Transform::CreateTranslation(capsuleHeight * HalfHeight * m_heightManipulator->GetAxis()));
        SetCapsuleHeight(capsuleHeight);
    }

    void CapsuleViewportEdit::BeginEditing()
    {
    }

    void CapsuleViewportEdit::FinishEditing()
    {
    }
} // namespace AzToolsFramework
