/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Mesh/EditorMeshComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentConstants.h>

namespace AZ
{
    namespace Render
    {
        namespace
        {
            // Replaces the unsigned-integer value following a JSON key (e.g.
            // "max_vertices": 64 -> "max_vertices": 256) in-place, preserving the rest
            // of the document. Returns true if the value was found AND changed. Hand-
            // rolled (no JSON parser) because the .meshletpack sidecar is a tiny, fixed-
            // shape document we author ourselves.
            bool ReplaceJsonUintValue(AZStd::string& json, const char* key, AZ::u32 newValue)
            {
                const AZStd::string needle = AZStd::string::format("\"%s\"", key);
                const size_t k = json.find(needle);
                if (k == AZStd::string::npos) { return false; }
                const size_t colon = json.find(':', k + needle.size());
                if (colon == AZStd::string::npos) { return false; }
                size_t i = colon + 1;
                while (i < json.size() && (json[i] == ' ' || json[i] == '\t')) { ++i; }
                const size_t start = i;
                while (i < json.size() && json[i] >= '0' && json[i] <= '9') { ++i; }
                if (i == start) { return false; }   // no digits after the key
                const AZStd::string oldVal = json.substr(start, i - start);
                const AZStd::string newVal = AZStd::string::format("%u", newValue);
                if (oldVal == newVal) { return false; }
                json.replace(start, i - start, newVal);
                return true;
            }

