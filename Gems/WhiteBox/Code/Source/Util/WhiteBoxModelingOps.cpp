/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Util/WhiteBoxModelingOps.h"

#include "EditorWhiteBoxComponent.h"
#include "Util/WhiteBoxEditorUtil.h"
#include "EditorWhiteBoxComponentModeBus.h"
#include "SubComponentModes/EditorWhiteBoxTransformModeBus.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

namespace WhiteBox::ModelingOps
{
    namespace
    {
        //! The component, only when it has a mesh to operate on.
        EditorWhiteBoxComponent* EditableComponentFor(const AZ::EntityComponentIdPair& pair)
        {
            EditorWhiteBoxComponent* component = FindWhiteBoxComponent(pair);
            return (component != nullptr && component->GetWhiteBoxMesh() != nullptr) ? component : nullptr;
        }

        Api::PolygonHandles SelectedPolygons(const AZ::EntityComponentIdPair& pair)
        {
            Api::PolygonHandles polygons;
            EditorWhiteBoxTransformModeRequestBus::EventResult(
                polygons, pair, &EditorWhiteBoxTransformModeRequests::GetSelectedPolygons);
            return polygons;
        }

        Api::EdgeHandles SelectedEdges(const AZ::EntityComponentIdPair& pair)
        {
            Api::EdgeHandles edges;
            EditorWhiteBoxTransformModeRequestBus::EventResult(
                edges, pair, &EditorWhiteBoxTransformModeRequests::GetSelectedEdges);
            return edges;
        }

        Api::VertexHandles SelectedVertices(const AZ::EntityComponentIdPair& pair)
        {
            Api::VertexHandles vertices;
            EditorWhiteBoxTransformModeRequestBus::EventResult(
                vertices, pair, &EditorWhiteBoxTransformModeRequests::GetSelectedVertices);
            return vertices;
        }

        //! Shared tail of Bridge and Weld: drop the layer's parametric flag and write back the result.
        void CommitMeshEdit(EditorWhiteBoxComponent& component, const AZ::EntityComponentIdPair& pair)
        {
            EditorWhiteBoxTransformModeRequestBus::Event(pair, &EditorWhiteBoxTransformModeRequests::ClearSelection);
            component.BakeParametricLayer(component.GetActiveLayerIndex());
            component.SerializeWhiteBox();
            EditorWhiteBoxComponentNotificationBus::Event(
                pair, &EditorWhiteBoxComponentNotifications::OnWhiteBoxMeshModified);
        }
    } // namespace

    bool HasLiveBevel(const AZ::EntityComponentIdPair& pair)
    {
        auto* component = EditableComponentFor(pair);
        return component && component->HasActiveBevel();
    }

    Selection CurrentSelection(const AZ::EntityComponentIdPair& pair)
    {
        Selection selection;
        EditorWhiteBoxComponent* component = EditableComponentFor(pair);
        if (component == nullptr)
        {
            return selection;
        }
        selection.m_editable = true;
        selection.m_liveBevel = component->HasActiveBevel();
        selection.m_polygons = SelectedPolygons(pair);
        selection.m_edges = SelectedEdges(pair);
        selection.m_vertices = SelectedVertices(pair);
        return selection;
    }

    bool CanSelectEdgePattern(const Selection& selection)
    {
        return selection.m_editable && !selection.m_edges.empty();
    }

    Result SelectEdgePattern(const AZ::EntityComponentIdPair& pair, const bool ring)
    {
        bool selected = false;
        EditorWhiteBoxTransformModeRequestBus::EventResult(
            selected, pair, &EditorWhiteBoxTransformModeRequests::ExpandEdgeSelection, ring);
        if (!selected) { return {false, "Select polygon boundary edges in Transform mode first."}; }
        return {true, AZStd::string::format("%zu edges selected. %s stop at ambiguous junctions.",
            SelectedEdges(pair).size(), ring ? "Rings" : "Loops")};
    }

    bool CanExtrudeInset(const Selection& selection)
    {
        return selection.m_editable && !selection.m_polygons.empty();
    }

    Result ExtrudeInset(const AZ::EntityComponentIdPair& pair, const float amount, const bool inset)
    {
        auto* component = EditableComponentFor(pair);
        if (!component) { return {false, "No editable White Box mesh."}; }
        Api::PolygonHandles result;
        AZStd::string error;
        AzToolsFramework::ScopedUndoBatch undo(inset ? "White Box Inset" : "White Box Extrude");
        if (!Api::ExtrudeInsetRegions(*component->GetWhiteBoxMesh(), SelectedPolygons(pair), amount, inset, result, error))
        {
            return {false, error};
        }
        CommitMeshEdit(*component, pair);
        EditorWhiteBoxTransformModeRequestBus::Event(
            pair, &EditorWhiteBoxTransformModeRequests::SetSelectedPolygons, result);
        undo.MarkEntityDirty(pair.GetEntityId());
        return {true, inset ? "Region inset." : "Region extruded."};
    }

    bool CanBridge(const Selection& selection)
    {
        if (!selection.m_editable)
        {
            return false;
        }
        return (selection.m_polygons.size() == 2 && selection.m_edges.empty()) ||
            (selection.m_edges.size() == 2 && selection.m_polygons.empty());
    }

    bool CanWeld(const Selection& selection)
    {
        return selection.m_editable && selection.m_vertices.size() >= 2;
    }

    bool CanLoopCut(const Selection& selection)
    {
        return selection.m_editable;
    }

