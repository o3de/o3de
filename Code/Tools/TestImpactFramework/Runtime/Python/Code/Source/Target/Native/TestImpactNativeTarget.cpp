/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Target/Native/TestImpactNativeTarget.h>

namespace TestImpact
{
    NativeTarget::NativeTarget(NativeTargetDescriptor* descriptor)
        : Target(descriptor)
        , m_descriptor(descriptor)
    {
    }

    const AZStd::string& NativeTarget::GetOutputName() const
    {
        return m_descriptor->m_outputName;
    }
} // namespace TestImpact
