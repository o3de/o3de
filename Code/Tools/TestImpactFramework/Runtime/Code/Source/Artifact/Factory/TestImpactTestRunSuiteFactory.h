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
