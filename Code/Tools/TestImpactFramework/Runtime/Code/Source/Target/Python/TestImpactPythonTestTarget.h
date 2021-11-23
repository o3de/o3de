/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactPythonTestTargetDescriptor.h>
#include <Target/Common/TestImpactTarget.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    //! Build target specialization for test targets (build targets containing test code and no production code).
    class TestScriptTarget
        : public Target
    {
    public:
        using Descriptor = TestScriptTargetDescriptor;

        TestScriptTarget(AZStd::unique_ptr<Descriptor> descriptor);

        //! Returns the test script target suite.
        const AZStd::string& GetSuite() const;

        //! Returns the path in the source tree to the test script.
        const RepoPath& GetScriptPath() const;

        //! Returns the test run timeout.
        AZStd::chrono::milliseconds GetTimeout() const;

    private:
        AZStd::unique_ptr<Descriptor> m_descriptor;
    };
} // namespace TestImpact
