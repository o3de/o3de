/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactRepoPath.h>

#include <Artifact/Static/TestImpactTestSuiteMeta.h>

namespace TestImpact
{
    //! Artifact produced by the target artifact compiler that represents a test build target in the repository.
    struct TestScriptDescriptor
    {
        AZStd::string m_name; //!< Python test name.
        TestSuiteMeta m_suiteMeta;
        RepoPath m_scriptPath; //!< Path to the Python script for this test (relative to repository root).
    };
} // namespace TestImpact
