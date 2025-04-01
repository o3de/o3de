/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzToolsFramework/ComponentModes/CylinderViewportEdit.h>

#include <AzFramework/Viewport/ViewportColors.h>
#include <AzFramework/Viewport/ViewportConstants.h>
#include <AzToolsFramework/ComponentModes/ViewportEditUtilities.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>

namespace AzToolsFramework
{
    namespace CylinderViewportEditConstants
    {
        const AZ::Vector3 RadiusManipulatorAxis = AZ::Vector3::CreateAxisX();
        const AZ::Vector3 HeightManipulatorAxis = AZ::Vector3::CreateAxisZ();
        const float MinCylinderRadius = 0.001f;
        const float MinCylinderHeight = 0.002f;
        const float ResetCylinderHeight = 1.0f;
        const float ResetCylindserRadius = 0.25f;
    } // namespace CylinderViewportEditConstants

    CylinderViewportEdit::CylinderViewportEdit(bool allowAsymmetricalEditing)
        : m_allowAsymmetricalEditing(allowAsymmetricalEditing)
    {
    }

    void CylinderViewportEdit::InstallGetCylinderRadius(AZStd::function<float()> getCylinderRadius)
    {
        m_getCylinderRadius = AZStd::move(getCylinderRadius);
    }

    void CylinderViewportEdit::InstallGetCylinderHeight(AZStd::function<float()> getCylinderHeight)
    {
        m_getCylinderHeight = AZStd::move(getCylinderHeight);
    }

    void CylinderViewportEdit::InstallSetCylinderRadius(AZStd::function<void(float)> setCylinderRadius)
    {
        m_setCylinderRadius = AZStd::move(setCylinderRadius);
    }

    void CylinderViewportEdit::InstallSetCylinderHeight(AZStd::function<void(float)> setCylinderHeight)
    {
        m_setCylinderHeight = AZStd::move(setCylinderHeight);
    }

    float CylinderViewportEdit::GetCylinderRadius() const
    {
        if (m_getCylinderRadius)
        {
            return m_getCylinderRadius();
        }
        AZ_ErrorOnce("CylinderViewportEdit", false, "No implementation provided for GetCylinderRadius");
        return CylinderViewportEditConstants::ResetCylindserRadius;
    }

    float CylinderViewportEdit::GetCylinderHeight() const
    {
        if (m_getCylinderHeight)
        {
            return m_getCylinderHeight();
        }
        AZ_ErrorOnce("CylinderViewportEdit", false, "No implementation provided for GetCylinderHeight");
        return CylinderViewportEditConstants::ResetCylinderHeight;
    }

    void CylinderViewportEdit::SetCylinderRadius(float radius)
    {
        if (m_setCylinderRadius)
        {
            m_setCylinderRadius(radius);
        }
        else
        {
            AZ_ErrorOnce("CylinderViewportEdit", false, "No implementation provided for SetCylinderRadius");
        }
    }

    void CylinderViewportEdit::SetCylinderHeight(float height)
    {
        if (m_setCylinderHeight)
        {
            m_setCylinderHeight(height);
        }
        else
        {
            AZ_ErrorOnce("CylinderViewportEdit", false, "No implementation provided for SetCylinderHeight");
        }
    }

