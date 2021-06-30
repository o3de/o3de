/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "WhiteBoxBuffer.h"

#include <Atom/RHI.Reflect/ShaderSemantic.h>
#include <Atom/RPI.Reflect/Model/ModelLodAssetCreator.h>

namespace WhiteBox
{
    //! Attributes for white box mesh vertices.
    enum class AttributeType
    {
        Position,
        Normal,
        Tangent,
        Bitangent,
        UV,
        Color
    };

    //! The number of attributes required by the white box mesh.
    inline constexpr uint32_t NumAttributes = 6;

    //! Trait to describe white box mesh vertex attribute format.
    template<AttributeType AttributeTypeT>
    struct AttributeTrait
    {
    };

    //! Attribute trait specialization for vertex position attribute.
    template<>
    struct AttributeTrait<AttributeType::Position>
    {
        static constexpr const char* ShaderSemantic = "POSITION";
        using BufferType = Vector3Buffer;
    };

    //! Attribute trait specialization for vertex normal attribute
    template<>
    struct AttributeTrait<AttributeType::Normal>
    {
        static constexpr const char* ShaderSemantic = "NORMAL";
        using BufferType = Vector3Buffer;
    };

    //! Attribute trait specialization for vertex tangent attribute.
    template<>
    struct AttributeTrait<AttributeType::Tangent>
    {
        static constexpr const char* ShaderSemantic = "TANGENT";
        using BufferType = Vector4Buffer;
    };

    //! Attribute trait specialization for vertex bitangent attribute.
    template<>
    struct AttributeTrait<AttributeType::Bitangent>
    {
        static constexpr const char* ShaderSemantic = "BITANGENT";
        using BufferType = Vector3Buffer;
    };

    //! Attribute trait specialization for vertex uv attribute.
    template<>
    struct AttributeTrait<AttributeType::UV>
    {
        static constexpr const char* ShaderSemantic = "UV";
        using BufferType = Vector2Buffer;
    };

    //! Attribute trait specialization for vertex color attribute.
    template<>
    struct AttributeTrait<AttributeType::Color>
    {
        static constexpr const char* ShaderSemantic = "COLOR";
        using BufferType = Vector4Buffer;
    };

    //! Buffer to hold white box mesh vertex attribute data.
    template<AttributeType AttributeTypeT>
    class AttributeBuffer
    {
    public:
        using Trait = AttributeTrait<AttributeTypeT>;

        //! Construct a new Attribute Buffer object from the specified data.
        template<typename VertexStreamDataType>
        AttributeBuffer(const AZStd::vector<VertexStreamDataType>& data);

        //! Retrieves the buffer asset.
        const AZ::Data::Asset<AZ::RPI::BufferAsset>& GetBuffer() const;

        //! Retrieves the buffer view descriptor.
        const AZ::RHI::BufferViewDescriptor& GetBufferViewDescriptor() const;

        //! Retrieves the buffer asset view.
        const AZ::RPI::BufferAssetView& GetBufferAssetView() const;

        //! Retrieves the attribute's shader semantic.
        const AZ::RHI::ShaderSemantic& GetShaderSemantic() const;

        //! Adds this attribute buffer to the Lod.
        void AddLodStreamBuffer(AZ::RPI::ModelLodAssetCreator& modelLodCreator) const;

        //! Adds this attribute buffer to the mesh.
        void AddMeshStreamBuffer(AZ::RPI::ModelLodAssetCreator& modelLodCreator) const;

        //! Returns true of the attribute buffer is valid, otherwise false.
        bool IsValid() const;

        //! Update the attribute buffer contents with the new data.
        template<typename VertexStreamDataType>
        bool UpdateData(const AZStd::vector<VertexStreamDataType>& data);

    private:
        typename Trait::BufferType m_buffer;
        AZ::RHI::ShaderSemantic m_shaderSemantic;
    };

    template<AttributeType AttributeTypeT>
    template<typename VertexStreamDataType>
    AttributeBuffer<AttributeTypeT>::AttributeBuffer(const AZStd::vector<VertexStreamDataType>& data)
        : m_buffer(data)
        , m_shaderSemantic(AZ::Name(Trait::ShaderSemantic))
    {
        if (!IsValid())
        {
            AZ_Error(
                "AttributeBuffer", false, "Couldn't create buffer for attribute %s",
                m_shaderSemantic.ToString().c_str());
        }
    }

    template<AttributeType AttributeTypeT>
    const AZ::Data::Asset<AZ::RPI::BufferAsset>& AttributeBuffer<AttributeTypeT>::GetBuffer() const
    {
        return m_buffer.GetBuffer();
    }

    template<AttributeType AttributeTypeT>
    const AZ::RHI::BufferViewDescriptor& AttributeBuffer<AttributeTypeT>::GetBufferViewDescriptor() const
    {
        return m_buffer.GetBufferViewDescriptor();
    }

    template<AttributeType AttributeTypeT>
    const AZ::RPI::BufferAssetView& AttributeBuffer<AttributeTypeT>::GetBufferAssetView() const
    {
        return m_buffer.GetBufferAssetView();
    }

    template<AttributeType AttributeTypeT>
    const AZ::RHI::ShaderSemantic& AttributeBuffer<AttributeTypeT>::GetShaderSemantic() const
    {
        return m_shaderSemantic;
    }

    template<AttributeType AttributeTypeT>
    void AttributeBuffer<AttributeTypeT>::AddLodStreamBuffer(AZ::RPI::ModelLodAssetCreator& modelLodCreator) const
    {
        modelLodCreator.AddLodStreamBuffer(GetBuffer());
    }

    template<AttributeType AttributeTypeT>
    void AttributeBuffer<AttributeTypeT>::AddMeshStreamBuffer(AZ::RPI::ModelLodAssetCreator& modelLodCreator) const
    {
        modelLodCreator.AddMeshStreamBuffer(GetShaderSemantic(), AZ::Name(), GetBufferAssetView());
    }

    template<AttributeType AttributeTypeT>
    bool AttributeBuffer<AttributeTypeT>::IsValid() const
    {
        return m_buffer.IsValid();
    }

    template<AttributeType AttributeTypeT>
    template<typename VertexStreamDataType>
    bool AttributeBuffer<AttributeTypeT>::UpdateData(const AZStd::vector<VertexStreamDataType>& data)
    {
        if (!m_buffer.UpdateData(data))
        {
            AZ_Error(
                "AttributeBuffer", false, "Couldn't update buffer for attribute %s",
                m_shaderSemantic.ToString().c_str());
            return false;
        }

        return true;
    }

    //! Attribute buffer alias for position attributes.
    using PositionAttribute = AttributeBuffer<AttributeType::Position>;

    //! Attribute buffer alias for normal attributes.
    using NormalAttribute = AttributeBuffer<AttributeType::Normal>;

    //! Attribute buffer alias for tangent attributes.
    using TangentAttribute = AttributeBuffer<AttributeType::Tangent>;

    //! Attribute buffer alias for bitangent attributes.
    using BitangentAttribute = AttributeBuffer<AttributeType::Bitangent>;

    //! Attribute buffer alias for uv attributes.
    using UVAttribute = AttributeBuffer<AttributeType::UV>;

    //! Attribute buffer alias for color attributes.
    using ColorAttribute = AttributeBuffer<AttributeType::Color>;
} // namespace WhiteBox
