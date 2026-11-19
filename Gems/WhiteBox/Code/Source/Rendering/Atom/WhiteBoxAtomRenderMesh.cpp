/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxAtomRenderMesh.h"

#include <Rendering/Atom/WhiteBoxMeshAtomData.h>
#include <Rendering/WhiteBoxLightmapUv.h>
#include <Rendering/WhiteBoxMaterialSlot.h>
#include <Rendering/WhiteBoxRenderData.h>
#include <Util/WhiteBoxMathUtil.h>
#include <Viewport/WhiteBoxViewportConstants.h>

#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelLodAssetCreator.h>
#include <Atom/RPI.Reflect/ResourcePoolAssetCreator.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialAssignment.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/PackedVector3.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/sort.h>

namespace WhiteBox
{
    AZ_CVAR(
        bool, wb_shareRenderModels, true, nullptr, AZ::ConsoleFunctorFlags::Null,
        "White Boxes with identical geometry and material layout draw with one shared model instead of each building their own.");

    //! A model built from one geometry, shared by every White Box whose geometry and material layout match.
    struct SharedModel
    {
        AZ::Data::Asset<AZ::RPI::ModelAsset> m_modelAsset;
        AZ::Data::Instance<AZ::RPI::Model> m_model;
        AZ::u32 m_vertexCount = 0;
    };

    //! A material instance with one asset's settings applied, shared by every White Box group that wants the same.
    struct SharedMaterial
    {
        AZ::Data::Instance<AZ::RPI::Material> m_instance;
        AZ::Data::Asset<AZ::RPI::MaterialAsset> m_asset;
        bool m_custom = false; //!< The group's own material asset was used, not the built-in one.
    };

    namespace
    {
        AZStd::unordered_map<AZ::u64, AZStd::weak_ptr<SharedMaterial>>& SharedMaterials()
        {
            static AZStd::unordered_map<AZ::u64, AZStd::weak_ptr<SharedMaterial>> materials;
            return materials;
        }

        // Process-wide, main thread only. Entries expire with their last user and are dropped on the next insert.
        AZStd::unordered_map<AZ::u64, AZStd::weak_ptr<SharedModel>>& SharedModels()
        {
            static AZStd::unordered_map<AZ::u64, AZStd::weak_ptr<SharedModel>> models;
            return models;
        }

        // Every AtomRenderMesh alive, for wb_renderStats and wb_renderVisible. Main thread only.
        AZStd::unordered_set<const AtomRenderMesh*>& LiveMeshes()
        {
            static AZStd::unordered_set<const AtomRenderMesh*> meshes;
            return meshes;
        }

        struct MeshBuildStats
        {
            AZ::u32 m_built = 0;
            AZ::u32 m_shared = 0;
            double m_builtSeconds = 0.0;
            double m_sharedSeconds = 0.0;
            // Where a shared mesh's time goes.
            double m_groupSeconds = 0.0;    //!< Culling and grouping the faces, before the key.
            double m_keySeconds = 0.0;      //!< Hashing the faces.
            double m_materialSeconds = 0.0; //!< Finding or making the materials.
            AZ::u32 m_materialsMade = 0;
            AZ::u32 m_materialsShared = 0;
            double m_acquireSeconds = 0.0;  //!< Handing the model to the mesh feature processor.
        };

        MeshBuildStats& Stats()
        {
            static MeshBuildStats stats;
            return stats;
        }

        void PrintMeshStats(const AZ::ConsoleCommandContainer&)
        {
            const MeshBuildStats& s = Stats();
            AZ_Printf(
                "WhiteBox", "Render meshes: %u built (%.1f ms total), %u shared (%.1f ms total), %zu models cached",
                s.m_built, s.m_builtSeconds * 1000.0, s.m_shared, s.m_sharedSeconds * 1000.0, SharedModels().size());
            AZ_Printf(
                "WhiteBox", "Shared mesh time: group %.1f ms, key %.1f ms, materials %.1f ms, acquire %.1f ms",
                s.m_groupSeconds * 1000.0, s.m_keySeconds * 1000.0, s.m_materialSeconds * 1000.0, s.m_acquireSeconds * 1000.0);
            AZ_Printf("WhiteBox", "Material instances: %u made, %u shared", s.m_materialsMade, s.m_materialsShared);
        }
        AZ_CONSOLEFREEFUNC("wb_meshStats", PrintMeshStats, AZ::ConsoleFunctorFlags::Null, "Print how many White Box render meshes were built or shared.");

        void ResetMeshStats(const AZ::ConsoleCommandContainer&)
        {
            Stats() = MeshBuildStats{};
        }
        AZ_CONSOLEFREEFUNC("wb_meshStatsReset", ResetMeshStats, AZ::ConsoleFunctorFlags::Null, "Reset the White Box render mesh counters.");

        // FNV-1a over the values that reach the vertex streams.
        struct KeyHasher
        {
            AZ::u64 m_hash = 14695981039346656037ull;

