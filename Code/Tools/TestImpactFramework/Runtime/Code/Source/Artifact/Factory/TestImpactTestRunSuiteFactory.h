/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Dynamic/TestImpactTestRunSuite.h>

namespace TestImpact
{
    namespace GTest
    {
        //! Constructs a list of test run suite artifacts from the specified test run data.
        //! @param testRunData The raw test run data in XML format.
        //! @return The constructed list of test run suite artifacts.
        AZStd::vector<TestRunSuite> TestRunSuitesFactory(const AZStd::string& testRunData);
    } // namespace GTest
} // namespace TestImpact
