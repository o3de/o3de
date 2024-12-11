/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/TransformService/TransformServiceFeatureProcessorInterface.h>
#include <Atom/RHI/DispatchItem.h>
#include <Atom/RPI.Public/Culling.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/Utils/StableDynamicArray.h>
#include <AzCore/EBus/Event.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/functional.h>

namespace AZ
{
    namespace Render
    {
        AZ_CVAR(bool,
            r_enablePerMeshShaderOptionFlags,
            false,
            nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "Enable allowing systems to set shader options on a per-mesh basis."
        );

        
        AZ_CVAR(
            bool,
            r_meshInstancingEnabled,
            false,
            nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "Enable instanced draw calls in the MeshFeatureProcessor.");

        AZ_CVAR(
            bool,
            r_meshInstancingEnabledForTransparentObjects,
            false,
            nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "Enable instanced draw calls for transparent objects in the MeshFeatureProcessor. Use this only if you have many instances of the same "
            "transparent object, but don't have multiple different transparent objects mixed together. See documentation for details.");

        AZ_CVAR(
            size_t,
            r_meshInstancingBucketSortScatterBatchSize,
            512,
            nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "Batch size for the first stage of the mesh instancing bucket sort. "
            "Can be modified to find optimal load balancing for the multi-threaded tasks.");

        AZ_CVAR(
            bool,
            r_meshInstancingDebugForceUniqueObjectsForProfiling,
            false,
            nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "Enable instanced draw calls in the MeshFeatureProcessor, but force one object per draw call. "
            "This is helpful for simulating the worst case scenario for instancing for profiling performance.");

        struct MeshInstanceGroupData;
        class StreamBufferViewsBuilderInterface;

        //! Custom material infill containing a material instance that will be substituted for an embedded material on a model and UV
        //! mapping reassignments
        struct CustomMaterialInfo
        {
            Data::Instance<RPI::Material> m_material;
            RPI::MaterialModelUvOverrideMap m_uvMapping;
        };

        //! Mesh feature processor data types for customizing model materials
        using CustomMaterialLodIndex = AZ::u64;

        //! Pair referring to the lod index and unique id corresponding to the material slot where the material should be applied 
        using CustomMaterialId = AZStd::pair<CustomMaterialLodIndex, uint32_t>;

        // ModelDataInstanceInterface provides an interface to ModelDataInstance
        // It provides information about an instance of a model in the scene
        // The class can be accessed through MeshHandles from the MeshFeatureProcessor
        class ModelDataInstanceInterface
        {
            friend class MeshFeatureProcessor;
            friend class MeshLoader;

        public:
            AZ_RTTI(AZ::Render::ModelDataInstanceInterface, "{0B990760-AB5C-4357-A983-AD066EC9AC2E}");
            virtual ~ModelDataInstanceInterface() = default;

            virtual const Data::Instance<RPI::Model>& GetModel() = 0;
            virtual const RPI::Cullable& GetCullable() = 0;

            virtual const uint32_t GetLightingChannelMask() = 0;

            using InstanceGroupHandle = StableDynamicArrayWeakHandle<MeshInstanceGroupData>;

            //! PostCullingInstanceData represents the data the MeshFeatureProcessor needs after culling
            //! in order to generate instanced draw calls
            struct PostCullingInstanceData
            {
                InstanceGroupHandle m_instanceGroupHandle;
                uint32_t m_instanceGroupPageIndex;
                TransformServiceFeatureProcessorInterface::ObjectId m_objectId;
            };

            using PostCullingInstanceDataList = AZStd::vector<PostCullingInstanceData>;
            virtual const bool IsSkinnedMesh() = 0;
            virtual const AZ::Uuid& GetRayTracingUuid() const = 0;

            //! Internally called when a DrawPacket used by this ModelDataInstance was updated.
            virtual void HandleDrawPacketUpdate(uint32_t lodIndex, uint32_t meshIndex, RPI::MeshDrawPacket& meshDrawPacket) = 0;

            //! Event that let's us know whenever one of the MeshDrawPackets has been updated.
            //! This event can occur on multiple threads.
            //! Provides the ModelDataInstance parent object that owns the MeshDrawPacket.
            using MeshDrawPacketUpdatedEvent = Event<const ModelDataInstanceInterface&, uint32_t /*lodIndex*/, uint32_t /*meshIndex*/, const AZ::RPI::MeshDrawPacket&>;
            //! Connects @handler to the MeshDrawPacketUpdatedEvent.
            //! One of the most common reasons a MeshDrawPacket gets updated is
            //! when a RenderPipeline is instantiated at runtime and it happens to contain
            //! a RasterPass with a DrawListTag that matches one of the Shaders of one of the Materials in
            //! a Mesh. Another scenario is when Shader assets or Material assets are reloaded.
            virtual void ConnectMeshDrawPacketUpdatedHandler(MeshDrawPacketUpdatedEvent::Handler& handler) = 0;