            void Bytes(const void* data, size_t size)
            {
                const auto* bytes = static_cast<const AZ::u8*>(data);
                for (size_t i = 0; i < size; ++i)
                {
                    m_hash = (m_hash ^ bytes[i]) * 1099511628211ull;
                }
            }
            template<typename T>
            void Add(const T& value)
            {
                Bytes(&value, sizeof(T));
            }
            void Add(const AZ::Vector2& v)
            {
                const float f[2] = { v.GetX(), v.GetY() };
                Bytes(f, sizeof(f));
            }
            void Add(const AZ::Vector3& v)
            {
                const float f[3] = { v.GetX(), v.GetY(), v.GetZ() };
                Bytes(f, sizeof(f));
            }
            void Add(const AZ::Vector4& v)
            {
                const float f[4] = { v.GetX(), v.GetY(), v.GetZ(), v.GetW() };
                Bytes(f, sizeof(f));
            }
            void Add(const WhiteBoxVertex& v)
            {
                Add(v.m_position);
                Add(v.m_uv);
                Add(v.m_normal);
                Add(v.m_blend);
                Add(v.m_lightmapUv);
            }
        };

        AZStd::string SlotLabel(const AZ::Data::AssetId& material, const AZ::u32 paintColor)
        {
            AZStd::string label = "White Box Default";
            if (material.IsValid())
            {
                AZStd::string path;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(path, &AZ::Data::AssetCatalogRequests::GetAssetPathById, material);
                label = path.empty() ? material.ToString<AZStd::string>() : AZStd::string(AZ::IO::PathView(path).Stem().Native());
            }
            if (paintColor != 0)
            {
                label += AZStd::string::format(
                    " (#%02X%02X%02X)", paintColor & 255, (paintColor >> 8) & 255, (paintColor >> 16) & 255);
            }
            return label;
        }

        // True when the Material component swaps this slot's asset (not just its properties).
        bool HasAssetOverride(const AZ::Render::MaterialAssignmentMap& overrides, const AZ::u32 stableId)
        {
            using AZ::Render::MaterialAssignmentId;
            for (const MaterialAssignmentId& id :
                 { MaterialAssignmentId::CreateFromLodAndStableId(0, stableId), MaterialAssignmentId::CreateFromStableIdOnly(stableId),
                   MaterialAssignmentId::CreateDefault() })
            {
                const auto it = overrides.find(id);
                if (it != overrides.end() && it->second.m_materialAsset.GetId().IsValid())
                {
                    return true;
                }
            }
            return false;
        }
    } // namespace

    AtomRenderMesh::AtomRenderMesh(AZ::EntityId entityId, bool isPrimary)
        : m_entityId(entityId)
        , m_isPrimary(isPrimary)
    {
        LiveMeshes().insert(this);
        // Only the primary mesh owns the per-entity mesh-handle state (single-handler bus).
        if (m_isPrimary)
        {
            AZ::Render::MeshHandleStateRequestBus::Handler::BusConnect(m_entityId);
            AZ::Render::MaterialConsumerRequestBus::Handler::BusConnect(m_entityId);
            AZ::Render::MaterialComponentNotificationBus::Handler::BusConnect(m_entityId);
        }
    }

    AtomRenderMesh::~AtomRenderMesh()
    {
        LiveMeshes().erase(this);
        m_materialGroups.clear();
        if (m_meshHandle.IsValid() && m_meshFeatureProcessor)
        {
            m_meshFeatureProcessor->ReleaseMesh(m_meshHandle);
            if (m_isPrimary)
            {
                AZ::Render::MeshHandleStateNotificationBus::Event(
                    m_entityId, &AZ::Render::MeshHandleStateNotificationBus::Events::OnMeshHandleSet, &m_meshHandle);
            }
        }

        if (m_isPrimary)
        {
            AZ::Render::MeshHandleStateRequestBus::Handler::BusDisconnect();
            AZ::Render::MaterialConsumerRequestBus::Handler::BusDisconnect();
            AZ::Render::MaterialComponentNotificationBus::Handler::BusDisconnect();
            if (!m_publishedSlots.empty())
            {
                // The slots go with this mesh; a Material component reloads against whatever replaces it.
                AZ::Render::MaterialConsumerNotificationBus::Event(
                    m_entityId, &AZ::Render::MaterialConsumerNotificationBus::Events::OnMaterialAssignmentSlotsChanged);
            }
        }
        AZ::TickBus::Handler::BusDisconnect();
    }

    bool AtomRenderMesh::AreAttributesValid() const
    {
        bool attributesAreValid = true;

        for (const auto& attribute : m_attributes)
        {
            AZStd::visit(
                [&attributesAreValid](const auto& att)
                {
                    if (!att->IsValid())
                    {
                        attributesAreValid = false;
                    }
                },
                attribute);
        }

        return attributesAreValid;
    }

    bool AtomRenderMesh::CreateMeshBuffers(const WhiteBoxMeshAtomData& meshData)
    {
        m_indexBuffer = AZStd::make_unique<IndexBuffer>(meshData.GetIndices());

        CreateAttributeBuffer<AttributeType::Position>(meshData.GetPositions());
        CreateAttributeBuffer<AttributeType::Normal>(meshData.GetNormals());
        CreateAttributeBuffer<AttributeType::Tangent>(meshData.GetTangents());
        CreateAttributeBuffer<AttributeType::Bitangent>(meshData.GetBitangents());
        CreateAttributeBuffer<AttributeType::UV>(meshData.GetUVs());
        CreateAttributeBuffer<AttributeType::Color>(meshData.GetColors());
        CreateAttributeBuffer<AttributeType::LightmapUV>(meshData.GetLightmapUVs());

        return AreAttributesValid();
    }

