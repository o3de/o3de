/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Manipulators/ManipulatorView.h"
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzFramework/Viewport/ViewportConstants.h>
#include <AzToolsFramework/ComponentModes/QuadViewportEdit.h>
#include <AzToolsFramework/ComponentModes/ViewportEditUtilities.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>

namespace AzToolsFramework
{
    namespace QuadViewportEditConstants
    {
        const AZ::Vector3 WidthManipulatorAxis = AZ::Vector3::CreateAxisX();
        const AZ::Vector3 HeightManipulatorAxis = AZ::Vector3::CreateAxisY();
        const float MinQuadWidth = 0.001f;
        const float MinQuadHeight = 0.002f;

        const float ResetQuadHeight = 1.0f;
        const float ResetQuadWidth = 1.0f;
    } // namespace QuadViewportEditConstants

    QuadViewportEdit::QuadViewportEdit()
    {
    }

    void QuadViewportEdit::InstallSetQuadWidth(AZStd::function<void(float)> setQuadWidth)
    {
        m_setQuadWidth = setQuadWidth;
    }
    void QuadViewportEdit::InstallSetQuadHeight(AZStd::function<void(float)> setQuadHeight)
    {
        m_setQuadHeight = setQuadHeight;
    }
    void QuadViewportEdit::InstallGetQuadWidth(AZStd::function<float()> getQuadWidth)
    {
        m_getQuadWidth = getQuadWidth;
    }
    void QuadViewportEdit::InstallGetQuadHeight(AZStd::function<float()> getQuadHeight)
    {
        m_getQuadHeight = getQuadHeight;
    }

    float QuadViewportEdit::GetQuadHeight() const
    {
        if (m_getQuadHeight)
        {
            return m_getQuadHeight();
        }
        return QuadViewportEditConstants::ResetQuadHeight;
    }
    float QuadViewportEdit::GetQuadWidth() const
    {
        if (m_getQuadWidth)
        {
            return m_getQuadWidth();
        }
        return QuadViewportEditConstants::ResetQuadWidth;
    }
    void QuadViewportEdit::SetQuadWidth(float width)
    {
        if (m_setQuadWidth)
        {
            m_setQuadWidth(width);
        }
    }
    void QuadViewportEdit::SetQuadHeight(float height)
    {
        if (m_setQuadWidth)
        {
            m_setQuadHeight(height);
        }
    }

    void QuadViewportEdit::OnCameraStateChanged(const AzFramework::CameraState& cameraState)
    {
        const AZ::Transform localTransform = GetLocalTransform();
        const AZ::Transform localInverse = localTransform.GetInverse();
        const AZ::Transform manipulatorSpaceInverse = GetManipulatorSpace().GetInverse();
        if (m_widthManipulator)
        {
            const AZ::Vector3 inverseTransformedForward =
                localInverse.TransformVector(manipulatorSpaceInverse.TransformVector(cameraState.m_forward) / GetNonUniformScale());
            if (inverseTransformedForward.Dot(m_widthManipulator->GetAxis()) > 0)
            {
                m_widthManipulator->SetAxis(m_widthManipulator->GetAxis() * -1);
                m_widthManipulator->SetLocalTransform(
                    localTransform * AZ::Transform::CreateTranslation(0.5f * GetQuadWidth() * m_widthManipulator->GetAxis()));
            }
        }

        if (m_heightManipulator)
        {
            const AZ::Vector3 inverseTransformedForward =
                localInverse.TransformVector(manipulatorSpaceInverse.TransformVector(cameraState.m_forward) / GetNonUniformScale());
            if (inverseTransformedForward.Dot(m_heightManipulator->GetAxis()) > 0)
            {
                m_heightManipulator->SetAxis(m_heightManipulator->GetAxis() * -1);
                m_heightManipulator->SetLocalTransform(
                    localTransform * AZ::Transform::CreateTranslation(0.5f * GetQuadHeight() * m_heightManipulator->GetAxis()));
            }
        }
    }

