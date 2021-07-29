/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactProductionTargetDescriptor.h>
#include <Target/TestImpactBuildTarget.h>

namespace TestImpact
{
    //! Build target specialization for production targets (build targets containing production code and no test code).
    class ProductionTarget
        : public BuildTarget
    {
    public:
        using Descriptor = ProductionTargetDescriptor;
        ProductionTarget(Descriptor&& descriptor);
    };

    template<typename Target>
    inline constexpr bool IsProductionTarget = AZStd::is_same_v<ProductionTarget, AZStd::remove_const_t<AZStd::remove_pointer_t<AZStd::decay_t<Target>>>>;
} // namespace TestImpact
