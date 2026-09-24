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
        return selection.m_editable && (!selection.m_edges.empty() || !selection.m_polygons.empty());
    }

    Result SelectEdgePattern(const AZ::EntityComponentIdPair& pair, const bool ring)
    {
        // Whichever the selection already is: faces grow into a strip, edges into a loop or ring.
        if (const auto polygons = SelectedPolygons(pair); !polygons.empty())
        {
            EditorWhiteBoxComponent* component = EditableComponentFor(pair);
            if (component == nullptr)
            {
                return { false, "No editable White Box mesh." };
            }
            auto* mesh = component->GetWhiteBoxMesh();
            const auto strip =
                ring ? Api::FindPolygonRing(*mesh, polygons) : Api::FindPolygonLoop(*mesh, polygons);
            if (strip.size() <= polygons.size())
            {
                return { false, "No face strip runs through that selection. Quads only, and it stops where the "
                    "edge ring does." };
            }
            EditorWhiteBoxTransformModeRequestBus::Event(
                pair, &EditorWhiteBoxTransformModeRequests::SetSelectedPolygons, strip);
            return { true, AZStd::string::format("%zu faces selected.", strip.size()) };
        }

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

    bool CanFillHole(const Selection& selection)
    {
        return selection.m_editable && !selection.m_edges.empty();
    }

    bool CanMergePolygons(const Selection& selection)
    {
        return selection.m_editable && selection.m_polygons.size() >= 2;
    }

    bool CanSelectLinked(const Selection& selection)
    {
        return selection.m_editable &&
            (!selection.m_polygons.empty() || !selection.m_edges.empty() || !selection.m_vertices.empty());
    }

    bool CanSelectCoplanar(const Selection& selection)
    {
        return selection.m_editable && !selection.m_polygons.empty();
    }

    bool CanDeletePolygon(const Selection& selection)
    {
        return selection.m_editable && !selection.m_polygons.empty();
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

    Result FillHole(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        EditorWhiteBoxComponent* component = EditableComponentFor(entityComponentIdPair);
        if (component == nullptr)
        {
            return { false, "No editable White Box mesh." };
        }

        const Api::EdgeHandles edges = SelectedEdges(entityComponentIdPair);

        AZStd::string error;
        Api::PolygonHandle filled;
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("White Box Fill Hole");
            if (!Api::FillHole(*component->GetWhiteBoxMesh(), edges, error, &filled))
            {
                return { false, error };
            }
            CommitMeshEdit(*component, entityComponentIdPair);
            // The new cap is selected so it can be extruded or textured without hunting for it.
            EditorWhiteBoxTransformModeRequestBus::Event(
                entityComponentIdPair, &EditorWhiteBoxTransformModeRequests::SetSelectedPolygons,
                Api::PolygonHandles{ filled });
            undoBatch.MarkEntityDirty(entityComponentIdPair.GetEntityId());
        }
        return { true, "Hole filled. Undo restores the open border." };
    }

    Result SelectLinked(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        EditorWhiteBoxComponent* component = EditableComponentFor(entityComponentIdPair);
        if (component == nullptr)
        {
            return { false, "No editable White Box mesh." };
        }
        auto* mesh = component->GetWhiteBoxMesh();

        // Whichever element type is selected is the one that grows, matching the filter buttons.
        const auto polygons = SelectedPolygons(entityComponentIdPair);
        const auto edges = SelectedEdges(entityComponentIdPair);
        const auto vertices = SelectedVertices(entityComponentIdPair);
        size_t before = 0;
        size_t after = 0;
        const char* noun = "";
        if (!polygons.empty())
        {
            const auto linked = Api::FindLinkedPolygons(*mesh, polygons);
            before = polygons.size();
            after = linked.size();
            noun = "polygons";
            EditorWhiteBoxTransformModeRequestBus::Event(
                entityComponentIdPair, &EditorWhiteBoxTransformModeRequests::SetSelectedPolygons, linked);
        }
        else if (!edges.empty())
        {
            const auto linked = Api::FindLinkedEdges(*mesh, edges);
            before = edges.size();
            after = linked.size();
            noun = "edges";
            EditorWhiteBoxTransformModeRequestBus::Event(
                entityComponentIdPair, &EditorWhiteBoxTransformModeRequests::SetSelectedEdges, linked);
        }
        else if (!vertices.empty())
        {
            const auto linked = Api::FindLinkedVertices(*mesh, vertices);
            before = vertices.size();
            after = linked.size();
            noun = "vertices";
            EditorWhiteBoxTransformModeRequestBus::Event(
                entityComponentIdPair, &EditorWhiteBoxTransformModeRequests::SetSelectedVertices, linked);
        }
        else
        {
            return { false, "Select something to grow from first." };
        }
        if (after <= before)
        {
            return { false, "Everything joined to the selection is already selected." };
        }
        return { true, AZStd::string::format("%zu %s selected.", after, noun) };
    }

    Result SelectCoplanar(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        EditorWhiteBoxComponent* component = EditableComponentFor(entityComponentIdPair);
        if (component == nullptr)
        {
            return { false, "No editable White Box mesh." };
        }
        const auto seeds = SelectedPolygons(entityComponentIdPair);
        if (seeds.empty())
        {
            return { false, "Select a polygon to grow from first." };
        }
        const auto region = Api::FindCoplanarRegion(*component->GetWhiteBoxMesh(), seeds);
        if (region.size() <= seeds.size())
        {
            return { false, "Nothing further to add - the neighbours face a different way." };
        }
        EditorWhiteBoxTransformModeRequestBus::Event(
            entityComponentIdPair, &EditorWhiteBoxTransformModeRequests::SetSelectedPolygons, region);
        return { true, AZStd::string::format("%zu polygons selected.", region.size()) };
    }

    Result MergePolygons(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        EditorWhiteBoxComponent* component = EditableComponentFor(entityComponentIdPair);
        if (component == nullptr)
        {
            return { false, "No editable White Box mesh." };
        }

        const Api::PolygonHandles polygons = SelectedPolygons(entityComponentIdPair);

        AZStd::string error;
        Api::PolygonHandle merged;
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("White Box Merge Polygons");
            if (!Api::MergePolygons(*component->GetWhiteBoxMesh(), polygons, error, &merged))
            {
                return { false, error };
            }
            CommitMeshEdit(*component, entityComponentIdPair);
            EditorWhiteBoxTransformModeRequestBus::Event(
                entityComponentIdPair, &EditorWhiteBoxTransformModeRequests::SetSelectedPolygons,
                Api::PolygonHandles{ merged });
            undoBatch.MarkEntityDirty(entityComponentIdPair.GetEntityId());
        }
        return { true, AZStd::string::format("%zu polygons merged into one.", polygons.size()) };
    }

    Result DeletePolygon(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        EditorWhiteBoxComponent* component = EditableComponentFor(entityComponentIdPair);
        if (component == nullptr)
        {
            return { false, "No editable White Box mesh." };
        }

        const Api::PolygonHandles polygons = SelectedPolygons(entityComponentIdPair);

        AZStd::string error;
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("White Box Delete Polygon");
            if (!Api::DeletePolygons(*component->GetWhiteBoxMesh(), polygons, error))
            {
                return { false, error };
            }
            CommitMeshEdit(*component, entityComponentIdPair);
            undoBatch.MarkEntityDirty(entityComponentIdPair.GetEntityId());
        }
        return { true, AZStd::string::format(
            "%zu polygon%s deleted, vertices kept. Undo restores the faces.",
            polygons.size(), polygons.size() == 1 ? "" : "s") };
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

    bool CanGrowShrink(const Selection& selection)
    {
        return CanSelectLinked(selection);
    }

    bool CanDetach(const Selection& selection)
    {
        return selection.m_editable && !selection.m_polygons.empty();
    }

    bool CanConnectVertices(const Selection& selection)
    {
        return selection.m_editable && selection.m_vertices.size() >= 2;
    }

    bool CanProjectUvs(const Selection& selection)
    {
        return selection.m_editable && !selection.m_polygons.empty();
    }

    bool CanSubdivide(const Selection& selection)
    {
        return selection.m_editable && !selection.m_polygons.empty();
    }

    bool CanSelectSimilar(const Selection& selection)
    {
        return CanSelectLinked(selection);
    }

    // Grow and shrink share everything but the step, so both run through here.
    static Result StepSelection(const AZ::EntityComponentIdPair& pair, const bool grow)
    {
        EditorWhiteBoxComponent* component = EditableComponentFor(pair);
        if (component == nullptr)
        {
            return { false, "No editable White Box mesh." };
        }
        const WhiteBoxMesh& mesh = *component->GetWhiteBoxMesh();
        const auto polygons = SelectedPolygons(pair);
        const auto edges = SelectedEdges(pair);
        const auto vertices = SelectedVertices(pair);
        size_t before = 0;
        size_t after = 0;
        const char* noun = "";
        if (!polygons.empty())
        {
            const auto stepped = grow ? Api::GrowPolygonSelection(mesh, polygons) : Api::ShrinkPolygonSelection(mesh, polygons);
            before = polygons.size();
            after = stepped.size();
            noun = "polygons";
            if (after != before && after > 0)
            {
                EditorWhiteBoxTransformModeRequestBus::Event(pair, &EditorWhiteBoxTransformModeRequests::SetSelectedPolygons, stepped);
            }
        }
        else if (!edges.empty())
        {
            const auto stepped = grow ? Api::GrowEdgeSelection(mesh, edges) : Api::ShrinkEdgeSelection(mesh, edges);
            before = edges.size();
            after = stepped.size();
            noun = "edges";
            if (after != before && after > 0)
            {
                EditorWhiteBoxTransformModeRequestBus::Event(pair, &EditorWhiteBoxTransformModeRequests::SetSelectedEdges, stepped);
            }
        }
        else if (!vertices.empty())
        {
            const auto stepped = grow ? Api::GrowVertexSelection(mesh, vertices) : Api::ShrinkVertexSelection(mesh, vertices);
            before = vertices.size();
            after = stepped.size();
            noun = "vertices";
            if (after != before && after > 0)
            {
                EditorWhiteBoxTransformModeRequestBus::Event(pair, &EditorWhiteBoxTransformModeRequests::SetSelectedVertices, stepped);
            }
        }
        else
        {
            return { false, "Select something to grow or shrink first." };
        }
        if (grow && after <= before)
        {
            return { false, "Nothing further to add - everything next to the selection is already selected." };
        }
        if (!grow && after == 0)
        {
            return { false, "Shrinking would empty the selection, so it was kept." };
        }
        if (!grow && after == before)
        {
            return { false, "Nothing to drop - no selected element touches an unselected one." };
        }
        return { true, AZStd::string::format("%zu %s selected.", after, noun) };
    }

    Result GrowSelection(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        return StepSelection(entityComponentIdPair, true);
    }

    Result ShrinkSelection(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        return StepSelection(entityComponentIdPair, false);
    }

    Result ConvertSelection(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const Api::SelectionElement target, const bool touching)
    {
        EditorWhiteBoxComponent* component = EditableComponentFor(entityComponentIdPair);
        if (component == nullptr)
        {
            return { false, "No editable White Box mesh." };
        }
        Api::ElementSelection source;
        source.m_polygons = SelectedPolygons(entityComponentIdPair);
        source.m_edges = SelectedEdges(entityComponentIdPair);
        source.m_vertices = SelectedVertices(entityComponentIdPair);
        const auto countOf = [target](const Api::ElementSelection& selection)
        {
            return target == Api::SelectionElement::Polygon ? selection.m_polygons.size()
                : target == Api::SelectionElement::Edge     ? selection.m_edges.size()
                                                            : selection.m_vertices.size();
        };
        // Nothing selected, or already the wanted type: the filter change alone is the whole job.
        if ((source.m_polygons.empty() && source.m_edges.empty() && source.m_vertices.empty()) || countOf(source) > 0)
        {
            return { true, {} };
        }
        const WhiteBoxMesh& mesh = *component->GetWhiteBoxMesh();
        auto converted = Api::ConvertSelection(mesh, source, target, touching);
        // Nothing enclosed (one vertex into faces, say): take what it touches rather than drop the selection.
        if (countOf(converted) == 0 && !touching)
        {
            converted = Api::ConvertSelection(mesh, source, target, true);
        }
        if (target == Api::SelectionElement::Polygon)
        {
            EditorWhiteBoxTransformModeRequestBus::Event(
                entityComponentIdPair, &EditorWhiteBoxTransformModeRequests::SetSelectedPolygons, converted.m_polygons);
        }
        else if (target == Api::SelectionElement::Edge)
        {
            EditorWhiteBoxTransformModeRequestBus::Event(
                entityComponentIdPair, &EditorWhiteBoxTransformModeRequests::SetSelectedEdges, converted.m_edges);
        }
        else
        {
            EditorWhiteBoxTransformModeRequestBus::Event(
                entityComponentIdPair, &EditorWhiteBoxTransformModeRequests::SetSelectedVertices, converted.m_vertices);
        }
        return { true, AZStd::string::format("%zu selected after conversion.", countOf(converted)) };
    }

    Result DetachToLayer(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        EditorWhiteBoxComponent* component = EditableComponentFor(entityComponentIdPair);
        if (component == nullptr)
        {
            return { false, "No editable White Box mesh." };
        }
        const Api::PolygonHandles polygons = SelectedPolygons(entityComponentIdPair);

        AZStd::string error;
        size_t detached = 0;
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("White Box Detach to Layer");
            // Success refreshes component mode, which drops the old handles; failure leaves the selection alone.
            if (!component->DetachPolygonsToLayer(polygons, error))
            {
                return { false, error };
            }
            EditorWhiteBoxComponentNotificationBus::Event(
                entityComponentIdPair, &EditorWhiteBoxComponentNotifications::OnWhiteBoxMeshModified);
            // The new layer is active and holds exactly what was detached, so select all of it.
            const Api::PolygonHandles moved = Api::MeshPolygonHandles(*component->GetWhiteBoxMesh());
            detached = moved.size();
            EditorWhiteBoxTransformModeRequestBus::Event(
                entityComponentIdPair, &EditorWhiteBoxTransformModeRequests::SetSelectedPolygons, moved);
            undoBatch.MarkEntityDirty(entityComponentIdPair.GetEntityId());
        }
        return { true, AZStd::string::format(
            "%zu polygon%s moved to a new layer. Undo puts them back.", detached, detached == 1 ? "" : "s") };
    }

    Result ConnectVertices(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        EditorWhiteBoxComponent* component = EditableComponentFor(entityComponentIdPair);
        if (component == nullptr)
        {
            return { false, "No editable White Box mesh." };
        }
        const Api::VertexHandles vertices = SelectedVertices(entityComponentIdPair);

        AZStd::string error;
        Api::EdgeHandles created;
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("White Box Connect Vertices");
            if (!Api::ConnectVertices(*component->GetWhiteBoxMesh(), vertices, error, &created))
            {
                return { false, error };
            }
            CommitMeshEdit(*component, entityComponentIdPair);
            // The new edges are selected so they can be slid or beveled straight away.
            EditorWhiteBoxTransformModeRequestBus::Event(
                entityComponentIdPair, &EditorWhiteBoxTransformModeRequests::SetSelectedEdges, created);
            undoBatch.MarkEntityDirty(entityComponentIdPair.GetEntityId());
        }
        return { true, AZStd::string::format("%zu edge%s added.", created.size(), created.size() == 1 ? "" : "s") };
    }

    Result BeginInsertVertex(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        if (EditableComponentFor(entityComponentIdPair) == nullptr)
        {
            return { false, "No editable White Box mesh." };
        }
        EditorWhiteBoxTransformModeRequestBus::Event(entityComponentIdPair, &EditorWhiteBoxTransformModeRequests::BeginInsertVertex);
        return { true, "Click polygon edges to add vertices. Ctrl: midpoint. Shift: tenths. Esc / right-click: finish." };
    }

    // The selected polygons that still exist; a stale handle would otherwise index a face that moved.
    static Api::PolygonHandles LivePolygons(const WhiteBoxMesh& mesh, const Api::PolygonHandles& polygons)
    {
        const Api::PolygonHandles all = Api::MeshPolygonHandles(mesh);
        Api::PolygonHandles live;
        for (const auto& polygon : polygons)
        {
            if (AZStd::find(all.begin(), all.end(), polygon) != all.end())
            {
                live.push_back(polygon);
            }
        }
        return live;
    }

    AZStd::optional<Api::UvProjection> SelectedUvProjection(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        EditorWhiteBoxComponent* component = EditableComponentFor(entityComponentIdPair);
        if (component == nullptr)
        {
            return AZStd::nullopt;
        }
        const auto polygons = LivePolygons(*component->GetWhiteBoxMesh(), SelectedPolygons(entityComponentIdPair));
        if (polygons.empty())
        {
            return AZStd::nullopt;
        }
        return Api::FaceUvProjection(*component->GetWhiteBoxMesh(), polygons.front().m_faceHandles.front());
    }

    // Shared by the UV and smoothing edits: the polygons keep their handles, so the selection survives the edit.
    template<typename Edit>
    static Result EditSelectedPolygons(const AZ::EntityComponentIdPair& pair, const char* undoName, const char* what, Edit&& edit)
    {
        EditorWhiteBoxComponent* component = EditableComponentFor(pair);
        if (component == nullptr)
        {
            return { false, "No editable White Box mesh." };
        }
        const auto polygons = LivePolygons(*component->GetWhiteBoxMesh(), SelectedPolygons(pair));
        if (polygons.empty())
        {
            return { false, "Select one or more polygons in Transform mode." };
        }
        {
            AzToolsFramework::ScopedUndoBatch undoBatch(undoName);
            edit(*component->GetWhiteBoxMesh(), polygons);
            // A parametric layer would regenerate over the stored attributes, so it is frozen like any face edit.
            component->BakeParametricLayer(component->GetActiveLayerIndex());
            component->SerializeWhiteBox();
            EditorWhiteBoxComponentNotificationBus::Event(pair, &EditorWhiteBoxComponentNotifications::OnWhiteBoxMeshModified);
            undoBatch.MarkEntityDirty(pair.GetEntityId());
        }
        return { true, AZStd::string::format("%s updated on %zu polygon%s.", what, polygons.size(), polygons.size() == 1 ? "" : "s") };
    }

    Result ApplyUvProjection(const AZ::EntityComponentIdPair& entityComponentIdPair, const Api::UvProjection& projection)
    {
        return EditSelectedPolygons(
            entityComponentIdPair, "White Box UV Projection", "UVs",
            [&projection](WhiteBoxMesh& mesh, const Api::PolygonHandles& polygons)
            {
                Api::SetPolygonUvProjection(mesh, polygons, projection);
            });
    }

    Result FitUvProjection(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        return EditSelectedPolygons(
            entityComponentIdPair, "White Box Fit UVs", "UVs",
            [](WhiteBoxMesh& mesh, const Api::PolygonHandles& polygons)
            {
                Api::FitPolygonUvProjection(mesh, polygons);
            });
    }

    AZStd::optional<float> SelectedTexelDensity(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        EditorWhiteBoxComponent* component = EditableComponentFor(entityComponentIdPair);
        if (component == nullptr)
        {
            return AZStd::nullopt;
        }
        const auto polygons = LivePolygons(*component->GetWhiteBoxMesh(), SelectedPolygons(entityComponentIdPair));
        if (polygons.empty())
        {
            return AZStd::nullopt;
        }
        return Api::PolygonTexelDensity(*component->GetWhiteBoxMesh(), polygons);
    }

    Result NormalizeTexelDensity(const AZ::EntityComponentIdPair& entityComponentIdPair, const float repeatsPerMetre)
    {
        return EditSelectedPolygons(
            entityComponentIdPair, "White Box Normalize Texel Density", "Texel density",
            [repeatsPerMetre](WhiteBoxMesh& mesh, const Api::PolygonHandles& polygons)
            {
                Api::NormalizePolygonTexelDensity(mesh, polygons, repeatsPerMetre);
            });
    }

    // One clipboard for the editor session, so a projection can move between entities.
    static AZStd::optional<Api::UvProjection>& UvClipboard()
    {
        static AZStd::optional<Api::UvProjection> clipboard;
        return clipboard;
    }

    Result CopyUvProjection(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        const auto projection = SelectedUvProjection(entityComponentIdPair);
        if (!projection)
        {
            return { false, "Select a polygon to copy its UV projection from." };
        }
        UvClipboard() = projection;
        return { true, "UV projection copied." };
    }

    Result PasteUvProjection(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        if (!UvClipboard())
        {
            return { false, "Copy a UV projection first." };
        }
        return ApplyUvProjection(entityComponentIdPair, *UvClipboard());
    }

    bool HasCopiedUvProjection()
    {
        return UvClipboard().has_value();
    }

    Result Subdivide(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        EditorWhiteBoxComponent* component = EditableComponentFor(entityComponentIdPair);
        if (component == nullptr)
        {
            return { false, "No editable White Box mesh." };
        }
        const Api::PolygonHandles polygons = LivePolygons(*component->GetWhiteBoxMesh(), SelectedPolygons(entityComponentIdPair));

        AZStd::string error;
        Api::PolygonHandles created;
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("White Box Subdivide");
            if (!Api::SubdividePolygons(*component->GetWhiteBoxMesh(), polygons, error, &created))
            {
                return { false, error };
            }
            CommitMeshEdit(*component, entityComponentIdPair);
            // The new quads are selected so another click subdivides again.
            EditorWhiteBoxTransformModeRequestBus::Event(
                entityComponentIdPair, &EditorWhiteBoxTransformModeRequests::SetSelectedPolygons, created);
            undoBatch.MarkEntityDirty(entityComponentIdPair.GetEntityId());
        }
        return { true, AZStd::string::format(
            "%zu polygon%s subdivided into %zu.", polygons.size(), polygons.size() == 1 ? "" : "s", created.size()) };
    }

    Result SelectSimilar(const AZ::EntityComponentIdPair& entityComponentIdPair, const Api::SimilarBy similarBy)
    {
        EditorWhiteBoxComponent* component = EditableComponentFor(entityComponentIdPair);
        if (component == nullptr)
        {
            return { false, "No editable White Box mesh." };
        }
        const WhiteBoxMesh& mesh = *component->GetWhiteBoxMesh();
        const auto polygons = SelectedPolygons(entityComponentIdPair);
        const auto edges = SelectedEdges(entityComponentIdPair);
        const auto vertices = SelectedVertices(entityComponentIdPair);
        size_t before = 0;
        size_t after = 0;
        const char* noun = "";
        if (!polygons.empty())
        {
            const auto similar = Api::FindSimilarPolygons(mesh, polygons, similarBy);
            before = polygons.size();
            after = similar.size();
            noun = "polygons";
            EditorWhiteBoxTransformModeRequestBus::Event(
                entityComponentIdPair, &EditorWhiteBoxTransformModeRequests::SetSelectedPolygons, similar);
        }
        else if (!edges.empty())
        {
            const auto similar = Api::FindSimilarEdges(mesh, edges);
            before = edges.size();
            after = similar.size();
            noun = "edges";
            EditorWhiteBoxTransformModeRequestBus::Event(
                entityComponentIdPair, &EditorWhiteBoxTransformModeRequests::SetSelectedEdges, similar);
        }
        else if (!vertices.empty())
        {
            const auto similar = Api::FindSimilarVertices(mesh, vertices);
            before = vertices.size();
            after = similar.size();
            noun = "vertices";
            EditorWhiteBoxTransformModeRequestBus::Event(
                entityComponentIdPair, &EditorWhiteBoxTransformModeRequests::SetSelectedVertices, similar);
        }
        else
        {
            return { false, "Select something to match first." };
        }
        if (after <= before)
        {
            return { false, "Nothing else matches the selection." };
        }
        return { true, AZStd::string::format("%zu %s selected.", after, noun) };
    }

    AZStd::optional<SmoothingState> SelectedSmoothingGroups(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        EditorWhiteBoxComponent* component = EditableComponentFor(entityComponentIdPair);
        if (component == nullptr)
        {
            return AZStd::nullopt;
        }
        const WhiteBoxMesh& mesh = *component->GetWhiteBoxMesh();
        const auto polygons = LivePolygons(mesh, SelectedPolygons(entityComponentIdPair));
        if (polygons.empty())
        {
            return AZStd::nullopt;
        }
        SmoothingState state{ ~0u, 0u };
        for (const auto& polygon : polygons)
        {
            for (const auto face : polygon.m_faceHandles)
            {
                const AZ::u32 groups = Api::FaceSmoothingGroups(mesh, face);
                state.m_all &= groups;
                state.m_any |= groups;
            }
        }
        return state;
    }

    Result EditSmoothingGroups(const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::u32 groups, const Api::SmoothingEdit edit)
    {
        return EditSelectedPolygons(
            entityComponentIdPair, "White Box Smoothing Groups", "Smoothing",
            [groups, edit](WhiteBoxMesh& mesh, const Api::PolygonHandles& polygons)
            {
                Api::SetPolygonSmoothingGroups(mesh, polygons, groups, edit);
            });
    }

    Result AutoSmooth(const AZ::EntityComponentIdPair& entityComponentIdPair, const float angleDegrees)
    {
        EditorWhiteBoxComponent* component = EditableComponentFor(entityComponentIdPair);
        if (component == nullptr)
        {
            return { false, "No editable White Box mesh." };
        }
        WhiteBoxMesh& mesh = *component->GetWhiteBoxMesh();
        Api::PolygonHandles polygons = LivePolygons(mesh, SelectedPolygons(entityComponentIdPair));
        const bool whole = polygons.empty();
        if (whole)
        {
            polygons = Api::MeshPolygonHandles(mesh);
        }
        if (polygons.empty())
        {
            return { false, "The layer has no polygons to smooth." };
        }
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("White Box Auto Smooth");
            Api::AutoSmoothPolygons(mesh, polygons, angleDegrees);
            component->BakeParametricLayer(component->GetActiveLayerIndex());
            component->SerializeWhiteBox();
            EditorWhiteBoxComponentNotificationBus::Event(
                entityComponentIdPair, &EditorWhiteBoxComponentNotifications::OnWhiteBoxMeshModified);
            undoBatch.MarkEntityDirty(entityComponentIdPair.GetEntityId());
        }
        return { true, AZStd::string::format(
            "Auto smoothed %s at %.0f degrees.", whole ? "the whole layer" : "the selection", angleDegrees) };
    }
} // namespace WhiteBox::ModelingOps
