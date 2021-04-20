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
