/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ::RHI
{
    void ShaderInputBufferDescriptor::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ShaderInputBufferDescriptor>()
                ->Version(5)
                ->Field("m_name", &ShaderInputBufferDescriptor::m_name)
                ->Field("m_type", &ShaderInputBufferDescriptor::m_type)
                ->Field("m_access", &ShaderInputBufferDescriptor::m_access)
                ->Field("m_count", &ShaderInputBufferDescriptor::m_count)
                ->Field("m_strideSize", &ShaderInputBufferDescriptor::m_strideSize)
                ->Field("m_registerId", &ShaderInputBufferDescriptor::m_registerId)
                ->Field("m_spaceId", &ShaderInputBufferDescriptor::m_spaceId);
        }

        ShaderInputBufferIndex::Reflect(context);
    }

    ShaderInputBufferDescriptor::ShaderInputBufferDescriptor(
        const Name& name,
        ShaderInputBufferAccess access,
        ShaderInputBufferType type,
        uint32_t bufferCount,
        uint32_t strideSize,
        uint32_t registerId,
        uint32_t spaceId)
        : m_name{ name }
        , m_access{ access }
        , m_type{ type }
        , m_count{ bufferCount }
        , m_strideSize{ strideSize }
        , m_registerId{ registerId }
        , m_spaceId{ spaceId }
    {}

    HashValue64 ShaderInputBufferDescriptor::GetHash(HashValue64 seed) const
    {
        seed = TypeHash64(m_name.GetHash(), seed);
        seed = TypeHash64(m_access, seed);
        seed = TypeHash64(m_type, seed);
        seed = TypeHash64(m_count, seed);
        seed = TypeHash64(m_strideSize, seed);
        seed = TypeHash64(m_registerId, seed);
        return seed;
    }

    void ShaderInputImageDescriptor::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ShaderInputImageDescriptor>()
                ->Version(4)
                ->Field("m_name", &ShaderInputImageDescriptor::m_name)
                ->Field("m_type", &ShaderInputImageDescriptor::m_type)
                ->Field("m_access", &ShaderInputImageDescriptor::m_access)
                ->Field("m_count", &ShaderInputImageDescriptor::m_count)
                ->Field("m_registerId", &ShaderInputImageDescriptor::m_registerId)
                ->Field("m_spaceId", &ShaderInputImageDescriptor::m_spaceId);
        }

        ShaderInputImageIndex::Reflect(context);
    }

    ShaderInputImageDescriptor::ShaderInputImageDescriptor(
        const Name& name,
        ShaderInputImageAccess access,
        ShaderInputImageType type,
        uint32_t imageCount,
        uint32_t registerId,
        uint32_t spaceId)
        : m_name{ name }
        , m_access{ access }
        , m_type{ type }
        , m_count{ imageCount }
        , m_registerId{ registerId }
        , m_spaceId{ spaceId }
    {}

    HashValue64 ShaderInputImageDescriptor::GetHash(HashValue64 seed) const
    {
        seed = TypeHash64(m_name.GetHash(), seed);
        seed = TypeHash64(m_access, seed);
        seed = TypeHash64(m_type, seed);
        seed = TypeHash64(m_count, seed);
        seed = TypeHash64(m_registerId, seed);
        return seed;
    }

    void ShaderInputBufferUnboundedArrayDescriptor::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ShaderInputBufferUnboundedArrayDescriptor>()
                ->Version(2)
                ->Field("m_name", &ShaderInputBufferUnboundedArrayDescriptor::m_name)
                ->Field("m_type", &ShaderInputBufferUnboundedArrayDescriptor::m_type)
                ->Field("m_access", &ShaderInputBufferUnboundedArrayDescriptor::m_access)
                ->Field("m_strideSize", &ShaderInputBufferUnboundedArrayDescriptor::m_strideSize)
                ->Field("m_registerId", &ShaderInputBufferUnboundedArrayDescriptor::m_registerId)
                ->Field("m_spaceId", &ShaderInputBufferUnboundedArrayDescriptor::m_spaceId);
        }

        ShaderInputBufferUnboundedArrayIndex::Reflect(context);
    }

    ShaderInputBufferUnboundedArrayDescriptor::ShaderInputBufferUnboundedArrayDescriptor(
        const Name& name,
        ShaderInputBufferAccess access,
        ShaderInputBufferType type,
        uint32_t strideSize,
        uint32_t registerId,
        uint32_t spaceId)
        : m_name{ name }
        , m_access{ access }
        , m_type{ type }
        , m_strideSize { strideSize }
        , m_registerId{ registerId }
        , m_spaceId{ spaceId }
    {}

    HashValue64 ShaderInputBufferUnboundedArrayDescriptor::GetHash(HashValue64 seed) const
    {
        seed = TypeHash64(m_name.GetHash(), seed);
        seed = TypeHash64(m_access, seed);
        seed = TypeHash64(m_type, seed);
        seed = TypeHash64(m_strideSize, seed);
        seed = TypeHash64(m_registerId, seed);
        return seed;
    }

    void ShaderInputImageUnboundedArrayDescriptor::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ShaderInputImageUnboundedArrayDescriptor>()
                ->Version(2)
                ->Field("m_name", &ShaderInputImageUnboundedArrayDescriptor::m_name)
                ->Field("m_type", &ShaderInputImageUnboundedArrayDescriptor::m_type)
                ->Field("m_access", &ShaderInputImageUnboundedArrayDescriptor::m_access)
                ->Field("m_registerId", &ShaderInputImageUnboundedArrayDescriptor::m_registerId)
                ->Field("m_spaceId", &ShaderInputImageUnboundedArrayDescriptor::m_spaceId);
        }

        ShaderInputImageUnboundedArrayIndex::Reflect(context);
    }

    ShaderInputImageUnboundedArrayDescriptor::ShaderInputImageUnboundedArrayDescriptor(
        const Name& name,
        ShaderInputImageAccess access,
        ShaderInputImageType type,
        uint32_t registerId,
        uint32_t spaceId)
        : m_name{ name }
        , m_access{ access }
        , m_type { type }
        , m_registerId{ registerId }
        , m_spaceId{ spaceId }
    {}

    HashValue64 ShaderInputImageUnboundedArrayDescriptor::GetHash(HashValue64 seed) const
    {
        seed = TypeHash64(m_name.GetHash(), seed);
        seed = TypeHash64(m_access, seed);
        seed = TypeHash64(m_type, seed);
        seed = TypeHash64(m_registerId, seed);
        return seed;
    }

    void ShaderInputSamplerDescriptor::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ShaderInputSamplerDescriptor>()
                ->Version(4)
                ->Field("m_name", &ShaderInputSamplerDescriptor::m_name)
                ->Field("m_count", &ShaderInputSamplerDescriptor::m_count)
                ->Field("m_registerId", &ShaderInputSamplerDescriptor::m_registerId)
                ->Field("m_spaceId", &ShaderInputSamplerDescriptor::m_spaceId);
        }

        ShaderInputSamplerIndex::Reflect(context);
    }

    ShaderInputSamplerDescriptor::ShaderInputSamplerDescriptor(
        const Name& name,
        uint32_t samplerCount,
        uint32_t registerId,
        uint32_t spaceId)
        : m_name{ name }
        , m_count{ samplerCount }
        , m_registerId{ registerId }
        , m_spaceId{ spaceId }
    {}

    HashValue64 ShaderInputSamplerDescriptor::GetHash(HashValue64 seed) const
    {
        seed = TypeHash64(m_name.GetHash(), seed);
        seed = TypeHash64(m_count, seed);
        seed = TypeHash64(m_registerId, seed);
        return seed;
    }

    void ShaderInputConstantDescriptor::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ShaderInputConstantDescriptor>()
                ->Version(4)
                ->Field("m_name", &ShaderInputConstantDescriptor::m_name)
                ->Field("m_constantByteOffset", &ShaderInputConstantDescriptor::m_constantByteOffset)
                ->Field("m_constantByteCount", &ShaderInputConstantDescriptor::m_constantByteCount)
                ->Field("m_registerId", &ShaderInputConstantDescriptor::m_registerId)
                ->Field("m_spaceId", &ShaderInputConstantDescriptor::m_spaceId);
        }

        ShaderInputConstantIndex::Reflect(context);
    }

    ShaderInputConstantDescriptor::ShaderInputConstantDescriptor(
        const Name& name,
        uint32_t constantByteOffset,
        uint32_t constantByteCount,
        uint32_t registerId,
        uint32_t spaceId)
        : m_name{ name }
        , m_constantByteOffset{ constantByteOffset }
        , m_constantByteCount{ constantByteCount }
        , m_registerId{ registerId }
        , m_spaceId{ spaceId }
    {}

    HashValue64 ShaderInputConstantDescriptor::GetHash(HashValue64 seed) const
    {
        seed = TypeHash64(m_name.GetHash(), seed);
        seed = TypeHash64(m_constantByteOffset, seed);
        seed = TypeHash64(m_constantByteCount, seed);
        seed = TypeHash64(m_registerId, seed);
        return seed;
    }

    void ShaderInputStaticSamplerDescriptor::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ShaderInputStaticSamplerDescriptor>()
                ->Version(2)
                ->Field("m_name", &ShaderInputStaticSamplerDescriptor::m_name)
                ->Field("m_samplerState", &ShaderInputStaticSamplerDescriptor::m_samplerState)
                ->Field("m_registerId", &ShaderInputStaticSamplerDescriptor::m_registerId)
                ->Field("m_spaceId", &ShaderInputStaticSamplerDescriptor::m_spaceId);
        }

        ShaderInputStaticSamplerIndex::Reflect(context);
    }

    ShaderInputStaticSamplerDescriptor::ShaderInputStaticSamplerDescriptor(
        const Name& name, const SamplerState& samplerState, uint32_t registerId, uint32_t spaceId)
        : m_name{name}
        , m_samplerState{ samplerState }
        , m_registerId{ registerId }
        , m_spaceId{ spaceId }
    {}

    HashValue64 ShaderInputStaticSamplerDescriptor::GetHash(HashValue64 seed) const
    {
        seed = TypeHash64(m_name.GetHash(), seed);
        seed = m_samplerState.GetHash(seed);
        seed = TypeHash64(m_registerId, seed);
        return seed;
    }

    const char* GetShaderInputTypeName(ShaderInputBufferType bufferInputType)
    {
        switch (bufferInputType)
        {
        case ShaderInputBufferType::Structured:
            return "Structured";
        case ShaderInputBufferType::Typed:
            return "Typed";
        case ShaderInputBufferType::Raw:
            return "Raw";
        case ShaderInputBufferType::AccelerationStructure:
            return "AccelerationStructure";
        }
        return "";
    }

    const char* GetShaderInputTypeName(ShaderInputImageType imageInputType)
    {
        switch (imageInputType)
        {
        case ShaderInputImageType::Image1D:
            return "Image1D";
        case ShaderInputImageType::Image1DArray:
            return "Image1DArray";
        case ShaderInputImageType::Image2D:
            return "Image2D";
        case ShaderInputImageType::Image2DArray:
            return "Image2DArray";
        case ShaderInputImageType::Image2DMultisample:
            return "Image2DMultisample";
        case ShaderInputImageType::Image2DMultisampleArray:
            return "Image2DMultisampleArray";
        case ShaderInputImageType::Image3D:
            return "Image3D";
        case ShaderInputImageType::ImageCube:
            return "ImageCube";
        case ShaderInputImageType::ImageCubeArray:
            return "ImageCubeArray";
        case ShaderInputImageType::SubpassInput:
            return "SubpassInput";
        }
        return "";
    }

    const char* GetShaderInputAccessName(ShaderInputBufferAccess bufferInputAcces)
    {
        switch (bufferInputAcces)
        {
        case ShaderInputBufferAccess::Constant:
            return "BufferConstant";
        case ShaderInputBufferAccess::Read:
            return "BufferRead";
        case ShaderInputBufferAccess::ReadWrite:
            return "BufferReadWrite";
        }
        return "";
    }

    const char* GetShaderInputAccessName(ShaderInputImageAccess imageInputAccess)
    {
        switch (imageInputAccess)
        {
        case ShaderInputImageAccess::Read:
            return "ImageRead";
        case ShaderInputImageAccess::ReadWrite:
            return "ImageReadWrite";
        }
        return "";
    }
}
