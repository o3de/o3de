/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomShaderCapabilitiesConfigFile.h>

namespace AZ::ShaderBuilder::AtomShaderConfig
{
    void CapabilitiesConfigFile::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<CapabilitiesConfigFile>()
                ->Version(0)
                ->Field("DescriptorCounts", &CapabilitiesConfigFile::m_descriptorCounts)
                ->Field("MaxSpaces", &CapabilitiesConfigFile::m_maxSpaces)
                ;
        }
    }
}
