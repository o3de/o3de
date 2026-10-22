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
#include <meshoptimizer.h>

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(RPI::CommonBufferPoolType, "{E3FD19DF-4395-46FD-8092-D27BC73A3688}");
    AZ_TYPE_INFO_SPECIALIZE(RPI::BufferAsset::CompressionFormat, "{D6624140-8972-4356-9A88-56BA8DD24D0A}");

    namespace RPI
    {
        AZ_CLASS_ALLOCATOR_IMPL(BufferAsset, BufferAssetAllocator)

        void BufferAsset::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<BufferAsset>()
                    ->Version(4)
                    ->Field("Name", &BufferAsset::m_name)
                    ->Field("Buffer", &BufferAsset::m_buffer)
                    ->Field("BufferDescriptor", &BufferAsset::m_bufferDescriptor)
                    ->Field("BufferViewDescriptor", &BufferAsset::m_bufferViewDescriptor)
                    ->Field("BufferPoolAsset", &BufferAsset::m_poolAsset)
                    ->Field("CommonBufferPoolType", &BufferAsset::m_poolType)
                    ->Field("CompressionFormat", &BufferAsset::m_compressionFormat)
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
                serializeContext->Enum<BufferAsset::CompressionFormat>()
                    ->Value("Uncompressed", BufferAsset::CompressionFormat::Uncompressed)
                    ->Value("Index", BufferAsset::CompressionFormat::Index)
                    ->Value("Vertex", BufferAsset::CompressionFormat::Vertex)
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

        const BufferAsset::CompressionFormat BufferAsset::GetCompressionFormat() const
        {
            return m_compressionFormat;
        }

        Data::AssetHandler::LoadResult BufferAssetHandler::LoadAssetData(
                const AZ::Data::Asset<AZ::Data::AssetData>& asset,
                AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
                const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            // Decompress the buffer data upon loading
            if (Data::AssetHandler::LoadResult::LoadComplete == AssetHandler<BufferAsset>::LoadAssetData(asset, stream,
            assetLoadFilterCB))
            {
                BufferAsset* bufferAsset = asset.GetAs<BufferAsset>();
                AZStd::vector<uint8_t, BufferAsset::Allocator>& buffer = bufferAsset->m_buffer;
                AZStd::vector<uint8_t, BufferAsset::Allocator> uncompressed(bufferAsset->m_bufferDescriptor.m_byteCount);

                size_t elementSize = bufferAsset->m_bufferViewDescriptor.m_elementSize;
                size_t elementCount = bufferAsset->m_bufferDescriptor.m_byteCount / elementSize;
                switch (bufferAsset->m_compressionFormat)
                {
                case BufferAsset::CompressionFormat::Index:
                {
                    if (0 == meshopt_decodeIndexBuffer(uncompressed.data(), elementCount, elementSize, buffer.data(), buffer.size()))
                    {
                        buffer = AZStd::move(uncompressed);
                        return AZ::Data::AssetHandler::LoadResult::LoadComplete;
                    }
                    AZ_Error("BufferAssetHandler", false, "Failure decompressing index buffer");
                    break;
                }
                case BufferAsset::CompressionFormat::Vertex:
                {
                    if (0 == meshopt_decodeVertexBuffer(uncompressed.data(), elementCount, elementSize, buffer.data(), buffer.size()))
                    {
                        buffer = AZStd::move(uncompressed);
                        return AZ::Data::AssetHandler::LoadResult::LoadComplete;
                    }
                    AZ_Error("BufferAssetHandler", false, "Failure decompressing vertex buffer");
                    break;
                }
                default: // Uncompressed
                    return AZ::Data::AssetHandler::LoadResult::LoadComplete;
                }
            }
            return AZ::Data::AssetHandler::LoadResult::Error;
        }
    } //namespace RPI
} // namespace AZ
