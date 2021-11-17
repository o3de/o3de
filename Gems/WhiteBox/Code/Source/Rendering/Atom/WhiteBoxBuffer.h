/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "PackedFloat2.h"

#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Reflect/Buffer/BufferAsset.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetView.h>
#include <AzCore/Math/PackedVector3.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

namespace WhiteBox
{
    //! Get the Atom format for the specified vertex stream type.
    template<typename VertexStreamDataType>
    constexpr AZ::RHI::Format GetFormatForVertexStreamDataType()
    {
        if (std::is_same_v<VertexStreamDataType, uint32_t>)
        {
            return AZ::RHI::Format::R32_UINT;
        }
        else if (std::is_same_v<VertexStreamDataType, PackedFloat2>)
        {
            return AZ::RHI::Format::R32G32_FLOAT;
        }
        else if (std::is_same_v<VertexStreamDataType, AZ::PackedVector3f>)
        {
            return AZ::RHI::Format::R32G32B32_FLOAT;
        }
        else if (std::is_same_v<VertexStreamDataType, AZ::Vector4>)
        {
            return AZ::RHI::Format::R32G32B32A32_FLOAT;
        }
        else
        {
            // will result in a compile time assertion failure in the Buffer class.
            return AZ::RHI::Format::Unknown;
        }
    }

    //! Buffer for holding vertex attribute data to be trasferred to the GPU for mesh rendering.
    template<typename VertexStreamDataType>
    class Buffer
    {
    public:
        //! Constructs the buffer from the specified data in vertex stream format.
        Buffer(const AZStd::vector<VertexStreamDataType>& data);

        //! Retrieves the buffer asset.
        const AZ::Data::Asset<AZ::RPI::BufferAsset>& GetBuffer() const;

        //! Retrieves the buffer view descriptor.
        const AZ::RHI::BufferViewDescriptor& GetBufferViewDescriptor() const;

        //! Retrieves the buffer asset view.
        const AZ::RPI::BufferAssetView& GetBufferAssetView() const;

        //! Returns true of the buffer is valid, otherwise false.
        bool IsValid() const;

        //! Update the buffer contents with the new data.
        bool UpdateData(const AZStd::vector<VertexStreamDataType>& data);

    private:
        AZ::Data::Asset<AZ::RPI::BufferAsset> m_buffer;
        AZ::RHI::BufferViewDescriptor m_bufferViewDescriptor;
        AZ::RPI::BufferAssetView m_bufferAssetView;
        bool m_isValid = false;

        //! The format used by the buffer (must be one of the supported types in GetFormatForVertexStreamDataType).
        static constexpr auto VertexStreamFormat = GetFormatForVertexStreamDataType<VertexStreamDataType>();
        static_assert(
            VertexStreamFormat != AZ::RHI::Format::Unknown, "Cannot initialize a buffer with an unknown format.");
    };

    template<typename VertexStreamDataType>
    Buffer<VertexStreamDataType>::Buffer(const AZStd::vector<VertexStreamDataType>& data)
    {
        const uint32_t elementCount = static_cast<uint32_t>(data.size());
        const uint32_t elementSize = sizeof(VertexStreamDataType);
        const uint32_t bufferSize = elementCount * elementSize;

        // create a buffer view spanning the entire buffer
        m_bufferViewDescriptor = AZ::RHI::BufferViewDescriptor::CreateTyped(0, elementCount, VertexStreamFormat);

        // specify the data format for vertex stream data
        AZ::RHI::BufferDescriptor bufferDescriptor;
        bufferDescriptor.m_bindFlags = AZ::RHI::BufferBindFlags::InputAssembly | AZ::RHI::BufferBindFlags::ShaderRead;
        bufferDescriptor.m_byteCount = bufferSize;
        bufferDescriptor.m_alignment = elementSize;

        // create the buffer with the specified data
        AZ::RPI::BufferAssetCreator bufferAssetCreator;
        bufferAssetCreator.Begin(AZ::Uuid::CreateRandom());
        bufferAssetCreator.SetUseCommonPool(AZ::RPI::CommonBufferPoolType::StaticInputAssembly);
        bufferAssetCreator.SetBuffer(data.data(), bufferDescriptor.m_byteCount, bufferDescriptor);
        bufferAssetCreator.SetBufferViewDescriptor(m_bufferViewDescriptor);

        if (!bufferAssetCreator.End(m_buffer))
        {
            AZ_Error("Buffer", false, "Couldn't create buffer asset.");
        }
        else if (!m_buffer.IsReady())
        {
            AZ_Error("Buffer", false, "Asset is not ready.");
        }
        else if (!m_buffer.Get())
        {
            AZ_Error("Buffer", false, "Asset is nullptr.");
        }
        else
        {
            m_bufferAssetView = AZ::RPI::BufferAssetView{m_buffer, m_bufferViewDescriptor};
            m_isValid = true;
        }
    }

    template<typename VertexStreamDataType>
    const AZ::Data::Asset<AZ::RPI::BufferAsset>& Buffer<VertexStreamDataType>::GetBuffer() const
    {
        return m_buffer;
    }

    template<typename VertexStreamDataType>
    const AZ::RHI::BufferViewDescriptor& Buffer<VertexStreamDataType>::GetBufferViewDescriptor() const
    {
        return m_bufferViewDescriptor;
    }

    template<typename VertexStreamDataType>
    const AZ::RPI::BufferAssetView& Buffer<VertexStreamDataType>::GetBufferAssetView() const
    {
        return m_bufferAssetView;
    }

    template<typename VertexStreamDataType>
    bool Buffer<VertexStreamDataType>::IsValid() const
    {
        return m_isValid;
        ;
    }

    template<typename VertexStreamDataType>
    bool Buffer<VertexStreamDataType>::UpdateData(const AZStd::vector<VertexStreamDataType>& data)
    {
        if (!IsValid())
        {
            AZ_Error("UpdateData", false, "Buffer is not valid.");
            return false;
        }

        auto buffer = AZ::RPI::Buffer::FindOrCreate(m_buffer);

        if (!buffer)
        {
            AZ_Error("UpdateData", false, "Buffer could not be found.");
            return false;
        }

        const uint32_t elementCount = static_cast<uint32_t>(data.size());
        const uint32_t elementSize = sizeof(VertexStreamDataType);
        const uint32_t bufferSize = elementCount * elementSize;

        if (bufferSize > m_bufferViewDescriptor.m_elementCount * m_bufferViewDescriptor.m_elementSize)
        {
            AZ_Error("UpdateData", false, "Specfied buffer update exceeds capacity.");
            return false;
        }

        if (!buffer->UpdateData(data.data(), bufferSize))
        {
            AZ_Error("UpdateData", false, "Buffer could not be updated.");
            m_isValid = false;
            return false;
        }

        return true;
    }

    //! Buffer alias for unsigned 32 bit integer indices.
    using IndexBuffer = Buffer<uint32_t>;

    //! Buffer alias for PackedFloat2 vertices.
    using Vector2Buffer = Buffer<PackedFloat2>;

    //! Buffer alias for AZ::PackedVector3f vertices.
    using Vector3Buffer = Buffer<AZ::PackedVector3f>;

    //! Buffer alias for AZ::Vector4 vertices.
    using Vector4Buffer = Buffer<AZ::Vector4>;
} // namespace WhiteBox