            virtual CustomMaterialInfo GetCustomMaterialWithFallback(const CustomMaterialId& id) const = 0;
        };

        using CustomMaterialMap = AZStd::unordered_map<CustomMaterialId, CustomMaterialInfo>;
        static const CustomMaterialLodIndex DefaultCustomMaterialLodIndex = AZStd::numeric_limits<CustomMaterialLodIndex>::max();
        static const uint32_t DefaultCustomMaterialStableId = AZStd::numeric_limits<uint32_t>::max();
        static const CustomMaterialId DefaultCustomMaterialId = CustomMaterialId(DefaultCustomMaterialLodIndex, DefaultCustomMaterialStableId);
        static const CustomMaterialMap DefaultCustomMaterialMap = CustomMaterialMap();

        //! Settings to apply to a mesh handle when acquiring it for the first time
        struct MeshHandleDescriptor
        {
            using RequiresCloneCallback = AZStd::function<bool(const Data::Asset<RPI::ModelAsset>& modelAsset)>;
            using ModelChangedEvent = Event<const Data::Instance<RPI::Model>&>;
            using ObjectSrgCreatedEvent = Event<const Data::Instance<RPI::ShaderResourceGroup>&>;

            MeshHandleDescriptor() = default;

            MeshHandleDescriptor(const Data::Asset<RPI::ModelAsset>& modelAsset)
                : m_modelAsset(modelAsset)
            {
            }

            MeshHandleDescriptor(const Data::Asset<RPI::ModelAsset>& modelAsset, const CustomMaterialMap& customMaterials)
                : m_modelAsset(modelAsset)
                , m_customMaterials(customMaterials)
            {
            }

            MeshHandleDescriptor(const Data::Asset<RPI::ModelAsset>& modelAsset, const Data::Instance<RPI::Material>& material)
                : m_modelAsset(modelAsset)
                , m_customMaterials({ { AZ::Render::DefaultCustomMaterialId, { material, {} } } })
            {
            }

            AZ::EntityId m_entityId{ AZ::EntityId::InvalidEntityId };
            Data::Asset<RPI::ModelAsset> m_modelAsset;
            bool m_isRayTracingEnabled = true;
            bool m_useForwardPassIblSpecular = false;
            bool m_isAlwaysDynamic = false;
            bool m_excludeFromReflectionCubeMaps = false;
            bool m_isSkinnedMesh = false;
            bool m_supportRayIntersection = false;

            CustomMaterialMap m_customMaterials;

            RequiresCloneCallback m_requiresCloneCallback{};

            //! Connects to an event that gets triggered whenever the model is changed, loaded, or reloaded.
            ModelChangedEvent::Handler m_modelChangedEventHandler{ [](const Data::Instance<RPI::Model>&){} };

            //! Connects to an event that triggers whenever the ObjectSrg is created.
            ObjectSrgCreatedEvent::Handler m_objectSrgCreatedHandler{ [](const Data::Instance<RPI::ShaderResourceGroup>&){} };
        };

        //! Helper structure used to create a DispatchItem from a DrawItem.
        //! This structure is created, optionally and on demand, by the MeshFeatureProcessor
        //! only for DrawItems with a PipelineState of Compute type.
        struct DispatchDrawItem
        {
            DispatchDrawItem() = delete;
            explicit DispatchDrawItem(const RHI::DrawItem* drawItem)
                : m_drawItem(drawItem)
                , m_distpatchItem(RHI::MultiDevice::AllDevices)
            {
            }
            const RHI::DrawItem* m_drawItem = nullptr;
            RHI::DispatchItem m_distpatchItem;
        };
        using DispatchDrawItemList = AZStd::vector<DispatchDrawItem>;

        //! MeshFeatureProcessorInterface provides an interface to acquire and release a MeshHandle from the underlying
        //! MeshFeatureProcessor
        class MeshFeatureProcessorInterface : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::MeshFeatureProcessorInterface, "{975D7F0C-2E7E-4819-94D0-D3C4E2024721}", AZ::RPI::FeatureProcessor);

            using MeshHandle = StableDynamicArrayHandle<ModelDataInstanceInterface>;

            //! Returns the object id for a mesh handle.
            virtual TransformServiceFeatureProcessorInterface::ObjectId GetObjectId(const MeshHandle& meshHandle) const = 0;

