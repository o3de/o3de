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
#include <RayTracing/RayTracingFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        class TransformServiceFeatureProcessor;
        class RayTracingFeatureProcessor;
        class ReflectionProbeFeatureProcessor;

        class ModelDataInstance
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

            void DeInit(RayTracingFeatureProcessor* rayTracingFeatureProcessor);
            void QueueInit(const Data::Instance<RPI::Model>& model);
            void Init();
            void BuildDrawPacketList(size_t modelLodIndex);
            void SetRayTracingData(
                RayTracingFeatureProcessor* rayTracingFeatureProcessor,
                TransformServiceFeatureProcessor* transformServiceFeatureProcessor);
            void RemoveRayTracingData(RayTracingFeatureProcessor* rayTracingFeatureProcessor);
            void SetIrradianceData(
                RayTracingFeatureProcessor::SubMesh& subMesh,
                const Data::Instance<RPI::Material> material,
                const Data::Instance<RPI::Image> baseColorImage);
            void SetRayTracingReflectionProbeData(
                TransformServiceFeatureProcessor* transformServiceFeatureProcessor,
                ReflectionProbeFeatureProcessor* reflectionProbeFeatureProcessor,
                RayTracingFeatureProcessor::Mesh::ReflectionProbe& reflectionProbe);
            void SetSortKey(RHI::DrawItemSortKey sortKey);
            RHI::DrawItemSortKey GetSortKey() const;
            void SetMeshLodConfiguration(RPI::Cullable::LodConfiguration meshLodConfig);
            RPI::Cullable::LodConfiguration GetMeshLodConfiguration() const;
            void UpdateDrawPackets(bool forceUpdate = false);
            void BuildCullable();
            void UpdateCullBounds(const TransformServiceFeatureProcessor* transformService);
            void UpdateObjectSrg(
                ReflectionProbeFeatureProcessor* reflectionProbeFeatureProcessor,
                TransformServiceFeatureProcessor* transformServiceFeatureProcessor);
            bool MaterialRequiresForwardPassIblSpecular(Data::Instance<RPI::Material> material) const;
            void SetVisible(bool isVisible);
            CustomMaterialInfo GetCustomMaterialWithFallback(const CustomMaterialId& id) const;

            RPI::MeshDrawPacketLods m_drawPacketListsByLod;

            RPI::Cullable m_cullable;
            CustomMaterialMap m_customMaterials;
            MeshHandleDescriptor m_descriptor;
            Data::Instance<RPI::Model> m_model;

            //! A reference to the original model asset in case it got cloned before creating the model instance.
            Data::Asset<RPI::ModelAsset> m_originalModelAsset;

            //! List of object SRGs used by meshes in this model 
            AZStd::vector<Data::Instance<RPI::ShaderResourceGroup>> m_objectSrgList;
            AZStd::unique_ptr<MeshLoader> m_meshLoader;
            RPI::Scene* m_scene = nullptr;
            RHI::DrawItemSortKey m_sortKey = 0;

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

        static constexpr size_t foo = sizeof(ModelDataInstance);

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
                        
            AZStd::concurrency_checker m_meshDataChecker;
            StableDynamicArray<ModelDataInstance> m_modelData;
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
        };
    } // namespace Render
} // namespace AZ
