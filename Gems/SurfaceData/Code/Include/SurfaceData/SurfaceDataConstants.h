/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Crc.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>

namespace SurfaceData
{
    namespace Constants
    {
        static const char* s_unassignedTagName = AzFramework::SurfaceData::Constants::s_unassignedTagName;
        static const char* s_terrainHoleTagName = "terrainHole";
        static const char* s_terrainTagName = "terrain";

        static const AZ::Crc32 s_unassignedTagCrc = AZ::Crc32(s_unassignedTagName);
        static const AZ::Crc32 s_terrainHoleTagCrc = AZ::Crc32(s_terrainHoleTagName);
        static const AZ::Crc32 s_terrainTagCrc = AZ::Crc32(s_terrainTagName);

        static const char* s_allTagNames[] =
        {
            s_unassignedTagName,
            s_terrainHoleTagName,
            s_terrainTagName,
        };
    }
}
