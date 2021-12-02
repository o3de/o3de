/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <cstddef>

namespace AZStd
{
    class allocator;

    template <class Element>
    struct char_traits;

    template <class Element, class Traits, class Allocator>
    class basic_string;
    
    template <class Element, size_t MaxElementCount, class Traits>
    class basic_fixed_string;

    template <typename T>
    struct hash;
}

namespace AZ
{
    class ReflectContext;
}

namespace AZ::IO
{
    //! Path Constants
    inline constexpr size_t MaxPathLength = 1024;
    inline constexpr char PosixPathSeparator = '/';
    inline constexpr char WindowsPathSeparator = '\\';

    //! Path aliases
    using FixedMaxPathString = AZStd::basic_fixed_string<char, MaxPathLength, AZStd::char_traits<char>>;

    //! Forward declarations for the PathView, Path and FixedMaxPath classes
    class PathView;

    template <typename StringType>
    class BasicPath;

    //! Only the following path types are supported
    //! The BasicPath template above is shared only amoung the following instantiations
    using Path = BasicPath<AZStd::basic_string<char, AZStd::char_traits<char>, AZStd::allocator>>;
    using FixedMaxPath = BasicPath<FixedMaxPathString>;

    // Forward declare the PathIterator type.
    // It depends on the path type
    template <typename PathType>
    class PathIterator;
}

namespace AZStd
{
    template <>
    struct hash<AZ::IO::PathView>;

    template <typename StringType>
    struct hash<AZ::IO::BasicPath<StringType>>;
}
