/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Draw/DrawableMeshEntity.h>

#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Model/ModelLodUtils.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace AZ::Render
{
    DrawableMeshEntity::DrawableMeshEntity(EntityId entityId, Data::Instance<RPI::Material> maskMaterial, Name drawList)
        : m_entityId(entityId)
        , m_maskMaterial(maskMaterial)
        , m_drawList(drawList)
    {
        AZ::Render::MeshHandleStateNotificationBus::Handler::BusConnect(m_entityId);
    }

    DrawableMeshEntity::~DrawableMeshEntity()
    {
        AZ::Render::MeshHandleStateNotificationBus::Handler::BusDisconnect();
    }

    void DrawableMeshEntity::ClearDrawData()
    {
        m_modelLodIndex = RPI::ModelLodIndex::Null;
        m_meshDrawPackets.clear();
    }

    void DrawableMeshEntity::Draw(RPI::View& view)
    {
        if (!m_meshHandle || !m_meshHandle->IsValid())
        {
            return;
        }
        auto* scene = RPI::Scene::GetSceneForEntityId(m_entityId);
        auto* meshes = scene->GetFeatureProcessor<MeshFeatureProcessorInterface>();
        const auto model = meshes->GetModel(*m_meshHandle);
        if (!model)
        {
            return;
        }
        if (const auto lod = GetModelLodIndex(view, model); lod != m_modelLodIndex)
        {
            CreateOrUpdateMeshDrawPackets(meshes, lod, model);
        }

        for (auto& drawPacket : m_meshDrawPackets)
        {
            drawPacket.Update(*scene);
            if (auto* rhiDrawPacket = drawPacket.GetRHIDrawPacket();
                rhiDrawPacket != nullptr)
            {
                view.AddDrawPacket(rhiDrawPacket);
            }
        }
    }

    RPI::ModelLodIndex DrawableMeshEntity::GetModelLodIndex(const RPI::View& view, Data::Instance<RPI::Model> model) const
    {
        const auto worldTM = AzToolsFramework::GetWorldTransform(m_entityId);
        return RPI::ModelLodUtils::SelectLod(&view, worldTM, *model);
    }

    void DrawableMeshEntity::OnMeshHandleSet(const MeshFeatureProcessorInterface::MeshHandle* meshHandle)
    {
        m_meshHandle = meshHandle;
        ClearDrawData();
    }

    void DrawableMeshEntity::CreateOrUpdateMeshDrawPackets(
        const MeshFeatureProcessorInterface* featureProcessor, const RPI::ModelLodIndex modelLodIndex, Data::Instance<RPI::Model> model)
    {
        ClearDrawData();

        const auto maskMeshObjectSrg = CreateMaskShaderResourceGroup(featureProcessor);
        m_modelLodIndex = modelLodIndex;
        BuildMeshDrawPackets(model->GetModelAsset(), maskMeshObjectSrg);
    }

    void DrawableMeshEntity::BuildMeshDrawPackets(
        const Data::Asset<RPI::ModelAsset> modelAsset, Data::Instance<RPI::ShaderResourceGroup> meshObjectSrg)
    {
        const auto modelLodAssets = modelAsset->GetLodAssets();
        const Data::Asset<RPI::ModelLodAsset>& modelLodAsset = modelLodAssets[m_modelLodIndex.m_index];
        Data::Instance<RPI::ModelLod> modelLod = RPI::ModelLod::FindOrCreate(modelLodAsset, modelAsset).get();

        for (size_t i = 0; i < modelLod->GetMeshes().size(); i++)
        {
            EditorStateMeshDrawPacket drawPacket(*modelLod, i, m_maskMaterial, m_drawList, meshObjectSrg);
            m_meshDrawPackets.emplace_back(drawPacket);
        }
    }

    Data::Instance<RPI::ShaderResourceGroup> DrawableMeshEntity::CreateMaskShaderResourceGroup(
        const MeshFeatureProcessorInterface* featureProcessor) const
    {
        const auto& shaderAsset = m_maskMaterial->GetAsset()->GetMaterialTypeAsset()->GetShaderAssetForObjectSrg();
        const auto& objectSrgLayout = m_maskMaterial->GetAsset()->GetObjectSrgLayout();
        const auto maskMeshObjectSrg = RPI::ShaderResourceGroup::Create(shaderAsset, objectSrgLayout->GetName());

        // Set the object id so the correct MVP matrices can be selected in the shader
        const auto objectId = featureProcessor->GetObjectId(*m_meshHandle).GetIndex();
        RHI::ShaderInputNameIndex objectIdIndex = "m_objectId";
        maskMeshObjectSrg->SetConstant(objectIdIndex, objectId);
        maskMeshObjectSrg->Compile();

        return maskMeshObjectSrg;
    }
} // namespace AZ::Render
