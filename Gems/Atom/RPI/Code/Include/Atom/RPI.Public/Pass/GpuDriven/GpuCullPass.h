/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>

namespace AZ
{
    namespace RPI
    {
        //! A compute pass specialized for GPU-driven frustum culling.
        //! Reads from the GpuInstanceData buffer in SceneSrg and writes surviving
        //! instances into an indirect draw argument buffer and a count buffer.
        class ATOM_RPI_PUBLIC_API GpuCullPass
            : public ComputePass
        {
            AZ_RPI_PASS(GpuCullPass);

        public:
            AZ_RTTI(GpuCullPass, "{A3B1C2D4-E5F6-7890-ABCD-EF1234567890}", ComputePass);
            AZ_CLASS_ALLOCATOR(GpuCullPass, SystemAllocator);
            virtual ~GpuCullPass() = default;

            static Ptr<GpuCullPass> Create(const PassDescriptor& descriptor);

            void SetInstanceCount(uint32_t instanceCount);

        protected:
            GpuCullPass(const PassDescriptor& descriptor);

            // Pass behavior overrides
            void BuildInternal() override;

            // ScopeProducer overrides
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

        private:
            static constexpr uint32_t ThreadGroupSize = 64;

            uint32_t m_instanceCount = 0;

            // Binding for the draw count buffer (cleared to 0 before dispatch)
            PassAttachmentBinding* m_drawCountBinding = nullptr;
            Name m_drawCountSlotName;

            // -- Phase 7: persistent per-batch visibility bitfield (two-pass occlusion culling) --
            // Only ever populated when this pass instance owns a BufferAttachments entry named
            // "VisibilityBitfieldBuffer" (see BuildInternal) -- the stock single-pass
            // GpuFrustumCullTemplate declares no such attachment, so every branch below is skipped
            // for it and that path stays byte-identical to before this was added.
            // One bit per batch slot (GPU_DRIVEN_MAX_BATCHES / 32); batch-level, not instance-level,
            // granularity -- see GpuCullPass.cpp for why.
            static constexpr uint32_t VisibilityBitfieldWords = 2048; // 65536 (GpuDrivenMaxBatches) / 32
            Data::Instance<Buffer> m_visibilityBitfieldBuffer;

            // True if this instance has a "VisibilityBitfield" slot, i.e. it is one of the Phase 7
            // two-pass occlusion cull kernels (pass 1 owns the buffer, pass 2 only references it) --
            // NOT the stock single-pass GpuFrustumCullPass. Gates CompileResources on
            // r_gpuTwoPassOcclusionCulling so the whole subtree dispatches zero threads (and
            // therefore draws nothing) until that cvar is flipped.
            bool IsTwoPassOcclusionCullInstance() const;

            // Wires up the persistent, cross-frame visibility bitfield buffer for `attachment`
            // (creating it once, on first BuildInternal). See the class comment above.
            void CreatePersistentVisibilityBitfield(Ptr<PassAttachment>& attachment);
        };
    } // namespace RPI
} // namespace AZ
