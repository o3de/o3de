/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>

namespace AZ
{
    namespace Render
    {
        //! EditorExposureControlConfig reflects edit context for non-editor data
        struct EditorExposureControlConfig final
        {
            AZ_TYPE_INFO(AZ::Render::EditorExposureControlConfig, "{91881B6A-3708-4970-A362-9F5BC0432A02}");
            static void Reflect(AZ::ReflectContext* context);
        };

        //! EditorLightConfig reflects edit context for non-editor data
        struct EditorLightConfig final
        {
            AZ_TYPE_INFO(AZ::Render::EditorLightConfig, "{789FB848-DE7C-4C7D-B843-432D2C51ED09}");
            static void Reflect(AZ::ReflectContext* context);
        };

        //! EditorLightingPreset reflects edit context for non-editor data
        struct EditorLightingPreset final
        {
            AZ_TYPE_INFO(AZ::Render::EditorLightingPreset, "{AC0F3076-6540-4FEA-956C-E0360850C89C}");
            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
