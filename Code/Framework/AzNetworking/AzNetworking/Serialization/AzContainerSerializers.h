/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/Serialization/ISerializer.h>
#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/fixed_unordered_map.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/VectorN.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/MatrixMxN.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Name/NameDictionary.h>
#include <limits>

namespace AzNetworking
{
    // Generic AZ Containers
    template <typename TYPE>
    struct SerializeAzContainer
    {
        static bool Serialize(ISerializer& serializer, TYPE& container)
        {
            using ValueType = typename TYPE::value_type;
            constexpr uint32_t max = std::numeric_limits<uint32_t>::max(); // Limit to uint32 max elements
            const bool write = (serializer.GetSerializerMode() == SerializerMode::WriteToObject);

            uint32_t size = static_cast<uint32_t>(container.size());
            bool success = serializer.Serialize(size, "Size");

            // Dynamic containers require different read/write serialization interfaces
            if (write)
            {
                container.clear();
                AzNetworking::AzContainerHelper::ReserveContainer<TYPE>(container, size);
                ValueType element;
                for (uint32_t i = 0; i < size; ++i)
                {
                    success &= serializer.Serialize(element, GenerateIndexLabel<max>(i).c_str());
                    container.insert(container.end(), element);
                }
            }
            else
            {
                uint32_t i = 0;
                for (auto it = container.begin(); it != container.end(); ++it, ++i)
                {
                    success &= serializer.Serialize(*it, GenerateIndexLabel<max>(i).c_str());
                }
            }
            return success;
        }
    };

    // Fixed size array
    template <typename TYPE, AZStd::size_t Size>
    struct SerializeAzContainer<AZStd::array<TYPE, Size>>
    {
        static bool Serialize(ISerializer& serializer, AZStd::array<TYPE, Size>& container)
        {
            constexpr uint32_t max = static_cast<uint32_t>(Size);
            static_assert(Size <= max, "Array size must be less than max.\n");
            bool success = true;

            int i = 0;
            for (auto &elem : container)
            {
                success &= serializer.Serialize(elem, GenerateIndexLabel<max>(i++).c_str());
            }
            return success;
        }
    };

    // Fixed unordered map
    template <typename Key, typename MappedType, AZStd::size_t FixedNumBuckets, AZStd::size_t FixedNumElements, class Hasher, class EqualKey>
    struct SerializeAzContainer<AZStd::fixed_unordered_map<Key, MappedType, FixedNumBuckets, FixedNumElements, Hasher, EqualKey>>
    {
        using TYPE = AZStd::fixed_unordered_map<Key, MappedType, FixedNumBuckets, FixedNumElements, Hasher, EqualKey>;
        static bool Serialize(ISerializer& serializer, TYPE& container)
        {
            using SizeType = typename TYPE::size_type;
            using ValueType = typename TYPE::value_type;
            static_assert(FixedNumElements >= FixedNumBuckets, "fixed_unordered_map buckets is less than elements.");
            constexpr uint32_t max = static_cast<uint32_t>(FixedNumElements); // Elements is > Buckets
            const bool write = (serializer.GetSerializerMode() == SerializerMode::WriteToObject);

            SizeType size = container.size();
            bool success = serializer.Serialize(size, "Size");

            if (write)
            {
                container.clear();
                ValueType element;
                for (uint32_t i = 0; i < size; ++i)
                {
                    success &= serializer.Serialize(element, GenerateIndexLabel<max>(i).c_str());
                    container.insert(element);

                }
            }
            else
            {
                uint32_t i = 0;
                for (auto it = container.begin(); it != container.end(); ++it, ++i)
                {
                    success &= serializer.Serialize(*it, GenerateIndexLabel<max>(i).c_str());
                }
            }
            return success;
        }
    };