            //! Acquire a mesh handle for a model configured using the descriptor
            virtual MeshHandle AcquireMesh(const MeshHandleDescriptor& descriptor) = 0;
            //! Releases the mesh handle
            virtual bool ReleaseMesh(MeshHandle& meshHandle) = 0;
            //! Creates a new instance and handle of a mesh using an existing MeshId. Currently, this will reset the new mesh to default materials.
            virtual MeshHandle CloneMesh(const MeshHandle& meshHandle) = 0;

            //! Gets the underlying RPI::Model instance for a meshHandle. May be null if the model has not loaded.
            virtual Data::Instance<RPI::Model> GetModel(const MeshHandle& meshHandle) const = 0;
            //! Gets the underlying RPI::ModelAsset for a meshHandle.
            virtual Data::Asset<RPI::ModelAsset> GetModelAsset(const MeshHandle& meshHandle) const = 0;

            //! This function provides insight into what materials, shaders, etc. are actively being used to render the model.
            //! Useful for custom feature processors that work in tandem with the MeshFeatureProcessor.
            virtual const RPI::MeshDrawPacketLods& GetDrawPackets(const MeshHandle& meshHandle) const = 0;

            //! Gets the ObjectSrgs for a meshHandle.
            //! Updating the ObjectSrgs should be followed by a call to QueueObjectSrgForCompile,
            //! instead of compiling the srgs directly. This way, if the srgs have already been queued for compile,
            //! they will not be queued twice in the same frame. The ObjectSrgs should not be updated during
            //! Simulate, or it will create a race between updating the data and the call to Compile
            //! Cases where there may be multiple ObjectSrgs: if a model has multiple submeshes and those submeshes use different
            //! materials with different object SRGs.
            virtual const AZStd::vector<Data::Instance<RPI::ShaderResourceGroup>>& GetObjectSrgs(const MeshHandle& meshHandle) const = 0;
            //! Queues the object srg for compile.
            virtual void QueueObjectSrgForCompile(const MeshHandle& meshHandle) const = 0;
            //! Sets the CustomMaterialMap for a meshHandle, using just a single material for the DefaultCustomMaterialId.
            //! Note if there is already a CustomMaterialMap, this will replace the entire map with just a single material.
            virtual void SetCustomMaterials(const MeshHandle& meshHandle, const Data::Instance<RPI::Material>& material) = 0;
            //! Sets the CustomMaterialMap for a meshHandle.
            virtual void SetCustomMaterials(const MeshHandle& meshHandle, const CustomMaterialMap& materials) = 0;
            //! Gets the CustomMaterialMap for a meshHandle.
            virtual const CustomMaterialMap& GetCustomMaterials(const MeshHandle& meshHandle) const = 0;

