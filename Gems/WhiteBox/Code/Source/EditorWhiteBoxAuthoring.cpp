/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorWhiteBoxAuthoring.h"

#include "EditorWhiteBoxComponent.h"
#include "EditorWhiteBoxComponentModeBus.h"
#include "SubComponentModes/EditorWhiteBoxTransformModeBus.h"

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/writer.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/sort.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

namespace WhiteBox
{
    namespace
    {
        constexpr int ShapeTypeCount = static_cast<int>(DrawShapeType::Polygon) + 1;

        AuthoringResult Succeeded(AZStd::string message = {}, AZStd::vector<int> ids = {})
        {
            AuthoringResult result;
            result.m_success = true;
            result.m_message = AZStd::move(message);
            result.m_ids = AZStd::move(ids);
            return result;
        }

        AuthoringResult Failed(AZStd::string message)
        {
            AuthoringResult result;
            result.m_message = AZStd::move(message);
            return result;
        }

        //! A polygon is named by the smallest face id it contains, which does not depend on how the handle orders its faces.
        int PolygonId(const Api::PolygonHandle& polygon)
        {
            int id = -1;
            for (const Api::FaceHandle face : polygon.m_faceHandles)
            {
                if (id < 0 || face.Index() < id)
                {
                    id = face.Index();
                }
            }
            return id;
        }

        AZStd::vector<int> PolygonIds(const Api::PolygonHandles& polygons)
        {
            AZStd::vector<int> ids;
            ids.reserve(polygons.size());
            for (const Api::PolygonHandle& polygon : polygons)
            {
                ids.push_back(PolygonId(polygon));
            }
            return ids;
        }

        //! Ids to polygon handles, dropping repeats. With @p emptyMeansAll an empty list selects every polygon.
        bool ResolvePolygons(
            const WhiteBoxMesh& mesh, const AZStd::vector<int>& ids, const bool emptyMeansAll, Api::PolygonHandles& out,
            AZStd::string& error)
        {
            out.clear();
            if (ids.empty())
            {
                if (emptyMeansAll)
                {
                    out = Api::MeshPolygonHandles(mesh);
                    return true;
                }
                error = "No polygon ids were given.";
                return false;
            }
            AZStd::unordered_set<int> seen;
            for (const int id : ids)
            {
                Api::PolygonHandle polygon = Api::FacePolygonHandle(mesh, Api::FaceHandle(id));
                if (polygon.m_faceHandles.empty())
                {
                    error = AZStd::string::format(
                        "Polygon id %d does not exist; ids change after topology edits, so query GetPolygons again.", id);
                    return false;
                }
                if (seen.insert(PolygonId(polygon)).second)
                {
                    out.push_back(AZStd::move(polygon));
                }
            }
            return true;
        }

        bool ResolveEdges(const WhiteBoxMesh& mesh, const AZStd::vector<int>& ids, Api::EdgeHandles& out, AZStd::string& error)
        {
            out.clear();
            if (ids.empty())
            {
                error = "No edge ids were given.";
                return false;
            }
            AZStd::unordered_set<int> valid;
            for (const Api::EdgeHandle edge : Api::MeshEdgeHandles(mesh))
            {
                valid.insert(edge.Index());
            }
            AZStd::unordered_set<int> seen;
            for (const int id : ids)
            {
                if (valid.find(id) == valid.end())
                {
                    error = AZStd::string::format("Edge id %d does not exist; query GetEdges again after an edit.", id);
                    return false;
                }
                if (seen.insert(id).second)
                {
                    out.push_back(Api::EdgeHandle(id));
                }
            }
            return true;
        }

        bool ResolveVertices(
            const WhiteBoxMesh& mesh, const AZStd::vector<int>& ids, Api::VertexHandles& out, AZStd::string& error)
        {
            out.clear();
            if (ids.empty())
            {
                error = "No vertex ids were given.";
                return false;
            }
            AZStd::unordered_set<int> valid;
            for (const Api::VertexHandle vertex : Api::MeshVertexHandles(mesh))
            {
                valid.insert(vertex.Index());
            }
            AZStd::unordered_set<int> seen;
            for (const int id : ids)
            {
                if (valid.find(id) == valid.end())
                {
                    error = AZStd::string::format("Vertex id %d does not exist; query GetVertexIds again after an edit.", id);
                    return false;
                }
                if (seen.insert(id).second)
                {
                    out.push_back(Api::VertexHandle(id));
                }
            }
            return true;
        }

        //! Product path to asset id; invalid for an empty path. A source `.material` path is accepted for its product.
        bool ResolveMaterial(const AZStd::string& path, AZ::Data::AssetId& id, AZStd::string& error)
        {
            id = AZ::Data::AssetId();
            if (path.empty())
            {
                return true;
            }
            const auto lookup = [&id](const AZStd::string& assetPath)
            {
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    id, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, assetPath.c_str(), AZ::Data::s_invalidAssetType,
                    false);
            };
            lookup(path);
            const AZStd::string_view source = ".material";
            if (!id.IsValid() && path.size() > source.size() &&
                AZStd::string_view(path).substr(path.size() - source.size()) == source)
            {
                lookup(path.substr(0, path.size() - source.size()) + ".azmaterial");
            }
            if (!id.IsValid())
            {
                error = AZStd::string::format("Material '%s' is not in the asset catalog.", path.c_str());
                return false;
            }
            return true;
        }

        AZStd::string MaterialPathOf(const AZ::Data::AssetId& id)
        {
            AZStd::string path;
            if (id.IsValid())
            {
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(path, &AZ::Data::AssetCatalogRequests::GetAssetPathById, id);
            }
            return path;
        }

        //! Packed RGBA the face paint stores (R in the low byte); an alpha of zero is no paint.
        AZ::u32 PackPaint(const AZ::Color& color)
        {
            const auto channel = [](const float value)
            {
                return static_cast<AZ::u32>(AZ::GetClamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
            };
            if (channel(color.GetA()) == 0)
            {
                return 0;
            }
            return channel(color.GetR()) | (channel(color.GetG()) << 8) | (channel(color.GetB()) << 16) |
                (channel(color.GetA()) << 24);
        }

        bool ValidShape(const int shape)
        {
            return shape >= 0 && shape < ShapeTypeCount;
        }

        using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

        void WriteVector(JsonWriter& writer, const AZ::Vector3& value)
        {
            writer.StartArray();
            writer.Double(value.GetX());
            writer.Double(value.GetY());
            writer.Double(value.GetZ());
            writer.EndArray();
        }

        void WriteBounds(JsonWriter& writer, const AZStd::vector<AZ::Vector3>& positions)
        {
            if (positions.empty())
            {
                writer.Null();
                return;
            }
            AZ::Vector3 min = positions.front();
            AZ::Vector3 max = positions.front();
            for (const AZ::Vector3& position : positions)
            {
                min = min.GetMin(position);
                max = max.GetMax(position);
            }
            writer.StartObject();
            writer.Key("min");
            WriteVector(writer, min);
            writer.Key("max");
            WriteVector(writer, max);
            writer.Key("size");
            WriteVector(writer, max - min);
            writer.EndObject();
        }
    } // namespace

    EditorWhiteBoxAuthoring::EditorWhiteBoxAuthoring(EditorWhiteBoxComponent& component)
        : m_component(component)
    {
        EditorWhiteBoxAuthoringRequestBus::Handler::BusConnect(m_component.GetEntityId());
    }

    EditorWhiteBoxAuthoring::~EditorWhiteBoxAuthoring()
    {
        EditorWhiteBoxAuthoringRequestBus::Handler::BusDisconnect();
    }

    WhiteBoxMesh* EditorWhiteBoxAuthoring::EditableMesh() const
    {
        return m_component.GetWhiteBoxMesh();
    }

