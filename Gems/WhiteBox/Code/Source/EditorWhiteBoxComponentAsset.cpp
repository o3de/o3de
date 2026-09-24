/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * EditorWhiteBoxComponent - ASSET & SERIALIZATION translation unit: mesh stream
 * serialize/deserialize (including all legacy migrations), Save As Asset, obj exports and
 * the editor mesh asset plumbing.
 */

#include "Asset/EditorWhiteBoxMeshAsset.h"
#include "Asset/WhiteBoxMeshAssetHandler.h"
#include "EditorWhiteBoxComponent.h"

#include "Util/WhiteBoxEditorUtil.h"
#include "Util/WhiteBoxGltfExport.h"
#include "Util/WhiteBoxMeshUtil.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <QMessageBox>

namespace WhiteBox
{
    static const char* const AssetSavedUndoRedoDesc = "White Box Mesh asset saved";
    static const char* const ObjExtension = "obj";

    namespace
    {
        AZStd::string WhiteBoxPathAtProjectRoot(const AZStd::string_view name, const AZStd::string_view extension)
        {
            AZ::IO::Path whiteBoxPath;
            if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
            {
                settingsRegistry->Get(whiteBoxPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath);
            }
            whiteBoxPath /= AZ::IO::FixedMaxPathString::format("%.*s.%.*s", AZ_STRING_ARG(name), AZ_STRING_ARG(extension));
            return whiteBoxPath.Native();
        }
    } // namespace

    void EditorWhiteBoxComponent::DeserializeWhiteBox()
    {
        // Fresh data is about to replace everything the cache was built from.
        m_layerRuntime.m_meshCache.clear();

        // Migrate a legacy single-mesh scene with geometry into one layer; empty components stay empty.
        const bool looseHasGeometry = !m_whiteBoxData.empty() || !m_voxel.m_legacyGridData.empty() ||
            !m_voxel.m_legacyGridMergedData.empty() || !m_voxel.m_cells.empty();
        if (m_layers.empty() && looseHasGeometry)
        {
            WhiteBoxLayer layer;
            layer.m_name = "Layer 1";
            layer.m_freeformData = m_whiteBoxData;
            layer.m_gridData = m_voxel.m_legacyGridData;
            layer.m_gridMergedData = m_voxel.m_legacyGridMergedData;
            layer.m_voxelCells = m_voxel.m_cells;
            layer.m_voxelCellSizes = m_voxel.m_sizes;
            layer.m_voxelMerged = m_voxel.m_legacyMerged;
            m_layers.push_back(AZStd::move(layer));
            m_activeLayerIndex = 0;
        }

        // Ensure every layer has a stable id (migrate old scenes) and keep the allocator ahead.
        for (const WhiteBoxLayer& existing : m_layers)
        {
            if (existing.m_id != 0 && existing.m_id >= m_nextLayerId)
            {
                m_nextLayerId = existing.m_id + 1;
            }
        }
        for (WhiteBoxLayer& toId : m_layers)
        {
            if (toId.m_id == 0)
            {
                toId.m_id = AllocLayerId();
            }
        }

        // Fold legacy grid streams into each layer's freeform mesh; voxel cell records stay for "Clear Cube Stamp".
        for (WhiteBoxLayer& migrate : m_layers)
        {
            if (!migrate.m_gridData.empty() || !migrate.m_gridMergedData.empty())
            {
                Api::WhiteBoxMeshPtr freeform = Api::CreateWhiteBoxMesh();
                Api::ReadMesh(*freeform, migrate.m_freeformData);
                bool folded = false;
                for (const Api::WhiteBoxMeshStream* gridStream : { &migrate.m_gridData, &migrate.m_gridMergedData })
                {
                    if (!gridStream->empty())
                    {
                        Api::WhiteBoxMeshPtr grid = Api::CreateWhiteBoxMesh();
                        Api::ReadMesh(*grid, *gridStream);
                        if (!Api::MeshFaceHandles(*grid).empty())
                        {
                            AppendMesh(*freeform, *grid);
                            folded = true;
                        }
                    }
                }
                if (folded)
                {
                    Api::CalculateNormals(*freeform);
                    Api::CalculatePlanarUVs(*freeform);
                    Api::WriteMesh(*freeform, migrate.m_freeformData);
                }
                migrate.m_gridData.clear();
                migrate.m_gridMergedData.clear();
            }
            migrate.m_voxelMerged.clear();
        }
        m_voxel.m_legacyGridData.clear();
        m_voxel.m_legacyGridMergedData.clear();
        m_voxel.m_legacyMerged.clear();

        m_layerRuntime.m_lastCount = static_cast<int>(m_layers.size());
        m_layerRuntime.m_lastSignature = LayerSignature();

        if (m_layers.empty())
        {
            // No layers -> an empty white box (nothing renders, no stamps).
            ClearWorkingLayer();
            return;
        }

        if (m_activeLayerIndex < 0)
        {
            m_activeLayerIndex = 0;
        }
        if (m_activeLayerIndex >= static_cast<int>(m_layers.size()))
        {
            m_activeLayerIndex = static_cast<int>(m_layers.size()) - 1;
        }

        // Point the working streams at the active layer, then build the working meshes from them.
        const WhiteBoxLayer& layer = m_layers[m_activeLayerIndex];
        m_whiteBoxData = layer.m_freeformData;
        m_voxel.m_legacyGridData = layer.m_gridData;
        m_voxel.m_cells = layer.m_voxelCells;
        m_voxel.m_sizes = layer.m_voxelCellSizes;

        m_whiteBox = Api::CreateWhiteBoxMesh();
        if (m_editorMeshAsset->InUse())
        {
            m_editorMeshAsset->Load();
        }
        else
        {
            const auto result = Api::ReadMesh(*m_whiteBox, m_whiteBoxData);
            AZ_Error(
                "EditorWhiteBoxComponent", result != WhiteBox::Api::ReadResult::Error,
                "Error deserializing white box mesh stream");
        }

        m_gridMesh = Api::CreateWhiteBoxMesh();
        Api::ReadMesh(*m_gridMesh, m_voxel.m_legacyGridData);
        m_layerRuntime.m_loadedIndex = m_activeLayerIndex;
        m_layerRuntime.m_loadedId = m_layers[m_activeLayerIndex].m_id;
    }

