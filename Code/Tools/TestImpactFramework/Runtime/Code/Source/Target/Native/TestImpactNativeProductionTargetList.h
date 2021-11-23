/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Target/Common/TestImpactTargetList.h>
#include <Target/Native/TestImpactNativeProductionTarget.h>

namespace TestImpact
{
    //! Container for set of sorted production targets containing no duplicates.
    class NativeProductionTargetList
        : public TargetList<NativeProductionTarget>
    {
    public:
        using TargetList<NativeProductionTarget>::TargetList;
    };
} // namespace TestImpact
