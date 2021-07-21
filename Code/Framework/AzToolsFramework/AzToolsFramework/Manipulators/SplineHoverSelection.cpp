/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SplineHoverSelection.h"

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Spline.h>
#include <AzToolsFramework/Manipulators/EditorVertexSelection.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Manipulators/SplineSelectionManipulator.h>

namespace AzToolsFramework
{
    static const AZ::Color SplineSelectManipulatorColor = AZ::Color(0.0f, 1.0f, 0.0f, 1.0f);

    SplineHoverSelection::SplineHoverSelection(
        const AZ::EntityComponentIdPair& entityComponentIdPair,
        const ManipulatorManagerId managerId,
        const AZStd::shared_ptr<AZ::Spline>& spline)
    {
        m_splineSelectionManipulator = SplineSelectionManipulator::MakeShared();
        m_splineSelectionManipulator->Register(managerId);
        m_splineSelectionManipulator->AddEntityComponentIdPair(entityComponentIdPair);
        m_splineSelectionManipulator->SetSpace(WorldFromLocalWithUniformScale(entityComponentIdPair.GetEntityId()));

        const float splineWidth = 0.05f;
        m_splineSelectionManipulator->SetSpline(spline);
        m_splineSelectionManipulator->SetView(
            CreateManipulatorViewSplineSelect(*m_splineSelectionManipulator, SplineSelectManipulatorColor, splineWidth));

        m_splineSelectionManipulator->InstallLeftMouseUpCallback(
            [entityComponentIdPair](const SplineSelectionManipulator::Action& action)
            {
                InsertVertexAfter(entityComponentIdPair, action.m_splineAddress.m_segmentIndex, action.m_localSplineHitPosition);
            });
    }

    SplineHoverSelection::~SplineHoverSelection()
    {
        m_splineSelectionManipulator->Unregister();
    }

    void SplineHoverSelection::Register(ManipulatorManagerId managerId)
    {
        if (m_splineSelectionManipulator)
        {
            m_splineSelectionManipulator->Register(managerId);
        }
    }

    void SplineHoverSelection::Unregister()
    {
        if (m_splineSelectionManipulator)
        {
            m_splineSelectionManipulator->Unregister();
        }
    }

    void SplineHoverSelection::SetBoundsDirty()
    {
        if (m_splineSelectionManipulator)
        {
            m_splineSelectionManipulator->SetBoundsDirty();
        }
    }

    void SplineHoverSelection::Refresh()
    {
        // do nothing
    }

    void SplineHoverSelection::SetSpace(const AZ::Transform& worldFromLocal)
    {
        if (m_splineSelectionManipulator)
        {
            m_splineSelectionManipulator->SetSpace(worldFromLocal);
        }
    }

    void SplineHoverSelection::SetNonUniformScale(const AZ::Vector3& nonUniformScale)
    {
        if (m_splineSelectionManipulator)
        {
            m_splineSelectionManipulator->SetNonUniformScale(nonUniformScale);
        }
    }
} // namespace AzToolsFramework