    bool AtomRenderMesh::UpdateMeshBuffers(const WhiteBoxMeshAtomData& meshData)
    {
        UpdateAttributeBuffer<AttributeType::Position>(meshData.GetPositions());
        UpdateAttributeBuffer<AttributeType::Normal>(meshData.GetNormals());
        UpdateAttributeBuffer<AttributeType::Tangent>(meshData.GetTangents());
        UpdateAttributeBuffer<AttributeType::Bitangent>(meshData.GetBitangents());
        UpdateAttributeBuffer<AttributeType::UV>(meshData.GetUVs());
        UpdateAttributeBuffer<AttributeType::Color>(meshData.GetColors());
        UpdateAttributeBuffer<AttributeType::LightmapUV>(meshData.GetLightmapUVs());

        return AreAttributesValid();
    }

    void AtomRenderMesh::AddLodBuffers(AZ::RPI::ModelLodAssetCreator& modelLodCreator)
    {
        modelLodCreator.SetLodIndexBuffer(m_indexBuffer->GetBuffer());

        for (auto& attribute : m_attributes)
        {
            AZStd::visit(
                [&modelLodCreator](auto& att)
                {
                    att->AddLodStreamBuffer(modelLodCreator);
                },
                attribute);
        }
    }

    void AtomRenderMesh::AddMeshBuffers(AZ::RPI::ModelLodAssetCreator& modelLodCreator, uint32_t indexOffset, uint32_t indexCount)
    {
        auto indexView = m_indexBuffer->GetBufferViewDescriptor();
        indexView.m_elementOffset = indexOffset;
        indexView.m_elementCount = indexCount;
        modelLodCreator.SetMeshIndexBuffer(AZ::RPI::BufferAssetView(m_indexBuffer->GetBuffer(), indexView));

        for (auto& attribute : m_attributes)
        {
            AZStd::visit(
                [&modelLodCreator](auto&& att)
                {
                    att->AddMeshStreamBuffer(modelLodCreator);
                },
                attribute);
        }
    }

    bool AtomRenderMesh::CreateLodAsset(const WhiteBoxMeshAtomData& meshData)
    {
        if (!CreateMeshBuffers(meshData))
        {
            return false;
        }

        AZ::RPI::ModelLodAssetCreator modelLodCreator;
        modelLodCreator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        AddLodBuffers(modelLodCreator);
        for (uint32_t slot = 0; slot < m_materialGroups.size(); ++slot)
        {
            const MaterialGroup& group = m_materialGroups[slot];
            modelLodCreator.BeginMesh();
            modelLodCreator.SetMeshAabb(meshData.GetAabb());
            modelLodCreator.SetMeshMaterialSlot(group.m_stableId);
            AddMeshBuffers(modelLodCreator, group.m_indexOffset, group.m_indexCount);
            modelLodCreator.EndMesh();
        }

        if (!modelLodCreator.End(m_lodAsset))
        {
            AZ_Error("CreateLodAsset", false, "Couldn't create LoD asset.");
            return false;
        }

        if (!m_lodAsset.IsReady())
        {
            AZ_Error("CreateLodAsset", false, "LoD asset is not ready.");
            return false;
        }

        if (!m_lodAsset.Get())
        {
            AZ_Error("CreateLodAsset", false, "LoD asset is nullptr.");
            return false;
        }

        return true;
    }

    namespace
    {
        // Settings a group's material instance is made with. A custom asset keeps its authored colour and texture.
        void ApplyMaterialSettings(
            AZ::RPI::Material& materialInstance, const bool customMaterial, const AZ::u32 paintColor, const WhiteBoxMaterial& material)
        {
            if (customMaterial && paintColor == 0)
            {
                return;
            }
            // Per-layer tint travels in COLOR0, so the base colour must be white; global tint uses baseColor.
            const bool painted = paintColor != 0;
            const bool useVertexColor = material.m_useVertexColor && !painted;
            const AZ::Color baseColor = painted
                ? AZ::Color(
                    float(paintColor & 255) / 255.0f,
                    float((paintColor >> 8) & 255) / 255.0f,
                    float((paintColor >> 16) & 255) / 255.0f, 1.0f)
                : (useVertexColor ? AZ::Color(1.0f, 1.0f, 1.0f, 1.0f) : AZ::Color(material.m_tint));
            if (const auto& materialPropertyIndex = materialInstance.FindPropertyIndex(AZ::Name("baseColor.color"));
                materialPropertyIndex.IsValid())
            {
                materialInstance.SetPropertyValue(materialPropertyIndex, baseColor);
            }

            // Enable StandardPBR's vertex colour group; the property id varies by engine version.
            for (const char* enableName :
                 {"vertexColor.enable", "vertexColor.useVertexColor", "vertexColor.enableVertexColor",
                  "vertexColor.toggle"})
            {
                if (const auto& idx = materialInstance.FindPropertyIndex(AZ::Name(enableName)); idx.IsValid())
                {
                    materialInstance.SetPropertyValue(idx, useVertexColor);
                    break;
                }
            }
            if (useVertexColor)
            {
                if (const auto& idx = materialInstance.FindPropertyIndex(AZ::Name("vertexColor.factor")); idx.IsValid())
                {
                    materialInstance.SetPropertyValue(idx, 1.0f);
                }
            }

            if (const auto& materialPropertyIndex = materialInstance.FindPropertyIndex(AZ::Name("baseColor.useTexture"));
                materialPropertyIndex.IsValid())
            {
                materialInstance.SetPropertyValue(materialPropertyIndex, material.m_useTexture && !painted);
            }
        }

        // Everything ApplyMaterialSettings and the asset choice depend on.
        AZ::u64 MaterialInstanceKey(const AZ::Data::AssetId& assetId, const AZ::u32 paintColor, const WhiteBoxMaterial& material)
        {
            KeyHasher key;
            key.Add(assetId.m_guid);
            key.Add(assetId.m_subId);
            key.Add(paintColor);
            key.Add(material.m_tint);
            key.Add(material.m_useTexture);
            key.Add(material.m_useVertexColor);
            return key.m_hash;
        }

