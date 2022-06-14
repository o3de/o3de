/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/Feature/SkinnedMesh/SkinnedMeshInstance.h>
#include <Atom/Feature/SkinnedMesh/SkinnedMeshShaderOptions.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <AtomCore/Instance/Instance.h>

namespace AZ
{
    namespace Render
    {
        class SkinnedMeshInputBuffers;
        class SkinnedMeshRenderProxy;

        //! SkinnedMeshFeatureProcessorInterface provides an interface to acquire and release a SkinnedMeshRenderProxy from the underlying SkinnedMeshFeatureProcessor
        class SkinnedMeshFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::SkinnedMeshFeatureProcessorInterface, "{6BE6D9D7-FFD7-4C35-9A84-4EFDE730F06B}");

            using SkinnedMeshHandle = StableDynamicArrayHandle<SkinnedMeshRenderProxy>;

            struct SkinnedMeshHandleDescriptor
            {
                Data::Instance<SkinnedMeshInputBuffers> m_inputBuffers;
                AZStd::intrusive_ptr<SkinnedMeshInstance> m_instance;
                AZStd::shared_ptr<MeshFeatureProcessorInterface::MeshHandle> m_meshHandle;
                Data::Instance<RPI::Buffer> m_boneTransforms;
                SkinnedMeshShaderOptions m_shaderOptions;
            };

            //! Given a descriptor of the input and output for skinning, acquire a handle to the instance that will be skinned
            virtual SkinnedMeshHandle AcquireSkinnedMesh(const SkinnedMeshHandleDescriptor& desc) = 0;
            //! Releases the skinned mesh handle
            virtual bool ReleaseSkinnedMesh(SkinnedMeshHandle& handle) = 0;
            //! Updates the data for the skinning transforms of a given skinned mesh handle
            virtual void SetSkinningMatrices(const SkinnedMeshHandle& handle, const AZStd::vector<float>& data) = 0;
            //! Updates the morph target weights for all meshes of a given lod of a skinned mesh handle
            //! The weights should be in the order that the morph targets were initially added to the SkinnedMeshInputBuffers for the handle
            virtual void SetMorphTargetWeights(const SkinnedMeshHandle& handle, uint32_t lodIndex, const AZStd::vector<float>& weights) = 0;
            //! Enable skinning for a given mesh and lod of a skinned mesh handle
            virtual void EnableSkinning(const SkinnedMeshHandle& handle, uint32_t lodIndex, uint32_t meshIndex) = 0;
            //! Disable skinning for a given mesh and lod of a skinned mesh handle
            virtual void DisableSkinning(const SkinnedMeshHandle& handle, uint32_t lodIndex, uint32_t meshIndex) = 0;
        };
    } // namespace Render
} // namespace AZ