    void EditorWhiteBoxAuthoring::RefreshPane()
    {
        EditorWhiteBoxComponentNotificationBus::Event(
            AZ::EntityComponentIdPair{m_component.GetEntityId(), m_component.GetId()},
            &EditorWhiteBoxComponentNotifications::OnLayerStructureChanged);
    }

    void EditorWhiteBoxAuthoring::CommitMeshEdit()
    {
        const AZ::EntityComponentIdPair pair{m_component.GetEntityId(), m_component.GetId()};
        // The edit-mode selection holds handles into a mesh that was just rewritten.
        EditorWhiteBoxTransformModeRequestBus::Event(pair, &EditorWhiteBoxTransformModeRequests::ClearSelection);
        // Explicit geometry edits freeze a parametric layer, or the next parameter change would regenerate over them.
        m_component.BakeParametricLayer(m_component.GetActiveLayerIndex());
        m_component.SerializeWhiteBox();
        EditorWhiteBoxComponentModeRequestBus::Event(
            pair, &EditorWhiteBoxComponentModeRequests::MarkWhiteBoxIntersectionDataDirty);
        EditorWhiteBoxComponentNotificationBus::Event(
            pair, &EditorWhiteBoxComponentNotifications::OnWhiteBoxMeshModified);
        RefreshPane();
    }

    AuthoringResult EditorWhiteBoxAuthoring::EditMesh(
        const char* undoLabel, const AZStd::function<AuthoringResult(WhiteBoxMesh&)>& edit)
    {
        WhiteBoxMesh* mesh = EditableMesh();
        if (mesh == nullptr)
        {
            return Failed("No editable White Box mesh.");
        }
        AzToolsFramework::ScopedUndoBatch undoBatch(undoLabel);
        AuthoringResult result = edit(*mesh);
        if (result.m_success)
        {
            CommitMeshEdit();
            undoBatch.MarkEntityDirty(m_component.GetEntityId());
        }
        return result;
    }

    // ---- Overview ---------------------------------------------------------------------------------------------------

    AZStd::string EditorWhiteBoxAuthoring::Describe()
    {
        rapidjson::StringBuffer buffer;
        JsonWriter writer(buffer);
        writer.StartObject();

        writer.Key("entityId");
        writer.Uint64(static_cast<AZ::u64>(m_component.GetEntityId()));
        writer.Key("activeLayer");
        writer.Int(m_component.GetActiveLayerIndex());

        writer.Key("layers");
        writer.StartArray();
        for (int index = 0; index < m_component.GetLayerCount(); ++index)
        {
            const AuthoringLayerSettings layer = GetLayerSettings(index);
            writer.StartObject();
            writer.Key("index");
            writer.Int(index);
            writer.Key("name");
            writer.String(layer.m_name.c_str());
            writer.Key("parametric");
            writer.Bool(m_component.IsLayerParametric(index));
            writer.Key("visible");
            writer.Bool(layer.m_visible);
            writer.Key("collision");
            writer.Bool(layer.m_collision);
            writer.Key("combineMode");
            writer.Int(layer.m_combineMode);
            writer.Key("position");
            WriteVector(writer, layer.m_position);
            writer.Key("rotation");
            WriteVector(writer, layer.m_rotation);
            writer.Key("scale");
            WriteVector(writer, layer.m_scale);
            if (m_component.IsLayerParametric(index))
            {
                const AuthoringShapeParams shape = GetShapeParams(index);
                writer.Key("shape");
                writer.StartObject();
                writer.Key("type");
                writer.Int(shape.m_shape);
                writer.Key("width");
                writer.Double(shape.m_width);
                writer.Key("depth");
                writer.Double(shape.m_depth);
                writer.Key("height");
                writer.Double(shape.m_height);
                writer.Key("sides");
                writer.Int(shape.m_sides);
                writer.EndObject();
            }
            writer.EndObject();
        }
        writer.EndArray();

        writer.Key("mesh");
        writer.StartObject();
        if (WhiteBoxMesh* mesh = EditableMesh())
        {
            writer.Key("vertices");
            writer.Uint64(Api::MeshVertexCount(*mesh));
            writer.Key("faces");
            writer.Uint64(Api::MeshFaceCount(*mesh));
            writer.Key("polygons");
            writer.Uint64(Api::MeshPolygonHandles(*mesh).size());
            writer.Key("bounds");
            WriteBounds(writer, Api::MeshVertexPositions(*mesh));
        }
        writer.EndObject();

        writer.Key("evaluated");
        writer.StartObject();
        if (WhiteBoxMesh* evaluated = m_component.GetEvaluatedWhiteBoxMesh())
        {
            writer.Key("vertices");
            writer.Uint64(Api::MeshVertexCount(*evaluated));
            writer.Key("faces");
            writer.Uint64(Api::MeshFaceCount(*evaluated));
            writer.Key("bounds");
            WriteBounds(writer, Api::MeshVertexPositions(*evaluated));
        }
        writer.EndObject();

        const AuthoringDisplaySettings display = GetDisplaySettings();
        writer.Key("display");
        writer.StartObject();
        writer.Key("edgesOnly");
        writer.Bool(display.m_edgesOnly);
        writer.Key("useGlobalTint");
        writer.Bool(display.m_useGlobalTint);
        writer.Key("tint");
        writer.StartArray();
        writer.Double(display.m_tint.GetR());
        writer.Double(display.m_tint.GetG());
        writer.Double(display.m_tint.GetB());
        writer.Double(display.m_tint.GetA());
        writer.EndArray();
        writer.Key("useTexture");
        writer.Bool(display.m_useTexture);
        writer.Key("material");
        writer.String(display.m_materialPath.c_str());
        writer.Key("csgSolver");
        writer.Int(display.m_csgSolver);
        writer.EndObject();

        const AuthoringBooleanSettings boolean = GetBooleanSettings();
        writer.Key("boolean");
        writer.StartObject();
        writer.Key("sourceEntity");
        writer.Uint64(static_cast<AZ::u64>(boolean.m_sourceEntity));
        writer.Key("operation");
        writer.Int(boolean.m_operation);
        writer.Key("live");
        writer.Bool(boolean.m_live);
        writer.Key("affectActiveOnly");
        writer.Bool(boolean.m_affectActiveOnly);
        writer.Key("sourceAfterApply");
        writer.Int(boolean.m_sourceAfterApply);
        writer.Key("excludeFromBoolean");
        writer.Bool(boolean.m_excludeFromBoolean);
        writer.Key("booleanOthers");
        writer.Bool(boolean.m_booleanOthers);
        writer.Key("cutterOperation");
        writer.Int(boolean.m_cutterOperation);
        writer.EndObject();

        const AuthoringDrawSettings draw = GetDrawSettings();
        writer.Key("draw");
        writer.StartObject();
        writer.Key("shape");
        writer.Int(draw.m_shape);
        writer.Key("sides");
        writer.Int(draw.m_sides);
        writer.Key("unitCube");
        writer.Bool(draw.m_unitCube);
        writer.Key("unitCubeSize");
        writer.Double(draw.m_unitCubeSize);
        writer.EndObject();

        writer.EndObject();
        return AZStd::string(buffer.GetString(), buffer.GetSize());
    }

    // ---- Layers -----------------------------------------------------------------------------------------------------

    int EditorWhiteBoxAuthoring::GetLayerCount()
    {
        return m_component.GetLayerCount();
    }

    int EditorWhiteBoxAuthoring::GetActiveLayer()
    {
        return m_component.GetActiveLayerIndex();
    }

    bool EditorWhiteBoxAuthoring::SetActiveLayer(const int index)
    {
        if (index < 0 || index >= m_component.GetLayerCount())
        {
            return false;
        }
        m_component.SetActiveLayer(index);
        RefreshPane();
        return true;
    }

    int EditorWhiteBoxAuthoring::AddLayer()
    {
        m_component.AddLayer();
        RefreshPane();
        return m_component.GetActiveLayerIndex();
    }

