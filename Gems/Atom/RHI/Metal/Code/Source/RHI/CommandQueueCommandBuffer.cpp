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
                        [[maybe_unused]] const char * cbLabel = [ buffer.label UTF8String ];
#if defined(CARBONATED)
                        AZ_Error("RHI", false, "Command Buffer %s failed to execute", cbLabel);
                        // this saves extended error information to the log file, NSLog below prints about the same
                        NSArray<id<MTLCommandBufferEncoderInfo>>* infos = buffer.error.userInfo[MTLCommandBufferEncoderInfoErrorKey];
                        for (id<MTLCommandBufferEncoderInfo> info in infos)
                        {
                            const char* infoLabel = [info.label UTF8String];
                            const char* infoState = "none";
                            switch (info.errorState)
                            {
                            case MTLCommandEncoderErrorStateUnknown:
                                infoState = "unknown";
                                break;
                            case MTLCommandEncoderErrorStateCompleted:
                                infoState = "completed";
                                break;
                            case MTLCommandEncoderErrorStateAffected:
                                infoState = "affected";
                                break;
                            case MTLCommandEncoderErrorStateFaulted:
                                infoState = "failed";
                                break;
                            }
                            AZ_Printf("RHI", "Command set %s, state %s", infoLabel, infoState);
             
                            for (NSString *sinopsis in info.debugSignposts)
                            {
                                const char* infoSinopsis = [sinopsis UTF8String];
                                AZ_Printf("RHI", "Debug sinopsis %s", infoSinopsis);
                            }
                        }
#else
                        AZ_Printf("RHI", "Command Buffer %s failed to execute\n", cbLabel);
#endif
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
#if defined(CARBONATED)
                        // we want the log messages above delivered to the log file, so we need main thread to process them
                        // sleep time allows the main thread to deliver the temporary stored in memory log messages to the log file
                        [NSThread sleepForTimeInterval: 0.2];
#endif
                        abort();
#endif
                    }
#if defined(CARBONATED)
                    else
                    {
                        CFTimeInterval begin = buffer.GPUStartTime;
                        CFTimeInterval end = buffer.GPUEndTime;
                        RHI::Device* pDevice = RHI::RHISystemInterface::Get()->GetDevice();
                        pDevice->CommandBufferCompleted(static_cast<const void*>(buffer), begin, end);
                        /*
                        {
                            const double t = double(clock_gettime_nsec_np(CLOCK_UPTIME_RAW)) / 1000000000.0;
                            AZ_Info("GPUtime", "buffer done at %f, start %f, stop %f, dt %f, current device frame %u, buffer %p",
                                            t, begin, end, end - begin, pDevice->GetFrameCounter(), static_cast<const void*>(buffer));
                        }
                        */
                    }
#endif
                }
            }];
            
            //we call retain here as this CB is active across the autoreleasepools of multiple threads. Calling
            //retain here means that if the current thread's autoreleasepool gets drained this CB will not die.
            //Only when this CB is committed and release is called on it this will it get reclaimed.
            [m_mtlCommandBuffer retain];
            AZ_Assert(m_mtlCommandBuffer != nil, "Could not create the command buffer");
            
#if defined(CARBONATED)
            RHI::RHISystemInterface::Get()->GetDevice()->RegisterCommandBuffer(static_cast<void*>(m_mtlCommandBuffer));
#endif
            
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
            if (RHI::Validation::IsEnabled())
            {
                renderCommandEncoder.label = [NSString stringWithCString:scopeName encoding:NSUTF8StringEncoding];
            }
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
         
        void CommandQueueCommandBuffer::CommitMetalCommandBuffer(bool isCommitNeeded)
        {
            if(isCommitNeeded)
            {
#if defined(CARBONATED)
                RHI::Device* pDevice = RHI::RHISystemInterface::Get()->GetDevice();
                pDevice->MarkCommandBufferCommit(static_cast<const void*>(m_mtlCommandBuffer));
#endif
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