        AZStd::shared_ptr<SharedMaterial> AcquireSharedMaterial(
        const AZ::Data::AssetId& assetId, const AZ::u32 paintColor, const WhiteBoxMaterial& material)
    {
        const AZ::u64 key = MaterialInstanceKey(assetId, paintColor, material);
        auto& cache = SharedMaterials();
        if (const auto found = cache.find(key); found != cache.end())
        {
            if (AZStd::shared_ptr<SharedMaterial> shared = found->second.lock())
            {
                Stats().m_materialsShared++;
                return shared;
            }
        }

        AZ::Data::Asset<AZ::RPI::MaterialAsset> materialAsset;
        if (assetId.IsValid())
        {
            materialAsset = AZ::RPI::AssetUtils::LoadAssetById<AZ::RPI::MaterialAsset>(assetId, AZ::RPI::AssetUtils::TraceLevel::Warning);
        }
        const bool custom = static_cast<bool>(materialAsset);
        if (!materialAsset)
        {
            materialAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::MaterialAsset>("materials/whiteboxdefault.azmaterial");
        }
        if (!materialAsset)
        {
            AZ_Error("WhiteBox", false, "Could not load WhiteBox material.");
            return nullptr;
        }

        auto shared = AZStd::make_shared<SharedMaterial>();
        shared->m_asset = materialAsset;
        shared->m_custom = custom;
        shared->m_instance = AZ::RPI::Material::Create(materialAsset);
        if (!shared->m_instance)
        {
            return nullptr;
        }
        ApplyMaterialSettings(*shared->m_instance, custom, paintColor, material);
        Stats().m_materialsMade++;

        if (cache.size() >= 64 && cache.size() % 64 == 0)
        {
            for (auto it = cache.begin(); it != cache.end();)
            {
                it = it->second.expired() ? cache.erase(it) : AZStd::next(it);
            }
        }
        cache[key] = shared;
        return shared;
    }
    } // namespace

    bool AtomRenderMesh::CreateMaterialInstances(
        const WhiteBoxMaterial& material, AZStd::vector<AZ::Data::Asset<AZ::RPI::MaterialAsset>>& assets)
    {
        assets.clear();
        for (MaterialGroup& group : m_materialGroups)
        {
            AZStd::shared_ptr<SharedMaterial> shared = AcquireSharedMaterial(group.m_assetId, group.m_paintColor, material);
            if (!shared)
            {
                return false;
            }
            group.m_shared = shared;
            group.m_instance = shared->m_instance;
            group.m_customMaterial = shared->m_custom;
            group.m_materialKey = MaterialInstanceKey(group.m_assetId, group.m_paintColor, material);
            assets.push_back(shared->m_asset);
        }
        return true;
    }

    bool AtomRenderMesh::CreateModelAsset(const AZStd::vector<AZ::Data::Asset<AZ::RPI::MaterialAsset>>& materialAssets)
    {
        AZ::RPI::ModelAssetCreator modelCreator;
        modelCreator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        modelCreator.SetName(ModelName);
        modelCreator.AddLodAsset(AZStd::move(m_lodAsset));
        
        for (uint32_t slot = 0; slot < m_materialGroups.size(); ++slot)
        {
            const MaterialGroup& group = m_materialGroups[slot];
            AZ::RPI::ModelMaterialSlot materialSlot;
            materialSlot.m_stableId = group.m_stableId;
            materialSlot.m_displayName = AZ::Name(SlotLabel(group.m_assetId, group.m_paintColor));
            materialSlot.m_defaultMaterialAsset = materialAssets[slot];
            modelCreator.AddMaterialSlot(materialSlot);
        }

        return modelCreator.End(m_modelAsset);
    }

    bool AtomRenderMesh::CreateModel()
    {
        m_model = AZ::RPI::Model::FindOrCreate(m_modelAsset);
        return AcquireMesh();
    }

    bool AtomRenderMesh::AcquireMesh()
    {
        m_meshFeatureProcessor =
            AZ::RPI::Scene::GetFeatureProcessorForEntity<AZ::Render::MeshFeatureProcessorInterface>(m_entityId);

        if (!m_meshFeatureProcessor)
        {
            AZ_Error(
                "MeshComponentController", m_meshFeatureProcessor,
                "Unable to find a MeshFeatureProcessorInterface on the entityId.");
            return false;
        }

        m_meshFeatureProcessor->ReleaseMesh(m_meshHandle);
        AZ::Render::MaterialAssignmentMap overrides;
        if (m_isPrimary)
        {
            AZ::Render::MaterialComponentRequestBus::EventResult(
                overrides, m_entityId, &AZ::Render::MaterialComponentRequests::GetMaterialMapCopy);
        }
        const AZ::Render::CustomMaterialMap materials = BuildCustomMaterials(overrides);
        RememberBoundMaterials(materials);
        m_meshHandle = m_meshFeatureProcessor->AcquireMesh(AZ::Render::MeshHandleDescriptor(m_modelAsset, materials));
        if (m_isPrimary)
        {
            AZ::Render::MeshHandleStateNotificationBus::Event(m_entityId, &AZ::Render::MeshHandleStateNotificationBus::Events::OnMeshHandleSet, &m_meshHandle);
        }

        return true;
    }

