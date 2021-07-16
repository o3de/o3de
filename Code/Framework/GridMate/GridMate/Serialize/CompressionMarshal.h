/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_UTILS_COMPRESSION_MARSHAL
#define GM_UTILS_COMPRESSION_MARSHAL

#include <GridMate/GridMate.h>
#include <GridMate/Serialize/Buffer.h>
#include <GridMate/Serialize/MarshalerTypes.h>
#include <GridMate/Serialize/MathMarshal.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/containers/bitset.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/typetraits/underlying_type.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>

namespace GridMate
{
    /**
    * Compresses a float into a float16 based on a range.
    * Precision will vary since we can encode 65535 values within
    * the provided range.
    */
    class Float16Marshaler
    {
    public:
        AZ_TYPE_INFO(Float16Marshaler, "{CEC3001A-3DE2-42A7-BCCB-38F61477237D}");

        typedef float DataType;

        static const AZStd::size_t MarshalSize = sizeof(AZ::u16);

        Float16Marshaler(float rangeMin, float rangeMax);

        void Marshal(WriteBuffer& wb, float value) const;
        void Unmarshal(float& f, ReadBuffer& rb) const;

    private:
        float m_min;
        float m_range;
    };

    /**
    * Compress to half, losing half of the precision.
    * The internal format is:
    * 1 bit sign bit
    * 5 bits exponent, biased by 15
    * 10 bits mantissa, hidden leading bit, normalized to 1.0
    */
    class HalfMarshaler
    {
    public:
        AZ_TYPE_INFO(HalfMarshaler, "{A11F3B68-423A-472D-8D8C-6A2923ECB155}");

        typedef float DataType;

        static const AZStd::size_t MarshalSize = sizeof(AZ::u16);

        void Marshal(WriteBuffer& wb, float value) const;
        void Unmarshal(float& f, ReadBuffer& rb) const;
    };

    /**
    * Writes a compressed Vector2.
    * Values are compressed with \ref HalfMarshaler.
    * Uses 4 bytes.
    */
    class Vec2CompMarshaler
    {
    public:
        AZ_TYPE_INFO(Vec2CompMarshaler, "{7BB471FB-1A1F-47BD-A599-C23417FEEDE0}");

        typedef AZ::Vector2 DataType;

        static const AZStd::size_t MarshalSize = HalfMarshaler::MarshalSize * 2;

        void Marshal(WriteBuffer& wb, const AZ::Vector2& vec) const;
        void Unmarshal(AZ::Vector2& vec, ReadBuffer& rb) const;
    };

    /**
    * Writes a compressed vector.
    * Values are compressed with \ref HalfMarshaler.
    * Uses 6 bytes.
    */
    class Vec3CompMarshaler
    {
    public:
        AZ_TYPE_INFO(Vec3CompMarshaler, "{F20132F4-CA69-4F6F-A379-0BCF990E6672}");

        typedef AZ::Vector3 DataType;

        static const AZStd::size_t MarshalSize = HalfMarshaler::MarshalSize * 3;

        void Marshal(WriteBuffer& wb, const AZ::Vector3& vec) const;
        void Unmarshal(AZ::Vector3& vec, ReadBuffer& rb) const;
    };

    /**
    * Writes a compressed (float 16) normalized vector.
    * Uses 1 to 5 bytes, depending on the data.
    * Compressed values are compressed using \ref Float16Marshaler [-1.0f,1.0f]
    */
    class Vec3CompNormMarshaler
    {
        enum Flags
        {
            X_NEG = (1 << 0),
            Y_ZERO = (1 << 1),
            Z_ZERO = (1 << 2),
            Y_ONE = (1 << 3),
            Z_ONE = (1 << 4)
        };
    public:
        AZ_TYPE_INFO(Vec3CompNormMarshaler, "{80A7F05E-2F24-4CF4-AC91-C1C683D7CB2B}");

        typedef AZ::Vector3 DataType;

        void Marshal(WriteBuffer& wb, const AZ::Vector3& norVec) const;
        void Unmarshal(AZ::Vector3& vec, ReadBuffer& rb) const;
    };

