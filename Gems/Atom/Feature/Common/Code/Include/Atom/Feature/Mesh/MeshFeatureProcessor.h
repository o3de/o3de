/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/Console.h>

#include <AzFramework/Asset/AssetCatalogBus.h>

#include <AtomCore/std/parallel/concurrency_checker.h>

#include <Atom/RHI/FreeListAllocator.h>
#include <Atom/RHI/TagBitRegistry.h>

#include <Atom/RPI.Public/Culling.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/Shader/ShaderSystemInterface.h>

#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <Atom/Feature/Material/MaterialAssignment.h>
#include <Atom/Feature/Material/MaterialAssignmentBus.h>
#include <Atom/Feature/TransformService/TransformServiceFeatureProcessor.h>
#include <Atom/Feature/Mesh/ModelReloaderSystemInterface.h>
#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <Atom/Utils/MultiIndexedStableDynamicArray.h>

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

        constexpr size_t postCullingPoolPageSize = size_t{ 1024 } * size_t{ 1024 } * size_t{ 64 };
        constexpr size_t postCullingPoolMinAllocationSize = size_t{ 32 };
        constexpr size_t postCullingPoolMaxAllocationSize = size_t{ 1024 } * size_t{ 1024 };
        class PostCullingMeshDataAllocator final : public AZ::Internal::PoolAllocatorHelper<AZ::PoolSchema>
        {
        public:
            AZ_CLASS_ALLOCATOR(PostCullingMeshDataAllocator, AZ::SystemAllocator, 0)
            AZ_TYPE_INFO(PostCullingMeshDataAllocator, "{A3199670-180C-4A46-92BF-8DEBFE5E8A47}")

            PostCullingMeshDataAllocator()
                // Invoke the base constructor explicitely to use the override that takes custom page, min, and max allocation sizes
                : AZ::Internal::PoolAllocatorHelper<AZ::PoolSchema>(
                      postCullingPoolPageSize, postCullingPoolMinAllocationSize, postCullingPoolMaxAllocationSize)
            {
            }
        };

        namespace MeshFP
        {
            struct MeshDataIndicesForLod
            {
                uint32_t m_startIndex = 0;
                uint32_t m_count = 0;
            };
            using InstanceIndicesByLod = AZStd::fixed_vector<MeshDataIndicesForLod, RPI::ModelLodAsset::LodCountMax>;
            struct EndCullingData
            {
                TransformServiceFeatureProcessorInterface::ObjectId m_objectId;
                InstanceIndicesByLod m_instanceIndicesByLod;
            };

            struct MeshData
            {
                uint32_t m_instanceGroupIndex = AZStd::numeric_limits<uint32_t>::max();
                TransformServiceFeatureProcessorInterface::ObjectId m_objectId;
            };
        }

        class ModelDataInstance
            : public MaterialAssignmentNotificationBus::MultiHandler
        {
            friend class MeshFeatureProcessor;
            friend class MeshLoader;

        public:
            // TODO: Pass in a pool allocator so the vectors can be cache coherent
            ModelDataInstance([[maybe_unused]] int i){ /* hack to fool the compiler */ };
            AZ_DEFAULT_COPY_MOVE(ModelDataInstance);
            const Data::Instance<RPI::Model>& GetModel() { return m_model; }
            const RPI::Cullable& GetCullable() { return m_cullable; }

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
                ModelReloadedEvent::Handler m_modelReloadedEventHandler { [&](Data::Asset<RPI::ModelAsset> modelAsset)
                                                                  {
                                                                             OnModelReloaded(modelAsset);
                                                                  } };
                MeshFeatureProcessorInterface::ModelChangedEvent m_modelChangedEvent;
                Data::Asset<RPI::ModelAsset> m_modelAsset;
                ModelDataInstance* m_parent = nullptr;
            };

            void DeInit(
                MeshFeatureProcessor* meshFeatureProcessor,
                MeshFP::EndCullingData* endCullingData,
                MeshInstanceManager& meshInstanceManager,
                RayTracingFeatureProcessor* rayTracingFeatureProcessor);
            void QueueInit(const Data::Instance<RPI::Model>& model);
            void Init(
                MeshFeatureProcessor* meshFeatureProcessor,
                MeshFP::EndCullingData* endCullingData,
                MeshInstanceManager& meshInstanceManager);
            void BuildDrawPacketList(
                MeshFeatureProcessor* meshFeatureProcessor,
                MeshFP::EndCullingData* endCullingData, MeshInstanceManager& meshInstanceManager, size_t modelLodIndex);
            void SetRayTracingData(
                RayTracingFeatureProcessor* rayTracingFeatureProcessor,
                TransformServiceFeatureProcessor* transformServiceFeatureProcessor);
            void RemoveRayTracingData(RayTracingFeatureProcessor* rayTracingFeatureProcessor);
            void SetIrradianceData(RayTracingFeatureProcessor::SubMesh& subMesh,
                    const Data::Instance<RPI::Material> material, const Data::Instance<RPI::Image> baseColorImage);
            void SetSortKey(
                MeshFeatureProcessor* meshFeatureProcessor,
                MeshFP::EndCullingData* endCullingData, 
                MeshInstanceManager& meshInstanceManager,
                RayTracingFeatureProcessor* rayTracingFeatureProcessor,
                RHI::DrawItemSortKey sortKey);
            RHI::DrawItemSortKey GetSortKey() const;
            void SetMeshLodConfiguration(RPI::Cullable::LodConfiguration meshLodConfig);
            RPI::Cullable::LodConfiguration GetMeshLodConfiguration() const;
            void UpdateDrawPackets(bool forceUpdate = false);
            void BuildCullable(
                MeshFeatureProcessor* meshFeatureProcessor,
                MeshFP::EndCullingData* endCullingData,
                MeshInstanceManager& meshInstanceManager);
            void UpdateCullBounds(MeshFeatureProcessor* meshFeatureProcessor, MeshFP::EndCullingData* endCullingData, const TransformServiceFeatureProcessor* transformService);
            void UpdateObjectSrg(
                ReflectionProbeFeatureProcessor* reflectionProbeFeatureProcessor,
                TransformServiceFeatureProcessor* transformServiceFeatureProcessor);
            bool MaterialRequiresForwardPassIblSpecular(Data::Instance<RPI::Material> material) const;
            void SetVisible(bool isVisible);
            void UpdateMaterialChangeIds();
            bool CheckForMaterialChanges() const;

            // MaterialAssignmentNotificationBus overrides
            void OnRebuildMaterialInstance() override;

            MeshFP::MeshDataIndicesForLod m_meshDataIndices;

            RPI::MeshDrawPacketLods m_drawPacketListsByLod;

            RPI::Cullable m_cullable;
            MaterialAssignmentMap m_materialAssignments;

            typedef AZStd::unordered_map<Data::Instance<RPI::Material>, RPI::Material::ChangeId> MaterialChangeIdMap;
            MaterialChangeIdMap m_materialChangeIds;

            MeshHandleDescriptor m_descriptor;
            Data::Instance<RPI::Model> m_model;

            //! A reference to the original model asset in case it got cloned before creating the model instance.
            Data::Asset<RPI::ModelAsset> m_originalModelAsset;

            //! List of object SRGs used by meshes in this model 
            AZStd::vector<Data::Instance<RPI::ShaderResourceGroup>> m_objectSrgList;
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
            bool m_excludeFromReflectionCubeMaps = false;
            bool m_visible = true;
            bool m_hasForwardPassIblSpecularMaterial = false;
            bool m_needsSetRayTracingData = false;
        };

        static constexpr size_t foo = sizeof(ModelDataInstance);

        //! This feature processor handles static and dynamic non-skinned meshes.
        class MeshFeatureProcessor final
            : public MeshFeatureProcessorInterface
        {
        public:

            AZ_RTTI(AZ::Render::MeshFeatureProcessor, "{6E3DFA1D-22C7-4738-A3AE-1E10AB88B29B}", AZ::Render::MeshFeatureProcessorInterface);

            AZ_CONSOLEFUNC(MeshFeatureProcessor, ReportShaderOptionFlags, AZ::ConsoleFunctorFlags::Null, "Report currently used shader option flags.");

            using FlagRegistry = RHI::TagBitRegistry<RPI::Cullable::FlagType>;

            static void Reflect(AZ::ReflectContext* context);

            MeshFeatureProcessor()
                : m_postCullingPoolAllocator()
                , m_perViewSortInstanceData(AZStdIAllocator(&m_postCullingPoolAllocator))
                , m_perViewInstanceData(AZStdIAllocator(&m_postCullingPoolAllocator))
            {
            }

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
            const RPI::MeshDrawPacketLods& GetDrawPackets(const MeshHandle& meshHandle) const override;
            const AZStd::vector<Data::Instance<RPI::ShaderResourceGroup>>& GetObjectSrgs(const MeshHandle& meshHandle) const override;
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
            void SetIsAlwaysDynamic(const MeshHandle& meshHandle, bool isAlwaysDynamic) override;
            bool GetIsAlwaysDynamic(const MeshHandle& meshHandle) const override;
            void SetRayTracingEnabled(const MeshHandle& meshHandle, bool rayTracingEnabled) override;
            bool GetRayTracingEnabled(const MeshHandle& meshHandle) const override;
            void SetVisible(const MeshHandle& meshHandle, bool visible) override;
            bool GetVisible(const MeshHandle& meshHandle) const override;
            void SetUseForwardPassIblSpecular(const MeshHandle& meshHandle, bool useForwardPassIblSpecular) override;

            const RPI::Cullable* GetCullable(const MeshHandle& meshHandle);

            RHI::Ptr <FlagRegistry> GetShaderOptionFlagRegistry();

            // called when reflection probes are modified in the editor so that meshes can re-evaluate their probes
            void UpdateMeshReflectionProbes();

            void ReportShaderOptionFlags(const AZ::ConsoleCommandContainer& arguments);

            MeshFP::MeshDataIndicesForLod AcquireMeshIndices(uint32_t lodCount, uint32_t meshCount);
            void ReleaseMeshIndices(MeshFP::MeshDataIndicesForLod meshDataIndices);

            
            //! This pool allocator keeps post-culling data cache friendly
            //! It should only be used for data that is accessed after culling is complete
            AZ::AZStdIAllocator GetPostCullingPoolAllocator()
            {
                return AZStdIAllocator(&m_postCullingPoolAllocator);
            }

            IAllocator* GetPostCullingPoolAllocatorPtr()
            {
                return &m_postCullingPoolAllocator;
            }

            void LockMeshDataMutex()
            {
                m_meshDataMutex.lock();
            }

            void UnlockMeshDataMutex()
            {
                m_meshDataMutex.unlock();
            }
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

            void ResizePerViewInstanceVectors(size_t viewCount);
            void ProcessVisibilityListForView(size_t viewIndex, const RPI::ViewPtr& view);
            void SortInstanceDataForView(size_t viewIndex);
            void AddInstancedDrawPacketsTasksForView(TaskGraph& taskGraph, size_t viewIndex, const RPI::ViewPtr& view);
            void UpdateGPUInstanceBufferForView(size_t viewIndex, const RPI::ViewPtr& view);

            AZStd::concurrency_checker m_meshDataChecker;

        public:
            enum ModelDataIndex
            {
                Instance = 0,
                EndCullingData
            };
        private:
            MultiIndexedStableDynamicArray<512, AZStd::allocator, Render::ModelDataInstance, MeshFP::EndCullingData> m_modelData;

        public:
            struct MeshData
            {
                // In the metadata, this holds the meshOffset for an lod
                uint32_t m_instanceGroupHandle_metaDataMeshOffset;
                // In the metadata, this holds the mesh count for an lod
                uint32_t m_objectId_metaDataMeshCount;
            };
            AZStd::vector<MeshData> m_meshData;

            struct SortInstanceData
            {
                uint32_t m_instanceIndex = 0;
                uint32_t m_objectId = 0;
                float m_depth = 0.0f;

                bool operator<(const SortInstanceData& rhs)
                {
                    return AZStd::tie(m_instanceIndex,  m_depth) < AZStd::tie(rhs.m_instanceIndex, rhs.m_depth);
                }
            };

            AZStd::vector<AZStd::vector<SortInstanceData, AZStdIAllocator>, AZStdIAllocator> m_perViewSortInstanceData;
        private:
            AZStd::mutex m_meshDataMutex;
            RHI::FreeListAllocator m_meshDataAllocator;

            // Use this pool allocator to keep data that is accessed after culling cache friendly
            PostCullingMeshDataAllocator m_postCullingPoolAllocator;
            MeshInstanceManager m_meshInstanceManager;
            // TODO: handle this in a better way, but for now we're using this to iterate over each instance group exactly once
            AZStd::unordered_set<MeshInstanceManager::Handle> m_instanceGroupIndices;
            TransformServiceFeatureProcessor* m_transformService;
            RayTracingFeatureProcessor* m_rayTracingFeatureProcessor = nullptr;
            ReflectionProbeFeatureProcessor* m_reflectionProbeFeatureProcessor = nullptr;
            AZ::RPI::ShaderSystemInterface::GlobalShaderOptionUpdatedEvent::Handler m_handleGlobalShaderOptionUpdate;
            RPI::MeshDrawPacketLods m_emptyDrawPacketLods;
            RHI::Ptr<FlagRegistry> m_flagRegistry = nullptr;
            AZ::RHI::Handle<uint32_t> m_meshMovedFlag;
            AZStd::vector<AZStd::vector<uint32_t, AZStdIAllocator>, AZStdIAllocator> m_perViewInstanceData;
            AZStd::vector<GpuBufferHandler> m_perViewInstanceDataBufferHandlers;
            bool m_forceRebuildDrawPackets = false;
            bool m_reportShaderOptionFlags = false;
            bool m_enablePerMeshShaderOptionFlags = false;
        };
    } // namespace Render
} // namespace AZ
