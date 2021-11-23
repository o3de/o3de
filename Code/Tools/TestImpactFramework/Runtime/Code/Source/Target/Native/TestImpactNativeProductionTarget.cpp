/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Target/Native/TestImpactNativeProductionTarget.h>

namespace TestImpact
{
    ProductionTarget::ProductionTarget(AZStd::unique_ptr<Descriptor> descriptor)
        : BuildTarget(descriptor.get(), SpecializedBuildTargetType::Production)
        , m_descriptor(AZStd::move(descriptor))
    {
    }
} // namespace TestImpact
