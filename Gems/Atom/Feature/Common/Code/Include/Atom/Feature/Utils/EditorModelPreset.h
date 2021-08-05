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
        //! EditorModelPreset reflects edit context for non-editor data
        struct EditorModelPreset final
        {
            AZ_TYPE_INFO(AZ::Render::EditorModelPreset, "{88AD9398-D800-4DE7-A05B-EB47FFBE70D7}");
            static void Reflect(AZ::ReflectContext* context);
        };
    }
}
