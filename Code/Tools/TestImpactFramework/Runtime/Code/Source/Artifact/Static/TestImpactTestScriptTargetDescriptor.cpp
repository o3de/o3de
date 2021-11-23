/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Artifact/Static/TestImpactTestScriptTargetDescriptor.h>

namespace TestImpact
{
    TestScriptTargetDescriptor::TestScriptTargetDescriptor(
        TargetDescriptor&& targetDescriptor, TestSuiteMeta&& testSuiteMeta, const RepoPath& scriptPath)
        : TargetDescriptor(AZStd::move(targetDescriptor))
        , m_testSuiteMeta(AZStd::move(testSuiteMeta))
        , m_scriptPath(scriptPath)
    {
    }
} // namespace TestImpact
