/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Target/TestImpactBuildTargetList.h>
#include <Target/TestImpactTestTarget.h>

namespace TestImpact
{
    //! Container for set of sorted test targets containing no duplicates.
    using TestTargetList = BuildTargetList<TestTarget>;
} // namespace TestImpact