    /**
    * Quaternion compressed marshaler
    * Values are compressed with \ref HalfMarshaler.
    * Uses 8 bytes.
    */
    class QuatCompMarshaler
    {
    public:
        AZ_TYPE_INFO(QuatCompMarshaler, "{21C83ED8-5E0E-4A5E-862D-9F1EBBD0CF4C}");

        typedef AZ::Quaternion DataType;

        static const AZStd::size_t MarshalSize = HalfMarshaler::MarshalSize * 4;

        void Marshal(WriteBuffer& wb, const AZ::Quaternion& quat) const;
        void Unmarshal(AZ::Quaternion& quat, ReadBuffer& rb) const;
    };

    /**
    * Compressed normalized Quaternion Marshaler uses 1-7 bytes depending of the data.
    * Compressed values are compressed using \ref Float16Marshaler [-1.0f,1.0f]
    */
    class QuatCompNormMarshaler
    {
        enum Flags
        {
            X_ZERO = (1 << 0),
            Y_ZERO = (1 << 1),
            Z_ZERO = (1 << 2),
            X_ONE = (1 << 3),
            Y_ONE = (1 << 4),
            Z_ONE = (1 << 5),
            W_NEG = (1 << 6),
        };
    public:
        AZ_TYPE_INFO(QuatCompNormMarshaler, "{8C39D143-F64E-45A8-B135-E10A06923CD2}");

        typedef AZ::Quaternion DataType;

        void Marshal(WriteBuffer& wb, const AZ::Quaternion& norQuat) const;
        void Unmarshal(AZ::Quaternion& quat, ReadBuffer& rb) const;
    };

    /**
    * Compressed normalized Quaternion Marshaler uses 1-4 bytes by converting to Euler angles.
    * Angles are quantized to angle * (360/255) and stored in a single byte.
    * A leading byte is used to indicate when are 0, or 1 and do not need to be sent in the data.
    */
    // quantized into a single byte so 360 degrees -> 256 different values
    static const float kDegreesPerQuantizedValue = 1.40625;

    class QuatCompNormQuantizedMarshaler
    {
        enum Flags
        {
            X_ZERO = (1 << 0),
            Y_ZERO = (1 << 1),
            Z_ZERO = (1 << 2),

            X_ONE = (1 << 3),
            Y_ONE = (1 << 4),
            Z_ONE = (1 << 5),
        };

    public:
        AZ_TYPE_INFO(QuatCompNormQuantizedMarshaler, "{D4318C51-839B-40BE-9850-417177AC9B22}");

        typedef AZ::Quaternion DataType;

        void Marshal(WriteBuffer& wb, const AZ::Quaternion& norQuat) const;
        void Unmarshal(AZ::Quaternion& quat, ReadBuffer& rb) const;
    };


    /**
    * Compressor/Marshaler for Transform
    * Uses 1 byte to describe marshaled components.
    * If present, scale uses 6 bytes.
    * If present, rotation uses 8 bytes.
    * If present, position uses 12 bytes.
    */
    class TransformCompressor
    {
    public:
        AZ_TYPE_INFO(TransformCompressor, "{30E9BADC-2CC3-46AF-B472-5A97E1FEC7EE}");

        typedef AZ::Transform DataType;

        enum Flags
        {
            HAS_SCALE = 1 << 0,
            HAS_ROT = 1 << 1,
            HAS_POS = 1 << 2
        };

        void Marshal(WriteBuffer& wb, const AZ::Transform& value) const;
        void Unmarshal(AZ::Transform& value, ReadBuffer& rb) const;
    };

    /**
    * Integer Quantizer to quantize an Integer value.
    * Uses unsigned 8, 16 or 32 bits to represent Quantized value.
    */
    template<int Min, int Max, size_t Bytes>
    class IntegerQuantizationMarshaler
    {
        static_assert(Bytes == 1 || Bytes == 2 || Bytes == 4, "Invalid Byte value, Supported values - 1 byte, 2 bytes and 4 bytes");
    };

