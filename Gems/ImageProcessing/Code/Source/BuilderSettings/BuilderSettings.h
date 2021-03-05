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

#pragma once

#include <BuilderSettings/PresetSettings.h>

namespace ImageProcessing
{
    //builder setting for a platform
    struct BuilderSettings
    {
        AZ_TYPE_INFO(BuilderSettings, "{4085AB56-934C-43A6-AF25-4443E1EEB71D}");
        AZ_CLASS_ALLOCATOR(BuilderSettings, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);

        //global settings
        float m_brdfGlossScale = 16.0f;
        float m_brdfGlossBias = 0.0f;
        bool m_enableStreaming = true;
        bool m_enablePlatform = true;
        AZStd::map<AZ::Uuid, PresetSettings> m_presets;
    };
} // namespace ImageProcessing