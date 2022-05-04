/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <FocusedEntity/FocusedMeshEntity.h>

#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Model/ModelLodUtils.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <AzCore/Component/TransformBus.h>

namespace AZ
{
    namespace Render
    {
        // Gets the view for the specified scene.
        static const RPI::ViewPtr GetViewFromScene(const RPI::Scene* scene)
        {
            const auto viewportContextRequests = RPI::ViewportContextRequests::Get();
            const auto viewportContext = viewportContextRequests->GetViewportContextByScene(scene);
            const RPI::ViewPtr viewPtr = viewportContext->GetDefaultView();
            return viewPtr;
        }

        // Get the world transform for the specified entity.
        static AZ::Transform GetWorldTransformForEntity(EntityId entityId)
        {
            AZ::Transform worldTM;
            AZ::TransformBus::EventResult(worldTM, entityId, &AZ::TransformBus::Events::GetWorldTM);
            return worldTM;
        }

        // Creates the mask shader resource group for the specified mesh.
        static Data::Instance<RPI::ShaderResourceGroup> CreateMaskShaderResourceGroup(
            const MeshFeatureProcessorInterface* featureProcessor,
            const MeshFeatureProcessorInterface::MeshHandle& meshHandle,
            Data::Instance<RPI::Material> maskMaterial)
        {
            const auto& shaderAsset = maskMaterial->GetAsset()->GetMaterialTypeAsset()->GetShaderAssetForObjectSrg();
            const auto& objectSrgLayout = maskMaterial->GetAsset()->GetObjectSrgLayout();
            const auto maskMeshObjectSrg = RPI::ShaderResourceGroup::Create(shaderAsset, objectSrgLayout->GetName());

            // Set the object id so the correct MVP matrices can be selected in the shader
            const auto objectId = featureProcessor->GetObjectId(meshHandle).GetIndex();
            RHI::ShaderInputNameIndex objectIdIndex = "m_objectId";
            maskMeshObjectSrg->SetConstant(objectIdIndex, objectId);
            maskMeshObjectSrg->Compile();

            return maskMeshObjectSrg;
        }

        // Builds the mesh draw packets for the specified model.
        static AZStd::vector<RPI::MeshDrawPacket> BuildMeshDrawPackets(
            const RPI::ModelLodIndex& modelLodIndex,
            const Data::Asset<RPI::ModelAsset> modelAsset,
            Data::Instance<RPI::Material> material,
            Data::Instance<RPI::ShaderResourceGroup> meshObjectSrg)
        {
            AZStd::vector<RPI::MeshDrawPacket> meshDrawPackets;
            const Data::Asset<RPI::ModelLodAsset>& modelLodAsset = modelAsset->GetLodAssets()[modelLodIndex.m_index];
            Data::Instance<RPI::ModelLod> modelLod = RPI::ModelLod::FindOrCreate(modelLodAsset, modelAsset).get();

            for (size_t i = 0; i < modelLod->GetMeshes().size(); i++)
            {
                RPI::MeshDrawPacket drawPacket(*modelLod, i, material, meshObjectSrg);
                meshDrawPackets.emplace_back(drawPacket);
            }

            return meshDrawPackets;
        }

        class DrawableMetaData
        {
        public:
            DrawableMetaData(EntityId entityId)
            {
                m_scene = RPI::Scene::GetSceneForEntityId(entityId);
                m_view = GetViewFromScene(m_scene);
                m_featureProcessor = m_scene->GetFeatureProcessor<MeshFeatureProcessorInterface>();
            }

            RPI::Scene* GetScene() const
            {
                return m_scene;
            }

            const RPI::ViewPtr GetView() const
            {
                return m_view;
            }

            const MeshFeatureProcessorInterface* GetFeatureProcessor() const
            {
                return m_featureProcessor;
            }

        private:
            RPI::Scene* m_scene = nullptr;
            RPI::ViewPtr m_view = nullptr;
            MeshFeatureProcessorInterface* m_featureProcessor = nullptr;
        };

        FocusedEntity::FocusedEntity(EntityId entityId, Data::Instance<RPI::Material> maskMaterial)
            : m_entityId(entityId)
            , m_maskMaterial(maskMaterial)
        {
            AtomBusConnect();
        }

        //FocusedEntity::FocusedEntity(const FocusedEntity& other)
        //{
        //    m_entityId = other.m_entityId;
        //    m_maskMaterial = other.m_maskMaterial;
        //    m_meshHandle = other.m_meshHandle;
        //    m_modelLodIndex = other.m_modelLodIndex;
        //    //m_meshDrawPackets = other.m_meshDrawPackets;
        //
        //    AtomBusConnect();
        //}
        //
        //FocusedEntity::FocusedEntity(FocusedEntity&& other)
        //{
        //    m_entityId = other.m_entityId;
        //    m_maskMaterial = other.m_maskMaterial;
        //    m_meshHandle = other.m_meshHandle;
        //    m_modelLodIndex = other.m_modelLodIndex;
        //    m_meshDrawPackets = AZStd::move(other.m_meshDrawPackets);
        //
        //    //AtomBusSoftConnect();
        //    AtomBusConnect();
        //    other.AtomBusDisconnect();
        //}
        
