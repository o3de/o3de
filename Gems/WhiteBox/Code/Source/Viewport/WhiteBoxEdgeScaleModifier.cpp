/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SubComponentModes/EditorWhiteBoxDefaultModeBus.h"
#include "Util/WhiteBoxMathUtil.h"
#include "Viewport/WhiteBoxViewportConstants.h"
#include "WhiteBoxEdgeScaleModifier.h"

#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <EditorWhiteBoxComponentModeBus.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(EdgeScaleModifier, AZ::SystemAllocator)

    EdgeScaleModifier::EdgeScaleModifier(
        const Api::EdgeHandle& edgeHandle, const AZ::EntityComponentIdPair& entityComponentIdPair)
        : m_entityComponentIdPair(entityComponentIdPair)
        , m_edgeHandle(edgeHandle)
    {
        CreateManipulators();
    }

    EdgeScaleModifier::~EdgeScaleModifier()
    {
        DestroyManipulators();
    }

    void EdgeScaleModifier::Refresh()
    {
        DestroyManipulators();
        CreateManipulators();
    }

    EdgeScaleModifier::ScaleMode EdgeScaleModifier::GetScaleModeFromModifierKey(
        const AzToolsFramework::ViewportInteraction::KeyboardModifiers& modifiers)
    {
        // default mode is uniform scale, holding alt changes to non-uniform scale
        return modifiers.Alt() ? ScaleMode::NonUniform : ScaleMode::Uniform;
    }

    void EdgeScaleModifier::InitializeScaleModifier(
        const WhiteBoxMesh* whiteBox, const AzToolsFramework::LinearManipulator::Action& action)
    {
        m_initialVertexPositions = Api::EdgeVertexPositions(*whiteBox, m_edgeHandle);
        m_scaleMode = GetScaleModeFromModifierKey(action.m_modifiers);

        // pick the edge midpoint (uniform scaling) or the opposite vertex (non-uniform scaling) for the pivot point
        const size_t oppositeVertex = m_selectedHandleIndex == 0 ? 1 : 0;
        m_pivotPoint = m_scaleMode == ScaleMode::Uniform ? Api::EdgeMidpoint(*whiteBox, m_edgeHandle)
                                                         : m_initialVertexPositions[oppositeVertex];

        m_startingDistance = (m_pivotPoint - m_initialVertexPositions[m_selectedHandleIndex]).GetLength();
    }

    void EdgeScaleModifier::CreateManipulators()
    {
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        // note: important vertex positions of an edge do not overlap
        const auto vertexPositions = Api::EdgeVertexPositions(*whiteBox, m_edgeHandle);

        const AZ::Vector3 axis_lr = (vertexPositions[1] - vertexPositions[0]).GetNormalized();
        const AZ::Vector3 axis_rl = -axis_lr;

        AZStd::array<AZ::Vector3, 2> axes = {{axis_rl, axis_lr}};
        for (size_t vertexIndex = 0; vertexIndex < vertexPositions.size(); ++vertexIndex)
        {
            auto manipulator = AzToolsFramework::LinearManipulator::MakeShared(
                AzToolsFramework::WorldFromLocalWithUniformScale(m_entityComponentIdPair.GetEntityId()));

            // configure manipulator
            manipulator->AddEntityComponentIdPair(m_entityComponentIdPair);
            manipulator->SetLocalPosition(vertexPositions[vertexIndex]);
            manipulator->SetLocalOrientation(CalculateLocalOrientation(axes[vertexIndex]));
            manipulator->SetAxis(AZ::Vector3::CreateAxisX());

            // configure views
            AzToolsFramework::ManipulatorViews views;
            auto sphereColor = [](const AzToolsFramework::ViewportInteraction::MouseInteraction&, const bool mouseOver,
                                  const AZ::Color& defaultColor)
            {
                return mouseOver ? ed_whiteBoxVertexHover : defaultColor;
            };

            auto sphereView = AzToolsFramework::CreateManipulatorViewSphere(
                ed_whiteBoxVertexUnselected, cl_whiteBoxVertexManipulatorSize, sphereColor, true);
            views.emplace_back(AZStd::move(sphereView));
            manipulator->SetViews(AZStd::move(views));
            manipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);

            manipulator->InstallLeftMouseDownCallback(
                [this, vertexIndex](const AzToolsFramework::LinearManipulator::Action& action)
                {
                    WhiteBoxMesh* whiteBox = nullptr;
                    EditorWhiteBoxComponentRequestBus::EventResult(
                        whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
                    m_selectedHandleIndex = static_cast<AZ::u32>(vertexIndex);
                    InitializeScaleModifier(whiteBox, action);
                });

            manipulator->InstallMouseMoveCallback(
                [this](const AzToolsFramework::LinearManipulator::Action& action)
                {
                    WhiteBoxMesh* whiteBox = nullptr;
                    EditorWhiteBoxComponentRequestBus::EventResult(
                        whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

                    // switch scale mode mid-action if modifier key is pressed/released
                    if (m_scaleMode != GetScaleModeFromModifierKey(action.m_modifiers))
                    {
                        InitializeScaleModifier(whiteBox, action);
                    }

                    const AZ::Vector3 vectorToPivotPoint = action.LocalPosition() - m_pivotPoint;
                    const float scale = vectorToPivotPoint.Dot(action.m_start.m_localAxis);
                    // ensure we do not allow any scaling when we're at the pivot epsilon
                    const float normalizedScale = (scale == 0.0f || m_startingDistance == 0.0f)
                        ? cl_whiteBoxModifierMidpointEpsilon
                        : AZ::GetMax(
                              scale / m_startingDistance, static_cast<float>(cl_whiteBoxModifierMidpointEpsilon));

                    const auto vertexHandles = Api::EdgeVertexHandles(*whiteBox, m_edgeHandle);
                    const AZ::Transform polygonSpace = Api::EdgeSpace(*whiteBox, m_edgeHandle, m_pivotPoint);
                    for (size_t vertexIndex = 0; vertexIndex < vertexHandles.size(); ++vertexIndex)
                    {
                        // for non-uniform scaling we only apply the transformation to the selected vertex
                        if (m_scaleMode == ScaleMode::NonUniform && vertexIndex != m_selectedHandleIndex)
                        {
                            continue;
                        }

                        Api::SetVertexPosition(
                            *whiteBox, vertexHandles[vertexIndex],
                            ScalePosition(normalizedScale, m_initialVertexPositions[vertexIndex], polygonSpace));
                    }

                    Api::CalculateNormals(*whiteBox);
                    Api::CalculatePlanarUVs(*whiteBox);

                    // update all manipulator positions
                    for (size_t manipulatorIndex = 0; manipulatorIndex < m_scaleManipulators.size(); ++manipulatorIndex)
                    {
                        m_scaleManipulators[manipulatorIndex]->SetLocalPosition(
                            Api::VertexPosition(*whiteBox, vertexHandles[manipulatorIndex]));
                    }

                    EditorWhiteBoxComponentModeRequestBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxComponentModeRequestBus::Events::MarkWhiteBoxIntersectionDataDirty);

                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshPolygonTranslationModifier);

                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshPolygonScaleModifier);

                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshEdgeTranslationModifier);

                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshVertexSelectionModifier);

                    EditorWhiteBoxComponentNotificationBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
                });

            manipulator->InstallLeftMouseUpCallback(
                [this](const AzToolsFramework::LinearManipulator::Action&)
                {
                    EditorWhiteBoxComponentRequestBus::Event(
                        m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);
                });

            m_scaleManipulators[vertexIndex] = AZStd::move(manipulator);
        }
    }

    void EdgeScaleModifier::DestroyManipulators()
    {
        for (auto& manipulator : m_scaleManipulators)
        {
            manipulator->Unregister();
            manipulator.reset();
        }
    }
} // namespace WhiteBox
