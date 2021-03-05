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
        //! EditorModelPreset reflects edit context for non-editor data
        struct EditorModelPreset final
        {
            AZ_TYPE_INFO(AZ::Render::EditorModelPreset, "{88AD9398-D800-4DE7-A05B-EB47FFBE70D7}");
            static void Reflect(AZ::ReflectContext* context);
        };
    }
}