    int EditorWhiteBoxAuthoring::AddShapeLayer(const int shape)
    {
        if (!ValidShape(shape))
        {
            AZ_Warning("WhiteBox", false, "AddShapeLayer: shape %d is not a DrawShapeType.", shape);
            return -1;
        }
        const int index = m_component.AddParametricShapeLayer(static_cast<DrawShapeType>(shape));
        RefreshPane();
        return index;
    }

    bool EditorWhiteBoxAuthoring::DeleteActiveLayer()
    {
        if (m_component.GetLayerCount() == 0)
        {
            return false;
        }
        m_component.DeleteActiveLayer();
        RefreshPane();
        return true;
    }

    int EditorWhiteBoxAuthoring::DuplicateActiveLayer()
    {
        if (m_component.GetLayerCount() == 0)
        {
            return -1;
        }
        m_component.DuplicateActiveLayer();
        RefreshPane();
        return m_component.GetActiveLayerIndex();
    }

    bool EditorWhiteBoxAuthoring::MoveLayer(const int from, const int to)
    {
        const int count = m_component.GetLayerCount();
        if (from < 0 || from >= count || to < 0 || to >= count)
        {
            return false;
        }
        AzToolsFramework::ScopedUndoBatch undoBatch("White Box Reorder Layer");
        m_component.MoveLayer(from, to);
        undoBatch.MarkEntityDirty(m_component.GetEntityId());
        RefreshPane();
        return true;
    }

    AuthoringShapeParams EditorWhiteBoxAuthoring::GetShapeParams(const int layer)
    {
        const EditorWhiteBoxComponent::ShapeParams params = m_component.GetLayerShapeParams(layer);
        AuthoringShapeParams out;
        out.m_shape = static_cast<int>(params.m_shape);
        out.m_width = params.m_width;
        out.m_depth = params.m_depth;
        out.m_height = params.m_height;
        out.m_sides = params.m_sides;
        out.m_steps = params.m_steps;
        out.m_stepsByHeight = params.m_stepsByHeight;
        out.m_stepHeight = params.m_stepHeight;
        out.m_wallThickness = params.m_wallThickness;
        out.m_cavityGap = params.m_cavityGap;
        out.m_floor = params.m_floor;
        out.m_ceiling = params.m_ceiling;
        out.m_doorFrame = params.m_doorFrame;
        out.m_archHeight = params.m_archHeight;
        out.m_innerRadius = params.m_innerRadius;
        out.m_sweepAngle = params.m_sweepAngle;
        out.m_holeRatio = params.m_holeRatio;
        out.m_tubeSides = params.m_tubeSides;
        return out;
    }

    AuthoringResult EditorWhiteBoxAuthoring::SetShapeParams(const int layer, const AuthoringShapeParams& params)
    {
        if (!m_component.IsLayerParametric(layer))
        {
            return Failed(AZStd::string::format(
                "Layer %d is not a parametric shape layer (it was baked, or it is a mesh layer).", layer));
        }
        if (!ValidShape(params.m_shape))
        {
            return Failed(AZStd::string::format("Shape %d is not a DrawShapeType.", params.m_shape));
        }
        EditorWhiteBoxComponent::ShapeParams in;
        in.m_shape = static_cast<DrawShapeType>(params.m_shape);
        in.m_width = params.m_width;
        in.m_depth = params.m_depth;
        in.m_height = params.m_height;
        in.m_sides = params.m_sides;
        in.m_steps = params.m_steps;
        in.m_stepsByHeight = params.m_stepsByHeight;
        in.m_stepHeight = params.m_stepHeight;
        in.m_wallThickness = params.m_wallThickness;
        in.m_cavityGap = params.m_cavityGap;
        in.m_floor = params.m_floor;
        in.m_ceiling = params.m_ceiling;
        in.m_doorFrame = params.m_doorFrame;
        in.m_archHeight = params.m_archHeight;
        in.m_innerRadius = params.m_innerRadius;
        in.m_sweepAngle = params.m_sweepAngle;
        in.m_holeRatio = params.m_holeRatio;
        in.m_tubeSides = params.m_tubeSides;

        AzToolsFramework::ScopedUndoBatch undoBatch("White Box Shape Parameters");
        m_component.SetLayerShapeParams(layer, in);
        undoBatch.MarkEntityDirty(m_component.GetEntityId());
        RefreshPane();
        return Succeeded("Shape rebuilt.");
    }

    AuthoringResult EditorWhiteBoxAuthoring::BakeShapeLayer(const int layer)
    {
        if (!m_component.IsLayerParametric(layer))
        {
            return Failed(AZStd::string::format("Layer %d is not a parametric shape layer.", layer));
        }
        AzToolsFramework::ScopedUndoBatch undoBatch("White Box Bake Shape");
        m_component.BakeParametricLayer(layer);
        undoBatch.MarkEntityDirty(m_component.GetEntityId());
        RefreshPane();
        return Succeeded("Layer baked to an ordinary mesh.");
    }

    AuthoringLayerSettings EditorWhiteBoxAuthoring::GetLayerSettings(const int layer)
    {
        const EditorWhiteBoxComponent::LayerMeta meta = m_component.GetLayerMeta(layer);
        AuthoringLayerSettings out;
        out.m_name = meta.m_name;
        out.m_visible = meta.m_visible;
        out.m_collision = meta.m_collision;
        out.m_tint = meta.m_tint;
        out.m_combineMode = static_cast<int>(meta.m_combineMode);
        out.m_invertNormals = meta.m_invertNormals;
        out.m_edgesOnly = meta.m_edgesOnly;
        out.m_position = meta.m_position;
        out.m_rotation = meta.m_rotation;
        out.m_scale = meta.m_scale;
        out.m_mirrorX = meta.m_mirrorX;
        out.m_mirrorY = meta.m_mirrorY;
        out.m_mirrorZ = meta.m_mirrorZ;
        out.m_arrayCount = meta.m_arrayCount;
        out.m_arrayOffset = meta.m_arrayOffset;
        return out;
    }

    AuthoringResult EditorWhiteBoxAuthoring::SetLayerSettings(const int layer, const AuthoringLayerSettings& settings)
    {
        if (layer < 0 || layer >= m_component.GetLayerCount())
        {
            return Failed(AZStd::string::format("Layer %d does not exist.", layer));
        }
        if (settings.m_combineMode < 0 || settings.m_combineMode > static_cast<int>(LayerCombineMode::Intersect))
        {
            return Failed(AZStd::string::format("Combine mode %d is not a LayerCombineMode.", settings.m_combineMode));
        }
        EditorWhiteBoxComponent::LayerMeta meta;
        meta.m_name = settings.m_name;
        meta.m_visible = settings.m_visible;
        meta.m_collision = settings.m_collision;
        meta.m_tint = settings.m_tint;
        meta.m_combineMode = static_cast<LayerCombineMode>(settings.m_combineMode);
        meta.m_invertNormals = settings.m_invertNormals;
        meta.m_edgesOnly = settings.m_edgesOnly;
        meta.m_position = settings.m_position;
        meta.m_rotation = settings.m_rotation;
        meta.m_scale = settings.m_scale;
        meta.m_mirrorX = settings.m_mirrorX;
        meta.m_mirrorY = settings.m_mirrorY;
        meta.m_mirrorZ = settings.m_mirrorZ;
        meta.m_arrayCount = settings.m_arrayCount;
        meta.m_arrayOffset = settings.m_arrayOffset;

        AzToolsFramework::ScopedUndoBatch undoBatch("White Box Layer Settings");
        m_component.SetLayerMeta(layer, meta);
        undoBatch.MarkEntityDirty(m_component.GetEntityId());
        RefreshPane();
        return Succeeded("Layer settings applied.");
    }

