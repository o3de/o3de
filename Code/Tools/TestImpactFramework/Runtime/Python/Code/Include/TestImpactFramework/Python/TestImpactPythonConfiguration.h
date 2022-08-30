/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactConfiguration.h>

namespace TestImpact
{
    //! Test engine configuration.
    struct PythonTestEngineConfig
    {
        //! Test runner configuration.
        struct TestRunner
        {
            RepoPath m_pythonCmd; //!< Path to the python command.
        };

        TestRunner m_testRunner;
    };

    //! Build target configuration.
    struct PythonTargetConfig
    {
        ExcludedTargets m_excludedTargets; 
    };

    //! Python runtime configuration.
    struct PythonRuntimeConfig
    {
        RuntimeConfig m_commonConfig;
        WorkspaceConfig m_workspace;
        PythonTestEngineConfig m_testEngine;
        PythonTargetConfig m_target;
    };
} // namespace TestImpact
