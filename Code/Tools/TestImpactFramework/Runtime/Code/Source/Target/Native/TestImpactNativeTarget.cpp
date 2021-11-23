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
    BuildTarget::BuildTarget(BuildTargetDescriptor* descriptor, SpecializedBuildTargetType type)
        : Target(descriptor)
        , m_descriptor(descriptor)
        , m_type(type)
    {
    }

    const AZStd::string& BuildTarget::GetOutputName() const
    {
        return m_descriptor->m_outputName;
    }

    SpecializedBuildTargetType BuildTarget::GetSpecializedBuildTargetType() const
    {
        return m_type;
    }
} // namespace TestImpact