    AuthoringResult EditorWhiteBoxAuthoring::ApplyLayerTransform()
    {
        if (m_component.GetLayerCount() == 0)
        {
            return Failed("There is no active layer.");
        }
        AzToolsFramework::ScopedUndoBatch undoBatch("White Box Apply Layer Transform");
        m_component.ApplyActiveLayerTransform();
        undoBatch.MarkEntityDirty(m_component.GetEntityId());
        RefreshPane();
        return Succeeded("Layer transform baked into the mesh.");
    }

    AuthoringResult EditorWhiteBoxAuthoring::ApplyLayerModifiers()
    {
        if (m_component.GetLayerCount() == 0)
        {
            return Failed("There is no active layer.");
        }
        AzToolsFramework::ScopedUndoBatch undoBatch("White Box Apply Layer Modifiers");
        m_component.ApplyActiveLayerModifiers();
        undoBatch.MarkEntityDirty(m_component.GetEntityId());
        RefreshPane();
        return Succeeded("Mirror and array baked into the mesh.");
    }

    // ---- Mesh queries -----------------------------------------------------------------------------------------------

    AZ::u64 EditorWhiteBoxAuthoring::GetVertexCount()
    {
        const WhiteBoxMesh* mesh = EditableMesh();
        return mesh ? Api::MeshVertexCount(*mesh) : 0;
    }

    AZ::u64 EditorWhiteBoxAuthoring::GetFaceCount()
    {
        const WhiteBoxMesh* mesh = EditableMesh();
        return mesh ? Api::MeshFaceCount(*mesh) : 0;
    }

    AZStd::vector<int> EditorWhiteBoxAuthoring::GetVertexIds()
    {
        AZStd::vector<int> ids;
        if (const WhiteBoxMesh* mesh = EditableMesh())
        {
            for (const Api::VertexHandle vertex : Api::MeshVertexHandles(*mesh))
            {
                ids.push_back(vertex.Index());
            }
        }
        return ids;
    }

    AZStd::vector<AZ::Vector3> EditorWhiteBoxAuthoring::GetVertexPositions()
    {
        AZStd::vector<AZ::Vector3> positions;
        if (const WhiteBoxMesh* mesh = EditableMesh())
        {
            for (const Api::VertexHandle vertex : Api::MeshVertexHandles(*mesh))
            {
                positions.push_back(Api::VertexPosition(*mesh, vertex));
            }
        }
        return positions;
    }

    AZStd::vector<int> EditorWhiteBoxAuthoring::GetFaceIds()
    {
        AZStd::vector<int> ids;
        if (const WhiteBoxMesh* mesh = EditableMesh())
        {
            for (const Api::FaceHandle face : Api::MeshFaceHandles(*mesh))
            {
                ids.push_back(face.Index());
            }
        }
        return ids;
    }

    AZStd::vector<int> EditorWhiteBoxAuthoring::GetFaceVertexIds()
    {
        AZStd::vector<int> ids;
        if (const WhiteBoxMesh* mesh = EditableMesh())
        {
            for (const Api::FaceHandle face : Api::MeshFaceHandles(*mesh))
            {
                for (const Api::VertexHandle vertex : Api::FaceVertexHandles(*mesh, face))
                {
                    ids.push_back(vertex.Index());
                }
            }
        }
        return ids;
    }

    AZStd::vector<AuthoringPolygonInfo> EditorWhiteBoxAuthoring::GetPolygons()
    {
        AZStd::vector<AuthoringPolygonInfo> polygons;
        const WhiteBoxMesh* mesh = EditableMesh();
        if (mesh == nullptr)
        {
            return polygons;
        }
        for (const Api::PolygonHandle& polygon : Api::MeshPolygonHandles(*mesh))
        {
            AuthoringPolygonInfo info;
            info.m_id = PolygonId(polygon);
            info.m_faceCount = static_cast<int>(polygon.m_faceHandles.size());
            info.m_normal = Api::PolygonNormal(*mesh, polygon);
            info.m_center = Api::PolygonMidpoint(*mesh, polygon);

            const AZStd::vector<AZ::Vector3> triangles = Api::PolygonFacesPositions(*mesh, polygon);
            for (size_t corner = 0; corner + 2 < triangles.size(); corner += 3)
            {
                info.m_area += 0.5f * (triangles[corner + 1] - triangles[corner])
                                          .Cross(triangles[corner + 2] - triangles[corner])
                                          .GetLength();
            }

            const Api::VertexHandles border = Api::PolygonBorderVertexHandlesFlattened(*mesh, polygon);
            for (const Api::VertexHandle vertex : border)
            {
                info.m_vertexIds.push_back(vertex.Index());
            }
            info.m_vertexPositions = Api::VertexPositions(*mesh, border);
            polygons.push_back(AZStd::move(info));
        }
        AZStd::sort(
            polygons.begin(), polygons.end(),
            [](const AuthoringPolygonInfo& lhs, const AuthoringPolygonInfo& rhs)
            {
                return lhs.m_id < rhs.m_id;
            });
        return polygons;
    }

    AZStd::vector<AuthoringEdgeInfo> EditorWhiteBoxAuthoring::GetEdges(const bool includeInterior)
    {
        AZStd::vector<AuthoringEdgeInfo> edges;
        const WhiteBoxMesh* mesh = EditableMesh();
        if (mesh == nullptr)
        {
            return edges;
        }
        const auto add = [&](const Api::EdgeHandle edge, const bool interior)
        {
            AuthoringEdgeInfo info;
            info.m_id = edge.Index();
            const auto vertices = Api::EdgeVertexHandles(*mesh, edge);
            info.m_vertexA = vertices[0].Index();
            info.m_vertexB = vertices[1].Index();
            const auto positions = Api::EdgeVertexPositions(*mesh, edge);
            info.m_start = positions[0];
            info.m_end = positions[1];
            info.m_length = (positions[1] - positions[0]).GetLength();
            info.m_boundary = Api::EdgeIsBoundary(*mesh, edge);
            info.m_interior = interior;
            edges.push_back(info);
        };
        const Api::EdgeTypes types = Api::MeshUserEdgeHandles(*mesh);
        for (const Api::EdgeHandle edge : types.m_user)
        {
            add(edge, false);
        }
        if (includeInterior)
        {
            for (const Api::EdgeHandle edge : types.m_mesh)
            {
                add(edge, true);
            }
        }
        return edges;
    }

    // ---- Mesh authoring ---------------------------------------------------------------------------------------------

