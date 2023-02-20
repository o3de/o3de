/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ComponentModes/BoxViewportEdit.h>
#include <AzToolsFramework/ComponentModes/ViewportEditUtilities.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzFramework/Viewport/ViewportConstants.h>

namespace AzToolsFramework
{
    namespace BoxViewportEditConstants
    {
        const float MinBoxDimension = 0.001f;
    } // namespace BoxViewportEditConstants

    const AZStd::array<AZ::Vector3, 6> s_boxAxes =
    { {
        AZ::Vector3::CreateAxisX(), -AZ::Vector3::CreateAxisX(),
        AZ::Vector3::CreateAxisY(), -AZ::Vector3::CreateAxisY(),
        AZ::Vector3::CreateAxisZ(), -AZ::Vector3::CreateAxisZ()
    } };

    BoxViewportEdit::BoxViewportEdit(bool allowAsymmetricalEditing)
        : m_allowAsymmetricalEditing(allowAsymmetricalEditing)
    {
    }

    void BoxViewportEdit::InstallGetBoxDimensions(AZStd::function<AZ::Vector3()> getBoxDimensions)
    {
        m_getBoxDimensions = AZStd::move(getBoxDimensions);
    }

    void BoxViewportEdit::InstallSetBoxDimensions(AZStd::function<void(const AZ::Vector3&)> setBoxDimensions)
    {
        m_setBoxDimensions = AZStd::move(setBoxDimensions);
    }

    AZ::Vector3 BoxViewportEdit::GetBoxDimensions() const
    {
        if (m_getBoxDimensions)
        {
            return m_getBoxDimensions();
        }
        AZ_ErrorOnce("BoxViewportEdit", false, "No implementation provided for GetBoxDimensions");
        return AZ::Vector3::CreateOne();
    }

    void BoxViewportEdit::SetBoxDimensions(const AZ::Vector3& boxDimensions)
    {
        if (m_setBoxDimensions)
        {
            m_setBoxDimensions(boxDimensions);
        }
        else
        {
            AZ_ErrorOnce("BoxViewportEdit", false, "No implementation provided for SetBoxDimensions");
        }
    }

    void BoxViewportEdit::UpdateManipulators()
    {
        const AZ::Transform manipulatorSpace = GetManipulatorSpace();
        const AZ::Vector3 nonUniformScale = GetNonUniformScale();
        const AZ::Vector3 boxDimensions = GetBoxDimensions();
        const AZ::Transform boxLocalTransform = GetLocalTransform();

        for (size_t manipulatorIndex = 0; manipulatorIndex < m_linearManipulators.size(); ++manipulatorIndex)
        {
            if (auto& linearManipulator = m_linearManipulators[manipulatorIndex])
            {
                linearManipulator->SetSpace(manipulatorSpace);
                linearManipulator->SetLocalTransform(
                    boxLocalTransform * AZ::Transform::CreateTranslation(s_boxAxes[manipulatorIndex] * 0.5f * boxDimensions));
                linearManipulator->SetNonUniformScale(nonUniformScale);
                linearManipulator->SetBoundsDirty();
            }
        }
    }

    void BoxViewportEdit::AddEntityComponentIdPairImpl(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        for (size_t manipulatorIndex = 0; manipulatorIndex < m_linearManipulators.size(); ++manipulatorIndex)
        {
            if (auto& linearManipulator = m_linearManipulators[manipulatorIndex]; linearManipulator)
            {
                linearManipulator->AddEntityComponentIdPair(entityComponentIdPair);
            }
            else
            {
                AZ_WarningOnce("BoxViewportEdit", false, "Attempting to AddEntityComponentIdPair before manipulators have been created");
            }
        }
    }

    void BoxViewportEdit::Setup(const ManipulatorManagerId manipulatorManagerId)
    {
        const AZ::Transform worldFromLocal = GetManipulatorSpace();

        for (size_t manipulatorIndex = 0; manipulatorIndex < m_linearManipulators.size(); ++manipulatorIndex)
        {
            auto& linearManipulator = m_linearManipulators[manipulatorIndex];

            if (linearManipulator == nullptr)
            {
                linearManipulator = LinearManipulator::MakeShared(worldFromLocal);
                linearManipulator->SetAxis(s_boxAxes[manipulatorIndex]);

                ManipulatorViews views;
                views.emplace_back(CreateManipulatorViewQuadBillboard(
                    AzFramework::ViewportColors::DefaultManipulatorHandleColor,
                    AzFramework::ViewportConstants::DefaultManipulatorHandleSize));
                linearManipulator->SetViews(AZStd::move(views));

                linearManipulator->InstallMouseMoveCallback(
                    [this,
                     transformScale{ linearManipulator->GetSpace().GetUniformScale() },
                     nonUniformScale{ linearManipulator->GetNonUniformScale() }](const LinearManipulator::Action& action)
                {
                    const bool symmetrical = !m_allowAsymmetricalEditing || action.m_modifiers.IsHeld(DefaultSymmetricalEditingModifier);

                    const AZ::Transform boxLocalTransform = GetLocalTransform();
                    const AZ::Vector3 manipulatorPosition = GetPositionInManipulatorFrame(transformScale, boxLocalTransform, action);
                    const AZ::Vector3 boxDimensions = GetBoxDimensions();

                    // factor of 2 for symmetrical editing because both ends of the box move
                    const float symmetryFactor = symmetrical ? 2.0f : 1.0f; 

                    const float newAxisLength = AZ::GetMax(
                        0.5f * BoxViewportEditConstants::MinBoxDimension, symmetryFactor * manipulatorPosition.Dot(action.m_fixed.m_axis));
                    const float oldAxisLength = 0.5f * symmetryFactor * boxDimensions.Dot(action.m_fixed.m_axis.GetAbs());
                    const AZ::Vector3 dimensionsDelta = (newAxisLength - oldAxisLength) * action.m_fixed.m_axis.GetAbs();

                    SetBoxDimensions(boxDimensions + dimensionsDelta);

                    if (!symmetrical)
                    {
                        const AZ::Vector3 transformedAxis = nonUniformScale * boxLocalTransform.TransformVector(action.m_fixed.m_axis);
                        const AZ::Vector3 translationOffsetDelta = 0.5f * (newAxisLength - oldAxisLength) * transformedAxis;
                        const AZ::Vector3 translationOffset = GetTranslationOffset();
                        SetTranslationOffset(translationOffset + translationOffsetDelta);
                    }

                    UpdateManipulators();
                });
            }

            linearManipulator->Register(manipulatorManagerId);
        }

        UpdateManipulators();
    }

    void BoxViewportEdit::Teardown()
    {
        for (auto& linearManipulator : m_linearManipulators)
        {
            if (linearManipulator)
            {
                linearManipulator->Unregister();
                linearManipulator.reset();
            }
        }
    }

    void BoxViewportEdit::ResetValuesImpl()
    {
        SetBoxDimensions(AZ::Vector3::CreateOne());
        SetTranslationOffset(AZ::Vector3::CreateZero());
    }
} // namespace AzToolsFramework
