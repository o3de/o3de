/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Viewport/ViewportColors.h>
#include <AzFramework/Viewport/ViewportConstants.h>
#include <AzToolsFramework/ComponentModes/CapsuleViewportEdit.h>
#include <AzToolsFramework/ComponentModes/ViewportEditUtilities.h>

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

    void CapsuleViewportEdit::InstallGetRotationOffset(AZStd::function<AZ::Quaternion()> getRotationOffset)
    {
        m_getRotationOffset = AZStd::move(getRotationOffset);
    }

    void CapsuleViewportEdit::InstallGetCapsuleRadius(AZStd::function<float()> getCapsuleRadius)
    {
        m_getCapsuleRadius = AZStd::move(getCapsuleRadius);
    }

    void CapsuleViewportEdit::InstallGetCapsuleHeight(AZStd::function<float()> getCapsuleHeight)
    {
        m_getCapsuleHeight = AZStd::move(getCapsuleHeight);
    }

    void CapsuleViewportEdit::InstallSetCapsuleRadius(AZStd::function<void(float)> setCapsuleRadius)
    {
        m_setCapsuleRadius = AZStd::move(setCapsuleRadius);
    }

    void CapsuleViewportEdit::InstallSetCapsuleHeight(AZStd::function<void(float)> setCapsuleHeight)
    {
        m_setCapsuleHeight = AZStd::move(setCapsuleHeight);
    }

    AZ::Quaternion CapsuleViewportEdit::GetRotationOffset() const
    {
        if (m_getRotationOffset)
        {
            return m_getRotationOffset();
        }
        AZ_ErrorOnce("CapsuleViewportEdit", false, "No implementation provided for GetRotationOffset");
        return AZ::Quaternion::CreateIdentity();
    }

    float CapsuleViewportEdit::GetCapsuleRadius() const
    {
        if (m_getCapsuleRadius)
        {
            return m_getCapsuleRadius();
        }
        AZ_ErrorOnce("CapsuleViewportEdit", false, "No implementation provided for GetCapsuleRadius");
        return ResetCapsuleRadius;
    }

    float CapsuleViewportEdit::GetCapsuleHeight() const
    {
        if (m_getCapsuleHeight)
        {
            return m_getCapsuleHeight();
        }
        AZ_ErrorOnce("CapsuleViewportEdit", false, "No implementation provided for GetCapsuleHeight");
        return ResetCapsuleHeight;
    }

    void CapsuleViewportEdit::SetCapsuleRadius(float radius)
    {
        if (m_setCapsuleRadius)
        {
            m_setCapsuleRadius(radius);
        }
        else
        {
            AZ_ErrorOnce("CapsuleViewportEdit", false, "No implementation provided for SetCapsuleRadius");
        }
    }

    void CapsuleViewportEdit::SetCapsuleHeight(float height)
    {
        if (m_setCapsuleHeight)
        {
            m_setCapsuleHeight(height);
        }
        else
        {
            AZ_ErrorOnce("CapsuleViewportEdit", false, "No implementation provided for SetCapsuleHeight");
        }
    }

    AZ::Transform CapsuleViewportEdit::GetLocalTransform() const
    {
        return AZ::Transform::CreateFromQuaternionAndTranslation(GetRotationOffset(), GetTranslationOffset());
    }

    void CapsuleViewportEdit::OnCameraStateChanged(const AzFramework::CameraState& cameraState)
    {
        if (m_radiusManipulator)
        {
            const float radius = GetCapsuleRadius();
            const AZ::Transform localTransform = GetLocalTransform();
            const AZ::Transform manipulatorSpace = GetManipulatorSpace();
            const AZ::Vector3 inverseTransformedForward =
                (manipulatorSpace * localTransform).GetInverse().TransformVector(cameraState.m_forward);
            AZ::Vector3 axis = inverseTransformedForward.Cross(HeightManipulatorAxis);
            if (axis.GetLengthSq() < AZ::Constants::Tolerance * AZ::Constants::Tolerance)
            {
                axis = AZ::Vector3::CreateAxisX();
            }
            else
            {
                axis.Normalize();
            }
            m_radiusManipulator->SetAxis(axis);
            m_radiusManipulator->SetLocalTransform(localTransform * AZ::Transform::CreateTranslation(radius * axis));
        }
    }

    void CapsuleViewportEdit::Setup(const ManipulatorManagerId manipulatorManagerId)
    {
        const AZ::Transform worldTransform = GetManipulatorSpace();
        const AZ::Vector3 nonUniformScale = GetNonUniformScale();
        const AZ::Transform localTransform = GetLocalTransform();

        SetupRadiusManipulator(manipulatorManagerId, worldTransform, localTransform, nonUniformScale);
        SetupHeightManipulator(manipulatorManagerId, worldTransform, localTransform, nonUniformScale);
    }

    void CapsuleViewportEdit::AddEntityComponentIdPair(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        if (m_heightManipulator)
        {
            m_heightManipulator->AddEntityComponentIdPair(entityComponentIdPair);
        }
        if (m_radiusManipulator)
        {
            m_radiusManipulator->AddEntityComponentIdPair(entityComponentIdPair);
        }
        AZ_WarningOnce(
            "CapsuleViewportEdit",
            m_heightManipulator && m_radiusManipulator,
            "Attempting to AddEntityComponentIdPair before manipulators have been created");
    }

    void CapsuleViewportEdit::UpdateManipulators()
    {
        const AZ::Transform worldTransform = GetManipulatorSpace();
        const AZ::Vector3 nonUniformScale = GetNonUniformScale();
        const AZ::Transform localTransform = GetLocalTransform();

        const float capsuleHeight = GetCapsuleHeight();
        const float capsuleRadius = GetCapsuleRadius();

        if (m_radiusManipulator)
        {
            m_radiusManipulator->SetSpace(worldTransform);
            m_radiusManipulator->SetLocalTransform(
                localTransform * AZ::Transform::CreateTranslation(m_radiusManipulator->GetAxis() * capsuleRadius));
            m_radiusManipulator->SetNonUniformScale(nonUniformScale);
        }
        if (m_heightManipulator)
        {
            m_heightManipulator->SetSpace(worldTransform);
            m_heightManipulator->SetLocalTransform(
                localTransform * AZ::Transform::CreateTranslation(m_heightManipulator->GetAxis() * capsuleHeight * HalfHeight));
            m_heightManipulator->SetNonUniformScale(nonUniformScale);
        }
    }

    void CapsuleViewportEdit::ResetValues()
    {
        BeginEditing();
        SetCapsuleHeight(ResetCapsuleHeight);
        SetCapsuleRadius(ResetCapsuleRadius);
        FinishEditing();
    }

    void CapsuleViewportEdit::Teardown()
    {
        if (m_radiusManipulator)
        {
            m_radiusManipulator->Unregister();
        }
        if (m_heightManipulator)
        {
            m_heightManipulator->Unregister();
        }
    }

    void CapsuleViewportEdit::SetupRadiusManipulator(
        const ManipulatorManagerId manipulatorManagerId,
        const AZ::Transform& worldTransform,
        const AZ::Transform& localTransform,
        const AZ::Vector3& nonUniformScale)
    {
        float capsuleRadius = GetCapsuleRadius();
        m_radiusManipulator = AzToolsFramework::LinearManipulator::MakeShared(worldTransform);
        m_radiusManipulator->SetAxis(RadiusManipulatorAxis);
        m_radiusManipulator->Register(manipulatorManagerId);
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
        const ManipulatorManagerId manipulatorManagerId,
        const AZ::Transform& worldTransform,
        const AZ::Transform& localTransform,
        const AZ::Vector3& nonUniformScale)
    {
        float capsuleHeight = GetCapsuleHeight();
        m_heightManipulator = AzToolsFramework::LinearManipulator::MakeShared(worldTransform);
        m_heightManipulator->SetAxis(HeightManipulatorAxis);
        m_heightManipulator->Register(manipulatorManagerId);
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
        const AZ::Transform localTransform = GetLocalTransform();
        const AZ::Vector3 manipulatorPosition = AzToolsFramework::GetPositionInManipulatorFrame(
            m_radiusManipulator->GetSpace().GetUniformScale(), localTransform, action);

        // Get the distance the manipulator has moved along the axis.
        float extent = manipulatorPosition.Dot(action.m_fixed.m_axis);

        // Clamp radius to a small value.
        extent = AZ::GetMax(extent, MinCapsuleRadius);

        // Update the manipulator and capsule radius.
        m_radiusManipulator->SetLocalTransform(localTransform * AZ::Transform::CreateTranslation(extent * action.m_fixed.m_axis));

        // Adjust the height manipulator so it is always clamped to twice the radius.
        AdjustHeightManipulator(extent);

        // The final radius of the capsule is the manipulator's extent.
        SetCapsuleRadius(extent);
    }

    void CapsuleViewportEdit::OnHeightManipulatorMoved(const AzToolsFramework::LinearManipulator::Action& action)
    {
        // manipulator action offsets do not take entity transform scale into account, so need to apply it here
        const AZ::Transform localTransform = GetLocalTransform();
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
        const AZ::Transform localTransform = GetLocalTransform();
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
        const AZ::Transform localTransform = GetLocalTransform();
        m_heightManipulator->SetLocalTransform(
            localTransform * AZ::Transform::CreateTranslation(capsuleHeight * HalfHeight * m_heightManipulator->GetAxis()));
        SetCapsuleHeight(capsuleHeight);
    }
} // namespace AzToolsFramework