    bool AtomRenderMesh::MeshRequiresFullRebuild([[maybe_unused]] const WhiteBoxMeshAtomData& meshData) const
    {
        return meshData.VertexCount() != m_vertexCount;
    }

    bool AtomRenderMesh::CreateMesh(const WhiteBoxMeshAtomData& meshData, const uint64_t modelKey, const WhiteBoxMaterial& material)
    {
        if (!CreateLodAsset(meshData))
        {
            return false;
        }

        AZStd::vector<AZ::Data::Asset<AZ::RPI::MaterialAsset>> materialAssets;
        if (!CreateMaterialInstances(material, materialAssets) || !CreateModelAsset(materialAssets))
        {
            return false;
        }

        if (!CreateModel())
        {
            return false;
        }

        m_vertexCount = meshData.VertexCount();

        // Offer the model to the next White Box with the same geometry.
        auto shared = AZStd::make_shared<SharedModel>();
        shared->m_modelAsset = m_modelAsset;
        shared->m_model = m_model;
        shared->m_vertexCount = meshData.VertexCount();
        m_sharedModel = shared;
        auto& models = SharedModels();
        if (models.size() >= 64 && models.size() % 64 == 0)
        {
            for (auto it = models.begin(); it != models.end();)
            {
                it = it->second.expired() ? models.erase(it) : AZStd::next(it);
            }
        }
        models[modelKey] = shared;

        return true;
    }

    bool AtomRenderMesh::UseSharedModel(const AZStd::shared_ptr<SharedModel>& shared, const WhiteBoxMaterial& material)
    {
        const auto materialStart = AZStd::chrono::steady_clock::now();
        AZStd::vector<AZ::Data::Asset<AZ::RPI::MaterialAsset>> materialAssets;
        if (!CreateMaterialInstances(material, materialAssets))
        {
            return false;
        }
        const auto acquireStart = AZStd::chrono::steady_clock::now();
        Stats().m_materialSeconds += AZStd::chrono::duration<double>(acquireStart - materialStart).count();

        m_modelAsset = shared->m_modelAsset;
        m_model = shared->m_model;
        if (!AcquireMesh())
        {
            return false;
        }
        Stats().m_acquireSeconds += AZStd::chrono::duration<double>(AZStd::chrono::steady_clock::now() - acquireStart).count();

        // The buffers of an earlier build of this mesh are no longer needed: the shared model owns its own.
        m_indexBuffer.reset();
        m_attributes = {};
        m_vertexCount = 0;
        m_sharedModel = shared;
        return true;
    }

    bool AtomRenderMesh::DoesMeshRequireFullRebuild([[maybe_unused]] const WhiteBoxMeshAtomData& meshData) const
    {
        // this has been disabled due to a some recent updates with Atom that a) cause visual artefacts
        // when updating the buffers and b) have a big performance boost when rebuilding from scratch anyway.
        //
        // this method for building the mesh will probably be replace anyway when the Atom DynamicDraw support
        // comes online.
        return true; // meshData.VertexCount() != m_vertexCount;
    }

