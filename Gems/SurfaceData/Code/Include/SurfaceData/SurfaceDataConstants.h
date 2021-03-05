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

#include <AzCore/Math/Crc.h>

namespace SurfaceData
{
    namespace Constants
    {
        static const char* s_unassignedTagName = "(unassigned)";
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