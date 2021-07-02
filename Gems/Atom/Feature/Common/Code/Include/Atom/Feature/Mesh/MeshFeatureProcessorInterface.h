/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/Event.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/functional.h>
#include <Atom/Feature/Material/MaterialAssignment.h>
#include <Atom/RPI.Public/Culling.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/Utils/StableDynamicArray.h>

namespace AZ
{
    namespace Render
    {
        class MeshDataInstance;

        //! Settings to apply to a mesh handle when acquiring it for the first time
        struct MeshHandleDescriptor
        {
            using RequiresCloneCallback = AZStd::function<bool(const Data::Asset<RPI::ModelAsset>& modelAsset)>;

            Data::Asset<RPI::ModelAsset> m_modelAsset;
            bool m_isRayTracingEnabled = true;
            bool m_useForwardPassIblSpecular = false;
            RequiresCloneCallback m_requiresCloneCallback = {};
        };

        //! MeshFeatureProcessorInterface provides an interface to acquire and release a MeshHandle from the underlying MeshFeatureProcessor
        class MeshFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::MeshFeatureProcessorInterface, "{975D7F0C-2E7E-4819-94D0-D3C4E2024721}", FeatureProcessor);

            using MeshHandle = StableDynamicArrayHandle<MeshDataInstance>;
            using ModelChangedEvent = Event<const Data::Instance<RPI::Model>>;

            //! Acquires a model with an optional collection of material assignments.
            //! @param requiresCloneCallback The callback indicates whether cloning is required for a given model asset.
            virtual MeshHandle AcquireMesh(
                const MeshHandleDescriptor& descriptor,
                const MaterialAssignmentMap& materials = {}) = 0;
            //! Acquires a model with a single material applied to all its meshes.
            virtual MeshHandle AcquireMesh(
                const MeshHandleDescriptor& descriptor,
                const Data::Instance<RPI::Material>& material) = 0;
            //! Releases the mesh handle
            virtual bool ReleaseMesh(MeshHandle& meshHandle) = 0;
            //! Creates a new instance and handle of a mesh using an existing MeshId. Currently, this will reset the new mesh to default materials.
            virtual MeshHandle CloneMesh(const MeshHandle& meshHandle) = 0;

            //! Gets the underlying RPI::Model instance for a meshHandle. May be null if the model has not loaded.
            virtual Data::Instance<RPI::Model> GetModel(const MeshHandle& meshHandle) const = 0;
            //! Gets the underlying RPI::ModelAsset for a meshHandle.
            virtual Data::Asset<RPI::ModelAsset> GetModelAsset(const MeshHandle& meshHandle) const = 0;
            //! Gets the ObjectSrg for a meshHandle.
            //! Updating the ObjectSrg should be followed by a call to QueueObjectSrgForCompile,
            //! instead of compiling the srg directly. This way, if the srg has already been queued for compile,
            //! it will not be queued twice in the same frame. The ObjectSrg should not be updated during
            //! Simulate, or it will create a race between updating the data and the call to Compile
            virtual Data::Instance<RPI::ShaderResourceGroup> GetObjectSrg(const MeshHandle& meshHandle) const = 0;
            //! Queues the object srg for compile.
            virtual void QueueObjectSrgForCompile(const MeshHandle& meshHandle) const = 0;
            //! Sets the MaterialAssignmentMap for a meshHandle, using just a single material for the DefaultMaterialAssignmentId.
            //! Note if there is already a material assignment map, this will replace the entire map with just a single material.
            virtual void SetMaterialAssignmentMap(const MeshHandle& meshHandle, const Data::Instance<RPI::Material>& material) = 0;
            //! Sets the MaterialAssignmentMap for a meshHandle.
            virtual void SetMaterialAssignmentMap(const MeshHandle& meshHandle, const MaterialAssignmentMap& materials) = 0;
            //! Gets the MaterialAssignmentMap for a meshHandle.
            virtual const MaterialAssignmentMap& GetMaterialAssignmentMap(const MeshHandle& meshHandle) const = 0;
            //! Connects a handler to any changes to an RPI::Model. Changes include loading and reloading.
            virtual void ConnectModelChangeEventHandler(const MeshHandle& meshHandle, ModelChangedEvent::Handler& handler) = 0;

            //! Sets the transform for a given mesh handle.
            virtual void SetTransform(const MeshHandle& meshHandle, const Transform& transform,
                const Vector3& nonUniformScale = Vector3::CreateOne()) = 0;
            //! Gets the transform for a given mesh handle.
            virtual Transform GetTransform(const MeshHandle& meshHandle) = 0;
            //! Gets the non-uniform scale for a given mesh handle.
            virtual Vector3 GetNonUniformScale(const MeshHandle& meshHandle) = 0;
            //! Sets the sort key for a given mesh handle.
            virtual void SetSortKey(const MeshHandle& meshHandle, RHI::DrawItemSortKey sortKey) = 0;
            //! Gets the sort key for a given mesh handle.
            virtual RHI::DrawItemSortKey GetSortKey(const MeshHandle& meshHandle) = 0;
            //! Sets an LOD override for a given mesh handle. This LOD will always be rendered instead being automatically determined.
            virtual void SetLodOverride(const MeshHandle& meshHandle, RPI::Cullable::LodOverride lodOverride) = 0;
            //! Gets the LOD override for a given mesh handle.
            virtual RPI::Cullable::LodOverride GetLodOverride(const MeshHandle& meshHandle) = 0;
            //! Sets the option to exclude this mesh from baked reflection probe cubemaps
            virtual void SetExcludeFromReflectionCubeMaps(const MeshHandle& meshHandle, bool excludeFromReflectionCubeMaps) = 0;
            //! Sets the option to exclude this mesh from raytracing
            virtual void SetRayTracingEnabled(const MeshHandle& meshHandle, bool rayTracingEnabled) = 0;
            //! Sets the mesh as visible or hidden.  When the mesh is hidden it will not be rendered by the feature processor.
            virtual void SetVisible(const MeshHandle& meshHandle, bool visible) = 0;
            //! Sets the mesh to render IBL specular in the forward pass.
            virtual void SetUseForwardPassIblSpecular(const MeshHandle& meshHandle, bool useForwardPassIblSpecular) = 0;
        };
    } // namespace Render
} // namespace AZ