    void CylinderViewportEdit::OnCameraStateChanged(const AzFramework::CameraState& cameraState)
    {
        if (m_radiusManipulator)
        {
            const float radius = GetCylinderRadius();
            const AZ::Transform localTransform = GetLocalTransform();
            const AZ::Transform localInverse = localTransform.GetInverse();
            const AZ::Transform manipulatorSpaceInverse = GetManipulatorSpace().GetInverse();

            const AZ::Vector3 inverseTransformedForward =
                localInverse.TransformVector(manipulatorSpaceInverse.TransformVector(cameraState.m_forward) / GetNonUniformScale());
            AZ::Vector3 axis = inverseTransformedForward.Cross(CylinderViewportEditConstants::HeightManipulatorAxis);
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

    void CylinderViewportEdit::Setup(const ManipulatorManagerId manipulatorManagerId)
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

    void CylinderViewportEdit::AddEntityComponentIdPairImpl(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        for (auto manipulator : { m_radiusManipulator.get(), m_topManipulator.get(), m_bottomManipulator.get() })
        {
            if (manipulator)
            {
                manipulator->AddEntityComponentIdPair(entityComponentIdPair);
            }
        }
        AZ_WarningOnce(
            "CylinderViewportEdit",
            m_radiusManipulator && m_topManipulator && (!m_allowAsymmetricalEditing || m_bottomManipulator),
            "Attempting to AddEntityComponentIdPair before manipulators have been created");
    }

    void CylinderViewportEdit::UpdateManipulators()
    {
        const AZ::Transform worldTransform = GetManipulatorSpace();
        const AZ::Vector3 nonUniformScale = GetNonUniformScale();
        const AZ::Transform localTransform = GetLocalTransform();

        const float cylinderHeight = GetCylinderHeight();
        const float cylinderRadius = GetCylinderRadius();

        if (m_radiusManipulator)
        {
            m_radiusManipulator->SetSpace(worldTransform);
            m_radiusManipulator->SetLocalTransform(
                localTransform * AZ::Transform::CreateTranslation(m_radiusManipulator->GetAxis() * cylinderRadius));
            m_radiusManipulator->SetNonUniformScale(nonUniformScale);
            m_radiusManipulator->SetBoundsDirty();
        }
        for (auto heightManipulator : { m_topManipulator.get(), m_bottomManipulator.get() })
        {
            if (heightManipulator)
            {
                heightManipulator->SetSpace(worldTransform);
                heightManipulator->SetLocalTransform(
                    localTransform * AZ::Transform::CreateTranslation(0.5f * cylinderHeight * heightManipulator->GetAxis()));
                heightManipulator->SetNonUniformScale(nonUniformScale);
                heightManipulator->SetBoundsDirty();
            }
        }
    }

    void CylinderViewportEdit::ResetValuesImpl()
    {
        SetCylinderHeight(CylinderViewportEditConstants::ResetCylinderHeight);
        SetCylinderRadius(CylinderViewportEditConstants::ResetCylindserRadius);
    }

    void CylinderViewportEdit::Teardown()
    {
        for (auto manipulator : { m_radiusManipulator.get(), m_topManipulator.get(), m_bottomManipulator.get() })
        {
            if (manipulator)
            {
                manipulator->Unregister();
            }
        }
    }

    void CylinderViewportEdit::SetupRadiusManipulator(
        const ManipulatorManagerId manipulatorManagerId,
        const AZ::Transform& worldTransform,
        const AZ::Transform& localTransform,
        const AZ::Vector3& nonUniformScale)
    {
        float cylinderRadius = GetCylinderRadius();
        m_radiusManipulator = LinearManipulator::MakeShared(worldTransform);
        m_radiusManipulator->SetAxis(CylinderViewportEditConstants::RadiusManipulatorAxis);
        m_radiusManipulator->Register(manipulatorManagerId);
        m_radiusManipulator->SetLocalTransform(
            localTransform * AZ::Transform::CreateTranslation(CylinderViewportEditConstants::RadiusManipulatorAxis * cylinderRadius));
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

    AZStd::shared_ptr<LinearManipulator> CylinderViewportEdit::SetupHeightManipulator(
        const ManipulatorManagerId manipulatorManagerId,
        const AZ::Transform& worldTransform,
        const AZ::Transform& localTransform,
        const AZ::Vector3& nonUniformScale,
        float axisDirection)
    {
        float cylinderHeight = GetCylinderHeight();
        auto manipulator = LinearManipulator::MakeShared(worldTransform);
        manipulator->SetAxis(axisDirection * CylinderViewportEditConstants::HeightManipulatorAxis);
        manipulator->Register(manipulatorManagerId);
        manipulator->SetLocalTransform(
            localTransform *
            AZ::Transform::CreateTranslation(axisDirection * 0.5f * cylinderHeight * CylinderViewportEditConstants::HeightManipulatorAxis));
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

    void CylinderViewportEdit::OnRadiusManipulatorMoved(const LinearManipulator::Action& action)
    {
        // manipulator action offsets do not take entity transform scale into account, so need to apply it here
        const AZ::Transform localTransform = GetLocalTransform();
        const AZ::Vector3 manipulatorPosition = GetPositionInManipulatorFrame(
            m_radiusManipulator->GetSpace().GetUniformScale(), localTransform, action);

        float extent = manipulatorPosition.Dot(action.m_fixed.m_axis);
        extent = AZ::GetMax(extent, CylinderViewportEditConstants::MinCylinderRadius);
        m_radiusManipulator->SetLocalTransform(localTransform * AZ::Transform::CreateTranslation(extent * action.m_fixed.m_axis));

        SetCylinderRadius(extent);
    }

    void CylinderViewportEdit::OnHeightManipulatorMoved(const LinearManipulator::Action& action)
    {
        const bool symmetrical = !m_allowAsymmetricalEditing || action.m_modifiers.IsHeld(DefaultSymmetricalEditingModifier);

        // manipulator action offsets do not take entity transform scale into account, so need to apply it here
        const AZ::Transform localTransform = GetLocalTransform();
        const AZ::Vector3 manipulatorPosition =
            GetPositionInManipulatorFrame(GetManipulatorSpace().GetUniformScale(), localTransform, action);

        // factor of 2 for symmetrical editing because both ends of the cylinder move
        const float symmetryFactor = symmetrical ? 2.0f : 1.0f;

        const float oldCylinderHeight = GetCylinderHeight();
        const float newAxisLength = AZ::GetMax(
            0.5f * CylinderViewportEditConstants::MinCylinderHeight, symmetryFactor * manipulatorPosition.Dot(action.m_fixed.m_axis));
        const float oldAxisLength = 0.5f * symmetryFactor * oldCylinderHeight ;
        const float cylinderHeightDelta = newAxisLength - oldAxisLength;

        const float newCylinderHeight = AZ::GetMax(oldCylinderHeight  + cylinderHeightDelta, CylinderViewportEditConstants::MinCylinderHeight);

        SetCylinderHeight(newCylinderHeight);

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
                    updatedLocalTransform * AZ::Transform::CreateTranslation(0.5f * newCylinderHeight * heightManipulator->GetAxis()));
            }
        }
    }

} // namespace AzToolsFramework

