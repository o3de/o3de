/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/numeric.h>
#include <AzCore/std/limits.h>
#include <Atom/RPI.Reflect/Model/ModelKdTree.h>
#include <AzCore/Math/IntersectSegment.h>

namespace AZ
{
    namespace RPI
    {
        AZStd::tuple<ModelKdTree::ESplitAxis, float> ModelKdTree::SearchForBestSplitAxis(const AZ::Aabb& aabb)
        {
            const float xsize = aabb.GetXExtent();
            const float ysize = aabb.GetYExtent();
            const float zsize = aabb.GetZExtent();

            if (xsize >= ysize && xsize >= zsize)
            {
                return {ModelKdTree::eSA_X, aabb.GetMin().GetX() + xsize * 0.5f};
            }
            if (ysize >= zsize && ysize >= xsize)
            {
                return {ModelKdTree::eSA_Y, aabb.GetMin().GetY() + ysize * 0.5f};
            }
            return {ModelKdTree::eSA_Z, aabb.GetMin().GetZ() + zsize * 0.5f};
        }

        bool ModelKdTree::SplitNode(const AZ::Aabb& boundbox, const AZStd::vector<ObjectIdTriangleIndices>& indices, ModelKdTree::ESplitAxis splitAxis, float splitPos, SSplitInfo& outInfo)
        {
            if (splitAxis != ModelKdTree::eSA_X && splitAxis != ModelKdTree::eSA_Y && splitAxis != ModelKdTree::eSA_Z)
            {
                return false;
            }

            outInfo.m_aboveBoundbox = boundbox;
            outInfo.m_belowBoundbox = boundbox;

            {
                Vector3 maxBound = outInfo.m_aboveBoundbox.GetMax();
                maxBound.SetElement(splitAxis, splitPos);
                outInfo.m_aboveBoundbox.SetMax(maxBound);
            }
            {
                Vector3 minBound = outInfo.m_belowBoundbox.GetMin();
                minBound.SetElement(splitAxis, splitPos);
                outInfo.m_belowBoundbox.SetMin(minBound);
            }

            const AZ::u32 iIndexSize = aznumeric_cast<AZ::u32>(indices.size());
            outInfo.m_aboveIndices.reserve(iIndexSize);
            outInfo.m_belowIndices.reserve(iIndexSize);

            for (const auto& [nObjIndex, triangleIndices] : indices)
            {
                const auto& [first, second, third] = triangleIndices;

                const AZStd::array_view<float>& positionBuffer = m_meshes[nObjIndex].m_vertexData;

                if (positionBuffer.empty())
                {
                    continue;
                }

                // If the split axis is Y, this uses a Vector3 to store the Y positions of each vertex in the triangle.
                const AZStd::array<const float, 3> triangleVerticesValuesForThisSplitAxis {
                    positionBuffer[first * 3 + splitAxis], positionBuffer[second * 3 + splitAxis], positionBuffer[third * 3 + splitAxis]
                };

                if (AZStd::any_of(begin(triangleVerticesValuesForThisSplitAxis), end(triangleVerticesValuesForThisSplitAxis), [splitPos](const float value) { return value < splitPos; }))
                {
                    outInfo.m_aboveIndices.emplace_back(nObjIndex, triangleIndices);
                }
                if (AZStd::any_of(begin(triangleVerticesValuesForThisSplitAxis), end(triangleVerticesValuesForThisSplitAxis), [splitPos](const float value) { return value >= splitPos; }))
                {
                    outInfo.m_belowIndices.emplace_back(nObjIndex, triangleIndices);
                }
            }

            // If either the top or bottom contain all the input indices, the triangles are too close to cut any
            // further and the split failed
            // Additionally, if too many triangles straddle the split-axis,
            // the triangles are too close and the split failed
            // [ATOM-15944] - Use a more sophisticated method to terminate KdTree generation
            return indices.size() != outInfo.m_aboveIndices.size() && indices.size() != outInfo.m_belowIndices.size()
                && aznumeric_cast<float>(outInfo.m_aboveIndices.size() + outInfo.m_belowIndices.size()) / aznumeric_cast<float>(indices.size()) < s_MaximumSplitAxisStraddlingTriangles;
        }

