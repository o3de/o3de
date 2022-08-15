/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Dynamic/TestImpactCoverage.h>
#include <Artifact/Dynamic/TestImpactTestSuite.h>

namespace TestImpact
{
    namespace Cobertura
    {
        //! Constructs a list of module coverage artifacts from the specified coverage data.
        //! @param coverageData The raw coverage data in XML format.
        //! @return The constructed list of module coverage artifacts.
        AZStd::vector<ModuleCoverage> ModuleCoveragesFactory(const AZStd::string& coverageData);
    } // namespace Cobertura

    namespace PythonCoverage
    {
        //! Coverage artifact for PyCoverage files.
        struct PythonModuleCoverage
        {
            AZStd::string m_testCase;
            AZStd::string m_testSuite;
            AZStd::vector<AZStd::string> m_components;
        };

        //! Constructs a list of module coverage artifacts from the specified coverage data.
        //! @param coverageData The raw coverage data in XML format.
        //! @return The constructed list of module coverage artifacts.
        PythonModuleCoverage ModuleCoveragesFactory(const AZStd::string& coverageData);
    } // namespace JUnit
} // namespace TestImpact
