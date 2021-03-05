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
