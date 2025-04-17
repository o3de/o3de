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
#include <AzToolsFramework/Viewport/ViewportSettings.h>

namespace AzToolsFramework
{
    namespace CapsuleViewportEditConstants
    {
        const AZ::Vector3 RadiusManipulatorAxis = AZ::Vector3::CreateAxisX();
        const AZ::Vector3 HeightManipulatorAxis = AZ::Vector3::CreateAxisZ();
        const float MinCapsuleRadius = 0.001f;
        const float MinCapsuleHeight = 0.002f;
        const float ResetCapsuleHeight = 1.0f;
        const float ResetCapsuleRadius = 0.25f;
    } // namespace CapsuleViewportEditConstants

    CapsuleViewportEdit::CapsuleViewportEdit(bool allowAsymmetricalEditing)
        : m_allowAsymmetricalEditing(allowAsymmetricalEditing)
    {
    }

    void CapsuleViewportEdit::SetEnsureHeightExceedsTwiceRadius(bool ensureHeightExceedsTwiceRadius)
    {
        m_ensureHeightExceedsTwiceRadius = ensureHeightExceedsTwiceRadius;
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

    float CapsuleViewportEdit::GetCapsuleRadius() const
    {
        if (m_getCapsuleRadius)
        {
            return m_getCapsuleRadius();
        }
        AZ_ErrorOnce("CapsuleViewportEdit", false, "No implementation provided for GetCapsuleRadius");
        return CapsuleViewportEditConstants::ResetCapsuleRadius;
    }

    float CapsuleViewportEdit::GetCapsuleHeight() const
    {
        if (m_getCapsuleHeight)
        {
            return m_getCapsuleHeight();
        }
        AZ_ErrorOnce("CapsuleViewportEdit", false, "No implementation provided for GetCapsuleHeight");
        return CapsuleViewportEditConstants::ResetCapsuleHeight;
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

    void CapsuleViewportEdit::OnCameraStateChanged(const AzFramework::CameraState& cameraState)
    {
        if (m_radiusManipulator)
        {
            const float radius = GetCapsuleRadius();
            const AZ::Transform localTransform = GetLocalTransform();
            const AZ::Transform localInverse = localTransform.GetInverse();
            const AZ::Transform manipulatorSpaceInverse = GetManipulatorSpace().GetInverse();

            const AZ::Vector3 inverseTransformedForward =
                localInverse.TransformVector(manipulatorSpaceInverse.TransformVector(cameraState.m_forward) / GetNonUniformScale());
            AZ::Vector3 axis = inverseTransformedForward.Cross(CapsuleViewportEditConstants::HeightManipulatorAxis);
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
        m_topManipulator = SetupHeightManipulator(manipulatorManagerId, worldTransform, localTransform, nonUniformScale, 1.0f);
        if (m_allowAsymmetricalEditing)
        {
            m_bottomManipulator = SetupHeightManipulator(manipulatorManagerId, worldTransform, localTransform, nonUniformScale, -1.0f);
        }
    }

    void CapsuleViewportEdit::AddEntityComponentIdPairImpl(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        for (auto manipulator : { m_radiusManipulator.get(), m_topManipulator.get(), m_bottomManipulator.get() })
        {
            if (manipulator)
            {
                manipulator->AddEntityComponentIdPair(entityComponentIdPair);
            }
        }
        AZ_WarningOnce(
            "CapsuleViewportEdit",
            m_radiusManipulator && m_topManipulator && (!m_allowAsymmetricalEditing || m_bottomManipulator),
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
            m_radiusManipulator->SetBoundsDirty();
        }
        for (auto heightManipulator : { m_topManipulator.get(), m_bottomManipulator.get() })
        {
            if (heightManipulator)
            {
                heightManipulator->SetSpace(worldTransform);
                heightManipulator->SetLocalTransform(
                    localTransform * AZ::Transform::CreateTranslation(0.5f * capsuleHeight * heightManipulator->GetAxis()));
                heightManipulator->SetNonUniformScale(nonUniformScale);
                heightManipulator->SetBoundsDirty();
            }
        }
    }

    void CapsuleViewportEdit::ResetValuesImpl()
    {
        SetCapsuleHeight(CapsuleViewportEditConstants::ResetCapsuleHeight);
        SetCapsuleRadius(CapsuleViewportEditConstants::ResetCapsuleRadius);
    }

    void CapsuleViewportEdit::Teardown()
    {
        for (auto manipulator : { m_radiusManipulator.get(), m_topManipulator.get(), m_bottomManipulator.get() })
        {
            if (manipulator)
            {
                manipulator->Unregister();
            }
        }
    }

    void CapsuleViewportEdit::SetupRadiusManipulator(
        const ManipulatorManagerId manipulatorManagerId,
        const AZ::Transform& worldTransform,
        const AZ::Transform& localTransform,
        const AZ::Vector3& nonUniformScale)
    {
        float capsuleRadius = GetCapsuleRadius();
        m_radiusManipulator = LinearManipulator::MakeShared(worldTransform);
        m_radiusManipulator->SetAxis(CapsuleViewportEditConstants::RadiusManipulatorAxis);
        m_radiusManipulator->Register(manipulatorManagerId);
        m_radiusManipulator->SetLocalTransform(
            localTransform * AZ::Transform::CreateTranslation(CapsuleViewportEditConstants::RadiusManipulatorAxis * capsuleRadius));
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

    AZStd::shared_ptr<LinearManipulator> CapsuleViewportEdit::SetupHeightManipulator(
        const ManipulatorManagerId manipulatorManagerId,
        const AZ::Transform& worldTransform,
        const AZ::Transform& localTransform,
        const AZ::Vector3& nonUniformScale,
        float axisDirection)
    {
        float capsuleHeight = GetCapsuleHeight();
        auto manipulator = LinearManipulator::MakeShared(worldTransform);
        manipulator->SetAxis(axisDirection * CapsuleViewportEditConstants::HeightManipulatorAxis);
        manipulator->Register(manipulatorManagerId);
        manipulator->SetLocalTransform(
            localTransform *
            AZ::Transform::CreateTranslation(axisDirection * 0.5f * capsuleHeight * CapsuleViewportEditConstants::HeightManipulatorAxis));
        manipulator->SetNonUniformScale(nonUniformScale);
        {
            ManipulatorViews views;
            views.emplace_back(CreateManipulatorViewQuadBillboard(
                AzFramework::ViewportColors::DefaultManipulatorHandleColor, AzFramework::ViewportConstants::DefaultManipulatorHandleSize));
            manipulator->SetViews(AZStd::move(views));
        }
        manipulator->InstallLeftMouseDownCallback(
            [this]([[maybe_unused]] const LinearManipulator::Action& action)
            {
                BeginEditing();
            });
        manipulator->InstallMouseMoveCallback(
            [this](const LinearManipulator::Action& action)
            {
                OnHeightManipulatorMoved(action);
            });
        manipulator->InstallLeftMouseUpCallback(
            [this]([[maybe_unused]] const LinearManipulator::Action& action)
            {
                EndEditing();
            });
        return manipulator;
    }

    void CapsuleViewportEdit::OnRadiusManipulatorMoved(const LinearManipulator::Action& action)
    {
        // manipulator action offsets do not take entity transform scale into account, so need to apply it here
        const AZ::Transform localTransform = GetLocalTransform();
        const AZ::Vector3 manipulatorPosition = GetPositionInManipulatorFrame(
            m_radiusManipulator->GetSpace().GetUniformScale(), localTransform, action);

        float extent = manipulatorPosition.Dot(action.m_fixed.m_axis);
        extent = AZ::GetMax(extent, CapsuleViewportEditConstants::MinCapsuleRadius);
        m_radiusManipulator->SetLocalTransform(localTransform * AZ::Transform::CreateTranslation(extent * action.m_fixed.m_axis));

        if (m_ensureHeightExceedsTwiceRadius)
        {
            AdjustHeightManipulators(extent);
        }

        SetCapsuleRadius(extent);
    }

    void CapsuleViewportEdit::OnHeightManipulatorMoved(const LinearManipulator::Action& action)
    {
        const bool symmetrical = !m_allowAsymmetricalEditing || action.m_modifiers.IsHeld(DefaultSymmetricalEditingModifier);

        // manipulator action offsets do not take entity transform scale into account, so need to apply it here
        const AZ::Transform localTransform = GetLocalTransform();
        const AZ::Vector3 manipulatorPosition =
            GetPositionInManipulatorFrame(GetManipulatorSpace().GetUniformScale(), localTransform, action);

        // factor of 2 for symmetrical editing because both ends of the capsule move
        const float symmetryFactor = symmetrical ? 2.0f : 1.0f;

        const float oldCapsuleHeight = GetCapsuleHeight();
        const float newAxisLength = AZ::GetMax(
            0.5f * CapsuleViewportEditConstants::MinCapsuleHeight, symmetryFactor * manipulatorPosition.Dot(action.m_fixed.m_axis));
        const float oldAxisLength = 0.5f * symmetryFactor * oldCapsuleHeight;
        const float capsuleHeightDelta = newAxisLength - oldAxisLength;

        const float newCapsuleHeight = AZ::GetMax(oldCapsuleHeight + capsuleHeightDelta, CapsuleViewportEditConstants::MinCapsuleHeight);

        if (m_ensureHeightExceedsTwiceRadius)
        {
            AdjustRadiusManipulator(newCapsuleHeight);
        }

        SetCapsuleHeight(newCapsuleHeight);

        if (!symmetrical)
        {
            const AZ::Vector3 transformedAxis = localTransform.TransformVector(action.m_fixed.m_axis);
            const AZ::Vector3 translationOffsetDelta = 0.5f * (newAxisLength - oldAxisLength) * transformedAxis;
            const AZ::Vector3 translationOffset = GetTranslationOffset();
            SetTranslationOffset(translationOffset + translationOffsetDelta);
        }

        for (auto heightManipulator : { m_topManipulator.get(), m_bottomManipulator.get() })
        {
            if (heightManipulator)
            {
                const AZ::Transform updatedLocalTransform = GetLocalTransform();
                heightManipulator->SetLocalTransform(
                    updatedLocalTransform * AZ::Transform::CreateTranslation(0.5f * newCapsuleHeight * heightManipulator->GetAxis()));
            }
        }
    }

    void CapsuleViewportEdit::AdjustRadiusManipulator(float capsuleHeight)
    {
        float capsuleRadius = GetCapsuleRadius();
        capsuleRadius = AZ::GetMin(capsuleRadius, 0.5f * capsuleHeight);
        const AZ::Transform localTransform = GetLocalTransform();
        m_radiusManipulator->SetLocalTransform(
            localTransform * AZ::Transform::CreateTranslation(capsuleRadius * m_radiusManipulator->GetAxis()));
        SetCapsuleRadius(capsuleRadius);
    }

    void CapsuleViewportEdit::AdjustHeightManipulators(float capsuleRadius)
    {
        float capsuleHeight = GetCapsuleHeight();
        capsuleHeight = AZ::GetMax(capsuleHeight, 2.0f * capsuleRadius);
        const AZ::Transform localTransform = GetLocalTransform();
        for (auto heightManipulator : { m_topManipulator.get(), m_bottomManipulator.get() })
        {
            if (heightManipulator)
            {
                heightManipulator->SetLocalTransform(
                    localTransform * AZ::Transform::CreateTranslation(0.5f * capsuleHeight * heightManipulator->GetAxis()));
            }
        }        
        SetCapsuleHeight(capsuleHeight);
    }
} // namespace AzToolsFramework