    void QuadViewportEdit::Setup(const ManipulatorManagerId manipulatorManagerId)
    {
        const AZ::Transform worldTransform = GetManipulatorSpace();
        const AZ::Vector3 nonUniformScale = GetNonUniformScale();
        const AZ::Transform localTransform = GetLocalTransform();

        LinearManipulator::MouseActionCallback leftMoustDownHandler = [this]([[maybe_unused]] const LinearManipulator::Action& action)
        {
            m_isMouseDown = true;
            BeginEditing();
        };
        LinearManipulator::MouseActionCallback leftMoustUpHandler = [this]([[maybe_unused]] const LinearManipulator::Action& action)
        {
            m_isMouseDown = false;
            EndEditing();
        };

        {
            m_widthManipulator = LinearManipulator::MakeShared(worldTransform);
            m_widthManipulator->SetAxis(QuadViewportEditConstants::WidthManipulatorAxis);
            m_widthManipulator->Register(manipulatorManagerId);
            m_widthManipulator->SetLocalTransform(
                localTransform * AZ::Transform::CreateTranslation(m_widthManipulator->GetAxis() * GetQuadWidth() * 0.5f));
            m_widthManipulator->SetNonUniformScale(nonUniformScale);
            m_widthManipulator->SetViews(AZStd::move(ManipulatorViews{ CreateManipulatorViewQuadBillboard(
                AzFramework::ViewportColors::DefaultManipulatorHandleColor,
                AzFramework::ViewportConstants::DefaultManipulatorHandleSize) }));
            m_widthManipulator->InstallMouseMoveCallback(
                [this](const LinearManipulator::Action& action)
                {
                    // manipulator action offsets do not take entity transform scale into account, so need to apply it here
                    const AZ::Transform localTransform = GetLocalTransform();
                    const AZ::Vector3 manipulatorPosition =
                        GetPositionInManipulatorFrame(m_widthManipulator->GetSpace().GetUniformScale(), localTransform, action);

                    const float oldQuadWidth = GetQuadWidth();
                    const float newAxisLength =
                        AZ::GetMax(0.5f * QuadViewportEditConstants::MinQuadHeight, manipulatorPosition.Dot(action.m_fixed.m_axis));
                    const float oldAxisLength = 0.5f * oldQuadWidth;
                    const float quadWidthDelta = newAxisLength - oldAxisLength;

                    const float newQuadWidth = AZ::GetMax(oldQuadWidth + quadWidthDelta, QuadViewportEditConstants::MinQuadWidth);
                    SetQuadWidth(newQuadWidth);

                    m_widthManipulator->SetLocalTransform(
                        localTransform * AZ::Transform::CreateTranslation(0.5f * newQuadWidth * m_widthManipulator->GetAxis()));
                });
            m_widthManipulator->InstallLeftMouseDownCallback(leftMoustDownHandler);
            m_widthManipulator->InstallLeftMouseUpCallback(leftMoustUpHandler);
        }
        {
            m_heightManipulator = LinearManipulator::MakeShared(worldTransform);
            m_heightManipulator->SetAxis(QuadViewportEditConstants::HeightManipulatorAxis);
            m_heightManipulator->Register(manipulatorManagerId);
            m_heightManipulator->SetLocalTransform(
                localTransform * AZ::Transform::CreateTranslation(m_heightManipulator->GetAxis() * GetQuadHeight() * 0.5f));
            m_heightManipulator->SetNonUniformScale(nonUniformScale);
            m_heightManipulator->SetViews(AZStd::move(ManipulatorViews{ CreateManipulatorViewQuadBillboard(
                AzFramework::ViewportColors::DefaultManipulatorHandleColor,
                AzFramework::ViewportConstants::DefaultManipulatorHandleSize) }));
            m_heightManipulator->InstallMouseMoveCallback(
                [this](const LinearManipulator::Action& action)
                {
                    // manipulator action offsets do not take entity transform scale into account, so need to apply it here
                    const AZ::Transform localTransform = GetLocalTransform();
                    const AZ::Vector3 manipulatorPosition =
                        GetPositionInManipulatorFrame(m_heightManipulator->GetSpace().GetUniformScale(), localTransform, action);

                    const float oldQuadWidth = GetQuadWidth();
                    const float newAxisLength =
                        AZ::GetMax(0.5f * QuadViewportEditConstants::MinQuadHeight, manipulatorPosition.Dot(action.m_fixed.m_axis));
                    const float oldAxisLength = 0.5f * oldQuadWidth;
                    const float quadWidthDelta = newAxisLength - oldAxisLength;

                    const float newQuadHeight = AZ::GetMax(oldQuadWidth + quadWidthDelta, QuadViewportEditConstants::MinQuadWidth);
                    SetQuadHeight(newQuadHeight);

                    m_heightManipulator->SetLocalTransform(
                        localTransform * AZ::Transform::CreateTranslation(0.5f * newQuadHeight * m_heightManipulator->GetAxis()));
                });
            m_heightManipulator->InstallLeftMouseDownCallback(leftMoustDownHandler);
            m_heightManipulator->InstallLeftMouseUpCallback(leftMoustUpHandler);
        }
    }

    void QuadViewportEdit::AddEntityComponentIdPairImpl(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        for (auto manipulator : { m_widthManipulator.get(), m_heightManipulator.get() })
        {
            if (manipulator)
            {
                manipulator->AddEntityComponentIdPair(entityComponentIdPair);
            }
        }
        AZ_WarningOnce(
            "QuadViewportEdit",
            m_widthManipulator && m_heightManipulator,
            "Attempting to AddEntityComponentIdPair before manipulators have been created");
    }

    void QuadViewportEdit::UpdateManipulators()
    {
        const AZ::Transform worldTransform = GetManipulatorSpace();
        const AZ::Vector3 nonUniformScale = GetNonUniformScale();
        const AZ::Transform localTransform = GetLocalTransform();

        const float quadWidth = GetQuadWidth();
        const float quadHeight = GetQuadHeight();

        if (m_widthManipulator)
        {
            m_widthManipulator->SetSpace(worldTransform);
            m_widthManipulator->SetLocalTransform(
                localTransform * AZ::Transform::CreateTranslation(m_widthManipulator->GetAxis() * quadWidth * 0.5f));
            m_widthManipulator->SetNonUniformScale(nonUniformScale);
            m_widthManipulator->SetBoundsDirty();
        }

        if (m_heightManipulator)
        {
            m_heightManipulator->SetSpace(worldTransform);
            m_heightManipulator->SetLocalTransform(
                localTransform * AZ::Transform::CreateTranslation(m_heightManipulator->GetAxis() * quadHeight * 0.5f));
            m_heightManipulator->SetNonUniformScale(nonUniformScale);
            m_heightManipulator->SetBoundsDirty();
        }
    }

    void QuadViewportEdit::ResetValuesImpl()
    {
        SetQuadWidth(QuadViewportEditConstants::ResetQuadWidth);
        SetQuadHeight(QuadViewportEditConstants::ResetQuadHeight);
    }

    void QuadViewportEdit::Teardown()
    {
        for (auto manipulator : { m_heightManipulator.get(), m_widthManipulator.get() })
        {
            if (manipulator)
            {
                manipulator->Unregister();
            }
        }
    }

} // namespace AzToolsFramework