    void AtomRenderMesh::BuildMesh(const WhiteBoxRenderData& renderData, const AZ::Transform& worldFromLocal)
    {
        const auto methodStart = AZStd::chrono::steady_clock::now();
        const WhiteBoxFaces culledFaceList = BuildCulledWhiteBoxFaces(renderData.m_faces);
        m_materialGroups.clear();
        AZStd::vector<WhiteBoxFaces> facesByMaterial;
        const AZ::Data::AssetId defaultMaterial = renderData.m_material.m_materialAsset.GetId().IsValid()
            ? renderData.m_material.m_materialAsset.GetId() : m_materialAssetOverride;
        AZ::Render::MaterialAssignmentMap overrides;
        if (m_isPrimary)
        {
            AZ::Render::MaterialComponentRequestBus::EventResult(
                overrides, m_entityId, &AZ::Render::MaterialComponentRequests::GetMaterialMapCopy);
        }
        for (const WhiteBoxFace& face : culledFaceList)
        {
            const AZ::Data::AssetId material = face.m_materialAsset.GetId().IsValid()
                ? face.m_materialAsset.GetId() : defaultMaterial;
            size_t group = 0;
            while (group < m_materialGroups.size() &&
                (m_materialGroups[group].m_assetId != material || m_materialGroups[group].m_paintColor != face.m_paintColor))
            {
                ++group;
            }
            if (group == m_materialGroups.size())
            {
                MaterialGroup newGroup;
                newGroup.m_assetId = material;
                newGroup.m_paintColor = face.m_paintColor;
                newGroup.m_stableId = MaterialSlotStableId(material, face.m_paintColor);
                // Probe past the invalid id and the (rare) hash collision.
                while (newGroup.m_stableId == AZ::RPI::ModelMaterialSlot::InvalidStableId ||
                       AZStd::any_of(m_materialGroups.begin(), m_materialGroups.end(),
                           [&newGroup](const MaterialGroup& other) { return other.m_stableId == newGroup.m_stableId; }))
                {
                    ++newGroup.m_stableId;
                }
                m_materialGroups.push_back(AZStd::move(newGroup));
                facesByMaterial.emplace_back();
            }
            facesByMaterial[group].push_back(face);
        }
        for (size_t group = 0; group < m_materialGroups.size(); ++group)
        {
            // Only the built-in material reads COLOR0 as a tint; a custom one (a vertex-blend material) gets the weights.
            const bool colorFromBlend =
                m_materialGroups[group].m_assetId.IsValid() || HasAssetOverride(overrides, m_materialGroups[group].m_stableId);
            for (WhiteBoxFace& face : facesByMaterial[group])
            {
                face.m_colorFromBlend = colorFromBlend;
            }
        }
        WhiteBoxFaces groupedFaces;
        groupedFaces.reserve(culledFaceList.size());
        for (size_t group = 0; group < m_materialGroups.size(); ++group)
        {
            m_materialGroups[group].m_indexOffset = static_cast<uint32_t>(groupedFaces.size() * 3);
            m_materialGroups[group].m_indexCount = static_cast<uint32_t>(facesByMaterial[group].size() * 3);
            groupedFaces.insert(groupedFaces.end(), facesByMaterial[group].begin(), facesByMaterial[group].end());
        }
        if (groupedFaces.empty())
        {
            if (m_meshFeatureProcessor && m_meshHandle.IsValid())
            {
                m_meshFeatureProcessor->ReleaseMesh(m_meshHandle);
            }
            m_modelAsset = {};
            m_sharedModel.reset();
            PublishMaterialSlots();
            return;
        }
        const auto buildStart = AZStd::chrono::steady_clock::now();
        const double groupSeconds = AZStd::chrono::duration<double>(buildStart - methodStart).count();

        // Identical geometry and material layout draws with one shared model (see SharedModel).
        KeyHasher key;
        key.Add(groupedFaces.size());
        key.Add(renderData.m_material.m_lightmapUvs);
        key.Add(renderData.m_material.m_lightmapMargin);
        for (const MaterialGroup& group : m_materialGroups)
        {
            key.Add(group.m_assetId.m_guid);
            key.Add(group.m_assetId.m_subId);
            key.Add(group.m_paintColor);
            key.Add(group.m_stableId);
            key.Add(group.m_indexOffset);
            key.Add(group.m_indexCount);
        }
        for (const WhiteBoxFace& face : groupedFaces)
        {
            key.Add(face.m_v1);
            key.Add(face.m_v2);
            key.Add(face.m_v3);
            key.Add(face.m_color);
            key.Add(face.m_colorFromBlend);
            key.Add(face.m_hasLightmapUv);
        }
        const AZ::u64 modelKey = key.m_hash;
        const double keySeconds = AZStd::chrono::duration<double>(AZStd::chrono::steady_clock::now() - buildStart).count();

        const auto secondsSince = [](const AZStd::chrono::steady_clock::time_point start)
        {
            return AZStd::chrono::duration<double>(AZStd::chrono::steady_clock::now() - start).count();
        };

        if (wb_shareRenderModels)
        {
            const auto found = SharedModels().find(modelKey);
            if (found != SharedModels().end())
            {
                if (AZStd::shared_ptr<SharedModel> shared = found->second.lock(); shared && UseSharedModel(shared, renderData.m_material))
                {
                    PublishMaterialSlots();
                    UpdateTransform(worldFromLocal);
                    Stats().m_shared++;
                    Stats().m_sharedSeconds += secondsSince(methodStart);
                    Stats().m_groupSeconds += groupSeconds;
                    Stats().m_keySeconds += keySeconds;
                    return;
                }
            }
        }

        if (renderData.m_material.m_lightmapUvs)
        {
            GenerateLightmapUvs(groupedFaces, renderData.m_material.m_lightmapMargin);
        }
        const WhiteBoxMeshAtomData meshData(groupedFaces);

        if (DoesMeshRequireFullRebuild(meshData))
        {
            if (!CreateMesh(meshData, modelKey, renderData.m_material))
            {
                return;
            }
            PublishMaterialSlots();
            Stats().m_built++;
            Stats().m_builtSeconds += secondsSince(methodStart);
        }
        else
        {
            if (!UpdateMeshBuffers(meshData))
            {
                return;
            }
        }

        UpdateTransform(worldFromLocal);
    }

    void AtomRenderMesh::UpdateTransform(const AZ::Transform& worldFromLocal)
    {
        // An empty white box never creates a model, so the feature processor may be missing.
        if (m_meshFeatureProcessor)
        {
            // Pass Non-Uniform Scale separately (the transform carries uniform scale only).
            AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
            AZ::NonUniformScaleRequestBus::EventResult(
                nonUniformScale, m_entityId, &AZ::NonUniformScaleRequests::GetScale);
            m_meshFeatureProcessor->SetTransform(m_meshHandle, worldFromLocal, nonUniformScale);
        }
    }

    void AtomRenderMesh::UpdateMaterial(const WhiteBoxMaterial& material)
    {
        // A group whose settings changed moves to the instance made for the new settings, so the others sharing
        // the old one are left alone.
        bool changed = false;
        for (MaterialGroup& group : m_materialGroups)
        {
            const AZ::u64 key = MaterialInstanceKey(group.m_assetId, group.m_paintColor, material);
            if (group.m_instance && key == group.m_materialKey)
            {
                continue;
            }
            if (AZStd::shared_ptr<SharedMaterial> shared = AcquireSharedMaterial(group.m_assetId, group.m_paintColor, material))
            {
                group.m_shared = shared;
                group.m_instance = shared->m_instance;
                group.m_customMaterial = shared->m_custom;
                group.m_materialKey = key;
                changed = true;
            }
        }
        if (changed)
        {
            AZ::Render::MaterialAssignmentMap overrides;
            if (m_isPrimary)
            {
                AZ::Render::MaterialComponentRequestBus::EventResult(
                    overrides, m_entityId, &AZ::Render::MaterialComponentRequests::GetMaterialMapCopy);
            }
            BindCustomMaterials(overrides, false);
        }
        OnTick(0.0f, AZ::ScriptTimePoint{});
    }

