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

#include <TestImpactFramework/TestImpactFrameworkPath.h>

namespace TestImpact
{
    FrameworkPath::FrameworkPath(const AZ::IO::Path& absolutePath)
    {
        m_absolutePath = AZ::IO::Path(absolutePath).MakePreferred();
        m_relativePath = m_absolutePath.LexicallyRelative(m_absolutePath);
    }

    FrameworkPath::FrameworkPath(const AZ::IO::Path& absolutePath, const FrameworkPath& relativeTo)
    {
        m_absolutePath = AZ::IO::Path(absolutePath).MakePreferred();
        m_relativePath = m_absolutePath.LexicallyRelative(relativeTo.Absolute());
    }

    const AZ::IO::Path& FrameworkPath::Absolute() const
    {
        return m_absolutePath;
    }

    const AZ::IO::Path& FrameworkPath::Relative() const
    {
        return m_relativePath;
    }
} // namespace TestImpact
