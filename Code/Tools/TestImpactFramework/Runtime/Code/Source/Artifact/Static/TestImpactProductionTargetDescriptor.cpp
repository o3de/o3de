/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Artifact/Static/TestImpactProductionTargetDescriptor.h>

namespace TestImpact
{
    ProductionTargetDescriptor::ProductionTargetDescriptor(BuildTargetDescriptor&& buildTargetDescriptor)
        : BuildTargetDescriptor(AZStd::move(buildTargetDescriptor))
    {
    }
} // namespace TestImpact