    bool CanBevel(const Selection& selection)
    {
        if (!selection.m_editable || selection.m_liveBevel)
        {
            return false; // one live bevel at a time; bake or cancel it first
        }
        return !selection.m_edges.empty() || !selection.m_polygons.empty();
    }

    Result Bridge(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        EditorWhiteBoxComponent* component = EditableComponentFor(entityComponentIdPair);
        if (component == nullptr)
        {
            return { false, "No editable White Box mesh." };
        }

        const Api::PolygonHandles polygons = SelectedPolygons(entityComponentIdPair);
        const Api::EdgeHandles edges = SelectedEdges(entityComponentIdPair);

        AZStd::string error;
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("White Box Bridge");
            if (!Api::BridgeSelection(*component->GetWhiteBoxMesh(), polygons, edges, error))
            {
                return { false, error };
            }
            CommitMeshEdit(*component, entityComponentIdPair);
            undoBatch.MarkEntityDirty(entityComponentIdPair.GetEntityId());
        }
        return { true, "Bridge created. Undo to restore the original selection's geometry." };
    }

    Result Weld(const AZ::EntityComponentIdPair& entityComponentIdPair, const bool atLastVertex)
    {
        EditorWhiteBoxComponent* component = EditableComponentFor(entityComponentIdPair);
        if (component == nullptr)
        {
            return { false, "No editable White Box mesh." };
        }

        const Api::VertexHandles vertices = SelectedVertices(entityComponentIdPair);

        AZStd::string error;
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("White Box Weld");
            if (!Api::WeldVertices(*component->GetWhiteBoxMesh(), vertices, atLastVertex, error))
            {
                return { false, error };
            }
            CommitMeshEdit(*component, entityComponentIdPair);
            undoBatch.MarkEntityDirty(entityComponentIdPair.GetEntityId());
        }
        return { true, "Vertices welded. Undo restores the original mesh." };
    }

    Result BeginLoopCut(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        EditorWhiteBoxComponent* component = EditableComponentFor(entityComponentIdPair);
        if (component == nullptr)
        {
            return { false, "No editable White Box mesh." };
        }

        namespace Cmf = AzToolsFramework::ComponentModeFramework;
        bool inComponentMode = false;
        Cmf::ComponentModeSystemRequestBus::BroadcastResult(
            inComponentMode, &Cmf::ComponentModeSystemRequests::InComponentMode);

        const auto beginOn = [](const AZ::EntityComponentIdPair& pair)
        {
            EditorWhiteBoxComponentModeRequestBus::Event(
                pair, &EditorWhiteBoxComponentModeRequests::SetSubMode, SubMode::Transform);
            EditorWhiteBoxTransformModeRequestBus::Event(pair, &EditorWhiteBoxTransformModeRequests::BeginLoopCut);
        };

        if (inComponentMode)
        {
            beginOn(entityComponentIdPair);
        }
        else
        {
            // Not in component mode yet (the pane can start a loop cut from outside it): select the
            // entity, enter component mode, and begin on the next tick once that has taken effect.
            const AZ::EntityId entityId = entityComponentIdPair.GetEntityId();
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities,
                AzToolsFramework::EntityIdList{ entityId });
            AZ::TickBus::QueueFunction(
                [entityId, beginOn]()
                {
                    AZ::Entity* entity = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(
                        entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
                    if (entity == nullptr)
                    {
                        return;
                    }
                    if (auto* whiteBox = entity->FindComponent<EditorWhiteBoxComponent>())
                    {
                        whiteBox->EnterComponentMode();
                        beginOn(AZ::EntityComponentIdPair(entityId, whiteBox->GetId()));
                    }
                });
        }

        return { true,
                 "Hover a face or edge. Wheel: count. Click: lock. Move: slide. Click again: cut. "
                 "Esc / right-click: cancel." };
    }

    Result BeginBevel(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        EditorWhiteBoxComponent* component = EditableComponentFor(entityComponentIdPair);
        if (component == nullptr || component->HasActiveBevel())
        {
            return { false, "Bake or cancel the live bevel first." };
        }

        Api::EdgeHandles edges = SelectedEdges(entityComponentIdPair);
        const Api::PolygonHandles polygons = SelectedPolygons(entityComponentIdPair);

        // Only the perimeter edges of the selected polygon region; an edge shared by two selected
        // polygons is interior to the region and would otherwise become an unwanted cut through it.
        WhiteBoxMesh* mesh = component->GetWhiteBoxMesh();
        for (const auto edge : Api::MeshPolygonEdgeHandles(*mesh))
        {
            int selectedSides = 0;
            for (const auto face : Api::EdgeFaceHandles(*mesh, edge))
            {
                const auto polygon = Api::FacePolygonHandle(*mesh, face);
                if (AZStd::find(polygons.begin(), polygons.end(), polygon) != polygons.end())
                {
                    ++selectedSides;
                }
            }
            if (selectedSides == 1 && AZStd::find(edges.begin(), edges.end(), edge) == edges.end())
            {
                edges.push_back(edge);
            }
        }

        AZStd::string error;
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("White Box Start Bevel");
            if (!component->SetParametricBevel(edges, component->GetBevelParams(), error))
            {
                return { false, error };
            }
            undoBatch.MarkEntityDirty(entityComponentIdPair.GetEntityId());
        }
        return { true, "Live bevel: adjust Width, Segments and Profile, then Bake or Cancel." };
    }
} // namespace WhiteBox::ModelingOps