    namespace Internal
    {
        template<int Min, int Max, typename SerializedType, AZ::u32 RatioMax>
        struct IntegerQuantizationMarshalerImpl
        {
            static_assert(Min < Max, "Enter a Valid Range");
            static const AZStd::size_t MarshalSize = sizeof(SerializedType);

            template<class T>
            void Marshal(WriteBuffer& wb, T integerQuant) const
            {
                AZ_Assert(integerQuant >= Min && integerQuant <= Max, "Value is out of range");

                const float quantRange = static_cast<float>(Max - Min);
                float quantScale = (integerQuant - Min) / quantRange;
                quantScale = AZ::GetClamp(quantScale, 0.0f, 1.0f);

                SerializedType quantizedValue = static_cast<SerializedType>(quantScale * aznumeric_cast<double>(RatioMax));
                wb.Write(quantizedValue);
            }

            template<class T>
            void Unmarshal(T& integerQuant, ReadBuffer& rb) const
            {
                const float ratio = static_cast<float>(Max - Min) / static_cast<float>(RatioMax);
                SerializedType readValue;
                rb.Read(readValue);
                integerQuant = static_cast<T>(Min + (readValue * ratio));
            }
        };
    }

    /**
    * Integer Quantization Marshaler of size 1 byte
    * Writes an 8 bit integer value.
    */
    template<int Min, int Max>
    class IntegerQuantizationMarshaler<Min, Max, 1>
        : public Internal::IntegerQuantizationMarshalerImpl<Min, Max, AZ::u8, 255>
    {
    };

    /**
    * Integer Quantization Marshaler of size 2 bytes
    * Writes a 16 bit integer value.
    */
    template<int Min, int Max>
    class IntegerQuantizationMarshaler<Min, Max, 2>
        : public Internal::IntegerQuantizationMarshalerImpl<Min, Max, AZ::u16, 65535>
    {
    };

    /**
    * Integer Quantization Marshaler of size 4 bytes
    * Writes a 32 bit integer value.
    */
    template<int Min, int Max>
    class IntegerQuantizationMarshaler<Min, Max, 4>
        : public Internal::IntegerQuantizationMarshalerImpl<Min, Max, AZ::u32, 4294967295>
    {
    };

    /**
    * Quantizes an AZ::u32 into 1, 2, 3, 4, or 5 bytes based on highest bit usage.
    * The format of the serialized value is a sequence of 1's specifying the
    * number of bytes trailing the initial byte, followed by a 0, followed by the bits
    * that make up the actual value.
    * For example, the number 98 decimal (one byte) will be encoded as 0|1100010, and
    * the largest number representable by a u32, 4294967295, will be represented as
    * 11110|111 11111111 11111111 11111111 111111000.
    *
    * These are the resulting encoding ranges:
    *   Bytes   Available Bits      Range
    *       1                7        127
    *       2               14       ~16K
    *       3               21        ~2M
    *       4               28      ~256M
    *       5               32        ~4B
    */
    class VlqU32Marshaler
    {
    public:
        AZ_TYPE_INFO(VlqU32Marshaler, "{BD9A38BB-713E-44FD-A517-8B3B782BDAAF}");

        void Marshal(WriteBuffer& wb, AZ::u32 v)
        {
            AZ::u8 data[5];
            if (v < 0x80) // fits in 1 byte
            {
                data[0] = static_cast<AZ::u8>(v);
                wb.WriteRaw(data, 1);
            }
            else if (v < 0x4000) // fits in 2 bytes
            {
                data[0] = static_cast<AZ::u8>(0x80 | (v & 0x3f));
                data[1] = static_cast<AZ::u8>((v & 0x3fc0) >> 6);
                wb.WriteRaw(data, 2);
            }
            else if (v < 0x200000) // fits in 3 bytes
            {
                data[0] = static_cast<AZ::u8>(0xc0 | (v & 0x1f));
                data[1] = static_cast<AZ::u8>((v & 0x1fe0) >> 5);
                data[2] = static_cast<AZ::u8>((v & 0x1fe000) >> 13);
                wb.WriteRaw(data, 3);
            }
            else if (v < 0x10000000) // fits in 4 bytes
            {
                data[0] = static_cast<AZ::u8>(0xe0 | (v & 0xf));
                data[1] = static_cast<AZ::u8>((v & 0xff0) >> 4);
                data[2] = static_cast<AZ::u8>((v & 0xff000) >> 12);
                data[3] = static_cast<AZ::u8>((v & 0xff00000) >> 20);
                wb.WriteRaw(data, 4);
            }
            else // needs 5 bytes
            {
                data[0] = static_cast<AZ::u8>(0xf0 | (v & 0x7));
                data[1] = static_cast<AZ::u8>((v & 0x7f8) >> 3);
                data[2] = static_cast<AZ::u8>((v & 0x7f800) >> 11);
                data[3] = static_cast<AZ::u8>((v & 0x7f80000) >> 19);
                data[4] = static_cast<AZ::u8>((v & 0xf8000000) >> 27);
                wb.WriteRaw(data, 5);
            }
        }

