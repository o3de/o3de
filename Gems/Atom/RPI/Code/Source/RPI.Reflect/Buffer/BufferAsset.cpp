/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Buffer/BufferAsset.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/Json/ByteStreamSerializer.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(RPI::CommonBufferPoolType, "{E3FD19DF-4395-46FD-8092-D27BC73A3688}");

    namespace RPI
    {
        AZ_CLASS_ALLOCATOR_IMPL(BufferAsset, BufferAssetAllocator)

        void BufferAsset::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<BufferAsset>()
                    ->Version(3)
                    ->Field("Name", &BufferAsset::m_name)
                    ->Field("Buffer", &BufferAsset::m_buffer)
                    ->Field("BufferDescriptor", &BufferAsset::m_bufferDescriptor)
                    ->Field("BufferViewDescriptor", &BufferAsset::m_bufferViewDescriptor)
                    ->Field("BufferPoolAsset", &BufferAsset::m_poolAsset)
                    ->Field("CommonBufferPoolType", &BufferAsset::m_poolType)
                    ;

                // register enum strings
                serializeContext->Enum<CommonBufferPoolType>()
                    ->Value("Constant", CommonBufferPoolType::Constant)
                    ->Value("StaticInputAssembly", CommonBufferPoolType::StaticInputAssembly)
                    ->Value("DynamicInputAssembly", CommonBufferPoolType::DynamicInputAssembly)
                    ->Value("ReadBack", CommonBufferPoolType::ReadBack)
                    ->Value("ReadWrite", CommonBufferPoolType::ReadWrite)
                    ->Value("ReadOnly", CommonBufferPoolType::ReadOnly)
                    ->Value("Indirect", CommonBufferPoolType::Indirect)
                    ->Value("Invalid", CommonBufferPoolType::Invalid)
                    ;
            }
            if (JsonRegistrationContext* jsonContext = azrtti_cast<JsonRegistrationContext*>(context))
            {
                jsonContext->Serializer<JsonByteStreamSerializer<Allocator>>()->HandlesType<AZStd::vector<uint8_t, Allocator>>();
            }
        }

        AZStd::span<const uint8_t> BufferAsset::GetBuffer() const
        {
            return AZStd::span<const uint8_t>(m_buffer);
        }

        const RHI::BufferDescriptor& BufferAsset::GetBufferDescriptor() const
        {
            return m_bufferDescriptor;
        }

        const RHI::BufferViewDescriptor& BufferAsset::GetBufferViewDescriptor() const
        {
            return m_bufferViewDescriptor;
        }

        void BufferAsset::SetReady()
        {
            m_status = AssetStatus::Ready;
        }

        const Data::Asset<ResourcePoolAsset>& BufferAsset::GetPoolAsset() const
        {
            return m_poolAsset;
        }

        CommonBufferPoolType BufferAsset::GetCommonPoolType() const
        {
            return m_poolType;
        }

        const AZStd::string& BufferAsset::GetName() const
        {
            return m_name;
        }
    } //namespace RPI
} // namespace AZ
