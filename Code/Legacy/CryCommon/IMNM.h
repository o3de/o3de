/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef __IMNM_H__
#define __IMNM_H__

#pragma once

namespace MNM
{
    namespace Constants
    {
        enum Edges
        {
            InvalidEdgeIndex = ~0u
        };

        enum TileIdConstants
        {
            InvalidTileID = 0,
        };
        enum TriangleIDConstants
        {
            InvalidTriangleID = 0,
        };

        enum EStaticIsland
        {
            eStaticIsland_InvalidIslandID = 0,
            eStaticIsland_FirstValidIslandID = 1,
        };
        enum EGlobalIsland
        {
            eGlobalIsland_InvalidIslandID = 0,
        };

        enum EOffMeshLink
        {
            eOffMeshLinks_InvalidOffMeshLinkID = 0,
        };
    }

    // ------------------------------------------------------------------------------------------
    // Basic types used in the MNM namespace

    typedef uint32 TileID;
    typedef uint32 TriangleID;
    typedef uint32 OffMeshLinkID;

    // StaticIslandIDs identify triangles that are statically connected inside a mesh
    // and that are reachable without the use of any off mesh links.
    typedef uint32 StaticIslandID;

    // GlobalIslandIDs define IDs able to code and connect islands between meshes.
    struct GlobalIslandID
    {
        GlobalIslandID(const uint64 defaultValue = MNM::Constants::eGlobalIsland_InvalidIslandID)
            : id(defaultValue)
        {
            STATIC_ASSERT(sizeof(StaticIslandID) <= 4, "The maximum supported size for StaticIslandIDs is 4 bytes.");
        }

        GlobalIslandID(const uint32 navigationMeshID, const MNM::StaticIslandID islandID)
        {
            id = ((uint64)navigationMeshID << 32) | islandID;
        }

        GlobalIslandID operator=(const GlobalIslandID other)
        {
            id = other.id;
            return *this;
        }

        inline bool operator==(const GlobalIslandID& rhs) const
        {
            return id == rhs.id;
        }

        inline bool operator<(const GlobalIslandID& rhs) const
        {
            return id < rhs.id;
        }

        MNM::StaticIslandID GetStaticIslandID() const
        {
            return (MNM::StaticIslandID) (id & ((uint64)1 << 32) - 1);
        }

        uint32 GetNavigationMeshIDAsUint32() const
        {
            return static_cast<uint32>(id >> 32);
        }

        uint64 id;
    };
}

#endif // __IMNM_H__