    void EditorWhiteBoxComponent::WriteAssetToComponent()
    {
        if (m_editorMeshAsset->Loaded())
        {
            Api::WriteMesh(*m_editorMeshAsset->GetWhiteBoxMesh(), m_whiteBoxData);
        }
    }

    void EditorWhiteBoxComponent::SerializeWhiteBox()
    {
        if (m_editorMeshAsset->Loaded())
        {
            m_editorMeshAsset->Serialize();
        }
        else
        {
            Api::WriteMesh(*m_whiteBox, m_whiteBoxData);
        }

        // The stamp/grid layer is component-local, persist it alongside the freeform mesh.
        if (m_gridMesh)
        {
            Api::WriteMesh(*m_gridMesh, m_voxel.m_legacyGridData);
        }

        // If there are no layers yet but the working mesh now has geometry (a draw/stamp into an
        // empty white box), start a first layer so the edit is preserved.
        if (m_layers.empty())
        {
            const bool hasGeometry =
                (m_whiteBox != nullptr && !Api::MeshFaceHandles(*m_whiteBox).empty()) || !m_voxel.m_cells.empty();
            if (hasGeometry)
            {
                WhiteBoxLayer layer;
                layer.m_name = "Layer 1";
                layer.m_id = AllocLayerId();
                m_layers.push_back(AZStd::move(layer));
                m_activeLayerIndex = 0;
                m_layerRuntime.m_loadedIndex = 0;
                m_layerRuntime.m_loadedId = m_layers[0].m_id;
                // Deliberately leave m_layerRuntime.m_lastCount unchanged so the tick handler notices the
                // 0 -> 1 layer change and refreshes the property grid (the new layer appears).
            }
        }

        // Mirror the just-written working state into the layer it belongs to so the serialized
        // layer list always reflects the latest edits.
        StoreLayer(m_layerRuntime.m_loadedIndex);
        m_rebuild.m_workingMeshUnwritten = false;
    }

