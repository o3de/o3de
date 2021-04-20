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

#include <AzCore/IO/Path/Path.h>

namespace TestImpact
{
    //! Wrapper for OS paths relative to a specified parent path.
    //! @note Mimics path semantics only, makes no guarantees about validity.
    class FrameworkPath
    {
    public:
        FrameworkPath() = default;
        //! Creates a path with no parent.
        explicit FrameworkPath(const AZ::IO::Path& absolutePath);
        //! Creates a path with an absolute path and path relative to the specified parent path.
        explicit FrameworkPath(const AZ::IO::Path& absolutePath, const FrameworkPath& relativeTo);

        //! Retrieves the absolute path.
        const AZ::IO::Path& Absolute() const;
        //! Retrieves the path relative to the specified parent path.
        const AZ::IO::Path& Relative() const;

    private:
        //! The absolute path value.
        AZ::IO::Path m_absolutePath;
        //! The path value relative to the specified parent path.
        AZ::IO::Path m_relativePath;
    };
} // namespace TestImpact
