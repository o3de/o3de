/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LineHoverSelection.h"

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/VertexContainerInterface.h>
#include <AzToolsFramework/Manipulators/EditorVertexSelection.h>
#include <AzToolsFramework/Manipulators/LineSegmentSelectionManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>

namespace AzToolsFramework
{
    static const AZ::Color LineSelectManipulatorColor = AZ::Color(0.0f, 1.0f, 0.0f, 1.0f);

    template<typename Vertex>
    static void UpdateLineSegmentPosition(const size_t vertIndex, const AZ::EntityId entityId, LineSegmentSelectionManipulator& lineSegment)
    {
        Vertex start;
        bool foundStart = false;
        AZ::FixedVerticesRequestBus<Vertex>::EventResult(
            foundStart, entityId, &AZ::FixedVerticesRequestBus<Vertex>::Handler::GetVertex, vertIndex, start);

        if (foundStart)
        {
            lineSegment.SetStart(AdaptVertexOut(start));
        }

        size_t size = 0;
        AZ::FixedVerticesRequestBus<Vertex>::EventResult(size, entityId, &AZ::FixedVerticesRequestBus<Vertex>::Handler::Size);

        Vertex end;
        bool foundEnd = false;
        AZ::FixedVerticesRequestBus<Vertex>::EventResult(
            foundEnd, entityId, &AZ::FixedVerticesRequestBus<Vertex>::Handler::GetVertex, (vertIndex + 1) % size, end);

        if (foundEnd)
        {
            lineSegment.SetEnd(AdaptVertexOut(end));
        }

        // update the view
        const float lineWidth = 0.05f;
        lineSegment.SetView(CreateManipulatorViewLineSelect(lineSegment, LineSelectManipulatorColor, lineWidth));
    }

    template<typename Vertex>
    LineSegmentHoverSelection<Vertex>::LineSegmentHoverSelection(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const ManipulatorManagerId managerId)
        : m_entityId(entityComponentIdPair.GetEntityId())
    {
        // create a line segment manipulator from vertex positions and setup its callback
        auto setupLineSegment =
            [this](const AZ::EntityComponentIdPair& entityComponentIdPair, const ManipulatorManagerId managerId, const size_t vertIndex)
        {
            m_lineSegmentManipulators.push_back(LineSegmentSelectionManipulator::MakeShared());
            AZStd::shared_ptr<LineSegmentSelectionManipulator>& lineSegmentManipulator = m_lineSegmentManipulators.back();
            lineSegmentManipulator->Register(managerId);
            lineSegmentManipulator->AddEntityComponentIdPair(entityComponentIdPair);
            lineSegmentManipulator->SetSpace(WorldFromLocalWithUniformScale(entityComponentIdPair.GetEntityId()));
            lineSegmentManipulator->SetNonUniformScale(GetNonUniformScale(entityComponentIdPair.GetEntityId()));

            UpdateLineSegmentPosition<Vertex>(vertIndex, entityComponentIdPair.GetEntityId(), *lineSegmentManipulator);

            lineSegmentManipulator->InstallLeftMouseUpCallback(
                [vertIndex, entityComponentIdPair](const LineSegmentSelectionManipulator::Action& action)
                {
                    InsertVertexAfter<Vertex>(entityComponentIdPair, vertIndex, AZ::AdaptVertexIn<Vertex>(action.m_localLineHitPosition));
                });
        };

        // create all line segment manipulators for the polygon prism (used for selection bounds)
        size_t vertexCount = 0;
        AZ::FixedVerticesRequestBus<Vertex>::EventResult(
            vertexCount, entityComponentIdPair.GetEntityId(), &AZ::FixedVerticesRequestBus<Vertex>::Handler::Size);

        if (vertexCount > 1)
        {
            // special case when there are only two vertices
            if (vertexCount == 2)
            {
                m_lineSegmentManipulators.reserve(vertexCount - 1);
                setupLineSegment(entityComponentIdPair, managerId, 0);
            }
            else
            {
                m_lineSegmentManipulators.reserve(vertexCount);
                for (size_t vertIndex = 0; vertIndex < vertexCount; ++vertIndex)
                {
                    setupLineSegment(entityComponentIdPair, managerId, vertIndex);
                }
            }
        }
    }

    template<typename Vertex>
    LineSegmentHoverSelection<Vertex>::~LineSegmentHoverSelection()
    {
        LineSegmentHoverSelection::Unregister();
        m_lineSegmentManipulators.clear();
    }

    template<typename Vertex>
    void LineSegmentHoverSelection<Vertex>::Register(const ManipulatorManagerId managerId)
    {
        for (auto& manipulator : m_lineSegmentManipulators)
        {
            manipulator->Register(managerId);
        }
    }

    template<typename Vertex>
    void LineSegmentHoverSelection<Vertex>::Unregister()
    {
        for (auto& manipulator : m_lineSegmentManipulators)
        {
            manipulator->Unregister();
        }
    }

    template<typename Vertex>
    void LineSegmentHoverSelection<Vertex>::SetBoundsDirty()
    {
        for (auto& manipulator : m_lineSegmentManipulators)
        {
            manipulator->SetBoundsDirty();
        }
    }

    template<typename Vertex>
    void LineSegmentHoverSelection<Vertex>::Refresh()
    {
        size_t vertexCount = 0;
        AZ::FixedVerticesRequestBus<Vertex>::EventResult(vertexCount, m_entityId, &AZ::FixedVerticesRequestBus<Vertex>::Handler::Size);

        // update the start/end positions of all the line segment manipulators to ensure
        // they stay consistent with the polygon prism shape
        if (vertexCount > 1)
        {
            for (size_t manipulatorIndex = 0; manipulatorIndex < m_lineSegmentManipulators.size(); ++manipulatorIndex)
            {
                LineSegmentSelectionManipulator& lineSegmentManipulator = *m_lineSegmentManipulators[manipulatorIndex];
                UpdateLineSegmentPosition<Vertex>(manipulatorIndex, m_entityId, lineSegmentManipulator);
                lineSegmentManipulator.SetBoundsDirty();
            }
        }
    }

    template<typename Vertex>
    void LineSegmentHoverSelection<Vertex>::SetSpace(const AZ::Transform& worldFromLocal)
    {
        for (auto& lineSegmentManipulator : m_lineSegmentManipulators)
        {
            lineSegmentManipulator->SetSpace(worldFromLocal);
        }
    }

    template<typename Vertex>
    void LineSegmentHoverSelection<Vertex>::SetNonUniformScale(const AZ::Vector3& nonUniformScale)
    {
        for (auto& lineSegmentManipulator : m_lineSegmentManipulators)
        {
            lineSegmentManipulator->SetNonUniformScale(nonUniformScale);
        }
    }

    template class LineSegmentHoverSelection<AZ::Vector2>;
    template class LineSegmentHoverSelection<AZ::Vector3>;
} // namespace AzToolsFramework
