/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_UTILS_CONTAINER_MARSHAL
#define GM_UTILS_CONTAINER_MARSHAL

#include <GridMate/GridMate.h>
#include <GridMate/Serialize/Buffer.h>
#include <GridMate/Serialize/MarshalerTypes.h>

#include <AzCore/std/containers/bitset.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/typetraits/underlying_type.h>

namespace GridMate
{
    template<typename C>
    struct IsContainerMarshaler
    {
        template<class T>
        static AZStd::false_type Evaluate(...);
        template<class T>
        static AZStd::true_type Evaluate(int,
            typename T::value_type = typename T::value_type(),
            typename T::const_iterator = C().begin(),
            typename T::const_iterator = C().end());

        enum
        {
            Value = AZStd::is_same<decltype(Evaluate<C>(0)), AZStd::true_type>::value
        };
    };
    /**
    Writes a container to the stream. This includes string, vector, map, set, list,
    both ordered and unordered versions. Assumes the value and key (if exists) has
    an appropriate marshaler defined.
    */
    template<typename Type>
    class Marshaler<Type, typename AZStd::Utils::enable_if_c<IsContainerMarshaler<Type>::Value>::type>
    {
    public:
        AZ_TYPE_INFO_LEGACY(Marshaler, "{93F8FF8B-6437-448B-A231-BDB3BB1448F2}", Type);

        typedef Type DataType;
        typedef typename Type::value_type ValueType;
        typedef Marshaler<ValueType> InnerMarshaler;

        InnerMarshaler m_marshaler;

        AZ_FORCE_INLINE void Marshal(WriteBuffer& wb, const DataType& value) const
        {
            AZ_Assert(value.size() < USHRT_MAX, "Container has too many elements for marshaling!");
            AZ::u16 size = static_cast<AZ::u16>(value.size());
            wb.Write(size);
            for (const auto& i : value)
            {
                wb.Write(i, m_marshaler);
            }
        }
        AZ_FORCE_INLINE void Unmarshal(DataType& value, ReadBuffer& rb) const
        {
            AZ::u16 size;
            if (rb.Read(size))
            {
                value.clear();
                ReserveContainer<Type>(value, size);
                while (size--)
                {
                    ValueType element;
                    if (rb.Read(element, m_marshaler))
                    {
                        value.insert(value.end(), element);
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

        template<typename T>
        struct HasReserveMethod
        {
            template<typename U>
            static decltype(U().reserve())Evaluate(int);
            template<typename U>
            static AZStd::false_type Evaluate(...);
            static const bool value = !AZStd::is_same<AZStd::false_type, decltype(Evaluate<T>(0))>::value;
        };

        template<typename T>
        typename AZStd::Utils::enable_if_c<HasReserveMethod<T>::value>::type ReserveContainer(T& value, typename T::size_type size) const
        {
            value.reserve(size);
        }

        template<typename T>
        typename AZStd::Utils::enable_if_c<!HasReserveMethod<T>::value>::type ReserveContainer(T&, typename T::size_type) const
        { }
    };

    /**
    Explicit container marshaler. Use this when you want the value type in a container to
    be serialized with a non-default marshaler. Compatible with array, vector, set and list.
    */
    template<class Container, class DataMarshaler = Marshaler<typename Container::value_type> >
    class ContainerMarshaler
    {
    public:
        typedef Container DataType;

        ContainerMarshaler(DataMarshaler marshaler = DataMarshaler())
            : m_marshaler(marshaler) { }

        AZ_FORCE_INLINE void Marshal(WriteBuffer& wb, const Container& container) const
        {
            AZ_Assert(container.size() < USHRT_MAX, "Container has too many elements for marshaling!");
            AZ::u16 size = static_cast<AZ::u16>(container.size());
            wb.Write(size);
            for (const auto& i : container)
            {
                m_marshaler.Marshal(wb, i);
            }
        }

        AZ_FORCE_INLINE void Unmarshal(Container& container, ReadBuffer& rb) const
        {
            container.clear();

            AZ::u16 size;
            rb.Read(size);
            for (AZ::u16 i = 0; i < size; ++i)
            {
                typename Container::value_type element;
                m_marshaler.Unmarshal(element, rb);
                container.insert(container.end(), element);
            }
        }
    protected:
        DataMarshaler m_marshaler;
    };

    /**
    Explicit key/value container marshaler. Use this when you want the value type in a container to
    be serialized with a non-default marshaler. Compatible with ordered and unordered map and multimap.
    */
    template<class MapContainer, class KeyMarshaler = Marshaler<typename MapContainer::key_type>, class DataMarshaler = Marshaler<typename MapContainer::mapped_type> >
    class MapContainerMarshaler
    {
        KeyMarshaler m_keyMarshaler;
        DataMarshaler m_dataMarshaler;
    public:
        typedef MapContainer DataType;

        MapContainerMarshaler(KeyMarshaler keyMarshaler = KeyMarshaler(), DataMarshaler dataMarshaler = DataMarshaler())
            : m_keyMarshaler(keyMarshaler)
            , m_dataMarshaler(dataMarshaler)
        { }

        AZ_FORCE_INLINE void Marshal(WriteBuffer& wb, const MapContainer& cont) const
        {
            AZ_Assert(cont.size() < USHRT_MAX, "Container has too many elements for marshaling!");
            AZ::u16 size = static_cast<AZ::u16>(cont.size());
            wb.Write(size);
            for (typename MapContainer::const_iterator iter = cont.begin(); iter != cont.end(); ++iter)
            {
                m_keyMarshaler.Marshal(wb, iter->first);
                m_dataMarshaler.Marshal(wb, iter->second);
            }
        }

        AZ_FORCE_INLINE void Unmarshal(MapContainer& cont, ReadBuffer& rb) const
        {
            AZ::u16 size;
            rb.Read(size);
            cont.clear();
            typename MapContainer::key_type key;
            for (AZ::u16 i = 0; i < size; ++i)
            {
                m_keyMarshaler.Unmarshal(key, rb);
                typename MapContainer::iterator iter = cont.insert(key).first;
                m_dataMarshaler.Unmarshal(iter->second, rb);
            }
        }
    };
}
#endif //GM_UTILS_CONTAINER_MARSHAL
#pragma once