        bool ModelKdTree::Build(const ModelAsset* model)
        {
            if (model == nullptr)
            {
                return false;
            }

            ConstructMeshList(model, AZ::Transform::CreateIdentity());

            AZ::Aabb entireBoundBox = AZ::Aabb::CreateNull();

            // indices with object ids
            AZStd::vector<ObjectIdTriangleIndices> indices;

            const size_t totalSizeNeed = AZStd::accumulate(begin(m_meshes), end(m_meshes), size_t{0}, [](const size_t current, const MeshData& data)
            {
                return current + data.m_mesh->GetVertexCount();
            });
            indices.reserve(totalSizeNeed);

            for (AZ::u8 meshIndex = 0, meshCount = aznumeric_caster(m_meshes.size()); meshIndex < meshCount; ++meshIndex)
            {
                const AZStd::array_view<float> positionBuffer = m_meshes[meshIndex].m_vertexData;
                for (size_t positionIndex = 0; positionIndex < positionBuffer.size(); positionIndex += 3)
                {
                    entireBoundBox.AddPoint({positionBuffer[positionIndex], positionBuffer[positionIndex + 1], positionBuffer[positionIndex + 2]});
                }

                // The view returned by GetIndexBuffer returns a tuple<uint32_t, uint32_t, uint32_t>, in order to read
                // 3 values at a time from the raw index buffer. It uses a reinterpret_cast to accomplish this. The
                // cast results in the order of the indices being reversed, which is why they are read [third, second,
                // first] here.
                for (const auto& [thirdIndex, secondIndex, firstIndex] : GetIndexBuffer(*m_meshes[meshIndex].m_mesh))
                {
                    indices.emplace_back(meshIndex, TriangleIndices{firstIndex, secondIndex, thirdIndex});
                }
            }

            m_pRootNode = AZStd::make_unique<ModelKdTreeNode>();

            BuildRecursively(m_pRootNode.get(), entireBoundBox, indices);

            return true;
        }

        AZStd::array_view<float> ModelKdTree::GetPositionsBuffer(const ModelLodAsset::Mesh& mesh)
        {
            AZStd::array_view<float> positionBuffer = mesh.GetSemanticBufferTyped<float>(AZ::Name{"POSITION"});
            AZ_Warning("ModelKdTree", !positionBuffer.empty(), "Could not find position buffers in a mesh");
            return positionBuffer;
        }

        AZStd::array_view<ModelKdTree::TriangleIndices> ModelKdTree::GetIndexBuffer(const ModelLodAsset::Mesh& mesh)
        {
            return mesh.GetIndexBufferTyped<ModelKdTree::TriangleIndices>();
        }

        void ModelKdTree::BuildRecursively(ModelKdTreeNode* pNode, const AZ::Aabb& boundbox, AZStd::vector<ObjectIdTriangleIndices>& indices)
        {
            pNode->SetBoundBox(boundbox);

            if (indices.size() <= s_MinimumVertexSizeInLeafNode)
            {
                pNode->SetVertexIndexBuffer(AZStd::move(indices));
                return;
            }

            const auto [splitAxis, splitPos] = SearchForBestSplitAxis(boundbox);
            pNode->SetSplitAxis(splitAxis);
            pNode->SetSplitPos(splitPos);

            SSplitInfo splitInfo;
            if (!SplitNode(boundbox, indices, splitAxis, splitPos, splitInfo))
            {
                pNode->SetVertexIndexBuffer(AZStd::move(indices));
                return;
            }

            if (splitInfo.m_aboveIndices.empty() || splitInfo.m_belowIndices.empty())
            {
                pNode->SetVertexIndexBuffer(AZStd::move(indices));
                return;
            }

            pNode->SetChild(0, AZStd::make_unique<ModelKdTreeNode>());
            pNode->SetChild(1, AZStd::make_unique<ModelKdTreeNode>());

            BuildRecursively(pNode->GetChild(0), splitInfo.m_aboveBoundbox, splitInfo.m_aboveIndices);
            BuildRecursively(pNode->GetChild(1), splitInfo.m_belowBoundbox, splitInfo.m_belowIndices);
        }

        void ModelKdTree::ConstructMeshList(const ModelAsset* model, [[maybe_unused]] const AZ::Transform& matParent)
        {
            if (model == nullptr || model->GetLodAssets().empty())
            {
                return;
            }

            if (ModelLodAsset* lodAssetPtr = model->GetLodAssets()[0].Get())
            {
                AZ_Warning("ModelKdTree", lodAssetPtr->GetMeshes().size() <= AZStd::numeric_limits<AZ::u8>::max() + 1,
                    "KdTree generation doesn't support models with greater than 256 meshes. RayIntersection results will be incorrect "
                    "unless the meshes are merged or broken up into multiple models");
                const size_t size = AZStd::min<size_t>(lodAssetPtr->GetMeshes().size(), AZStd::numeric_limits<AZ::u8>::max() + 1);
                m_meshes.reserve(size);
                AZStd::transform(
                    lodAssetPtr->GetMeshes().begin(), AZStd::next(lodAssetPtr->GetMeshes().begin(), size),
                    AZStd::back_inserter(m_meshes),
                    [](const auto& mesh) { return MeshData{&mesh, GetPositionsBuffer(mesh)}; }
                );
            }
        }

