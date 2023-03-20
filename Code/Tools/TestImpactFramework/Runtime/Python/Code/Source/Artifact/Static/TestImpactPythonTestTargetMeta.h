/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactRepoPath.h>

#include <Artifact/Static/TestImpactTestTargetMeta.h>

#include <AzCore/std/containers/unordered_map.h>

namespace TestImpact
{
    struct PythonTargetScriptMeta
    {
        RepoPath m_scriptPath; //!< Path to the Python script for this test (relative to repository root).
        AZStd::string m_testCommand; //!< Command string to execute this test.
    };

    //! Artifact produced by the target artifact compiler that represents a test build target in the repository.
    struct PythonTestTargetMeta
    {
        TestTargetMeta m_testTargetMeta; //<! The meta-data about this target's test suite.
        PythonTargetScriptMeta m_scriptMeta; //!< The meta-data about this target's test script.
    };

    //! Map between test target name and test target meta-data.
    using PythonTestTargetMetaMap = AZStd::unordered_map<AZStd::string, PythonTestTargetMeta>;
} // namespace TestImpact