        FocusedEntity::~FocusedEntity()
        {
            AtomBusDisconnect();
        }

        void FocusedEntity::AtomBusConnect()
        {
            AZ::Render::AtomMeshNotificationBus::Handler::BusConnect(m_entityId);
        }

        //void FocusedEntity::AtomBusSoftConnect()
        //{
        //    m_consumeMeshAcquisition = false;
        //    AtomBusConnect();
        //}

        void FocusedEntity::AtomBusDisconnect()
        {
            AZ::Render::AtomMeshNotificationBus::Handler::BusDisconnect();
        }

        void FocusedEntity::ClearDrawData()
        {
            m_modelLodIndex = RPI::ModelLodIndex::Null;
            m_meshDrawPackets.clear();
        }
        
        bool FocusedEntity::CanDraw() const
        {
            return !m_meshDrawPackets.empty();
        }

        void FocusedEntity::Draw()
        {
            if (!CanDraw())
            {
                AZ_Warning(
                    "EditorModeFeedbackSystemComponent",
                    false,
                    "Attempted to draw entity '%s' but entity has no draw data!",
                    m_entityId.ToString().c_str());
        
                return;
            }
        
            const DrawableMetaData drawabaleMetaData(m_entityId);
            const auto model = drawabaleMetaData.GetFeatureProcessor()->GetModel(*m_meshHandle);
            if (const auto modelLodIndex = GetModelLodIndex(drawabaleMetaData.GetView(), model);
                m_modelLodIndex != modelLodIndex)
            {
                CreateOrUpdateMeshDrawPackets(drawabaleMetaData.GetFeatureProcessor(), modelLodIndex, model);
            }
        
            AZ::RPI::DynamicDrawInterface* dynamicDraw = AZ::RPI::GetDynamicDraw();
            for (auto& drawPacket : m_meshDrawPackets)
            {
                drawPacket.Update(*drawabaleMetaData.GetScene());
                dynamicDraw->AddDrawPacket(drawabaleMetaData.GetScene(), drawPacket.GetRHIDrawPacket());
            }
        }
         
        RPI::ModelLodIndex FocusedEntity::GetModelLodIndex(const RPI::ViewPtr view, Data::Instance<RPI::Model> model) const
        {
            const auto worldTM = GetWorldTransformForEntity(m_entityId);
            return RPI::ModelLodUtils::SelectLod(view.get(), worldTM, *model);
        }
        
        void FocusedEntity::OnAcquireMesh(const MeshFeatureProcessorInterface::MeshHandle* meshHandle)
        {
            //if (!m_consumeMeshAcquisition)
            //{
            //    m_consumeMeshAcquisition = true;
            //    return;
            //}

            AZ_Printf("OnAcquireMesh", "%s", m_entityId.ToString().c_str());

            m_meshHandle = meshHandle;
            const auto scene = RPI::Scene::GetSceneForEntityId(m_entityId);
            const auto view = GetViewFromScene(scene);
            const auto* featureProcessor = scene->GetFeatureProcessor<MeshFeatureProcessorInterface>();
            const auto model = featureProcessor->GetModel(*m_meshHandle);
            const auto modelLodIndex = GetModelLodIndex(view, model);
            CreateOrUpdateMeshDrawPackets(featureProcessor, modelLodIndex, model);
        }
        
        void FocusedEntity::CreateOrUpdateMeshDrawPackets(
            const MeshFeatureProcessorInterface* featureProcessor, const RPI::ModelLodIndex modelLodIndex, Data::Instance<RPI::Model> model)
        {
            if (!m_meshHandle || !m_meshHandle->IsValid())
            {
                return;
            }
        
            ClearDrawData();
        
            // Build the mesh draw packets for this mesh if no draw packets currently exist or if the LoD for the
            // draw packets no longer matches the loD of the model
            if (m_meshHandle->IsValid())
            {
                // The id value to write to the mask texture for this entity (unused in the current use case)
                const auto maskMeshObjectSrg =
                    CreateMaskShaderResourceGroup(featureProcessor, *m_meshHandle, m_maskMaterial);
                m_modelLodIndex = modelLodIndex;
        
                m_meshDrawPackets = BuildMeshDrawPackets(
                    m_modelLodIndex,
                    model->GetModelAsset(),
                    m_maskMaterial,
                    maskMeshObjectSrg);
            }
        }
    } // namespace Render
} // namespace AZ
