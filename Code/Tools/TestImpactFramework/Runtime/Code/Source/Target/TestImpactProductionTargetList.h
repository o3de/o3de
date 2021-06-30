/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Target/TestImpactBuildTargetList.h>
#include <Target/TestImpactProductionTarget.h>

namespace TestImpact
{
    //! Container for set of sorted production targets containing no duplicates.
    using ProductionTargetList = BuildTargetList<ProductionTarget>;
} // namespace TestImpact
