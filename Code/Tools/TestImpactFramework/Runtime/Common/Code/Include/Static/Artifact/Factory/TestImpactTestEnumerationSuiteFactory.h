/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    namespace Python
    {
        //! Pair containing a Python script name and a vector of the related TestEnumerationSuites
        using PythonTestEnumerationSuite = AZStd::pair<AZStd::string, AZStd::vector<TestEnumerationSuite>>;

        //! Constructs a list of test enumeration suite artifacts from the specified test enumeraion data.
        //! @param testEnumerationData The raw test enumeration data collected from pytest using the --collect-only argument, with -q mode
        //! enabled.
        //! @return The constructed list of pairs of absolute paths to the test script, and the corresponding test enumeration suite
        //! artifacts.
        AZStd::vector<PythonTestEnumerationSuite> TestEnumerationSuitesFactory(
            const AZStd::string& testEnumerationData);
    } // namespace Python
} // namespace TestImpact