    void EditorWhiteBoxComponent::ExportToFile()
    {
        const AZStd::string initialAbsolutePathToExport =
            WhiteBoxPathAtProjectRoot(GetEntity()->GetName(), ObjExtension);

        const QString fileFilter = AZStd::string::format("*.%s", ObjExtension).c_str();
        const QString absoluteSaveFilePath = AzQtComponents::FileDialog::GetSaveFileName(
            nullptr, "Save As...", QString(initialAbsolutePathToExport.c_str()), fileFilter);
        if (absoluteSaveFilePath.isEmpty() || GetWhiteBoxMesh() == nullptr)
        {
            return; // dialog cancelled
        }

        // The flip goes on a copy; flipping the live mesh used to rotate the White Box itself on every export.
        Api::WhiteBoxMeshPtr exported = Api::CloneMesh(*GetWhiteBoxMesh());
        if (m_flipYZForExport)
        {
            for (const auto& handle : Api::MeshVertexHandles(*exported))
            {
                AZ::Vector3 p = Api::VertexPosition(*exported, handle);
                const float temp = p.GetY();
                p.SetY(p.GetZ());
                p.SetZ(-temp);
                Api::SetVertexPosition(*exported, handle, p);
            }
            Api::CalculateNormals(*exported);
        }

        const auto absoluteSaveFilePathUtf8 = absoluteSaveFilePath.toUtf8();
        const auto absoluteSaveFilePathCstr = absoluteSaveFilePathUtf8.constData();
        if (WhiteBox::Api::SaveToObj(*exported, absoluteSaveFilePathCstr))
        {
            AZ_Printf("EditorWhiteBoxComponent", "Exported white box mesh to: %s", absoluteSaveFilePathCstr);
            RequestEditSourceControl(absoluteSaveFilePathCstr);
        }
        else
        {
            AZ_Warning(
                "EditorWhiteBoxComponent", false, "Failed to export white box mesh to: %s", absoluteSaveFilePathCstr);
        }
    }

    void EditorWhiteBoxComponent::ExportToGltf()
    {
        const AZStd::string initialPath = WhiteBoxPathAtProjectRoot(GetEntity()->GetName(), "glb");
        const QString path = AzQtComponents::FileDialog::GetSaveFileName(
            nullptr, "Export glTF...", QString(initialPath.c_str()), "glTF (*.glb *.gltf)");
        if (path.isEmpty())
        {
            return;
        }
        // The cached render data is exactly what is on screen; build it when nothing has rendered yet.
        const WhiteBoxRenderData renderData = m_renderData.m_faces.empty() && EvaluatedMesh() != nullptr
            ? CreateWhiteBoxRenderData(*EvaluatedMesh(), m_material)
            : m_renderData;
        const AZStd::string filePath = path.toUtf8().constData();
        AZStd::string error;
        if (SaveToGltf(renderData, GetEntity()->GetName(), filePath, error))
        {
            AZ_Printf("EditorWhiteBoxComponent", "Exported white box mesh to: %s", filePath.c_str());
            RequestEditSourceControl(filePath.c_str());
        }
        else
        {
            AZ_Warning("EditorWhiteBoxComponent", false, "%s", error.c_str());
        }
    }

