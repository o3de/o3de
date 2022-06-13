/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorWhiteBoxTransformMode.h"
#include "EditorWhiteBoxComponentModeCommon.h"

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/utility/move.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <Manipulators/LinearManipulator.h>
#include <Manipulators/ManipulatorManager.h>
#include <Manipulators/PlanarManipulator.h>
#include <Manipulators/RotationManipulators.h>
#include <Manipulators/ScaleManipulators.h>
#include <Manipulators/TranslationManipulators.h>
#include <Viewport/ViewportSettings.h>
#include <Viewport/WhiteBoxModifierUtil.h>
#include <Viewport/WhiteBoxViewportConstants.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(TransformMode, AZ::SystemAllocator, 0)

    TransformMode::TransformMode(const AZ::EntityComponentIdPair& entityComponentIdPair)
        : m_entityComponentIdPair(entityComponentIdPair)
    {
    }

    TransformMode::~TransformMode()
    {
        DestroyManipulators();
    }


    void TransformMode::DestroyManipulators() {
        if (m_manipulator)
        {
            m_manipulator->Unregister();
            m_manipulator.reset();
        }
    }

    void TransformMode::Refresh()
    {
        m_selection = AZStd::monostate{};
        DestroyManipulators();
    }

    AZStd::vector<AzToolsFramework::ActionOverride> TransformMode::PopulateActions(
        [[maybe_unused]] const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        return {};
    }

    void TransformMode::DrawFace(
        AzFramework::DebugDisplayRequests& debugDisplay, WhiteBoxMesh* mesh, const Api::PolygonHandle& polygon, const AZ::Color& color)
    {
        AZStd::vector<AZ::Vector3> triangles = Api::PolygonFacesPositions(*mesh, polygon);
        const AZ::Vector3 normal = Api::PolygonNormal(*mesh, polygon);

        if (!triangles.empty())
        {
            // draw selected polygon
            debugDisplay.PushMatrix(AZ::Transform::CreateTranslation(normal * ed_whiteBoxPolygonViewOverlapOffset));

            debugDisplay.SetColor(color);
            debugDisplay.DrawTriangles(triangles, color);

            debugDisplay.PopMatrix();
        }
    }

    void TransformMode::DrawOutline(
        AzFramework::DebugDisplayRequests& debugDisplay, WhiteBoxMesh* mesh, const Api::PolygonHandle& polygon, const AZ::Color& color)
    {
        Api::VertexPositionsCollection outlines = Api::PolygonBorderVertexPositions(*mesh, polygon);
        if (!outlines.empty())
        {
            // draw outline
            debugDisplay.DepthTestOn();
            debugDisplay.SetColor(color);
            debugDisplay.SetLineWidth(cl_whiteBoxEdgeVisualWidth);
            for (const auto& outline : outlines)
            {
                debugDisplay.DrawPolyLine(outline.data(), aznumeric_caster(outline.size()));
            }
            debugDisplay.DepthTestOff();
        }
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

        debugDisplay.PushMatrix(worldFromLocal);

        if(m_polygonIntersection.has_value()) {
            auto& polygonIntersection = m_polygonIntersection.value();
            DrawFace(debugDisplay, whiteBox, polygonIntersection.GetHandle(), cl_whiteBoxPolygonHoveredColor);
            DrawOutline(debugDisplay, whiteBox, polygonIntersection.GetHandle(), cl_whiteBoxPolygonHoveredOutlineColor);
        }

        if (auto selection = AZStd::get_if<PolygonIntersection>(&m_selection))
        {
            if(m_polygonIntersection.value_or(PolygonIntersection{}).GetHandle() != selection->GetHandle()) {
                DrawFace(debugDisplay, whiteBox, selection->GetHandle(), cl_whiteBoxVertexSelectedModifierColor);
                DrawOutline(debugDisplay, whiteBox, selection->GetHandle(), cl_whiteBoxVertexSelectedModifierColor);
            }
        }
        debugDisplay.PopMatrix();
    }

    void TransformMode::SetTransformMode(WhiteBox::TransformType type)
    {
        m_transformType = type;
        RefreshManipulator();
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
        // update stored edge and vertex intersection
        switch (closestIntersection)
        {
        case GeometryIntersection::Polygon:
            m_polygonIntersection = polygonIntersection;
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
                    m_selection = polygonIntersection.value();
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
        if(!AZStd::holds_alternative<AZStd::monostate>(m_selection)) {
            switch (m_transformType)
            {
            case WhiteBox::TransformType::Translation:
                CreateTranslationManipulators();
                break;
            case WhiteBox::TransformType::Rotation:
                CreateRotationManipulators();
                break;
            case WhiteBox::TransformType::Scale:
                CreateScaleManipulators();
                break;
            default:
                break;
            }
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

        struct SharedState
        {
            AZ::Vector3 m_polygonMidpoint = AZ::Vector3::CreateZero();

            AZStd::vector<AZ::Vector3> m_vertexPositions;

            Api::VertexHandles m_vertexHandles;
        };

        auto sharedState = AZStd::make_shared<SharedState>();
        if (auto selection = AZStd::get_if<PolygonIntersection>(&m_selection))
        {
            WhiteBoxMesh* whiteBox = nullptr;
            EditorWhiteBoxComponentRequestBus::EventResult(
                whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

            sharedState->m_vertexHandles = Api::PolygonVertexHandles(*whiteBox, selection->GetHandle());
            sharedState->m_vertexPositions = Api::VertexPositions(*whiteBox, sharedState->m_vertexHandles);
            sharedState->m_polygonMidpoint = Api::PolygonMidpoint(*whiteBox, selection->GetHandle());
            translationManipulators->SetLocalPosition(sharedState->m_polygonMidpoint);
        }

        auto mouseMoveHandlerFn =
            [entityComponentIdPair = m_entityComponentIdPair, sharedState,
            translationManipulator = AZStd::weak_ptr<AzToolsFramework::TranslationManipulators>(translationManipulators)](const auto& action)
        {
            WhiteBoxMesh* whiteBox = nullptr;
            EditorWhiteBoxComponentRequestBus::EventResult(
                whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

            size_t vertexIndex = 0;
            for (const Api::VertexHandle& vertexHandle : sharedState->m_vertexHandles)
            {
                const AZ::Vector3 vertexPosition = sharedState->m_vertexPositions[vertexIndex++] + action.LocalPositionOffset();
                Api::SetVertexPosition(*whiteBox, vertexHandle, vertexPosition);
            }
            if(auto manipulator = translationManipulator.lock()) {
                manipulator->SetLocalPosition(
                    sharedState->m_polygonMidpoint + action.LocalPositionOffset()
                );
            }

            Api::CalculatePlanarUVs(*whiteBox);
            EditorWhiteBoxComponentNotificationBus::Event(
                entityComponentIdPair, &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
        };

         auto mouseUpHandlerFn =
            [entityComponentIdPair = m_entityComponentIdPair, sharedState,
            translationManipulator = AZStd::weak_ptr<AzToolsFramework::TranslationManipulators>(translationManipulators)](const auto& action)
        {
            WhiteBoxMesh* whiteBox = nullptr;
            EditorWhiteBoxComponentRequestBus::EventResult(
                whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

            size_t vertexIndex = 0;
            for (const Api::VertexHandle& vertexHandle : sharedState->m_vertexHandles)
            {
                const AZ::Vector3 vertexPosition = sharedState->m_vertexPositions[vertexIndex++] + action.LocalPositionOffset();
                Api::SetVertexPosition(*whiteBox, vertexHandle, vertexPosition);
            }

            sharedState->m_vertexPositions = Api::VertexPositions(*whiteBox, sharedState->m_vertexHandles);
            sharedState->m_polygonMidpoint = sharedState->m_polygonMidpoint + action.LocalPositionOffset();
            
            if(auto manipulator = translationManipulator.lock()) {
                manipulator->SetLocalPosition(sharedState->m_polygonMidpoint);
            }

            Api::CalculateNormals(*whiteBox);
            Api::CalculatePlanarUVs(*whiteBox);

            EditorWhiteBoxComponentRequestBus::Event(
                    entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);
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

        struct SharedState
        {
            AZ::Vector3 m_polygonMidpoint = AZ::Vector3::CreateZero();

            AZStd::vector<AZ::Vector3> m_vertexPositions;

            Api::VertexHandles m_vertexHandles;
        };

        auto sharedState = AZStd::make_shared<SharedState>();
        if (auto selection = AZStd::get_if<PolygonIntersection>(&m_selection))
        {
            rotationManipulators->SetLocalPosition(Api::PolygonMidpoint(*whiteBox, selection->GetHandle()));
            rotationManipulators->SetLocalOrientation(AZ::Quaternion::CreateIdentity());
            
            sharedState->m_vertexHandles = Api::PolygonVertexHandles(*whiteBox, selection->GetHandle());
            sharedState->m_vertexPositions = Api::VertexPositions(*whiteBox, sharedState->m_vertexHandles);
            sharedState->m_polygonMidpoint = Api::PolygonMidpoint(*whiteBox, selection->GetHandle());;
        }

        rotationManipulators->SetLocalAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
        rotationManipulators->ConfigureView(
            AzToolsFramework::RotationManipulatorRadius(), AzFramework::ViewportColors::XAxisColor, AzFramework::ViewportColors::YAxisColor,
            AzFramework::ViewportColors::ZAxisColor);

        rotationManipulators->InstallMouseMoveCallback(
            [entityComponentIdPair = m_entityComponentIdPair, sharedState,
             translationManipulator = AZStd::weak_ptr<AzToolsFramework::RotationManipulators>(rotationManipulators)](
                const AzToolsFramework::AngularManipulator::Action& action)
            {
                WhiteBoxMesh* whiteBox = nullptr;
                EditorWhiteBoxComponentRequestBus::EventResult(
                    whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
                size_t vertexIndex = 0;
                for (const Api::VertexHandle& vertexHandle : sharedState->m_vertexHandles)
                {
                    const AZ::Vector3 vertexPosition =
                        (action.LocalOrientation())
                            .TransformVector(sharedState->m_vertexPositions[vertexIndex++] - sharedState->m_polygonMidpoint) +
                        sharedState->m_polygonMidpoint;
                    Api::SetVertexPosition(*whiteBox, vertexHandle, vertexPosition);
                }

                if (auto manipulator = translationManipulator.lock())
                {
                    manipulator->SetLocalOrientation(action.LocalOrientation());
                }

                Api::CalculatePlanarUVs(*whiteBox);
                EditorWhiteBoxComponentNotificationBus::Event(
                    entityComponentIdPair, &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
            });

        rotationManipulators->InstallLeftMouseUpCallback(
            [entityComponentIdPair = m_entityComponentIdPair, sharedState,
             translationManipulator = AZStd::weak_ptr<AzToolsFramework::RotationManipulators>(rotationManipulators)](
                const AzToolsFramework::AngularManipulator::Action& action)
            {
                WhiteBoxMesh* whiteBox = nullptr;
                EditorWhiteBoxComponentRequestBus::EventResult(
                    whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
                size_t vertexIndex = 0;
                for (const Api::VertexHandle& vertexHandle : sharedState->m_vertexHandles)
                {
                    const AZ::Vector3 vertexPosition =
                        (action.LocalOrientation())
                            .TransformVector(sharedState->m_vertexPositions[vertexIndex++] - sharedState->m_polygonMidpoint) +
                        sharedState->m_polygonMidpoint;
                    Api::SetVertexPosition(*whiteBox, vertexHandle, vertexPosition);
                }
                sharedState->m_vertexPositions = Api::VertexPositions(*whiteBox, sharedState->m_vertexHandles);
                if (auto manipulator = translationManipulator.lock())
                {
                    manipulator->SetLocalOrientation(AZ::Quaternion::CreateIdentity());
                }

                Api::CalculateNormals(*whiteBox);
                Api::CalculatePlanarUVs(*whiteBox);

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
            AzToolsFramework::LinearManipulatorAxisLength(), AzFramework::ViewportColors::XAxisColor,
            AzFramework::ViewportColors::YAxisColor, AzFramework::ViewportColors::ZAxisColor);

        struct SharedState
        {
            AZ::Vector3 m_polygonMidpoint = AZ::Vector3::CreateZero();

            AZ::Vector3 m_polygonScale = AZ::Vector3::CreateOne();

            AZStd::vector<AZ::Vector3> m_vertexPositions;

            Api::VertexHandles m_vertexHandles;
        };

        auto sharedState = AZStd::make_shared<SharedState>();
        if (auto selection = AZStd::get_if<PolygonIntersection>(&m_selection))
        {
            sharedState->m_vertexHandles = Api::PolygonVertexHandles(*whiteBox, selection->GetHandle());
            sharedState->m_vertexPositions = Api::VertexPositions(*whiteBox, sharedState->m_vertexHandles);
            sharedState->m_polygonMidpoint = Api::PolygonMidpoint(*whiteBox, selection->GetHandle());
            scaleManipulators->SetLocalPosition(sharedState->m_polygonMidpoint);
        }

        auto mouseMoveHandlerFn =
            [entityComponentIdPair = m_entityComponentIdPair, sharedState,
            scaleManipulator = AZStd::weak_ptr<AzToolsFramework::ScaleManipulators>(scaleManipulators)](const auto& action)
        {
            WhiteBoxMesh* whiteBox = nullptr;
            EditorWhiteBoxComponentRequestBus::EventResult(
                whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
            size_t vertexIndex = 0;
            for (const Api::VertexHandle& vertexHandle : sharedState->m_vertexHandles)
            {
                const AZ::Vector3 vertexLocalPosition = (sharedState->m_vertexPositions[vertexIndex++] - sharedState->m_polygonMidpoint);
                const AZ::Vector3 vertexPosition =  (vertexLocalPosition * (sharedState->m_polygonScale + (action.m_start.m_sign * action.LocalScaleOffset()))) + sharedState->m_polygonMidpoint;
                Api::SetVertexPosition(*whiteBox, vertexHandle, vertexPosition);
            }

            Api::CalculatePlanarUVs(*whiteBox);
            EditorWhiteBoxComponentNotificationBus::Event(
                entityComponentIdPair, &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
        };

        auto mouseUpHandlerFn =
            [entityComponentIdPair = m_entityComponentIdPair, sharedState,
            scaleManipulator = AZStd::weak_ptr<AzToolsFramework::ScaleManipulators>(scaleManipulators)](const auto& action)
        {
            WhiteBoxMesh* whiteBox = nullptr;
            EditorWhiteBoxComponentRequestBus::EventResult(
                whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
            
            size_t vertexIndex = 0;
            for (const Api::VertexHandle& vertexHandle : sharedState->m_vertexHandles)
            {
                const AZ::Vector3 vertexLocalPosition = (sharedState->m_vertexPositions[vertexIndex++] - sharedState->m_polygonMidpoint);
                const AZ::Vector3 vertexPosition =  (vertexLocalPosition * (sharedState->m_polygonScale + (action.m_start.m_sign * action.LocalScaleOffset()))) + sharedState->m_polygonMidpoint;
                Api::SetVertexPosition(*whiteBox, vertexHandle, vertexPosition);
            }
            sharedState->m_vertexPositions = Api::VertexPositions(*whiteBox, sharedState->m_vertexHandles);

            Api::CalculateNormals(*whiteBox);
            Api::CalculatePlanarUVs(*whiteBox);
            EditorWhiteBoxComponentRequestBus::Event(
                    entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);
        };

        scaleManipulators->InstallAxisMouseMoveCallback(mouseMoveHandlerFn);
        scaleManipulators->InstallUniformMouseMoveCallback(mouseMoveHandlerFn);

        scaleManipulators->InstallAxisLeftMouseUpCallback(mouseUpHandlerFn);
        scaleManipulators->InstallUniformLeftMouseUpCallback(mouseUpHandlerFn);

        scaleManipulators->Register(AzToolsFramework::g_mainManipulatorManagerId);
        m_manipulator = AZStd::move(scaleManipulators);
    }

} // namespace WhiteBox
