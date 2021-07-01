/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
