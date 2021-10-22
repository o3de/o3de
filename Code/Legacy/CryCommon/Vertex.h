/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/array.h>
#include <CryCommon/BaseTypes.h>
#include <CryCommon/VertexFormats.h>

namespace AZ
{
    namespace Vertex
    {
        const uint32_t VERTEX_BUFFER_ALIGNMENT = 4;
        // This enum must only have 8 entries because only 3 bits are used to store usage.
        enum class AttributeUsage : uint8
        {
            Position,
            Color,
            Normal,
            TexCoord,
            Weights,
            Indices,
            Tangent,
            BiTangent,
            NumUsages
        };

        struct AttributeUsageData
        {
            AZStd::string friendlyName;
            AZStd::string semanticName;
        };

        static AttributeUsageData AttributeUsageDataTable[(uint)AttributeUsage::NumUsages] =
        {
            // {friendlyName, semanticName}
            { "Position", "POSITION" },
            { "Color", "COLOR" },
            { "Normal", "NORMAL" },
            { "TexCoord", "TEXCOORD" },
            { "Weights", "BLENDWEIGHT" },
            { "Indices", "BLENDINDICES" },
            { "Tangent", "TEXCOORD" },
            { "BiTangent", "TEXCOORD" }
        };

        // This enum must have 32 or less entries as 5 bits are used to store type.
        enum class AttributeType : uint8
        {
            Float16_1 = 0,
            Float16_2,
            Float16_4,

            Float32_1,
            Float32_2,
            Float32_3,
            Float32_4,

            Byte_1,
            Byte_2,
            Byte_4,

            Short_1,
            Short_2,
            Short_4,

            UInt16_1,
            UInt16_2,
            UInt16_4,

            UInt32_1,
            UInt32_2,
            UInt32_3,
            UInt32_4,

            NumTypes
        };

        struct AttributeTypeData
        {
            AZStd::string friendlyName;
            uint8 byteSize;
        };

        static AttributeTypeData AttributeTypeDataTable[(unsigned int)AZ::Vertex::AttributeType::NumTypes] =
        {
            { "Float16_1", 2 },
            { "Float16_2", 4 },
            { "Float16_4", 8 },

            { "Float32_1", 4 },
            { "Float32_2", 8 },
            { "Float32_3", 12 },
            { "Float32_4", 16 },

            { "Byte_1", 1 },
            { "Byte_2", 2 },
            { "Byte_4", 4 },

            { "Short_1", 2 },
            { "Short_2", 4 },
            { "Short_4", 8 },

            { "UInt16_1", 2 },
            { "UInt16_2", 4 },
            { "UInt16_4", 8 },

            { "UInt32_1", 4 },
            { "UInt32_2", 8 },
            { "UInt32_3", 12 },
            { "UInt32_4", 16 },
        };

        //! Stores the usage, type, and byte length of an individual vertex attribute
        class Attribute
        {
        public:
            // Usage stored in the 3 lower bits and Type stored in the 5 upper bits.
            static const uint8 kUsageBitCount = 3;
            static const uint8 kUsageMask = 0x07;
            static const uint8 kTypeMask = 0xf8;
            static uint8 CreateAttribute(AttributeUsage usage, AttributeType type)
            {
                return (static_cast<uint8>(type) << kUsageBitCount) | static_cast<uint8>(usage);
            }
            static AttributeUsage GetUsage(const uint8 attribute)
            {
                return static_cast<AttributeUsage>(attribute & kUsageMask);
            }
            static AttributeType GetType(const uint8 attribute)
            {
                return static_cast<AttributeType>((attribute & kTypeMask) >> kUsageBitCount);
            }
            static uint8 GetByteLength(const uint8 attribute)
            {
                return AttributeTypeDataTable[(uint)GetType(attribute)].byteSize;
            }
            static const AZStd::string& GetSemanticName(uint8 attribute)
            {
                return AttributeUsageDataTable[(uint)GetUsage(attribute)].semanticName;
            }
        };

        //! Flexible vertex format class
        class Format
        {
        public:
            Format(){};


            //! Conversion from old hard-coded EVertexFormat enum to new, flexible vertex class
            Format(EVertexFormat format)
            {
                m_enum = eVF_Unknown;
                m_numAttributes = 0;
                switch (format)
                {
                case eVF_Unknown:
                    m_enum = eVF_Unknown;
                    break;
                case eVF_P3F_C4B_T2F:
                    AddAttribute(Attribute::CreateAttribute(AttributeUsage::Position, AttributeType::Float32_3));
                    AddAttribute(Attribute::CreateAttribute(AttributeUsage::Color, AttributeType::Byte_4));
                    AddAttribute(Attribute::CreateAttribute(AttributeUsage::TexCoord, AttributeType::Float32_2));
                    m_enum = eVF_P3F_C4B_T2F;
                    break;
                case eVF_P3S_C4B_T2S:
                    AddAttribute(Attribute::CreateAttribute(AttributeUsage::Position, AttributeType::Float16_4));// vec3f16 is backed by a CryHalf4
                    AddAttribute(Attribute::CreateAttribute(AttributeUsage::Color, AttributeType::Byte_4));
                    AddAttribute(Attribute::CreateAttribute(AttributeUsage::TexCoord, AttributeType::Float16_2));
                    m_enum = eVF_P3S_C4B_T2S;
                    break;
                // Additional streams
                case eVF_W4B_I4S:// Skinned weights/indices stream.
                    AddAttribute(Attribute::CreateAttribute(AttributeUsage::Weights, AttributeType::Byte_4));
                    AddAttribute(Attribute::CreateAttribute(AttributeUsage::Indices, AttributeType::UInt16_4));
                    m_enum = eVF_W4B_I4S;
                    break;
                case eVF_P3F:// Velocity stream.
                    AddAttribute(Attribute::CreateAttribute(AttributeUsage::Position, AttributeType::Float32_3));
                    m_enum = eVF_P3F;
                    break;
                case eVF_P2F_C4B_T2F_F4B:
                    AddAttribute(Attribute::CreateAttribute(AttributeUsage::Position, AttributeType::Float32_2));
                    AddAttribute(Attribute::CreateAttribute(AttributeUsage::Color, AttributeType::Byte_4));
                    AddAttribute(Attribute::CreateAttribute(AttributeUsage::TexCoord, AttributeType::Float32_2));
                    AddAttribute(Attribute::CreateAttribute(AttributeUsage::Indices, AttributeType::UInt16_2));
                    m_enum = eVF_P2F_C4B_T2F_F4B;
                    break;
                case eVF_P3F_C4B:
                    AddAttribute(Attribute::CreateAttribute(AttributeUsage::Position, AttributeType::Float32_3));
                    AddAttribute(Attribute::CreateAttribute(AttributeUsage::Color, AttributeType::Byte_4));
                    m_enum = eVF_P3F_C4B;
                    break;
                case eVF_Max:
                default:
                    AZ_Error("VF", false, "Invalid vertex format");
                    m_enum = eVF_Unknown;
                }
                CalculateStrideAndUsageCounts();
            }

