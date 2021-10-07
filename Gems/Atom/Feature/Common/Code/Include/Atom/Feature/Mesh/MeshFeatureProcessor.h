/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Culling.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/Shader/ShaderSystemInterface.h>
#include <Atom/Feature/Material/MaterialAssignment.h>
#include <Atom/Feature/TransformService/TransformServiceFeatureProcessor.h>
#include <Atom/Feature/Mesh/ModelReloaderSystemInterface.h>
#include <RayTracing/RayTracingFeatureProcessor.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AtomCore/std/parallel/concurrency_checker.h>
#include <AzCore/Console/Console.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

#include <AzCore/Component/TickBus.h>

namespace AZ
{
    namespace Render
    {
        class TransformServiceFeatureProcessor;
        class RayTracingFeatureProcessor;

        class MeshDataInstance
        {
            friend class MeshFeatureProcessor;
            friend class MeshLoader;

        public:
            const Data::Instance<RPI::Model>& GetModel() { return m_model; }
            const RPI::Cullable& GetCullable() { return m_cullable; }

        private:
            class MeshLoader
                : private Data::AssetBus::Handler
                , private AzFramework::AssetCatalogEventBus::Handler
            {
            public:
                using ModelChangedEvent = MeshFeatureProcessorInterface::ModelChangedEvent;

                MeshLoader(const Data::Asset<RPI::ModelAsset>& modelAsset, MeshDataInstance* parent);
                ~MeshLoader();

                ModelChangedEvent& GetModelChangedEvent();

            private:
                // AssetBus::Handler overrides...
                void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
                void OnAssetError(Data::Asset<Data::AssetData> asset) override;

                // AssetCatalogEventBus::Handler overrides...
                void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;
                void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;

                void OnModelReloaded(Data::Asset<Data::AssetData> asset);
                ModelReloadedEvent::Handler m_modelReloadedEventHandler { [&](Data::Asset<RPI::ModelAsset> modelAsset)
                                                                  {
                                                                             OnModelReloaded(modelAsset);
                                                                  } };
                MeshFeatureProcessorInterface::ModelChangedEvent m_modelChangedEvent;
                Data::Asset<RPI::ModelAsset> m_modelAsset;
                MeshDataInstance* m_parent = nullptr;
            };

            void DeInit();
            void Init(Data::Instance<RPI::Model> model);
            void BuildDrawPacketList(size_t modelLodIndex);
            void SetRayTracingData();
            void RemoveRayTracingData();
            void SetSortKey(RHI::DrawItemSortKey sortKey);
            RHI::DrawItemSortKey GetSortKey() const;
            void SetMeshLodConfiguration(RPI::Cullable::LodConfiguration meshLodConfig);
            RPI::Cullable::LodConfiguration GetMeshLodConfiguration() const;
            void UpdateDrawPackets(bool forceUpdate = false);
            void BuildCullable();
            void UpdateCullBounds(const TransformServiceFeatureProcessor* transformService);
            void UpdateObjectSrg();
            bool MaterialRequiresForwardPassIblSpecular(Data::Instance<RPI::Material> material) const;
            void SetVisible(bool isVisible);

            using DrawPacketList = AZStd::vector<RPI::MeshDrawPacket>;

            AZStd::fixed_vector<DrawPacketList, RPI::ModelLodAsset::LodCountMax> m_drawPacketListsByLod;
            RPI::Cullable m_cullable;
            MaterialAssignmentMap m_materialAssignments;

            MeshHandleDescriptor m_descriptor;
            Data::Instance<RPI::Model> m_model;

            //! A reference to the original model asset in case it got cloned before creating the model instance.
            Data::Asset<RPI::ModelAsset> m_originalModelAsset;

            Data::Instance<RPI::ShaderResourceGroup> m_shaderResourceGroup;
            AZStd::unique_ptr<MeshLoader> m_meshLoader;
            RPI::Scene* m_scene = nullptr;
            RHI::DrawItemSortKey m_sortKey;

            TransformServiceFeatureProcessorInterface::ObjectId m_objectId;

            Aabb m_aabb = Aabb::CreateNull();

            bool m_cullBoundsNeedsUpdate = false;
            bool m_cullableNeedsRebuild = false;
            bool m_objectSrgNeedsUpdate = true;
            bool m_excludeFromReflectionCubeMaps = false;
            bool m_visible = true;
            bool m_hasForwardPassIblSpecularMaterial = false;
        };

