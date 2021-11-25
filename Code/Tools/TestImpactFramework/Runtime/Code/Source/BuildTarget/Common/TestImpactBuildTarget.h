/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/variant.h>

namespace TestImpact
{
    //! Holder for build target types.
    template<typename TestTarget, typename ProductionTarget>
    using BuildTarget = AZStd::variant<const TestTarget*, const ProductionTarget*>;

    //! Optional holder for optional build target types.
    template<typename TestTarget, typename ProductionTarget>
    using OptionalBuildTarget = AZStd::variant<AZStd::monostate, const TestTarget*, const ProductionTarget*>;
} // namespace TestImpact
