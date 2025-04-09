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
    AtomRenderMesh::AtomRenderMesh(AZ::EntityId entityId)
        : m_entityId(entityId)
    {
        AZ::Render::MeshHandleStateRequestBus::Handler::BusConnect(m_entityId);
    }

    AtomRenderMesh::~AtomRenderMesh()
    {
        m_materialInstance = {};
        if (m_meshHandle.IsValid() && m_meshFeatureProcessor)
        {
            m_meshFeatureProcessor->ReleaseMesh(m_meshHandle);
            AZ::Render::MeshHandleStateNotificationBus::Event(
                m_entityId, &AZ::Render::MeshHandleStateNotificationBus::Events::OnMeshHandleSet, &m_meshHandle);
        }
        
        AZ::Render::MeshHandleStateRequestBus::Handler::BusDisconnect();
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
            m_materialInstance = AZ::RPI::Material::FindOrCreate(materialAsset);

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
        m_meshHandle = m_meshFeatureProcessor->AcquireMesh(AZ::Render::MeshHandleDescriptor(m_modelAsset, m_materialInstance));
        AZ::Render::MeshHandleStateNotificationBus::Event(m_entityId, &AZ::Render::MeshHandleStateNotificationBus::Events::OnMeshHandleSet, &m_meshHandle);

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
        const WhiteBoxMeshAtomData meshData(culledFaceList);

        if (DoesMeshRequireFullRebuild(meshData))
        {
            if (!CreateMesh(meshData))
            {
                return;
            }
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
        m_meshFeatureProcessor->SetTransform(m_meshHandle, worldFromLocal);
    }

    void AtomRenderMesh::UpdateMaterial(const WhiteBoxMaterial& material)
    {
        if (m_meshFeatureProcessor && m_materialInstance)
        {            
            if (const auto& materialPropertyIndex = m_materialInstance->FindPropertyIndex(AZ::Name("baseColor.color"));
                materialPropertyIndex.IsValid())
            {
                m_materialInstance->SetPropertyValue(materialPropertyIndex, AZ::Color(material.m_tint));
            }

            if (const auto& materialPropertyIndex = m_materialInstance->FindPropertyIndex(AZ::Name("baseColor.useTexture"));
                materialPropertyIndex.IsValid())
            {
                m_materialInstance->SetPropertyValue(materialPropertyIndex, material.m_useTexture);
            }

            // If the material changes were successfully applied then disconnect from the tick bus. Otherwise, make another attempt on the next tick.
            if (!m_materialInstance->NeedsCompile() || m_materialInstance->Compile())
            {
                AZ::TickBus::Handler::BusDisconnect();
            }
            else if (!AZ::TickBus::Handler::BusIsConnected())
            {
                AZ::TickBus::Handler::BusConnect();
            }
        }
    }

    void AtomRenderMesh::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (!m_materialInstance || !m_materialInstance->NeedsCompile() || m_materialInstance->Compile())
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
    }

    void AtomRenderMesh::SetVisiblity(bool visibility)
    {
        m_visible = visibility;
        m_meshFeatureProcessor->SetVisible(m_meshHandle, m_visible);
    }

    bool AtomRenderMesh::IsVisible() const
    {
        return m_visible;
    }

    const AZ::Render::MeshFeatureProcessorInterface::MeshHandle* AtomRenderMesh::GetMeshHandle() const
    {
        return &m_meshHandle;
    }
} // namespace WhiteBox
