/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactTargetDescriptor.h>
#include <Artifact/Static/TestImpactTestSuiteMeta.h>

namespace TestImpact
{
    //! Artifact produced by the target artifact compiler that represents a test build target in the repository.
    struct TestScriptTargetDescriptor
        : public TargetDescriptor
    {
        TestScriptTargetDescriptor() = default;
        TestScriptTargetDescriptor(TargetDescriptor&& targetDescriptor, TestSuiteMeta&& testSuiteMeta, const RepoPath& scriptPath);

        TestSuiteMeta m_testSuiteMeta;
        RepoPath m_scriptPath; //!< Path to the Python script for this test (relative to repository root).
    };
} // namespace TestImpact
