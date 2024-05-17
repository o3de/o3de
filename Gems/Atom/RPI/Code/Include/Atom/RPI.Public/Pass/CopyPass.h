/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/SingleDeviceCopyItem.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI.Reflect/Viewport.h>

#include <Atom/RPI.Reflect/Pass/CopyPassData.h>

#include <Atom/RPI.Public/Pass/RenderPass.h>

namespace AZ
{
    namespace RPI
    {
        //! A copy pass is a leaf pass (pass with no children) used for copying images and buffers on the GPU.
        class CopyPass
            : public RenderPass
        {
            AZ_RPI_PASS(CopyPass);

        public:
            AZ_RTTI(CopyPass, "{7387500D-B1BA-4916-B38C-24F5C8DAF839}", RenderPass);
            AZ_CLASS_ALLOCATOR(CopyPass, SystemAllocator);
            virtual ~CopyPass() = default;

            static Ptr<CopyPass> Create(const PassDescriptor& descriptor);

        protected:
            explicit CopyPass(const PassDescriptor& descriptor);

            // Sets up the copy item to perform an image to image copy
            void CopyBuffer(const RHI::FrameGraphCompileContext& context);
            void CopyImage(const RHI::FrameGraphCompileContext& context);
            void CopyBufferToImage(const RHI::FrameGraphCompileContext& context);
            void CopyImageToBuffer(const RHI::FrameGraphCompileContext& context);

            // Pass behavior overrides
            void BuildInternal() override;

            // Scope producer functions...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // Retrieves the copy item type based on the input and output attachment type
            RHI::CopyItemType GetCopyItemType();

            // The copy item submitted to the command list
            RHI::SingleDeviceCopyItem m_copyItem;

            // Potential data provided by the PassRequest
            CopyPassData m_data;
        };
    }   // namespace RPI
}   // namespace AZ
