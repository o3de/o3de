/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Pass/GpuDriven/GpuCullPass.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Reflect/Pass/ComputePassData.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace RPI
    {
        Ptr<GpuCullPass> GpuCullPass::Create(const PassDescriptor& descriptor)
        {
            Ptr<GpuCullPass> pass = aznew GpuCullPass(descriptor);
            return pass;
        }

        GpuCullPass::GpuCullPass(const PassDescriptor& descriptor)
            : ComputePass(descriptor)
        {
            m_drawCountSlotName = Name("DrawCount");
        }

        void GpuCullPass::SetInstanceCount(uint32_t instanceCount)
        {
            m_instanceCount = instanceCount;
        }

        void GpuCullPass::BuildInternal()
        {
            // Set buffer view descriptors to match the shader SRG variable types.
            // Each buffer must have the correct stride/format for its SRG binding:
            //   m_indirectDrawArgs  -> RWStructuredBuffer<DrawIndirectCommand> (stride 16)
            //   m_drawCount         -> RWByteAddressBuffer (Raw)
            //   m_visibleInstanceIds -> RWStructuredBuffer<uint> (stride 4)
            static const Name indirectDrawArgsBufferName("IndirectDrawArgsBuffer");
            static const Name drawCountBufferName("DrawCountBuffer");
            static const Name survivorsBufferName("SurvivorsBuffer");
            static const Name survivorCountBufferName("SurvivorCountBuffer");
            static const Name visibilityBitfieldBufferName("VisibilityBitfieldBuffer");

            for (auto& attachment : m_ownedAttachments)
            {
                if (attachment->GetAttachmentType() != RHI::AttachmentType::Buffer)
                {
                    continue;
                }

                uint32_t byteCount = static_cast<uint32_t>(attachment->m_descriptor.m_buffer.m_byteCount);

                if (attachment->m_name == indirectDrawArgsBufferName)
                {
                    // GpuDrivenIndirectCommand = rootconstant(4) + DrawIndirectCommand(16) = 20 bytes
                    attachment->m_descriptor.m_bufferView =
                        RHI::BufferViewDescriptor::CreateStructured(0, AZStd::max(1u, byteCount / 20), 20);
                }
                else if (attachment->m_name == survivorsBufferName)
                {
                    // StructuredBuffer<uint2> (stride 8)
                    attachment->m_descriptor.m_bufferView =
                        RHI::BufferViewDescriptor::CreateStructured(0, AZStd::max(1u, byteCount / 8), 8);
                }
                else if (attachment->m_name == drawCountBufferName || attachment->m_name == survivorCountBufferName)
                {
                    // RWByteAddressBuffer -> Raw view (Format::R32_UINT internally)
                    attachment->m_descriptor.m_bufferView =
                        RHI::BufferViewDescriptor::CreateRaw(0, AZStd::max(4u, byteCount));
                }
                else if (attachment->m_name == visibilityBitfieldBufferName)
                {
                    // Phase 7: persistent, cross-frame -- RWStructuredBuffer<uint> (stride 4), one bit
                    // per batch slot. See CreatePersistentVisibilityBitfield.
                    attachment->m_descriptor.m_bufferView =
                        RHI::BufferViewDescriptor::CreateStructured(0, VisibilityBitfieldWords, 4);
                    CreatePersistentVisibilityBitfield(attachment);
                }
                else if (attachment->m_descriptor.m_bufferView.m_elementSize == 0)
                {
                    // Default: StructuredBuffer<uint> (stride 4)
                    attachment->m_descriptor.m_bufferView =
                        RHI::BufferViewDescriptor::CreateStructured(0, AZStd::max(1u, byteCount / 4), 4);
                }
            }

            ComputePass::BuildInternal();

            m_drawCountBinding = nullptr;
            for (auto& binding : m_attachmentBindings)
            {
                if (binding.m_name == m_drawCountSlotName)
                {
                    m_drawCountBinding = &binding;
                    break;
                }
            }
        }

        bool GpuCullPass::IsTwoPassOcclusionCullInstance() const
        {
            return FindAttachmentBinding(Name("VisibilityBitfield")) != nullptr;
        }

        void GpuCullPass::CreatePersistentVisibilityBitfield(Ptr<PassAttachment>& attachment)
        {
            if (!m_visibilityBitfieldBuffer)
            {
                // Zero-initialized: "nothing was visible last frame" until pass 2 populates it --
                // conservative for pass 1 (it simply redraws nothing extra the first frame; pass 2
                // still frustum+HiZ-tests everything and fills the buffer in for the next frame).
                AZStd::vector<uint32_t> zeroInit(VisibilityBitfieldWords, 0);

                CommonBufferDescriptor desc;
                desc.m_bufferName = AZStd::string::format("%s.VisibilityBitfield", GetPathName().GetCStr());
                desc.m_poolType = CommonBufferPoolType::ReadWrite;
                desc.m_elementSize = sizeof(uint32_t);
                desc.m_byteCount = VisibilityBitfieldWords * sizeof(uint32_t);
                desc.m_bufferData = zeroInit.data();
                m_visibilityBitfieldBuffer = BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
                AZ_Assert(
                    m_visibilityBitfieldBuffer,
                    "GpuCullPass '%s': failed to create the persistent visibility bitfield buffer.",
                    GetPathName().GetCStr());
            }

            // Imported (cross-frame) lifetime: m_importedResource must be set BEFORE any
            // PassAttachmentBinding::SetAttachment call on this attachment or it asserts -- so this
            // attachment is deliberately NOT wired via a "Connections" entry in the .pass JSON (which
            // would call SetAttachment while building, before this method has run). Bind its slot
            // manually instead, after the resource exists. Same pattern as
            // ReflectionScreenSpaceFilterPass's persistent History image.
            attachment->m_lifetime = RHI::AttachmentLifetimeType::Imported;
            attachment->m_importedResource = m_visibilityBitfieldBuffer;

            if (PassAttachmentBinding* binding = FindAttachmentBinding(Name("VisibilityBitfield")))
            {
                binding->SetAttachment(attachment);
            }
        }

        void GpuCullPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            uint32_t instanceCount = m_instanceCount;

            // Phase 7 two-pass occlusion cull kernels are wired but off by default: dispatch zero
            // threads (nothing survives culling, nothing draws) until r_gpuTwoPassOcclusionCulling is
            // explicitly enabled. Does not affect the stock single-pass GpuFrustumCullPass instance.
            bool forceZeroDispatch = false;
            if (IsTwoPassOcclusionCullInstance())
            {
                bool twoPassEnabled = false;
                if (auto* console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
                {
                    console->GetCvarValue("r_gpuTwoPassOcclusionCulling", twoPassEnabled);
                }
                if (!twoPassEnabled)
                {
                    instanceCount = 0;
                    forceZeroDispatch = true;
                }
            }

            uint32_t threadGroupCountX = (instanceCount + ThreadGroupSize - 1) / ThreadGroupSize;
            // BUG FIX: the unconditional max(...,1u) floor below used to force a minimum of one thread
            // group (64 threads) even when forceZeroDispatch just set instanceCount to 0 as a deliberate
            // "draw nothing" gate. Those 64 threads read the REAL SceneSrg::m_gpuInstanceCount inside the
            // shader (unrelated to this pass-local instanceCount), so they would process up to 64 real
            // scene instances regardless of the cvar the moment this subtree is wired into a pipeline --
            // silently defeating the off-by-default guarantee. Only apply the floor when NOT deliberately
            // zeroed; a genuine SetTargetThreadCounts(0,0,0) dispatch is a well-defined GPU no-op.
            if (!forceZeroDispatch)
            {
                threadGroupCountX = AZStd::max(threadGroupCountX, 1u);
            }
            SetTargetThreadCounts(threadGroupCountX * ThreadGroupSize, 1, 1);

            ComputePass::CompileResources(context);
        }

        void GpuCullPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            // The draw count buffer clear is handled by the pass template's LoadAction::Clear
            // on the DrawCount slot, which zeros the buffer at the start of the scope.
            // The cull shader atomically increments this counter for each visible instance.
            ComputePass::BuildCommandListInternal(context);
        }

    } // namespace RPI
} // namespace AZ
