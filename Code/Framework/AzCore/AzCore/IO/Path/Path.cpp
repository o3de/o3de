/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/SerializeContext.h>

// Explicit instantations of our support Path classes
namespace AZ::IO
{
    // Class template instantations
    template class BasicPath<AZStd::string>;
    template class BasicPath<FixedMaxPathString>;
    template class PathIterator<PathView>;
    template class PathIterator<Path>;
    template class PathIterator<FixedMaxPath>;

    // Swap function instantiations
    template void swap<AZStd::string>(Path& lhs, Path& rhs) noexcept;
    template void swap<FixedMaxPathString>(FixedMaxPath& lhs, FixedMaxPath& rhs) noexcept;

    // Hash function instantiations
    template size_t hash_value<AZStd::string>(const Path& pathToHash);
    template size_t hash_value<FixedMaxPathString>(const FixedMaxPath& pathToHash);

    // Append operator instantiations
    template BasicPath<AZStd::string> operator/<AZStd::string>(const BasicPath<AZStd::string>& lhs, const PathView& rhs);
    template BasicPath<FixedMaxPathString> operator/<FixedMaxPathString>(const BasicPath<FixedMaxPathString>& lhs, const PathView& rhs);
    template BasicPath<AZStd::string> operator/<AZStd::string>(const BasicPath<AZStd::string>& lhs, AZStd::string_view rhs);
    template BasicPath<FixedMaxPathString> operator/<FixedMaxPathString>(const BasicPath<FixedMaxPathString>& lhs, AZStd::string_view rhs);
    template BasicPath<AZStd::string> operator/<AZStd::string>(const BasicPath<AZStd::string>& lhs,
        const typename BasicPath<AZStd::string>::value_type* rhs);
    template BasicPath<FixedMaxPathString> operator/<FixedMaxPathString>(const BasicPath<FixedMaxPathString>& lhs,
        const typename BasicPath<FixedMaxPathString>::value_type* rhs);

    // Iterator compare instantiations
    template bool operator==<PathView>(const PathIterator<PathView>& lhs,
        const PathIterator<PathView>& rhs);
    template bool operator==<Path>(const PathIterator<Path>& lhs,
        const PathIterator<Path>& rhs);
    template bool operator==<FixedMaxPath>(const PathIterator<FixedMaxPath>& lhs,
        const PathIterator<FixedMaxPath>& rhs);
    template bool operator!=<PathView>(const PathIterator<PathView>& lhs,
        const PathIterator<PathView>& rhs);
    template bool operator!=<Path>(const PathIterator<Path>& lhs,
        const PathIterator<Path>& rhs);
    template bool operator!=<FixedMaxPath>(const PathIterator<FixedMaxPath>& lhs,
        const PathIterator<FixedMaxPath>& rhs);
}
