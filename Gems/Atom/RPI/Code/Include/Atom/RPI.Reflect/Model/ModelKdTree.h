/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Configuration.h>
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
        class ATOM_RPI_REFLECT_API ModelKdTree
        {
        public:

            struct TriangleIndices { uint32_t index1, index2, index3; };
            using ObjectIdTriangleIndices = AZStd::tuple<AZ::u8, TriangleIndices>;

            ModelKdTree() = default;

            bool Build(const ModelAsset* model);
            //! Return if a ray intersected the model.
            //! @param raySrc The starting point of the ray.
            //! @param rayDir The direction and length of the ray (magnitude is encoded in the direction).
            //! @param[out] The normalized distance of the intersection (in the range 0.0-1.0) - to calculate the actual
            //! distance, multiply distanceNormalized by the magnitude of rayDir.
            //! @param[out] The surface normal of the intersection with the model.
            //! @return Return true if there was an intersection with the model, false otherwise.
            bool RayIntersection(
                const AZ::Vector3& raySrc, const AZ::Vector3& rayDir, float& distanceNormalized, AZ::Vector3& normal) const;
            void GetPenetratedBoxes(const AZ::Vector3& raySrc, const AZ::Vector3& rayDir, AZStd::vector<AZ::Aabb>& outBoxes);

            enum ESplitAxis
            {
                eSA_X = 0,
                eSA_Y,
                eSA_Z,
                eSA_Invalid
            };

            static AZStd::span<const float> GetPositionsBuffer(const ModelLodAsset::Mesh& mesh);

            static AZStd::span<const TriangleIndices> GetIndexBuffer(const ModelLodAsset::Mesh& mesh);

        private:

            void BuildRecursively(ModelKdTreeNode* pNode, const AZ::Aabb& boundbox, AZStd::vector<ObjectIdTriangleIndices>& indices);
            bool RayIntersectionRecursively(
                ModelKdTreeNode* pNode,
                const AZ::Vector3& raySrc,
                const AZ::Vector3& rayDir,
                float& distanceNormalized,
                AZ::Vector3& normal) const;
            void GetPenetratedBoxesRecursively(
                ModelKdTreeNode* pNode, const AZ::Vector3& raySrc, const AZ::Vector3& rayDir, AZStd::vector<AZ::Aabb>& outBoxes);
            void ConstructMeshList(const ModelAsset* model, const AZ::Transform& matParent);

            static constexpr int s_MinimumVertexSizeInLeafNode = 3 * 10;
            // Stop splitting the tree if more than 10% of the triangles are straddling the split axis
            static constexpr float s_MaximumSplitAxisStraddlingTriangles = 1.1;
            AZStd::unique_ptr<ModelKdTreeNode> m_pRootNode;

            struct MeshData
            {
                const ModelLodAsset::Mesh* m_mesh = nullptr;
                AZStd::span<const float> m_vertexData;
            };

            AZStd::vector<MeshData> m_meshes;

            struct SSplitInfo
            {
                AZ::Aabb m_aboveBoundbox;
                AZStd::vector<ObjectIdTriangleIndices> m_aboveIndices;
                AZ::Aabb m_belowBoundbox;
                AZStd::vector<ObjectIdTriangleIndices> m_belowIndices;
            };

            bool SplitNode(const AZ::Aabb& boundbox, const AZStd::vector<ObjectIdTriangleIndices>& indices, ModelKdTree::ESplitAxis splitAxis, float splitPos, SSplitInfo& outInfo);

            static AZStd::tuple<ESplitAxis, float> SearchForBestSplitAxis(const AZ::Aabb& aabb);
        };

        class ATOM_RPI_REFLECT_API ModelKdTreeNode
        {
        public:
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
                return m_splitAxis;
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
            void SetVertexIndexBuffer(AZStd::vector<AZStd::tuple<AZ::u8, ModelKdTree::TriangleIndices>>&& vertexInfos)
            {
                m_vertexIndices.swap(vertexInfos);
            }

            ModelKdTree::TriangleIndices GetVertexIndex(AZ::u32 nIndex) const
            {
                return AZStd::get<1>(m_vertexIndices[nIndex]);
            }
            AZ::u8 GetObjIndex(AZ::u32 nIndex) const
            {
                return AZStd::get<0>(m_vertexIndices[nIndex]);
            }

        private:
            AZ::Aabb m_boundBox{};  // Both
            AZStd::vector<AZStd::tuple<AZ::u8, ModelKdTree::TriangleIndices>> m_vertexIndices;
            AZStd::array<AZStd::unique_ptr<ModelKdTreeNode>, 2> m_children{};   // Interior
            float m_splitPos{}; // Interior
            ModelKdTree::ESplitAxis m_splitAxis = ModelKdTree::eSA_Invalid;  // Interior;
        };
    }
}