        //! This feature processor handles static and dynamic non-skinned meshes.
        class MeshFeatureProcessor final
            : public MeshFeatureProcessorInterface
        {
        public:

            AZ_RTTI(AZ::Render::MeshFeatureProcessor, "{6E3DFA1D-22C7-4738-A3AE-1E10AB88B29B}", MeshFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            MeshFeatureProcessor() = default;
            virtual ~MeshFeatureProcessor() = default;

            // FeatureProcessor overrides ...
            //! Creates pools, buffers, and buffer views
            void Activate() override;
            //! Releases GPU resources.
            void Deactivate() override;
            //! Updates GPU buffers with latest data from render proxies
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;

            // RPI::SceneNotificationBus overrides ...
            void OnBeginPrepareRender() override;
            void OnEndPrepareRender() override;

            MeshHandle AcquireMesh(
                const MeshHandleDescriptor& descriptor,
                const MaterialAssignmentMap& materials = {}) override;
            MeshHandle AcquireMesh(
                const MeshHandleDescriptor& descriptor,
                const Data::Instance<RPI::Material>& material) override;
            bool ReleaseMesh(MeshHandle& meshHandle) override;
            MeshHandle CloneMesh(const MeshHandle& meshHandle) override;

            Data::Instance<RPI::Model> GetModel(const MeshHandle& meshHandle) const override;
            Data::Asset<RPI::ModelAsset> GetModelAsset(const MeshHandle& meshHandle) const override;
            Data::Instance<RPI::ShaderResourceGroup> GetObjectSrg(const MeshHandle& meshHandle) const override;
            void QueueObjectSrgForCompile(const MeshHandle& meshHandle) const override;
            void SetMaterialAssignmentMap(const MeshHandle& meshHandle, const Data::Instance<RPI::Material>& material) override;
            void SetMaterialAssignmentMap(const MeshHandle& meshHandle, const MaterialAssignmentMap& materials) override;
            const MaterialAssignmentMap& GetMaterialAssignmentMap(const MeshHandle& meshHandle) const override;
            void ConnectModelChangeEventHandler(const MeshHandle& meshHandle, ModelChangedEvent::Handler& handler) override;

            void SetTransform(const MeshHandle& meshHandle, const AZ::Transform& transform,
                const AZ::Vector3& nonUniformScale = AZ::Vector3::CreateOne()) override;
            Transform GetTransform(const MeshHandle& meshHandle) override;
            Vector3 GetNonUniformScale(const MeshHandle& meshHandle) override;

            void SetLocalAabb(const MeshHandle& meshHandle, const AZ::Aabb& localAabb) override;
            AZ::Aabb GetLocalAabb(const MeshHandle& meshHandle) const override;

            void SetSortKey(const MeshHandle& meshHandle, RHI::DrawItemSortKey sortKey) override;
            RHI::DrawItemSortKey GetSortKey(const MeshHandle& meshHandle) const override;

            void SetMeshLodConfiguration(const MeshHandle& meshHandle, const RPI::Cullable::LodConfiguration& meshLodConfig) override;
            RPI::Cullable::LodConfiguration GetMeshLodConfiguration(const MeshHandle& meshHandle) const override;

            void SetExcludeFromReflectionCubeMaps(const MeshHandle& meshHandle, bool excludeFromReflectionCubeMaps) override;
            void SetRayTracingEnabled(const MeshHandle& meshHandle, bool rayTracingEnabled) override;
            void SetVisible(const MeshHandle& meshHandle, bool visible) override;
            void SetUseForwardPassIblSpecular(const MeshHandle& meshHandle, bool useForwardPassIblSpecular) override;

            // called when reflection probes are modified in the editor so that meshes can re-evaluate their probes
            void UpdateMeshReflectionProbes();
        private:
            void ForceRebuildDrawPackets(const AZ::ConsoleCommandContainer& arguments);
            AZ_CONSOLEFUNC(MeshFeatureProcessor,
                ForceRebuildDrawPackets,
                AZ::ConsoleFunctorFlags::Null,
                "(For Testing) Invalidates all mesh draw packets, causing them to rebuild on the next frame."
            );

            MeshFeatureProcessor(const MeshFeatureProcessor&) = delete;

            // RPI::SceneNotificationBus::Handler overrides...
            void OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline) override;
            void OnRenderPipelineRemoved(RPI::RenderPipeline* pipeline) override;
                        
            AZStd::concurrency_checker m_meshDataChecker;
            StableDynamicArray<MeshDataInstance> m_meshData;
            TransformServiceFeatureProcessor* m_transformService;
            RayTracingFeatureProcessor* m_rayTracingFeatureProcessor = nullptr;
            AZ::RPI::ShaderSystemInterface::GlobalShaderOptionUpdatedEvent::Handler m_handleGlobalShaderOptionUpdate;
            bool m_forceRebuildDrawPackets = false;
        };
    } // namespace Render
} // namespace AZ
