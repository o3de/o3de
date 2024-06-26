/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/Device.h>
#include <RHI/PipelineLibrary.h>
#include <AzCore/Name/Name.h>

namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<PipelineLibrary> PipelineLibrary::Create()
        {
            return aznew PipelineLibrary;
        }

        id<MTLBinaryArchive> PipelineLibrary::GetNativePipelineCache() const
        {
            return m_mtlBinaryArchive;
        }
    
        RHI::ResultCode PipelineLibrary::InitInternal(RHI::Device& deviceBase, const RHI::DevicePipelineLibraryDescriptor& descriptor)
        {
            DeviceObject::Init(deviceBase);
            auto& device = static_cast<Device&>(deviceBase);

            m_descriptor = descriptor;
            NSError* error = nil;
            MTLBinaryArchiveDescriptor* desc = [[MTLBinaryArchiveDescriptor alloc] init];
        
            NSString* psoCacheFilePath = [NSString stringWithCString:descriptor.m_filePath.c_str() encoding:NSUTF8StringEncoding];
            NSURL* filePathURL = [NSURL fileURLWithPath:psoCacheFilePath isDirectory:NO];
            
            //Pass in the file path if it exists
            if ([filePathURL checkResourceIsReachableAndReturnError:&error])
            {
                desc.url = filePathURL;
            }
            
            //Create a new Pso cache. Use the existing fOile on disk if provided
            m_mtlBinaryArchive = [device.GetMtlDevice() newBinaryArchiveWithDescriptor:desc error:&error];
            [desc release];
            desc = nil;
            
            SetName(GetName());
            NSString* labelName = [NSString stringWithCString:GetName().GetCStr() encoding:NSUTF8StringEncoding];
            m_mtlBinaryArchive.label = labelName;
            return RHI::ResultCode::Success;
        }

        void PipelineLibrary::ShutdownInternal()
        {
            [m_mtlBinaryArchive release];
            m_mtlBinaryArchive = nil;
            m_renderPipelineStates.clear();
            m_computePipelineStates.clear();
        }

        id<MTLRenderPipelineState> PipelineLibrary::CreateGraphicsPipelineState(uint64_t hash, MTLRenderPipelineDescriptor* pipelineStateDesc, NSError** error)
        {
            Device& device = static_cast<Device&>(GetDevice());
            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
            
            NSArray* binArchives = [NSArray arrayWithObjects:m_mtlBinaryArchive,nil];
            pipelineStateDesc.binaryArchives = binArchives;
            //Create a new PSO. The drivers will use the Pso cache if the PSO resides in it
            id<MTLRenderPipelineState> graphicsPipelineState =
                [device.GetMtlDevice() newRenderPipelineStateWithDescriptor:pipelineStateDesc
                                                                      error:error];
            if(graphicsPipelineState)
            {
                m_renderPipelineStates.emplace(hash, pipelineStateDesc);
            }
            else
            {
                if (RHI::Validation::IsEnabled())
                {
                    NSLog(@" error => %@ ", [*error userInfo] );
                }
                AZ_Assert(false, "Could not create Pipeline object!.");
            }
            return graphicsPipelineState;
        }
        
        id<MTLComputePipelineState> PipelineLibrary::CreateComputePipelineState(uint64_t hash, MTLComputePipelineDescriptor* pipelineStateDesc, NSError** error)
        {
            Device& device = static_cast<Device&>(GetDevice());
            MTLComputePipelineReflection* ref;
            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
            
            NSArray* binArchives = [NSArray arrayWithObjects:m_mtlBinaryArchive,nil];
            pipelineStateDesc.binaryArchives = binArchives;
            //Create a new PSO. The drivers will use the Pso cache if the PSO resides in it
            id<MTLComputePipelineState> computePipelineState =
                [device.GetMtlDevice() newComputePipelineStateWithDescriptor: pipelineStateDesc
                                                                     options: MTLPipelineOptionBufferTypeInfo
                                                                  reflection: &ref
                                                                       error: error];
            if(computePipelineState)
            {
                m_computePipelineStates.emplace(hash, pipelineStateDesc);
            }
            return computePipelineState;
        }
    
        RHI::ResultCode PipelineLibrary::MergeIntoInternal(AZStd::span<const RHI::DevicePipelineLibrary* const> pipelineLibraries)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
            NSError* error = nil;
            for (const RHI::DevicePipelineLibrary* libraryBase : pipelineLibraries)
            {
                const PipelineLibrary* library = static_cast<const PipelineLibrary*>(libraryBase);
                for (const auto& pipelineStateEntry : library->m_renderPipelineStates)
                {
                    if (m_renderPipelineStates.find(pipelineStateEntry.first) == m_renderPipelineStates.end())
                    {
                        [m_mtlBinaryArchive addRenderPipelineFunctionsWithDescriptor:pipelineStateEntry.second
                                                                               error:&error];
                        m_renderPipelineStates.emplace(pipelineStateEntry.first, pipelineStateEntry.second);
                    }
                }
                
                for (const auto& pipelineStateEntry : library->m_computePipelineStates)
                {
                    if (m_computePipelineStates.find(pipelineStateEntry.first) == m_computePipelineStates.end())
                    {
                        [m_mtlBinaryArchive addComputePipelineFunctionsWithDescriptor:pipelineStateEntry.second
                                                                               error:&error];
                        m_computePipelineStates.emplace(pipelineStateEntry.first, pipelineStateEntry.second);
                    }
                }
            }
            
            return RHI::ResultCode::Success;
        }

        RHI::ConstPtr<RHI::PipelineLibraryData> PipelineLibrary::GetSerializedDataInternal() const
        {
            return nullptr;
        }
    
        bool PipelineLibrary::SaveSerializedDataInternal(const AZStd::string& filePath) const
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
     
            NSError* error = nil;
            NSString*  psoCacheFilePath = [NSString stringWithCString:filePath.c_str() encoding:NSUTF8StringEncoding];
            NSURL *baseURL = [NSURL fileURLWithPath:psoCacheFilePath];
            
            BOOL isDir;
            NSFileManager *fileManager= [NSFileManager defaultManager];
            NSString *directory = [psoCacheFilePath stringByDeletingLastPathComponent];
            //If the directory where the PSO cache will reside does not exist create one
            if(![fileManager fileExistsAtPath:directory isDirectory:&isDir])
            {
                if(![fileManager createDirectoryAtPath:directory withIntermediateDirectories:YES attributes:nil error:NULL])
                {
                    AZ_Error("PipelineStateCache", false, "Error: Unable to create the folder %s in order to save the PSO Cache", psoCacheFilePath);
                    return false;
                }
            }
            
            if(m_mtlBinaryArchive)
            {
                [m_mtlBinaryArchive serializeToURL:baseURL
                                             error:&error];
            }
            return error==nil;
        }
    
        bool PipelineLibrary::IsMergeRequired() const
        {
            return !m_renderPipelineStates.empty() || !m_computePipelineStates.empty();
        }
    }
}
