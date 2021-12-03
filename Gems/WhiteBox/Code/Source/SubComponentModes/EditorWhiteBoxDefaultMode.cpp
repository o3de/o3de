/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorWhiteBoxComponentModeCommon.h"
#include "EditorWhiteBoxDefaultMode.h"
#include "Viewport/WhiteBoxEdgeScaleModifier.h"
#include "Viewport/WhiteBoxEdgeTranslationModifier.h"
#include "Viewport/WhiteBoxPolygonScaleModifier.h"
#include "Viewport/WhiteBoxPolygonTranslationModifier.h"
#include "Viewport/WhiteBoxVertexTranslationModifier.h"
#include "Viewport/WhiteBoxViewportConstants.h"

#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <QKeySequence>
#include <QLayout>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

namespace WhiteBox
{
    AZ_CVAR(
        float, cl_whiteBoxVertexIndicatorLength, 0.1f, nullptr, AZ::ConsoleFunctorFlags::Null,
        "The length of each vertex indicator axis");
    AZ_CVAR(
        float, cl_whiteBoxVertexIndicatorWidth, 5.0f, nullptr, AZ::ConsoleFunctorFlags::Null,
        "The width/thickness of each vertex indicator axis");
    AZ_CVAR(
        AZ::Color, cl_whiteBoxVertexIndicatorColor, AZ::Color::CreateFromRgba(0, 0, 0, 102), nullptr,
        AZ::ConsoleFunctorFlags::Null, "The color of the vertex indicator");

    static const AZ::Crc32 HideEdge = AZ_CRC("com.o3de.action.whitebox.hide_edge", 0x84f6a9b9);
    static const AZ::Crc32 HideVertex = AZ_CRC("com.o3de.action.whitebox.hide_vertex", 0x5f81c937);

    static const char* const HideEdgeTitle = "Hide Edge";
    static const char* const HideEdgeDesc = "Hide the selected edge to merge the two connected polygons";
    static const char* const HideVertexTitle = "Hide Vertex";
    static const char* const HideVertexDesc = "Hide the selected vertex to merge the two connected edges";

    static const char* const HideEdgeUndoRedoDesc = "Hide an edge to merge two connected polygons together";
    static const char* const HideVertexUndoRedoDesc = "Hide a vertex to merge two connected edges together";

    const QKeySequence HideKey = QKeySequence{Qt::Key_H};

