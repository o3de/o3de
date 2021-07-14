/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Atom/RHI.Reflect/ShaderSemantic.h>
#include <Atom/RHI/RHIUtils.h>
#include <Atom/RHI/Factory.h>

#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/ResourcePoolAssetCreator.h>

#include <Rendering/SharedBuffer.h>
#include <Rendering/HairCommon.h>
//#include <numeric>

#pragma optimize("",off)
namespace AZ
{
    namespace Render
    {
        /*
        * Setting the constructor as private will create compile error to remind me to set the buffer Init
        * In the FeatureProcessor and initialize properly
        * */
        SharedBuffer::SharedBuffer()
        {
            AZ_Warning("SharedBuffer", false, "Missing information to properly create SharedBuffer. Init is required");
        }

        SharedBuffer::SharedBuffer(AZStd::string bufferName, AZStd::vector<SrgBufferDescriptor>& buffersDescriptors)
        {
            m_bufferName = bufferName;
            Init(bufferName, buffersDescriptors);
        }

        SharedBuffer::~SharedBuffer()
        {
            m_bufferAsset = {};
        }

        //! Crucial method that will ensure that the alignment for the BufferViews is always kept.
        //! This is important when requesting a BufferView as the offset needs to be aligned according
        //!  to the element type of the buffer.
        void SharedBuffer::CalculateAlignment(AZStd::vector<SrgBufferDescriptor>& buffersDescriptors)
        {
            m_alignment = 1;
            for (uint8_t bufferIndex = 0; bufferIndex < buffersDescriptors.size() ; ++bufferIndex)
            {
                // Using the least common multiple enables resource views to be typed and ensures they can get
                // an offset in bytes that is a multiple of an element count
                m_alignment = std::lcm(m_alignment, buffersDescriptors[bufferIndex].m_elementSize);
            }
        }

        void SharedBuffer::InitAllocator()
        {
            RHI::FreeListAllocator::Descriptor allocatorDescriptor;
            allocatorDescriptor.m_alignmentInBytes = m_alignment;
            allocatorDescriptor.m_capacityInBytes = m_sizeInBytes;
            allocatorDescriptor.m_policy = RHI::FreeListAllocatorPolicy::BestFit;
            allocatorDescriptor.m_garbageCollectLatency = 0;
            m_freeListAllocator.Init(allocatorDescriptor);
        }

        void SharedBuffer::CreateBuffer()
        {
            SrgBufferDescriptor  descriptor = SrgBufferDescriptor(
                RPI::CommonBufferPoolType::ReadWrite, RHI::Format::Unknown,
                sizeof(float), m_sizeInBytes,
                Name{ "HairSharedDynamicBuffer" }, Name{ "m_skinnedHairSharedBuffer" }, 0, 0
            );
            m_buffer = Hair::UtilityClass::CreateBuffer("Hair Gem", descriptor, nullptr);
        }
        
