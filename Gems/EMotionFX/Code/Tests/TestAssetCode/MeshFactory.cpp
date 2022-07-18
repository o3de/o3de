/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/TestAssetCode/MeshFactory.h>

#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/SubMesh.h>
#include <EMotionFX/Source/SkinningInfoVertexAttributeLayer.h>
namespace EMotionFX
{
    EMotionFX::Mesh* MeshFactory::Create(
        const AZStd::vector<AZ::u32>& indices,
        const AZStd::vector<AZ::Vector3>& vertices,
        const AZStd::vector<AZ::Vector3>& normals,
        const AZStd::vector<AZ::Vector2>& uvs,
        const AZStd::vector<VertexSkinInfluences>& skinningInfo
    ) {
        const AZ::u32 vertCount = aznumeric_cast<AZ::u32>(vertices.size());
        const AZ::u32 faceCount = aznumeric_cast<AZ::u32>(indices.size()) / 3;

        auto* mesh = EMotionFX::Mesh::Create(vertCount, aznumeric_caster(indices.size()), faceCount, vertCount, false);

        // Skinning info
        if (!skinningInfo.empty() && skinningInfo.size() == vertices.size())
        {
            auto* skinningLayer = EMotionFX::SkinningInfoVertexAttributeLayer::Create(vertCount);
            for (size_t vertex = 0; vertex < skinningInfo.size(); ++vertex)
            {
                for (const auto& [nodeNr, weight] : skinningInfo[vertex])
                {
                    skinningLayer->AddInfluence(static_cast<AZ::u32>(vertex), aznumeric_caster(nodeNr), weight, 0);
                }
            }

            mesh->AddSharedVertexAttributeLayer(skinningLayer);
        }

        // // Original vertex numbers
        mesh->CreateVertexAttribute<EMotionFX::AttributeType::OrginalVertexNumber>(indices, true);
        // // The positions layer
        mesh->CreateVertexAttribute<EMotionFX::AttributeType::Position>(vertices, true);
        // // The normals layer
        mesh->CreateVertexAttribute<EMotionFX::AttributeType::Normal>(normals, true);

        // The UVs layer.
        if (!uvs.empty() && uvs.size() == vertices.size())
        {
            mesh->CreateVertexAttribute<EMotionFX::AttributeType::UVCoords>(uvs, true);
        }

        auto* subMesh = EMotionFX::SubMesh::Create(
            /*parentMesh=*/    mesh,
            /*startVertex=*/   0,
            /*startIndex=*/    0,
            /*startPolygon=*/  0,
            /*numVerts=*/      mesh->GetNumVertices(),
            /*numIndices=*/    mesh->GetNumIndices(),
            /*numPolygons=*/   mesh->GetNumPolygons(),
            /*numBones=*/      aznumeric_caster(skinningInfo.size())
        );
        mesh->AddSubMesh(subMesh);
        if (!skinningInfo.empty() && skinningInfo.size() == vertices.size())
        {
            for (size_t vertex = 0; vertex < skinningInfo.size(); ++vertex)
            {
                subMesh->SetBone(aznumeric_caster(vertex), aznumeric_caster(vertex));
            }
        }

        AZ_PUSH_DISABLE_WARNING_MSVC(4244); //  warning C4244: '=': conversion from 'const int' to 'uint8', possible loss of data
        AZStd::fill(mesh->GetPolygonVertexCounts(), mesh->GetPolygonVertexCounts() + faceCount, 3);
        AZ_POP_DISABLE_WARNING_MSVC
        AZStd::copy(indices.begin(), indices.end(), mesh->GetIndices());

        return mesh;
    }
} // namespace EMotionFX