    // handle translation and scale modifiers for either polygon or edge - if a translation
    // modifier is set (either polygon or edge), update the intersection point so the next time
    // mouse down happens the delta offsets of the manipulator are calculated from the correct
    // position and also handle clicking off of a selected modifier to clear the selected state
    // note: the Intersection type must match the geometry for the translation and scale modifier
    template<typename TranslationModifier, typename Intersection, typename DestroyOtherModifierFn>
    static void HandleMouseInteractionForModifier(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction,
        DefaultMode::SelectedTranslationModifier& selectedTranslationModifier,
        DestroyOtherModifierFn&& destroyOtherModifierFn, const AZStd::optional<Intersection>& geometryIntersection)
    {
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<TranslationModifier>>(&selectedTranslationModifier))
        {
            // handle clicking off of selected geometry
            if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
                mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Up &&
                !geometryIntersection.has_value())
            {
                selectedTranslationModifier = AZStd::monostate{};
                destroyOtherModifierFn();

                AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                    &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::RefreshActions);
            }
        }
    }

    // in this generic context TranslationModifier and OtherTranslationModifier correspond
    // to Polygon/Edge or Edge/Polygon depending on the context (which was intersected)
    template<typename Intersection, typename TranslationModifier, typename DestroyOtherModifierFn>
    static void HandleCreatingDestroyingModifiersOnIntersectionChange(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction,
        DefaultMode::SelectedTranslationModifier& selectedTranslationModifier,
        AZStd::unique_ptr<TranslationModifier>& translationModifier, DestroyOtherModifierFn&& destroyOtherModifierFn,
        const AZStd::optional<Intersection>& geometryIntersection,
        const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        // if we have a valid mouse ray intersection with the geometry (e.g. edge or polygon)
        if (geometryIntersection.has_value())
        {
            // does the geometry the mouse is hovering over match the currently selected geometry
            const bool matchesSelected = [&geometryIntersection, &selectedTranslationModifier]()
            {
                if (auto modifier = AZStd::get_if<AZStd::unique_ptr<TranslationModifier>>(&selectedTranslationModifier))
                {
                    return (*modifier)->GetHandle() == geometryIntersection->GetHandle();
                }
                return false;
            }();

            // check if there's currently no modifier or if we need to make a different modifier as
            // the geometry is different
            const bool shouldCreateTranslationModifier =
                !translationModifier || (translationModifier->GetHandle() != geometryIntersection->GetHandle());

            if (shouldCreateTranslationModifier && !matchesSelected)
            {
                // created a modifier for the intersected geometry
                translationModifier = AZStd::make_unique<TranslationModifier>(
                    entityComponentIdPair, geometryIntersection->GetHandle(),
                    geometryIntersection->m_intersection.m_localIntersectionPoint);

                translationModifier->ForwardMouseOverEvent(mouseInteraction.m_mouseInteraction);

                destroyOtherModifierFn();
            }
        }
    }

    AZ_CLASS_ALLOCATOR_IMPL(DefaultMode, AZ::SystemAllocator, 0)

    DefaultMode::DefaultMode(const AZ::EntityComponentIdPair& entityComponentIdPair)
        : m_entityComponentIdPair(entityComponentIdPair)
    {
        EditorWhiteBoxDefaultModeRequestBus::Handler::BusConnect(entityComponentIdPair);
        EditorWhiteBoxPolygonModifierNotificationBus::Handler::BusConnect(entityComponentIdPair);
        EditorWhiteBoxEdgeModifierNotificationBus::Handler::BusConnect(entityComponentIdPair);
    }

    DefaultMode::~DefaultMode()
    {
        EditorWhiteBoxEdgeModifierNotificationBus::Handler::BusDisconnect();
        EditorWhiteBoxPolygonModifierNotificationBus::Handler::BusDisconnect();
        EditorWhiteBoxDefaultModeRequestBus::Handler::BusDisconnect();
    }

    void DefaultMode::Refresh()
    {
        // destroy all active modifiers
        m_polygonScaleModifier.reset();
        m_edgeScaleModifier.reset();
        m_polygonTranslationModifier.reset();
        m_edgeTranslationModifier.reset();
        m_vertexTranslationModifier.reset();
        m_selectedModifier = AZStd::monostate{};
    }

    AZStd::vector<AzToolsFramework::ActionOverride> DefaultMode::PopulateActions(
        const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        // edge selection test - ensure an edge is selected before allowing this shortcut
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<EdgeTranslationModifier>>(&m_selectedModifier))
        {
            return AZStd::vector<AzToolsFramework::ActionOverride>{
                AzToolsFramework::ActionOverride()
                    .SetUri(HideEdge)
                    .SetKeySequence(HideKey)
                    .SetTitle(HideEdgeTitle)
                    .SetTip(HideEdgeDesc)
                    .SetEntityComponentIdPair(entityComponentIdPair)
                    .SetCallback(
                        [entityComponentIdPair, modifier]()
                        {
                            WhiteBoxMesh* whiteBox = nullptr;
                            EditorWhiteBoxComponentRequestBus::EventResult(
                                whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

                            Api::HideEdge(*whiteBox, (*modifier)->GetEdgeHandle());
                            (*modifier)->SetEdgeHandle(Api::EdgeHandle{});

                            RecordWhiteBoxAction(*whiteBox, entityComponentIdPair, HideEdgeUndoRedoDesc);
                        })};
        }
        // vertex selection test - ensure a vertex is selected before allowing this shortcut
        else if (auto modifier2 = AZStd::get_if<AZStd::unique_ptr<VertexTranslationModifier>>(&m_selectedModifier))
        {
            return AZStd::vector<AzToolsFramework::ActionOverride>{
                AzToolsFramework::ActionOverride()
                    .SetUri(HideVertex)
                    .SetKeySequence(HideKey)
                    .SetTitle(HideVertexTitle)
                    .SetTip(HideVertexDesc)
                    .SetEntityComponentIdPair(entityComponentIdPair)
                    .SetCallback(
                        [entityComponentIdPair, modifier2]()
                        {
                            WhiteBoxMesh* whiteBox = nullptr;
                            EditorWhiteBoxComponentRequestBus::EventResult(
                                whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

                            Api::HideVertex(*whiteBox, (*modifier2)->GetVertexHandle());
                            (*modifier2)->SetVertexHandle(Api::VertexHandle{});

                            RecordWhiteBoxAction(*whiteBox, entityComponentIdPair, HideVertexUndoRedoDesc);
                        })};
        }
        else
        {
            return {};
        }
    }

    template<typename ModifierUniquePtr>
    static void TryDestroyModifier(ModifierUniquePtr& modifier)
    {
        // has the mouse moved off of the modifier
        if (modifier && !modifier->MouseOver())
        {
            modifier.reset();
        }
    }

    static void DrawVertices(
        AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Transform& worldFromLocal,
        const AzFramework::CameraState& cameraState, const IntersectionAndRenderData& renderData)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        const float vertexIndicatorLength = cl_whiteBoxVertexIndicatorLength;
        const float vertexIndicatorWidth = cl_whiteBoxVertexIndicatorWidth;
        const AZ::Color vertexIndicatorColor = cl_whiteBoxVertexIndicatorColor;

        debugDisplay.SetLineWidth(vertexIndicatorWidth);
        debugDisplay.SetColor(vertexIndicatorColor);

        const auto drawVertIndicator = [&debugDisplay, &worldFromLocal, &cameraState, vertexIndicatorLength](
                                           const AZ::Vector3& start, const AZ::Vector3& axis, const float length)
        {
            const auto scale =
                AzToolsFramework::CalculateScreenToWorldMultiplier(worldFromLocal.TransformPoint(start), cameraState);
            debugDisplay.DrawLine(start, start + axis * AZ::GetMin<float>(length, scale * vertexIndicatorLength));
        };

        for (const auto& edgeBound : renderData.m_whiteBoxEdgeRenderData.m_bounds.m_user)
        {
            const auto& start = edgeBound.m_bound.m_start;
            const auto& end = edgeBound.m_bound.m_end;
            const auto edge = end - start;
            const auto length = edge.GetLength();

            if (length > 0.0f)
            {
                const auto axis = edge / length;
                drawVertIndicator(start, axis, length);
                drawVertIndicator(end, -axis, length);
            }
        }

        debugDisplay.SetLineWidth(1.0f);
    }

    void DefaultMode::Display(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Transform& worldFromLocal,
        const IntersectionAndRenderData& renderData, [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        TryDestroyModifier(m_polygonTranslationModifier);
        TryDestroyModifier(m_edgeTranslationModifier);
        TryDestroyModifier(m_vertexTranslationModifier);

        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        debugDisplay.PushMatrix(worldFromLocal);

        DrawEdges(
            debugDisplay, cl_whiteBoxEdgeUserColor, renderData.m_whiteBoxIntersectionData.m_edgeBounds,
            FindInteractiveEdgeHandles(*whiteBox));

        DrawVertices(
            debugDisplay, worldFromLocal, AzToolsFramework::GetCameraState(viewportInfo.m_viewportId), renderData);

        debugDisplay.PopMatrix();
    }

    Api::EdgeHandles DefaultMode::FindInteractiveEdgeHandles(const WhiteBoxMesh& whiteBox)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // get all edge handles for hovered polygon
        const Api::EdgeHandles polygonHoveredEdgeHandles = m_polygonTranslationModifier
            ? Api::PolygonBorderEdgeHandlesFlattened(whiteBox, m_polygonTranslationModifier->GetPolygonHandle())
            : Api::EdgeHandles{};

        // find edge handles being used by active modifiers
        const Api::EdgeHandles selectedEdgeHandles = [&whiteBox, this]()
        {
            if (auto modifier = AZStd::get_if<AZStd::unique_ptr<PolygonTranslationModifier>>(&m_selectedModifier))
            {
                return Api::PolygonBorderEdgeHandlesFlattened(whiteBox, (*modifier)->GetPolygonHandle());
            }

            if (auto modifier = AZStd::get_if<AZStd::unique_ptr<EdgeTranslationModifier>>(&m_selectedModifier))
            {
                return Api::EdgeHandles{(*modifier)->GetEdgeHandle()};
            }

            return Api::EdgeHandles{};
        }();

        // combine all potentially interactive edge handles
        Api::EdgeHandles interactiveEdgeHandles{polygonHoveredEdgeHandles};
        interactiveEdgeHandles.insert(
            interactiveEdgeHandles.end(), selectedEdgeHandles.begin(), selectedEdgeHandles.end());

        if (m_edgeTranslationModifier)
        {
            // get edge handle for hovered edge (and associated group)
            interactiveEdgeHandles.insert(
                interactiveEdgeHandles.end(), m_edgeTranslationModifier->EdgeHandlesBegin(),
                m_edgeTranslationModifier->EdgeHandlesEnd());
        }

        return interactiveEdgeHandles;
    }

    // if an edge or polygon scale modifier are selected, their scale manipulators (situated
    // at the same position as a vertex) should take priority, so do not attempt to create
    // a vertex selection modifier for those vertices that currently have a scale modifier
    static bool IgnoreVertexHandle(
        const WhiteBoxMesh& whiteBox, const PolygonScaleModifier* polygonScaleModifier,
        const EdgeScaleModifier* edgeScaleModifier, const Api::VertexHandle vertexHandle)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (Api::VertexIsHidden(whiteBox, vertexHandle))
        {
            return true;
        }

        Api::VertexHandles vertexHandlesToIgnore;

        if (polygonScaleModifier)
        {
            const auto polygonVertexHandles =
                Api::PolygonVertexHandles(whiteBox, polygonScaleModifier->GetPolygonHandle());

            vertexHandlesToIgnore.insert(
                vertexHandlesToIgnore.end(), polygonVertexHandles.cbegin(), polygonVertexHandles.cend());
        }

        if (edgeScaleModifier)
        {
            const auto edgeVertexHandles = Api::EdgeVertexHandles(whiteBox, edgeScaleModifier->GetEdgeHandle());

            vertexHandlesToIgnore.insert(
                vertexHandlesToIgnore.end(), edgeVertexHandles.cbegin(), edgeVertexHandles.cend());
        }

        return AZStd::find(vertexHandlesToIgnore.cbegin(), vertexHandlesToIgnore.cend(), vertexHandle) !=
            vertexHandlesToIgnore.cend();
    }

    // only return a valid optional if the vertex intersection is valid and it is not hidden
    static AZStd::optional<VertexIntersection> FilterHiddenVertexIntersection(
        const AZStd::optional<VertexIntersection>& vertexIntersection, const WhiteBoxMesh& whiteBox)
    {
        if (!vertexIntersection.has_value() || Api::VertexIsHidden(whiteBox, vertexIntersection->GetHandle()))
        {
            return AZStd::nullopt;
        }

        return vertexIntersection;
    }

    bool DefaultMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction,
        const AZ::EntityComponentIdPair& entityComponentIdPair,
        const AZStd::optional<EdgeIntersection>& edgeIntersection,
        const AZStd::optional<PolygonIntersection>& polygonIntersection,
        const AZStd::optional<VertexIntersection>& vertexIntersection)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        // polygon
        HandleMouseInteractionForModifier<PolygonTranslationModifier, PolygonIntersection>(
            mouseInteraction, m_selectedModifier,
            [this]
            {
                m_polygonScaleModifier.reset();
            },
            polygonIntersection);

        // edge
        HandleMouseInteractionForModifier<EdgeTranslationModifier, EdgeIntersection>(
            mouseInteraction, m_selectedModifier,
            [this]
            {
                m_edgeScaleModifier.reset();
            },
            edgeIntersection);

        // do not allow intersections with hidden vertices in the default mode
        const auto allowedVertexIntersection = FilterHiddenVertexIntersection(vertexIntersection, *whiteBox);

        // vertex
        HandleMouseInteractionForModifier<VertexTranslationModifier, VertexIntersection>(
            mouseInteraction, m_selectedModifier, [] { /*noop*/ }, allowedVertexIntersection);

        switch (FindClosestGeometryIntersection(edgeIntersection, polygonIntersection, allowedVertexIntersection))
        {
        case GeometryIntersection::Edge:
            {
                HandleCreatingDestroyingModifiersOnIntersectionChange<EdgeIntersection, EdgeTranslationModifier>(
                    mouseInteraction, m_selectedModifier, m_edgeTranslationModifier,
                    [this]
                    {
                        m_polygonTranslationModifier.reset();
                        m_vertexTranslationModifier.reset();
                    },
                    edgeIntersection, entityComponentIdPair);
            }
            break;
        case GeometryIntersection::Polygon:
            {
                HandleCreatingDestroyingModifiersOnIntersectionChange<PolygonIntersection, PolygonTranslationModifier>(
                    mouseInteraction, m_selectedModifier, m_polygonTranslationModifier,
                    [this]
                    {
                        m_edgeTranslationModifier.reset();
                        m_vertexTranslationModifier.reset();
                    },
                    polygonIntersection, entityComponentIdPair);
            }
            break;
        case GeometryIntersection::Vertex:
            {
                if (!IgnoreVertexHandle(
                        *whiteBox, m_polygonScaleModifier.get(), m_edgeScaleModifier.get(),
                        allowedVertexIntersection->GetHandle()))
                {
                    HandleCreatingDestroyingModifiersOnIntersectionChange<
                        VertexIntersection, VertexTranslationModifier>(
                        mouseInteraction, m_selectedModifier, m_vertexTranslationModifier,
                        [this]
                        {
                            m_edgeTranslationModifier.reset();
                            m_polygonTranslationModifier.reset();
                        },
                        allowedVertexIntersection, entityComponentIdPair);
                }
            }
            break;
        case GeometryIntersection::None:
            // do nothing
            break;
        default:
            // do nothing
            break;
        }

        return false;
    }

    void DefaultMode::CreatePolygonScaleModifier(const Api::PolygonHandle& polygonHandle)
    {
        m_polygonScaleModifier = AZStd::make_unique<PolygonScaleModifier>(polygonHandle, m_entityComponentIdPair);
    }

    void DefaultMode::CreateEdgeScaleModifier(const Api::EdgeHandle edgeHandle)
    {
        m_edgeScaleModifier = AZStd::make_unique<EdgeScaleModifier>(edgeHandle, m_entityComponentIdPair);
    }

    void DefaultMode::AssignSelectedPolygonTranslationModifier()
    {
        if (m_polygonTranslationModifier)
        {
            m_selectedModifier = AZStd::move(m_polygonTranslationModifier);

            AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::RefreshActions);

            if (auto modifier = AZStd::get_if<AZStd::unique_ptr<PolygonTranslationModifier>>(&m_selectedModifier))
            {
                (*modifier)->SetColors(
                    AZ::Color::CreateFromVector3AndFloat(
                        static_cast<AZ::Color>(cl_whiteBoxSelectedModifierColor).GetAsVector3(), 0.5f),
                    AZ::Color::CreateFromVector3AndFloat(
                        static_cast<AZ::Color>(cl_whiteBoxSelectedModifierColor).GetAsVector3(), 1.0f));
                (*modifier)->CreateView();
            }

            m_edgeScaleModifier.reset();
            m_vertexTranslationModifier.reset();

            m_polygonTranslationModifier = nullptr;
        }
    }

    void DefaultMode::AssignSelectedEdgeTranslationModifier()
    {
        if (m_edgeTranslationModifier)
        {
            m_selectedModifier = AZStd::move(m_edgeTranslationModifier);

            AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::RefreshActions);

            if (auto modifier = AZStd::get_if<AZStd::unique_ptr<EdgeTranslationModifier>>(&m_selectedModifier))
            {
                (*modifier)->SetColors(cl_whiteBoxSelectedModifierColor, cl_whiteBoxSelectedModifierColor);
                (*modifier)->SetWidths(cl_whiteBoxSelectedEdgeVisualWidth, cl_whiteBoxSelectedEdgeVisualWidth);
                (*modifier)->CreateView();
            }

            m_polygonScaleModifier.reset();
            m_vertexTranslationModifier.reset();

            m_edgeTranslationModifier = nullptr;
        }
    }

    void DefaultMode::AssignSelectedVertexSelectionModifier()
    {
        if (m_vertexTranslationModifier)
        {
            m_selectedModifier = AZStd::move(m_vertexTranslationModifier);

            AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::RefreshActions);

            if (auto modifier = AZStd::get_if<AZStd::unique_ptr<VertexTranslationModifier>>(&m_selectedModifier))
            {
                (*modifier)->SetColor(cl_whiteBoxVertexSelectedModifierColor);
                (*modifier)->CreateView();
            }

            m_polygonScaleModifier.reset();
            m_edgeScaleModifier.reset();

            m_vertexTranslationModifier = nullptr;
        }
    }

    void DefaultMode::RefreshPolygonScaleModifier()
    {
        if (m_polygonScaleModifier)
        {
            m_polygonScaleModifier->Refresh();
        }
    }

    void DefaultMode::RefreshEdgeScaleModifier()
    {
        if (m_edgeScaleModifier)
        {
            m_edgeScaleModifier->Refresh();
        }
    }

    void DefaultMode::RefreshPolygonTranslationModifier()
    {
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<PolygonTranslationModifier>>(&m_selectedModifier))
        {
            if (!(*modifier)->PerformingAction())
            {
                (*modifier)->Refresh();
            }
        }

        if (m_polygonTranslationModifier && !m_polygonTranslationModifier->PerformingAction())
        {
            m_polygonTranslationModifier->Refresh();
        }
    }

    void DefaultMode::RefreshEdgeTranslationModifier()
    {
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<EdgeTranslationModifier>>(&m_selectedModifier))
        {
            if (!(*modifier)->PerformingAction())
            {
                (*modifier)->Refresh();
            }
        }

        if (m_edgeTranslationModifier && !m_edgeTranslationModifier->PerformingAction())
        {
            m_edgeTranslationModifier->Refresh();
        }
    }

    void DefaultMode::RefreshVertexSelectionModifier()
    {
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<VertexTranslationModifier>>(&m_selectedModifier))
        {
            if (!(*modifier)->PerformingAction())
            {
                (*modifier)->Refresh();
            }
        }

        if (m_vertexTranslationModifier && !m_vertexTranslationModifier->PerformingAction())
        {
            m_vertexTranslationModifier->Refresh();
        }
    }

    template<typename Modifier>
    auto ModifierHandles(const DefaultMode::SelectedTranslationModifier& selectedModifier)
    {
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<Modifier>>(&selectedModifier))
        {
            return AZStd::vector<typename Modifier::HandleType>{(*modifier)->GetHandle()};
        }

        return AZStd::vector<typename Modifier::HandleType>{};
    }

    Api::VertexHandles DefaultMode::SelectedVertexHandles() const
    {
        return ModifierHandles<VertexTranslationModifier>(m_selectedModifier);
    }

    Api::EdgeHandles DefaultMode::SelectedEdgeHandles() const
    {
        return ModifierHandles<EdgeTranslationModifier>(m_selectedModifier);
    }

    Api::PolygonHandles DefaultMode::SelectedPolygonHandles() const
    {
        return ModifierHandles<PolygonTranslationModifier>(m_selectedModifier);
    }

    Api::VertexHandle DefaultMode::HoveredVertexHandle() const
    {
        return m_vertexTranslationModifier ? m_vertexTranslationModifier->GetVertexHandle() : Api::VertexHandle();
    }

    Api::EdgeHandle DefaultMode::HoveredEdgeHandle() const
    {
        return m_edgeTranslationModifier ? m_edgeTranslationModifier->GetEdgeHandle() : Api::EdgeHandle();
    }

    Api::PolygonHandle DefaultMode::HoveredPolygonHandle() const
    {
        return m_polygonTranslationModifier ? m_polygonTranslationModifier->GetPolygonHandle() : Api::PolygonHandle();
    }

    void DefaultMode::OnPolygonModifierUpdatedPolygonHandle(
        const Api::PolygonHandle& previousPolygonHandle, const Api::PolygonHandle& nextPolygonHandle)
    {
        // an operation has caused the currently selected polygon handle to update (e.g. an append/extrusion)
        // if the previous polygon handle matches the selected polygon translation modifier, we know it caused
        // the extrusion and should be updated
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<PolygonTranslationModifier>>(&m_selectedModifier))
        {
            if ((*modifier)->GetPolygonHandle() == previousPolygonHandle)
            {
                (*modifier)->SetPolygonHandle(nextPolygonHandle);
                m_polygonScaleModifier->SetPolygonHandle(nextPolygonHandle);
            }
        }
    }

    void DefaultMode::OnEdgeModifierUpdatedEdgeHandle(
        const Api::EdgeHandle previousEdgeHandle, const Api::EdgeHandle nextEdgeHandle)
    {
        // an operation has caused the currently selected edge handle to update (e.g. an append/extrusion)
        // if the previous edge handle matches the selected edge translation modifier, we know it caused
        // the extrusion and should be updated
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<EdgeTranslationModifier>>(&m_selectedModifier))
        {
            if ((*modifier)->GetEdgeHandle() == previousEdgeHandle)
            {
                m_edgeScaleModifier->SetEdgeHandle(nextEdgeHandle);
            }
        }
    }
} // namespace WhiteBox