    AuthoringResult EditorWhiteBoxAuthoring::SetMeshFromTriangles(
        const AZStd::vector<AZ::Vector3>& positions, const AZStd::vector<AZ::u32>& indices)
    {
        if (positions.size() < 3 || indices.size() < 3)
        {
            return Failed("A triangle list needs at least three positions and three indices.");
        }
        return EditMesh(
            "White Box Set Mesh",
            [&](WhiteBoxMesh& mesh)
            {
                if (!Api::BuildFromTriangles(mesh, positions, indices))
                {
                    return Failed("No usable triangles: indices must be in range and not repeat a vertex within a triangle.");
                }
                Api::CalculateNormals(mesh);
                Api::CalculatePlanarUVs(mesh);
                return Succeeded(AZStd::string::format(
                    "Mesh has %llu vertices and %llu faces.", static_cast<unsigned long long>(Api::MeshVertexCount(mesh)),
                    static_cast<unsigned long long>(Api::MeshFaceCount(mesh))));
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::AddPolygon(const AZStd::vector<AZ::Vector3>& outline)
    {
        if (outline.size() < 3)
        {
            return Failed("A polygon needs at least three outline points.");
        }
        return EditMesh(
            "White Box Add Polygon",
            [&](WhiteBoxMesh& mesh)
            {
                Api::VertexHandles vertices;
                for (const AZ::Vector3& point : outline)
                {
                    vertices.push_back(Api::AddVertex(mesh, point));
                }
                Api::FaceVertHandlesList faces;
                for (size_t corner = 1; corner + 1 < vertices.size(); ++corner)
                {
                    faces.push_back(Api::FaceVertHandles{vertices[0], vertices[corner], vertices[corner + 1]});
                }
                const Api::PolygonHandle polygon = Api::AddPolygon(mesh, faces);
                Api::CalculateNormals(mesh);
                Api::CalculatePlanarUVs(mesh);
                return Succeeded("Polygon added.", {PolygonId(polygon)});
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::ClearMesh()
    {
        return EditMesh(
            "White Box Clear Mesh",
            [](WhiteBoxMesh& mesh)
            {
                Api::Clear(mesh);
                return Succeeded("Mesh cleared.");
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::SetVertexPositions(
        const AZStd::vector<int>& vertexIds, const AZStd::vector<AZ::Vector3>& positions)
    {
        if (vertexIds.size() != positions.size())
        {
            return Failed("SetVertexPositions needs one position per vertex id.");
        }
        return EditMesh(
            "White Box Set Vertex Positions",
            [&](WhiteBoxMesh& mesh)
            {
                AZStd::string error;
                Api::VertexHandles vertices;
                if (!ResolveVertices(mesh, vertexIds, vertices, error))
                {
                    return Failed(error);
                }
                // Resolution drops repeated ids, so walk the input to keep each id paired with its own position.
                for (size_t i = 0; i < vertexIds.size(); ++i)
                {
                    Api::SetVertexPosition(mesh, Api::VertexHandle(vertexIds[i]), positions[i]);
                }
                Api::CalculateNormals(mesh);
                Api::CalculatePlanarUVs(mesh);
                return Succeeded(AZStd::string::format("%zu vertices moved.", vertexIds.size()));
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::TranslateVertices(const AZStd::vector<int>& vertexIds, const AZ::Vector3& offset)
    {
        return EditMesh(
            "White Box Translate Vertices",
            [&](WhiteBoxMesh& mesh)
            {
                AZStd::string error;
                Api::VertexHandles vertices;
                if (!ResolveVertices(mesh, vertexIds, vertices, error))
                {
                    return Failed(error);
                }
                for (const Api::VertexHandle vertex : vertices)
                {
                    Api::SetVertexPosition(mesh, vertex, Api::VertexPosition(mesh, vertex) + offset);
                }
                Api::CalculateNormals(mesh);
                Api::CalculatePlanarUVs(mesh);
                return Succeeded(AZStd::string::format("%zu vertices moved.", vertices.size()));
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::ExtrudePolygons(const AZStd::vector<int>& polygonIds, const float distance)
    {
        return EditMesh(
            "White Box Extrude",
            [&](WhiteBoxMesh& mesh)
            {
                AZStd::string error;
                Api::PolygonHandles polygons;
                if (!ResolvePolygons(mesh, polygonIds, false, polygons, error))
                {
                    return Failed(error);
                }
                Api::PolygonHandles result;
                if (!Api::ExtrudeInsetRegions(mesh, polygons, distance, false, result, error))
                {
                    return Failed(error);
                }
                return Succeeded("Region extruded.", PolygonIds(result));
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::InsetPolygons(const AZStd::vector<int>& polygonIds, const float fraction)
    {
        if (!(fraction > 0.0f && fraction < 1.0f))
        {
            return Failed("Inset is a fraction strictly between 0 and 1.");
        }
        return EditMesh(
            "White Box Inset",
            [&](WhiteBoxMesh& mesh)
            {
                AZStd::string error;
                Api::PolygonHandles polygons;
                if (!ResolvePolygons(mesh, polygonIds, false, polygons, error))
                {
                    return Failed(error);
                }
                Api::PolygonHandles result;
                if (!Api::ExtrudeInsetRegions(mesh, polygons, fraction, true, result, error))
                {
                    return Failed(error);
                }
                return Succeeded("Region inset.", PolygonIds(result));
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::TranslatePolygons(const AZStd::vector<int>& polygonIds, const float distance)
    {
        return EditMesh(
            "White Box Translate Polygons",
            [&](WhiteBoxMesh& mesh)
            {
                AZStd::string error;
                Api::PolygonHandles polygons;
                if (!ResolvePolygons(mesh, polygonIds, false, polygons, error))
                {
                    return Failed(error);
                }
                for (const Api::PolygonHandle& polygon : polygons)
                {
                    Api::TranslatePolygon(mesh, polygon, distance);
                }
                return Succeeded(AZStd::string::format("%zu polygons moved.", polygons.size()), PolygonIds(polygons));
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::BevelEdges(
        const AZStd::vector<int>& edgeIds, const float width, const int segments, const float profile)
    {
        if (!(width > 0.0f) || segments < 1 || segments > 32)
        {
            return Failed("Bevel needs a width above zero and 1 to 32 segments.");
        }
        return EditMesh(
            "White Box Bevel",
            [&](WhiteBoxMesh& mesh)
            {
                AZStd::string error;
                Api::EdgeHandles edges;
                if (!ResolveEdges(mesh, edgeIds, edges, error))
                {
                    return Failed(error);
                }
                if (!Api::BevelEdges(mesh, edges, width, segments, error, profile))
                {
                    return Failed(error);
                }
                return Succeeded("Edges beveled; query GetPolygons for the new ids.");
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::Bridge(const AZStd::vector<int>& polygonIds, const AZStd::vector<int>& edgeIds)
    {
        return EditMesh(
            "White Box Bridge",
            [&](WhiteBoxMesh& mesh)
            {
                AZStd::string error;
                Api::PolygonHandles polygons;
                Api::EdgeHandles edges;
                if (!polygonIds.empty() && !ResolvePolygons(mesh, polygonIds, false, polygons, error))
                {
                    return Failed(error);
                }
                if (!edgeIds.empty() && !ResolveEdges(mesh, edgeIds, edges, error))
                {
                    return Failed(error);
                }
                if (!Api::BridgeSelection(mesh, polygons, edges, error))
                {
                    return Failed(error);
                }
                return Succeeded("Bridge created.");
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::FillHole(const AZStd::vector<int>& edgeIds)
    {
        return EditMesh(
            "White Box Fill Hole",
            [&](WhiteBoxMesh& mesh)
            {
                AZStd::string error;
                Api::EdgeHandles edges;
                if (!ResolveEdges(mesh, edgeIds, edges, error))
                {
                    return Failed(error);
                }
                Api::PolygonHandle filled;
                if (!Api::FillHole(mesh, edges, error, &filled))
                {
                    return Failed(error);
                }
                return Succeeded("Hole filled.", {PolygonId(filled)});
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::DeletePolygons(const AZStd::vector<int>& polygonIds)
    {
        return EditMesh(
            "White Box Delete Polygons",
            [&](WhiteBoxMesh& mesh)
            {
                AZStd::string error;
                Api::PolygonHandles polygons;
                if (!ResolvePolygons(mesh, polygonIds, false, polygons, error))
                {
                    return Failed(error);
                }
                if (!Api::DeletePolygons(mesh, polygons, error))
                {
                    return Failed(error);
                }
                return Succeeded(AZStd::string::format("%zu polygons deleted.", polygons.size()));
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::MergePolygons(const AZStd::vector<int>& polygonIds)
    {
        return EditMesh(
            "White Box Merge Polygons",
            [&](WhiteBoxMesh& mesh)
            {
                AZStd::string error;
                Api::PolygonHandles polygons;
                if (!ResolvePolygons(mesh, polygonIds, false, polygons, error))
                {
                    return Failed(error);
                }
                Api::PolygonHandle merged;
                if (!Api::MergePolygons(mesh, polygons, error, &merged))
                {
                    return Failed(error);
                }
                return Succeeded("Polygons merged.", {PolygonId(merged)});
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::WeldVertices(const AZStd::vector<int>& vertexIds, const bool atLastVertex)
    {
        return EditMesh(
            "White Box Weld Vertices",
            [&](WhiteBoxMesh& mesh)
            {
                AZStd::string error;
                Api::VertexHandles vertices;
                if (!ResolveVertices(mesh, vertexIds, vertices, error))
                {
                    return Failed(error);
                }
                if (!Api::WeldVertices(mesh, vertices, atLastVertex, error))
                {
                    return Failed(error);
                }
                return Succeeded("Vertices welded.");
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::InsertEdgeLoops(const int edgeId, const int count, const float slide)
    {
        if (count < 1 || count > 64 || slide < -1.0f || slide > 1.0f)
        {
            return Failed("Loop cut needs 1 to 64 cuts and a slide between -1 and 1.");
        }
        return EditMesh(
            "White Box Loop Cut",
            [&](WhiteBoxMesh& mesh)
            {
                AZStd::string error;
                Api::EdgeHandles edges;
                if (!ResolveEdges(mesh, {edgeId}, edges, error))
                {
                    return Failed(error);
                }
                if (!Api::InsertEdgeLoops(mesh, edges.front(), count, error, slide))
                {
                    return Failed(error);
                }
                return Succeeded("Edge loops inserted.");
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::SubdividePolygons(const AZStd::vector<int>& polygonIds)
    {
        return EditMesh(
            "White Box Subdivide",
            [&](WhiteBoxMesh& mesh)
            {
                AZStd::string error;
                Api::PolygonHandles polygons;
                if (!ResolvePolygons(mesh, polygonIds, false, polygons, error))
                {
                    return Failed(error);
                }
                Api::PolygonHandles created;
                if (!Api::SubdividePolygons(mesh, polygons, error, &created))
                {
                    return Failed(error);
                }
                return Succeeded("Polygons subdivided.", PolygonIds(created));
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::ConnectVertices(const AZStd::vector<int>& vertexIds)
    {
        return EditMesh(
            "White Box Connect Vertices",
            [&](WhiteBoxMesh& mesh)
            {
                AZStd::string error;
                Api::VertexHandles vertices;
                if (!ResolveVertices(mesh, vertexIds, vertices, error))
                {
                    return Failed(error);
                }
                Api::EdgeHandles created;
                if (!Api::ConnectVertices(mesh, vertices, error, &created))
                {
                    return Failed(error);
                }
                AZStd::vector<int> ids;
                for (const Api::EdgeHandle edge : created)
                {
                    ids.push_back(edge.Index());
                }
                return Succeeded("Vertices connected; the ids are the new edges.", AZStd::move(ids));
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::InsertVertexOnEdge(const int edgeId, const float fraction)
    {
        if (!(fraction > 0.0f && fraction < 1.0f))
        {
            return Failed("The fraction along the edge is strictly between 0 and 1.");
        }
        return EditMesh(
            "White Box Insert Vertex",
            [&](WhiteBoxMesh& mesh)
            {
                AZStd::string error;
                Api::EdgeHandles edges;
                if (!ResolveEdges(mesh, {edgeId}, edges, error))
                {
                    return Failed(error);
                }
                const Api::VertexHandle vertex = Api::InsertVertexOnEdge(mesh, edges.front(), fraction, error);
                if (!vertex.IsValid())
                {
                    return Failed(error);
                }
                return Succeeded("Vertex inserted.", {vertex.Index()});
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::FlipEdge(const int edgeId)
    {
        return EditMesh(
            "White Box Flip Edge",
            [&](WhiteBoxMesh& mesh)
            {
                AZStd::string error;
                Api::EdgeHandles edges;
                if (!ResolveEdges(mesh, {edgeId}, edges, error))
                {
                    return Failed(error);
                }
                if (!Api::FlipEdge(mesh, edges.front()))
                {
                    return Failed("The edge cannot be flipped; it must sit inside a quad.");
                }
                return Succeeded("Edge flipped.");
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::DetachPolygonsToLayer(const AZStd::vector<int>& polygonIds)
    {
        const WhiteBoxMesh* mesh = EditableMesh();
        if (mesh == nullptr)
        {
            return Failed("No editable White Box mesh.");
        }
        AZStd::string error;
        Api::PolygonHandles polygons;
        if (!ResolvePolygons(*mesh, polygonIds, false, polygons, error))
        {
            return Failed(error);
        }
        AzToolsFramework::ScopedUndoBatch undoBatch("White Box Detach to Layer");
        if (!m_component.DetachPolygonsToLayer(polygons, error))
        {
            return Failed(error);
        }
        undoBatch.MarkEntityDirty(m_component.GetEntityId());
        RefreshPane();
        return Succeeded("Polygons moved to a new layer; the id is that layer's index.", {m_component.GetActiveLayerIndex()});
    }

    AuthoringResult EditorWhiteBoxAuthoring::RepairMesh()
    {
        return EditMesh(
            "White Box Repair Mesh",
            [](WhiteBoxMesh& mesh)
            {
                if (!Api::RepairMesh(mesh))
                {
                    return Failed("The mesh is empty, so there is nothing to repair.");
                }
                return Succeeded("Mesh welded and regrouped into polygons.");
            });
    }

    // ---- Surface ----------------------------------------------------------------------------------------------------

    AuthoringResult EditorWhiteBoxAuthoring::SetPolygonMaterial(
        const AZStd::vector<int>& polygonIds, const AZStd::string& materialPath)
    {
        AZ::Data::AssetId material;
        AZStd::string error;
        if (!ResolveMaterial(materialPath, material, error))
        {
            return Failed(error);
        }
        return EditMesh(
            "White Box Polygon Material",
            [&](WhiteBoxMesh& mesh)
            {
                Api::PolygonHandles polygons;
                if (!ResolvePolygons(mesh, polygonIds, true, polygons, error))
                {
                    return Failed(error);
                }
                for (const Api::PolygonHandle& polygon : polygons)
                {
                    Api::SetPolygonMaterial(mesh, polygon, material);
                }
                return Succeeded(AZStd::string::format("%zu polygons updated.", polygons.size()));
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::SetPolygonPaint(const AZStd::vector<int>& polygonIds, const AZ::Color& color)
    {
        const AZ::u32 packed = PackPaint(color);
        return EditMesh(
            "White Box Polygon Paint",
            [&](WhiteBoxMesh& mesh)
            {
                AZStd::string error;
                Api::PolygonHandles polygons;
                if (!ResolvePolygons(mesh, polygonIds, true, polygons, error))
                {
                    return Failed(error);
                }
                for (const Api::PolygonHandle& polygon : polygons)
                {
                    for (const Api::FaceHandle face : polygon.m_faceHandles)
                    {
                        Api::SetFacePaintColor(mesh, face, packed);
                    }
                }
                return Succeeded(AZStd::string::format("%zu polygons updated.", polygons.size()));
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::SetPolygonSmoothing(
        const AZStd::vector<int>& polygonIds, const AZ::u32 groups, const int edit)
    {
        if (edit < 0 || edit > static_cast<int>(Api::SmoothingEdit::Remove))
        {
            return Failed("Smoothing edit is 0 (set), 1 (add) or 2 (remove).");
        }
        return EditMesh(
            "White Box Smoothing Groups",
            [&](WhiteBoxMesh& mesh)
            {
                AZStd::string error;
                Api::PolygonHandles polygons;
                if (!ResolvePolygons(mesh, polygonIds, true, polygons, error))
                {
                    return Failed(error);
                }
                Api::SetPolygonSmoothingGroups(mesh, polygons, groups, static_cast<Api::SmoothingEdit>(edit));
                return Succeeded(AZStd::string::format("%zu polygons updated.", polygons.size()));
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::AutoSmooth(const AZStd::vector<int>& polygonIds, const float angleDegrees)
    {
        return EditMesh(
            "White Box Auto Smooth",
            [&](WhiteBoxMesh& mesh)
            {
                AZStd::string error;
                Api::PolygonHandles polygons;
                if (!ResolvePolygons(mesh, polygonIds, true, polygons, error))
                {
                    return Failed(error);
                }
                Api::AutoSmoothPolygons(mesh, polygons, angleDegrees);
                return Succeeded(AZStd::string::format("%zu polygons updated.", polygons.size()));
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::SetPolygonUvProjection(
        const AZStd::vector<int>& polygonIds, const int mode, const float scaleU, const float scaleV, const float offsetU,
        const float offsetV, const float rotationDegrees)
    {
        if (mode < 0 || mode > static_cast<int>(Api::UvProjectionMode::Manual))
        {
            return Failed("UV mode is 0 (world), 1 (planar) or 2 (manual).");
        }
        Api::UvProjection projection;
        projection.m_mode = static_cast<Api::UvProjectionMode>(mode);
        projection.m_scale = AZ::Vector2(scaleU, scaleV);
        projection.m_offset = AZ::Vector2(offsetU, offsetV);
        projection.m_rotationDegrees = rotationDegrees;
        return EditMesh(
            "White Box UV Projection",
            [&](WhiteBoxMesh& mesh)
            {
                AZStd::string error;
                Api::PolygonHandles polygons;
                if (!ResolvePolygons(mesh, polygonIds, true, polygons, error))
                {
                    return Failed(error);
                }
                Api::SetPolygonUvProjection(mesh, polygons, projection);
                return Succeeded(AZStd::string::format("UVs updated on %zu polygons.", polygons.size()));
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::FitPolygonUvs(const AZStd::vector<int>& polygonIds)
    {
        return EditMesh(
            "White Box Fit UVs",
            [&](WhiteBoxMesh& mesh)
            {
                AZStd::string error;
                Api::PolygonHandles polygons;
                if (!ResolvePolygons(mesh, polygonIds, true, polygons, error))
                {
                    return Failed(error);
                }
                Api::FitPolygonUvProjection(mesh, polygons);
                return Succeeded(AZStd::string::format("UVs fitted on %zu polygons.", polygons.size()));
            });
    }

    AuthoringResult EditorWhiteBoxAuthoring::RecalculateUvs()
    {
        return EditMesh(
            "White Box Recalculate UVs",
            [](WhiteBoxMesh& mesh)
            {
                Api::CalculateNormals(mesh);
                Api::CalculatePlanarUVs(mesh);
                return Succeeded("Normals and UVs recalculated.");
            });
    }

    // ---- Entity settings --------------------------------------------------------------------------------------------

    AuthoringDisplaySettings EditorWhiteBoxAuthoring::GetDisplaySettings()
    {
        AuthoringDisplaySettings out;
        out.m_edgesOnly = m_component.GetEdgesOnly();
        out.m_useGlobalTint = m_component.GetUseGlobalTint();
        out.m_tint = m_component.GetMaterialTint();
        out.m_useTexture = m_component.GetMaterialUseTexture();
        out.m_materialPath = MaterialPathOf(m_component.GetDefaultMaterialAssetId());
        out.m_csgSolver = static_cast<int>(m_component.GetCsgSolver());
        out.m_flipYZForExport = m_component.GetFlipYZForExport();
        return out;
    }

    AuthoringResult EditorWhiteBoxAuthoring::SetDisplaySettings(const AuthoringDisplaySettings& settings)
    {
        if (settings.m_csgSolver < 0 || settings.m_csgSolver > static_cast<int>(Api::CsgSolver::Fast))
        {
            return Failed("CSG solver is 0 (manifold) or 1 (fast).");
        }
        AZ::Data::AssetId material;
        AZStd::string error;
        if (!ResolveMaterial(settings.m_materialPath, material, error))
        {
            return Failed(error);
        }

        // Each setter rebuilds the mesh, so only the values that actually change are written.
        AzToolsFramework::ScopedUndoBatch undoBatch("White Box Display Settings");
        if (settings.m_csgSolver != static_cast<int>(m_component.GetCsgSolver()))
        {
            m_component.SetCsgSolver(static_cast<Api::CsgSolver>(settings.m_csgSolver));
        }
        if (settings.m_useGlobalTint != m_component.GetUseGlobalTint())
        {
            m_component.SetUseGlobalTint(settings.m_useGlobalTint);
        }
        if (settings.m_edgesOnly != m_component.GetEdgesOnly())
        {
            m_component.SetEdgesOnly(settings.m_edgesOnly);
        }
        if (!settings.m_tint.IsClose(m_component.GetMaterialTint()))
        {
            m_component.SetMaterialTint(settings.m_tint);
        }
        if (settings.m_useTexture != m_component.GetMaterialUseTexture())
        {
            m_component.SetMaterialUseTexture(settings.m_useTexture);
        }
        if (material != m_component.GetDefaultMaterialAssetId())
        {
            m_component.SetMaterialOverride(material);
        }
        m_component.SetFlipYZForExport(settings.m_flipYZForExport);
        undoBatch.MarkEntityDirty(m_component.GetEntityId());
        RefreshPane();
        return Succeeded("Display settings applied.");
    }

    AuthoringBooleanSettings EditorWhiteBoxAuthoring::GetBooleanSettings()
    {
        AuthoringBooleanSettings out;
        out.m_sourceEntity = m_component.GetBooleanSourceEntity();
        out.m_operation = static_cast<int>(m_component.GetBooleanOperation());
        out.m_live = m_component.GetLiveBoolean();
        out.m_affectActiveOnly = m_component.GetBooleanAffectActiveOnly();
        out.m_sourceAfterApply = static_cast<int>(m_component.GetSourceAfterApply());
        out.m_excludeFromBoolean = m_component.GetExcludeFromBoolean();
        out.m_booleanOthers = m_component.GetBooleanOthers();
        out.m_cutterOperation = static_cast<int>(m_component.GetCutterOperation());
        return out;
    }

    AuthoringResult EditorWhiteBoxAuthoring::SetBooleanSettings(const AuthoringBooleanSettings& settings)
    {
        const int lastOperation = static_cast<int>(Api::BooleanOperation::Intersection);
        if (settings.m_operation < 0 || settings.m_operation > lastOperation || settings.m_cutterOperation < 0 ||
            settings.m_cutterOperation > lastOperation)
        {
            return Failed("Boolean operation is 0 (union), 1 (subtraction) or 2 (intersection).");
        }
        if (settings.m_sourceAfterApply < 0 || settings.m_sourceAfterApply > static_cast<int>(SourceAfterApply::Delete))
        {
            return Failed("Source after apply is 0 (keep), 1 (hide) or 2 (delete).");
        }

        AzToolsFramework::ScopedUndoBatch undoBatch("White Box Boolean Settings");
        if (settings.m_sourceEntity != m_component.GetBooleanSourceEntity())
        {
            m_component.SetBooleanSourceEntity(settings.m_sourceEntity);
        }
        if (settings.m_operation != static_cast<int>(m_component.GetBooleanOperation()))
        {
            m_component.SetBooleanOperation(static_cast<Api::BooleanOperation>(settings.m_operation));
        }
        if (settings.m_affectActiveOnly != m_component.GetBooleanAffectActiveOnly())
        {
            m_component.SetBooleanAffectActiveOnly(settings.m_affectActiveOnly);
        }
        m_component.SetSourceAfterApply(static_cast<SourceAfterApply>(settings.m_sourceAfterApply));
        if (settings.m_excludeFromBoolean != m_component.GetExcludeFromBoolean())
        {
            m_component.SetExcludeFromBoolean(settings.m_excludeFromBoolean);
        }
        if (settings.m_cutterOperation != static_cast<int>(m_component.GetCutterOperation()))
        {
            m_component.SetCutterOperation(static_cast<Api::BooleanOperation>(settings.m_cutterOperation));
        }
        if (settings.m_booleanOthers != m_component.GetBooleanOthers())
        {
            m_component.SetBooleanOthers(settings.m_booleanOthers);
        }
        if (settings.m_live != m_component.GetLiveBoolean())
        {
            m_component.SetLiveBoolean(settings.m_live);
        }
        undoBatch.MarkEntityDirty(m_component.GetEntityId());
        RefreshPane();
        return Succeeded("Boolean settings applied.");
    }

    AuthoringResult EditorWhiteBoxAuthoring::ApplyBoolean()
    {
        const WhiteBoxMesh* mesh = EditableMesh();
        if (mesh == nullptr)
        {
            return Failed("No editable White Box mesh.");
        }
        const AZ::EntityId source = m_component.GetBooleanSourceEntity();
        if (!source.IsValid() || source == m_component.GetEntityId())
        {
            return Failed("Set a boolean source entity (another White Box) with SetBooleanSettings first.");
        }

        // ApplyBoolean only warns when the CSG cannot run, and a run that finds nothing to cut still rewrites the stored
        // bytes, so compare the geometry itself to tell whether the boolean did anything.
        const auto geometry = [](const WhiteBoxMesh& target)
        {
            AZStd::vector<AZ::Vector3> positions = Api::MeshVertexPositions(target);
            AZStd::sort(
                positions.begin(), positions.end(),
                [](const AZ::Vector3& lhs, const AZ::Vector3& rhs)
                {
                    if (lhs.GetX() != rhs.GetX())
                    {
                        return lhs.GetX() < rhs.GetX();
                    }
                    return lhs.GetY() != rhs.GetY() ? lhs.GetY() < rhs.GetY() : lhs.GetZ() < rhs.GetZ();
                });
            return AZStd::make_pair(Api::MeshFaceCount(target), AZStd::move(positions));
        };
        const auto before = geometry(*mesh);
        m_component.ApplyBoolean();
        const WhiteBoxMesh* after = EditableMesh();
        bool changed = after != nullptr;
        if (changed)
        {
            const auto afterGeometry = geometry(*after);
            changed = afterGeometry.first != before.first || afterGeometry.second.size() != before.second.size();
            for (size_t i = 0; !changed && i < before.second.size(); ++i)
            {
                changed = !before.second[i].IsClose(afterGeometry.second[i], 1e-5f);
            }
        }
        if (!changed)
        {
            return Failed("The boolean changed nothing; check the Editor log (the meshes may not overlap or may not be closed).");
        }
        RefreshPane();
        return Succeeded("Boolean applied.");
    }

    AuthoringResult EditorWhiteBoxAuthoring::RefreshGlobalBooleans()
    {
        EditorWhiteBoxComponent::RefreshGlobalBooleans();
        return Succeeded("Global booleans refreshed.");
    }

    AuthoringDrawSettings EditorWhiteBoxAuthoring::GetDrawSettings()
    {
        const DrawStairInfo stairs = m_component.GetDrawStairInfo();
        AuthoringDrawSettings out;
        out.m_shape = static_cast<int>(m_component.GetDrawShape());
        out.m_sides = m_component.GetDrawSides();
        out.m_holeRatio = m_component.GetDrawHoleRatio();
        out.m_tubeSides = m_component.GetDrawTubeSides();
        out.m_carve = m_component.GetDrawCarve();
        out.m_mergeUnion = m_component.GetDrawMergeUnion();
        out.m_polygonExtrude = m_component.GetDrawPolygonExtrude();
        out.m_unitCube = m_component.GetDrawUnitCube();
        out.m_unitCubeSize = m_component.GetDrawUnitCubeSize();
        out.m_unitCubeShowGrid = m_component.GetDrawUnitCubeShowGrid();
        out.m_stairSteps = stairs.m_steps;
        out.m_stairByHeight = stairs.m_byHeight;
        out.m_stairStepHeight = stairs.m_stepHeight;
        out.m_stairRotation = stairs.m_rotation;
        return out;
    }

    AuthoringResult EditorWhiteBoxAuthoring::SetDrawSettings(const AuthoringDrawSettings& settings)
    {
        if (!ValidShape(settings.m_shape))
        {
            return Failed(AZStd::string::format("Shape %d is not a DrawShapeType.", settings.m_shape));
        }
        AzToolsFramework::ScopedUndoBatch undoBatch("White Box Draw Settings");
        // Changing the shape resets Draw Sides to that shape's default, so the shape goes first.
        if (settings.m_shape != static_cast<int>(m_component.GetDrawShape()))
        {
            m_component.SetDrawShape(static_cast<DrawShapeType>(settings.m_shape));
        }
        m_component.SetDrawSides(settings.m_sides);
        m_component.SetDrawHoleRatio(settings.m_holeRatio);
        m_component.SetDrawTubeSides(settings.m_tubeSides);
        m_component.SetDrawCarve(settings.m_carve);
        m_component.SetDrawMergeUnion(settings.m_mergeUnion);
        m_component.SetDrawPolygonExtrude(settings.m_polygonExtrude);
        m_component.SetDrawUnitCube(settings.m_unitCube);
        m_component.SetDrawUnitCubeSize(settings.m_unitCubeSize);
        m_component.SetDrawUnitCubeShowGrid(settings.m_unitCubeShowGrid);
        m_component.SetDrawStairInfo(
            DrawStairInfo{
                settings.m_stairSteps, settings.m_stairByHeight, settings.m_stairStepHeight, settings.m_stairRotation});
        undoBatch.MarkEntityDirty(m_component.GetEntityId());
        RefreshPane();
        return Succeeded("Draw settings applied.");
    }

    // ---- Voxel stamp, collision and export --------------------------------------------------------------------------

    AuthoringResult EditorWhiteBoxAuthoring::SetVoxelCells(const AZStd::vector<AZ::Vector3>& cellMins, const bool filled)
    {
        if (cellMins.empty())
        {
            return Failed("No voxel cells were given.");
        }
        AzToolsFramework::ScopedUndoBatch undoBatch(filled ? "White Box Fill Voxels" : "White Box Clear Voxels");
        m_component.SetVoxelCells(cellMins, filled);
        undoBatch.MarkEntityDirty(m_component.GetEntityId());
        RefreshPane();
        return Succeeded(AZStd::string::format("%zu cells %s.", cellMins.size(), filled ? "filled" : "cleared"));
    }

    AuthoringResult EditorWhiteBoxAuthoring::ClearCubeStamp()
    {
        AzToolsFramework::ScopedUndoBatch undoBatch("White Box Clear Cube Stamp");
        m_component.ClearCubeStamp();
        undoBatch.MarkEntityDirty(m_component.GetEntityId());
        RefreshPane();
        return Succeeded("Cube stamp cleared.");
    }

    AuthoringResult EditorWhiteBoxAuthoring::AddCollision()
    {
        AzToolsFramework::ScopedUndoBatch undoBatch("White Box Add Collision");
        m_component.AddCollision();
        undoBatch.MarkEntityDirty(m_component.GetEntityId());
        return Succeeded("White Box collider ensured on the entity.");
    }

    AuthoringResult EditorWhiteBoxAuthoring::ExportObj(const AZStd::string& filePath)
    {
        const WhiteBoxMesh* mesh = m_component.GetEvaluatedWhiteBoxMesh();
        if (mesh == nullptr || filePath.empty())
        {
            return Failed("There is no mesh to export, or no file path was given.");
        }
        if (!Api::SaveToObj(*mesh, filePath))
        {
            return Failed(AZStd::string::format("Could not write '%s'.", filePath.c_str()));
        }
        return Succeeded(AZStd::string::format("Wrote '%s'.", filePath.c_str()));
    }
} // namespace WhiteBox
