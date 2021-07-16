/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <BuilderSettings/BuilderSettings.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace ImageProcessingAtom
{
    void BuilderSettings::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<BuilderSettings>()
                ->Version(1)
                ->Field("GlossScale", &BuilderSettings::m_brdfGlossScale)
                ->Field("GlossBias", &BuilderSettings::m_brdfGlossBias)
                ->Field("Streaming", &BuilderSettings::m_enableStreaming)
                ->Field("Enable", &BuilderSettings::m_enablePlatform)
                ;
        }
    }
} // namespace ImageProcessingAtom
