/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/Base.h>
#include <RHI/CommandQueue.h>

namespace AZ
{
    namespace Metal
    {
        CommandQueueCommandBuffer::~CommandQueueCommandBuffer()
        {
            if (m_mtlParallelEncoder)
            {
                [m_mtlParallelEncoder release];
            }
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
                    [mtlCommandBufferDesc release];
                    mtlCommandBufferDesc = nil;
                }
            }
#endif
            
            if(m_mtlCommandBuffer == nil)
            {
                m_mtlCommandBuffer = [m_hwQueue commandBuffer];
            }
            
            // Add a addCompletedHandler which can be used for outputting useful information in case of an error
            [m_mtlCommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer)
            {
                if(RHI::BuildOptions::IsDebugBuild || RHI::BuildOptions::IsProfileBuild)
                {
                    // check command buffer's status for errors, print out all of its contents
                    m_statusAfterExecution = buffer.status;
                    if (m_statusAfterExecution == MTLCommandBufferStatusError)
                    {
                        const char * cbLabel = [ buffer.label UTF8String ];
                        AZ_Printf("RHI", "Command Buffer %s failed to execute\n", cbLabel);
                        
                        int eCode = static_cast<int>(buffer.error.code);
                        switch (eCode)
                        {
                        case MTLCommandBufferErrorNone:
                            break;
                        case MTLCommandBufferErrorInternal:
                            AZ_Printf("RHI","Internal error has occurred");
                            break;
                        case MTLCommandBufferErrorTimeout:
                            AZ_Printf("RHI","Execution of this command buffer took more time than system allows. execution interrupted and aborted.\n");
                            break;
                        case MTLCommandBufferErrorPageFault:
                            AZ_Printf("RHI","Execution of this command generated an unserviceable GPU page fault. This error maybe caused by buffer read/write attribute mismatch or out of boundary access.\n");
                            break;
                        case MTLCommandBufferErrorAccessRevoked:
                            AZ_Printf("RHI","Access to this device has been revoked because this client has been responsible for too many timeouts or hangs.\n");
                            break;
                        case MTLCommandBufferErrorNotPermitted:
                            AZ_Printf("RHI","This process does not have access to use device.\n");
                            break;
                        case MTLCommandBufferErrorOutOfMemory:
                            AZ_Printf("RHI","Insufficient memory.\n");
                            break;
                        case MTLCommandBufferErrorInvalidResource:
                            AZ_Printf("RHI","This error is most commonly caused when the caller deletes a resource before executing a command buffer that refers to it. It would also trigger if the caller deletes the resource while the GPU is working on the command buffer.\n");
                            break;
                        default:
                            break;
                        }
                 
                        NSLog(@"%@",buffer.error);
#if !defined (AZ_FORCE_CPU_GPU_INSYNC)
                        // When in cpu/gpu lockstep mode (i.e AZ_FORCE_CPU_GPU_INSYNC) we break in the main thread
                        // with proper logging and a dialog box with info related to the last executing scope before the crash
                        AZ_Assert(false, "Assert here as the app is about to abort");
                        abort();
#endif
                    }
                }
            }];
            
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
                // We need the parallel encoder to survive until all FrameExecuteGroups have finished.
                [m_mtlParallelEncoder retain];
            }
            
            //Each context will get a sub render encoder.
            id <MTLRenderCommandEncoder> renderCommandEncoder = [m_mtlParallelEncoder renderCommandEncoder];
            if (RHI::Validation::IsEnabled())
            {
                renderCommandEncoder.label = [NSString stringWithCString:scopeName encoding:NSUTF8StringEncoding];
            }
            AZ_Assert(renderCommandEncoder != nil, "Could not create the RenderCommandEncoder");
            [renderCommandEncoder retain];
            return renderCommandEncoder;
        }

        void CommandQueueCommandBuffer::FlushParallelEncoder()
        {
            if (m_mtlParallelEncoder)
            {
                [m_mtlParallelEncoder endEncoding];
                [m_mtlParallelEncoder release];
                m_mtlParallelEncoder = nil;
            }
        }
         
        void CommandQueueCommandBuffer::CommitMetalCommandBuffer(bool isCommitNeeded)
        {
            if(isCommitNeeded)
            {
                [m_mtlCommandBuffer commit];
#if defined (AZ_FORCE_CPU_GPU_INSYNC)
                // Wait for the gpu to finish executing the work related to the command buffer
                [m_mtlCommandBuffer waitUntilCompleted];
#endif
            }
            
            //Release to match the retain at creation.
            [m_mtlCommandBuffer release];
            m_mtlCommandBuffer = nil;
        }
    
        const id<MTLCommandBuffer> CommandQueueCommandBuffer::GetMtlCommandBuffer() const
        {
            return m_mtlCommandBuffer;
        }
    
        MTLCommandBufferStatus CommandQueueCommandBuffer::GetCommandBufferStatus() const
        {
            return m_statusAfterExecution;
        }
    }
}
