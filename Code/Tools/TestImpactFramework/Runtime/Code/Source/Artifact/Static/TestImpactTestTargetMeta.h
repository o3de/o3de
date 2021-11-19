/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactTestSuiteMeta.h>

#include <AzCore/std/containers/unordered_map.h>

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
        TestSuiteMeta m_suiteMeta;
        AZStd::string m_customArgs;
        LaunchMethod m_launchMethod = LaunchMethod::TestRunner;
    };

    //! Map between test target name and test target meta-data.
    using TestTargetMetaMap = AZStd::unordered_map<AZStd::string, TestTargetMeta>;
} // namespace TestImpact
