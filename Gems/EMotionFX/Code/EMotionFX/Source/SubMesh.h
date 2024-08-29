/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "EMotionFXConfig.h"
#include <MCore/Source/RefCounted.h>
#include <AzCore/std/containers/vector.h>


namespace EMotionFX
{
    // forward declarations
    class Mesh;
    class SkinningInfoVertexAttributeLayer;

    /**
     * The submesh class.
     * A submesh is a part of a mesh, with vertex and polygon data having the same material properties.
     * This allows us to easily render these submeshes on the graphics hardware on an efficient way.
     * You can see the SubMesh class as a draw primitive. It specifies a range inside the Mesh class's vertex data
     * and combines this with a material and a possible list of bones.
     * The submesh itself do not store any vertex data. All vertex and polygon (indices) are stored
     * in the Mesh class. You can access this parent mesh by the GetParentMesh() method.
     * All vertex and index data of all submeshes are stored in big arrays which contain all data for
     * all submeshes. This prevents small memory allocations and allows very efficient mesh updates.
     * The submeshes contain information about what place in the arrays the data for this submesh is stored.
     * So where the vertex data begins, and how many vertices are following after that. As well as where the
     * index values start in the big array, and how many indices will follow for this submesh.
     * Also there are some methods which gives you easy access to the vertex and index data stored inside
     * the parent mesh, so that you do not have to deal with the offsets returned by GetStartIndex() and
     * GetStartVertex().
     */
    class EMFX_API SubMesh
        : public MCore::RefCounted
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        /**
         * Creation method.
         * @param parentMesh A pointer to the Mesh to which this submesh belongs to.
         * @param startVertex The start vertex.
         * @param startIndex The start index.
         * @param startPolygon The start polygon number.
         * @param numVerts Number of vertices which submesh holds.
         * @param numIndices Number of indices which submesh holds.
         * @param numPolygons The number of polygons inside this submesh.
         * @param numBones The number of bones inside the submesh.
         */
        static SubMesh* Create(Mesh* parentMesh, uint32 startVertex, uint32 startIndex, uint32 startPolygon, uint32 numVerts, uint32 numIndices, uint32 numPolygons, size_t numBones);

        /**
         * Get the start index. This is the offset in the index array of the parent mesh where the index data for this
         * submesh starts. So it is not the start vertex or whatsoever. The index array of the parent mesh contains
         * the index data of all its submeshes. So it is one big array, with all index data of the submeshes sticked after
         * eachother. The value returned by this method just contains the offset in the array where the index data for this
         * submesh starts. The number of index values to follow equals the value returned by GetNumIndices().
         * You can also request a pointer to the first index value of this submesh by using the method GetIndices().
         * Please keep in mind that the index values stored are absolute and not relative. This means that index values
         * for every submesh point directly into the array of vertex data from the Mesh where this submesh is a part of.
         * @result The offset in the array of indices of the parent mesh, where the index data for this submesh starts.
         */
        uint32 GetStartIndex() const;

        /**
         * Get the start vertex offset. This offset points into the vertex data arrays (positions, normals, uvs) of the
         * parent mesh. The number of vertices to follow equals the amount returned by GetNumVertices().
         * @result The offset in the vertex data arrays in the parent mesh, where the vertex data for this submesh starts.
         */
        uint32 GetStartVertex() const;

        /**
         * Get the start polygon.
         * This represents the polygon index inside the parent mesh where the polygon vertex count data starts for this submesh.
         * @result The start polygon index inside the parent mesh.
         */
        uint32 GetStartPolygon() const;

        /**
         * Get a pointer to the index data for this submesh.
         * The number of indices to follow equals the value returned by GetNumIndices().
         * The index values are stored on an absolute way, so they point directly into the vertex data arrays of the
         * Mesh where this submesh belongs to.
         * @result The pointer to the index data for this submesh.
         */
        uint32* GetIndices() const;

        /**
         * Get the pointer to the polygon vertex counts.
         * The number of integers inside this buffer equals GetNumPolygons().
         * @result A pointer to the polygon vertex counts for each polygon inside this submesh.
         */
        uint8* GetPolygonVertexCounts() const;

        /**
         * Return the number of vertices.
         * @result The number of vertices contained in the submesh.
         */
        uint32 GetNumVertices() const;

        /**
         * Return the number of indices.
         * @result The number of indices contained in the submesh.
         */
        uint32 GetNumIndices() const;

        /**
         * Return the number of polygons.
         * @result The number of polygons contained in the submesh.
         */
        uint32 GetNumPolygons() const;

        /**
         * Return parent mesh.
         * @result A pointer to the parent mesh to which this submesh belongs to.
         */
        Mesh* GetParentMesh() const;

        /**
         * Set parent mesh.
         * @param mesh A pointer to the parent mesh to which this submesh belongs to.
         */
        void SetParentMesh(Mesh* mesh);

        /**
         * Set the offset in the index array of the mesh where this submesh is part of.
         * @param indexOffset The offset inside the index array of the mesh returned by GetParentMesh().
         */
        void SetStartIndex(uint32 indexOffset);

        /**
         * Set the start polygon number.
         * @param polygonNumber The polygon number inside the polygon vertex count array of the mesh returned by GetParentMesh().
         */
        void SetStartPolygon(uint32 polygonNumber);

        /**
         * Set the offset in the vertex array of the mesh where this submesh is part of.
         * @param vertexOffset The offset inside the index array of the mesh returned by GetParentMesh().
         */
        void SetStartVertex(uint32 vertexOffset);

