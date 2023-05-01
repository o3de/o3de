/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorWhiteBoxPolygonModifierBus.h"
#include "SubComponentModes/EditorWhiteBoxDefaultModeBus.h"
#include "Util/WhiteBoxMathUtil.h"
#include "Viewport/WhiteBoxViewportConstants.h"
#include "WhiteBoxPolygonScaleModifier.h"

#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <EditorWhiteBoxComponentModeBus.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(PolygonScaleModifier, AZ::SystemAllocator)

    PolygonScaleModifier::PolygonScaleModifier(
        const Api::PolygonHandle& polygonHandle, const AZ::EntityComponentIdPair& entityComponentIdPair)
        : m_entityComponentIdPair(entityComponentIdPair)
        , m_polygonHandle(polygonHandle)
    {
        CreateManipulators();
    }

    PolygonScaleModifier::~PolygonScaleModifier()
    {
        DestroyManipulators();
    }

    void PolygonScaleModifier::Refresh()
    {
        DestroyManipulators();
        CreateManipulators();
    }

    void PolygonScaleModifier::DestroyManipulators()
    {
        for (auto& manipulator : m_scaleManipulators)
        {
            manipulator->Unregister();
        }

        m_scaleManipulators.clear();
    }

    void PolygonScaleModifier::CreateManipulators()
    {
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        const auto borderVertexHandlesCollection = Api::PolygonBorderVertexHandles(*whiteBox, m_polygonHandle);
        const auto midpoint = Api::PolygonMidpoint(*whiteBox, m_polygonHandle);

        for (const auto& borderVertexHandles : borderVertexHandlesCollection)
        {
            for (const auto& vertexHandle : borderVertexHandles)
            {
                auto manipulator = AzToolsFramework::LinearManipulator::MakeShared(
                    AzToolsFramework::WorldFromLocalWithUniformScale(m_entityComponentIdPair.GetEntityId()));

                const AZ::Vector3 vertexPosition = Api::VertexPosition(*whiteBox, vertexHandle);
                const AZ::Vector3 axis = (vertexPosition - midpoint).GetNormalized();

                manipulator->AddEntityComponentIdPair(m_entityComponentIdPair);
                manipulator->SetLocalPosition(vertexPosition);
                manipulator->SetLocalOrientation(CalculateLocalOrientation(axis));
                manipulator->SetAxis(AZ::Vector3::CreateAxisX());

                AzToolsFramework::ManipulatorViews views;
                auto sphereColor = [](const AzToolsFramework::ViewportInteraction::MouseInteraction&,
                                      const bool mouseOver, const AZ::Color& defaultColor)
                {
                    return mouseOver ? ed_whiteBoxVertexHover : defaultColor;
                };

                auto sphereView = AzToolsFramework::CreateManipulatorViewSphere(
                    ed_whiteBoxVertexUnselected, cl_whiteBoxVertexManipulatorSize, sphereColor, true);

                views.emplace_back(AZStd::move(sphereView));
                manipulator->SetViews(AZStd::move(views));
                manipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);

                manipulator->InstallLeftMouseDownCallback(
                    [this, vertexHandle]([[maybe_unused]] const AzToolsFramework::LinearManipulator::Action& action)
                    {
                        WhiteBoxMesh* whiteBox = nullptr;
                        EditorWhiteBoxComponentRequestBus::EventResult(
                            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

                        const AZ::Vector3 position = Api::VertexPosition(*whiteBox, vertexHandle);
                        m_midPoint = Api::PolygonMidpoint(*whiteBox, m_polygonHandle);
                        m_startingDistance = (m_midPoint - position).GetLength();
                        m_initialVertexPositions = Api::PolygonVertexPositions(*whiteBox, m_polygonHandle);

                        m_appendStage = AppendStage::None;
                    });

                manipulator->InstallMouseMoveCallback(
                    [this](const AzToolsFramework::LinearManipulator::Action& action)
                    {
                        OnMouseMove(action);
                    });

                manipulator->InstallLeftMouseUpCallback(
                    [this](const AzToolsFramework::LinearManipulator::Action&)
                    {
                        EditorWhiteBoxComponentRequestBus::Event(
                            m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);
                    });

                m_scaleManipulators.push_back(manipulator);
            }
        }
    }

    void PolygonScaleModifier::OnMouseMove(const AzToolsFramework::LinearManipulator::Action& action)
    {
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        // reset append
        if (!action.m_modifiers.Ctrl() && m_appendStage != AppendStage::None)
        {
            m_appendStage = AppendStage::None;
        }

        // append the corners
        // start trying to extrude
        if (action.m_modifiers.Ctrl() && m_appendStage == AppendStage::None)
        {
            m_offsetWhenExtruded = action.LocalPositionOffset().GetLength();
            m_appendStage = AppendStage::Initiated;
        }

        const float currentOffset = action.LocalPositionOffset().GetLength();
        const float extrusion = fabsf(currentOffset - m_offsetWhenExtruded);
        // only extrude after having moved a small amount (to prevent overlapping verts
        // and normals being calculated incorrectly)
        if (extrusion > 0.0f && m_appendStage == AppendStage::Initiated)
        {
            const auto polygonHandle = Api::ScalePolygonAppendRelative(*whiteBox, m_polygonHandle, 0.0f);

            EditorWhiteBoxPolygonModifierNotificationBus::Broadcast(
                &EditorWhiteBoxPolygonModifierNotificationBus::Events::OnPolygonModifierUpdatedPolygonHandle,
                m_polygonHandle, polygonHandle);

            m_polygonHandle = polygonHandle;

            m_appendStage = AppendStage::Complete;
        }

        if (m_appendStage == AppendStage::None || m_appendStage == AppendStage::Complete)
        {
            // the closest the manipulators are allowed to get to the midpoint (so they do not overlap)
            const AZ::Vector3 vectorToMidpoint = action.LocalPosition() - m_midPoint;

            const float uniformScale = vectorToMidpoint.Dot(action.m_start.m_localAxis);
            // ensure we do not allow any scaling when we're at the midpoint epsilon
            const float normalizedUniformScale =
                AZ::GetMax(uniformScale / m_startingDistance, static_cast<float>(cl_whiteBoxModifierMidpointEpsilon));

            {
                // have to set the position of all vertices, not just those bound to manipulators
                const auto vertexHandles = Api::PolygonVertexHandles(*whiteBox, m_polygonHandle);
                const AZ::Transform polygonSpace = Api::PolygonSpace(*whiteBox, m_polygonHandle, m_midPoint);
                for (size_t vertexIndex = 0; vertexIndex < vertexHandles.size(); ++vertexIndex)
                {
                    Api::SetVertexPosition(
                        *whiteBox, vertexHandles[vertexIndex],
                        ScalePosition(normalizedUniformScale, m_initialVertexPositions[vertexIndex], polygonSpace));
                }
            }

            Api::CalculateNormals(*whiteBox);
            Api::CalculatePlanarUVs(*whiteBox);

            {
                // border vertex handles match those used for manipulators
                const auto borderVertexHandles = Api::PolygonBorderVertexHandlesFlattened(*whiteBox, m_polygonHandle);
                // update all manipulator positions
                for (size_t manipulatorIndex = 0; manipulatorIndex < m_scaleManipulators.size(); ++manipulatorIndex)
                {
                    m_scaleManipulators[manipulatorIndex]->SetLocalPosition(
                        Api::VertexPosition(*whiteBox, borderVertexHandles[manipulatorIndex]));
                }
            }

            EditorWhiteBoxComponentModeRequestBus::Event(
                m_entityComponentIdPair,
                &EditorWhiteBoxComponentModeRequestBus::Events::MarkWhiteBoxIntersectionDataDirty);

            EditorWhiteBoxDefaultModeRequestBus::Event(
                m_entityComponentIdPair,
                &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshPolygonTranslationModifier);

            EditorWhiteBoxDefaultModeRequestBus::Event(
                m_entityComponentIdPair, &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshEdgeTranslationModifier);

            EditorWhiteBoxDefaultModeRequestBus::Event(
                m_entityComponentIdPair, &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshEdgeScaleModifier);

            EditorWhiteBoxDefaultModeRequestBus::Event(
                m_entityComponentIdPair, &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshVertexSelectionModifier);

            EditorWhiteBoxComponentNotificationBus::Event(
                m_entityComponentIdPair, &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
        }
    }

    void PolygonScaleModifier::SetPolygonHandle(const Api::PolygonHandle& polygonHandle)
    {
        m_polygonHandle = polygonHandle;
    }
} // namespace WhiteBox
