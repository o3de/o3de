/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxCsgCore.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <tuple>
#include <AzCore/std/containers/map.h>

#include <AzCore/Debug/Trace.h>

// manifold (Apache-2.0) - see 3rdParty/FindManifold.cmake
#include <manifold/manifold.h>

namespace WhiteBox
{
    namespace Csg
    {
        void WeldVertices(TriangleMesh& mesh, const double tolerance)
        {
            const size_t vertexCount = mesh.VertexCount();
            const double invTolerance = 1.0 / tolerance;

            // map quantized position -> first (compacted) vertex index found at that position
            std::map<std::tuple<int64_t, int64_t, int64_t>, uint32_t> grid;
            std::vector<uint32_t> remap(vertexCount, 0);
            std::vector<double> weldedPositions;
            weldedPositions.reserve(mesh.m_positions.size());

            const auto quantize = [invTolerance](const double value)
            {
                return static_cast<int64_t>(std::llround(value * invTolerance));
            };

            for (size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
            {
                const double x = mesh.m_positions[vertexIndex * 3 + 0];
                const double y = mesh.m_positions[vertexIndex * 3 + 1];
                const double z = mesh.m_positions[vertexIndex * 3 + 2];

                const auto key = std::make_tuple(quantize(x), quantize(y), quantize(z));
                const auto gridIt = grid.find(key);
                if (gridIt != grid.end())
                {
                    remap[vertexIndex] = gridIt->second;
                }
                else
                {
                    const auto weldedIndex = static_cast<uint32_t>(weldedPositions.size() / 3);
                    weldedPositions.push_back(x);
                    weldedPositions.push_back(y);
                    weldedPositions.push_back(z);
                    grid.emplace(key, weldedIndex);
                    remap[vertexIndex] = weldedIndex;
                }
            }

            // remap the index buffer, dropping triangles that have collapsed to a line/point
            std::vector<uint32_t> weldedIndices;
            weldedIndices.reserve(mesh.m_indices.size());
            std::vector<std::string> weldedMaterials;
            const bool hasMaterials = !mesh.m_materials.empty();
            std::vector<uint32_t> weldedColors;
            const bool hasColors = !mesh.m_colors.empty();
            for (size_t triangleIndex = 0; triangleIndex < mesh.TriangleCount(); ++triangleIndex)
            {
                const uint32_t i0 = remap[mesh.m_indices[triangleIndex * 3 + 0]];
                const uint32_t i1 = remap[mesh.m_indices[triangleIndex * 3 + 1]];
                const uint32_t i2 = remap[mesh.m_indices[triangleIndex * 3 + 2]];
                if (i0 != i1 && i1 != i2 && i2 != i0)
                {
                    weldedIndices.push_back(i0);
                    weldedIndices.push_back(i1);
                    weldedIndices.push_back(i2);
                    if (hasColors)
                    {
                        weldedColors.push_back(mesh.Color(triangleIndex));
                    }
                    if (hasMaterials)
                    {
                        weldedMaterials.push_back(mesh.Material(triangleIndex));
                    }
                }
            }

            mesh.m_positions = std::move(weldedPositions);
            mesh.m_indices = std::move(weldedIndices);
            mesh.m_materials = std::move(weldedMaterials);
            mesh.m_colors = std::move(weldedColors);
        }

        namespace
        {
            // Convert a TriangleMesh into a manifold::Manifold (double precision).
            manifold::Manifold ToManifold(const TriangleMesh& mesh, uint64_t faceOffset, bool preserveMaterials)
            {
                manifold::MeshGL64 meshGl;
                meshGl.numProp = 3;
                meshGl.vertProperties = mesh.m_positions;
                meshGl.triVerts.assign(mesh.m_indices.begin(), mesh.m_indices.end());
                // Manifold preserves source face IDs and the boundaries between them.
                if (preserveMaterials)
                {
                    meshGl.faceID.reserve(mesh.TriangleCount());
                    for (size_t triangle = 0; triangle < mesh.TriangleCount(); ++triangle)
                    {
                        meshGl.faceID.push_back(faceOffset + triangle);
                    }
                }
                // fill the merge vectors in case the input is not perfectly indexed
                meshGl.Merge();
                return manifold::Manifold(meshGl);
            }
        } // anonymous namespace

        bool MeshBoolean(
            const TriangleMesh& meshA, const TriangleMesh& meshB, const BooleanOperation operation,
            TriangleMesh& result)
        {
            if (meshA.TriangleCount() == 0 || meshB.TriangleCount() == 0)
            {
                return false;
            }


            const bool preserveMaterials = !meshA.m_materials.empty() || !meshB.m_materials.empty() ||
                !meshA.m_colors.empty() || !meshB.m_colors.empty();
            const manifold::Manifold manifoldA = ToManifold(meshA, 0, preserveMaterials);
            if (manifoldA.Status() != manifold::Manifold::Error::NoError)
            {
                AZ_Warning("WhiteBox", false,
                    "MeshBoolean: mesh A is not a valid manifold (manifold error %d)",
                    static_cast<int>(manifoldA.Status()));
                return false;
            }

            const manifold::Manifold manifoldB = ToManifold(meshB, meshA.TriangleCount(), preserveMaterials);
            if (manifoldB.Status() != manifold::Manifold::Error::NoError)
            {
                AZ_Warning("WhiteBox", false,
                    "MeshBoolean: mesh B is not a valid manifold (manifold error %d)",
                    static_cast<int>(manifoldB.Status()));
                return false;
            }

            const auto opType = [operation]
            {
                switch (operation)
                {
                case BooleanOperation::Union:
                    return manifold::OpType::Add;
                case BooleanOperation::Intersection:
                    return manifold::OpType::Intersect;
                case BooleanOperation::Subtraction:
                default:
                    return manifold::OpType::Subtract;
                }
            }();

            const manifold::Manifold booleanResult = manifoldA.Boolean(manifoldB, opType);
            if (booleanResult.Status() != manifold::Manifold::Error::NoError)
            {
                AZ_Warning("WhiteBox", false,
                    "MeshBoolean: boolean operation failed (manifold error %d)",
                    static_cast<int>(booleanResult.Status()));
                return false;
            }

            if (booleanResult.IsEmpty())
            {
                AZ_Warning("WhiteBox", false,
                    "MeshBoolean: boolean operation produced an empty mesh "
                    "(meshes may not intersect, or A is completely inside B)");
                return false;
            }

            const manifold::MeshGL64 outputMesh = booleanResult.GetMeshGL64();

            // copy vertex positions (vertProperties is interleaved with stride numProp,
            // the first three properties are always the position)
            const size_t outputVertexCount = outputMesh.NumVert();
            const auto numProp = static_cast<size_t>(outputMesh.numProp);
            result.m_positions.clear();
            result.m_positions.reserve(outputVertexCount * 3);
            for (size_t vertexIndex = 0; vertexIndex < outputVertexCount; ++vertexIndex)
            {
                result.m_positions.push_back(outputMesh.vertProperties[vertexIndex * numProp + 0]);
                result.m_positions.push_back(outputMesh.vertProperties[vertexIndex * numProp + 1]);
                result.m_positions.push_back(outputMesh.vertProperties[vertexIndex * numProp + 2]);
            }

            // copy triangle indices (narrowing from MeshGL64's uint64_t)
            result.m_indices.clear();
            result.m_indices.reserve(outputMesh.triVerts.size());
            for (const uint64_t index : outputMesh.triVerts)
            {
                result.m_indices.push_back(static_cast<uint32_t>(index));
            }

            result.m_materials.clear();
            result.m_colors.clear();
            if (preserveMaterials)
            {
                for (const uint64_t face : outputMesh.faceID)
                {
                    result.m_colors.push_back(face < meshA.TriangleCount()
                        ? meshA.Color(face) : meshB.Color(face - meshA.TriangleCount()));
                    result.m_materials.push_back(face < meshA.TriangleCount()
                        ? meshA.Material(face) : meshB.Material(face - meshA.TriangleCount()));
                }
            }

            // compact any duplicate vertices so the rebuilt mesh is a closed two-manifold
            WeldVertices(result, 1e-6);

            return result.TriangleCount() > 0;
        }
    } // namespace Csg
} // namespace WhiteBox
