/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Artifact produced by the build system for each test target containing the additional meta-data about the test.
    struct TestSuiteMeta
    {
        AZStd::string m_name;
        AZStd::chrono::milliseconds m_timeout = AZStd::chrono::milliseconds{ 0 };
    };
} // namespace TestImpact
