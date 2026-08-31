/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>

#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/IndirectBufferSignature.h>
#include <Atom/RHI/IndirectBufferView.h>

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Pass/RasterPass.h>
#include <Atom/RPI.Public/PipelineState.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>

namespace AZ
{
    namespace RPI
    {
        //! A raster pass that submits draw calls from a GPU-filled indirect buffer.
        //! Instead of iterating a CPU-side DrawList, it issues one ExecuteIndirect call per
        //! material-type bucket, sourced from a shared indirect-args buffer attachment.
        class ATOM_RPI_PUBLIC_API IndirectRasterPass
            : public RasterPass
            , private ShaderReloadNotificationBus::Handler
        {
            AZ_RPI_PASS(IndirectRasterPass);

        public:
            AZ_RTTI(IndirectRasterPass, "{B4C5D6E7-F8A9-0123-4567-89ABCDEF0123}", RasterPass);
            AZ_CLASS_ALLOCATOR(IndirectRasterPass, SystemAllocator);
            virtual ~IndirectRasterPass();

            static Ptr<IndirectRasterPass> Create(const PassDescriptor& descriptor);

            void SetMaxDrawCount(uint32_t maxCount);

            //! One ExecuteIndirect submission: the material type it draws (selects/creates the
            //! PSO + indirect signature) and the element range of the shared indirect-args buffer
            //! attachment that belongs to it (m_offset/m_count are element counts, not bytes).
            //!
            //! Contract: ranges must be non-overlapping slices of the same indirect-args buffer.
            //! ponytail: today's GPU-side buffer producer (GpuBatchFinalize.azsl) writes one entry
            //! per batchId in batchId order, NOT grouped by material type, so a caller can only
            //! supply a correct multi-bucket set once batchIds happen to be contiguous per type
            //! (true today because GpuDrivenEligibility only ever admits one material type).
            //! Upgrade path: have the GPU batch-finalize/scatter passes write per-type-grouped
            //! ranges once eligibility widens to more than one material type.
            struct IndirectDrawBucket
            {
                uint32_t m_materialTypeId = 0;
                uint32_t m_offset = 0;
                uint32_t m_count = 0;
            };

            //! Supply this frame's material-type buckets. Passing an empty list (the default,
            //! unchanged from before this API existed) makes the pass submit exactly one
            //! ExecuteIndirect over the whole buffer -- identical to today's depth-only behavior.
            void SetIndirectBuckets(AZStd::vector<IndirectDrawBucket> buckets);

        protected:
            IndirectRasterPass(const PassDescriptor& descriptor);

            // Pass behavior overrides
            void BuildInternal() override;
            void InitializeInternal() override;
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;

            // Scope producer overrides
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

        private:
            void LoadShader();

            // ShaderReloadNotificationBus::Handler overrides
            void OnShaderReinitialized(const Shader& shader) override;
            void OnShaderAssetReinitialized(const Data::Asset<ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const ShaderVariant& shaderVariant) override;

            // Slot names for the indirect buffer and optional count buffer
            Name m_indirectDrawBufferSlotName;
            Name m_countBufferSlotName;

            // Resolved bindings
            PassAttachmentBinding* m_indirectDrawBufferBinding = nullptr;
            PassAttachmentBinding* m_countBufferBinding = nullptr;

            //! Per-material-type PSO + indirect signature, built and invalidated independently
            //! of every other entry (a shader reload drops the whole map; a lazily-added entry
            //! for a newly-seen material type never disturbs entries already built).
            struct MaterialTypePso
            {
                PipelineStateForDraw m_pipelineStateForDraw;
                const RHI::PipelineState* m_pipelineState = nullptr;
                RHI::Ptr<RHI::IndirectBufferSignature> m_indirectBufferSignature;
                bool m_indirectSignatureBuilt = false;
            };

            //! Build (if not already built) the PSO + indirect signature for a material type.
            //! Only safe to call from single-threaded pass phases (CompileResources), never from
            //! the (possibly parallel) BuildCommandListInternal.
            MaterialTypePso& EnsurePsoEntry(uint32_t materialTypeId);
            void BuildIndirectSignature(MaterialTypePso& entry);

            //! m_buckets if the caller supplied any this frame, otherwise a single synthetic
            //! bucket spanning the whole buffer -- this is what keeps IndirectDepthPass (which
            //! never calls SetIndirectBuckets) behaving exactly as it did before this class grew
            //! multi-bucket support.
            AZStd::vector<IndirectDrawBucket> GetEffectiveBuckets() const;

            AZStd::unordered_map<uint32_t, MaterialTypePso> m_psoByMaterialType;
            AZStd::vector<IndirectDrawBucket> m_buckets;

            // The shader providing the PSO for indirect draws
            Data::Instance<Shader> m_shader;
            Data::Instance<ShaderResourceGroup> m_drawSrg;
            bool m_needsPipelineStateRebuild = true;

            // Cached buffer pointers resolved during CompileResources for use in BuildCommandListInternal
            const RHI::Buffer* m_cachedIndirectBuffer = nullptr;
            const RHI::Buffer* m_cachedCountBuffer = nullptr;

            uint32_t m_maxDrawCount = 0;
            bool m_indexedDraw = true;
        };
    } // namespace RPI
} // namespace AZ