            //! Enables/Disables the mesh's DrawItem for the given drawListTag
            virtual void SetDrawItemEnabled(const MeshHandle& meshHandle, RHI::DrawListTag drawListTag, bool enabled) = 0;
            //! Sets the transform for a given mesh handle.
            virtual void SetTransform(const MeshHandle& meshHandle, const Transform& transform,
                const Vector3& nonUniformScale = Vector3::CreateOne()) = 0;
            //! Gets the transform for a given mesh handle.
            virtual Transform GetTransform(const MeshHandle& meshHandle) = 0;
            //! Gets the non-uniform scale for a given mesh handle.
            virtual Vector3 GetNonUniformScale(const MeshHandle& meshHandle) = 0;
            //! Sets the local space bbox for a given mesh handle. You don't need to call this for static models, only skinned/animated models
            virtual void SetLocalAabb(const MeshHandle& meshHandle, const AZ::Aabb& localAabb) = 0;
            //! Gets the local space bbox for a given mesh handle. Unless SetLocalAabb has been called before, this will be the bbox of the model asset
            virtual AZ::Aabb GetLocalAabb(const MeshHandle& meshHandle) const = 0;
            //! Sets the sort key for a given mesh handle.
            virtual void SetSortKey(const MeshHandle& meshHandle, RHI::DrawItemSortKey sortKey) = 0;
            //! Gets the sort key for a given mesh handle.
            virtual RHI::DrawItemSortKey GetSortKey(const MeshHandle& meshHandle) const = 0;
            //! Sets the lighting channel mask for a given mesh handle.
            virtual void SetLightingChannelMask(const MeshHandle& meshHandle, uint32_t lightingChannelMask) = 0;
            //! Gets the lighting channel mask for a given mesh handle.
            virtual uint32_t GetLightingChannelMask(const MeshHandle& meshHandle) const = 0;
            //! Sets LOD mesh configurations to be used in the Mesh Feature Processor
            virtual void SetMeshLodConfiguration(const MeshHandle& meshHandle, const RPI::Cullable::LodConfiguration& meshLodConfig) = 0;
            //! Gets the LOD mesh configurations being used in the Mesh Feature Processor
            virtual RPI::Cullable::LodConfiguration GetMeshLodConfiguration(const MeshHandle& meshHandle) const = 0;
            //! Sets the option to exclude this mesh from baked reflection probe cubemaps
            virtual void SetExcludeFromReflectionCubeMaps(const MeshHandle& meshHandle, bool excludeFromReflectionCubeMaps) = 0;
            //! Gets the if this mesh is excluded from baked reflection probe cubemaps
            virtual bool GetExcludeFromReflectionCubeMaps(const MeshHandle& meshHandle) const = 0;
            //! Sets a mesh to be considered to be always moving even if the transform hasn't changed. This is useful for meshes that are skinned or have vertex animation.
            virtual void SetIsAlwaysDynamic(const MeshHandle& meshHandle, bool isAlwaysDynamic) = 0;
            //! Gets if a mesh is considered to always be moving.
            virtual bool GetIsAlwaysDynamic(const MeshHandle& meshHandle) const = 0;
            //! Sets the option to exclude this mesh from raytracing
            virtual void SetRayTracingEnabled(const MeshHandle& meshHandle, bool enabled) = 0;
            //! Gets whether this mesh is excluded from raytracing
            virtual bool GetRayTracingEnabled(const MeshHandle& meshHandle) const = 0;
            //! Sets the mesh as visible or hidden.  When the mesh is hidden it will not be rendered by the feature processor.
            virtual void SetVisible(const MeshHandle& meshHandle, bool visible) = 0;
            //! Returns the visibility state of the mesh.
            //! This only refers to whether or not the mesh has been explicitly hidden, and is not related to view frustum visibility.
            virtual bool GetVisible(const MeshHandle& meshHandle) const = 0;
            //! Sets the mesh to render IBL specular in the forward pass.
            virtual void SetUseForwardPassIblSpecular(const MeshHandle& meshHandle, bool useForwardPassIblSpecular) = 0;
            //! Set a flag that the ray tracing data needs to be updated, usually after material changes. 
            virtual void SetRayTracingDirty(const MeshHandle& meshHandle) = 0;
            //! Print out info about the mesh draw packet
            virtual void PrintDrawPacketInfo(const MeshHandle& meshHandle) = 0;
            //! A helper function, typically called by another FeatureProcessor, when Compute or RayTracing shaders need to bind
            //! Mesh Input Streams like "POSITION", "NORMAL", "UV1" etc as regular AZ::RHI::BufferViews.
            //! This function instantiates a concrete Builder-like object that helps creating the RHI::BufferViews. 
            virtual AZStd::unique_ptr<StreamBufferViewsBuilderInterface> CreateStreamBufferViewsBuilder(const MeshHandle& meshHandle) const = 0;
            //! MaterialTypes and MaterialPipelines support Compute Shaders (With DrawListTag) in their ShaderItem collections.
            //! Given that this is an uncommon use case, the DispatchItems are not created automatically by the MeshDrawPacket.
            //! Additionally DispatchItems require knowledge of the Total number of threads X,Y,Z, which should be customizable.
            //! The following function helps the creation of the DispatchItems and the user must supply a callback that
            //! allows full control on the number of Total Threads X,Y,Z.
            //! REMARK 1: It is recommended to call this function whenever ModelDataInstanceInterface::MeshDrawPacketUpdatedEvent is signaled.
            //! REMARK 2: This function is typically called by a custom FeatureProcessor that leverages the MeshFeatureProcessor.
            //!           The custom FeatureProcessor will own the returned list and submit the DispatchItems in a custom Pass.
            using DispatchArgumentsSetupCB = AZStd::function<void(uint32_t /*lodIndex*/, uint32_t /*meshIndex*/, uint32_t /*drawItemIdx*/, const RHI::DrawItem*, RHI::DispatchDirect&)>;
            //! DisptachItems will be created for the DrawItems that match both the @drawListTagsFilter and @materialPipelineFilter.
            //! Also, only DrawItems whose PipelineState is of Compute type will be considered.
            virtual DispatchDrawItemList BuildDispatchDrawItemList(
                const MeshHandle& meshHandle,
                const uint32_t lodIndex,
                const uint32_t meshIndex,
                const RHI::DrawListMask drawListTagsFilter,
                const RHI::DrawFilterMask materialPipelineFilter,
                DispatchArgumentsSetupCB dispatchArgumentsSetupCB) const = 0;

        };
    } // namespace Render
} // namespace AZ
