/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Crc.h>
#include <AzFramework/SurfaceData/SurfaceData.h>

namespace SurfaceData
{
    namespace Constants
    {
        static const char* s_unassignedTagName = AzFramework::SurfaceData::Constants::s_unassignedTagName;

        static const AZ::Crc32 s_unassignedTagCrc = AZ::Crc32(s_unassignedTagName);
    }
}
