/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactTestScriptTargetDescriptor.h>

namespace TestImpact
{
    //! Build target specialization for test targets (build targets containing test code and no production code).
    class TestScriptTarget
    {
    public:
        using Descriptor = TestScriptTargetDescriptor;

        TestScriptTarget(Descriptor&& descriptor);

        //! Returns the test script target name.
        const AZStd::string& GetName() const;

        //! Returns the test script target suite.
        const AZStd::string& GetSuite() const;

        //! Returns the path in the source tree to the test script.
        const RepoPath& GetScriptPath() const;

        //! Returns the test run timeout.
        AZStd::chrono::milliseconds GetTimeout() const;

    private:
        const Descriptor m_testScriptTargetDescriptor;
    };
} // namespace TestImpact
