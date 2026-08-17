/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "MetaUtils.h"
#include "ReflectableEnums.h"

namespace AZ
{
    // GenratedEnum are enums made with the MAKE_REFLECTABLE_ENUM macro

    // get a std::make_tuple(Enumerator1, Enumerator2...);
    template<typename GeneratedEnum>
    constexpr auto enumeratorsAsTuple_v = valueListAsTuple_v< typename GeneratedEnum::MetaValueList >;

    // get a tuple< T::EnumType, T::EnumType...>
    template<typename GeneratedEnum>
    using EnumeratorsAsTuple_t = ValueListAsTuple_t< typename GeneratedEnum::MetaValueList >;

    template<typename GeneratedEnum>
    constexpr auto enumeratorsCount_v = countTemplateParameters_v< typename GeneratedEnum::MetaValueList >;
}

#ifndef NDEBUG
#include <cassert>

namespace AZ::Tests
{
    static_assert(at_v<1, MyEnum::MetaValueList> == MyEnum::Enumerand2);
    static_assert(at_v<2, MyEnum::MetaValueList> == MyEnum::Enumerand3);
    static_assert(at_v<3, MyEnum::MetaValueList> == MyEnum::EndEnumeratorSentinel_);

    static_assert(at_v<0, MyEnumFlaggable::MetaValueList> == MyEnumFlaggable::Flag1);
    static_assert(at_v<1, MyEnumFlaggable::MetaValueList> == MyEnumFlaggable::Flag2);
    static_assert(at_v<3, MyEnumFlaggable::MetaValueList> == MyEnumFlaggable::EndEnumeratorSentinel_);
}
#endif
