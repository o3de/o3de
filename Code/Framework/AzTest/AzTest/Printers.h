/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <iosfwd>
#include <AzCore/IO/Path/Path_fwd.h>

namespace AZStd
{
    template<class Element, class Traits, class Allocator>
    class basic_string;

    template <class Element, class Traits>
    class basic_string_view;

    template <class Element, size_t MaxElementCount, class Traits>
    class basic_fixed_string;

    template<class Element, class Traits, class Allocator>
    void PrintTo(const AZStd::basic_string<Element, Traits, Allocator>& value, ::std::ostream* os);
    template<class Element, class Traits>
    void PrintTo(const AZStd::basic_string_view<Element, Traits>& value, ::std::ostream* os);
    template <class Element, size_t MaxElementCount, class Traits>
    void PrintTo(const AZStd::basic_fixed_string<Element, MaxElementCount, Traits>& value, ::std::ostream* os);
}

namespace AZ::IO
{
    // Add Googletest printers for the AZ::IO::Path classes
    void PrintTo(const AZ::IO::PathView& path, ::std::ostream* os);
    void PrintTo(const AZ::IO::Path& path, ::std::ostream* os);
    void PrintTo(const AZ::IO::FixedMaxPath& path, ::std::ostream* os);
}

namespace AZ
{
    class EntityId;
    struct Uuid;

    void PrintTo(AZ::EntityId entityId, ::std::ostream* os);

    void PrintTo(AZ::Uuid& uuid, ::std::ostream* os);
}

#include <AzTest/Printers.inl>
