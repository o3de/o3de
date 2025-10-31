/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ::RHI
{
    struct DefaultNamespaceType {
        AZ_TYPE_INFO(DefaultNamespaceType, "{51372f60-2387-4d98-a66c-e6f0d6881087}");
    };

    //! Handle a simple wrapper around an integral type, which adds the formal concept of a 'Null' value. It
    //! is designed to accommodate a zero-based 'index' where a value of 0 is considered valid. As such, the null value
    //! is equal to -1 casted to the type.
    //!
    //! @tparam T
    //!  An integral type held by the Handle container. A value of -1 (or max value for unsigned types) is reserved for
    //!  the null index.
    //!
    //! @tparam NamespaceType
    //!  An optional typename used to create a compile-time unique variant of Handle. This disallows trivial
    //!  copying of unrelated 'types'. Useful to make a handle variant typed to a client class.
    //!
    //! Sample Usage:
    //! @code{.cpp}
    //!     class Foo;
    //!     using FooHandle = Handle<uint16_t, Foo>;
    //!     FooHandle fooHandle;
    //!
    //!     class Bar;
    //!     using BarHandle = Handle<uint16_t, Bar>;
    //!     BarHandle barHandle;
    //!
    //!     fooHandle = barHandle; // Error! Different types!
    //!     fooHandle.IsNull();    // true
    //!     fooHandle.GetIndex();  // FooHandle::NullIndex
    //!
    //!     fooHandle = 15;
    //!     fooHandle.GetIndex();  // 15
    //!     fooHandle.IsNull();    // false
    //! @endcode
    template <typename T = uint32_t, typename NamespaceType = DefaultNamespaceType>
    struct Handle
    {
        using IndexType = T;
        static_assert(AZStd::is_integral<T>::value, "Integral type required for Handle<>.");

        static void Reflect(AZ::ReflectContext* context);

        static const constexpr T NullIndex = T(-1);
        struct NullType {};
        static constexpr NullType Null{};
        constexpr Handle(NullType) {};

        constexpr Handle() = default;
        constexpr explicit Handle(T index) : m_index{index} {}

        template <typename U>
        constexpr explicit Handle(U index) : m_index{aznumeric_caster(index)} {}

        constexpr bool operator == (const Handle& rhs) const;
        constexpr bool operator != (const Handle& rhs) const;
        constexpr bool operator < (const Handle& rhs) const;
        constexpr bool operator > (const Handle& rhs) const;
        constexpr bool operator <= (const Handle& rhs) const;

        /// Resets the handle to NullIndex.
        void Reset();

        /// Returns the index currently stored in the handle.
        constexpr T GetIndex() const;

        /// Returns whether the handle is equal to NullIndex.
        constexpr bool IsNull() const;

        /// Returns whether the handle is NOT equal to NullIndex.
        constexpr bool IsValid() const;

        T m_index = NullIndex;
    };

    template <typename T, typename NamespaceType>
    void Handle<T, NamespaceType>::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<Handle<T, NamespaceType>>()
                ->Version(1)
                ->Field("m_index", &Handle<T, NamespaceType>::m_index);
        }

        if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->Class<Handle<T, NamespaceType>>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "RHI")
                ->Attribute(AZ::Script::Attributes::Module, "rhi")
                ->Method("IsValid", &Handle<T, NamespaceType>::IsValid)
                ->Method("GetIndex", &Handle<T, NamespaceType>::GetIndex)
                ;
        }
    }

    template <typename T, typename NamespaceType>
    constexpr bool Handle<T, NamespaceType>::operator==(const Handle& rhs) const
    {
        return m_index == rhs.m_index;
    }

    template <typename T, typename NamespaceType>
    constexpr bool Handle<T, NamespaceType>::operator!=(const Handle& rhs) const
    {
        return m_index != rhs.m_index;
    }

    template <typename T, typename NamespaceType>
    constexpr bool Handle<T, NamespaceType>::operator<(const Handle& rhs) const
    {
        return m_index < rhs.m_index;
    }

    template <typename T, typename NamespaceType>
    constexpr bool Handle<T, NamespaceType>::operator<=(const Handle& rhs) const
    {
        return m_index <= rhs.m_index;
    }
        
    template <typename T, typename NamespaceType>
    constexpr bool Handle<T, NamespaceType>::operator>(const Handle& rhs) const
    {
        return m_index > rhs.m_index;
    }

    template <typename T, typename NamespaceType>
    void Handle<T, NamespaceType>::Reset()
    {
        m_index = NullIndex;
    }

    template <typename T, typename NamespaceType>
    constexpr T Handle<T, NamespaceType>::GetIndex() const
    {
        return m_index;
    }

    template <typename T, typename NamespaceType>
    constexpr bool Handle<T, NamespaceType>::IsNull() const
    {
        return m_index == NullIndex;
    }

    template <typename T, typename NamespaceType>
    constexpr bool Handle<T, NamespaceType>::IsValid() const
    {
        return m_index != NullIndex;
    }
}

namespace AZ
{
    AZ_TYPE_INFO_TEMPLATE(AZ::RHI::Handle, "{273A36DB-D62B-45EB-9E05-E097EE9743BB}", AZ_TYPE_INFO_TYPENAME, AZ_TYPE_INFO_TYPENAME);
}

namespace AZStd
{
    template<class T>
    struct hash;

    template<typename HandleType, typename NamespaceType>
    struct hash<AZ::RHI::Handle<HandleType, NamespaceType>>
    {
        typedef size_t                          result_type;
        typedef AZ::RHI::Handle<HandleType, NamespaceType> argument;
        using argument_type = typename argument::IndexType;

        result_type operator()(const argument& value) const
        {
            return static_cast<result_type>(*reinterpret_cast<const argument_type*>(&value));
        }
    };
}   // namespace AZStd
