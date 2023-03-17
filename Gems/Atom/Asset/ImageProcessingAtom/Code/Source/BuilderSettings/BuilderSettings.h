/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <BuilderSettings/PresetSettings.h>

namespace ImageProcessingAtom
{
    //builder setting for a platform
    struct BuilderSettings
    {
        AZ_TYPE_INFO(BuilderSettings, "{3C70C3C0-E395-4948-97AB-31541847147F}");
        AZ_CLASS_ALLOCATOR(BuilderSettings, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);

        //global settings
        float m_brdfGlossScale = 16.0f;
        float m_brdfGlossBias = 0.0f;
        bool m_enableStreaming = true;
        bool m_enablePlatform = true;
    };
} // namespace ImageProcessingAtom
