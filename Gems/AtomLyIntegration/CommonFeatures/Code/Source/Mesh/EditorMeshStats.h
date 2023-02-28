/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace Render
    {
        struct EditorMeshStatsForLod final
        {
            AZ_RTTI(EditorMeshStatsForLod, "{626E3AEB-0F7A-4777-BAF1-2BBA8C1857ED}");
            AZ_CLASS_ALLOCATOR(EditorMeshStatsForLod, SystemAllocator);

            static void Reflect(ReflectContext* context);

            AZ::u32 m_meshCount = 0;
            AZ::u32 m_vertCount = 0;
            AZ::u32 m_triCount = 0;
        };

        struct EditorMeshStats final
        {
            AZ_RTTI(EditorMeshStats, "{68D0D3EF-17BB-46EA-B98F-51355402CCD6}");
            AZ_CLASS_ALLOCATOR(EditorMeshStats, SystemAllocator);

            static void Reflect(ReflectContext* context);
            AZStd::string GetLodLabel(int index) const;

            AZStd::vector<EditorMeshStatsForLod> m_meshStatsForLod;
        };
    } // namespace Render
} // namespace AZ
