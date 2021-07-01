/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include "MeshBuilderVertexAttributeLayers.h"

namespace AZ::MeshBuilder
{
    class MeshBuilder;

    class MeshBuilderSubMesh
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        MeshBuilderSubMesh(size_t materialIndex, MeshBuilder* mesh);

        size_t GetNumIndices() const { return m_indices.size(); }
        size_t GetNumPolygons() const { return m_polyVertexCounts.size(); }
        size_t GetNumJoints() const { return m_jointList.size(); }
        size_t GetMaterialIndex() const { return m_materialIndex; }
        size_t GetNumVertices() const { return m_numVertices; }
        size_t GetJoint(size_t index) const { return m_jointList[index]; }
        AZ::u8 GetPolygonVertexCount(size_t polyIndex) const { return m_polyVertexCounts[polyIndex]; }
        const MeshBuilderVertexLookup& GetVertex(size_t index) const
        {
            AZ_Assert(m_vertexOrder.size() == m_numVertices, "Call GenerateVertexOrder() first")
            return m_vertexOrder[index];
        }
        const MeshBuilder* GetMesh() const { return m_mesh; }
        size_t GetIndex(size_t index) const;

        void GenerateVertexOrder();

        void SetJoints(const AZStd::vector<size_t>& jointList) { m_jointList = jointList; }
        const AZStd::vector<size_t>& GetJoints() const { return m_jointList; }

        void AddPolygon(const AZStd::vector<MeshBuilderVertexLookup>& indices, const AZStd::vector<size_t>& jointList);
        bool CanHandlePolygon(const AZStd::vector<size_t>& orgVertexNumbers, size_t materialIndex, AZStd::vector<size_t>& outJointList) const;

        size_t CalcNumSimilarJoints(const AZStd::vector<size_t>& jointList) const;

    private:
        AZStd::vector<MeshBuilderVertexLookup> m_indices;
        AZStd::vector<MeshBuilderVertexLookup> m_vertexOrder;
        AZStd::vector<size_t> m_jointList;
        AZStd::vector<AZ::u8> m_polyVertexCounts;
        size_t m_materialIndex = InvalidIndex;
        size_t m_numVertices = 0;
        MeshBuilder* m_mesh = nullptr;

        bool CheckIfHasVertex(const MeshBuilderVertexLookup& vertex);
    };
} // namespace AZ::MeshBuilder
