/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactTestSuiteMeta.h>

namespace TestImpact
{
    //! Artifact produced by the build system for each test target containing the additional meta-data about the test.
    struct TestTargetMeta
    {
        TestSuiteMeta m_suiteMeta;
        AZStd::string m_namespace;
    };
} // namespace TestImpact