            // Returns the absolute path the sidecar was written to, or empty on
            // failure. Authors (or updates) a *.meshletpack JSON in the project's
            // Assets/MeshletPacks/ directory using the supplied cluster budgets. If a
            // sidecar already exists, only the max_vertices/max_triangles values are
            // updated in place (the rest -- source id, cone weight, mesh filter -- is
            // preserved), and only when they actually differ.
            AZStd::string TryAutoAuthorMeshletPackSidecar(
                const AZ::Data::AssetId& modelAssetId,
                const AZStd::string& modelDisplayName,
                AZ::u16 maxVertices,
                AZ::u16 maxTriangles)
            {
                if (!modelAssetId.IsValid())
                {
                    return {};
                }

                // Resolve project root via @projectroot@ alias. Engine offers this
                // through AZ::IO::FileIOBase. If unavailable (shouldn't happen in
                // editor), fall back to current working dir.
                AZ::IO::FileIOBase* io = AZ::IO::FileIOBase::GetInstance();
                if (!io)
                {
                    return {};
                }

                // Determine the project's source root. Use @projectroot@ for the
                // installation/source root that AP scans.
                char projectRoot[AZ::IO::MaxPathLength] = {};
                if (!io->ResolvePath("@projectroot@", projectRoot, AZ::IO::MaxPathLength))
                {
                    return {};
                }

                // Sanitize the model display name into a filename-safe stem.
                AZStd::string stem = modelDisplayName;
                if (stem.empty())
                {
                    stem = modelAssetId.m_guid.ToString<AZStd::string>();
                }
                // Replace any '/', '\\', ':' in stem with '_'.
                for (char& c : stem)
                {
                    if (c == '/' || c == '\\' || c == ':' || c == ' ') c = '_';
                }

                const AZStd::string targetDir = AZStd::string::format("%s/Assets/MeshletPacks", projectRoot);
                const AZStd::string targetPath = AZStd::string::format("%s/%s.meshletpack",
                    targetDir.c_str(), stem.c_str());

                // If a sidecar already exists, UPDATE its cluster budgets in place
                // (preserving source id / cone weight / mesh filter) when they differ --
                // this is what lets the artist change cluster size from the Mesh
                // component on an already-baked model and trigger a re-bake.
                if (io->Exists(targetPath.c_str()))
                {
                    AZ::u64 size = 0;
                    if (!io->Size(targetPath.c_str(), size) || size == 0 || size > (1u << 20))
                    {
                        AZ_TracePrintf("Meshlets",
                            "Sidecar exists at %s but could not be sized; leaving as-is.\n", targetPath.c_str());
                        return targetPath;
                    }
                    AZStd::string existing;
                    existing.resize(static_cast<size_t>(size));
                    AZ::IO::HandleType rh;
                    if (io->Open(targetPath.c_str(), AZ::IO::OpenMode::ModeRead, rh))
                    {
                        AZ::u64 read = 0;
                        io->Read(rh, existing.data(), size, false, &read);
                        io->Close(rh);
                        existing.resize(static_cast<size_t>(read));
                    }
                    bool changed = ReplaceJsonUintValue(existing, "max_vertices", maxVertices);
                    changed = ReplaceJsonUintValue(existing, "max_triangles", maxTriangles) || changed;
                    if (!changed)
                    {
                        return targetPath;   // already up to date
                    }
                    AZ::IO::HandleType wh;
                    // ModeWrite without ModeAppend truncates the file (see OpenMode.h).
                    if (io->Open(targetPath.c_str(), AZ::IO::OpenMode::ModeWrite, wh))
                    {
                        AZ::u64 wrote = 0;
                        io->Write(wh, existing.data(), existing.size(), &wrote);
                        io->Close(wh);
                        AZ_Printf("Meshlets",
                            "Updated cluster budgets in %s (max_vertices=%u, max_triangles=%u). "
                            "AssetProcessor will re-bake the .azmeshletpack.\n",
                            targetPath.c_str(), maxVertices, maxTriangles);
                    }
                    return targetPath;
                }

                // Create directory if missing.
                if (!io->IsDirectory(targetDir.c_str()))
                {
                    if (!io->CreatePath(targetDir.c_str()))
                    {
                        AZ_Warning("Meshlets", false,
                            "Auto-author: could not create directory %s", targetDir.c_str());
                        return {};
                    }
                }

                // Build the JSON with the component's cluster budgets. AssetId is written
                // as "{uuid}:subid". The builder clamps to meshopt limits at bake time.
                const AZStd::string assetIdStr = modelAssetId.ToString<AZStd::string>();
                const AZStd::string json = AZStd::string::format(
                    "{\n"
                    "    \"source_model_asset_id\": \"%s\",\n"
                    "    \"max_vertices\": %u,\n"
                    "    \"max_triangles\": %u,\n"
                    "    \"cone_weight\": 0.5,\n"
                    "    \"mesh_filter\": [\"*\"]\n"
                    "}\n",
                    assetIdStr.c_str(),
                    static_cast<unsigned>(maxVertices),
                    static_cast<unsigned>(maxTriangles));

                AZ::IO::HandleType handle;
                if (!io->Open(targetPath.c_str(),
                              AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath,
                              handle))
                {
                    AZ_Warning("Meshlets", false,
                        "Auto-author: could not open %s for writing.", targetPath.c_str());
                    return {};
                }
                AZ::u64 written = 0;
                io->Write(handle, json.data(), json.size(), &written);
                io->Close(handle);

                if (written != json.size())
                {
                    AZ_Warning("Meshlets", false,
                        "Auto-author: short write to %s (%llu of %zu bytes)",
                        targetPath.c_str(), written, json.size());
                    return {};
                }

                AZ_Printf("Meshlets",
                    "Auto-created %s. AssetProcessor will produce the corresponding "
                    ".azmeshletpack on the next scan. Delete this file to disable "
                    "meshlets for this model.\n", targetPath.c_str());
                return targetPath;
            }
        } // anonymous namespace

        void EditorMeshComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);
            EditorMeshStats::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->RegisterGenericType<EditorMeshStats>();

                serializeContext->Class<EditorMeshComponent, BaseClass>()
                    ->Version(2, ConvertToEditorRenderComponentAdapter<1>)
                    ->Field("meshStats", &EditorMeshComponent::m_stats)
                    ;