        void SharedBuffer::CreateBufferAsset()
        {
            // Create the shared buffer pool
            {
                auto bufferPoolDesc = AZStd::make_unique<RHI::BufferPoolDescriptor>();
                // Output buffers are both written to during skinning and used as input assembly buffers
                bufferPoolDesc->m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::Indirect;
                bufferPoolDesc->m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
                bufferPoolDesc->m_hostMemoryAccess = RHI::HostMemoryAccess::Write;

                RPI::ResourcePoolAssetCreator creator;
                creator.Begin(Uuid::CreateRandom());
                creator.SetPoolDescriptor(AZStd::move(bufferPoolDesc));
                creator.SetPoolName("SharedBufferPool");
                creator.End(m_bufferPoolAsset);
            }

            // Create the shared buffer
            {
                RPI::BufferAssetCreator creator;
                Uuid uuid = Uuid::CreateRandom();
                creator.Begin(uuid);
                creator.SetBufferName(m_bufferName);
                creator.SetPoolAsset(m_bufferPoolAsset);

                // Adi: the following RHI::BufferBindFlags::InputAssembly might not be required if we don't
                // read/write CPU and GPU every frame (possibly only one time set, after that it might
                // be read/write in GPU only)
                RHI::BufferDescriptor bufferDescriptor;
                bufferDescriptor.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::Indirect;
                bufferDescriptor.m_byteCount = m_sizeInBytes;
                bufferDescriptor.m_alignment = m_alignment;
                creator.SetBuffer(nullptr, 0, bufferDescriptor);
                
                RHI::BufferViewDescriptor viewDescriptor;
                viewDescriptor.m_elementFormat = RHI::Format::Unknown;
                // [To Do] Adi: try set it as AZ::Vector4 - might solve alot on the shader side
                viewDescriptor.m_elementSize = sizeof(float);
                viewDescriptor.m_elementCount = aznumeric_cast<uint32_t>(m_sizeInBytes) / sizeof(float);
                viewDescriptor.m_elementOffset = 0;
                creator.SetBufferViewDescriptor(viewDescriptor);
                
                creator.End(m_bufferAsset);
            }
            /*
            ////////////////////////////
            RHI::Ptr<RHI::Device> device = RHI::GetRHIDevice();

            RHI::BufferPoolDescriptor bufferPoolDesc;
            bufferPoolDesc.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite;
            bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            bufferPoolDesc.m_hostMemoryAccess = RHI::HostMemoryAccess::Write;

            RHI::ResultCode result = m_bufferPool->Init(*device, bufferPoolDesc);
            AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialized compute buffer pool");

            m_buffer = RHI::Factory::Get().CreateBuffer();
            uint32_t bufferSize = m_sizeInBytes;

            RHI::BufferInitRequest request;
            request.m_buffer = m_buffer.get();
            request.m_descriptor = RHI::BufferDescriptor{ RHI::BufferBindFlags::ShaderReadWrite, bufferSize };
            result = m_bufferPool->InitBuffer(request);
            AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialized compute buffer");


            RHI::BufferViewDescriptor bufferViewDescriptor = RHI::BufferViewDescriptor::CreateStructured(
                0,
                aznumeric_cast<uint32_t>(m_sizeInBytes) / sizeof(float),
                sizeof(float)
            );
            m_bufferView = m_buffer->GetBufferView(bufferViewDescriptor);
            //////////////////////////////
            */
        }

        void SharedBuffer::Init(AZStd::string bufferName, AZStd::vector<SrgBufferDescriptor>& buffersDescriptors)
        {
            m_bufferName = bufferName;
            // m_sizeInBytes = 256u * (1024u * 1024u);
            //
            // Adi: [ToDo] replace this with max size request for allocation that can be given by the calling function
            // This has the following problems:
            //  1. The need to have this aggregated size in advance
            //  2. The size might grow dynamically between frames
            //  3. Due to having several stream buffers (position, tangent, structured), alignment padding
            //      size calculation must be added.
            // Requirement: the buffer already has an assert on allocation beyond the memory.  In the future it should
            // support greedy memory allocation when memory has reached its end.  This must not invalidate the buffer during
            // the current frame, hence allocation of second buffer, fence and a copy must take place. 

            CalculateAlignment(buffersDescriptors);

            InitAllocator();

//            CreateBufferAsset();
            CreateBuffer();

            SystemTickBus::Handler::BusConnect();
        }

        AZStd::intrusive_ptr<SharedBufferAllocation> SharedBuffer::Allocate(size_t byteCount)
        {
            RHI::VirtualAddress result;
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_allocatorMutex);
                result = m_freeListAllocator.Allocate(byteCount, m_alignment);
            }

            if (result.IsValid())
            {
                return aznew SharedBufferAllocation(result);
            }

