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
#include <TransformService/TransformServiceFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        class TransformServiceFeatureProcessor;
        class RayTracingFeatureProcessor;
        class ReflectionProbeFeatureProcessor;
        class MeshFeatureProcessor;
        class GpuBufferHandler;

        class ModelDataInstance : ModelDataInstanceInterface
        {
            friend class MeshFeatureProcessor;
            friend class MeshLoader;

        public:
            AZ_RTTI(AZ::Render::ModelDataInstance, "{AF38DCED-9692-4B88-8738-9710BA70F49C}", AZ::Render::ModelDataInstanceInterface);
            ModelDataInstance();

            const Data::Instance<RPI::Model>& GetModel() override
            {
                return m_model;
            }
            const RPI::Cullable& GetCullable() override
            {
                return m_cullable;
            }

            const uint32_t GetLightingChannelMask() override
            {
                return m_lightingChannelMask;
            }

            using InstanceGroupHandle = StableDynamicArrayWeakHandle<MeshInstanceGroupData>;

            using PostCullingInstanceDataList = AZStd::vector<PostCullingInstanceData>;
            const bool IsSkinnedMesh() override
            {
                return m_descriptor.m_isSkinnedMesh;
            }
            const AZ::Uuid& GetRayTracingUuid() const override
            {
                return m_rayTracingUuid;
            }

            void HandleDrawPacketUpdate(uint32_t lodIndex, uint32_t meshIndex, RPI::MeshDrawPacket& meshDrawPacket) override;
            void ConnectMeshDrawPacketUpdatedHandler(MeshDrawPacketUpdatedEvent::Handler& handler) override;

            CustomMaterialInfo GetCustomMaterialWithFallback(const CustomMaterialId& id) const override;

        private:
            class MeshLoader
                : private SystemTickBus::Handler
                , private Data::AssetBus::Handler
                , private AzFramework::AssetCatalogEventBus::Handler
            {
            public:
                MeshLoader(const Data::Asset<RPI::ModelAsset>& modelAsset, ModelDataInstance* parent);
                ~MeshLoader();

            private:
                // SystemTickBus::Handler overrides...
                void OnSystemTick() override;

                // AssetBus::Handler overrides...
                void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
                void OnAssetError(Data::Asset<Data::AssetData> asset) override;

                // AssetCatalogEventBus::Handler overrides...
                void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;
                void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;
                void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;

                void OnModelReloaded(Data::Asset<Data::AssetData> asset);
                ModelReloadedEvent::Handler m_modelReloadedEventHandler{ [&](Data::Asset<RPI::ModelAsset> modelAsset)
                                                                         {
                                                                             OnModelReloaded(modelAsset);
                                                                         } };
                Data::Asset<RPI::ModelAsset> m_modelAsset;
                ModelDataInstance* m_parent = nullptr;
            };// class MeshLoader

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
            void SetLightingChannelMask(uint32_t lightingChannelMask);
            void SetMeshLodConfiguration(RPI::Cullable::LodConfiguration meshLodConfig);
            RPI::Cullable::LodConfiguration GetMeshLodConfiguration() const;
            void UpdateDrawPackets(bool forceUpdate = false);
            void BuildCullable();
            void UpdateCullBounds(const MeshFeatureProcessor* meshFeatureProcessor);
            void UpdateObjectSrg(MeshFeatureProcessor* meshFeatureProcessor);
            bool MaterialRequiresForwardPassIblSpecular(Data::Instance<RPI::Material> material) const;
            void SetVisible(bool isVisible);

            // When instancing is disabled, draw packets are owned by the ModelDataInstance
            RPI::MeshDrawPacketLods m_meshDrawPacketListsByLod;
            
            // When instancing is enabled, draw packets are owned by the MeshInstanceManager,
            // and the ModelDataInstance refers to those draw packets via InstanceGroupHandles,
            // which are turned into instance draw calls after culling
            AZStd::vector<PostCullingInstanceDataList> m_postCullingInstanceDataByLod;
            
            size_t m_lodBias = 0;

            RPI::Cullable m_cullable;
            MeshHandleDescriptor m_descriptor;
            Data::Instance<RPI::Model> m_model;

            //! A reference to the original model asset in case it got cloned before creating the model instance.
            Data::Asset<RPI::ModelAsset> m_originalModelAsset;

            //! List of object SRGs used by meshes in this model 
            AZStd::vector<Data::Instance<RPI::ShaderResourceGroup>> m_objectSrgList;
            //! Event that gets triggered whenever the model is changed, loaded, or reloaded.
            MeshHandleDescriptor::ModelChangedEvent m_modelChangedEvent;

            //! Event that triggers whenever the ObjectSrg is created.
            MeshHandleDescriptor::ObjectSrgCreatedEvent m_objectSrgCreatedEvent;

            //! Event that triggers whenever a MeshDrawPacket gets updated.
            MeshDrawPacketUpdatedEvent m_meshDrawPacketUpdatedEvent;

            // MeshLoader is a shared pointer because it can queue a reference to itself on the SystemTickBus. The reference
            // needs to stay alive until the queued function is executed.
            AZStd::shared_ptr<MeshLoader> m_meshLoader;
            RPI::Scene* m_scene = nullptr;
            RHI::DrawItemSortKey m_sortKey = 0;
            uint32_t m_lightingChannelMask = 1;

            TransformServiceFeatureProcessorInterface::ObjectId m_objectId;
            AZ::Uuid m_rayTracingUuid;

            Aabb m_aabb = Aabb::CreateNull();

            struct Flags
            {
                bool m_cullBoundsNeedsUpdate : 1;
                bool m_cullableNeedsRebuild : 1;
                bool m_needsInit : 1;
                bool m_objectSrgNeedsUpdate : 1;
                bool m_isAlwaysDynamic : 1;
                bool m_dynamic : 1;                             // True if the model's transformation was changed than the initial position
                bool m_isDrawMotion : 1;                        // Whether draw to the motion vector
                bool m_visible : 1;
                bool m_useForwardPassIblSpecular : 1;
                bool m_hasForwardPassIblSpecularMaterial : 1;
                bool m_needsSetRayTracingData : 1;
                bool m_hasRayTracingReflectionProbe : 1;
                bool m_keepBufferAssetsInMemory : 1;            // If true, we need to keep BufferAssets referenced by ModelAsset stay in memory. This is needed when editor use RayIntersection
            } m_flags;
        }; //class ModelDataInstance

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
            //! Updates ViewSrgs with per-view instance data for visible instances
            void OnEndCulling(const RenderPacket& packet) override;

            // RPI::SceneNotificationBus overrides ...
            void OnBeginPrepareRender() override;
            void OnEndPrepareRender() override;

            TransformServiceFeatureProcessorInterface::ObjectId GetObjectId(const MeshHandle& meshHandle) const override;
            MeshHandle AcquireMesh(const MeshHandleDescriptor& descriptor) override;
            bool ReleaseMesh(MeshHandle& meshHandle) override;
            MeshHandle CloneMesh(const MeshHandle& meshHandle) override;

            void PrintDrawPacketInfo(const MeshHandle& meshHandle) override;
            void SetDrawItemEnabled(const MeshHandle& meshHandle, RHI::DrawListTag drawListTag, bool enabled) override;
            Data::Instance<RPI::Model> GetModel(const MeshHandle& meshHandle) const override;
            Data::Asset<RPI::ModelAsset> GetModelAsset(const MeshHandle& meshHandle) const override;
            const RPI::MeshDrawPacketLods& GetDrawPackets(const MeshHandle& meshHandle) const override;
            const AZStd::vector<Data::Instance<RPI::ShaderResourceGroup>>& GetObjectSrgs(const MeshHandle& meshHandle) const override;
            void QueueObjectSrgForCompile(const MeshHandle& meshHandle) const override;
            void SetCustomMaterials(const MeshHandle& meshHandle, const Data::Instance<RPI::Material>& material) override;
            void SetCustomMaterials(const MeshHandle& meshHandle, const CustomMaterialMap& materials) override;
            const CustomMaterialMap& GetCustomMaterials(const MeshHandle& meshHandle) const override;
            AZStd::unique_ptr<StreamBufferViewsBuilderInterface> CreateStreamBufferViewsBuilder(const MeshHandle& meshHandle) const override;
            DispatchDrawItemList BuildDispatchDrawItemList(const MeshHandle& meshHandle,
                const uint32_t lodIndex, const uint32_t meshIndex,
                const RHI::DrawListMask drawListTagsFilter, const RHI::DrawFilterMask materialPipelineFilter,
                DispatchArgumentsSetupCB dispatchArgumentsSetupCB) const override;

            void SetTransform(const MeshHandle& meshHandle, const AZ::Transform& transform,
                const AZ::Vector3& nonUniformScale = AZ::Vector3::CreateOne()) override;
            Transform GetTransform(const MeshHandle& meshHandle) override;
            Vector3 GetNonUniformScale(const MeshHandle& meshHandle) override;

            void SetLocalAabb(const MeshHandle& meshHandle, const AZ::Aabb& localAabb) override;
            AZ::Aabb GetLocalAabb(const MeshHandle& meshHandle) const override;

            void SetSortKey(const MeshHandle& meshHandle, RHI::DrawItemSortKey sortKey) override;
            RHI::DrawItemSortKey GetSortKey(const MeshHandle& meshHandle) const override;

            void SetLightingChannelMask(const MeshHandle& meshHandle, uint32_t lightingChannelMask) override;
            uint32_t GetLightingChannelMask(const MeshHandle& meshHandle) const override;

            void SetMeshLodConfiguration(const MeshHandle& meshHandle, const RPI::Cullable::LodConfiguration& meshLodConfig) override;
            RPI::Cullable::LodConfiguration GetMeshLodConfiguration(const MeshHandle& meshHandle) const override;

            void SetExcludeFromReflectionCubeMaps(const MeshHandle& meshHandle, bool excludeFromReflectionCubeMaps) override;
            bool GetExcludeFromReflectionCubeMaps(const MeshHandle& meshHandle) const override;
            void SetIsAlwaysDynamic(const MeshHandle& meshHandle, bool isAlwaysDynamic) override;
            bool GetIsAlwaysDynamic(const MeshHandle& meshHandle) const override;
            void SetRayTracingEnabled(const MeshHandle& meshHandle, bool enabled) override;
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

            RHI::DrawListTag GetTransparentDrawListTag() const;

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
            
            
            void ResizePerViewInstanceVectors(size_t viewCount);
            void AddVisibleObjectsToBuckets(TaskGraph& addVisibleObjectsToBucketsTG, size_t viewIndex, const RPI::ViewPtr& view);
            void SortInstanceBufferBuckets(TaskGraph& sortInstanceBufferBucketsTG, size_t viewIndex);
            void BuildInstanceBufferAndDrawCalls(TaskGraph& taskGraph, size_t viewIndex, const RPI::ViewPtr& view);
            void UpdateGPUInstanceBufferForView(size_t viewIndex, const RPI::ViewPtr& view);

            // Helper function for BuildDispatchDrawItemList()
            void InitializeDispatchItemFromDrawItem(
                RHI::DispatchItem& dstDispatchItem, const RHI::DrawItem* srcDrawItem,
                const RHI::DispatchDirect& dispatchDirect) const;

            AZStd::concurrency_checker m_meshDataChecker;
            StableDynamicArray<ModelDataInstance> m_modelData;

            MeshInstanceManager m_meshInstanceManager;

            // SortInstanceData represents the data needed to do the sorting (sort by instance group, then by depth)
            // as well as the data being sorted (ObjectId)
            struct SortInstanceData
            {
                ModelDataInstance::InstanceGroupHandle m_instanceGroupHandle;
                float m_depth = 0.0f;
                TransformServiceFeatureProcessorInterface::ObjectId m_objectId;

                bool operator<(const SortInstanceData& rhs) const
                {
                    return AZStd::tie(m_instanceGroupHandle, m_depth) < AZStd::tie(rhs.m_instanceGroupHandle, rhs.m_depth);
                }
            };

            // An InstanceGroupBucket represents all of the instance groups from a single page in the MeshInstanceManager
            // There is one InstanceGroupBucket per-page, per-view
            // This is used to perform a bucket-sort, where all of the visible meshes for a given view are first added to their bucket,
            // then they are sorted within the bucket.
            struct InstanceGroupBucket
            {
                AZStd::atomic<uint32_t> m_currentElementIndex{};
                AZStd::vector<SortInstanceData> m_sortInstanceData{};

                InstanceGroupBucket() = default;

                InstanceGroupBucket(const InstanceGroupBucket& rhs)
                    : m_currentElementIndex(rhs.m_currentElementIndex.load())
                    , m_sortInstanceData(rhs.m_sortInstanceData)
                {
                }

                void operator=(const InstanceGroupBucket& rhs)
                {
                    m_currentElementIndex = rhs.m_currentElementIndex.load();
                    m_sortInstanceData = rhs.m_sortInstanceData;
                }
            };
            
            AZStd::vector<AZStd::vector<InstanceGroupBucket>> m_perViewInstanceGroupBuckets;
            AZStd::vector<AZStd::vector<TransformServiceFeatureProcessorInterface::ObjectId>> m_perViewInstanceData;
            AZStd::vector<GpuBufferHandler> m_perViewInstanceDataBufferHandlers;
            
            TransformServiceFeatureProcessor* m_transformService = nullptr;
            RayTracingFeatureProcessor* m_rayTracingFeatureProcessor = nullptr;
            ReflectionProbeFeatureProcessor* m_reflectionProbeFeatureProcessor = nullptr;
            AZ::RPI::ShaderSystemInterface::GlobalShaderOptionUpdatedEvent::Handler m_handleGlobalShaderOptionUpdate;
            RPI::MeshDrawPacketLods m_emptyDrawPacketLods;
            RHI::Ptr<FlagRegistry> m_flagRegistry = nullptr;
            AZ::RHI::Handle<uint32_t> m_meshMovedFlag;
            RHI::DrawListTag m_meshMotionDrawListTag;
            RHI::DrawListTag m_transparentDrawListTag;
            bool m_forceRebuildDrawPackets = false;
            bool m_reportShaderOptionFlags = false;
            bool m_enablePerMeshShaderOptionFlags = false;
            bool m_enableMeshInstancing = false;
            bool m_enableMeshInstancingForTransparentObjects = false;
        };
    } // namespace Render
} // namespace AZ
