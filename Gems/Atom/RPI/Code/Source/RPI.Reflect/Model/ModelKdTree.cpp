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

#include <Atom/RPI.Reflect/Model/ModelKdTree.h>
#include <AzCore/Math/IntersectSegment.h>

namespace AZ
{
    namespace RPI
    {
        ModelKdTree::ESplitAxis ModelKdTree::SearchForBestSplitAxis(const AZ::Aabb& aabb, float& splitPosition)
        {
            const float xsize = aabb.GetXExtent();
            const float ysize = aabb.GetYExtent();
            const float zsize = aabb.GetZExtent();

            ModelKdTree::ESplitAxis axis;
            if (xsize >= ysize && xsize >= zsize)
            {
                axis = ModelKdTree::eSA_X;
                splitPosition = aabb.GetMin().GetX() + xsize * 0.5f;
            }
            else if (ysize >= zsize && ysize >= xsize)
            {
                axis = ModelKdTree::eSA_Y;
                splitPosition = aabb.GetMin().GetY() + ysize * 0.5f;
            }
            else
            {
                axis = ModelKdTree::eSA_Z;
                splitPosition = aabb.GetMin().GetZ() + zsize * 0.5f;
            }

            return axis;
        }

        bool ModelKdTree::SplitNode(const AZ::Aabb& boundbox, const AZStd::vector<AZ::u32>& indices, ModelKdTree::ESplitAxis splitAxis, float splitPos, SSplitInfo& outInfo)
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

            AZStd::array<AZ::Vector3, 3> triangleVertex;
            for (AZ::u32 i = 0; i <= iIndexSize - 3; i += 3)
            {
                const AZ::u32 nObjIndex = (indices[i] & 0xFF000000) >> 24; // asuming that all 3 verices belong to the same triangle from the same object
                const AZ::u32 nVertexIndices[3] = { indices[i] & 0xFFFFFF, indices[i + 1] & 0xFFFFFF, indices[i + 2] & 0xFFFFFF };

                const AZStd::array_view<float>& positionBuffer = m_meshes[nObjIndex].m_vertexData;

                if (positionBuffer.empty() == false)
                {
                    for (AZStd::size_t triangleVertexIndex = 0; triangleVertexIndex < triangleVertex.size(); ++triangleVertexIndex)
                    {
                        triangleVertex[triangleVertexIndex].Set(const_cast<float*>(positionBuffer.data() + 3 * nVertexIndices[triangleVertexIndex]));
                    }
                }
                else
                {
                    continue;
                }

                if (triangleVertex[0].GetElement(splitAxis) < splitPos || triangleVertex[1].GetElement(splitAxis) < splitPos || triangleVertex[2].GetElement(splitAxis) < splitPos)
                {
                    outInfo.m_aboveIndices.push_back(indices[i + 0]);
                    outInfo.m_aboveIndices.push_back(indices[i + 1]);
                    outInfo.m_aboveIndices.push_back(indices[i + 2]);
                }
                if (triangleVertex[0].GetElement(splitAxis) >= splitPos || triangleVertex[1].GetElement(splitAxis) >= splitPos || triangleVertex[2].GetElement(splitAxis) >= splitPos)
                {
                    outInfo.m_belowIndices.push_back(indices[i + 0]);
                    outInfo.m_belowIndices.push_back(indices[i + 1]);
                    outInfo.m_belowIndices.push_back(indices[i + 2]);
                }
            }

            if (indices.size() == outInfo.m_aboveIndices.size() || indices.size() == outInfo.m_belowIndices.size())
            {
                // triangles are too close to cut any further
                return false;
            }

            return true;
        }

        bool ModelKdTree::Build(const ModelAsset* model)
        {
            if (model == nullptr)
            {
                return false;
            }

            ConstructMeshList(model, AZ::Transform::CreateIdentity());

            AZ::Aabb entireBoundBox;
            entireBoundBox.SetNull();

            // indices with object ids
            AZStd::vector<AZ::u32> indices;

            int totalSizeNeed = 0;
            for (const MeshData& data : m_meshes)
            {
                totalSizeNeed += data.m_mesh->GetVertexCount();
            }
            indices.reserve(totalSizeNeed);

            AZ::Vector3 vertex;

            for (AZ::u32 meshIndex = 0, meshCount = aznumeric_cast<AZ::u32>(m_meshes.size()); meshIndex < meshCount; ++meshIndex)
            {
                AZStd::array_view<float> positionBuffer = m_meshes[meshIndex].m_vertexData;
                if (positionBuffer.empty() == false)
                {
                    const int nVertexCount = m_meshes[meshIndex].m_mesh->GetVertexCount();
                    for (int k = 0; k < nVertexCount; ++k)
                    {
                        vertex.Set(const_cast<float*>((positionBuffer.data() + 3 * k)));

                        entireBoundBox.AddPoint(vertex);
                        indices.push_back((meshIndex << 24) | k);
                    }
                }
            }

            m_pRootNode = AZStd::make_unique<ModelKdTreeNode>();

            BuildRecursively(m_pRootNode.get(), entireBoundBox, indices);

            return true;
        }