    // Bytes one vertex takes across the seven streams (position, normal, tangent, bitangent, UV, colour, lightmap UV),
    // plus its entry in the index buffer.
    constexpr AZ::u64 VertexStreamBytes = 12 + 12 + 16 + 12 + 8 + 16 + 8;
    constexpr AZ::u64 IndexBytes = 4;

    void AtomRenderMesh::PrintRenderStats()
    {
        AZ::u32 meshes = 0;
        AZ::u32 visible = 0;
        AZ::u64 submittedTriangles = 0;
        AZ::u64 unsharedBytes = 0; // what the buffers would take if no two meshes shared a model
        AZ::u32 drawCalls = 0;
        AZStd::unordered_set<const SharedModel*> uniqueModels;
        AZ::u64 sharedBytes = 0;
        AZ::u64 uniqueTriangles = 0;
        for (const AtomRenderMesh* mesh : LiveMeshes())
        {
            if (!mesh->m_modelAsset.GetId().IsValid() || mesh->m_materialGroups.empty())
            {
                continue;
            }
            meshes++;
            AZ::u64 triangles = 0;
            for (const MaterialGroup& group : mesh->m_materialGroups)
            {
                triangles += group.m_indexCount / 3;
            }
            const AZ::u64 vertices = triangles * 3;
            unsharedBytes += vertices * (VertexStreamBytes + IndexBytes);
            if (mesh->m_visible)
            {
                visible++;
                submittedTriangles += triangles;
                drawCalls += static_cast<AZ::u32>(mesh->m_materialGroups.size());
            }
            if (mesh->m_sharedModel && uniqueModels.insert(mesh->m_sharedModel.get()).second)
            {
                sharedBytes += static_cast<AZ::u64>(mesh->m_sharedModel->m_vertexCount) * (VertexStreamBytes + IndexBytes);
                uniqueTriangles += mesh->m_sharedModel->m_vertexCount / 3;
            }
        }
        AZ_Printf(
            "WhiteBox",
            "Render stats: %u meshes (%u visible), %u draw calls, %llu triangles submitted per view; "
            "%zu unique models hold %llu triangles; GPU buffers %.2f MiB shared vs %.2f MiB if unshared",
            meshes, visible, drawCalls, static_cast<unsigned long long>(submittedTriangles), uniqueModels.size(),
            static_cast<unsigned long long>(uniqueTriangles), sharedBytes / (1024.0 * 1024.0), unsharedBytes / (1024.0 * 1024.0));
    }

    void AtomRenderMesh::SetAllVisible(const bool visible)
    {
        for (const AtomRenderMesh* mesh : LiveMeshes())
        {
            const_cast<AtomRenderMesh*>(mesh)->SetVisiblity(visible);
        }
    }

    namespace
    {
        void PrintRenderStatsCommand(const AZ::ConsoleCommandContainer&)
        {
            AtomRenderMesh::PrintRenderStats();
        }
        AZ_CONSOLEFREEFUNC("wb_renderStats", PrintRenderStatsCommand, AZ::ConsoleFunctorFlags::Null, "Print what every live White Box render mesh submits and stores.");

        void SetRenderVisibleCommand(const AZ::ConsoleCommandContainer& arguments)
        {
            const bool visible = arguments.empty() || arguments[0] != "0";
            AtomRenderMesh::SetAllVisible(visible);
        }
        AZ_CONSOLEFREEFUNC("wb_renderVisible", SetRenderVisibleCommand, AZ::ConsoleFunctorFlags::Null, "Show (1, default) or hide (0) every live White Box render mesh, to compare frame times.");
    } // namespace

    void AtomRenderMesh::SetMaterialAssetOverride(const AZ::Data::AssetId& materialAssetId)
    {
        m_materialAssetOverride = materialAssetId;
    }

