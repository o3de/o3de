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
    NativeTarget::NativeTarget(TargetDescriptor&& descriptor, NativeTargetDescriptor&& nativeDescriptor)
        : Target(AZStd::move(descriptor))
        , m_nativeDescriptor(nativeDescriptor)
    {
    }

    const AZStd::string& NativeTarget::GetOutputName() const
    {
        return m_nativeDescriptor.m_outputName;
    }
} // namespace TestImpact
