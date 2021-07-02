/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Metal/Metal.h>

namespace AZ
{
    namespace Metal
    {        
        class CommandQueueCommandBuffer
        {
        public:
            CommandQueueCommandBuffer() = default;
            CommandQueueCommandBuffer(const CommandQueueCommandBuffer&) = delete;
            ~CommandQueueCommandBuffer();
            
            void Init(id<MTLCommandQueue> hwQueue);
            
            //! Commit the native metal command buffer to the command queue
            void CommitMetalCommandBuffer();
                        
            //! Creates a ParallelEncoder if one doesnt exist. Creates a new sub renderencoder from it and returns it
            id <MTLCommandEncoder> AcquireSubRenderEncoder(MTLRenderPassDescriptor* renderPassDescriptor, const char * scopeName);
            
            //! Flushes the parallelEncoder
            void FlushParallelEncoder();
            
            //! Grab command buffer from the queue
            id <MTLCommandBuffer> AcquireMTLCommandBuffer();
            
            const id<MTLCommandBuffer> GetMtlCommandBuffer() const;

        private:
            
            //! ParallelCommandEncoder needed to encode data across multiple threads
            id <MTLParallelRenderCommandEncoder> m_mtlParallelEncoder = nil;
            
            //! Native metal command buffer
            id <MTLCommandBuffer> m_mtlCommandBuffer = nil;
            
            //Metal queue that this commandbuffer is attached to
            id<MTLCommandQueue> m_hwQueue = nil;
        };
    }
}
