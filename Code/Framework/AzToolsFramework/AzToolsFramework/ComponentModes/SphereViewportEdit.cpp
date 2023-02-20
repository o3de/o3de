/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Viewport/ViewportColors.h>
#include <AzFramework/Viewport/ViewportConstants.h>
#include <AzToolsFramework/ComponentModes/SphereViewportEdit.h>
#include <AzToolsFramework/ComponentModes/ViewportEditUtilities.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>

namespace AzToolsFramework
{
    namespace SphereViewportEditConstants
    {
        const AZ::Vector3 RadiusManipulatorAxis = AZ::Vector3::CreateAxisX();
        const AZ::Vector3 UpAxis = AZ::Vector3::CreateAxisZ();
        const float MinSphereRadius = 0.001f;
        const float ResetSphereRadius = 0.5f;
    } // namespace SphereViewportEditConstants

    void SphereViewportEdit::InstallGetSphereRadius(AZStd::function<float()> getSphereRadius)
    {
        m_getSphereRadius = AZStd::move(getSphereRadius);
    }

    void SphereViewportEdit::InstallSetSphereRadius(AZStd::function<void(float)> setSphereRadius)
    {
        m_setSphereRadius = AZStd::move(setSphereRadius);
    }

    float SphereViewportEdit::GetSphereRadius() const
    {
        if (m_getSphereRadius)
        {
            return m_getSphereRadius();
        }
        AZ_ErrorOnce("SphereViewportEdit", false, "No implementation provided for GetSphereRadius");
        return SphereViewportEditConstants::ResetSphereRadius;
    }

    void SphereViewportEdit::SetSphereRadius(float radius)
    {
        if (m_setSphereRadius)
        {
            m_setSphereRadius(radius);
        }
        else
        {
            AZ_ErrorOnce("SphereViewportEdit", false, "No implementation provided for SetSphereRadius");
        }
    }

    void SphereViewportEdit::OnCameraStateChanged(const AzFramework::CameraState& cameraState)
    {
        if (m_radiusManipulator)
        {
            const float radius = GetSphereRadius();
            const AZ::Transform localTransform = GetLocalTransform();
            const AZ::Transform localInverse = localTransform.GetInverse();
            const AZ::Transform manipulatorSpaceInverse = GetManipulatorSpace().GetInverse();

            const AZ::Vector3 inverseTransformedForward =
                localInverse.TransformVector(manipulatorSpaceInverse.TransformVector(cameraState.m_forward) / GetNonUniformScale());
            AZ::Vector3 axis = inverseTransformedForward.Cross(SphereViewportEditConstants::UpAxis);
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

    void SphereViewportEdit::Setup(const ManipulatorManagerId manipulatorManagerId)
    {
        const AZ::Transform worldTransform = GetManipulatorSpace();
        const AZ::Vector3 nonUniformScale = GetNonUniformScale();
        const AZ::Transform localTransform = GetLocalTransform();

        SetupRadiusManipulator(manipulatorManagerId, worldTransform, localTransform, nonUniformScale);
    }

    void SphereViewportEdit::AddEntityComponentIdPairImpl(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        if (m_radiusManipulator)
        {
            m_radiusManipulator->AddEntityComponentIdPair(entityComponentIdPair);
        }
        AZ_WarningOnce(
            "SphereViewportEdit", m_radiusManipulator, "Attempting to AddEntityComponentIdPair before manipulators have been created");
    }

    void SphereViewportEdit::UpdateManipulators()
    {
        if (m_radiusManipulator)
        {
            const AZ::Transform worldTransform = GetManipulatorSpace();
            const AZ::Vector3 nonUniformScale = GetNonUniformScale();
            const AZ::Transform localTransform = GetLocalTransform();
            const float sphereRadius = GetSphereRadius();

            m_radiusManipulator->SetSpace(worldTransform);
            m_radiusManipulator->SetLocalTransform(
                localTransform * AZ::Transform::CreateTranslation(m_radiusManipulator->GetAxis() * sphereRadius));
            m_radiusManipulator->SetNonUniformScale(nonUniformScale);
            m_radiusManipulator->SetBoundsDirty();
        }
    }

    void SphereViewportEdit::ResetValuesImpl()
    {
        SetSphereRadius(SphereViewportEditConstants::ResetSphereRadius);
    }

    void SphereViewportEdit::Teardown()
    {
        if (m_radiusManipulator)
        {
            m_radiusManipulator->Unregister();
        }
    }

    void SphereViewportEdit::SetupRadiusManipulator(
        const ManipulatorManagerId manipulatorManagerId,
        const AZ::Transform& worldTransform,
        const AZ::Transform& localTransform,
        const AZ::Vector3& nonUniformScale)
    {
        float sphereRadius = GetSphereRadius();
        m_radiusManipulator = LinearManipulator::MakeShared(worldTransform);
        m_radiusManipulator->SetAxis(SphereViewportEditConstants::RadiusManipulatorAxis);
        m_radiusManipulator->Register(manipulatorManagerId);
        m_radiusManipulator->SetLocalTransform(
            localTransform * AZ::Transform::CreateTranslation(SphereViewportEditConstants::RadiusManipulatorAxis * sphereRadius));
        m_radiusManipulator->SetNonUniformScale(nonUniformScale);
        {
            ManipulatorViews views;
            views.emplace_back(CreateManipulatorViewQuadBillboard(
                AzFramework::ViewportColors::DefaultManipulatorHandleColor, AzFramework::ViewportConstants::DefaultManipulatorHandleSize));
            m_radiusManipulator->SetViews(AZStd::move(views));
        }
        m_radiusManipulator->InstallLeftMouseDownCallback(
            [this]([[maybe_unused]] const LinearManipulator::Action& action)
            {
                BeginEditing();
            });
        m_radiusManipulator->InstallMouseMoveCallback(
            [this](const LinearManipulator::Action& action)
            {
                OnRadiusManipulatorMoved(action);
            });
        m_radiusManipulator->InstallLeftMouseUpCallback(
            [this]([[maybe_unused]] const LinearManipulator::Action& action)
            {
                EndEditing();
            });
    }

    void SphereViewportEdit::OnRadiusManipulatorMoved(const LinearManipulator::Action& action)
    {
        // manipulator action offsets do not take entity transform scale into account, so need to apply it here
        const AZ::Transform localTransform = GetLocalTransform();
        const AZ::Vector3 manipulatorPosition = GetPositionInManipulatorFrame(
            m_radiusManipulator->GetSpace().GetUniformScale(), localTransform, action);

        float extent = manipulatorPosition.Dot(action.m_fixed.m_axis);
        extent = AZ::GetMax(extent, SphereViewportEditConstants::MinSphereRadius);
        m_radiusManipulator->SetLocalTransform(localTransform * AZ::Transform::CreateTranslation(extent * action.m_fixed.m_axis));

        SetSphereRadius(extent);
    }
} // namespace AzToolsFramework
