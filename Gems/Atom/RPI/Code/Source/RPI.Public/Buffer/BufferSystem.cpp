/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>

#include <Atom/RPI.Public/Buffer/BufferSystem.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Buffer/BufferPool.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetView.h>

#include <AtomCore/Instance/InstanceDatabase.h>
#include <AzCore/Interface/Interface.h>

namespace AZ
{
    namespace RPI
    {
        void BufferSystem::Reflect(ReflectContext* context)
        {
            BufferAsset::Reflect(context);
            BufferAssetView::Reflect(context);
        }

        void BufferSystem::GetAssetHandlers(AssetHandlerPtrList& assetHandlers)
        {
            assetHandlers.emplace_back(MakeAssetHandler<BufferAssetHandler>());
        }

        void BufferSystem::Init()
        {
            {
                Data::InstanceHandler<Buffer> handler;
                handler.m_createFunction = [](Data::AssetData* bufferAsset)
                {
                    return Buffer::CreateInternal(*(azrtti_cast<BufferAsset*>(bufferAsset)));
                };
                Data::InstanceDatabase<Buffer>::Create(azrtti_typeid<BufferAsset>(), handler);
            }

            {
                Data::InstanceHandler<BufferPool> handler;
                handler.m_createFunction = [](Data::AssetData* poolAsset)
                {
                    return BufferPool::CreateInternal(*(azrtti_cast<ResourcePoolAsset*>(poolAsset)));
                };
                Data::InstanceDatabase<BufferPool>::Create(azrtti_typeid<ResourcePoolAsset>(), handler);
            }
            Interface<BufferSystemInterface>::Register(this);

            m_initialized = true;
        }

        void BufferSystem::Shutdown()
        {
            if (!m_initialized)
            {
                return;
            }
            for (uint8_t index = 0; index < static_cast<uint8_t>(CommonBufferPoolType::Count); index++)
            {
                m_commonPools[index] = nullptr;
            }
            Interface<BufferSystemInterface>::Unregister(this);
            Data::InstanceDatabase<Buffer>::Destroy();
            Data::InstanceDatabase<BufferPool>::Destroy();
            m_initialized = false;
        }
        
        RHI::Ptr<RHI::BufferPool> BufferSystem::GetCommonBufferPool(CommonBufferPoolType poolType)
        {
            const uint8_t index = static_cast<uint8_t>(poolType);
            if (!m_commonPools[index])
            {
                CreateCommonBufferPool(poolType);
            }

            return m_commonPools[index];
        }

        bool BufferSystem::CreateCommonBufferPool(CommonBufferPoolType poolType)
        {
            if (!m_initialized)
            {
                return false;
            }

            RHI::Ptr<RHI::BufferPool> bufferPool = aznew RHI::BufferPool;

            RHI::BufferPoolDescriptor bufferPoolDesc;
            switch (poolType)
            {
            case CommonBufferPoolType::Constant:
                bufferPoolDesc.m_bindFlags = RHI::BufferBindFlags::Constant;
                bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
                bufferPoolDesc.m_hostMemoryAccess = RHI::HostMemoryAccess::Write;
                break;
            case CommonBufferPoolType::StaticInputAssembly:
                bufferPoolDesc.m_bindFlags = RHI::BufferBindFlags::InputAssembly | RHI::BufferBindFlags::ShaderRead;
                bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
                bufferPoolDesc.m_hostMemoryAccess = RHI::HostMemoryAccess::Write;
                break;
            case CommonBufferPoolType::DynamicInputAssembly:
                bufferPoolDesc.m_bindFlags = RHI::BufferBindFlags::DynamicInputAssembly | RHI::BufferBindFlags::ShaderRead;
                bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Host;
                bufferPoolDesc.m_hostMemoryAccess = RHI::HostMemoryAccess::Write;
                break;
            case CommonBufferPoolType::ReadBack:
                bufferPoolDesc.m_bindFlags = RHI::BufferBindFlags::CopyWrite;
                bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Host;
                bufferPoolDesc.m_hostMemoryAccess = RHI::HostMemoryAccess::Read;
                break;
            case CommonBufferPoolType::Staging:
                bufferPoolDesc.m_bindFlags = RHI::BufferBindFlags::CopyRead;
                bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Host;
                bufferPoolDesc.m_hostMemoryAccess = RHI::HostMemoryAccess::Write;
                break;
            case CommonBufferPoolType::ReadWrite:
                // Add CopyRead flag too since it's often we need to read back GPU attachment buffers.
                bufferPoolDesc.m_bindFlags =
//                  [To Do] - the following line (and possibly InputAssembly / DynamicInputAssembly) will need to
//                  be added to support future indirect buffer usage for GPU driven render pipeline
//                    RHI::BufferBindFlags::Indirect |  
                    RHI::BufferBindFlags::ShaderWrite | RHI::BufferBindFlags::ShaderRead | RHI::BufferBindFlags::CopyRead;
                bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
                bufferPoolDesc.m_hostMemoryAccess = RHI::HostMemoryAccess::Write;
                break;
            case CommonBufferPoolType::ReadOnly:
//                  [To Do] - the following line (and possibly InputAssembly / DynamicInputAssembly) will need to
//                  be added to support future indirect buffer usage for GPU driven render pipeline
                bufferPoolDesc.m_bindFlags = // RHI::BufferBindFlags::Indirect |
                    RHI::BufferBindFlags::ShaderRead;
                bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
                bufferPoolDesc.m_hostMemoryAccess = RHI::HostMemoryAccess::Write;
                break;
            case CommonBufferPoolType::Indirect:
                bufferPoolDesc.m_bindFlags = AZ::RHI::BufferBindFlags::ShaderReadWrite | AZ::RHI::BufferBindFlags::Indirect |
                    AZ::RHI::BufferBindFlags::CopyRead | AZ::RHI::BufferBindFlags::CopyWrite;
                bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
                bufferPoolDesc.m_hostMemoryAccess = RHI::HostMemoryAccess::Write;
                break;
            default:
                AZ_Error("BufferSystem", false, "Unknown common buffer pool type: %d", poolType);
                return false;
            }

            bufferPool->SetName(Name(AZStd::string::format("RPI::CommonBufferPool_%i", static_cast<uint32_t>(poolType))));
            RHI::ResultCode resultCode = bufferPool->Init(bufferPoolDesc);
            if (resultCode != RHI::ResultCode::Success)
            {
                AZ_Error("BufferSystem", false, "Failed to create buffer pool: %d", poolType);
                return false;
            }

            m_commonPools[static_cast<uint8_t>(poolType)] = bufferPool;
            return true;
        }

