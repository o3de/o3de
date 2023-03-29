/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestRunner/Common/Run/TestImpactTestRun.h>

#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Serializes the specified test run to JSON format.
    AZStd::string SerializeTestRun(const TestRun& testRun);

    //! Deserializes a test run from the specified test run data in JSON format.
    TestRun DeserializeTestRun(const AZStd::string& testRunString);

    namespace GTest
    {
        //! Serializes the specified test run to GTest JUnit format.
        AZStd::string SerializeTestRun(const TestRun& testRun);
    } // namespace GTest
} // namespace TestImpact
