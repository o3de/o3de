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

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <EMotionFX/Source/MeshBuilderVertexAttributeLayers.h>

namespace EMotionFX
{
    // forward declarations
    class MeshBuilderSubMesh;
    class MeshBuilderSkinningInfo;

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
     * OptimizeMemoryUsage()
     * OptimizeTriangleList()
     */
    class MeshBuilder
    {
        friend class MeshBuilderSubMesh;

    public:
        AZ_CLASS_ALLOCATOR_DECL

        explicit MeshBuilder(size_t jointIndex, size_t numOrgVerts, bool optimizeDuplicates = true);
        explicit MeshBuilder(size_t jointIndex, size_t numOrgVerts, size_t maxBonesPerSubMesh, size_t maxSubMeshVertices, bool optimizeDuplicates = true);
        ~MeshBuilder();

        // add a layer, all added layers will be deleted automatically by the destructor of this class
        void AddLayer(MeshBuilderVertexAttributeLayer* layer);
        MeshBuilderVertexAttributeLayer* FindLayer(uint32 layerID, uint32 occurrence = 0) const;
        size_t GetNumLayers(uint32 layerID) const;

        void BeginPolygon(size_t materialIndex);    // begin a poly
        void AddPolygonVertex(size_t orgVertexNr);  // add a vertex to it (do this n-times, for an n-gon)
        void EndPolygon();                          // end the polygon, after adding all polygon vertices

        size_t CalcNumIndices() const;  // calculate the number of indices in the mesh
        size_t CalcNumVertices() const; // calculate the number of vertices in the mesh

        void OptimizeMemoryUsage();     // call this after your mesh is filled with data and won't change anymore
        void OptimizeTriangleList();    // call this in order to optimize the index buffers on cache efficiency

        void LogContents();
        bool CheckIfIsTriangleMesh() const;
        bool CheckIfIsQuadMesh() const;

        size_t GetNumOrgVerts() const { return m_numOrgVerts; }
        void SetSkinningInfo(MeshBuilderSkinningInfo* skinningInfo)    { m_skinningInfo = skinningInfo; }
        MeshBuilderSkinningInfo* GetSkinningInfo() const               { return m_skinningInfo; }
        size_t GetMaxBonesPerSubMesh() const                           { return m_maxBonesPerSubMesh; }
        size_t GetMaxVerticesPerSubMesh() const                        { return m_maxSubMeshVertices; }
        void SetMaxBonesPerSubMesh(size_t maxBones)                    { m_maxBonesPerSubMesh = maxBones; }
        size_t GetJointIndex() const { return m_jointIndex; }
        void SetJointIndex(size_t jointIndex) { m_jointIndex = jointIndex; }
        size_t GetNumLayers() const { return m_layers.size(); }
        size_t GetNumSubMeshes() const { return m_subMeshes.size(); }
        MeshBuilderSubMesh* GetSubMesh(size_t index) const { return m_subMeshes[index].get(); }
        MeshBuilderVertexAttributeLayer* GetLayer(size_t index) const  { return m_layers[index]; }
        size_t GetNumPolygons() const { return m_polyVertexCounts.size(); }

        struct SubMeshVertex
        {
            size_t m_realVertexNr;
            size_t m_dupeNr;
            MeshBuilderSubMesh* m_subMesh;
        };

        size_t FindRealVertexNr(MeshBuilderSubMesh* subMesh, size_t orgVtx, size_t dupeNr);
        SubMeshVertex* FindSubMeshVertex(MeshBuilderSubMesh* subMesh, size_t orgVtx, size_t dupeNr);
        size_t CalcNumVertexDuplicates(MeshBuilderSubMesh* subMesh, size_t orgVtx) const;

        void GenerateSubMeshVertexOrders();

        void AddSubMeshVertex(size_t orgVtx, const SubMeshVertex& vtx);
        size_t GetNumSubMeshVertices(size_t orgVtx);
        AZ_FORCE_INLINE SubMeshVertex* GetSubMeshVertex(size_t orgVtx, size_t index) { return &m_vertices[orgVtx][index]; }

    private:
        static const int s_defaultMaxBonesPerSubMesh;
        static const int s_defaultMaxSubMeshVertices;

        AZStd::vector<AZStd::unique_ptr<MeshBuilderSubMesh>> m_subMeshes;
        AZStd::vector<MeshBuilderVertexAttributeLayer*> m_layers;
        AZStd::vector<AZStd::vector<SubMeshVertex>> m_vertices;
        AZStd::vector<size_t> m_polyJointList;
        MeshBuilderSkinningInfo* m_skinningInfo = nullptr;
        size_t m_jointIndex;

        AZStd::vector<MeshBuilderVertexLookup> m_polyIndices;
        AZStd::vector<size_t> m_polyOrgVertexNumbers;
        AZStd::vector<AZ::u8> m_polyVertexCounts;

        size_t m_materialIndex = 0;
        size_t m_maxBonesPerSubMesh = s_defaultMaxBonesPerSubMesh;
        size_t m_maxSubMeshVertices = s_defaultMaxSubMeshVertices;
        size_t m_numOrgVerts = 0;
        bool m_optimizeDuplicates = true;

        MeshBuilderVertexLookup FindMatchingDuplicate(size_t orgVertexNr);
        MeshBuilderVertexLookup AddVertex(size_t orgVertexNr);
        MeshBuilderVertexLookup FindVertexIndex(size_t orgVertexNr);
        MeshBuilderSubMesh* FindSubMeshForPolygon(const AZStd::vector<size_t>& orgVertexNumbers, size_t materialIndex);
        void ExtractBonesForPolygon(const AZStd::vector<size_t>& orgVertexNumbers, AZStd::vector<size_t>& outPolyJointList) const;
        void AddPolygon(const AZStd::vector<MeshBuilderVertexLookup>& indices, const AZStd::vector<size_t>& orgVertexNumbers, size_t materialIndex);
    };
} // namespace EMotionFX
