/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <Atom/Feature/SkinnedMesh/SkinnedMeshInputBuffers.h>
#include <Atom/Feature/SkinnedMesh/SkinnedMeshShaderOptions.h>
#include <SkinnedMesh/SkinnedMeshShaderOptionsCache.h>

#include <Atom/RHI/DispatchItem.h>
#include <AtomCore/Instance/Instance.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>

namespace AZ
{
    namespace RHI
    {
        class BufferView;
        class PipelineState;
    }

    namespace RPI
    {
        class Buffer;
        class ModelLod;
        class Shader;
        class ShaderResourceGroup;
    }

    namespace Render
    {
        class SkinnedMeshComputePass;

        //! Holds and manages an RHI DispatchItem for a specific skinned mesh, and the resources that are needed to build and maintain it.
        class SkinnedMeshDispatchItem
            : private SkinnedMeshShaderOptionNotificationBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(SkinnedMeshDispatchItem, AZ::SystemAllocator, 0);

            SkinnedMeshDispatchItem() = delete;
            //! Create one dispatch item per mesh for each actor instance
            explicit SkinnedMeshDispatchItem(
                AZStd::intrusive_ptr<SkinnedMeshInputBuffers> inputBuffers,
                const AZStd::vector<uint32_t>& outputBufferOffsetsInBytes,
                size_t lodIndex,
                Data::Instance<RPI::Buffer> skinningMatrices,
                const SkinnedMeshShaderOptions& shaderOptions,
                RPI::Ptr<SkinnedMeshComputePass> skinnedMeshComputePass,
                uint32_t morphTargetDeltaOffsetInBytes,
                float morphTargetDeltaIntegerEncoding
            );
            ~SkinnedMeshDispatchItem();

            // The event handler cannot be copied
            AZ_DISABLE_COPY_MOVE(SkinnedMeshDispatchItem);

            bool Init();

            const RHI::DispatchItem& GetRHIDispatchItem() const;

            Data::Instance<RPI::Buffer> GetBoneTransforms() const;
            AZStd::array_view<RHI::Ptr<RHI::BufferView>> GetSourceUnskinnedBufferViews() const;
            AZStd::array_view<RHI::Ptr<RHI::BufferView>> GetTargetSkinnedBufferViews() const;
            size_t GetVertexCount() const;
        private:
            // SkinnedMeshShaderOptionNotificationBus::Handler
            void OnShaderReinitialized(const CachedSkinnedMeshShaderOptions* cachedShaderOptions) override;

            RHI::DispatchItem m_dispatchItem;

            // The skinning shader used for this instance
            Data::Instance<RPI::Shader> m_skinningShader;

            // Offsets into the SkinnedMeshOutputVertexStream where the lod streams start 
            AZStd::vector<uint32_t> m_outputBufferOffsetsInBytes;

            // The unskinned vertices used as the source of the skinning
            AZStd::intrusive_ptr<SkinnedMeshInputBuffers> m_inputBuffers;

            // The index of the lod within m_inputBuffers that is represented by the DispatchItem
            size_t m_lodIndex;
            // The per-object shader resource group
            Data::Instance<RPI::ShaderResourceGroup> m_instanceSrg;

            // Writable buffer views used for writing the per-actorInstance skinning data in the skinning shader
            AZStd::array<AZ::RHI::Ptr<RHI::BufferView>, static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::NumVertexStreams)> m_actorInstanceBufferViews;
            Data::Instance<RPI::Buffer> m_boneTransforms;

            // Options for the skinning shader
            SkinnedMeshShaderOptions m_shaderOptions;
            RPI::ShaderOptionGroup m_shaderOptionGroup;

            // Byte offset into the SkinnedMeshOutputVertexStream buffer to a location that has the accumulated morph target deltas
            uint32_t m_morphTargetDeltaOffsetInBytes;
            // A conservative value for encoding/decoding the accumulated deltas
            float m_morphTargetDeltaIntegerEncoding;
        };

        //! The skinned mesh compute shader has Nx1x1 threads per group and dispatches a total number of threads greater than or equal to the number of vertices in the mesh, with one vertex skinned per thread.
        //! We increase the total number of threads along the x dimension until it overflows what can fit in that dimension,
        //! and subsequently increment the total number of threads in the y dimension as much as needed for the total number of threads to equal or exceed the vertex count.
        void CalculateSkinnedMeshTotalThreadsPerDimension(uint32_t vertexCount, uint32_t& xThreads, uint32_t& yThreads);
    } // namespace Render
} // namespace AZ
