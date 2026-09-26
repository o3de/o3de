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
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/PackedVector3.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/sort.h>

namespace WhiteBox
{
    namespace
    {
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

    void AtomRenderMesh::CreateModelAsset()
    {
        AZ::RPI::ModelAssetCreator modelCreator;
        modelCreator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        modelCreator.SetName(ModelName);
        modelCreator.AddLodAsset(AZStd::move(m_lodAsset));
        
        for (uint32_t slot = 0; slot < m_materialGroups.size(); ++slot)
        {
            MaterialGroup& group = m_materialGroups[slot];
            AZ::Data::Asset<AZ::RPI::MaterialAsset> materialAsset;
            if (group.m_assetId.IsValid())
            {
                materialAsset = AZ::RPI::AssetUtils::LoadAssetById<AZ::RPI::MaterialAsset>(
                    group.m_assetId, AZ::RPI::AssetUtils::TraceLevel::Warning);
            }
            group.m_customMaterial = static_cast<bool>(materialAsset);
            if (!materialAsset)
            {
                materialAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::MaterialAsset>(TexturedMaterialPath.data());
            }
            if (!materialAsset)
            {
                AZ_Error("WhiteBox", false, "Could not load WhiteBox material.");
                return;
            }
            group.m_instance = AZ::RPI::Material::Create(materialAsset);
            AZ::RPI::ModelMaterialSlot materialSlot;
            materialSlot.m_stableId = group.m_stableId;
            materialSlot.m_displayName = AZ::Name(SlotLabel(group.m_assetId, group.m_paintColor));
            materialSlot.m_defaultMaterialAsset = materialAsset;
            modelCreator.AddMaterialSlot(materialSlot);
        }

        modelCreator.End(m_modelAsset);
    }

    bool AtomRenderMesh::CreateModel()
    {
        m_model = AZ::RPI::Model::FindOrCreate(m_modelAsset);
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

    bool AtomRenderMesh::CreateMesh(const WhiteBoxMeshAtomData& meshData)
    {
        if (!CreateLodAsset(meshData))
        {
            return false;
        }

        CreateModelAsset();

        if (!CreateModel())
        {
            return false;
        }

        m_vertexCount = meshData.VertexCount();

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
            PublishMaterialSlots();
            return;
        }
        if (renderData.m_material.m_lightmapUvs)
        {
            GenerateLightmapUvs(groupedFaces, renderData.m_material.m_lightmapMargin);
        }
        const WhiteBoxMeshAtomData meshData(groupedFaces);

        if (DoesMeshRequireFullRebuild(meshData))
        {
            if (!CreateMesh(meshData))
            {
                return;
            }
            PublishMaterialSlots();
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
        for (MaterialGroup& group : m_materialGroups)
        {
            // Custom assets keep their authored color and texture settings.
            auto& materialInstance = group.m_instance;
            if (!materialInstance || (group.m_customMaterial && group.m_paintColor == 0))
            {
                continue;
            }
            // Per-layer tint travels in COLOR0, so the base colour must be white; global tint uses baseColor.
            const bool painted = group.m_paintColor != 0;
            const bool useVertexColor = material.m_useVertexColor && !painted;
            const AZ::Color baseColor = painted
                ? AZ::Color(
                    float(group.m_paintColor & 255) / 255.0f,
                    float((group.m_paintColor >> 8) & 255) / 255.0f,
                    float((group.m_paintColor >> 16) & 255) / 255.0f, 1.0f)
                : (useVertexColor ? AZ::Color(1.0f, 1.0f, 1.0f, 1.0f) : AZ::Color(material.m_tint));
            if (const auto& materialPropertyIndex = materialInstance->FindPropertyIndex(AZ::Name("baseColor.color"));
                materialPropertyIndex.IsValid())
            {
                materialInstance->SetPropertyValue(materialPropertyIndex, baseColor);
            }

            // Enable StandardPBR's vertex colour group; the property id varies by engine version.
            for (const char* enableName :
                 {"vertexColor.enable", "vertexColor.useVertexColor", "vertexColor.enableVertexColor",
                  "vertexColor.toggle"})
            {
                if (const auto& idx = materialInstance->FindPropertyIndex(AZ::Name(enableName)); idx.IsValid())
                {
                    materialInstance->SetPropertyValue(idx, useVertexColor);
                    break;
                }
            }
            if (useVertexColor)
            {
                if (const auto& idx = materialInstance->FindPropertyIndex(AZ::Name("vertexColor.factor"));
                    idx.IsValid())
                {
                    materialInstance->SetPropertyValue(idx, 1.0f);
                }
            }

            if (const auto& materialPropertyIndex = materialInstance->FindPropertyIndex(AZ::Name("baseColor.useTexture"));
                materialPropertyIndex.IsValid() && !group.m_customMaterial)
            {
                materialInstance->SetPropertyValue(materialPropertyIndex, material.m_useTexture && !painted);
            }

        }
        OnTick(0.0f, AZ::ScriptTimePoint{});
    }

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
