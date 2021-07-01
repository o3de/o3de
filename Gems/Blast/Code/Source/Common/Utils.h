/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Vector3.h>
#include <NvBlastDebugRender.h>

namespace Blast
{
    inline NvcVec3 Convert(AZ::Vector3 vector3)
    {
        return {
            static_cast<float>(vector3.GetX()), static_cast<float>(vector3.GetY()), static_cast<float>(vector3.GetZ())};
    }
} // namespace Blast