        void Unmarshal(AZ::u32& v, ReadBuffer& rb)
        {
            v = 0;
            AZ::u8 data[5];
            rb.ReadRaw(data, 1);
            if (data[0] < 0x80) // 1 byte
            {
                v = data[0];
            }
            else if (data[0] < 0xc0) // 2 bytes
            {
                rb.ReadRaw(data + 1, 1);
                v = (data[0] & ~(0xc0)) | (data[1] << 6);
            }
            else if (data[0] < 0xe0) // 3 bytes
            {
                rb.ReadRaw(data + 1, 2);
                v = (data[0] & ~(0xe0)) | (data[1] << 5) | (data[2] << 13);
            }
            else if (data[0] < 0xf0) // 4 bytes
            {
                rb.ReadRaw(data + 1, 3);
                v = (data[0] & ~(0xf0)) | (data[1] << 4) | (data[2] << 12) | (data[3] << 20);
            }
            else // 5 bytes
            {
                rb.ReadRaw(data + 1, 4);
                v = (data[0] & ~(0xf8)) | (data[1] << 3) | (data[2] << 11) | (data[3] << 19) | (data[4] << 27);
            }
        }
    };

    /**
    * Quantizes an AZ::u64 into 1, 2, 3, 4, 5, 7, 8, or 9 bytes based on highest bit usage.
    * The format of the serialized value is a sequence of 1's specifying the
    * number of bytes trailing the initial byte, followed by a 0, followed by the bits
    * that make up the actual value.
    * For example, the number 98 decimal (one byte) will be encoded as 0|1100010
    *
    * In the case of 9 byte encoding, the first byte is 0xFF and indicates 9 byte encoding
    *     (without a zero to follow for optimization based on assumption that 64bit is the largest integer supported)
    * where as in 8 byte encoding, the first byte is 0xFE.
    */
    class VlqU64Marshaler
    {
    public:
        AZ_TYPE_INFO(VlqU64Marshaler, "{F1141AF7-499D-4A75-A35E-8325B2EB182B}");

        static const AZ::u32 MaxEncodingBytes = 9;

        /*
         * Used as a mask to grab the next byte out of an integer
         */
        template <AZ::u32 bits_shift>
        struct GetMask
        {
            static const AZ::u64 value = (0xffULL << bits_shift);
        };

        /*
         * Using template to make sure constexpr GetMask() is evaluated at compile time
         */
        template<AZ::u32 after_bits>
        static AZ::u8 GetByteAfterBits(AZ::u64 v)
        {
            return static_cast<AZ::u8>((v & GetMask<after_bits>::value) >> after_bits);
        }

