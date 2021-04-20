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

#include <Artifact/Dynamic/TestImpactCoverage.h>

namespace TestImpact
{
    namespace Cobertura
    {
        //! Constructs a list of module coverage artifacts from the specified coverage data.
        //! @param coverageData The raw coverage data in XML format.
        //! @return The constructed list of module coverage artifacts.
        AZStd::vector<ModuleCoverage> ModuleCoveragesFactory(const AZStd::string& coverageData);
    } // namespace Cobertura
} // namespace TestImpact