    void AtomRenderMesh::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        bool compiled = true;
        for (MaterialGroup& group : m_materialGroups)
        {
            if (group.m_instance && group.m_instance->NeedsCompile() && !group.m_instance->Compile())
            {
                compiled = false;
            }
        }
        if (compiled)
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
        else if (!AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void AtomRenderMesh::SetVisiblity(bool visibility)
    {
        m_visible = visibility;
        // No feature processor means no model was ever built (empty white box) - nothing to show.
        if (m_meshFeatureProcessor)
        {
            m_meshFeatureProcessor->SetVisible(m_meshHandle, m_visible);
        }
    }

    bool AtomRenderMesh::IsVisible() const
    {
        return m_visible;
    }

    const AZ::Render::MeshFeatureProcessorInterface::MeshHandle* AtomRenderMesh::GetMeshHandle() const
    {
        return &m_meshHandle;
    }

    AZ::Render::CustomMaterialMap AtomRenderMesh::BuildCustomMaterials(const AZ::Render::MaterialAssignmentMap& overrides) const
    {
        AZ::Render::CustomMaterialMap materials;
        for (const MaterialGroup& group : m_materialGroups)
        {
            const AZ::Render::MaterialAssignment& assignment = AZ::Render::GetMaterialAssignmentFromMapWithFallback(
                overrides, AZ::Render::MaterialAssignmentId::CreateFromLodAndStableId(0, group.m_stableId));
            materials[{ AZ::Render::DefaultCustomMaterialLodIndex, group.m_stableId }] = assignment.m_materialInstance
                ? AZ::Render::CustomMaterialInfo{ assignment.m_materialInstance, assignment.m_matModUvOverrides }
                : AZ::Render::CustomMaterialInfo{ group.m_instance, {} };
        }
        return materials;
    }

    void AtomRenderMesh::PublishMaterialSlots()
    {
        if (!m_isPrimary)
        {
            return;
        }
        AZStd::vector<AZ::u32> slots;
        if (m_modelAsset.IsReady())
        {
            for (const MaterialGroup& group : m_materialGroups)
            {
                slots.push_back(group.m_stableId);
            }
        }
        AZStd::sort(slots.begin(), slots.end());
        // Every edit rebuilds the model, but a Material component only reloads when the slots themselves change.
        if (slots != m_publishedSlots)
        {
            m_publishedSlots = AZStd::move(slots);
            m_reloadRequested.clear();
            AZ::Render::MaterialConsumerNotificationBus::Event(
                m_entityId, &AZ::Render::MaterialConsumerNotificationBus::Events::OnMaterialAssignmentSlotsChanged);
        }
    }

    AZ::Render::MaterialAssignmentId AtomRenderMesh::FindMaterialAssignmentId(
        const AZ::Render::MaterialAssignmentLodIndex lod, const AZStd::string& label) const
    {
        return AZ::Render::GetMaterialSlotIdFromModelAsset(m_modelAsset, lod, label);
    }

    AZ::Render::MaterialAssignmentLabelMap AtomRenderMesh::GetMaterialLabels() const
    {
        return AZ::Render::GetMaterialSlotLabelsFromModelAsset(m_modelAsset);
    }

    AZ::Render::MaterialAssignmentMap AtomRenderMesh::GetDefaultMaterialMap() const
    {
        return AZ::Render::GetDefaultMaterialMapFromModelAsset(m_modelAsset);
    }

    AZStd::unordered_set<AZ::Name> AtomRenderMesh::GetModelUvNames() const
    {
        return m_model ? m_model->GetUvNames() : AZStd::unordered_set<AZ::Name>();
    }

    void AtomRenderMesh::RememberBoundMaterials(const AZ::Render::CustomMaterialMap& materials)
    {
        m_boundMaterials.clear();
        for (const auto& [id, info] : materials)
        {
            m_boundMaterials[id.second] = info.m_material.get();
        }
    }

    void AtomRenderMesh::BindCustomMaterials(const AZ::Render::MaterialAssignmentMap& overrides, const bool force)
    {
        if (!m_meshFeatureProcessor || !m_meshHandle.IsValid())
        {
            return;
        }
        const AZ::Render::CustomMaterialMap materials = BuildCustomMaterials(overrides);
        bool changed = force || materials.size() != m_boundMaterials.size();
        for (const auto& [id, info] : materials)
        {
            const auto bound = m_boundMaterials.find(id.second);
            changed = changed || bound == m_boundMaterials.end() || bound->second != info.m_material.get();
        }
        if (changed)
        {
            RememberBoundMaterials(materials);
            m_meshFeatureProcessor->SetCustomMaterials(m_meshHandle, materials);
        }
    }

    void AtomRenderMesh::ReloadOverridesWithoutInstance(const AZ::Render::MaterialAssignmentMap& overrides)
    {
        using AZ::Render::MaterialAssignmentId;
        for (const MaterialGroup& group : m_materialGroups)
        {
            for (const MaterialAssignmentId& id :
                 { MaterialAssignmentId::CreateFromLodAndStableId(0, group.m_stableId), MaterialAssignmentId::CreateFromStableIdOnly(group.m_stableId) })
            {
                const auto it = overrides.find(id);
                // A first property edit on an untouched slot makes an entry that knows no asset, so it never gets an instance.
                if (it == overrides.end() || it->second.m_materialInstance || it->second.m_defaultMaterialAsset.GetId().IsValid() ||
                    (it->second.m_propertyOverrides.empty() && !it->second.m_materialAsset.GetId().IsValid()) ||
                    !m_reloadRequested.insert(group.m_stableId).second)
                {
                    continue;
                }
                // Re-setting the entry's asset makes the Material component reload and fill in the slot default.
                // Deferred: the component is mid-notification.
                const AZ::EntityId entityId = m_entityId;
                const AZ::Data::AssetId asset = it->second.m_materialAsset.GetId();
                AZ::TickBus::QueueFunction(
                    [entityId, id, asset]()
                    {
                        AZ::Render::MaterialComponentRequestBus::Event(
                            entityId, &AZ::Render::MaterialComponentRequests::SetMaterialAssetId, id, asset);
                    });
                return; // one reload covers every slot
            }
        }
    }

    void AtomRenderMesh::OnMaterialsUpdated(const AZ::Render::MaterialAssignmentMap& materials)
    {
        BindCustomMaterials(materials, true);
        ReloadOverridesWithoutInstance(materials);
    }

    void AtomRenderMesh::OnMaterialPropertiesUpdated(const AZ::Render::MaterialAssignmentMap& materials)
    {
        // Property edits normally land on instances already bound, but a slot whose instance was just rebuilt only
        // reaches the mesh here: the Material component holds OnMaterialsUpdated back while any other slot is pending.
        BindCustomMaterials(materials, false);
        ReloadOverridesWithoutInstance(materials);
        if (m_meshFeatureProcessor && m_meshHandle.IsValid())
        {
            m_meshFeatureProcessor->SetRayTracingDirty(m_meshHandle); // ray tracing keeps its own copy of material data
        }
    }
} // namespace WhiteBox
