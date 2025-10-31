/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/typetraits/underlying_type.h>
#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzCore/std/typetraits/is_enum.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/RTTI/TypeSafeIntegral.h>

namespace AzNetworking
{
    // Identifies AZStd containers
    struct AzContainerHelper
    {
        template <typename C>
        struct IsIterableContainer
        {
            template <class TYPE>
            static AZStd::false_type Evaluate(...);

            template <class TYPE>
            static AZStd::true_type Evaluate(int,
                typename TYPE::value_type = typename TYPE::value_type(),
                typename TYPE::const_iterator = C().begin(),
                typename TYPE::const_iterator = C().end(),
                typename TYPE::size_type = C().size());

            static constexpr bool Value = AZStd::is_same<decltype(Evaluate<C>(0)), AZStd::true_type>::value;
        };

        template <typename TYPE>
        struct HasReserveMethod
        {
            template <typename U>
            static decltype(U().reserve()) Evaluate(int);

            template <typename U>
            static AZStd::false_type Evaluate(...);

            static constexpr bool value = !AZStd::is_same<AZStd::false_type, decltype(Evaluate<TYPE>(0))>::value;
        };

        template <typename TYPE>
        static AZStd::enable_if_t<HasReserveMethod<TYPE>::value> ReserveContainer(TYPE& value, typename TYPE::size_type size)
        {
            value.reserve(size);
        }

        template<typename TYPE>
        static AZStd::enable_if_t<!HasReserveMethod<TYPE>::value> ReserveContainer(TYPE&, typename TYPE::size_type)
        {
            ;
        }
    };

    template <typename OBJECT_TYPE>
    struct SerializeType
    {
        static bool Serialize(ISerializer& serializer, OBJECT_TYPE& value)
        {
            return value.Serialize(serializer);
        }
    };

    // Base template
    template <typename TYPE, typename = void>
    struct SerializeObjectHelper
    {
        static bool SerializeObject(ISerializer& serializer, TYPE& value)
        {
            return SerializeType<TYPE>::Serialize(serializer, value);
        }
    };

    // Non-containers
    template <typename TYPE>
    struct SerializeObjectHelper<TYPE, AZStd::enable_if_t<!AzContainerHelper::IsIterableContainer<TYPE>::value>>
    {
        static bool SerializeObject(ISerializer& serializer, TYPE& value)
        {
            return SerializeType<TYPE>::Serialize(serializer, value);
        }
    };

    inline bool ISerializer::IsValid() const
    {
        return m_serializerValid;
    }

    inline void ISerializer::Invalidate()
    {
        m_serializerValid = false;
    }

    inline bool ISerializer::Serialize(char& value, const char* name, uint8_t minValue, uint8_t maxValue)
    {
        return Serialize(reinterpret_cast<uint8_t&>(value), name, minValue, maxValue);
    }

    template <typename TYPE>
    inline bool ISerializer::Serialize(TYPE& value, const char* name)
    {
        enum { IsEnum = AZStd::is_enum<TYPE>::value };
        enum { IsTypeSafeIntegral = AZStd::is_type_safe_integral<TYPE>::value };
        return SerializeHelper<IsEnum, IsTypeSafeIntegral>::Serialize(*this, value, name);
    }

    // Helper for objects and structures
    template <>
    struct ISerializer::SerializeHelper<false, false>
    {
        template <typename TYPE>
        static bool Serialize(ISerializer& serializer, TYPE& value, const char* name)
        {
            if (serializer.BeginObject(name))
            {
                if (SerializeObjectHelper<TYPE>::SerializeObject(serializer, value))
                {
                    return serializer.EndObject(name);
                }
            }
            return false;
        }
    };

    // Helper for enums
    template <>
    struct ISerializer::SerializeHelper<true, false>
    {
        template <typename TYPE>
        static bool Serialize(ISerializer& serializer, TYPE& value, const char* name)
        {
            using SizeType = typename AZStd::underlying_type<TYPE>::type;

            SizeType& integralValue = reinterpret_cast<SizeType&>(value);

            if (!serializer.Serialize(integralValue, name))
            {
                return false;
            }

            //auto enumMembers = AzEnumTraits<TYPE>::Members;
            //if (AZStd::find(enumMembers.begin(), enumMembers.end(), static_cast<Type>(integralValue)) == enumMembers.end())
            //{
            //    return false;
            //}

            return true;
        }
    };

    // Helper for type-safe integrals
    template <>
    struct ISerializer::SerializeHelper<true, true>
    {
        template <typename TYPE>
        static bool Serialize(ISerializer& serializer, TYPE& value, const char* name)
        {
            using RawType = typename AZStd::underlying_type<TYPE>::type;

            RawType& rawValue = reinterpret_cast<RawType&>(value);

            if (!serializer.Serialize(rawValue, name))
            {
                return false;
            }

            return true;
        }
    };
}

#include <AzNetworking/Serialization/AzContainerSerializers.h>