        /**
         * Set the number of indices used by this submesh.
         * @param numIndices The number of indices used by this submesh.
         */
        void SetNumIndices(uint32 numIndices);

        /**
         * Set the number of vertices used by this submesh.
         * @param numVertices The number of vertices used by this submesh.
         */
        void SetNumVertices(uint32 numVertices);

        //-------------------------------------------------------------------------------

        /**
         * Set the number of bones that is being used by this submesh.
         * @param numBones The number of bones used by the submesh.
         */
        void SetNumBones(size_t numBones);

        /**
         * Set the index of a given bone.
         * @param index The bone number, which must be in range of [0..GetNumBones()-1].
         * @param nodeIndex The node index number that acts as bone on this submesh.
         */
        void SetBone(size_t index, size_t nodeIndex);

        /**
         * Get the number of bones used by this submesh.
         * @result The number of bones used by this submesh.
         */
        MCORE_INLINE size_t GetNumBones() const                                 { return m_bones.size(); }

        /**
         * Get the node index for a given bone.
         * @param index The bone number, which must be in range of [0..GetNumBones()-1].
         * @result The node index value for the given bone.
         */
        MCORE_INLINE size_t GetBone(size_t index)   const                       { return m_bones[index]; }

        /**
         * Get direct access to the bone values, by getting a pointer to the first bone index.
         * Each integer in the array represents the node number that acts as bone on this submesh.
         * @result A pointer to the array of bones used by this submesh.
         */
        MCORE_INLINE size_t* GetBones()                                         { return m_bones.data(); }

        /**
         * Get direct access to the bones array.
         * Each integer in the array represents the node number that acts as bone on this submesh.
         * @result A read only reference to the array of bones used by this submesh.
         */
        MCORE_INLINE const AZStd::vector<size_t>& GetBonesArray() const          { return m_bones; }

        /**
         * Get direct access to the bones array.
         * Each integer in the array represents the node number that acts as bone on this submesh.
         * @result A reference to the array of bones used by this submesh.
         */
        MCORE_INLINE AZStd::vector<size_t>& GetBonesArray()                      { return m_bones; }

        /**
         * Reinitialize the bones.
         * Iterate over the influences from the given skin and make sure all bones used in there are inside the local bones array.
         * @param[in] skinLayer Pointer to the skinning attribute layer used to deform the mesh.
         */
        void ReinitBonesArray(SkinningInfoVertexAttributeLayer* skinLayer);

        /**
         * Find the bone number, which would be in range of [0..GetNumBones()-1] for a given node number.
         * So you can use this to map a global node number into a local index inside the array of bones of this submesh.
         * This is useful when giving each vertex a list of offsets into the bone matrix array inside your shader.
         * @param nodeNr The global node number that is a bone. This must be in range with the number of nodes in the actor.
         * @result The bone number inside the submesh, which is in range of [0..GetNumBones()-1].
         *         A value of MCORE_INVALIDINDEX32 is returned when the specified node isn't used as bone inside this submesh.
         */
        size_t FindBoneIndex(size_t nodeNr) const;

        /**
         * Remap bone to a new bone. This will overwrite the given old bones with the new one.
         * @param oldNodeNr The node number to be searched and replaced.
         * @param newNodeNr The node number with which the old bones will be replaced with.
         */
        void RemapBone(size_t oldNodeNr, size_t newNodeNr);

        /**
         * Remove the given bone from the bones list.
         * @param index The index of the bone to be removed in range of [0..GetNumBones()-1].
         */
        void RemoveBone(size_t index);

        /**
         * Clone the submesh.
         * Please note that this method does not actually add the clone to the new (specified) parent mesh.
         * @param newParentMesh A pointer to the mesh that will get the cloned submesh added to it.
         * @result A pointer to a submesh that is an exact clone of this submesh.
         */
        SubMesh* Clone(Mesh* newParentMesh);

        /**
         * Calculate how many triangles this submesh has.
         * In case the mesh contains polygons of more than 3 vertices, triangulation will be taken into account.
         * @result The number of triangles that are needed to draw this submesh.
         */
        uint32 CalcNumTriangles() const;


    protected:
        AZStd::vector<size_t>    m_bones;         /**< The collection of bones. These are stored as node numbers that point into the actor. */
        uint32                  m_startVertex;   /**< The start vertex number in the vertex data arrays of the parent mesh. */
        uint32                  m_startIndex;    /**< The start index number in the index array of the parent mesh. */
        uint32                  m_startPolygon;  /**< The start polygon number in the polygon vertex count array of the parent mesh. */
        uint32                  m_numVertices;   /**< The number of vertices in this submesh. */
        uint32                  m_numIndices;    /**< The number of indices in this submesh. */
        uint32                  m_numPolygons;   /**< The number of polygons in this submesh. */
        Mesh*                   m_parentMesh;    /**< The parent mesh. */

        /**
         * Constructor.
         * @param parentMesh A pointer to the Mesh to which this submesh belongs to.
         * @param startVertex The start vertex.
         * @param startIndex The start index.
         * @param startPolygon The start polygon number.
         * @param numVerts Number of vertices which submesh holds.
         * @param numIndices Number of indices which submesh holds.
         * @param numPolygons The number of polygons inside this submesh.
         * @param numBones The number of bones inside the submesh.
         */
        SubMesh(Mesh* parentMesh, uint32 startVertex, uint32 startIndex, uint32 startPolygon, uint32 numVerts, uint32 numIndices, uint32 numPolygons, size_t numBones);

        /**
         * Destructor.
         */
        ~SubMesh();
    };
} // namespace EMotionFX
