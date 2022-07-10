/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Vector3.h>

namespace EMotionFX
{
    enum AttributeType : AZ::u32
    {
        Position,
        Normal,
        Tangent,
        UVCoords,
        Bitangent,
        OrginalVertexNumber
    };

    //! The number of attributes required for the whitebox mesh.
    inline constexpr uint32_t NumAttributes = 6;

    //! Trait to describe EmotionFX mesh vertex attribute format.
    template<AttributeType AttributeTypeT>
    struct AttributeTrait
    {
    };

    template<>
    struct AttributeTrait<AttributeType::Position>
    {
        using TargetType = AZ::Vector3;
    };

    template<>
    struct AttributeTrait<AttributeType::OrginalVertexNumber>
    {
        using TargetType = AZ::u32;
    };

    template<>
    struct AttributeTrait<AttributeType::Normal>
    {
        using TargetType = AZ::Vector3;
    };

    template<>
    struct AttributeTrait<AttributeType::UVCoords>
    {
        using TargetType = AZ::Vector2;
    };

    template<>
    struct AttributeTrait<AttributeType::Tangent>
    {
        using TargetType = AZ::Vector4;
    };

    template<>
    struct AttributeTrait<AttributeType::Bitangent>
    {
        using TargetType = AZ::Vector3;
    };

    template<class T>
    class VertexAttributeLayerBuffer
    {
    public:

        VertexAttributeLayerBuffer():
            m_keepOrignal(false),
            m_data{{},{}}{

        }

        VertexAttributeLayerBuffer(const VertexAttributeLayerBuffer& attr):
            m_type(attr.m_type),
            m_keepOrignal(attr.m_keepOrignal),
            m_data{attr.m_data[0],attr.m_data[1]}{

        }

        VertexAttributeLayerBuffer(AttributeType type, const AZStd::vector<T>& buffer, bool keepOrignal)
            : m_type(type), 
            m_keepOrignal(keepOrignal),
            m_data({buffer, {}})
        {
            if(m_keepOrignal) {
               for (size_t i = 0; i < m_data[0].size(); i++)
                {
                    m_data[1].emplace_back(m_data[0][i]);
                }
            }
        }

        VertexAttributeLayerBuffer(AttributeType type, AZStd::vector<T>&& buffer, bool keepOrignal)
            : m_type(type), 
            m_keepOrignal(keepOrignal),
            m_data({buffer, {}})
        {
            if(m_keepOrignal) {
               for (size_t i = 0; i < m_data[0].size(); i++)
                {
                    m_data[1].emplace_back(m_data[0][i]);
                }
            }
        }

        const AttributeType GetType() const {
            return m_type;
        }

        AZStd::span<T> GetOrignal()
        {
            if (m_keepOrignal)
            {
                return m_data[1];
            }
            return m_data[0];
        }

        AZStd::span<T> GetData()
        {
            return m_data[0];
        }

        bool hasOrignal()
        {
            return m_keepOrignal;
        }

        void ResetToOriginalData()
        {
            if (m_keepOrignal)
            {
                for (size_t i = 0; i < m_data[0].size(); i++)
                {
                    m_data[0][i] = m_data[1][i];
                }
            }
        }

    private:
        AttributeType m_type;
        bool m_keepOrignal;
        AZStd::array<AZStd::vector<T>, 2> m_data;
    };


    // //! Attribute buffer alias for position attributes.
    // using PositionAttributeLayerBuffer = VertexAttributeLayerBuffer<AttributeType::Position>;

    // //! Attribute buffer alias for normal attributes.
    // using NormalAttributeLayerBuffer = VertexAttributeLayerBuffer<AttributeType::Normal>;

    // //! Attribute buffer alias for tangent attributes.
    // using TangentAttributeLayerBuffer = VertexAttributeLayerBuffer<AttributeType::Tangent>;

    // //! Attribute buffer alias for bitangent attributes.
    // using BitangentAttributeLayerBuffer = VertexAttributeLayerBuffer<AttributeType::Bitangent>;

    // //! Attribute buffer alias for uv attributes.
    // using UVAttributeLayerBuffer = VertexAttributeLayerBuffer<AttributeType::UVCoords>;
    
    // //! Attribute buffer alias for OrgVertex attributes.
    // using OrignalVertexAttributeLayerBuffer = VertexAttributeLayerBuffer<AttributeType::OrginalVertexNumber>;

} // namespace EMotionFX
