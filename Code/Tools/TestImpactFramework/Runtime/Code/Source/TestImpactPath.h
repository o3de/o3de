/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/IO/Path/Path.h>

namespace TestImpact
{
    class Path
        : public AZ::IO::Path
    {
    public:
        constexpr Path() = default;
        explicit Path(AZStd::string&&);
        explicit Path(const AZStd::string&);
        explicit Path(AZ::IO::Path&&);
        explicit Path(const AZ::IO::Path&);
        explicit Path(Path&) = default;
        explicit Path(Path&&) noexcept = default;

        Path& operator=(const Path&) noexcept = default;
        Path& operator=(const AZStd::string&) noexcept;
        Path& operator=(const AZ::IO::Path& str) noexcept;
    };
} // namespace TestImpact