            //! Get the equivalent old-style EVertexFormat enum
            uint GetEnum() const { return m_enum; }

            static const uint8 kHas16BitFloatPosition  = 0x4;
            static const uint8 kHas16BitFloatTexCoords = 0x2;
            static const uint8 kHas32BitFloatTexCoords = 0x1;


            uint32 GetAttributeUsageCount(AttributeUsage usage) const
            {
                return (uint32)m_attributeUsageCounts[(uint)usage];
            }

            const uint8* GetAttributes( uint32 &outCount) const
            {
                outCount = m_numAttributes;
                return m_vertexAttributes;
            }

            uint GetStride(void) const { return m_stride; }

            // Quick comparison operators.
            bool operator==(const Format& other) const
            {
                return m_enum == other.m_enum;
            }
            bool operator!=(const Format& other) const
            {
                return !(*this == other);
            }
            bool operator==(const EVertexFormat& other) const
            {
                return m_enum == other;
            }
            // Used in RendermeshMerger.cpp
            // CHWShader_D3D::mfUpdateFXVertexFormat and CHWShader_D3D::mfVertexFormat want the max between two vertex formats...why? It seems like a bad guess of which vertex format to use since there's no particular order to EVertexFormats...other than the more specialized EVertexFormats come after the base EVertexFormats
            bool operator<(const Format& other) const
            {
                return m_enum < other.m_enum;
            }
            bool operator<=(const Format& other) const
            {
                return (*this == other || *this < other);
            }
            bool operator>(const Format& other) const
            {
                return !(*this <= other);
            }
            bool operator>=(const Format& other) const
            {
                return (*this == other || *this > other);
            }
        private:
            void AddAttribute(uint8 attribute)
            {
                AZ_Assert(m_numAttributes < kMaxAttributes, "Too many attributes added.  Change the size of kMaxAttributes");
                m_vertexAttributes[m_numAttributes++] = attribute;

                // Update the flags.
                AttributeUsage usage = Attribute::GetUsage(attribute);
                AttributeType type = Attribute::GetType(attribute);
                if (usage == AttributeUsage::TexCoord)
                {
                    if (type == AttributeType::Float16_2) {
                        m_flags |= kHas16BitFloatTexCoords;
                    }
                    else if (type == AttributeType::Float32_2 || type == AttributeType::Float32_3 || type == AttributeType::Float32_4) {
                        m_flags |= kHas32BitFloatTexCoords;
                    }
                }
                else if (usage == AttributeUsage::Position && type == AttributeType::Float16_4) {
                    m_flags |= kHas16BitFloatPosition;
                }
            }

            //! Calculates the sum of the size in bytes of all attributes that make up this format
            void CalculateStrideAndUsageCounts()
            {
                static_assert((uint32)AttributeUsage::NumUsages <= 8, "We use 3 bits to represent usage so we only support 8 usages for a vertex format attribute.");
                static_assert((uint32)AttributeType::NumTypes <= 32, "We use 5 bits to represent type so we only support up to 32 types for a vertex format attribute.");

                for (uint index = 0; index < (uint)AttributeUsage::NumUsages; ++index)
                {
                    m_attributeUsageCounts[index] = 0;
                }
                uint32 stride = 0;
                for (uint ii = 0; ii < m_numAttributes; ++ii)
                {
                    uint8 attribute = m_vertexAttributes[ii];
                    stride += Attribute::GetByteLength(attribute);
                    m_attributeUsageCounts[(uint)Attribute::GetUsage(attribute)]++;
                }
                AZ_Assert(stride < (0x1 << (sizeof(m_stride) * 8)), "Vertex stride is larger than the maximum supported, update the type for m_stride in Vertex.h");

                m_stride = static_cast<uint8>(stride);
            }



#ifdef PARTICLE_MOTION_BLUR
            static const uint32_t kMaxAttributes = 8;
#else
            static const uint32_t kMaxAttributes = 10;
#endif
            uint8 m_vertexAttributes[kMaxAttributes] = { 0 };

            uint8 m_attributeUsageCounts[(uint)AttributeUsage::NumUsages] = { 0 };
            uint8 m_numAttributes = 0;
            uint8 m_enum = eVF_Unknown;
            uint8 m_stride = 0;
            uint8 m_flags = 0x0;
        };

    }
}