    // Fixed unordered multimap
    template <typename Key, typename MappedType, AZStd::size_t FixedNumBuckets, AZStd::size_t FixedNumElements, class Hasher, class EqualKey>
    struct SerializeAzContainer<AZStd::fixed_unordered_multimap<Key, MappedType, FixedNumBuckets, FixedNumElements, Hasher, EqualKey>>
    {
        using TYPE = AZStd::fixed_unordered_multimap<Key, MappedType, FixedNumBuckets, FixedNumElements, Hasher, EqualKey>;
        static bool Serialize(ISerializer& serializer, TYPE& container)
        {
            using SizeType = typename TYPE::size_type;
            using ValueType = typename TYPE::value_type;
            static_assert(FixedNumElements >= FixedNumBuckets, "fixed_unordered_multimap buckets is less than elements.");
            constexpr uint32_t max = static_cast<uint32_t>(FixedNumElements); // Elements is > Buckets
            const bool write = (serializer.GetSerializerMode() == SerializerMode::WriteToObject);

            SizeType size = container.size();
            bool success = serializer.Serialize(size, "Size");

            if (write)
            {
                container.clear();
                ValueType element;
                for (uint32_t i = 0; i < size; ++i)
                {
                    success &= serializer.Serialize(element, GenerateIndexLabel<max>(i).c_str());
                    container.insert(element);

                }
            }
            else
            {
                uint32_t i = 0;
                for (auto it = container.begin(); it != container.end(); ++it, ++i)
                {
                    success &= serializer.Serialize(*it, GenerateIndexLabel<max>(i).c_str());
                }
            }
            return success;
        }
    };

    // Multimap
    template <class Key, class MappedType, class Compare, class Allocator>
    struct SerializeAzContainer<AZStd::multimap<Key, MappedType, Compare, Allocator>>
    {
        using TYPE = AZStd::multimap<Key, MappedType, Compare, Allocator>;
        static bool Serialize(ISerializer& serializer, TYPE& container)
        {
            using SizeType = typename TYPE::size_type;
            using ValueType = typename TYPE::value_type;
            constexpr uint32_t max = std::numeric_limits<uint32_t>::max(); // Limit to uint32 max elements
            const bool write = (serializer.GetSerializerMode() == SerializerMode::WriteToObject);

            SizeType size = static_cast<uint32_t>(container.size());
            bool success = serializer.Serialize(size, "Size");

            if (write)
            {
                container.clear();
                ValueType element;
                for (uint32_t i = 0; i < size; ++i)
                {
                    success &= serializer.Serialize(element, GenerateIndexLabel<max>(i).c_str());
                    container.insert(element);
                }
            }
            else
            {
                uint32_t i = 0;
                for (auto it = container.begin(); it != container.end(); ++it, ++i)
                {
                    success &= serializer.Serialize(*it, GenerateIndexLabel<max>(i).c_str());
                }
            }
            return success;
        }
    };

    
    // Pair
    template<class KeyType, class ValueType>
    struct SerializeObjectHelper<AZStd::pair<KeyType, ValueType>>
    {
        static bool SerializeObject(ISerializer& serializer, AZStd::pair<KeyType, ValueType>& value)
        {
            bool result = serializer.Serialize(value.first, "key") && serializer.Serialize(value.second, "value");
            return result;
        }
    };

    // String
    template<>
    struct SerializeAzContainer<AZStd::string>
    {
        static bool Serialize(ISerializer& serializer, AZStd::string& value)
        {
            uint32_t size = aznumeric_cast<uint32_t>(value.length());
            uint32_t outBytes = size;

            bool success = serializer.Serialize(size, "Size");
            value.resize_no_construct(size);
            success &= serializer.SerializeBytes(reinterpret_cast<uint8_t*>(value.data()), size, true, outBytes, "String");

            return success && outBytes == size;
        }
    };

    // Fixed string
    template <AZStd::size_t MaxElementCount>
    struct SerializeAzContainer<AZStd::fixed_string<MaxElementCount>>
    {
        static bool Serialize(ISerializer& serializer, AZStd::fixed_string<MaxElementCount>& value)
        {
            using SizeType = typename AZ::SizeType<AZ::RequiredBytesForValue<MaxElementCount>(), false>::Type;
            SizeType size = aznumeric_cast<SizeType>(value.length());
            uint32_t outBytes = static_cast<uint32_t>(size);

            bool success = serializer.Serialize(size, "Size");
            value.resize_no_construct(size);
            success &= serializer.SerializeBytes(reinterpret_cast<uint8_t*>(value.data()), static_cast<uint32_t>(size), true, outBytes, "String");

            return success && outBytes == size;
        }
    };

