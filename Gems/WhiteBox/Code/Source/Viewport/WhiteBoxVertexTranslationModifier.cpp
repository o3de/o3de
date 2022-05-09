/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SubComponentModes/EditorWhiteBoxDefaultModeBus.h"
#include "Util/WhiteBoxMathUtil.h"
#include "Viewport/WhiteBoxModifierUtil.h"
#include "WhiteBoxVertexTranslationModifier.h"

#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Manipulators/MultiLinearManipulator.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <EditorWhiteBoxComponentModeBus.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

AZ_CVAR(
    float, cl_whiteBoxVertexTranslationPressTime, 0.1f, nullptr, AZ::ConsoleFunctorFlags::Null,
    "How long must the modifier be held before we display the axes the vertex can be moved along");
AZ_CVAR(
    float, cl_whiteBoxVertexTranslationAxisLength, 500.0f, nullptr, AZ::ConsoleFunctorFlags::Null,
    "The length of the vertex translation axis to draw while moving the vertex");
AZ_CVAR(
    AZ::Color, cl_whiteBoxVertexTranslationAxisColor, AZ::Color::CreateFromRgba(255, 100, 0, 255), nullptr,
    AZ::ConsoleFunctorFlags::Null, "The color of the vertex translation axes before movement has occurred");
AZ_CVAR(
    AZ::Color, cl_whiteBoxVertexTranslationAxisInactiveColor, AZ::Color::CreateFromRgba(255, 100, 0, 90), nullptr,
    AZ::ConsoleFunctorFlags::Null, "The color of the vertex translation axes after movement has occurred");
AZ_CVAR(
    AZ::Color, cl_whiteBoxVertexSelectedTranslationAxisColor, AZ::Color::CreateFromRgba(0, 150, 255, 255), nullptr,
    AZ::ConsoleFunctorFlags::Null, "The color of the vertex translation axis the vertex is moving along");
