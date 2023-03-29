/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <Atom/Feature/Mesh/ModelReloaderSystemInterface.h>
#include <Atom/Feature/TransformService/TransformServiceFeatureProcessor.h>
#include <Atom/RHI/TagBitRegistry.h>
#include <Atom/RPI.Public/Culling.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/Shader/ShaderSystemInterface.h>
#include <AtomCore/std/parallel/concurrency_checker.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/Console.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <Mesh/MeshInstanceManager.h>
#include <RayTracing/RayTracingFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        class TransformServiceFeatureProcessor;
        class RayTracingFeatureProcessor;
        class ReflectionProbeFeatureProcessor;
        class MeshFeatureProcessor;

        class ModelDataInstance
        {
            friend class MeshFeatureProcessor;
            friend class MeshLoader;

        public:
            using ObjectSrgCreatedEvent = MeshFeatureProcessorInterface::ObjectSrgCreatedEvent;

            const Data::Instance<RPI::Model>& GetModel() { return m_model; }
            const RPI::Cullable& GetCullable() { return m_cullable; }

            ObjectSrgCreatedEvent& GetObjectSrgCreatedEvent() { return m_objectSrgCreatedEvent; }

        private:
            class MeshLoader
                : private Data::AssetBus::Handler
                , private AzFramework::AssetCatalogEventBus::Handler
            {
            public:
                using ModelChangedEvent = MeshFeatureProcessorInterface::ModelChangedEvent;

                MeshLoader(const Data::Asset<RPI::ModelAsset>& modelAsset, ModelDataInstance* parent);
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
                ModelReloadedEvent::Handler m_modelReloadedEventHandler{ [&](Data::Asset<RPI::ModelAsset> modelAsset)
                                                                         {
                                                                             OnModelReloaded(modelAsset);
                                                                         } };
                MeshFeatureProcessorInterface::ModelChangedEvent m_modelChangedEvent;
                Data::Asset<RPI::ModelAsset> m_modelAsset;
                ModelDataInstance* m_parent = nullptr;
            };

            // Free all the resources owned by this mesh handle
            void DeInit(MeshFeatureProcessor* meshFeatureProcessor);
            // Clear all the data that is created by the MeshFeatureProcessor, such as the draw packets, cullable, and ray-tracing data,
            // but preserve all the settings such as the model, material assignment, etc., then queue it for re-initialization
            void ReInit(MeshFeatureProcessor* meshFeatureProcessor);
            void QueueInit(const Data::Instance<RPI::Model>& model);
            void Init(MeshFeatureProcessor* meshFeatureProcessor);
            void BuildDrawPacketList(MeshFeatureProcessor* meshFeatureProcessor, size_t modelLodIndex);
            void SetRayTracingData(MeshFeatureProcessor* meshFeatureProcessor);
            void RemoveRayTracingData(RayTracingFeatureProcessor* rayTracingFeatureProcessor);
            void SetIrradianceData(
                RayTracingFeatureProcessor::SubMesh& subMesh,
                const Data::Instance<RPI::Material> material,
                const Data::Instance<RPI::Image> baseColorImage);
            void SetRayTracingReflectionProbeData(
                MeshFeatureProcessor* meshFeatureProcessor, 
                RayTracingFeatureProcessor::Mesh::ReflectionProbe& reflectionProbe);
            void SetSortKey(MeshFeatureProcessor* meshFeatureProcessor, RHI::DrawItemSortKey sortKey);
            RHI::DrawItemSortKey GetSortKey() const;
            void SetMeshLodConfiguration(RPI::Cullable::LodConfiguration meshLodConfig);
            RPI::Cullable::LodConfiguration GetMeshLodConfiguration() const;
            void UpdateDrawPackets(bool forceUpdate = false);
            void BuildCullable(MeshFeatureProcessor* meshFeatureProcessor);
            void UpdateCullBounds(const MeshFeatureProcessor* meshFeatureProcessor);
            void UpdateObjectSrg(MeshFeatureProcessor* meshFeatureProcessor);
            bool MaterialRequiresForwardPassIblSpecular(Data::Instance<RPI::Material> material) const;
            void SetVisible(bool isVisible);
            CustomMaterialInfo GetCustomMaterialWithFallback(const CustomMaterialId& id) const;
            void HandleDrawPacketUpdate();

            // When instancing is disabled, draw packets are owned by the ModelDataInstance
            RPI::MeshDrawPacketLods m_drawPacketListsByLod;
            
            // When instancing is enabled, draw packets are owned by the MeshInstanceManager,
            // and the ModelDataInstance refers to those draw packets via InstanceGroupHandles
            using InstanceGroupHandle = StableDynamicArrayWeakHandle<MeshInstanceGroupData>;
            using InstanceGroupHandleList = AZStd::vector<InstanceGroupHandle>;
            AZStd::vector<InstanceGroupHandleList> m_instanceGroupHandlesByLod;
            
            // AZ::Event is used to communicate back to all the objects that refer to an instance group whenever a draw packet is updated
            // This is used to trigger an update to the cullable to use the new draw packet
            using UpdateDrawPacketHandlerList = AZStd::vector<AZ::Event<>::Handler>;
            AZStd::vector<UpdateDrawPacketHandlerList> m_updateDrawPacketEventHandlersByLod;

            size_t m_lodBias = 0;

            RPI::Cullable m_cullable;
            CustomMaterialMap m_customMaterials;
            MeshHandleDescriptor m_descriptor;
            Data::Instance<RPI::Model> m_model;

            //! A reference to the original model asset in case it got cloned before creating the model instance.
            Data::Asset<RPI::ModelAsset> m_originalModelAsset;

            //! List of object SRGs used by meshes in this model 
            AZStd::vector<Data::Instance<RPI::ShaderResourceGroup>> m_objectSrgList;
            MeshFeatureProcessorInterface::ObjectSrgCreatedEvent m_objectSrgCreatedEvent;
            AZStd::unique_ptr<MeshLoader> m_meshLoader;
            RPI::Scene* m_scene = nullptr;
            RHI::DrawItemSortKey m_sortKey;

            TransformServiceFeatureProcessorInterface::ObjectId m_objectId;
            AZ::Uuid m_rayTracingUuid;

            Aabb m_aabb = Aabb::CreateNull();

            bool m_cullBoundsNeedsUpdate = false;
            bool m_cullableNeedsRebuild = false;
            bool m_needsInit = false;
            bool m_objectSrgNeedsUpdate = true;
            bool m_isAlwaysDynamic = false;
            bool m_visible = true;
            bool m_hasForwardPassIblSpecularMaterial = false;
            bool m_needsSetRayTracingData = false;
            bool m_hasRayTracingReflectionProbe = false;
        };

        //! This feature processor handles static and dynamic non-skinned meshes.
        class MeshFeatureProcessor final : public MeshFeatureProcessorInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(MeshFeatureProcessor, AZ::SystemAllocator)

            AZ_RTTI(AZ::Render::MeshFeatureProcessor, "{6E3DFA1D-22C7-4738-A3AE-1E10AB88B29B}", AZ::Render::MeshFeatureProcessorInterface);

            AZ_CONSOLEFUNC(MeshFeatureProcessor, ReportShaderOptionFlags, AZ::ConsoleFunctorFlags::Null, "Report currently used shader option flags.");

            using FlagRegistry = RHI::TagBitRegistry<RPI::Cullable::FlagType>;

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

            TransformServiceFeatureProcessorInterface::ObjectId GetObjectId(const MeshHandle& meshHandle) const override;
            MeshHandle AcquireMesh(const MeshHandleDescriptor& descriptor, const CustomMaterialMap& materials = {}) override;
            MeshHandle AcquireMesh(const MeshHandleDescriptor& descriptor, const Data::Instance<RPI::Material>& material) override;
            bool ReleaseMesh(MeshHandle& meshHandle) override;
            MeshHandle CloneMesh(const MeshHandle& meshHandle) override;

            Data::Instance<RPI::Model> GetModel(const MeshHandle& meshHandle) const override;
            Data::Asset<RPI::ModelAsset> GetModelAsset(const MeshHandle& meshHandle) const override;
            const RPI::MeshDrawPacketLods& GetDrawPackets(const MeshHandle& meshHandle) const override;
            const AZStd::vector<Data::Instance<RPI::ShaderResourceGroup>>& GetObjectSrgs(const MeshHandle& meshHandle) const override;
            void QueueObjectSrgForCompile(const MeshHandle& meshHandle) const override;
            void SetCustomMaterials(const MeshHandle& meshHandle, const Data::Instance<RPI::Material>& material) override;
            void SetCustomMaterials(const MeshHandle& meshHandle, const CustomMaterialMap& materials) override;
            const CustomMaterialMap& GetCustomMaterials(const MeshHandle& meshHandle) const override;
            void ConnectModelChangeEventHandler(const MeshHandle& meshHandle, ModelChangedEvent::Handler& handler) override;
            void ConnectObjectSrgCreatedEventHandler(const MeshHandle& meshHandle, ObjectSrgCreatedEvent::Handler& handler) override;

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
            bool GetExcludeFromReflectionCubeMaps(const MeshHandle& meshHandle) const override;
            void SetIsAlwaysDynamic(const MeshHandle& meshHandle, bool isAlwaysDynamic) override;
            bool GetIsAlwaysDynamic(const MeshHandle& meshHandle) const override;
            void SetRayTracingEnabled(const MeshHandle& meshHandle, bool rayTracingEnabled) override;
            bool GetRayTracingEnabled(const MeshHandle& meshHandle) const override;
            void SetVisible(const MeshHandle& meshHandle, bool visible) override;
            bool GetVisible(const MeshHandle& meshHandle) const override;
            void SetUseForwardPassIblSpecular(const MeshHandle& meshHandle, bool useForwardPassIblSpecular) override;
            void SetRayTracingDirty(const MeshHandle& meshHandle) override;

            RHI::Ptr <FlagRegistry> GetShaderOptionFlagRegistry();

            // called when reflection probes are modified in the editor so that meshes can re-evaluate their probes
            void UpdateMeshReflectionProbes();

            void ReportShaderOptionFlags(const AZ::ConsoleCommandContainer& arguments);

            // Quick functions to get other relevant feature processors that have already been cached by the MeshFeatureProcessor
            // without needing to go through the RPI's list of feature processors
            RayTracingFeatureProcessor* GetRayTracingFeatureProcessor() const;
            ReflectionProbeFeatureProcessor* GetReflectionProbeFeatureProcessor() const;
            TransformServiceFeatureProcessor* GetTransformServiceFeatureProcessor() const;

            MeshInstanceManager& GetMeshInstanceManager();
            bool IsMeshInstancingEnabled() const;
        private:
            MeshFeatureProcessor(const MeshFeatureProcessor&) = delete;

            void ForceRebuildDrawPackets(const AZ::ConsoleCommandContainer& arguments);
            AZ_CONSOLEFUNC(MeshFeatureProcessor,
                ForceRebuildDrawPackets,
                AZ::ConsoleFunctorFlags::Null,
                "(For Testing) Invalidates all mesh draw packets, causing them to rebuild on the next frame."
            );

            void PrintShaderOptionFlags();

            // RPI::SceneNotificationBus::Handler overrides...
            void OnRenderPipelineChanged(AZ::RPI::RenderPipeline* pipeline, RPI::SceneNotification::RenderPipelineChangeType changeType) override;

            void CheckForInstancingCVarChange();
            AZStd::vector<AZ::Job*> CreateInitJobQueue();
            AZStd::vector<AZ::Job*> CreatePerInstanceGroupJobQueue();
            AZStd::vector<AZ::Job*> CreateUpdateCullingJobQueue();
            void ExecuteSimulateJobQueue(AZStd::span<Job*> jobQueue, Job* parentJob);
            void ExecuteCombinedJobQueue(AZStd::span<Job*> initQueue, AZStd::span<Job*> updateCullingQueue, Job* parentJob);

            AZStd::concurrency_checker m_meshDataChecker;
            StableDynamicArray<ModelDataInstance> m_modelData;

            MeshInstanceManager m_meshInstanceManager;
            TransformServiceFeatureProcessor* m_transformService;
            RayTracingFeatureProcessor* m_rayTracingFeatureProcessor = nullptr;
            ReflectionProbeFeatureProcessor* m_reflectionProbeFeatureProcessor = nullptr;
            AZ::RPI::ShaderSystemInterface::GlobalShaderOptionUpdatedEvent::Handler m_handleGlobalShaderOptionUpdate;
            RPI::MeshDrawPacketLods m_emptyDrawPacketLods;
            RHI::Ptr<FlagRegistry> m_flagRegistry = nullptr;
            AZ::RHI::Handle<uint32_t> m_meshMovedFlag;
            bool m_forceRebuildDrawPackets = false;
            bool m_reportShaderOptionFlags = false;
            bool m_enablePerMeshShaderOptionFlags = false;
            bool m_enableMeshInstancing = false;
        };
    } // namespace Render
} // namespace AZ
