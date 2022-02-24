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
    NativeProductionTarget::NativeProductionTarget(AZStd::unique_ptr<Descriptor> descriptor)
        : NativeTarget(descriptor.get())
        , m_descriptor(AZStd::move(descriptor))
    {
    }
} // namespace TestImpact
