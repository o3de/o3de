/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <numeric>
#include <AzCore/base.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/string/string_view.h>

namespace AZ
{
    namespace IO
    {
        //! The path to the file used by a request.
        //! RequestPath uses lazy evaluation so the relative path is
        //! not resolved to an absolute path until the path is requested
        //! or a check is done for validity.
        class RequestPath
        {
        public:
            RequestPath() = default;
            RequestPath(const RequestPath& rhs) = default;
            RequestPath(RequestPath&& rhs) = default;
            // Allow path views to be cast to a request path.
            RequestPath(AZ::IO::PathView path);

            RequestPath& operator=(const RequestPath& rhs) = default;
            RequestPath& operator=(RequestPath&& rhs) = default;
            RequestPath& operator=(AZ::IO::PathView path);

            bool operator==(const RequestPath& rhs) const;
            bool operator!=(const RequestPath& rhs) const;

            bool IsValid() const;
            void Clear();

            const char* GetAbsolutePathCStr() const;
            const char* GetRelativePathCStr() const;
            PathView GetAbsolutePath() const;
            PathView GetRelativePath() const;
            size_t GetHash() const;

        private:
            constexpr static const size_t s_invalidPathHash = std::numeric_limits<size_t>::max();
            constexpr static const size_t s_emptyPathHash = std::numeric_limits<size_t>::min();

            void ResolvePath() const;
            size_t FindAliasOffset(AZStd::string_view path) const;

            mutable FixedMaxPath m_path;
            mutable size_t m_absolutePathHash = s_emptyPathHash;
            mutable size_t m_relativePathOffset = 0;
        };
    } // namespace IO
} // namespace AZ

namespace AZStd
{
    template<>
    struct hash<AZ::IO::RequestPath>
    {
        using argument_type = AZ::IO::RequestPath;
        using result_type = AZStd::size_t;
        inline result_type operator()(const argument_type& value) const { return value.GetHash(); }
    };
} // namespace AZStd
