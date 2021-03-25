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

#include <AzCore/base.h>

namespace AZ
{
    namespace Render
    {
        namespace StencilRefs
        {
            const uint32_t None = 0x00;

            // The MeshFeatureProcessor sets this stencil bit on any geometry that should receive IBL specular in the reflections pass,
            // otherwise IBL specular is rendered in the Forward pass.
            // The Reflections pass only renders to areas with this stencil bit set.
            //
            // Pass Range: Forward -> Reflections.
            // Note: The Reflections pass may also overwrite other bits in the stencil buffer.
            const uint32_t UseIBLSpecularPass = 0xF;
        }
    }
}
