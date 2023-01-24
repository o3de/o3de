/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ComponentModes/BoxViewportEdit.h>
#include <AzToolsFramework/ComponentModes/ViewportEditUtilities.h>
#include <AzToolsFramework/Manipulators/BoxManipulatorRequestBus.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Manipulators/ShapeManipulatorRequestBus.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzFramework/Viewport/ViewportConstants.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/NonUniformScaleBus.h>

namespace AzToolsFramework
{
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

    void BoxViewportEdit::UpdateManipulators()
    {
        AZ::Transform manipulatorSpace = AZ::Transform::CreateIdentity();
        ShapeManipulatorRequestBus::EventResult(
            manipulatorSpace, m_entityComponentIdPair, &ShapeManipulatorRequestBus::Events::GetManipulatorSpace);

        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(
            nonUniformScale, m_entityComponentIdPair.GetEntityId(), &AZ::NonUniformScaleRequestBus::Events::GetScale);

        AZ::Vector3 boxDimensions = AZ::Vector3::CreateZero();
        BoxManipulatorRequestBus::EventResult(
            boxDimensions, m_entityComponentIdPair, &BoxManipulatorRequestBus::Events::GetDimensions);

        AZ::Transform boxLocalTransform = AZ::Transform::CreateIdentity();
        BoxManipulatorRequestBus::EventResult(
            boxLocalTransform, m_entityComponentIdPair, &BoxManipulatorRequestBus::Events::GetCurrentLocalTransform);

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

    void BoxViewportEdit::Setup(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        m_entityComponentIdPair = entityComponentIdPair;

        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(
            worldFromLocal, entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        for (size_t manipulatorIndex = 0; manipulatorIndex < m_linearManipulators.size(); ++manipulatorIndex)
        {
            auto& linearManipulator = m_linearManipulators[manipulatorIndex];

            if (linearManipulator == nullptr)
            {
                linearManipulator = LinearManipulator::MakeShared(worldFromLocal);

                linearManipulator->AddEntityComponentIdPair(entityComponentIdPair);
                linearManipulator->SetAxis(s_boxAxes[manipulatorIndex]);

                ManipulatorViews views;
                views.emplace_back(CreateManipulatorViewQuadBillboard(
                    AzFramework::ViewportColors::DefaultManipulatorHandleColor,
                    AzFramework::ViewportConstants::DefaultManipulatorHandleSize));
                linearManipulator->SetViews(AZStd::move(views));

                linearManipulator->InstallMouseMoveCallback(
                    [this,
                     entityComponentIdPair,
                     transformScale{ linearManipulator->GetSpace().GetUniformScale() },
                     nonUniformScale{ linearManipulator->GetNonUniformScale() }](const LinearManipulator::Action& action)
                {
                    const bool symmetrical = !m_allowAsymmetricalEditing || action.m_modifiers.IsHeld(DefaultSymmetricalEditingModifier);

                    AZ::Transform boxLocalTransform = AZ::Transform::CreateIdentity();
                    BoxManipulatorRequestBus::EventResult(
                        boxLocalTransform, entityComponentIdPair, &BoxManipulatorRequestBus::Events::GetCurrentLocalTransform);

                    const AZ::Vector3 manipulatorPosition = GetPositionInManipulatorFrame(transformScale, boxLocalTransform, action);

                    AZ::Vector3 boxDimensions = AZ::Vector3::CreateZero();
                    BoxManipulatorRequestBus::EventResult(
                        boxDimensions, entityComponentIdPair, &BoxManipulatorRequestBus::Events::GetDimensions);

                    // factor of 2 for symmetrical editing because both ends of the box move
                    const float symmetryFactor = symmetrical ? 2.0f : 1.0f; 

                    const float newAxisLength = AZ::GetMax(0.0f, symmetryFactor * manipulatorPosition.Dot(action.m_fixed.m_axis));
                    const float oldAxisLength = 0.5f * symmetryFactor * boxDimensions.Dot(action.m_fixed.m_axis.GetAbs());
                    const AZ::Vector3 dimensionsDelta = (newAxisLength - oldAxisLength) * action.m_fixed.m_axis.GetAbs();

                    BoxManipulatorRequestBus::Event(
                        entityComponentIdPair, &BoxManipulatorRequestBus::Events::SetDimensions, boxDimensions + dimensionsDelta);

                    if (!symmetrical)
                    {
                        const AZ::Vector3 transformedAxis = nonUniformScale * boxLocalTransform.TransformVector(action.m_fixed.m_axis);
                        const AZ::Vector3 translationOffsetDelta = 0.5f * (newAxisLength - oldAxisLength) * transformedAxis;
                        AZ::Vector3 translationOffset = AZ::Vector3::CreateZero();
                        ShapeManipulatorRequestBus::EventResult(
                            translationOffset, entityComponentIdPair, &ShapeManipulatorRequestBus::Events::GetTranslationOffset);
                        ShapeManipulatorRequestBus::Event(
                            entityComponentIdPair,
                            &ShapeManipulatorRequestBus::Events::SetTranslationOffset,
                            translationOffset + translationOffsetDelta);
                    }

                    UpdateManipulators();
                });
            }

            linearManipulator->Register(g_mainManipulatorManagerId);
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

    void BoxViewportEdit::ResetValues()
    {
        BoxManipulatorRequestBus::Event(
            m_entityComponentIdPair, &BoxManipulatorRequestBus::Events::SetDimensions, AZ::Vector3::CreateOne());
        ShapeManipulatorRequestBus::Event(
            m_entityComponentIdPair, &ShapeManipulatorRequestBus::Events::SetTranslationOffset, AZ::Vector3::CreateZero());
    }
} // namespace AzToolsFramework