    // Az Containers
    template <typename TYPE>
    struct SerializeObjectHelper<TYPE, AZStd::enable_if_t<AzContainerHelper::IsIterableContainer<TYPE>::Value>>
    {
        static bool SerializeObject(ISerializer& serializer, TYPE& container)
        {
            return SerializeAzContainer<TYPE>::Serialize(serializer, container);
        }
    };

    // Az Types
    template <>
    struct SerializeObjectHelper<AZ::Vector2>
    {
        static bool SerializeObject(ISerializer& serializer, AZ::Vector2& value)
        {
            float values[4];
            value.StoreToFloat2(values);
            serializer.Serialize(values[0], "xValue");
            serializer.Serialize(values[1], "yValue");
            value = AZ::Vector2::CreateFromFloat2(values);
            return serializer.IsValid();
        }
    };

    template <>
    struct SerializeObjectHelper<AZ::Vector3>
    {
        static bool SerializeObject(ISerializer& serializer, AZ::Vector3& value)
        {
            float values[4];
            value.StoreToFloat3(values);
            serializer.Serialize(values[0], "xValue");
            serializer.Serialize(values[1], "yValue");
            serializer.Serialize(values[2], "zValue");
            value = AZ::Vector3::CreateFromFloat3(values);
            return serializer.IsValid();
        }
    };

    template <>
    struct SerializeObjectHelper<AZ::Vector4>
    {
        static bool SerializeObject(ISerializer& serializer, AZ::Vector4& value)
        {
            float values[4];
            value.StoreToFloat4(values);
            serializer.Serialize(values[0], "xValue");
            serializer.Serialize(values[1], "yValue");
            serializer.Serialize(values[2], "zValue");
            serializer.Serialize(values[3], "wValue");
            value = AZ::Vector4::CreateFromFloat4(values);
            return serializer.IsValid();
        }
    };

    template <>
    struct SerializeObjectHelper<AZ::VectorN>
    {
        static bool SerializeObject(ISerializer& serializer, AZ::VectorN& value)
        {
            AZStd::size_t size = value.GetDimensionality();
            if (serializer.Serialize(size, "size"))
            {
                value.Resize(size);
            }
            return serializer.Serialize(value.GetVectorValues(), "data");
        }
    };

    template <>
    struct SerializeObjectHelper<AZ::Matrix3x3>
    {
        static bool SerializeObject(ISerializer& serializer, AZ::Matrix3x3& value)
        {
            float values[9];
            value.StoreToRowMajorFloat9(values);
            serializer.Serialize(values[0], "0");
            serializer.Serialize(values[1], "1");
            serializer.Serialize(values[2], "2");
            serializer.Serialize(values[3], "3");
            serializer.Serialize(values[4], "4");
            serializer.Serialize(values[5], "5");
            serializer.Serialize(values[6], "6");
            serializer.Serialize(values[7], "7");
            serializer.Serialize(values[8], "8");
            value = AZ::Matrix3x3::CreateFromRowMajorFloat9(values);
            return serializer.IsValid();
        }
    };

    template <>
    struct SerializeObjectHelper<AZ::Matrix3x4>
    {
        static bool SerializeObject(ISerializer& serializer, AZ::Matrix3x4& value)
        {
            float values[12];
            value.StoreToRowMajorFloat12(values);
            serializer.Serialize(values[ 0], "0");
            serializer.Serialize(values[ 1], "1");
            serializer.Serialize(values[ 2], "2");
            serializer.Serialize(values[ 3], "3");
            serializer.Serialize(values[ 4], "4");
            serializer.Serialize(values[ 5], "5");
            serializer.Serialize(values[ 6], "6");
            serializer.Serialize(values[ 7], "7");
            serializer.Serialize(values[ 8], "8");
            serializer.Serialize(values[ 9], "9");
            serializer.Serialize(values[10], "A");
            serializer.Serialize(values[11], "B");
            value = AZ::Matrix3x4::CreateFromRowMajorFloat12(values);
            return serializer.IsValid();
        }
    };

