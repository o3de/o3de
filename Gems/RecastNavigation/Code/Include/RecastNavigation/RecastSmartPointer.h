/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>
#include <Recast.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace RecastNavigation
{
    template <typename T>
    struct CustomRecastDeleter;

    //! A memory management helper for various Recast objects that require different methods to free memory.
    template <typename T>
    using RecastPointer = AZStd::unique_ptr<T, CustomRecastDeleter<T>>;

    template <>
    struct CustomRecastDeleter<rcHeightfield>
    {
        void operator ()(rcHeightfield* p)
        {
            rcFreeHeightField(p);
        }
    };

    template <>
    struct CustomRecastDeleter<rcCompactHeightfield>
    {
        void operator ()(rcCompactHeightfield* p)
        {
            rcFreeCompactHeightfield(p);
        }
    };

    template <>
    struct CustomRecastDeleter<rcContourSet>
    {
        void operator ()(rcContourSet* p)
        {
            rcFreeContourSet(p);
        }
    };

    template <>
    struct CustomRecastDeleter<rcPolyMesh>
    {
        void operator ()(rcPolyMesh* p)
        {
            rcFreePolyMesh(p);
        }
    };

    template <>
    struct CustomRecastDeleter<rcPolyMeshDetail>
    {
        void operator ()(rcPolyMeshDetail* p)
        {
            rcFreePolyMeshDetail(p);
        }
    };

    template <>
    struct CustomRecastDeleter<dtNavMesh>
    {
        void operator ()(dtNavMesh* p)
        {
            dtFreeNavMesh(p);
        }
    };

    template <>
    struct CustomRecastDeleter<dtNavMeshQuery>
    {
        void operator ()(dtNavMeshQuery* p)
        {
            dtFreeNavMeshQuery(p);
        }
    };
} // namespace RecastNavigation