AZ_CVAR(
    float, cl_whiteBoxVertexTranslationAxisWidth, 5.0f, nullptr, AZ::ConsoleFunctorFlags::Null,
    "The thickness of the line for the vertex translation axes");

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(VertexTranslationModifier, AZ::SystemAllocator, 0)

    static bool IsAxisValid(const int axisIndex)
    {
        return axisIndex != VertexTranslationModifier::InvalidAxisIndex;
    }

    VertexTranslationModifier::VertexTranslationModifier(
        const AZ::EntityComponentIdPair& entityComponentIdPair, Api::VertexHandle vertexHandle,
        [[maybe_unused]] const AZ::Vector3& intersectionPoint)
        : m_entityComponentIdPair{entityComponentIdPair}
        , m_vertexHandle{vertexHandle}
    {
        CreateManipulator();

        AzFramework::ViewportDebugDisplayEventBus::Handler::BusConnect(AzToolsFramework::GetEntityContextId());
    }

    VertexTranslationModifier::~VertexTranslationModifier()
    {
        AzFramework::ViewportDebugDisplayEventBus::Handler::BusDisconnect();

        DestroyManipulator();
    }

    static int FindClosestAxis(
        const AZ::EntityId entityId, const AzToolsFramework::MultiLinearManipulator::Action& action,
        const AZStd::vector<AZStd::pair<AZ::Vector3, AZ::Vector3>>& edgeBeginEnds)
    {
        const auto cameraState = AzToolsFramework::GetCameraState(action.m_viewportId);
        const auto worldFromLocal = AzToolsFramework::WorldFromLocalWithUniformScale(entityId);

        int axisIndex = VertexTranslationModifier::InvalidAxisIndex;
        float maxLength = 0.0f;
        for (size_t actionIndex = 0; actionIndex < action.m_actions.size(); ++actionIndex)
        {
            const auto& currentAction = action.m_actions[actionIndex];
            const auto& edgeAxis = edgeBeginEnds[actionIndex];

            const auto worldStart = worldFromLocal.TransformPoint(edgeAxis.first);
            const auto worldEnd = worldFromLocal.TransformPoint(edgeAxis.second);
            const auto screenAxis = AzFramework::Vector2FromScreenVector(
                                        AzFramework::WorldToScreen(worldEnd, cameraState) -
                                        AzFramework::WorldToScreen(worldStart, cameraState))
                                        .GetNormalizedSafe();

            const auto screenLength = std::fabs(currentAction.ScreenOffset().Dot(screenAxis));
            if (screenLength > maxLength)
            {
                axisIndex = static_cast<int>(actionIndex);
                maxLength = screenLength;
            }
        }

        return axisIndex;
    }

    void VertexTranslationModifier::CreateManipulator()
    {
        using AzToolsFramework::MultiLinearManipulator;
        using AzToolsFramework::ViewportInteraction::MouseInteraction;

        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        // create the manipulator in the local space of the entity the white box component is on
        m_translationManipulator = MultiLinearManipulator::MakeShared(
            AzToolsFramework::WorldFromLocalWithUniformScale(m_entityComponentIdPair.GetEntityId()));

        m_translationManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
        m_translationManipulator->AddEntityComponentIdPair(m_entityComponentIdPair);
        m_translationManipulator->SetLocalPosition(Api::VertexPosition(*whiteBox, m_vertexHandle));

        // add all axes connecting to vertex
        m_translationManipulator->AddAxes(Api::VertexUserEdgeAxes(*whiteBox, m_vertexHandle));

        struct SharedState
        {
            // the previous position when moving the manipulator, used to calculate manipulator delta position
            AZ::Vector3 m_prevPosition;
            // what state of appending are we currently in
            AppendStage m_appendStage = AppendStage::None;
            // store all begin and end positions for each edge
            AZStd::vector<AZStd::pair<AZ::Vector3, AZ::Vector3>> m_edgeBeginEnds;
            // has the modifier moved during the action
            bool m_moved = false;
        };

        auto sharedState = AZStd::make_shared<SharedState>();

        CreateView();

        // setup callback for translation (linear) manipulator
        m_translationManipulator->InstallLeftMouseDownCallback(
            [this, sharedState]([[maybe_unused]] const MultiLinearManipulator::Action& action)
            {
                WhiteBoxMesh* whiteBox = nullptr;
                EditorWhiteBoxComponentRequestBus::EventResult(
                    whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

                sharedState->m_appendStage = AppendStage::None;
                sharedState->m_moved = false;
                sharedState->m_edgeBeginEnds.clear();
                m_actionIndex = InvalidAxisIndex;

                m_localPositionAtMouseDown = m_translationManipulator->GetLocalPosition();

                for (const auto& edgeHandle : Api::VertexUserEdgeHandles(*whiteBox, m_vertexHandle))
                {
                    const auto edgeVertexPositions = Api::EdgeVertexPositions(*whiteBox, edgeHandle);
                    sharedState->m_edgeBeginEnds.push_back(
                        AZStd::make_pair(edgeVertexPositions[0], edgeVertexPositions[1]));
                }

                this->AZ::TickBus::Handler::BusConnect();
            });

        m_translationManipulator->InstallMouseMoveCallback(
            [this, sharedState](const MultiLinearManipulator::Action& action)
            {
                WhiteBoxMesh* whiteBox = nullptr;
                EditorWhiteBoxComponentRequestBus::EventResult(
                    whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

                m_actionIndex =
                    FindClosestAxis(m_entityComponentIdPair.GetEntityId(), action, sharedState->m_edgeBeginEnds);

                if (m_actionIndex != InvalidAxisIndex)
                {
                    // has the modifier moved during this interaction
                    sharedState->m_moved = sharedState->m_moved ||
                        action.m_actions[m_actionIndex].LocalPositionOffset().GetLength() >=
                            cl_whiteBoxMouseClickDeltaThreshold;

                    // update vertex and position of manipulator
                    Api::SetVertexPosition(*whiteBox, m_vertexHandle, action.m_actions[m_actionIndex].LocalPosition());
                    m_translationManipulator->SetLocalPosition(Api::VertexPosition(*whiteBox, m_vertexHandle));

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
                        &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshEdgeScaleModifier);

                    EditorWhiteBoxComponentNotificationBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
                }

                Api::CalculateNormals(*whiteBox);
                Api::CalculatePlanarUVs(*whiteBox);
            });

        m_translationManipulator->InstallLeftMouseUpCallback(
            [this, sharedState,
             translationManipulator = AZStd::weak_ptr<MultiLinearManipulator>(m_translationManipulator)](
                [[maybe_unused]] const MultiLinearManipulator::Action& action)
            {
                // we haven't moved, count as a click
                if (!sharedState->m_moved)
                {
                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxDefaultModeRequestBus::Events::AssignSelectedVertexSelectionModifier);
                }
                else
                {
                    WhiteBoxMesh* whiteBox = nullptr;
                    EditorWhiteBoxComponentRequestBus::EventResult(
                        whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

                    // refresh and update all manipulator axes after mouse up
                    if (auto manipulator = translationManipulator.lock())
                    {
                        manipulator->ClearAxes();
                        manipulator->AddAxes(Api::VertexUserEdgeAxes(*whiteBox, m_vertexHandle));
                    }

                    EditorWhiteBoxComponentRequestBus::Event(
                        m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);
                }

                m_pressTime = 0.0f;
                m_actionIndex = InvalidAxisIndex;
                this->AZ::TickBus::Handler::BusDisconnect();
            });
    }

    void VertexTranslationModifier::CreateView()
    {
        using AzToolsFramework::ViewportInteraction::MouseInteraction;

        if (!m_vertexView)
        {
            m_vertexView = AzToolsFramework::CreateManipulatorViewSphere(
                m_color, cl_whiteBoxVertexManipulatorSize,
                []([[maybe_unused]] const MouseInteraction& mouseInteraction, [[maybe_unused]] const bool mouseOver,
                   const AZ::Color& defaultColor)
                {
                    return defaultColor;
                });
        }

        m_vertexView->m_color = m_color;

        m_translationManipulator->SetViews(AzToolsFramework::ManipulatorViews{m_vertexView});
    }

    void VertexTranslationModifier::DestroyManipulator()
    {
        m_translationManipulator->Unregister();
        m_translationManipulator.reset();
    }

    bool VertexTranslationModifier::MouseOver() const
    {
        return m_translationManipulator->MouseOver();
    }

    void VertexTranslationModifier::ForwardMouseOverEvent(
        const AzToolsFramework::ViewportInteraction::MouseInteraction& interaction)
    {
        m_translationManipulator->ForwardMouseOverEvent(interaction);
    }

    void VertexTranslationModifier::Refresh()
    {
        DestroyManipulator();
        CreateManipulator();
    }

    bool VertexTranslationModifier::PerformingAction() const
    {
        return m_translationManipulator->PerformingAction();
    }

    void VertexTranslationModifier::DisplayViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (PerformingAction() && m_pressTime >= cl_whiteBoxVertexTranslationPressTime)
        {
            const auto worldFromLocal =
                AzToolsFramework::WorldFromLocalWithUniformScale(m_entityComponentIdPair.GetEntityId());

            debugDisplay.PushMatrix(worldFromLocal);

            debugDisplay.DepthTestOff();
            debugDisplay.SetLineWidth(cl_whiteBoxVertexTranslationAxisWidth);

            AZStd::for_each(
                m_translationManipulator->FixedBegin(), m_translationManipulator->FixedEnd(),
                [this, &debugDisplay, actionIndex = 0](const AzToolsFramework::LinearManipulator::Fixed& fixed) mutable
                {
                    if (!IsAxisValid(m_actionIndex))
                    {
                        debugDisplay.SetColor(cl_whiteBoxVertexTranslationAxisColor);
                    }
                    else
                    {
                        debugDisplay.SetColor(
                            actionIndex == m_actionIndex ? cl_whiteBoxVertexSelectedTranslationAxisColor
                                                         : cl_whiteBoxVertexTranslationAxisInactiveColor);
                    }

                    const float axisLength = cl_whiteBoxVertexTranslationAxisLength;
                    debugDisplay.DrawLine(
                        m_localPositionAtMouseDown - fixed.m_axis * axisLength * 0.5f,
                        m_localPositionAtMouseDown + fixed.m_axis * axisLength * 0.5f);

                    actionIndex++;
                });

            debugDisplay.DepthTestOn();
            debugDisplay.PopMatrix();
        }
    }

    void VertexTranslationModifier::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        m_pressTime += deltaTime;
    }
} // namespace WhiteBox
