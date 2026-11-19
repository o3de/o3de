/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxMeshSimplify.h"

#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/containers/unordered_map.h>
#include <cmath>

// OpenMesh is not warning-level-4 clean, and the Decimater templates instantiate in this file, so the whole file is quiet.
#if defined(_MSC_VER)
#pragma warning(push, 0)
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <OpenMesh/Tools/Decimater/DecimaterT.hh>
#include <OpenMesh/Tools/Decimater/ModNormalFlippingT.hh>
#include <OpenMesh/Tools/Decimater/ModQuadricT.hh>

namespace WhiteBox
{
    namespace
    {
        using SimplifyMesh = OpenMesh::TriMesh_ArrayKernelT<>;
    }

    bool SimplifyTriangles(
        AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const float keepFraction,
        const float maxNormalDeviationDegrees)
    {
        const size_t triangleCount = indices.size() / 3;
        if (keepFraction >= 1.0f || triangleCount < 8)
        {
            return false;
        }

        SimplifyMesh mesh;
        mesh.request_vertex_status();
        mesh.request_edge_status();
        mesh.request_face_status();
        mesh.request_face_normals();

        AZStd::vector<SimplifyMesh::VertexHandle> handles;
        handles.reserve(vertices.size());
        for (const AZ::Vector3& vertex : vertices)
        {
            handles.push_back(mesh.add_vertex(SimplifyMesh::Point(vertex.GetX(), vertex.GetY(), vertex.GetZ())));
        }
        // Triangles that would make the half-edge mesh non-manifold are kept aside and added back untouched.
        AZStd::vector<AZ::u32> untouched;
        for (size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            const AZ::u32 a = indices[i];
            const AZ::u32 b = indices[i + 1];
            const AZ::u32 c = indices[i + 2];
            if (a >= handles.size() || b >= handles.size() || c >= handles.size())
            {
                continue;
            }
            if (!mesh.add_face(handles[a], handles[b], handles[c]).is_valid())
            {
                untouched.insert(untouched.end(), { a, b, c });
            }
        }
        if (mesh.n_faces() < 8)
        {
            return false;
        }
        mesh.update_face_normals();

        OpenMesh::Decimater::DecimaterT<SimplifyMesh> decimater(mesh);
        OpenMesh::Decimater::ModQuadricT<SimplifyMesh>::Handle quadric;
        decimater.add(quadric);
        decimater.module(quadric).unset_max_err(); // the triangle budget decides, the error only orders the collapses
        OpenMesh::Decimater::ModNormalFlippingT<SimplifyMesh>::Handle flipping;
        decimater.add(flipping);
        decimater.module(flipping).set_max_normal_deviation(AZ::GetClamp(maxNormalDeviationDegrees, 1.0f, 180.0f));
        if (!decimater.initialize())
        {
            return false;
        }
        const auto target = static_cast<size_t>(std::ceil(static_cast<double>(mesh.n_faces()) * AZ::GetClamp(keepFraction, 0.001f, 1.0f)));
        decimater.decimate_to_faces(0, AZStd::max<size_t>(target, 4));
        mesh.garbage_collection();

        AZStd::vector<AZ::Vector3> outVertices;
        AZStd::vector<AZ::u32> outIndices;
        outVertices.reserve(mesh.n_vertices());
        for (const auto vertex : mesh.vertices())
        {
            const auto& point = mesh.point(vertex);
            outVertices.emplace_back(point[0], point[1], point[2]);
        }
        outIndices.reserve(mesh.n_faces() * 3 + untouched.size());
        for (const auto face : mesh.faces())
        {
            for (const auto vertex : mesh.fv_range(face))
            {
                outIndices.push_back(static_cast<AZ::u32>(vertex.idx()));
            }
        }
        // Set-aside triangles reference the original vertices, which garbage collection renumbered, so they bring their own.
        AZStd::unordered_map<AZ::u32, AZ::u32> copied;
        for (const AZ::u32 original : untouched)
        {
            auto [slot, fresh] = copied.emplace(original, static_cast<AZ::u32>(outVertices.size()));
            if (fresh)
            {
                outVertices.push_back(vertices[original]);
            }
            outIndices.push_back(slot->second);
        }
        if (outIndices.size() < 3)
        {
            return false;
        }
        vertices = AZStd::move(outVertices);
        indices = AZStd::move(outIndices);
        return true;
    }
} // namespace WhiteBox

#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif
