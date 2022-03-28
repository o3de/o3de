/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Crc.h>
<<<<<<< HEAD
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
=======
#include <AzFramework/SurfaceData/SurfaceData.h>
>>>>>>> development

namespace SurfaceData
{
    namespace Constants
    {
        static const char* s_unassignedTagName = AzFramework::SurfaceData::Constants::s_unassignedTagName;
<<<<<<< HEAD
        static const char* s_terrainHoleTagName = "terrainHole";
        static const char* s_terrainTagName = "terrain";
=======
>>>>>>> development

        static const AZ::Crc32 s_unassignedTagCrc = AZ::Crc32(s_unassignedTagName);
    }
}