    template <>
    struct SerializeObjectHelper<AZ::Matrix4x4>
    {
        static bool SerializeObject(ISerializer& serializer, AZ::Matrix4x4& value)
        {
            float values[16];
            value.StoreToRowMajorFloat16(values);
            serializer.Serialize(values[ 0], "0");
            serializer.Serialize(values[ 1], "1");
            serializer.Serialize(values[ 2], "2");
            serializer.Serialize(values[ 3], "3");
            serializer.Serialize(values[ 4], "4");
            serializer.Serialize(values[ 5], "5");
            serializer.Serialize(values[ 6], "6");
            serializer.Serialize(values[ 7], "7");
            serializer.Serialize(values[ 8], "8");
            serializer.Serialize(values[ 9], "9");
            serializer.Serialize(values[10], "A");
            serializer.Serialize(values[11], "B");
            serializer.Serialize(values[12], "C");
            serializer.Serialize(values[13], "D");
            serializer.Serialize(values[14], "E");
            serializer.Serialize(values[15], "F");
            value = AZ::Matrix4x4::CreateFromRowMajorFloat16(values);
            return serializer.IsValid();
        }
    };

    template <>
    struct SerializeObjectHelper<AZ::MatrixMxN>
    {
        static bool SerializeObject(ISerializer& serializer, AZ::MatrixMxN& value)
        {
            AZStd::size_t rows = value.GetRowCount();
            AZStd::size_t cols = value.GetColumnCount();
            if (serializer.Serialize(rows, "rows") && serializer.Serialize(cols, "cols"))
            {
                value.Resize(rows, cols);
            }
            return serializer.Serialize(value.GetMatrixElements(), "data");
        }
    };

    template <>
    struct SerializeObjectHelper<AZ::Quaternion>
    {
        static bool SerializeObject(ISerializer& serializer, AZ::Quaternion& value)
        {
            float values[4];
            value.StoreToFloat4(values);
            serializer.Serialize(values[0], "xValue");
            serializer.Serialize(values[1], "yValue");
            serializer.Serialize(values[2], "zValue");
            serializer.Serialize(values[3], "wValue");
            value = AZ::Quaternion::CreateFromFloat4(values);
            return serializer.IsValid();
        }
    };

    template <>
    struct SerializeObjectHelper<AZ::Transform>
    {
        static bool SerializeObject(ISerializer& serializer, AZ::Transform& value)
        {
            AZ::Vector3 translation = value.GetTranslation();
            AZ::Quaternion rotation = value.GetRotation();
            float uniformScale = value.GetUniformScale();
            serializer.Serialize(translation, "Translation");
            serializer.Serialize(rotation, "Rotation");
            serializer.Serialize(uniformScale, "Scale");
            value.SetTranslation(translation);
            value.SetRotation(rotation);
            value.SetUniformScale(uniformScale);
            return serializer.IsValid();
        }
    };

    template <>
    struct SerializeObjectHelper<AZ::Aabb>
    {
        static bool SerializeObject(ISerializer& serializer, AZ::Aabb& value)
        {
            AZ::Vector3 minValue = value.GetMin();
            AZ::Vector3 maxValue = value.GetMax();
            serializer.Serialize(minValue, "minValue");
            serializer.Serialize(maxValue, "maxValue");
            value.SetMin(minValue);
            value.SetMax(maxValue);
            return serializer.IsValid();
        }
    };

    template <>
    struct SerializeObjectHelper<AZ::Name>
    {
        static bool SerializeObject(ISerializer& serializer, AZ::Name& value)
        {
            AZ::Name::Hash nameHash = value.GetHash();
            bool result = serializer.Serialize(nameHash, "NameHash");

            if (result && serializer.GetSerializerMode() == SerializerMode::WriteToObject)
            {
                value = AZ::NameDictionary::Instance().FindName(nameHash);
            }
            return result;
        }
    };
}
