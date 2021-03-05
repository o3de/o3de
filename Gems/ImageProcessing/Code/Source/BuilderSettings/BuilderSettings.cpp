/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "ImageProcessing_precompiled.h"
#include <BuilderSettings/BuilderSettings.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace ImageProcessing
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
                ->Field("Presets", &BuilderSettings::m_presets);
        }
    }
} // namespace ImageProcessing