        void Marshal(WriteBuffer& wb, AZ::u64 v)
        {
            AZ::u8 data[9];
            if (v < 0x80 /*0b1000'0000*/) // fits in 1 byte
            {
                data[0] = static_cast<AZ::u8>(v);
                wb.WriteRaw(data, 1);
            }
            else if (v < 0x4000) // fits in 2 bytes
            {
                data[0] = static_cast<AZ::u8>(0x80 /*0b1000'0000*/ | (v & 0x3f /*0b0011'1111*/));
                data[1] = GetByteAfterBits<6>(v);
                wb.WriteRaw(data, 2);
            }
            else if (v < 0x200000) // fits in 3 bytes
            {
                data[0] = static_cast<AZ::u8>(0xc0 /*0b1100'0000*/ | (v & 0x1f /*0b0001'1111*/));
                data[1] = GetByteAfterBits<5>(v);
                data[2] = GetByteAfterBits<13>(v);
                wb.WriteRaw(data, 3);
            }
            else if (v < 0x10000000) // fits in 4 bytes
            {
                data[0] = static_cast<AZ::u8>(0xe0 /*0b1110'0000*/ | (v & 0xf /*0b1111*/));
                data[1] = GetByteAfterBits<4>(v);
                data[2] = GetByteAfterBits<12>(v);
                data[3] = GetByteAfterBits<20>(v);
                wb.WriteRaw(data, 4);
            }
            else if (v < 0x0800000000) // needs 5 bytes
            {
                data[0] = static_cast<AZ::u8>(0xf0 | (v & 0x7));
                data[1] = GetByteAfterBits<3>(v);
                data[2] = GetByteAfterBits<11>(v);
                data[3] = GetByteAfterBits<19>(v);
                data[4] = GetByteAfterBits<27>(v);
                wb.WriteRaw(data, 5);
            }
            else if (v < 0x040000000000) // needs 6 bytes
            {
                data[0] = static_cast<AZ::u8>(0xF8 /*0b1111'1000*/ | (v & 0x3 /*0b11*/));
                data[1] = GetByteAfterBits<2>(v);
                data[2] = GetByteAfterBits<10>(v);
                data[3] = GetByteAfterBits<18>(v);
                data[4] = GetByteAfterBits<26>(v);
                data[5] = GetByteAfterBits<34>(v);
                wb.WriteRaw(data, 6);
            }
            else if (v < 0x02000000000000) // needs 7 bytes
            {
                data[0] = static_cast<AZ::u8>(0xFC /*0b1111'1100*/ | (v & 0x1));
                data[1] = GetByteAfterBits<1>(v);
                data[2] = GetByteAfterBits<9>(v);
                data[3] = GetByteAfterBits<17>(v);
                data[4] = GetByteAfterBits<25>(v);
                data[5] = GetByteAfterBits<33>(v);
                data[6] = GetByteAfterBits<41>(v);
                wb.WriteRaw(data, 7);
            }
            else if (v < 0x0100000000000000) // needs 8 bytes
            {
                data[0] = static_cast<AZ::u8>(0xFE /*0b1111'1110*/);
                data[1] = GetByteAfterBits<0>(v);
                data[2] = GetByteAfterBits<8>(v);
                data[3] = GetByteAfterBits<16>(v);
                data[4] = GetByteAfterBits<24>(v);
                data[5] = GetByteAfterBits<32>(v);
                data[6] = GetByteAfterBits<40>(v);
                data[7] = GetByteAfterBits<48>(v);
                wb.WriteRaw(data, 8);
            }
            else // needs 9 bytes, worst case
            {
                data[0] = static_cast<AZ::u8>(0xFF);
                /*
                * Special case here: assume that we don't support larger integers than 64bit.
                * Thus, the encoding of 9 bytes is identical to 8 bytes except the first byte is different.
                * So 0xFF indicates 9 byte encoding, where as 0xFE indicates 8 byte encoding.
                */
                data[1] = GetByteAfterBits<0>(v);
                data[2] = GetByteAfterBits<8>(v);
                data[3] = GetByteAfterBits<16>(v);
                data[4] = GetByteAfterBits<24>(v);
                data[5] = GetByteAfterBits<32>(v);
                data[6] = GetByteAfterBits<40>(v);
                data[7] = GetByteAfterBits<48>(v);
                data[8] = GetByteAfterBits<56>(v);
                wb.WriteRaw(data, 9);
            }
        }

