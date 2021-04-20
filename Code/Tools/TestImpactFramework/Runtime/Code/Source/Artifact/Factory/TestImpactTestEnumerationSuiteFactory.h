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

#include <Artifact/Dynamic/TestImpactTestEnumerationSuite.h>

namespace TestImpact
{
    namespace GTest
    {
        //! Constructs a list of test enumeration suite artifacts from the specified test enumeraion data.
        //! @param testEnumerationData The raw test enumeration data in XML format.
        //! @return The constructed list of test enumeration suite artifacts.
        AZStd::vector<TestEnumerationSuite> TestEnumerationSuitesFactory(const AZStd::string& testEnumerationData);
    } // namespace GTest
} // namespace TestImpact
