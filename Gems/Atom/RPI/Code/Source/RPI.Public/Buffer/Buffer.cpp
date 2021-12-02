/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Buffer/Buffer.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/Fence.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RPI.Public/Buffer/BufferPool.h>
#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <AtomCore/Instance/InstanceDatabase.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Debug/EventTrace.h>

namespace AZ
{
    namespace RPI
    {
        Data::Instance<Buffer> Buffer::FindOrCreate(const Data::Asset<BufferAsset>& bufferAsset)
        {
            auto buffer = Data::InstanceDatabase<Buffer>::Instance().FindOrCreate(
                Data::InstanceId::CreateFromAssetId(bufferAsset.GetId()),
                bufferAsset);
            return buffer;
        }
        
        Buffer::Buffer()
        {
            /**
             * Buffer views are persistently initialized on their parent buffer, and
             * shader resource groups hold buffer view references. If we re-create the buffer
             * view instance entirely, that will not automatically propagate to dependent
             * shader resource groups.
             *
             * buffer views remain valid when their host buffer shuts down and re-initializes
             * (it will force a rebuild), so the best course of action is to keep a persistent
             * pointer around at all times, and then only initialize the buffer view once.
             */

            auto& factory = RHI::Factory::Get();
            m_rhiBuffer = factory.CreateBuffer();
            AZ_Assert(m_rhiBuffer, "Failed to acquire an buffer instance from the RHI. Is the RHI initialized?");
        }

        Buffer::~Buffer()
        {
            WaitForUpload();
        }

        RHI::Buffer* Buffer::GetRHIBuffer()
        {
            return m_rhiBuffer.get();
        }

        const RHI::Buffer* Buffer::GetRHIBuffer() const
        {
            return m_rhiBuffer.get();
        }

        const RHI::BufferView* Buffer::GetBufferView() const
        {
            if (m_rhiBuffer->GetDescriptor().m_bindFlags == RHI::BufferBindFlags::InputAssembly ||
                m_rhiBuffer->GetDescriptor().m_bindFlags == RHI::BufferBindFlags::DynamicInputAssembly)
            {
                
                AZ_Assert(false, "Input assembly buffer doesn't need a regular buffer view, it requires a stream or index buffer view.");
                return nullptr;
            }
            return m_bufferView.get();
        }

        Data::Instance<Buffer> Buffer::CreateInternal(BufferAsset& bufferAsset)
        {
            Data::Instance<Buffer> buffer = aznew Buffer();
            const RHI::ResultCode resultCode = buffer->Init(bufferAsset);
            if (resultCode == RHI::ResultCode::Success)
            {
                return buffer;
            }

            return nullptr;
        }

        RHI::ResultCode Buffer::Init(BufferAsset& bufferAsset)
        {
            AZ_TRACE_METHOD();

            RHI::ResultCode resultCode = RHI::ResultCode::Fail;

            m_rhiBufferPool = nullptr;
            if (bufferAsset.GetPoolAsset().GetId().IsValid())
            {
                Data::Instance<BufferPool> pool = BufferPool::FindOrCreate(bufferAsset.GetPoolAsset());
                if (pool)
                {
                    // Keep the reference so it won't be released
                    m_bufferPool = pool;
                    m_rhiBufferPool = pool->GetRHIPool();
                }
                else
                {
                    AZ_Error("RPI::Buffer", false, "Failed to acquire the buffer pool instance from asset.");
                    return resultCode;
                }
            }
            else if (bufferAsset.GetCommonPoolType() != CommonBufferPoolType::Invalid)
            {
                m_rhiBufferPool = BufferSystemInterface::Get()->GetCommonBufferPool(bufferAsset.GetCommonPoolType()).get();
            }

            if (!m_rhiBufferPool)
            {
                AZ_Error("RPI::Buffer", false, "Failed to acquire the buffer pool.");
                return resultCode;
            }

            m_bufferViewDescriptor = bufferAsset.GetBufferViewDescriptor();

            const uint32_t MinStreamSize = 64*1024; // Using streaming if the buffer data size is large than this size. Otherwise use init request to upload the data. 

            bool initWithData = (bufferAsset.GetBuffer().size() > 0 && bufferAsset.GetBuffer().size() <= MinStreamSize);

            RHI::BufferInitRequest request;
            request.m_buffer = m_rhiBuffer.get();
            request.m_descriptor = bufferAsset.GetBufferDescriptor();
            request.m_initialData = initWithData ? bufferAsset.GetBuffer().data() : nullptr;
            resultCode = m_rhiBufferPool->InitBuffer(request);

            if (resultCode == RHI::ResultCode::Success)
            {
                m_bufferAsset = { &bufferAsset, AZ::Data::AssetLoadBehavior::PreLoad };
                InitBufferView();
                
                if (bufferAsset.GetBuffer().size() > 0 && !initWithData)
                {
                    AZ_TRACE_METHOD_NAME("Stream Upload");
                    m_streamFence = RHI::Factory::Get().CreateFence();
                    if (m_streamFence)
                    {
                        m_streamFence->Init(m_rhiBufferPool->GetDevice(), RHI::FenceState::Reset);
                    }

                    RHI::BufferDescriptor bufferDescriptor = bufferAsset.GetBufferDescriptor();

                    RHI::BufferStreamRequest request2;
                    request2.m_buffer = m_rhiBuffer.get();
                    request2.m_fenceToSignal = m_streamFence.get();
                    request2.m_byteCount = bufferDescriptor.m_byteCount;
                    request2.m_sourceData = bufferAsset.GetBuffer().data();

                    resultCode = m_rhiBufferPool->StreamBuffer(request2);
                    if (resultCode != RHI::ResultCode::Success)
                    {
                        AZ_Error("Buffer", false, "Buffer::Init() failed to stream buffer contents to GPU.");
                        return resultCode;
                    }
                }
                
                m_rhiBuffer->SetName(Name(bufferAsset.GetName()));

                // Only generate buffer's attachment id if the buffer is writable 
                if (RHI::CheckBitsAny(m_rhiBuffer->GetDescriptor().m_bindFlags,
                        RHI::BufferBindFlags::ShaderWrite | RHI::BufferBindFlags::CopyWrite | RHI::BufferBindFlags::DynamicInputAssembly))
                {
                    // attachment id = bufferName_bufferInstanceId
                    m_attachmentId = Name(bufferAsset.GetName() + "_" + bufferAsset.GetId().m_guid.ToString<AZStd::string>(false, false));
                }

                return RHI::ResultCode::Success;
            }

            AZ_Error("Buffer", false, "Buffer::Init() failed to initialize RHI buffer. Error code: %d", static_cast<uint32_t>(resultCode));
            return resultCode;
        }
        