    void EditorWhiteBoxComponent::ExportDescendantsToFile()
    {
        // Get all child entities in the viewport
        AzToolsFramework::EntityIdList children;
        AZ::TransformBus::EventResult(children, GetEntityId(), &AZ::TransformBus::Events::GetAllDescendants);

        if (children.empty())
        {
            AZ_Warning("EditorWhiteBoxComponent", false, "Failed to export descendant whitebox meshes: No descendant entities found.");
            return;
        }

        const AZStd::string initialAbsolutePathToExport = WhiteBoxPathAtProjectRoot(GetEntity()->GetName(), ObjExtension);

        const QString fileFilter = AZStd::string::format("*.%s", ObjExtension).c_str();
        const QString absoluteSaveFilePath =
            AzQtComponents::FileDialog::GetSaveFileName(nullptr, "Save As...", QString(initialAbsolutePathToExport.c_str()), fileFilter);
        if (absoluteSaveFilePath.isEmpty())
        {
            return; // dialog cancelled
        }

        // Create a new empty white box mesh
        Api::WhiteBoxMeshPtr mesh = Api::CreateWhiteBoxMesh();
        for (auto& id : children)
        {
            AZ::Entity* e;
            AZ::ComponentApplicationBus::BroadcastResult(e, &AZ::ComponentApplicationRequests::FindEntity, id);
            AZ::Transform worldTM = e->GetTransform()->GetWorldTM();

            // Add all polys from selected white boxes
            for (auto component : e->FindComponents<EditorWhiteBoxComponent>())
            {
                WhiteBoxMesh* m = component->GetWhiteBoxMesh();
                Api::PolygonHandles polys = Api::MeshPolygonHandles(*m);
                for (auto& poly : polys)
                {
                    AZStd::vector<AZ::Vector3> verts = Api::PolygonVertexPositions(*m, poly);
                    if (verts.size() == 4) // if this is in fact a quad
                    {
                        Api::VertexHandle vertexHandles[4];

                        for (unsigned int i = 0; i < 4; i++)
                        {
                            AZ::Vector3 worldV = worldTM.TransformPoint(verts[i]);
                            if (m_flipYZForExport)
                            {
                                float temp = worldV.GetY();
                                worldV.SetY(worldV.GetZ());
                                worldV.SetZ(-temp);
                            }
                            vertexHandles[i] = Api::AddVertex(*mesh.get(), worldV);
                        }
                        Api::AddQuadPolygon(*mesh.get(), vertexHandles[0], vertexHandles[1], vertexHandles[2], vertexHandles[3]);
                    }
                }
            }
        }

        Api::CalculateNormals(*mesh.get());
        Api::CalculatePlanarUVs(*mesh.get());

        const auto absoluteSaveFilePathUtf8 = absoluteSaveFilePath.toUtf8();
        const auto absoluteSaveFilePathCstr = absoluteSaveFilePathUtf8.constData();
        if (WhiteBox::Api::SaveToObj(*mesh.get(), absoluteSaveFilePathCstr))
        {
            AZ_Printf("EditorWhiteBoxComponent", "Exported white box mesh to: %s", absoluteSaveFilePathCstr);
            RequestEditSourceControl(absoluteSaveFilePathCstr);
        }
        else
        {
            AZ_Warning("EditorWhiteBoxComponent", false, "Failed to export white box mesh to: %s", absoluteSaveFilePathCstr);
        }
    }

    AZStd::optional<WhiteBoxSaveResult> TrySaveAs(
        const AZStd::string_view entityName,
        const AZStd::function<AZStd::string(const AZStd::string&)>& absoluteSavePathFn,
        const AZStd::function<AZStd::optional<AZStd::string>(const AZStd::string&)>& relativePathFn,
        const AZStd::function<int()>& saveDecisionFn)
    {
        const AZStd::string initialAbsolutePathToSave =
            WhiteBoxPathAtProjectRoot(entityName, Pipeline::WhiteBoxMeshAssetHandler::AssetFileExtension);

        const QString absoluteSaveFilePath = QString(absoluteSavePathFn(initialAbsolutePathToSave).c_str());

        // user pressed cancel
        if (absoluteSaveFilePath.isEmpty())
        {
            return AZStd::nullopt;
        }

        const auto absoluteSaveFilePathUtf8 = absoluteSaveFilePath.toUtf8();
        const auto absoluteSaveFilePathCstr = absoluteSaveFilePathUtf8.constData();

        const AZStd::optional<AZStd::string> relativePath =
            relativePathFn(AZStd::string(absoluteSaveFilePathCstr, absoluteSaveFilePathUtf8.length()));

        if (!relativePath.has_value())
        {
            int saveDecision = saveDecisionFn();

            // save the file but do not attempt to create an asset
            if (saveDecision == QMessageBox::Save)
            {
                return WhiteBoxSaveResult{AZStd::nullopt, AZStd::string(absoluteSaveFilePathCstr)};
            }

            // the user decided not to save the asset outside the project folder after the prompt
            return AZStd::nullopt;
        }

        return WhiteBoxSaveResult{relativePath, AZStd::string(absoluteSaveFilePathCstr)};
    }

