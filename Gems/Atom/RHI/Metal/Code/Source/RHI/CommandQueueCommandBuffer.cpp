/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/Base.h>
#include <AzCore/Debug/EventTrace.h>
#include <RHI/CommandQueue.h>

namespace AZ
{
    namespace Metal
    {
        CommandQueueCommandBuffer::~CommandQueueCommandBuffer()
        {
        }
    
        void CommandQueueCommandBuffer::Init(id<MTLCommandQueue> hwQueue)
        {
            m_hwQueue = hwQueue;
        }

        id <MTLCommandBuffer> CommandQueueCommandBuffer::AcquireMTLCommandBuffer()
        {
            AZ_Assert(m_mtlCommandBuffer==nil, "Previous command buffer was not commited");
            
            //Create a new command buffer
#if defined(__IPHONE_14_0) || defined(__MAC_11_0)
            if(@available(iOS 14.0, macOS 11.0, *))
            {
                if(RHI::BuildOptions::IsDebugBuild)
                {
                    //There is a perf cost associated with enhanced command buffer errors so only enabling them for debug builds.
                    MTLCommandBufferDescriptor* mtlCommandBufferDesc = [[MTLCommandBufferDescriptor alloc] init];
                    mtlCommandBufferDesc.errorOptions = MTLCommandBufferErrorOptionEncoderExecutionStatus;
                    m_mtlCommandBuffer = [m_hwQueue commandBufferWithDescriptor:mtlCommandBufferDesc];

                    [m_mtlCommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer)
                     {
                        // check command buffer's status for errors, print out all of its contents
                        MTLCommandBufferStatus stat = buffer.status;
                        if (stat == MTLCommandBufferStatusError)
                        {
                            NSLog(@"%@",buffer.error);
                            abort();
                        }
                    }];
                }
            }
#endif
            if(m_mtlCommandBuffer == nil)
            {
                m_mtlCommandBuffer = [m_hwQueue commandBuffer];
            }
            
            //we call retain here as this CB is active across the autoreleasepools of multiple threads. Calling
            //retain here means that if the current thread's autoreleasepool gets drained this CB will not die.
            //Only when this CB is committed and release is called on it this will it get reclaimed.
            [m_mtlCommandBuffer retain];
            AZ_Assert(m_mtlCommandBuffer != nil, "Could not create the command buffer");
            return m_mtlCommandBuffer;
        }
    
        id <MTLCommandEncoder> CommandQueueCommandBuffer::AcquireSubRenderEncoder(MTLRenderPassDescriptor* renderPassDescriptor, const char * scopeName)
        {
            if(!m_mtlParallelEncoder)
            {
                //Create the parallel encoder which will be used to create all the sub render encoders.
                m_mtlParallelEncoder = [m_mtlCommandBuffer parallelRenderCommandEncoderWithDescriptor:renderPassDescriptor];
                AZ_Assert(m_mtlParallelEncoder != nil, "Could not create the ParallelRenderCommandEncoder");
            }
            
            //Each context will get a sub render encoder.
            id <MTLRenderCommandEncoder> renderCommandEncoder = [m_mtlParallelEncoder renderCommandEncoder];
            renderCommandEncoder.label = [NSString stringWithCString:scopeName encoding:NSUTF8StringEncoding];
            AZ_Assert(renderCommandEncoder != nil, "Could not create the RenderCommandEncoder");
            return renderCommandEncoder;
        }

        void CommandQueueCommandBuffer::FlushParallelEncoder()
        {
            if (m_mtlParallelEncoder)
            {
                [m_mtlParallelEncoder endEncoding];
                m_mtlParallelEncoder = nil;
            }
        }
         
        void CommandQueueCommandBuffer::CommitMetalCommandBuffer()
        {
            [m_mtlCommandBuffer commit];
            
            MTLCommandBufferStatus stat = m_mtlCommandBuffer.status;
            if (stat == MTLCommandBufferStatusError)
            {
                int eCode = m_mtlCommandBuffer.error.code;
                switch (eCode)
                {
                    case MTLCommandBufferErrorNone:
                        break;
                    case MTLCommandBufferErrorInternal:
                        AZ_Assert(false, "Internal error has occurred");
                        break;
                    case MTLCommandBufferErrorTimeout:
                        AZ_Assert(false,"Execution of this command buffer took more time than system allows. execution interrupted and aborted.");
                        break;
                    case MTLCommandBufferErrorPageFault:
                        AZ_Assert(false,"Execution of this command generated an unserviceable GPU page fault. This error maybe caused by buffer read/write attribute mismatch or outof boundary access");
                        break;
                    case MTLCommandBufferErrorBlacklisted:
                        AZ_Assert(false,"Access to this device has been revoked because this client has been responsible for too many timeouts or hangs");
                        break;
                    case MTLCommandBufferErrorNotPermitted:
                        AZ_Assert(false,"This process does not have aceess to use device");
                        break;
                    case MTLCommandBufferErrorOutOfMemory:
                        AZ_Assert(false,"Insufficient memory");
                        break;
                    case MTLCommandBufferErrorInvalidResource:
                        AZ_Assert(false,"This error is most commonly caused when the caller deletes a resource before executing a command buffer that refers to it. It would also trigger if the caller deletes the resource while the GPU is working on the command buffer");
                        break;
                    default:
                        break;
                }
            }
            
            //Release to match the retain at creation.
            [m_mtlCommandBuffer release];
            m_mtlCommandBuffer = nil;
        }
    
        const id<MTLCommandBuffer> CommandQueueCommandBuffer::GetMtlCommandBuffer() const
        {
            return m_mtlCommandBuffer;
        }
    }
}