        AZStd::array_view<float> ModelKdTree::GetPositionsBuffer(const ModelLodAsset::Mesh& mesh)
        {
            const AZStd::array_view<uint8_t> positionRawBuffer = mesh.GetSemanticBuffer(m_positionName);
            if (positionRawBuffer.empty() == false)
            {
                AZStd::array_view<float> floatBuffer(reinterpret_cast<const float*>(positionRawBuffer.data()), positionRawBuffer.size() / 12);
                return floatBuffer;
            }

            AZ_Warning("ModelKdTree", false, "Could not find position buffers in a mesh");
            return {};
        }

        void ModelKdTree::BuildRecursively(ModelKdTreeNode* pNode, const AZ::Aabb& boundbox, AZStd::vector<AZ::u32>& indices)
        {
            pNode->SetBoundBox(boundbox);

            if (indices.size() <= s_MinimumVertexSizeInLeafNode)
            {
                pNode->SetVertexIndexBuffer(AZStd::move(indices));
                return;
            }

            float splitPos(0);
            const ESplitAxis splitAxis = SearchForBestSplitAxis(boundbox, splitPos);
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
            if (model == nullptr)
            {
                return;
            }

            if (model->GetLodAssets().empty() == false)
            {
                if (ModelLodAsset* loadAssetPtr = model->GetLodAssets()[0].Get())
                {
                    for (const ModelLodAsset::Mesh& data : loadAssetPtr->GetMeshes())
                    {
                        m_meshes.push_back({ &data, GetPositionsBuffer(data) });
                    }
                }
            }
        }

        bool ModelKdTree::RayIntersection(const AZ::Vector3& raySrc, const AZ::Vector3& rayDir, float& distance) const
        {
            return RayIntersectionRecursively(m_pRootNode.get(), raySrc, rayDir, distance);
        }

        bool ModelKdTree::RayIntersectionRecursively(ModelKdTreeNode* pNode, const AZ::Vector3& raySrc, const AZ::Vector3& rayDir, float& distance) const
        {
            if (!pNode)
            {
                return false;
            }

            float start, end;
            if (AZ::Intersect::IntersectRayAABB2(raySrc, rayDir.GetReciprocal(), pNode->GetBoundBox(), start, end) == Intersect::ISECT_RAY_AABB_NONE)
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

                AZ::Vector3 ignoreNormal;
                float hitDistanceNormalized;
                const float maxDist(FLT_MAX);
                float nearestDist = maxDist;

                for (AZ::u32 i = 0; i <= nVBuffSize - 3; i += 3)
                {
                    const AZ::u32 nVertexIndex = pNode->GetVertexIndex(i);
                    const AZ::u32 nObjIndex = pNode->GetObjIndex(i);

                    AZStd::array_view<float> positionBuffer = m_meshes[nObjIndex].m_vertexData;

                    AZStd::array<AZ::Vector3, 3> trianglePoints;
                    if (positionBuffer.empty() == false)
                    {
                        trianglePoints[0].Set(const_cast<float*>(positionBuffer.data() + 3 * nVertexIndex));
                        trianglePoints[1].Set(const_cast<float*>(positionBuffer.data() + 3 * pNode->GetVertexIndex(i + 1)));
                        trianglePoints[2].Set(const_cast<float*>(positionBuffer.data() + 3 * pNode->GetVertexIndex(i + 2)));
                    }
                    else
                    {
                        continue;
                    }

                    const AZ::Vector3 rayEnd = raySrc + rayDir * distance;

                    if (AZ::Intersect::IntersectSegmentTriangleCCW(raySrc, rayEnd, trianglePoints[0], trianglePoints[1], trianglePoints[2],
                        ignoreNormal, hitDistanceNormalized) != Intersect::ISECT_RAY_AABB_NONE)
                    {
                        float hitDistance = hitDistanceNormalized * distance;
                        nearestDist = AZStd::GetMin(nearestDist, hitDistance);
                    }
                }

                if (nearestDist < maxDist)
                {
                    distance = AZStd::GetMin(distance, nearestDist);
                    return true;
                }

                return false;
            }

            // running both sides to find the closest intersection
            const bool bFoundChild0 = RayIntersectionRecursively(pNode->GetChild(0), raySrc, rayDir, distance);
            const bool bFoundChild1 = RayIntersectionRecursively(pNode->GetChild(1), raySrc, rayDir, distance);

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
    }
}
