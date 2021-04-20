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

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Method used to launch the test target.
    enum class LaunchMethod : bool
    {
        TestRunner, //!< Target is launched through a separate test runner binary.
        StandAlone //!< Target is launched directly by itself.
    };

    //! Artifact produced by the build system for each test target containing the additional meta-data about the test.
    struct TestTargetMeta
    {
        AZStd::string m_suite;
        LaunchMethod m_launchMethod = LaunchMethod::TestRunner;
    };

    //! Map between test target name and test target meta-data.
    using TestTargetMetaMap = AZStd::unordered_map<AZStd::string, TestTargetMeta>;
} // namespace TestImpact
