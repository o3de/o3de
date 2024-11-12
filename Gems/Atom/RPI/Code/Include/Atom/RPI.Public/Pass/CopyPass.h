/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI.Reflect/Viewport.h>
#include <Atom/RHI/CopyItem.h>
#include <Atom/RHI/ScopeProducer.h>

#include <Atom/RPI.Reflect/Pass/CopyPassData.h>

#include <Atom/RPI.Public/Pass/Pass.h>

namespace AZ
{
    namespace RPI
    {
        //! A copy pass is a leaf pass (pass with no children) used for copying images and buffers on the GPU.
        class CopyPass : public Pass
        {
            AZ_RPI_PASS(CopyPass);

        public:
            AZ_RTTI(CopyPass, "{7387500D-B1BA-4916-B38C-24F5C8DAF839}", Pass);
            AZ_CLASS_ALLOCATOR(CopyPass, SystemAllocator);
            virtual ~CopyPass();

            static Ptr<CopyPass> Create(const PassDescriptor& descriptor);

        protected:
            explicit CopyPass(const PassDescriptor& descriptor);

            void CopyBuffer(const RHI::FrameGraphCompileContext& context);
            void CopyImage(const RHI::FrameGraphCompileContext& context);
            void CopyBufferToImage(const RHI::FrameGraphCompileContext& context);
            void CopyImageToBuffer(const RHI::FrameGraphCompileContext& context);

            // Pass behavior overrides
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            void ResetInternal() override;

            // Scope producer functions...
            void SetupFrameGraphDependenciesSameDevice(RHI::FrameGraphInterface frameGraph);
            void CompileResourcesSameDevice(const RHI::FrameGraphCompileContext& context);
            void BuildCommandListInternalSameDevice(const RHI::FrameGraphExecuteContext& context);
            void SetupFrameGraphDependenciesDeviceToHost(RHI::FrameGraphInterface frameGraph);
            void CompileResourcesDeviceToHost(const RHI::FrameGraphCompileContext& context);
            void BuildCommandListInternalDeviceToHost(const RHI::FrameGraphExecuteContext& context);
            void SetupFrameGraphDependenciesHostToDevice(RHI::FrameGraphInterface frameGraph);
            void CompileResourcesHostToDevice(const RHI::FrameGraphCompileContext& context);
            void BuildCommandListInternalHostToDevice(const RHI::FrameGraphExecuteContext& context);

            // Retrieves the copy item type based on the input and output attachment type
            RHI::CopyItemType GetCopyItemType();

            // The copy item submitted to the command list
            RHI::CopyItem m_copyItemSameDevice;

            AZStd::shared_ptr<AZ::RHI::ScopeProducer> m_copyScopeProducerSameDevice;
            AZStd::shared_ptr<AZ::RHI::ScopeProducer> m_copyScopeProducerDeviceToHost;
            AZStd::shared_ptr<AZ::RHI::ScopeProducer> m_copyScopeProducerHostToDevice;

            // Potential data provided by the PassRequest
            CopyPassData m_data;

            RHI::HardwareQueueClass m_hardwareQueueClass = RHI::HardwareQueueClass::Graphics;

            enum class CopyMode
            {
                SameDevice,
                DifferentDevicesIntermediateHost,
                Invalid
            };
            CopyMode m_copyMode = CopyMode::Invalid;
            // Set to true for the MultiDeviceCopyPass, which uses one InputOutput slot instead of one Input and one Output slot.
            bool m_inputOutputCopy = false;

            constexpr static int MaxFrames = RHI::Limits::Device::FrameCountMax;

            struct PerAspectCopyInfo
            {
                RHI::CopyItem m_copyItemDeviceToHost{};
                RHI::CopyItem m_copyItemHostToDevice{};
                AZStd::array<Data::Instance<Buffer>, MaxFrames> m_device1HostBuffer{};
                AZStd::array<Data::Instance<Buffer>, MaxFrames> m_device2HostBuffer{};
                AZStd::array<AZ::u64, MaxFrames> m_deviceHostBufferByteCount{};
                RHI::DeviceImageSubresourceLayout m_inputImageLayout;
            };

            // Multiple aspects cannot be copied at the same time, so we need a copy items (and corresponding other members)
            // for each aspect. This is the case for example, when we want to copy a depth-stencil-image.
            AZStd::vector<PerAspectCopyInfo> m_perAspectCopyInfos;

            int m_currentBufferIndex = 0;
            AZStd::array<Ptr<RHI::Fence>, MaxFrames> m_device1SignalFence;
            AZStd::array<Ptr<RHI::Fence>, MaxFrames> m_device2WaitFence;
        };
    } // namespace RPI
} // namespace AZ