        void Buffer::Resize(uint64_t bufferSize)
        {
            RHI::BufferDescriptor desc = m_rhiBuffer->GetDescriptor();            
            m_rhiBuffer = RHI::Factory::Get().CreateBuffer();
            AZ_Assert(m_rhiBuffer, "Failed to acquire an buffer instance from the RHI. Is the RHI initialized?");
            
            desc.m_byteCount = bufferSize;
            RHI::BufferInitRequest request;
            request.m_buffer = m_rhiBuffer.get();
            request.m_descriptor = desc;

            RHI::ResultCode resultCode = m_rhiBufferPool->InitBuffer(request);
            if (resultCode != RHI::ResultCode::Success)
            {
                AZ_Error("Buffer", false, "Buffer::Resize() failed to resize buffer. Error code: %d", static_cast<uint32_t>(resultCode));
                return;
            }

            //update buffer view 
            m_bufferViewDescriptor.m_elementCount = aznumeric_cast<uint32_t>(bufferSize / m_bufferViewDescriptor.m_elementSize);
            InitBufferView();
        }
        
        void Buffer::InitBufferView()
        {
            // Skip buffer view creation for input assembly buffers
            if (m_rhiBuffer->GetDescriptor().m_bindFlags == RHI::BufferBindFlags::InputAssembly ||
                m_rhiBuffer->GetDescriptor().m_bindFlags == RHI::BufferBindFlags::DynamicInputAssembly)
            {
                return;
            }
               
            m_bufferView = m_rhiBuffer->GetBufferView(m_bufferViewDescriptor);

            if(!m_bufferView.get())
            {
                AZ_Assert(false, "Buffer::InitBufferView() failed to initialize RHI buffer view.");                
            }
        }

        void* Buffer::Map(size_t byteCount, uint64_t byteOffset)
        {
            if (byteOffset + byteCount > m_rhiBuffer->GetDescriptor().m_byteCount)
            {
                AZ_Error("Buffer", false, "Map out of range");
                return nullptr;
            }

            RHI::BufferMapRequest request;
            request.m_buffer = m_rhiBuffer.get();
            request.m_byteCount = byteCount;
            request.m_byteOffset = byteOffset;

            RHI::BufferMapResponse response;
            RHI::ResultCode result = m_rhiBufferPool->MapBuffer(request, response);

            if (result == RHI::ResultCode::Success)
            {
                return response.m_data;
            }
            else
            {
                AZ_Error("RPI::Buffer", false, "Failed to update RHI buffer. Error code: %d", result);
                return nullptr;
            }
        }

        void Buffer::Unmap()
        {
            m_rhiBufferPool->UnmapBuffer(*m_rhiBuffer);
        }

        void Buffer::WaitForUpload()
        {
            if (m_streamFence)
            {
                m_streamFence->WaitOnCpu();

                // Guard against calling release twice on the same pointer from different threads, 
                // which would decrement the ref count twice and result in a crash
                m_pendingUploadMutex.lock();
                // Release buffer asset reference now that the upload is complete.
                m_bufferAsset.Reset();
                m_pendingUploadMutex.unlock();
            }
        }

        bool Buffer::Orphan()
        {
            if (m_rhiBufferPool->GetDescriptor().m_heapMemoryLevel != RHI::HeapMemoryLevel::Host)
            {
                return false;
            }
            else
            {
                return m_rhiBufferPool->OrphanBuffer(*m_rhiBuffer) == RHI::ResultCode::Success;
            }
        }

        bool Buffer::OrphanAndUpdateData(const void* sourceData, uint64_t sourceDataSize)
        {
            if (sourceDataSize > m_rhiBuffer->GetDescriptor().m_byteCount || !Orphan())
            {
                return false;
            }

            return UpdateData(sourceData, sourceDataSize, 0);
        }
        
        bool Buffer::UpdateData(const void* sourceData, uint64_t sourceDataSize, uint64_t bufferByteOffset)
        {
            if (sourceDataSize == 0)
            {
                return true;
            }

            if (void* buf = Map(sourceDataSize, bufferByteOffset))
            {
                memcpy(buf, sourceData, sourceDataSize);
                Unmap();
                return true;
            }

            return false;
        }

        const RHI::AttachmentId& Buffer::GetAttachmentId() const
        {
            AZ_Assert(!m_attachmentId.GetStringView().empty(), "Read-only buffer doesn't need attachment id");
            return m_attachmentId;
        }
        
        const RHI::BufferViewDescriptor& Buffer::GetBufferViewDescriptor() const
        {
            return m_bufferViewDescriptor;
        }

        uint64_t Buffer::GetBufferSize() const
        {
            return m_rhiBuffer->GetDescriptor().m_byteCount;
        }

    } // namespace RPI
} // namespace AZ
