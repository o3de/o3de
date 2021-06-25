/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TestImpactProductionTarget.h"

namespace TestImpact
{
    ProductionTarget::ProductionTarget(Descriptor&& descriptor)
        : BuildTarget(AZStd::move(descriptor), TargetType::Production)
    {
    }
} // namespace TestImpact
