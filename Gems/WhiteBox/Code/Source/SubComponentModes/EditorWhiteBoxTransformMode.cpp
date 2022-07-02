/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorWhiteBoxTransformMode.h"
#include "AzCore/std/smart_ptr/make_shared.h"
#include "EditorWhiteBoxComponentModeCommon.h"
#include "EditorWhiteBoxComponentModeTypes.h"
#include "Util/WhiteBoxEditorDrawUtil.h"

#include <AzCore/Math/Color.h>
#include <AzCore/std/base.h>
#include <AzCore/std/optional.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <Manipulators/LinearManipulator.h>
#include <Manipulators/ManipulatorManager.h>
#include <Manipulators/RotationManipulators.h>
#include <Manipulators/ScaleManipulators.h>
#include <Manipulators/TranslationManipulators.h>
#include <Viewport/ViewportSettings.h>
#include <Viewport/WhiteBoxModifierUtil.h>
#include <Viewport/WhiteBoxViewportConstants.h>

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(TransformMode, AZ::SystemAllocator, 0)

    static void SetViewportUiClusterActiveButton(
        AzToolsFramework::ViewportUi::ClusterId clusterId, AzToolsFramework::ViewportUi::ButtonId buttonId)
    {
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton,
            clusterId,
            buttonId);
    }

    static AzToolsFramework::ViewportUi::ButtonId RegisterClusterButton(
        AzToolsFramework::ViewportUi::ClusterId clusterId, const char* iconName)
    {
        AzToolsFramework::ViewportUi::ButtonId buttonId;
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
            buttonId,
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateClusterButton,
            clusterId,
            AZStd::string::format(":/stylesheet/img/UI20/toolbar/%s.svg", iconName));

        return buttonId;
    }

    TransformMode::TransformMode(const AZ::EntityComponentIdPair& entityComponentIdPair)
        : m_entityComponentIdPair(entityComponentIdPair)
    {
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
            m_transformClusterId,
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateCluster,
            AzToolsFramework::ViewportUi::Alignment::TopRight);
        m_transformTranslateButtonId = RegisterClusterButton(m_transformClusterId, "Move");
        m_transformRotateButtonId = RegisterClusterButton(m_transformClusterId, "Rotate");
        m_transformScaleButtonId = RegisterClusterButton(m_transformClusterId, "Scale");

        // set translation tooltips
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip,
            m_transformClusterId,
            m_transformTranslateButtonId,
            ManipulatorModeClusterTranslateTooltip);
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip,
            m_transformClusterId,
            m_transformRotateButtonId,
            ManipulatorModeClusterRotateTooltip);
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip,
            m_transformClusterId,
            m_transformScaleButtonId,
            ManipulatorModeClusterScaleTooltip);

        m_transformSelectionHandler = AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler(
            [this](AzToolsFramework::ViewportUi::ButtonId buttonId)
            {
                if (buttonId == m_transformTranslateButtonId)
                {
                    m_transformType = TransformType::Translation;
                }
                else if (buttonId == m_transformRotateButtonId)
                {
                    m_transformType = TransformType::Rotation;
                }
                else if (buttonId == m_transformScaleButtonId)
                {
                    m_transformType = TransformType::Scale;
                }

                RefreshManipulator();
            });

        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler,
            m_transformClusterId,
            m_transformSelectionHandler);

        RefreshManipulator();
    }

    TransformMode::~TransformMode()
    {
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RemoveCluster,
            m_transformClusterId);

        DestroyManipulators();
    }

    void TransformMode::DestroyManipulators()
    {
        if (m_manipulator)
        {
            m_manipulator->Unregister();
            m_manipulator.reset();
        }
    }

    void TransformMode::Refresh()
    {
        m_whiteBoxSelection.reset();
        DestroyManipulators();
    }

    AZStd::vector<AzToolsFramework::ActionOverride> TransformMode::PopulateActions(
        [[maybe_unused]] const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        return {};
    }

    void TransformMode::Display(
        [[maybe_unused]] const AZ::EntityComponentIdPair& entityComponentIdPair,
        const AZ::Transform& worldFromLocal,
        const IntersectionAndRenderData& renderData,
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        debugDisplay.DepthTestOn();
        debugDisplay.PushMatrix(worldFromLocal);

        if (m_polygonIntersection.has_value())
        {
            auto& polygonIntersection = m_polygonIntersection.value();
            DrawFace(debugDisplay, whiteBox, polygonIntersection.GetHandle(), ed_whiteBoxPolygonHover);
            DrawOutline(debugDisplay, whiteBox, polygonIntersection.GetHandle(), ed_whiteBoxOutlineHover);
        }

        if (m_edgeIntersection.has_value())
        {
            auto& edgeIntersection = m_edgeIntersection.value();
            DrawEdge(debugDisplay, whiteBox, edgeIntersection.GetHandle(), ed_whiteBoxOutlineHover);
        }

        if (m_vertexIntersection.has_value())
        {
            auto& vertexIntersection = m_vertexIntersection.value();
            auto handles = AZStd::array<Api::VertexHandle, 1>({ vertexIntersection.GetHandle() });
            DrawPoints(debugDisplay, whiteBox, viewportInfo, handles, ed_whiteBoxVertexHover);
        }

        if (m_whiteBoxSelection)
        {
            if (auto polygonSelection = AZStd::get_if<PolygonIntersection>(&m_whiteBoxSelection->m_selection))
            {
                auto vertexHandles = Api::PolygonVertexHandles(*whiteBox, polygonSelection->GetHandle());
                DrawPoints(debugDisplay, whiteBox, viewportInfo, vertexHandles, ed_whiteBoxVertexSelection);
                if (m_polygonIntersection.value_or(PolygonIntersection{}).GetHandle() != polygonSelection->GetHandle())
                {
                    DrawFace(debugDisplay, whiteBox, polygonSelection->GetHandle(), ed_whiteBoxPolygonSelection);
                    DrawOutline(debugDisplay, whiteBox, polygonSelection->GetHandle(), ed_whiteBoxOutlineSelection);
                }
            }
            else if (auto edgeSelection = AZStd::get_if<EdgeIntersection>(&m_whiteBoxSelection->m_selection))
            {
                auto vertexHandles = Api::EdgeVertexHandles(*whiteBox, edgeSelection->GetHandle());
                DrawPoints(debugDisplay, whiteBox, viewportInfo, vertexHandles, ed_whiteBoxVertexSelection);
                if (m_edgeIntersection.value_or(EdgeIntersection{}).GetHandle() != edgeSelection->GetHandle())
                {
                    DrawEdge(debugDisplay, whiteBox, edgeSelection->GetHandle(), ed_whiteBoxOutlineSelection);
                }
            }
            else if (auto vertexSelection = AZStd::get_if<VertexIntersection>(&m_whiteBoxSelection->m_selection))
            {
                if (m_vertexIntersection.value_or(VertexIntersection{}).GetHandle() != vertexSelection->GetHandle())
                {
                    auto handles = AZStd::array<Api::VertexHandle, 1>({ vertexSelection->GetHandle() });
                    DrawPoints(debugDisplay, whiteBox, viewportInfo, handles, ed_whiteBoxVertexSelection);
                }
            }
        }
        debugDisplay.PopMatrix();
        debugDisplay.DepthTestOff();
    }

    bool TransformMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction,
        [[maybe_unused]] const AZ::EntityComponentIdPair& entityComponentIdPair,
        const AZStd::optional<EdgeIntersection>& edgeIntersection,
        const AZStd::optional<PolygonIntersection>& polygonIntersection,
        const AZStd::optional<VertexIntersection>& vertexIntersection)
    {
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        const auto closestIntersection = FindClosestGeometryIntersection(edgeIntersection, polygonIntersection, vertexIntersection);
        m_polygonIntersection.reset();
        m_edgeIntersection.reset();

        // update stored edge and vertex intersection
        switch (closestIntersection)
        {
        case GeometryIntersection::Polygon:
            m_polygonIntersection = polygonIntersection;
            break;
        case GeometryIntersection::Edge:
            m_edgeIntersection = edgeIntersection;
            break;
        case GeometryIntersection::Vertex:
            m_vertexIntersection = vertexIntersection;
            break;
        default:
            // do nothing
            break;
        }

        if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
            mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Down)
        {
            switch (closestIntersection)
            {
            case GeometryIntersection::Polygon:
                if (polygonIntersection.has_value())
                {
                    m_whiteBoxSelection = AZStd::make_shared<TransformMode::VertexTransformSelection>();
                    m_whiteBoxSelection->m_selection = polygonIntersection.value();
                    RefreshManipulator();
                }
                break;
            case GeometryIntersection::Edge:
                if (edgeIntersection.has_value())
                {
                    m_whiteBoxSelection = AZStd::make_shared<TransformMode::VertexTransformSelection>();
                    m_whiteBoxSelection->m_selection = edgeIntersection.value();
                    RefreshManipulator();
                }
                break;
            case GeometryIntersection::Vertex:
                if (vertexIntersection.has_value())
                {
                    m_whiteBoxSelection = AZStd::make_shared<TransformMode::VertexTransformSelection>();
                    m_whiteBoxSelection->m_selection = vertexIntersection.value();
                    RefreshManipulator();
                }
                break;
            default:
                // do nothing
                break;
            }
        }

        return false;
    }

    void TransformMode::RefreshManipulator()
    {
        DestroyManipulators();
        if (m_whiteBoxSelection)
        {
            switch (m_transformType)
            {
            case TransformType::Translation:
                CreateTranslationManipulators();
                SetViewportUiClusterActiveButton(m_transformClusterId, m_transformTranslateButtonId);
                break;
            case TransformType::Rotation:
                CreateRotationManipulators();
                SetViewportUiClusterActiveButton(m_transformClusterId, m_transformRotateButtonId);
                break;
            case TransformType::Scale:
                CreateScaleManipulators();
                SetViewportUiClusterActiveButton(m_transformClusterId, m_transformScaleButtonId);
                break;
            default:
                break;
            }
        }
    }

    void TransformMode::UpdateTransformHandles(WhiteBoxMesh* mesh)
    {
        if (auto polygonSelection = AZStd::get_if<PolygonIntersection>(&m_whiteBoxSelection->m_selection))
        {
            m_whiteBoxSelection->m_vertexHandles = Api::PolygonVertexHandles(*mesh, polygonSelection->GetHandle());
            m_whiteBoxSelection->m_vertexPositions = Api::VertexPositions(*mesh, m_whiteBoxSelection->m_vertexHandles);
            m_whiteBoxSelection->m_localPosition = Api::PolygonMidpoint(*mesh, polygonSelection->GetHandle());
        }
        else if (auto edgeSelection = AZStd::get_if<EdgeIntersection>(&m_whiteBoxSelection->m_selection))
        {
            auto edgeHandle = Api::EdgeVertexHandles(*mesh, edgeSelection->GetHandle());
            m_whiteBoxSelection->m_vertexHandles = Api::VertexHandles(edgeHandle.cbegin(), edgeHandle.cend());
            m_whiteBoxSelection->m_vertexPositions = Api::VertexPositions(*mesh, m_whiteBoxSelection->m_vertexHandles);
            m_whiteBoxSelection->m_localPosition = Api::EdgeMidpoint(*mesh, edgeSelection->GetHandle());
        }
        else if (auto vertexSelection = AZStd::get_if<VertexIntersection>(&m_whiteBoxSelection->m_selection))
        {
            m_whiteBoxSelection->m_vertexHandles = Api::VertexHandles({ vertexSelection->GetHandle() });
            m_whiteBoxSelection->m_vertexPositions = Api::VertexPositions(*mesh, m_whiteBoxSelection->m_vertexHandles);
            m_whiteBoxSelection->m_localPosition = Api::VertexPosition(*mesh, vertexSelection->GetHandle());
        }
    }

    void TransformMode::CreateTranslationManipulators()
    {
        AZ::Transform worldTranform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldTranform, m_entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        AZStd::shared_ptr<AzToolsFramework::TranslationManipulators> translationManipulators =
            AZStd::make_shared<AzToolsFramework::TranslationManipulators>(
                AzToolsFramework::TranslationManipulators::Dimensions::Three, worldTranform, AZ::Vector3::CreateOne());

        translationManipulators->AddEntityComponentIdPair(m_entityComponentIdPair);
        AzToolsFramework::ConfigureTranslationManipulatorAppearance3d(translationManipulators.get());

        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        UpdateTransformHandles(whiteBox);
        translationManipulators->SetLocalPosition(m_whiteBoxSelection->m_localPosition);

        auto mouseMoveHandlerFn = [entityComponentIdPair = m_entityComponentIdPair,
                                   transformSelection = m_whiteBoxSelection,
                                   currentManipulator = AZStd::weak_ptr<AzToolsFramework::TranslationManipulators>(translationManipulators)](const auto& action)
        {
            WhiteBoxMesh* whiteBox = nullptr;
            EditorWhiteBoxComponentRequestBus::EventResult(
                whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

            size_t vertexIndex = 0;
            for (const Api::VertexHandle& vertexHandle : transformSelection->m_vertexHandles)
            {
                const AZ::Vector3 vertexPosition = transformSelection->m_vertexPositions[vertexIndex++] + action.LocalPositionOffset();
                Api::SetVertexPosition(*whiteBox, vertexHandle, vertexPosition);
            }
            if (auto manipulator = currentManipulator.lock())
            {
                manipulator->SetLocalPosition(transformSelection->m_localPosition + action.LocalPositionOffset());
            }

            Api::CalculateNormals(*whiteBox);
            Api::CalculatePlanarUVs(*whiteBox);

            EditorWhiteBoxComponentNotificationBus::Event(
                entityComponentIdPair, &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
        };

        auto mouseUpHandlerFn = [mouseMoveHandlerFn, entityComponentIdPair = m_entityComponentIdPair,
                                 transformSelection = m_whiteBoxSelection,
                                 currentManipulator = AZStd::weak_ptr<AzToolsFramework::TranslationManipulators>(
                                     translationManipulators)](const auto& action)
        {
            WhiteBoxMesh* whiteBox = nullptr;
            EditorWhiteBoxComponentRequestBus::EventResult(
                whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

            mouseMoveHandlerFn(action);

            transformSelection->m_vertexPositions = Api::VertexPositions(*whiteBox, transformSelection->m_vertexHandles);
            transformSelection->m_localPosition = transformSelection->m_localPosition + action.LocalPositionOffset();
            if (auto manipulator = currentManipulator.lock())
            {
                manipulator->SetLocalPosition(transformSelection->m_localPosition);
            }
            EditorWhiteBoxComponentRequestBus::Event(entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);
        };

        translationManipulators->InstallLinearManipulatorMouseMoveCallback(mouseMoveHandlerFn);
        translationManipulators->InstallPlanarManipulatorMouseMoveCallback(mouseMoveHandlerFn);
        translationManipulators->InstallSurfaceManipulatorMouseMoveCallback(mouseMoveHandlerFn);

        translationManipulators->InstallSurfaceManipulatorMouseUpCallback(mouseUpHandlerFn);
        translationManipulators->InstallPlanarManipulatorMouseUpCallback(mouseUpHandlerFn);
        translationManipulators->InstallLinearManipulatorMouseUpCallback(mouseUpHandlerFn);

        translationManipulators->Register(AzToolsFramework::g_mainManipulatorManagerId);
        m_manipulator = AZStd::move(translationManipulators);
    }

    void TransformMode::CreateRotationManipulators()
    {
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        AZ::Transform worldTranform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldTranform, m_entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        AZStd::shared_ptr<AzToolsFramework::RotationManipulators> rotationManipulators =
            AZStd::make_shared<AzToolsFramework::RotationManipulators>(worldTranform);
        rotationManipulators->SetCircleBoundWidth(AzToolsFramework::ManipulatorCicleBoundWidth());
        rotationManipulators->AddEntityComponentIdPair(m_entityComponentIdPair);

        UpdateTransformHandles(whiteBox);
        rotationManipulators->SetLocalPosition(m_whiteBoxSelection->m_localPosition);
        rotationManipulators->SetLocalOrientation(AZ::Quaternion::CreateIdentity());

        rotationManipulators->SetLocalAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
        rotationManipulators->ConfigureView(
            AzToolsFramework::RotationManipulatorRadius(),
            AzFramework::ViewportColors::XAxisColor,
            AzFramework::ViewportColors::YAxisColor,
            AzFramework::ViewportColors::ZAxisColor);

        auto mouseMoveHandlerFn = [entityComponentIdPair = m_entityComponentIdPair,
             transformSelection = m_whiteBoxSelection,
             currentManipulator = AZStd::weak_ptr<AzToolsFramework::RotationManipulators>(rotationManipulators)](
                const AzToolsFramework::AngularManipulator::Action& action)
            {
                WhiteBoxMesh* whiteBox = nullptr;
                EditorWhiteBoxComponentRequestBus::EventResult(
                    whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
                size_t vertexIndex = 0;
                for (const Api::VertexHandle& vertexHandle : transformSelection->m_vertexHandles)
                {
                    const AZ::Vector3 vertexPosition =
                        (action.LocalOrientation())
                            .TransformVector(transformSelection->m_vertexPositions[vertexIndex++] - transformSelection->m_localPosition) +
                        transformSelection->m_localPosition;
                    Api::SetVertexPosition(*whiteBox, vertexHandle, vertexPosition);
                }

                if (auto manipulator = currentManipulator.lock())
                {
                    manipulator->SetLocalOrientation(action.LocalOrientation());
                }

                Api::CalculateNormals(*whiteBox);
                Api::CalculatePlanarUVs(*whiteBox);
                EditorWhiteBoxComponentNotificationBus::Event(
                    entityComponentIdPair, &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
            };

        rotationManipulators->InstallMouseMoveCallback(mouseMoveHandlerFn);
        rotationManipulators->InstallLeftMouseUpCallback(
            [mouseMoveHandlerFn, entityComponentIdPair = m_entityComponentIdPair,
             transformSelection = m_whiteBoxSelection,
             currentManipulator = AZStd::weak_ptr<AzToolsFramework::RotationManipulators>(rotationManipulators)](
                const AzToolsFramework::AngularManipulator::Action& action)
            {
                WhiteBoxMesh* whiteBox = nullptr;
                EditorWhiteBoxComponentRequestBus::EventResult(
                    whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
                mouseMoveHandlerFn(action);

                transformSelection->m_vertexPositions = Api::VertexPositions(*whiteBox, transformSelection->m_vertexHandles);
                if (auto manipulator = currentManipulator.lock())
                {
                    manipulator->SetLocalOrientation(AZ::Quaternion::CreateIdentity());
                }

                EditorWhiteBoxComponentRequestBus::Event(entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);
            });

        rotationManipulators->Register(AzToolsFramework::g_mainManipulatorManagerId);
        m_manipulator = AZStd::move(rotationManipulators);
    }

    void TransformMode::CreateScaleManipulators()
    {
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        AZ::Transform worldTranform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldTranform, m_entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        AZStd::shared_ptr<AzToolsFramework::ScaleManipulators> scaleManipulators =
            AZStd::make_shared<AzToolsFramework::ScaleManipulators>(worldTranform);
        scaleManipulators->SetLineBoundWidth(AzToolsFramework::ManipulatorLineBoundWidth());
        scaleManipulators->AddEntityComponentIdPair(m_entityComponentIdPair);
        scaleManipulators->SetAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
        scaleManipulators->ConfigureView(
            AzToolsFramework::LinearManipulatorAxisLength(),
            AzFramework::ViewportColors::XAxisColor,
            AzFramework::ViewportColors::YAxisColor,
            AzFramework::ViewportColors::ZAxisColor);

        UpdateTransformHandles(whiteBox);
        scaleManipulators->SetLocalPosition(m_whiteBoxSelection->m_localPosition);

        auto mouseMoveHandlerFn =
            [entityComponentIdPair = m_entityComponentIdPair,
             transformSelection = m_whiteBoxSelection](const auto& action)
        {
            WhiteBoxMesh* whiteBox = nullptr;
            EditorWhiteBoxComponentRequestBus::EventResult(
                whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
            size_t vertexIndex = 0;
            for (const Api::VertexHandle& vertexHandle : transformSelection->m_vertexHandles)
            {
                const AZ::Vector3 vertexLocalPosition =
                    (transformSelection->m_vertexPositions[vertexIndex++] - transformSelection->m_localPosition);
                const AZ::Vector3 vertexPosition =
                    (vertexLocalPosition * (AZ::Vector3::CreateOne() + (action.m_start.m_sign * action.LocalScaleOffset()))) +
                    transformSelection->m_localPosition;
                Api::SetVertexPosition(*whiteBox, vertexHandle, vertexPosition);
            }

            Api::CalculateNormals(*whiteBox);
            Api::CalculatePlanarUVs(*whiteBox);
            EditorWhiteBoxComponentNotificationBus::Event(
                entityComponentIdPair, &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
        };

        auto mouseUpHandlerFn =
            [mouseMoveHandlerFn, entityComponentIdPair = m_entityComponentIdPair,
             transformSelection = m_whiteBoxSelection](const auto& action)
        {
            WhiteBoxMesh* whiteBox = nullptr;
            EditorWhiteBoxComponentRequestBus::EventResult(
                whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

            mouseMoveHandlerFn(action);
            transformSelection->m_vertexPositions = Api::VertexPositions(*whiteBox, transformSelection->m_vertexHandles);

            EditorWhiteBoxComponentRequestBus::Event(entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);
        };

        scaleManipulators->InstallAxisMouseMoveCallback(mouseMoveHandlerFn);
        scaleManipulators->InstallUniformMouseMoveCallback(mouseMoveHandlerFn);

        scaleManipulators->InstallAxisLeftMouseUpCallback(mouseUpHandlerFn);
        scaleManipulators->InstallUniformLeftMouseUpCallback(mouseUpHandlerFn);

        scaleManipulators->Register(AzToolsFramework::g_mainManipulatorManagerId);
        m_manipulator = AZStd::move(scaleManipulators);
    }

} // namespace WhiteBox