        bool ModelKdTree::RayIntersection(
            const AZ::Vector3& raySrc, const AZ::Vector3& rayDir, float& distanceNormalized, AZ::Vector3& normal) const
        {
            float shortestDistanceNormalized = AZStd::numeric_limits<float>::max();
            if (RayIntersectionRecursively(m_pRootNode.get(), raySrc, rayDir, shortestDistanceNormalized, normal))
            {
                distanceNormalized = shortestDistanceNormalized;
                return true;
            }

            return false;
        }

        bool ModelKdTree::RayIntersectionRecursively(
            ModelKdTreeNode* pNode,
            const AZ::Vector3& raySrc,
            const AZ::Vector3& rayDir,
            float& distanceNormalized,
            AZ::Vector3& normal) const
        {
            using Intersect::IntersectRayAABB2;
            using Intersect::IntersectSegmentTriangleCCW;
            using Intersect::ISECT_RAY_AABB_NONE;

            if (!pNode)
            {
                return false;
            }

            float start, end;
            if (IntersectRayAABB2(raySrc, rayDir.GetReciprocal(), pNode->GetBoundBox(), start, end) == ISECT_RAY_AABB_NONE)
            {
                return false;
            }

            if (start > distanceNormalized)
            {
                return false;
            }

            if (pNode->IsLeaf())
            {
                if (m_meshes.empty())
                {
                    return false;
                }

                const AZ::u32 nVBuffSize = pNode->GetVertexBufferSize();
                if (nVBuffSize == 0)
                {
                    return false;
                }

                float nearestDistanceNormalized = distanceNormalized;
                for (AZ::u32 i = 0; i < nVBuffSize; ++i)
                {
                    const auto& [first, second, third] = pNode->GetVertexIndex(i);
                    const AZ::u32 nObjIndex = pNode->GetObjIndex(i);

                    const AZStd::array_view<float> positionBuffer = m_meshes[nObjIndex].m_vertexData;

                    if (positionBuffer.empty())
                    {
                        continue;
                    }

                    const AZStd::array trianglePoints {
                        AZ::Vector3{positionBuffer[first * 3 + 0], positionBuffer[first * 3 + 1], positionBuffer[first * 3 + 2]},
                        AZ::Vector3{positionBuffer[second * 3 + 0], positionBuffer[second * 3 + 1], positionBuffer[second * 3 + 2]},
                        AZ::Vector3{positionBuffer[third * 3 + 0], positionBuffer[third * 3 + 1], positionBuffer[third * 3 + 2]},
                    };

                    float hitDistanceNormalized;
                    AZ::Vector3 intersectionNormal;
                    const AZ::Vector3 rayEnd = raySrc + rayDir;
                    if (IntersectSegmentTriangleCCW(raySrc, rayEnd, trianglePoints[0], trianglePoints[1], trianglePoints[2],
                        intersectionNormal, hitDistanceNormalized) != ISECT_RAY_AABB_NONE)
                    {
                        if (nearestDistanceNormalized > hitDistanceNormalized)
                        {
                            normal = intersectionNormal;
                            nearestDistanceNormalized = hitDistanceNormalized;
                        }
                    }
                }

                if (nearestDistanceNormalized < distanceNormalized)
                {
                    distanceNormalized = nearestDistanceNormalized;
                    return true;
                }

                return false;
            }

            // running both sides to find the closest intersection
            const bool bFoundChild0 = RayIntersectionRecursively(pNode->GetChild(0), raySrc, rayDir, distanceNormalized, normal);
            const bool bFoundChild1 = RayIntersectionRecursively(pNode->GetChild(1), raySrc, rayDir, distanceNormalized, normal);

            return bFoundChild0 || bFoundChild1;
        }

        void ModelKdTree::GetPenetratedBoxes(const AZ::Vector3& raySrc, const AZ::Vector3& rayDir, AZStd::vector<AZ::Aabb>& outBoxes)
        {
            GetPenetratedBoxesRecursively(m_pRootNode.get(), raySrc, rayDir, outBoxes);
        }

        void ModelKdTree::GetPenetratedBoxesRecursively(ModelKdTreeNode* pNode, const AZ::Vector3& raySrc, const AZ::Vector3& rayDir, AZStd::vector<AZ::Aabb>& outBoxes)
        {
            AZ::Vector3 ignoreNormal;
            float ignore;
            if (!pNode || (!pNode->GetBoundBox().Contains(raySrc) &&
                (AZ::Intersect::IntersectRayAABB(raySrc, rayDir, rayDir.GetReciprocal(), pNode->GetBoundBox(),
                    ignore, ignore, ignoreNormal)) == Intersect::ISECT_RAY_AABB_NONE))
            {
                return;
            }

            outBoxes.push_back(pNode->GetBoundBox());

            GetPenetratedBoxesRecursively(pNode->GetChild(0), raySrc, rayDir, outBoxes);
            GetPenetratedBoxesRecursively(pNode->GetChild(1), raySrc, rayDir, outBoxes);
        }
    } // namespace RPI
} // namespace AZ
