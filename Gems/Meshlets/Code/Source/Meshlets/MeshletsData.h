#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/list.h>

#include "../../External/meshoptimizer.h"
//#include <meshoptimizer.h>

#pragma optimize("", off)

namespace AZ
{
    namespace RPI
    {
        class MeshDrawPacket;
    }

    namespace Meshlets
    {
        enum class ComputeStreamsSemantics : uint8_t
        {
            Indices = 0,
            UVs,            // for debug coloring purposes
            MeshletsData,
            MehsletsTriangles,
            MeshletsIndicesIndirection,
            NumBufferStreams
        };

        enum class RenderStreamsSemantics : uint8_t
        {
            Indices = 0,
            Positions,
            Normals,
            UVs,
            Tangents,
            BiTangents,
            NumBufferStreams
        };

        struct MeshletDescriptor
        {
            //! Offset into the indirect indices array representing the global index of
            //! all the meshlet vertices.
            //! The Indirect vertices array is built as follows:
            //!     std::vector<uint32_t> indirectIndices;
            //!     indirectIndices = { { meshlet 1 vertex indices }, { meshlet 2 }, .. { meshlet n} }
            uint32_t vertexOffset;      // In uint32_t steps

            //! Offset into the global meshlets triangleIndices array represented as:
            //!     std::vector<uint8_t> triangleIndices;
            //!     triangleIndices = { {meshlet 1 local indices group}, ... { meshlet n} }
            //! The local indices are an 8 bits index that can represent up to 256 entries.
            uint32_t triangleOffset;    // In bytes from the start of the array

            //! Finding a vertex within the meshlet is done like that:
            //!     triangleOffset = currentMeshlet.triangleOffset + meshletTrIndex * 3;
            //!     localIndex_i = meshletTriangles[triangleOffset + i];    // i = triangle vertex index 0..2
            //!     vertexIndex_i =  indirectIndices[currentMeshlet.vertexOffset + localIndex_i];

            //! Amount of vertices and triangle for the mesh - based on this the arrays
            //! indirectIndices and triangleIndices are created per meshlet.
            uint32_t vertexCount;
            uint32_t triangleCount;
        };

        struct GeneratorVertex
        {
            float px, py, pz;
            float nx, ny, nz;
            float tx, ty;
        };

        struct GeneratorMesh
        {
            std::vector<GeneratorVertex> vertices;
            std::vector<unsigned int> indices;
        };

        struct MeshletsData
        {
            std::vector<meshopt_Meshlet> Descriptors;
            std::vector<uint32_t> EncodedTriangles;	    // Meshlet triangles local indices [0..256]
            std::vector<uint32_t> IndicesIndirection;   // Vertex Index indirection map - local to global

            bool ValidateData(uint32_t vtxCount)
            {
                bool validData = true;
                for (uint32_t meshletId = 0; meshletId < Descriptors.size(); ++meshletId)
                {
                    meshopt_Meshlet& meshlet = Descriptors[meshletId];

                    for (uint32_t triIdx = 0; triIdx < meshlet.triangle_count; ++triIdx)
                    {
                        uint32_t encodedTri = EncodedTriangles[meshlet.triangle_offset + triIdx];
                        // Next bring decode the uint32_t and separate into three elements.
                        uint8_t vtxIndirectIndex[3] = {
                            (encodedTri >> 0) & 0xff,
                            (encodedTri >> 8) & 0xff,
                            (encodedTri >> 16) & 0xff
                        };

                        for (uint32_t vtx = 0; vtx < 3; ++vtx)
                        {
                            uint32_t vtxIndex = IndicesIndirection[meshlet.vertex_offset + vtxIndirectIndex[vtx]];
                            if (vtxIndex >= vtxCount)
                            {
                                AZ_Warning("Meshlets", false, "Invalid triangle vertex index [%d] - maximum allowed [%d]",
                                    vtxIndex, vtxCount);
                                EncodedTriangles[meshlet.triangle_offset + triIdx] = 0;  // Decimate the triangle to the first vertex
                                validData = false;
                                break;
                            }
                        }
                    }
                }
                return validData;
            }
            // Given triangle local index vector, convert the indices to an encoded
            // triangle vector where every entry of uint32_t represents three indices.
            // Although it is not as optimal (25% more space), it fits the packing
            // of data for the GPU using a uint32_t buffer.
            uint32_t EncodeTrianglesData(std::vector<unsigned char> triangles)
            {
                // Start by converting the per byte indices into encoded triangles indices
                uint32_t triNum = (uint32_t) triangles.size() / 3;
                EncodedTriangles.resize(triNum);
                for (uint32_t tri = 0, orgTri = 0; tri < triNum; ++tri, orgTri += 3)
                {
                    EncodedTriangles[tri] = 0x00ffffff & 
                        (triangles[orgTri + 0] << 0) |
                        (triangles[orgTri + 1] << 8) |
                        (triangles[orgTri + 2] << 16);
                }

                // Convert the per index byte offsets into per triangle uint32 offsets
                for (uint32_t meshletId = 0; meshletId < Descriptors.size(); ++meshletId)
                {
                    Descriptors[meshletId].triangle_offset /= 3;
                }

                return triNum;
            }
        };

    } // namespace Meshlets
} // namespace AZ

#pragma optimize("", on)