            return nullptr;
        }

        void SharedBuffer::DeAllocate(RHI::VirtualAddress allocation)
        {
            if (allocation.IsValid())
            {
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_allocatorMutex);
                    m_freeListAllocator.DeAllocate(allocation);
                }

                m_memoryWasFreed = true;
                m_broadcastMemoryAvailableEvent = true;
            }
        }

        void SharedBuffer::DeAllocateNoSignal(RHI::VirtualAddress allocation)
        {
            if (allocation.IsValid())
            {
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_allocatorMutex);
                    m_freeListAllocator.DeAllocate(allocation);
                }
                m_memoryWasFreed = true;
            }
        }

        Data::Asset<RPI::BufferAsset> SharedBuffer::GetBufferAsset() const
        {
            return m_bufferAsset;
        }

        Data::Instance<RPI::Buffer> SharedBuffer::GetBuffer()
        {
            if (!m_buffer)
            {
                m_buffer = RPI::Buffer::FindOrCreate(m_bufferAsset);
            }
            return m_buffer;
        }
        /*

        Data::Instance<RHI::Buffer> SharedBuffer::GetBuffer()
        {
            AZ_Error("Hair Gem", m_buffer, "Shared Buffer was not created!");
            return m_buffer;
        }


        const RHI::BufferView* SharedBuffer::GetBufferView()
        {
            if (!m_buffer)
            {
                m_buffer = RPI::Buffer::FindOrCreate(m_bufferAsset);
            }

            if (m_buffer)
            {
                m_bufferView = m_buffer->GetBufferView();
            }

            AZ_Error("Hair Gem", m_bufferView, "Shared Buffer View and Buffer were not created!");
            return m_bufferView;
        }
        */

        //! Update buffer's content with sourceData at an offset of bufferByteOffset
        bool SharedBuffer::UpdateData(const void* sourceData, uint64_t sourceDataSizeInBytes, uint64_t bufferByteOffset) 
        {
            // [To Do] Adibugbug - fix this now that it's RHI

            AZStd::lock_guard<AZStd::mutex> lock(m_allocatorMutex);
            if (m_buffer.get())
            {
                return m_buffer->UpdateData(sourceData, sourceDataSizeInBytes, bufferByteOffset);
            }
            AZ_Assert(false, "SharedBuffer error in data allocation - the buffer doesn't exist yet");
            return false;
        }

        void SharedBuffer::OnSystemTick()
        {
            GarbageCollect();
        }

        void SharedBuffer::GarbageCollect()
        {
            if (m_memoryWasFreed)
            {
                m_memoryWasFreed = false;
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_allocatorMutex);
                    m_freeListAllocator.GarbageCollect();
                }
                if (m_broadcastMemoryAvailableEvent)
                {
                    SharedBufferNotificationBus::Broadcast(&SharedBufferNotificationBus::Events::OnSharedBufferMemoryAvailable);
                    m_broadcastMemoryAvailableEvent = false;
                }
            }
        }

        //! Utility function to create a resource view of different type than the shared buffer data.
        //! Since this class is sub-buffer container, this method should be used after creating
        //!  a new allocation to be used as a sub-buffer.
        //! Notice the alignment required according to the element size - this might need 
        RHI::BufferViewDescriptor SharedBuffer::CreateResourceViewWithDifferentFormat(
            uint32_t offsetInBytes, uint32_t elementCount, uint32_t elementSize,
            RHI::Format format, RHI::BufferBindFlags overrideBindFlags)
        {
            RHI::BufferViewDescriptor viewDescriptor;

            // In the following line I use the element size and not the size based of the
            // element format since in the more interesting case of structured buffer, the
            // size will result in an error.
            uint32_t elementOffset = offsetInBytes / elementSize;
            viewDescriptor.m_elementOffset = elementOffset;
            viewDescriptor.m_elementCount = elementCount;
            viewDescriptor.m_elementFormat = format;
            viewDescriptor.m_elementSize = elementSize;
            viewDescriptor.m_overrideBindFlags = overrideBindFlags;
            return viewDescriptor;
        }

    }// namespace Render
}// namespace AZ

#pragma optimize("",on)
