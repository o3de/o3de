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

#pragma once

#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace RPI
    {
        class ModelKdTreeNode;

        //! Spatial structure for a single model.
        //! May contain indices pointing to triangles from multiple meshes, if a model contains multiple meshes.
        class ModelKdTree
        {
        public:

            ModelKdTree() = default;

            bool Build(const ModelAsset* model);
            bool RayIntersection(const AZ::Vector3& raySrc, const AZ::Vector3& rayDir, float& distance) const;
            void GetPenetratedBoxes(const AZ::Vector3& raySrc, const AZ::Vector3& rayDir, AZStd::vector<AZ::Aabb>& outBoxes);

            enum ESplitAxis
            {
                eSA_X = 0,
                eSA_Y,
                eSA_Z,
                eSA_Invalid
            };

            AZStd::array_view<float> GetPositionsBuffer(const ModelLodAsset::Mesh& mesh);

        private:

            void BuildRecursively(ModelKdTreeNode* pNode, const AZ::Aabb& boundbox, AZStd::vector<AZ::u32>& indices);
            bool RayIntersectionRecursively(ModelKdTreeNode* pNode, const AZ::Vector3& raySrc, const AZ::Vector3& rayDir, float& distance) const;
            void GetPenetratedBoxesRecursively(ModelKdTreeNode* pNode, const AZ::Vector3& raySrc, const AZ::Vector3& rayDir, AZStd::vector<AZ::Aabb>& outBoxes);
            void ConstructMeshList(const ModelAsset* model, const AZ::Transform& matParent);

            static const int s_MinimumVertexSizeInLeafNode = 3 * 10;

            AZStd::unique_ptr<ModelKdTreeNode> m_pRootNode;

            struct MeshData
            {
                const ModelLodAsset::Mesh* m_mesh = nullptr;
                AZStd::array_view<float> m_vertexData;
            };

            AZStd::vector<MeshData> m_meshes;

            struct SSplitInfo
            {
                AZ::Aabb m_aboveBoundbox;
                AZStd::vector<AZ::u32> m_aboveIndices;
                AZ::Aabb m_belowBoundbox;
                AZStd::vector<AZ::u32> m_belowIndices;
            };

            bool SplitNode(const AZ::Aabb& boundbox, const AZStd::vector<AZ::u32>& indices, ModelKdTree::ESplitAxis splitAxis, float splitPos, SSplitInfo& outInfo);

            AZ::Name m_positionName{ "POSITION" };

            static ESplitAxis SearchForBestSplitAxis(const AZ::Aabb& aabb, float& splitPosition);
        };

        class ModelKdTreeNode
        {
        public:
            ModelKdTreeNode() = default;

            ~ModelKdTreeNode()
            {
                for (auto& child : m_children)
                {
                    child.reset();
                }
            }
            AZ::u32 GetVertexBufferSize() const
            {
                return aznumeric_cast<AZ::u32>(m_vertexIndices.size());
            }
            float GetSplitPos() const
            {
                return m_splitPos;
            }
            void SetSplitPos(float pos)
            {
                m_splitPos = pos;
            }
            ModelKdTree::ESplitAxis GetSplitAxis() const
            {
                if (m_splitAxis == 0)
                {
                    return ModelKdTree::eSA_X;
                }
                if (m_splitAxis == 1)
                {
                    return ModelKdTree::eSA_Y;
                }
                if (m_splitAxis == 2)
                {
                    return ModelKdTree::eSA_Z;
                }
                return ModelKdTree::eSA_Invalid;
            }
            void SetSplitAxis(const ModelKdTree::ESplitAxis& axis)
            {
                m_splitAxis = axis;
            }
            bool IsLeaf() const
            {
                return m_children[0] == nullptr && m_children[1] == nullptr;
            }
            ModelKdTreeNode* GetChild(AZ::u32 nIndex) const
            {
                if (nIndex > 1)
                {
                    return nullptr;
                }
                return m_children[nIndex].get();
            }
            void SetChild(AZ::u32 nIndex, AZStd::unique_ptr<ModelKdTreeNode>&& pNode)
            {
                if (nIndex > 1)
                {
                    return;
                }
                m_children[nIndex] = AZStd::move(pNode);
            }
            const AZ::Aabb& GetBoundBox()
            {
                return m_boundBox;
            }
            void SetBoundBox(const AZ::Aabb& aabb)
            {
                m_boundBox = aabb;
            }
            void SetVertexIndexBuffer(AZStd::vector<AZ::u32>&& vertexInfos)
            {
                m_vertexIndices.swap(vertexInfos);
            }

            AZ::u32 GetVertexIndex(AZ::u32 nIndex) const
            {
                if (GetVertexBufferSize() == 1)
                {
                    return m_oneIndex & 0x00FFFFFF;
                }

                return m_vertexIndices[nIndex] & 0x00FFFFFF;
            }
            AZ::u32 GetObjIndex(AZ::u32 nIndex) const
            {
                if (GetVertexBufferSize() == 1)
                {
                    return (m_oneIndex & 0xFF000000) >> 24;
                }

                return (m_vertexIndices[nIndex] & 0xFF000000) >> 24;
            }

        private:
            union
            {
                float m_splitPos;   // Interior
                AZ::u32 m_oneIndex; // Leaf
            };

            AZStd::vector<AZ::u32> m_vertexIndices; // Leaf : high 8bits - object index, low 24bits - vertex index

            AZ::u32 m_splitAxis;  // Interior;
            AZ::Aabb m_boundBox;  // Both

            AZStd::unique_ptr<ModelKdTreeNode> m_children[2] = {};   // Interior
        };
    }
}
