/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/tuple.h>
#include <AzCore/std/typetraits/is_convertible.h>
#include <AzCore/std/typetraits/is_same.h>
#include "MeshBuilderVertexAttributeLayers.h"
#include "MeshBuilderSubMesh.h"
#include "MeshBuilderSkinningInfo.h"

namespace AZ::MeshBuilder
{
    /*
     * Small usage tutorial:
     *
     * For all your vertex data types (position, normal, uvs etc)
     *    AddLayer( layer );
     *
     * For all polygons inside a mesh you want to export
     *    BeginPolygon( polyMaterialIndex )
     *       For all added layers you added to the mesh
     *          layer->SetCurrentVertexValue( )
     *       AddPolygonVertex( originalVertexNr )
     *    EndPolygon()
     *
     * OptimizeTriangleList()
     */
    class MeshBuilder
    {
        friend class MeshBuilderSubMesh;

    public:
        AZ_CLASS_ALLOCATOR_DECL

        explicit MeshBuilder(size_t numOrgVerts, bool optimizeDuplicates = true);
        explicit MeshBuilder(size_t numOrgVerts, size_t maxBonesPerSubMesh, size_t maxSubMeshVertices, bool optimizeDuplicates = true);

        template <class LayerType, class... Args>
        AZStd::enable_if_t<AZStd::is_convertible_v<LayerType*, MeshBuilderVertexAttributeLayer*>, LayerType*> AddLayer(Args&&... args)
        {
            if constexpr (AZStd::is_same_v<AZStd::tuple_element_t<0, AZStd::tuple<Args...>>, AZStd::unique_ptr<LayerType>>)
            {
                // If the thing we were passed is already a unique_ptr, just move it in
                m_layers.emplace_back(AZStd::move(args)...);
            }
            else
            {
                m_layers.emplace_back(AZStd::make_unique<LayerType>(AZStd::forward<Args>(args)...));
            }
            return static_cast<LayerType*>(m_layers.back().get());
        }

        void BeginPolygon(size_t materialIndex);    // begin a poly
        void AddPolygonVertex(size_t orgVertexNr);  // add a vertex to it (do this n-times, for an n-gon)
        void EndPolygon();                          // end the polygon, after adding all polygon vertices

        size_t CalcNumIndices() const;  // calculate the number of indices in the mesh
        size_t CalcNumVertices() const; // calculate the number of vertices in the mesh

        size_t GetNumOrgVerts() const { return m_numOrgVerts; }
        void SetSkinningInfo(AZStd::unique_ptr<MeshBuilderSkinningInfo> skinningInfo);
        const MeshBuilderSkinningInfo* GetSkinningInfo() const         { return m_skinningInfo.get(); }
        MeshBuilderSkinningInfo* GetSkinningInfo()                     { return m_skinningInfo.get(); }
        size_t GetMaxBonesPerSubMesh() const                           { return m_maxBonesPerSubMesh; }
        size_t GetMaxVerticesPerSubMesh() const                        { return m_maxSubMeshVertices; }
        void SetMaxBonesPerSubMesh(size_t maxBones)                    { m_maxBonesPerSubMesh = maxBones; }
        size_t GetNumLayers() const { return m_layers.size(); }
        size_t GetNumSubMeshes() const { return m_subMeshes.size(); }
        const MeshBuilderSubMesh* GetSubMesh(size_t index) const { return m_subMeshes[index].get(); }
        const MeshBuilderVertexAttributeLayer* GetLayer(size_t index) const { return m_layers[index].get(); }
        size_t GetNumPolygons() const { return m_polyVertexCounts.size(); }

        struct SubMeshVertex
        {
            size_t m_realVertexNr;
            size_t m_dupeNr;
            MeshBuilderSubMesh* m_subMesh;
        };

        size_t FindRealVertexNr(const MeshBuilderSubMesh* subMesh, size_t orgVtx, size_t dupeNr) const;
        void SetRealVertexNrForSubMeshVertex(const MeshBuilderSubMesh* subMesh, size_t orgVtx, size_t dupeNr, size_t realVertexNr);
        const SubMeshVertex* FindSubMeshVertex(const MeshBuilderSubMesh* subMesh, size_t orgVtx, size_t dupeNr) const;
        size_t CalcNumVertexDuplicates(const MeshBuilderSubMesh* subMesh, size_t orgVtx) const;

        void GenerateSubMeshVertexOrders();

        void AddSubMeshVertex(size_t orgVtx, SubMeshVertex&& vtx);
        size_t GetNumSubMeshVertices(size_t orgVtx) const;
        const SubMeshVertex& GetSubMeshVertex(size_t orgVtx, size_t index) const { return m_vertices[orgVtx][index]; }

    private:
        static constexpr inline int s_defaultMaxBonesPerSubMesh = 512;
        static constexpr inline int s_defaultMaxSubMeshVertices = 65535;

        AZStd::vector<AZStd::unique_ptr<MeshBuilderSubMesh>> m_subMeshes;
        AZStd::vector<AZStd::unique_ptr<MeshBuilderVertexAttributeLayer>> m_layers;
        AZStd::vector<AZStd::vector<SubMeshVertex>> m_vertices;
        AZStd::vector<size_t> m_polyJointList;
        AZStd::unique_ptr<MeshBuilderSkinningInfo> m_skinningInfo{};

        AZStd::vector<MeshBuilderVertexLookup> m_polyIndices;
        AZStd::vector<size_t> m_polyOrgVertexNumbers;
        AZStd::vector<AZ::u8> m_polyVertexCounts;

        size_t m_materialIndex = 0;
        size_t m_maxBonesPerSubMesh = s_defaultMaxBonesPerSubMesh;
        size_t m_maxSubMeshVertices = s_defaultMaxSubMeshVertices;
        size_t m_numOrgVerts = 0;
        bool m_optimizeDuplicates = true;

        const MeshBuilderVertexLookup FindMatchingDuplicate(size_t orgVertexNr) const;
        const MeshBuilderVertexLookup AddVertex(size_t orgVertexNr);
        const MeshBuilderVertexLookup FindVertexIndex(size_t orgVertexNr) const;
        const MeshBuilderSubMesh* FindSubMeshForPolygon(const AZStd::vector<size_t>& orgVertexNumbers, size_t materialIndex) const;
        MeshBuilderSubMesh* FindSubMeshForPolygon(const AZStd::vector<size_t>& orgVertexNumbers, size_t materialIndex)
        {
            return const_cast<MeshBuilderSubMesh*>(static_cast<const MeshBuilder*>(this)->FindSubMeshForPolygon(orgVertexNumbers, materialIndex));
        }
        SubMeshVertex* FindSubMeshVertex(const MeshBuilderSubMesh* subMesh, size_t orgVtx, size_t dupeNr)
        {
            return const_cast<SubMeshVertex*>(static_cast<const MeshBuilder*>(this)->FindSubMeshVertex(subMesh, orgVtx, dupeNr));
        }
        void ExtractBonesForPolygon(const AZStd::vector<size_t>& orgVertexNumbers, AZStd::vector<size_t>& outPolyJointList) const;
        void AddPolygon(const AZStd::vector<MeshBuilderVertexLookup>& indices, const AZStd::vector<size_t>& orgVertexNumbers, size_t materialIndex);
    };
} // namespace AZ::MeshBuilder
