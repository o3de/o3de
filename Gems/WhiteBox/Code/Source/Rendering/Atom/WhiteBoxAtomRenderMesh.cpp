/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxAtomRenderMesh.h"

#include <Rendering/Atom/WhiteBoxMeshAtomData.h>
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
#include <AzCore/Math/PackedVector3.h>

namespace WhiteBox
{
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

    void AtomRenderMesh::AddMeshBuffers(AZ::RPI::ModelLodAssetCreator& modelLodCreator)
    {
        modelLodCreator.SetMeshIndexBuffer(m_indexBuffer->GetBufferAssetView());

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
        modelLodCreator.BeginMesh();
        modelLodCreator.SetMeshAabb(meshData.GetAabb());
        
        modelLodCreator.SetMeshMaterialSlot(OneMaterialSlotId);

        AddMeshBuffers(modelLodCreator);
        modelLodCreator.EndMesh();

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
        
        if (auto materialAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::MaterialAsset>(TexturedMaterialPath.data()))
        {
            AZ::RPI::ModelMaterialSlot materialSlot;
            materialSlot.m_stableId = OneMaterialSlotId;
            materialSlot.m_defaultMaterialAsset = materialAsset;
            modelCreator.AddMaterialSlot(materialSlot);
        }
        else
        {
            AZ_Error("CreateLodAsset", false, "Could not load material.");
            return;
        }

        modelCreator.End(m_modelAsset);
    }

    bool AtomRenderMesh::CreateModel(AZ::EntityId entityId)
    {
        m_model = AZ::RPI::Model::FindOrCreate(m_modelAsset);
        m_meshFeatureProcessor =
            AZ::RPI::Scene::GetFeatureProcessorForEntity<AZ::Render::MeshFeatureProcessorInterface>(entityId);

        if (!m_meshFeatureProcessor)
        {
            AZ_Error(
                "MeshComponentController", m_meshFeatureProcessor,
                "Unable to find a MeshFeatureProcessorInterface on the entityId.");
            return false;
        }

        m_meshFeatureProcessor->ReleaseMesh(m_meshHandle);
        m_meshHandle = m_meshFeatureProcessor->AcquireMesh(AZ::Render::MeshHandleDescriptor{ m_modelAsset });
        return true;
    }

    bool AtomRenderMesh::MeshRequiresFullRebuild([[maybe_unused]] const WhiteBoxMeshAtomData& meshData) const
    {
        return meshData.VertexCount() != m_vertexCount;
    }

    bool AtomRenderMesh::CreateMesh(const WhiteBoxMeshAtomData& meshData, AZ::EntityId entityId)
    {
        if (!CreateLodAsset(meshData))
        {
            // TODO: LYN-808
            return false;
        }

        CreateModelAsset();

        if (!CreateModel(entityId))
        {
            // TODO: LYN-808
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

    void AtomRenderMesh::BuildMesh(
        const WhiteBoxRenderData& renderData, const AZ::Transform& worldFromLocal, AZ::EntityId entityId)
    {
        const WhiteBoxFaces culledFaceList = BuildCulledWhiteBoxFaces(renderData.m_faces);
        const WhiteBoxMeshAtomData meshData(culledFaceList);

        if (DoesMeshRequireFullRebuild(meshData))
        {
            if (!CreateMesh(meshData, entityId))
            {
                // TODO: LYN-808
                return;
            }
        }
        else
        {
            if (!UpdateMeshBuffers(meshData))
            {
                // TODO: LYN-808
                return;
            }
        }

        UpdateTransform(worldFromLocal);
    }

    void AtomRenderMesh::UpdateTransform(const AZ::Transform& worldFromLocal)
    {
        m_meshFeatureProcessor->SetTransform(m_meshHandle, worldFromLocal);
    }

    void AtomRenderMesh::UpdateMaterial([[maybe_unused]] const WhiteBoxMaterial& material)
    {
        // TODO: LYN-784
        // colors: vertex colors probs not used.
        // (use constant color for material -> material editor)
        //
        // m_meshFeatureProcessor->SetMaterialAssignmentMap() (not in spectra mainline yet)
        //
        // for now:
        // m_meshFeatureProcessor->ReleaseMesh(m_meshHandle);
        // MaterialAssignmentMap materialMap;
        // materialMap[DefaultMaterialAssignmentId] = (see below)
        // m_meshHandle = m_meshFeatureProcessor->AcquireMesh(m_modelAsset, materialMap)

        // from tommy (SkinnedMeshContainer.cpp)
        /*
            auto materialAsset =
           AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::MaterialAsset>(SkinnedMeshMaterial); auto
           materialOverrideInstance = AZ::RPI::Material::FindOrCreate(materialAsset);

            AZ::Render::MaterialAssignmentMap materialMap;
            materialMap[AZ::Render::DefaultMaterialAssignmentId].m_materialAsset = materialAsset;
            materialMap[AZ::Render::DefaultMaterialAssignmentId].m_materialInstance = materialOverrideInstance;
        */

        /*
            changing material properties:

            (todo)
        */
    }

    bool AtomRenderMesh::IsVisible() const
    {
        // TODO: LYN-788
        return true;
    }

    void AtomRenderMesh::SetVisiblity([[maybe_unused]] bool visibility)
    {
        // TODO: LYN-788
        // hide: m_meshFeatureProcessor->ReleaseMesh(m_meshHandle);
        // show: m_meshHandle = m_meshFeatureProcessor->AcquireMesh(m_modelAsset);
    }
} // namespace WhiteBox