        Data::Instance<Buffer> BufferSystem::CreateBufferFromCommonPool(const CommonBufferDescriptor& descriptor)
        {            
            AZ::Uuid bufferId;
            if (descriptor.m_isUniqueName)
            {
                bufferId = Uuid::CreateName(descriptor.m_bufferName);
                // Report error if there is a buffer with same name.
                // Note: this shouldn't return the existing buffer because users are expecting a newly created buffer.
                if (Data::InstanceDatabase<Buffer>::Instance().Find(Data::InstanceId::CreateUuid(bufferId)))
                {
                    AZ_Error("BufferSystem", false, "Buffer with same name '%s' already exist", descriptor.m_bufferName.c_str());
                    return nullptr;
                }
            }
            else
            {
                bufferId = Uuid::CreateRandom();
            }

            RHI::Ptr<RHI::BufferPool> bufferPool = GetCommonBufferPool(descriptor.m_poolType);

            if (!bufferPool)
            {
                AZ_Error("BufferSystem", false, "Common buffer pool type %d doesn't exist", descriptor.m_poolType);
                return nullptr;
            }

            RHI::BufferDescriptor bufferDesc;
            bufferDesc.m_alignment = descriptor.m_elementSize;
            bufferDesc.m_bindFlags = bufferPool->GetDescriptor().m_bindFlags;
            bufferDesc.m_byteCount = descriptor.m_byteCount;

            Data::Asset<BufferAsset> bufferAsset;
            BufferAssetCreator creator;
            creator.Begin(bufferId);
            creator.SetBufferName(descriptor.m_bufferName);
            creator.SetBuffer(descriptor.m_bufferData, descriptor.m_bufferData? descriptor.m_byteCount : 0, bufferDesc);
            creator.SetUseCommonPool(descriptor.m_poolType);

            RHI::BufferViewDescriptor viewDescriptor;
            if (descriptor.m_elementFormat != RHI::Format::Unknown)
            {
                viewDescriptor = RHI::BufferViewDescriptor::CreateTyped(
                    0,
                    aznumeric_cast<uint32_t>(bufferDesc.m_byteCount / descriptor.m_elementSize),
                    descriptor.m_elementFormat);
            }
            else
            {
                viewDescriptor = RHI::BufferViewDescriptor::CreateStructured(
                    0, aznumeric_cast<uint32_t>(bufferDesc.m_byteCount / descriptor.m_elementSize), descriptor.m_elementSize);
            }
            creator.SetBufferViewDescriptor(viewDescriptor);

            if (creator.End(bufferAsset))
            {
                return Data::InstanceDatabase<Buffer>::Instance().FindOrCreate(Data::InstanceId::CreateUuid(bufferId), bufferAsset);
            }

            return nullptr;
        }

        Data::Instance<Buffer> BufferSystem::FindCommonBuffer(AZStd::string_view uniqueBufferName)
        {
            const AZ::Uuid bufferId = Uuid::CreateName(uniqueBufferName);
            return Data::InstanceDatabase<Buffer>::Instance().Find(Data::InstanceId::CreateUuid(bufferId));
        }
    } // namespace RPI
} // namespace AZ
