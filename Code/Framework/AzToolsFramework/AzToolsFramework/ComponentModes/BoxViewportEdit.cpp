/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ComponentModes/BoxViewportEdit.h>
#include <AzToolsFramework/Manipulators/BoxManipulatorRequestBus.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
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

    /// Pass a single axis, and return not of elements
    /// Example: In -> (1, 0, 0) Out -> (0, 1, 1)
    static AZ::Vector3 NotAxis(const AZ::Vector3& offset)
    {
        return AZ::Vector3::CreateOne() - AZ::Vector3(
            fabsf(Sign(offset.GetX())),
            fabsf(Sign(offset.GetY())),
            fabsf(Sign(offset.GetZ())));
    }

    void BoxViewportEdit::UpdateManipulators()
    {
        AZ::Transform boxWorldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(
            boxWorldFromLocal, m_entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

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
                linearManipulator->SetSpace(boxWorldFromLocal);
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
                    [this, entityComponentIdPair,
                     transformScale{ linearManipulator->GetSpace().GetUniformScale() }](const LinearManipulator::Action& action)
                {
                    AZ::Transform boxLocalTransform = AZ::Transform::CreateIdentity();
                    BoxManipulatorRequestBus::EventResult(
                        boxLocalTransform, entityComponentIdPair, &BoxManipulatorRequestBus::Events::GetCurrentLocalTransform);

                    const AZ::Vector3 manipulatorPosition = GetPositionInManipulatorFrame(transformScale, boxLocalTransform, action);

                    // calculate the amount of displacement along an axis this manipulator has moved
                    // clamp movement so it cannot go negative based on axis direction
                    const AZ::Vector3 axisDisplacement =
                        manipulatorPosition.GetAbs() * 2.0f
                        * AZ::GetMax(0.0f, manipulatorPosition.GetNormalized().Dot(action.m_fixed.m_axis));

                    AZ::Vector3 boxDimensions = AZ::Vector3::CreateZero();
                    BoxManipulatorRequestBus::EventResult(
                        boxDimensions, entityComponentIdPair, &BoxManipulatorRequestBus::Events::GetDimensions);

                    // update dimensions - preserve dimensions not effected by this
                    // axis, and update current axis displacement
                    BoxManipulatorRequestBus::Event(
                        entityComponentIdPair, &BoxManipulatorRequestBus::Events::SetDimensions,
                        (NotAxis(action.m_fixed.m_axis) * boxDimensions).GetMax(axisDisplacement));

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

    AZ::Vector3 GetPositionInManipulatorFrame(float worldUniformScale, const AZ::Transform& manipulatorLocalTransform,
        const LinearManipulator::Action& action)
    {
        return manipulatorLocalTransform.GetInverse().TransformPoint(
            action.m_start.m_localPosition +
            action.m_current.m_localPositionOffset / AZ::GetClamp(worldUniformScale, AZ::MinTransformScale, AZ::MaxTransformScale));
    }
} // namespace AzToolsFramework