        void Unmarshal(AZ::u64& v, ReadBuffer& rb)
        {
            v = 0;
            AZ::u8 data[9];
            rb.ReadRaw(data, 1);
            if (data[0] < 0x80) // 1 byte
            {
                v = data[0];
            }
            else if (data[0] < 0xc0) // 2 bytes
            {
                rb.ReadRaw(data + 1, 1);
                v = (data[0] & ~(0xc0)) | (data[1] << 6);
            }
            else if (data[0] < 0xe0) // 3 bytes
            {
                rb.ReadRaw(data + 1, 2);
                v = (data[0] & ~(0xe0)) | (data[1] << 5) | (data[2] << 13);
            }
            else if (data[0] < 0xf0) // 4 bytes
            {
                rb.ReadRaw(data + 1, 3);
                v = (data[0] & ~(0xf0)) | (data[1] << 4) | (data[2] << 12) | (data[3] << 20);
            }
            else if (data[0] < 0xF8 /*0b1111'1000*/) // 5 bytes
            {
                rb.ReadRaw(data + 1, 4);
                v = (data[0] & ~(0xf8)) | (data[1] << 3) | (data[2] << 11) | (data[3] << 19) | (static_cast<AZ::u64>(data[4]) << 27);
            }
            else if (data[0] < 0xFC /*0b1111'1100*/) // 6 bytes
            {
                rb.ReadRaw(data + 1, 5);
                v = (data[0] & ~(0xFC /*0b1111'1100*/)) | (data[1] << 2) | (data[2] << 10) | (data[3] << 18) |
                    /* NOTE: one has to cast to u64 otherwise the shift operation will default to 32bit range */
                    (static_cast<AZ::u64>(data[4]) << 26) | (static_cast<AZ::u64>(data[5]) << 34);
            }
            else if (data[0] < 0xFE /*0b1111'1110*/) // 7 bytes
            {
                rb.ReadRaw(data + 1, 6);
                v = (data[0] & ~(0xFE /*0b1111'1110*/)) | (data[1] << 1) | (data[2] << 9) | (data[3] << 17) |
                    /* NOTE: one has to cast to u64 otherwise the shift operation will default to 32bit range */
                    (static_cast<AZ::u64>(data[4]) << 25) | (static_cast<AZ::u64>(data[5]) << 33) | (static_cast<AZ::u64>(data[6]) << 41);
            }
            else if (data[0] < 0xFF /*0b1111'1111*/) // 8 bytes
            {
                rb.ReadRaw(data + 1, 7);
                v = /* first byte contains no data in this case */ data[1] | (data[2] << 8) | (data[3] << 16) |
                    /* NOTE: one has to cast to u64 otherwise the shift operation will default to 32bit range */
                    (static_cast<AZ::u64>(data[4]) << 24) | (static_cast<AZ::u64>(data[5]) << 32) |
                    (static_cast<AZ::u64>(data[6]) << 40) | (static_cast<AZ::u64>(data[7]) << 48);
            }
            else // data[0] == 0xFF /*0b1111'1111*/ // 9 bytes
            {
                rb.ReadRaw(data + 1, 8);
                v = /* first byte contains no data in this case */ data[1] | (data[2] << 8) | (data[3] << 16) |
                    /* NOTE: one has to cast to u64 otherwise the shift operation will default to 32bit range */
                    (static_cast<AZ::u64>(data[4]) << 24) | (static_cast<AZ::u64>(data[5]) << 32) |
                    (static_cast<AZ::u64>(data[6]) << 40) | (static_cast<AZ::u64>(data[7]) << 48) | (static_cast<AZ::u64>(data[8]) << 56);
            }
        }
    };

    /*
    * Marshaler for PackedSize objects.
    *
    * Note, that PackedSize::additionalBits only needs 3 bits since it has a range of [0..7].
    */
    template<>
    class Marshaler<PackedSize>
    {
    public:
        void Marshal(WriteBuffer& wb, const PackedSize& value) const
        {
            wb.Write(static_cast<AZ::u32>(value.GetTotalSizeInBits()), VlqU32Marshaler());
        }
        void Unmarshal(PackedSize& value, ReadBuffer& rb) const
        {
            AZ::u32 bits;
            rb.Read(bits, VlqU32Marshaler());
            value = PackedSize(0, bits);
        }
    };
}

#endif //GM_UTILS_COMPRESSION_MARSHAL

#pragma once
