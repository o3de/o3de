/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
