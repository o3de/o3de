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
            nonUniformScale, m_entityComponentIdPair.GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

        AZ::Vector3 boxDimensions = AZ::Vector3::CreateZero();
        BoxManipulatorRequestBus::EventResult(
            boxDimensions, m_entityComponentIdPair, &BoxManipulatorRequests::GetDimensions);

        AZ::Transform boxLocalTransform = AZ::Transform::CreateIdentity();
        BoxManipulatorRequestBus::EventResult(
            boxLocalTransform, m_entityComponentIdPair, &BoxManipulatorRequests::GetCurrentLocalTransform);

        for (size_t manipulatorIndex = 0; manipulatorIndex < m_linearManipulators.size(); ++manipulatorIndex)
        {
            if (auto& linearManipulator = m_linearManipulators[manipulatorIndex])
            {
                linearManipulator->SetSpace(boxWorldFromLocal);
                linearManipulator->SetLocalTransform(boxLocalTransform * AZ::Transform::CreateTranslation(
                    s_boxAxes[manipulatorIndex] * 0.5f * boxDimensions));
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
                    AzFramework::ViewportColors::DefaultManipulatorHandleColor, AzFramework::ViewportConstants::DefaultManipulatorHandleSize));
                linearManipulator->SetViews(AZStd::move(views));

                linearManipulator->InstallMouseMoveCallback(
                    [this, entityComponentIdPair](
                        const LinearManipulator::Action& action)
                {
                    AZ::Transform boxLocalTransform = AZ::Transform::CreateIdentity();
                    BoxManipulatorRequestBus::EventResult(
                        boxLocalTransform, entityComponentIdPair, &BoxManipulatorRequests::GetCurrentLocalTransform);

                    AZ::Transform boxWorldTransform = AZ::Transform::CreateIdentity();
                    AZ::TransformBus::EventResult(
                        boxWorldTransform, m_entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
                    float boxScale = AZ::GetMax(AZ::MinTransformScale, boxWorldTransform.GetUniformScale());

                    // calculate the position of the manipulator in the reference frame of the box
                    // the local position offset of the manipulator does not take the transform scale into account, so need to apply it here
                    const AZ::Vector3 localPosition = boxLocalTransform.GetInverse().TransformPoint(
                        action.m_start.m_localPosition + action.m_current.m_localPositionOffset / boxScale);

                    // calculate the amount of displacement along an axis this manipulator has moved
                    // clamp movement so it cannot go negative based on axis direction
                    const AZ::Vector3 axisDisplacement =
                        localPosition.GetAbs() * 2.0f
                        * AZ::GetMax<float>(0.0f, localPosition.GetNormalized().Dot(action.m_fixed.m_axis));

                    AZ::Vector3 boxDimensions = AZ::Vector3::CreateZero();
                    BoxManipulatorRequestBus::EventResult(
                        boxDimensions, entityComponentIdPair, &BoxManipulatorRequests::GetDimensions);

                    // update dimensions - preserve dimensions not effected by this
                    // axis, and update current axis displacement
                    BoxManipulatorRequestBus::Event(
                        entityComponentIdPair, &BoxManipulatorRequests::SetDimensions,
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
} // namespace AzToolsFramework
