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

#include "RenderGLConfig.h"
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector3.h>

namespace RenderGL
{
    struct RENDERGL_API Light
    {
        Light()
            : m_dir(-1.0f, 0.0f, -1.0f)
            , m_diffuse(1.0f, 1.0f, 1.0f, 1.0f)
            , m_specular(1.0f, 1.0f, 1.0f, 1.0f)
            , m_ambient(0.0f, 0.0f, 0.0f, 0.0f)
        {}

        AZ::Vector3 m_dir;
        AZ::Color m_diffuse;
        AZ::Color m_specular;
        AZ::Color m_ambient;
    };
}
