/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/base.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    //! Voxel (stamped-cube) helpers: cell packing, size grouping, surface meshing and face signatures.
    namespace VoxelDetail
    {
        //! Pack integer cell coordinates into a single key (21 bits per axis, biased so
        //! negatives work; range ~ +/-1,000,000 cells per axis).
        AZ::u64 PackCell(int x, int y, int z);
        //! Unpack a cell key back into integer coordinates.
        void UnpackCell(AZ::u64 key, int& x, int& y, int& z);

        //! Quantize a world-space cube size so tiny float differences map to the same group.
        AZ::s32 QuantizeSize(float size);
        //! The world-space cube size a quantized key represents.
        float SizeFromQuantized(AZ::s32 q);

        //! Cells grouped by quantized cube size, one set per size.
        using CellSet = AZStd::unordered_set<AZ::u64>;
        using SizeGroups = AZStd::unordered_map<AZ::s32, CellSet>;

        SizeGroups GroupBySize(const AZStd::vector<AZ::u64>& cells, const AZStd::vector<float>& sizes);
        void FlattenGroups(
            const SizeGroups& groups, AZStd::vector<AZ::u64>& outCells, AZStd::vector<float>& outSizes);

        //! Greedy-merged surface for filled cells. May have T-junctions, so not a CSG operand (use GenerateSurfacePerCell).
        void GenerateSurface(WhiteBoxMesh& mesh, const AZStd::unordered_set<AZ::u64>& cells, float cellSize);

        //! One quad per exposed cell face: watertight and manifold, so a valid CSG operand.
        void GenerateSurfacePerCell(WhiteBoxMesh& mesh, const AZStd::unordered_set<AZ::u64>& cells, float cellSize);

        //! Quantize a vertex position to its voxel-grid cell index. Returns false if the
        //! position is not on the grid (such a vertex belongs to freeform geometry).
        bool QuantizeCorner(const AZ::Vector3& p, float cellSize, int& x, int& y, int& z);

        //! Order-independent identity of a voxel-surface triangle: the sorted packed keys of
        //! its three integer corners.
        using FaceSignature = AZStd::array<AZ::u64, 3>;

        //! AZStd::array provides no operator< in this engine version, so order the set
        //! explicitly (lexicographically over the three packed corner keys).
        struct FaceSignatureLess
        {
            bool operator()(const FaceSignature& a, const FaceSignature& b) const
            {
                for (size_t i = 0; i < 3; ++i)
                {
                    if (a[i] != b[i])
                    {
                        return a[i] < b[i];
                    }
                }
                return false;
            }
        };
        using FaceSignatureSet = AZStd::set<FaceSignature, FaceSignatureLess>;

        //! Signature of a triangle from its three vertex positions; false when any corner is
        //! off-grid (freeform geometry).
        bool FaceSignatureFromPositions(
            const AZStd::vector<AZ::Vector3>& positions, float cellSize, FaceSignature& outSig);

        //! The set of triangle signatures that make up the voxel surface for @p cells,
        //! generated through the exact same path as the live mesh.
        FaceSignatureSet SurfaceFaceSignatures(const AZStd::unordered_set<AZ::u64>& cells, float cellSize);

        //! Greedy-mesh the voxel cells into a minimal indexed triangle set for a physics
        //! collider (triangles are APPENDED to verts/indices, absolute indices).
        void GreedyColliderTriangles(
            const AZStd::unordered_set<AZ::u64>& cells, float cellSize, AZStd::vector<AZ::Vector3>& verts,
            AZStd::vector<AZ::u32>& indices);
    } // namespace VoxelDetail

    //! Whether every vertex of @p mesh lies within the cells - Manifold reports an empty result as failure.
    bool MeshFullyInsideCells(
        const WhiteBoxMesh& mesh, const AZStd::vector<AZ::u64>& cells, const AZStd::vector<float>& sizes);
} // namespace WhiteBox