    AZ::Crc32 EditorWhiteBoxComponent::SaveAsAsset()
    {
        // let the user select final location of the saved asset
        const auto absoluteSavePathFn = [](const AZStd::string& initialAbsolutePath)
        {
            const QString fileFilter =
                AZStd::string::format("WhiteBoxMesh (*.%s)", Pipeline::WhiteBoxMeshAssetHandler::AssetFileExtension).c_str();
            const QString absolutePath =
                AzQtComponents::FileDialog::GetSaveFileName(nullptr, "Save As Asset...", QString(initialAbsolutePath.c_str()), fileFilter);

            return AZStd::string(absolutePath.toUtf8());
        };

        // ask the asset system to try and convert the absolutePath to a cache relative path
        const auto relativePathFn = [](const AZStd::string& absolutePath) -> AZStd::optional<AZStd::string>
        {
            AZStd::string relativePath;
            bool foundRelativePath = false;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                foundRelativePath,
                &AzToolsFramework::AssetSystem::AssetSystemRequest::GetRelativeProductPathFromFullSourceOrProductPath,
                absolutePath,
                relativePath);

            if (foundRelativePath)
            {
                return relativePath;
            }

            return AZStd::nullopt;
        };

        // present the user with the option of accepting saving outside the project folder or allow them to cancel the
        // operation
        const auto saveDecisionFn = []()
        {
            return QMessageBox::warning(
                AzToolsFramework::GetActiveWindow(),
                "Warning",
                "Saving a White Box Mesh Asset (.wbm) outside of the project root will not create an Asset for the "
                "Component to use. The file will be saved but will not be processed. For live updates to happen the "
                "asset must be saved somewhere in the current project folder. Would you like to continue?",
                (QMessageBox::Save | QMessageBox::Cancel),
                QMessageBox::Cancel);
        };

        const AZStd::optional<WhiteBoxSaveResult> saveResult =
            TrySaveAs(GetEntity()->GetName(), absoluteSavePathFn, relativePathFn, saveDecisionFn);

        // user pressed cancel
        if (!saveResult.has_value())
        {
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        const char* const absoluteSaveFilePath = saveResult.value().m_absoluteFilePath.c_str();
        if (saveResult.value().m_relativeAssetPath.has_value())
        {
            const auto& relativeAssetPath = saveResult.value().m_relativeAssetPath.value();

            // notify undo system the entity has been changed (m_meshAsset)
            AzToolsFramework::ScopedUndoBatch undoBatch(AssetSavedUndoRedoDesc);

            // if there was a previous asset selected, it has to be cloned to a new one
            // otherwise the internal mesh can simply be moved into the new asset
            m_editorMeshAsset->TakeOwnershipOfWhiteBoxMesh(
                relativeAssetPath,
                m_editorMeshAsset->InUse() ? Api::CloneMesh(*GetWhiteBoxMesh()) : AZStd::exchange(m_whiteBox, Api::CreateWhiteBoxMesh()));

            // change default shape to asset
            m_defaultShape = DefaultShapeType::Asset;

            // ensure this change gets tracked
            undoBatch.MarkEntityDirty(GetEntityId());

            RefreshProperties();

            m_editorMeshAsset->Save(absoluteSaveFilePath);
        }
        else
        {
            // save the asset to disk outside the project folder
            if (Api::SaveToWbm(*GetWhiteBoxMesh(), absoluteSaveFilePath))
            {
                RequestEditSourceControl(absoluteSaveFilePath);
            }
        }

        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    bool EditorWhiteBoxComponent::AssetInUse() const
    {
        return m_editorMeshAsset->InUse();
    }

    void EditorWhiteBoxComponent::OverrideEditorWhiteBoxMeshAsset(EditorWhiteBoxMeshAsset* editorMeshAsset)
    {
        // ensure we do not leak resources
        delete m_editorMeshAsset;

        m_editorMeshAsset = editorMeshAsset;
    }

    AZ::Crc32 EditorWhiteBoxComponent::AssetVisibility() const
    {
        return DisplayingAsset(m_defaultShape) ? AZ::Edit::PropertyVisibility::ShowChildrenOnly
                                               : AZ::Edit::PropertyVisibility::Hide;
    }
} // namespace WhiteBox
