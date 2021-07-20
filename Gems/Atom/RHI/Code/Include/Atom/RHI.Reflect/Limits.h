/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>

namespace AZ
{
    namespace RHI
    {
        namespace Limits
        {
            namespace Image
            {
                constexpr uint32_t MipCountMax = 15;
                constexpr uint32_t ArraySizeMax = 2048;
                constexpr uint32_t SizeMax = 16384;
                constexpr uint32_t SizeVolumeMax = 2048;
            }

            namespace Pipeline
            {
                constexpr uint32_t AttachmentColorCountMax = 8;
                constexpr uint32_t ShaderResourceGroupCountMax = 8;
                constexpr uint32_t StreamCountMax = 12;
                constexpr uint32_t StreamChannelCountMax = 16;
                constexpr uint32_t DrawListTagCountMax = 64;
                constexpr uint32_t DrawFilterTagCountMax = 32;
                constexpr uint32_t MultiSampleCustomLocationsCountMax = 16;
                constexpr uint32_t MultiSampleCustomLocationGridSize = 16;
                constexpr uint32_t SubpassCountMax = 10;
                constexpr uint32_t RenderAttachmentCountMax = 2 * AttachmentColorCountMax + 1; // RenderAttachments + ResolveAttachments + DepthStencilAttachment
            }

            namespace Device
            {
                /**
                 * Maximum number of GPU frames that can be buffered before the CPU will
                 * stall. This includes the current frame being built by the CPU. For example,
                 * a value of 1 means only single frame is allowed to be build and dispatched at
                 * a time. In this example, the CPU timeline would serialize with the GPU timeline
                 * because only a single copy of CPU state is available.
                 *
                 * In a more realistic scenario, a value of 3 would allow the CPU to build the current
                 * frame, while the GPU could have up to two frames queued up before the CPU would wait.
                 */
                constexpr uint32_t FrameCountMax = 3;
            }

            namespace APIType
            {
                /**
                 * RHI::Factory has a virtual method called GetAPIUniqueIndex(), see its documentation
                 * for details. GetAPIUniqueIndex() should not return a value greater than this.
                 */
                constexpr uint32_t PerPlatformApiUniqueIndexMax = 3;
            }
        } // namespace Limits

        namespace DefaultValues
        {
            namespace Memory
            {
                constexpr uint64_t StagingBufferBudgetInBytes          = 128ul * 1024 * 1024;

                constexpr uint64_t AsyncQueueStagingBufferSizeInBytes  = 4ul   * 1024 * 1024;
                constexpr uint64_t MediumStagingBufferPageSizeInBytes  = 2ul   * 1024 * 1024;
                constexpr uint64_t LargestStagingBufferPageSizeInBytes = 128ul * 1024 * 1024;
                constexpr uint64_t ImagePoolPageSizeInBytes            = 2ul   * 1024 * 1024;
                constexpr uint64_t BufferPoolPageSizeInBytes           = 16ul  * 1024 * 1024;
            }
        } // namespace DefaultValues

        namespace Alignment
        {
            constexpr uint32_t InputAssembly = 4;
            constexpr uint32_t Constant = 256;
            constexpr uint32_t Buffer = 16;
        }
    }
}