                // This shouldn't be registered here, but is required to make a vector from EditorMeshComponentTypeId. This can be removed when one of the following happens:
                // - The generic type for AZStd::vector<AZ::Uuid> is registered in a more generic place
                // - EditorLevelComponentAPIComponent has a version of AddComponentsOfType that takes a single Uuid instead of a vector
                serializeContext->RegisterGenericType<AZStd::vector<AZ::Uuid>>();

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorMeshComponent>(
                        "Mesh", "The mesh component is the primary method of adding visual geometry to entities")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Graphics/Mesh")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Mesh.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Mesh.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/user-guide/components/reference/atom/mesh/")
                            ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<RPI::ModelAsset>::Uuid())
                        ->UIElement(AZ::Edit::UIHandlers::Button, "Add Material Component", "Add Material Component")
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                            ->Attribute(AZ::Edit::Attributes::ButtonText, "Add Material Component")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorMeshComponent::AddEditorMaterialComponent)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &EditorMeshComponent::GetEditorMaterialComponentVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorMeshComponent::m_stats, "Model Stats", "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ;

                    editContext->Class<MeshComponentController>(
                        "MeshComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<MeshComponentConfig>(
                        "MeshComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentConfig::m_modelAsset, "Model Asset", "Model asset reference", "Mesh Asset")
                                ->Attribute(AZ_CRC_CE("EditButton"), "")
                                ->Attribute(AZ_CRC_CE("EditDescription"), "Open in Scene Settings")
                                ->Attribute(AZ_CRC_CE("DisableEditButtonWhenNoAssetSelected"), true)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentConfig::m_sortKey, "Sort Key", "Transparent meshes are first drawn by sort key, then depth. Use this to force certain transparent meshes to draw before or after others.")
                            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &MeshComponentConfig::m_excludeFromReflectionCubeMaps, "Exclude from reflection cubemaps", "Model will not be visible in baked reflection probe cubemaps")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &MeshComponentConfig::m_useForwardPassIblSpecular, "Use Forward Pass IBL Specular",
                                "Renders image-based lighting (IBL) specular reflections in the forward pass, by using only the most influential probe (based on the position of the entity) and the global IBL cubemap. It can reduce rendering costs, but is only recommended for static objects that are affected by at most one reflection probe.  Note that this will also disable SSR on the mesh.")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &MeshComponentConfig::m_isRayTracingEnabled, "Use ray tracing",
                                "Includes this mesh in ray tracing calculations.")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &MeshComponentConfig::m_enableRayIntersection, "Support ray intersection",
                                "Set to true when the entity has UiCanvasOnMeshComponent")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &MeshComponentConfig::m_isAlwaysDynamic, "Always Moving", "Forces this mesh to be considered to always be moving, even if the transform didn't update. Useful for meshes with vertex shader animation.")
                            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &MeshComponentConfig::m_useMeshlets, "Use Virtual Geometry (Meshlets)",
                                "Render this mesh through the Meshlets gem (virtual geometry pipeline). Requires the Meshlets gem to be enabled in the project. Falls back to standard rendering if unavailable.")
                                // Reverting to PropertyRefreshLevels::ValuesOnly: function-pointer ChangeNotify can't
                                // point to EditorMeshComponent::OnConfigurationChanged from within Class<MeshComponentConfig>
                                // (EditContext asserts "Attribute doesn't belong to MeshComponentConfig class"). The
                                // base EditorComponentAdapter calls OnConfigurationChanged on framework commit events
                                // (entity deselect, save, undo) -- the auto-author runs from there.
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &MeshComponentConfig::m_meshletMaxVerticesPerCluster,
                              "Max Vertices/Cluster",
                              "Cluster vertex budget baked into the sidecar. Larger = fewer, bigger clusters = fewer "
                              "per-cluster draws + better vertex-cache reuse when many instances are on screen. "
                              "Range [32, 255] (meshopt cap). Changing this re-writes the .meshletpack sidecar so "
                              "AssetProcessor re-bakes the pack.")
                                ->Attribute(AZ::Edit::Attributes::Min, 32)
                                ->Attribute(AZ::Edit::Attributes::Max, 255)
                                ->Attribute(AZ::Edit::Attributes::Visibility, &MeshComponentConfig::m_useMeshlets)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &MeshComponentConfig::m_meshletMaxTrianglesPerCluster,
                              "Max Triangles/Cluster",
                              "Cluster triangle budget baked into the sidecar. Raise to make clusters bigger and cut the "
                              "per-cluster draw-command count. Range [16, 512] (meshopt cap; rounded DOWN to a multiple "
                              "of 4 by the builder). Changing this re-writes the .meshletpack sidecar.")
                                ->Attribute(AZ::Edit::Attributes::Min, 16)
                                ->Attribute(AZ::Edit::Attributes::Max, 512)
                                ->Attribute(AZ::Edit::Attributes::Step, 4)
                                ->Attribute(AZ::Edit::Attributes::Visibility, &MeshComponentConfig::m_useMeshlets)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->DataElement(AZ::Edit::UIHandlers::MultiLineEdit, &MeshComponentConfig::m_meshletPackStatus,
                              "Meshlet pack status",
                              "OK if a sibling .azmeshletpack is loaded; otherwise an actionable diagnostic.")
                                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                                ->Attribute(AZ::Edit::Attributes::Visibility, &MeshComponentConfig::m_useMeshlets)
                            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MeshComponentConfig::m_lodType, "Lod Type", "Determines how level of detail (LOD) will be selected during rendering.")
                                ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "LOD Type")
                                ->EnumAttribute(RPI::Cullable::LodType::Default, "Default")
                                ->EnumAttribute(RPI::Cullable::LodType::ScreenCoverage, "Screen Coverage")
                                ->EnumAttribute(RPI::Cullable::LodType::SpecificLod, "Specific LOD")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::EntireTree)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentConfig::m_lightingChannelConfig, "Lighting Channels", "")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Lod Configuration")
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "LOD Configuration")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &MeshComponentConfig::ShowLodConfig)
                            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MeshComponentConfig::m_lodOverride, "Lod Override", "Specifies the LOD to render, overriding the automatic LOD calculations")
                                ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "LOD Override")
                                ->Attribute(AZ::Edit::Attributes::EnumValues, &MeshComponentConfig::GetLodOverrideValues)
                                ->Attribute(AZ::Edit::Attributes::Visibility, &MeshComponentConfig::LodTypeIsSpecificLOD)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &MeshComponentConfig::m_minimumScreenCoverage, "Minimum Screen Coverage", "Minimum proportion of the screen that the entity will cover. If the entity is smaller than the minimum coverage, it is culled.")
                                ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                                ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                                ->Attribute(AZ::Edit::Attributes::Suffix, " percent")
                                ->Attribute(AZ::Edit::Attributes::Visibility, &MeshComponentConfig::LodTypeIsScreenCoverage)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &MeshComponentConfig::m_qualityDecayRate, "Quality Decay Rate",
                                "Rate at which the mesh quality decays. 0 - Always stays at highest quality LOD. 1 - Immediately falls off to lowest quality LOD.")
                                ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                                ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                                ->Attribute(AZ::Edit::Attributes::Visibility, &MeshComponentConfig::LodTypeIsScreenCoverage)
                        ;
                }
            }

            if (BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->ConstantProperty("EditorMeshComponentTypeId", BehaviorConstant(Uuid(EditorMeshComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

                behaviorContext->Class<EditorMeshComponent>()->RequestBus("RenderMeshComponentRequestBus");
            }
        }

        EditorMeshComponent::EditorMeshComponent(const MeshComponentConfig& config)
            : BaseClass(config)
        {
        }

        void EditorMeshComponent::Activate()
        {
            using EditorVisibilityRequestBus = AzToolsFramework::EditorVisibilityRequestBus;
            bool isVisible = true;
            EditorVisibilityRequestBus::EventResult(isVisible, GetEntityId(), &EditorVisibilityRequestBus::Events::GetVisibilityFlag);
            m_controller.SetVisibility(isVisible);

            m_controller.m_configuration.m_editorRayIntersection = true;
            BaseClass::Activate();
            AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(GetEntityId());
            AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
            MeshComponentNotificationBus::Handler::BusConnect(GetEntityId());
        }

        void EditorMeshComponent::Deactivate()
        {
            MeshComponentNotificationBus::Handler::BusDisconnect();
            AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
            AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
            BaseClass::Deactivate();
        }

        void EditorMeshComponent::SetPrimaryAsset(const AZ::Data::AssetId& assetId)
        {
            m_controller.SetModelAssetId(assetId);
        }

        AZ::Aabb EditorMeshComponent::GetEditorSelectionBoundsViewport(
            [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo)
        {
            return m_controller.GetWorldBounds();
        }

        bool EditorMeshComponent::EditorSelectionIntersectRayViewport(
            [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& src,
            const AZ::Vector3& dir, float& distance)
        {
            if (!m_controller.GetModel())
            {
                return false;
            }

            AZ::Transform transform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

            AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
            AZ::NonUniformScaleRequestBus::EventResult(nonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

            float t;
            AZ::Vector3 ignoreNormal;
            constexpr float rayLength = 1000.0f;
            if (m_controller.GetModel()->RayIntersection(transform, nonUniformScale, src, dir * rayLength, t, ignoreNormal))
            {
                distance = rayLength * t;
                return true;
            }

            return false;
        }

        bool EditorMeshComponent::SupportsEditorRayIntersect()
        {
            return true;
        }

        void EditorMeshComponent::DisplayEntityViewport(
            const AzFramework::ViewportInfo&, AzFramework::DebugDisplayRequests& debugDisplay)
        {
            if (!IsSelected())
            {
                return;
            }

            const AZ::Aabb localAabb = m_controller.GetLocalBounds();
            if (!localAabb.IsValid())
            {
                return;
            }

            AZ::Transform worldTM;
            AZ::TransformBus::EventResult(worldTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

            debugDisplay.PushMatrix(worldTM);

            debugDisplay.SetColor(AZ::Colors::White);
            debugDisplay.DrawWireBox(localAabb.GetMin(), localAabb.GetMax());

            debugDisplay.PopMatrix();
        }

        AZ::Crc32 EditorMeshComponent::AddEditorMaterialComponent()
        {
            const AZStd::vector<AZ::EntityId> entityList = { GetEntityId() };
            const AZ::ComponentTypeList componentsToAdd = { AZ::Uuid(AZ::Render::EditorMaterialComponentTypeId) };

            AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome outcome =
                AZ::Failure(AZStd::string("Failed to add AZ::Render::EditorMaterialComponentTypeId"));
            AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(outcome, &AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities, entityList, componentsToAdd);
            return Edit::PropertyRefreshLevels::EntireTree;
        }

        bool EditorMeshComponent::HasEditorMaterialComponent() const
        {
            return GetEntity() && GetEntity()->FindComponent(AZ::Uuid(AZ::Render::EditorMaterialComponentTypeId)) != nullptr;
        }

        AZ::u32 EditorMeshComponent::GetEditorMaterialComponentVisibility() const
        {
            return HasEditorMaterialComponent() ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show;
        }

        void EditorMeshComponent::OnModelReady(const Data::Asset<RPI::ModelAsset>& /*modelAsset*/, const Data::Instance<RPI::Model>& /*model*/)
        {
            const auto& lodAssets = m_controller.GetConfiguration().m_modelAsset->GetLodAssets();
            m_stats.m_meshStatsForLod.clear();
            m_stats.m_meshStatsForLod.reserve(lodAssets.size());
            for (const auto& lodAsset : lodAssets)
            {
                EditorMeshStatsForLod stats;
                const auto& meshes = lodAsset->GetMeshes();
                stats.m_meshCount = static_cast<AZ::u32>(meshes.size());
                stats.m_subMeshStatsForLod.reserve(stats.m_meshCount);
                for (const auto& mesh : meshes)
                {
                    const auto vertexCount = mesh.GetVertexCount();
                    const auto triCount = mesh.GetIndexCount() / 3;
                    stats.m_subMeshStatsForLod.push_back({});
                    stats.m_subMeshStatsForLod.back().m_vertCount = vertexCount;
                    stats.m_subMeshStatsForLod.back().m_triCount = triCount;
                    stats.m_vertCount += vertexCount;
                    stats.m_triCount += triCount;
                }
                m_stats.m_meshStatsForLod.emplace_back(AZStd::move(stats));
            }

            // Refresh the tree when the model loads to update UI based on the model.
            InvalidatePropertyDisplay(AzToolsFramework::Refresh_EntireTree);

        }

        AZ::u32 EditorMeshComponent::OnConfigurationChanged()
        {
            // temp variable is needed to hold reference to m_modelAsset while it's being loaded.
            // Otherwise it gets released in Deactivate function, and instantly re-activating the component
            // places it in a bad state, which happens in OnConfigurationChanged base function.
            // This is a bug with AssetManager [LYN-2249]
            auto temp = m_controller.m_configuration.m_modelAsset;

            m_stats.m_meshStatsForLod = {};
            SetDirty();

            m_controller.RefreshMeshletPackStatus();

            // SP1.5 convenience: when the toggle just flipped on and there's no
            // pack registered, auto-author a default sidecar so AssetProcessor
            // produces the pack without forcing the artist to leave the editor.
            // Diagnostic traces help confirm where the path bails when artists
            // report "toggle was flipped but no pack appeared".
            AZ_TracePrintf("Meshlets",
                "OnConfigurationChanged: useMeshlets=%d, modelAssetReady=%d\n",
                m_controller.m_configuration.m_useMeshlets ? 1 : 0,
                m_controller.m_configuration.m_modelAsset.IsReady() ? 1 : 0);
            if (m_controller.m_configuration.m_useMeshlets &&
                m_controller.m_configuration.m_modelAsset.IsReady())
            {
                auto* fp = m_controller.GetMeshletsFeatureProcessor();
                using PRS = Meshlets::MeshletsFeatureProcessorInterface::PackResolutionStatus;
                if (!fp)
                {
                    AZ_TracePrintf("Meshlets",
                        "Auto-author skipped: MeshletsFeatureProcessor not present on this scene.\n");
                }
                else
                {
                    const PRS status = fp->GetPackStatus(m_controller.m_configuration.m_modelAsset.GetId());
                    AZ_TracePrintf("Meshlets", "Auto-author: pack status = %d (NoPack=2, NotChecked=0)\n", static_cast<int>(status));
                    // The sidecar writer is idempotent: it CREATES the sidecar when
                    // missing and UPDATES the cluster budgets in place when they changed.
                    // Author/update on every commit while meshlets is enabled -- this
                    // covers both first-enable (NoPack/NotChecked) and a later cluster-size
                    // edit on an already-baked model (Ok). (status is still traced above.)
                    {
                        // Use a reasonable display name from the model asset's hint, or
                        // fall back to the asset id.
                        AZStd::string displayName;
                        AZ::Data::AssetCatalogRequestBus::BroadcastResult(displayName,
                            &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById,
                            m_controller.m_configuration.m_modelAsset.GetId());
                        // GetAssetPathById returns the product path (e.g. "objects/cube.azmodel").
                        // Strip directories and extension to get a name stem.
                        AZStd::string stem;
                        AZ::StringFunc::Path::GetFileName(displayName.c_str(), stem);
                        AZ_TracePrintf("Meshlets",
                            "Auto-author: invoking helper for model path '%s' (stem '%s')\n",
                            displayName.c_str(), stem.c_str());

                        TryAutoAuthorMeshletPackSidecar(
                            m_controller.m_configuration.m_modelAsset.GetId(), stem,
                            m_controller.m_configuration.m_meshletMaxVerticesPerCluster,
                            m_controller.m_configuration.m_meshletMaxTrianglesPerCluster);
                    }
                }
            }

            return BaseClass::OnConfigurationChanged();
        }

        void EditorMeshComponent::OnEntityVisibilityChanged(bool visibility)
        {
            m_controller.SetVisibility(visibility);
        }

        bool EditorMeshComponent::ShouldActivateController() const
        {
            // By default, components using the EditorRenderComponentAdapter will only activate if the component is visible
            // Since the mesh component handles visibility changes by not rendering the mesh, rather than deactivating the component entirely,
            // it can be activated even if it is not visible
            return true;
        }
    } // namespace Render
} // namespace AZ
