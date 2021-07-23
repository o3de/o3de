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
        BoxManipulatorRequestBus::EventResult(
            boxWorldFromLocal, m_entityComponentIdPair, &BoxManipulatorRequests::GetCurrentTransform);

        AZ::Vector3 boxScale = AZ::Vector3::CreateOne();
        BoxManipulatorRequestBus::EventResult(
            boxScale, m_entityComponentIdPair, &BoxManipulatorRequests::GetBoxScale);

        AZ::Vector3 boxDimensions = AZ::Vector3::CreateZero();
        BoxManipulatorRequestBus::EventResult(
            boxDimensions, m_entityComponentIdPair, &BoxManipulatorRequests::GetDimensions);

        // ensure we apply the entity scale to the box dimensions so
        // the manipulators appear in the correct location
        boxDimensions *= boxScale;

        for (size_t manipulatorIndex = 0; manipulatorIndex < m_linearManipulators.size(); ++manipulatorIndex)
        {
            if (auto& linearManipulator = m_linearManipulators[manipulatorIndex])
            {
                linearManipulator->SetSpace(boxWorldFromLocal);
                linearManipulator->SetLocalTransform(
                    AZ::Transform::CreateTranslation(s_boxAxes[manipulatorIndex] * 0.5f * boxDimensions));
                linearManipulator->SetBoundsDirty();
            }
        }
    }

    void BoxViewportEdit::Setup(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        m_entityComponentIdPair = entityComponentIdPair;

        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        BoxManipulatorRequestBus::EventResult(
            worldFromLocal, entityComponentIdPair, &BoxManipulatorRequests::GetCurrentTransform);

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
                    // calculate the amount of displacement along an axis this manipulator has moved
                    // clamp movement so it cannot go negative based on axis direction
                    const AZ::Vector3 axisDisplacement =
                        action.LocalPosition().GetAbs() * 2.0f
                        * AZ::GetMax<float>(0.0f, action.LocalPosition().GetNormalized().Dot(action.m_fixed.m_axis));

                    AZ::Vector3 boxScale = AZ::Vector3::CreateOne();
                    BoxManipulatorRequestBus::EventResult(
                        boxScale, entityComponentIdPair, &BoxManipulatorRequests::GetBoxScale);

                    AZ::Vector3 boxDimensions = AZ::Vector3::CreateZero();
                    BoxManipulatorRequestBus::EventResult(
                        boxDimensions, entityComponentIdPair, &BoxManipulatorRequests::GetDimensions);

                    // ensure we take into account the entity scale using the axis displacement
                    const AZ::Vector3 scaledAxisDisplacement =
                        axisDisplacement / boxScale;

                    // update dimensions - preserve dimensions not effected by this
                    // axis, and update current axis displacement
                    BoxManipulatorRequestBus::Event(
                        entityComponentIdPair, &BoxManipulatorRequests::SetDimensions,
                        (NotAxis(action.m_fixed.m_axis) * boxDimensions).GetMax(scaledAxisDisplacement));

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
