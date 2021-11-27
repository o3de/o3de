/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <BuildTarget/Common/TestImpactBuildTarget.h>
#include <Target/Common/TestImpactTargetList.h>

#include <AzCore/std/typetraits/is_same.h>

namespace TestImpact
{
    template<typename TestTargetType, typename ProductionTargetType>
    struct BuildTargetTraits
    {
        //!
        using TestTarget = typename TestTargetType;

        //!
        using ProductionTarget = ProductionTargetType;

        //!
        using TestTargetList = TargetList<TestTarget>;

        //!
        using ProductionTargetList = TargetList<ProductionTarget>;

        //!
        using BuildTarget = BuildTarget<TestTarget, ProductionTarget>;

        //!
        using OptionalBuildTarget = OptionalBuildTarget<TestTarget, ProductionTarget>;

        //!
        template<typename Target>
        static constexpr bool IsProductionTarget =
            AZStd::is_same_v<ProductionTarget, AZStd::remove_const_t<AZStd::remove_pointer_t<AZStd::decay_t<Target>>>>;

        //!
        template<typename Target>
        static constexpr bool IsTestTarget =
            AZStd::is_same_v<TestTarget, AZStd::remove_const_t<AZStd::remove_pointer_t<AZStd::decay_t<Target>>>>;
    };
} // namespace TestImpact
