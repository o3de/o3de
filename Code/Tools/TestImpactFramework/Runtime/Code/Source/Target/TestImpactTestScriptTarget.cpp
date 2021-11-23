/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TestImpactTestScriptTarget.h"

namespace TestImpact
{
    TestScriptTarget::TestScriptTarget(AZStd::unique_ptr<Descriptor> descriptor)
        : Target(descriptor.get())
        , m_descriptor(AZStd::move(descriptor))
    {
    }

    const AZStd::string& TestScriptTarget::GetSuite() const
    {
        return m_descriptor->m_testSuiteMeta.m_name;
    }

    const RepoPath& TestScriptTarget::GetScriptPath() const
    {
        return m_descriptor->m_scriptPath;
    }

    AZStd::chrono::milliseconds TestScriptTarget::GetTimeout() const
    {
        return m_descriptor->m_testSuiteMeta.m_timeout;
    }
} // namespace TestImpact
