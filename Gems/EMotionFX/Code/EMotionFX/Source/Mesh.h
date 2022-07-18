/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Source/VertexAttributeLayerBuffer.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/iterator.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/utility/move.h>
#include "EMotionFXConfig.h"
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/string/string.h>
#include "BaseObject.h"
#include "VertexAttributeLayer.h"
#include "Transform.h"

#include <MCore/Source/Vector.h>
#include <MCore/Source/Ray.h>

namespace AZ::RPI
{
    class ModelLodAsset;
}

namespace EMotionFX
{
    // forward declarations
    class SubMesh;
    class Actor;

    /**
     * The mesh base class.
     * Every mesh contains a list of vertex data (positions, normals and uv coordinates) and a set of indices.
     * The indices describe the polygons and point into the vertex data.
     * The vertex data is stored in seperate layers. So the position, normal, uv and tangent data is stored in three layers/arrays.
     * You can access the vertex data with FindVertexData() and FindOriginalVertexData().
     * The length of all these arrays equals the value returned by GetNumVertices().
     * So this means that there are ALWAYS the same amount of positions, normals and uv coordinates in the mesh.
     * Vertices automatically are duplicated when needed. This means that a flat shaded cube of triangles which normally has 8 vertices
     * will have 24 vertices in this mesh. This is done, because you can use this vertex data directly to render it on the
     * graphics hardware and it allows multiple normals and texture coordinates per vertex, which is needed for good
     * texture mapping support and support for smoothing groups and flat shading.
     * Still, each of the 24 vertices in the 'cube' example will have indices to their original vertices (which are not stored).
     * This array of indices can be accessed by calling GetOrgVerts(). The values stored in this array will range from [0..7].
     * The mesh also contains a set of vertex attributes, which are user defined attributes per vertex.
     * Next to that there also are shared vertex attributes, which are only there for the real number of vertices.
     * All vertices have the same number of attributes, and attribute number 'n' has to be of the same type for all vertices as well.
     * @see SubMesh.
     * @see FindVertexData.
     * @see FindOriginalVertexData.
     */
    class EMFX_API Mesh
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        using VertexAttributes = AZStd::variant<
            AZStd::unique_ptr<VertexAttributeLayerBuffer<AZ::Vector3>>,
            AZStd::unique_ptr<VertexAttributeLayerBuffer<AZ::u32>>,
            AZStd::unique_ptr<VertexAttributeLayerBuffer<AZ::Vector2>>,
            AZStd::unique_ptr<VertexAttributeLayerBuffer<AZ::Vector4>>>;
        /**
         * VertexAttributeLayerAbstractData::GetType() values for the vertex data
         * Use these with the Mesh::FindVertexData() and Mesh::FindOriginalVertexData() methods.
         */
        enum : uint32
        {
            ATTRIB_POSITIONS        = 0,    /**< Vertex positions. Typecast to AZ::Vector3. Positions are always exist. */
            ATTRIB_NORMALS          = 1,    /**< Vertex normals. Typecast to AZ::Vector3. Normals are always exist. */
            ATTRIB_TANGENTS         = 2,    /**< Vertex tangents. Typecast to <b> AZ::Vector4 </b>. */
            ATTRIB_UVCOORDS         = 3,    /**< Vertex uv coordinates. Typecast to AZ::Vector2. */
            ATTRIB_ORGVTXNUMBERS    = 5,    /**< Original vertex numbers. Typecast to uint32. Original vertex numbers always exist. */
            ATTRIB_BITANGENTS       = 7,    /**< Vertex bitangents (aka binormal). Typecast to AZ::Vector3. When tangents exists bitangents may still not exist! */
        };

        /**
         * The memory block ID where mesh data will be allocated in.
         * Giving mesh data their own memory blocks can reduce the amount of used total blocks when memory gets deleted again.
         */
        enum
        {
            MEMORYBLOCK_ID = 100
        };

        /**
         * Default constructor.
         */
        static Mesh* Create();

        /**
         * Constructor which allocates mesh data.
         * Please keep in mind that this does not create and add any layers for position, normal, tangent, uv data etc.
         * @param numVerts The number of vertices to allocate.
         * @param numIndices The number of indices to allocate.
         * @param numPolygons The number of polygons to allocate.
         * @param numOrgVerts The number of original vertices.
         * @param isCollisionMesh Set to true if this mesh is a collision mesh.
         */
        static Mesh* Create(uint32 numVerts, uint32 numIndices, uint32 numPolygons, uint32 numOrgVerts, bool isCollisionMesh);

        static Mesh* CreateFromModelLod(const AZ::Data::Asset<AZ::RPI::ModelLodAsset>& sourceModelLod, const AZStd::unordered_map<AZ::u16, AZ::u16>& skinToSkeletonIndexMap);

        /**
         * Allocate mesh data. If there is already data allocated, this data will be deleted first.
         * Please keep in mind this does not create and add any layers for position, normal, uv and tangent data etc.
         * Only indices will be allocated.
         * @param numVerts The number of vertices to allocate.
         * @param numIndices The number of indices to allocate.
         * @param numPolygons The number of polygons to allocate.
         * @param numOrgVerts The number of original vertices.
         */
        void Allocate(uint32 numVerts, uint32 numIndices, uint32 numPolygons, uint32 numOrgVerts);

        /**
         * Releases all allocated mesh data.
         */
        void ReleaseData();

        /**
         * Get the number of vertices in the mesh.
         * The number of positions, normals and uv coordinates is equal to the value returned by this method.
         * @result The number of vertices.
         */
        MCORE_INLINE uint32 GetNumVertices() const;

        /**
         * Get the number of original vertices. This can be lower compared to the value returned by GetNumVertices().
         * For the example of the cube with 8 real vertices, but 24 vertices in this mesh, this method would return
         * a value of 8, while the method GetNumVertices() would return 24.
         * Please keep in mind that the number of elements in the array returned by GetOrgVerts() however equals
         * the amount returned by GetNumVertices() and not the amount returned by this method!
         * @result The number of original vertices in the mesh (which are not stored).
         */
        MCORE_INLINE uint32 GetNumOrgVertices() const;

        /**
         * Resets all final/output vertex data to its original data, as it was before any deformers where applied.
         * This will copy the original positions over the final positions, the original normals over the final normals
         * and the original uv coordinates over the final uv coordinates.
         */
        void ResetToOriginalData();

        /**
         * Get a pointer to the face indices.
         * Every face has 3 indices and all faces indices are stored after eachother.
         * @result A pointer to the face index data.
         */
        MCORE_INLINE uint32* GetIndices() const;

        /**
         * Get a pointer to the polygon vertex count.
         * Every polygon stores an integer which represents the number of vertices/indices this polygon stores.
         * The length of this array equals GetNumPolygons().
         * @result A pointer to the polygon vertex count.
         */
        MCORE_INLINE uint8* GetPolygonVertexCounts() const;

        /**
         * Returns the number of indices in the mesh, which is the sum of all polygon vertex counts.
         * @result The number of indices in the mesh.
         */
        MCORE_INLINE uint32 GetNumIndices() const;

        /**
         * Returns the number of polygons.
         * @result The number of polygons inside this mesh.
         */
        MCORE_INLINE uint32 GetNumPolygons() const;

        /**
         * Add a SubMesh to this mesh.
         * @param subMesh A pointer to the SubMesh to add.
         */
        MCORE_INLINE void AddSubMesh(SubMesh* subMesh);

        /**
         * Get the number of sub meshes currently in the mesh.
         * @result The number of sub meshes.
         */
        MCORE_INLINE size_t GetNumSubMeshes() const;

        /**
         * Get a given SubMesh.
         * @param nr The SubMesh number to get.
         * @result A pointer to the SubMesh.
         */
        MCORE_INLINE SubMesh* GetSubMesh(size_t nr) const;

        /**
         * Set the value for a given submesh.
         * @param nr The submesh number, which must be in range of [0..GetNumSubMeshes()-1].
         * @param subMesh The submesh to use.
         */
        MCORE_INLINE void SetSubMesh(size_t nr, SubMesh* subMesh)           { m_subMeshes[nr] = subMesh; }

        /**
         * Set the number of submeshes.
         * This adjusts the number returned by GetNumSubMeshes().
         * Do not forget to use SetSubMesh() to initialize all submeshes!
         * @param numSubMeshes The number of submeshes to use.
         */
        MCORE_INLINE void SetNumSubMeshes(size_t numSubMeshes)              { m_subMeshes.resize(numSubMeshes); }

        /**
         * Remove a given submesh from this mesh.
         * @param nr The submesh index number to remove, which must be in range of 0..GetNumSubMeshes()-1.
         * @param delFromMem Set to true when you want to delete the submesh from memory as well, otherwise set to false.
         */
        void RemoveSubMesh(size_t nr, bool delFromMem = true);

        /**
         * Insert a submesh into the array of submeshes.
         * @param insertIndex The position in the submesh array to insert this new submesh.
         * @param subMesh A pointer to the submesh to insert into this mesh.
         */
        void InsertSubMesh(size_t insertIndex, SubMesh* subMesh);

        /**
         * Get the shared vertex attribute data of a given layer.
         * The number of arrays inside the array returned equals the value returned by GetNumOrgVertices().
         * @param layerNr The layer number to get the attributes from. Must be below the value returned by GetNumSharedVertexAttributeLayers().
         * @result A pointer to the array of shared vertex attributes. You can typecast this pointer if you know the type of the vertex attributes.
         */
        VertexAttributeLayer* GetSharedVertexAttributeLayer(size_t layerNr);

        /**
         * Adds a new layer of shared vertex attributes.
         * The data will automatically be deleted from memory on destruction of the mesh.
         * @param layer The layer to add. The array must contain GetNumOrgVertices() elements.
         */
        void AddSharedVertexAttributeLayer(VertexAttributeLayer* layer);

        /**
         * Get the number of shared vertex attributes.
         * This value is the same for all shared vertices.
         * @result The number of shared vertex attributes for every vertex.
         */
        size_t GetNumSharedVertexAttributeLayers() const;

        /**
         * Find and return the shared vertex attribute layer of a given type.
         * If you like to find the first layer of the given type, the occurence parameter must be set to a value of 0.
         * If you like to find the second layer of this type, the occurence parameter must be set to a value of of 1, etc.
         * This function will return nullptr when the layer cannot be found.
         * @param layerTypeID the layer type ID to search for.
         * @param occurrence The occurence to search for. Set to 0 when you want the first layer of this type, set to 1 if you
         *                  want the second layer of the given type, etc.
         * @result The vertex attribute layer index number that you can pass to GetSharedVertexAttributeLayer. A value of MCORE_INVALIDINDEX32 is returned
         *         when no result could be found.
         */
        size_t FindSharedVertexAttributeLayerNumber(uint32 layerTypeID, size_t occurrence = 0) const;

        /**
         * Find and return the shared vertex attribute layer of a given type.
         * If you like to find the first layer of the given type, the occurence parameter must be set to a value of 0.
         * If you like to find the second layer of this type, the occurence parameter must be set to a value of of 1, etc.
         * This function will return nullptr when the layer cannot be found.
         * @param layerTypeID the layer type ID to search for.
         * @param occurence The occurence to search for. Set to 0 when you want the first layer of this type, set to 1 if you
         *                  want the second layer of the given type, etc.
         * @result A pointer to the vertex attribute layer, or nullptr when none could be found.
         */
        VertexAttributeLayer* FindSharedVertexAttributeLayer(uint32 layerTypeID, size_t occurence = 0) const;

        /**
         * Removes all shared vertex attributes for all shared vertices.
         * The previously allocated attributes will be deleted from memory automatically.
         */
        void RemoveAllSharedVertexAttributeLayers();

        /**
         * Remove a layer of shared vertex attributes.
         * Automatically deletes the data from memory.
         * @param layerNr The layer number to remove, must be below the value returned by GetNumSharedVertexAttributeLayers().
         */
        void RemoveSharedVertexAttributeLayer(size_t layerNr);

        /**
         * Get the number of vertex attributes.
         * This value is the same for all vertices.
         * @result The number of vertex attributes for every vertex.
         */
        size_t GetNumVertexAttributeLayers() const;


        size_t FindSharedVertexAttributeLayerIndexByName(const char* name) const;
        size_t FindSharedVertexAttributeLayerIndexByNameString(const AZStd::string& name) const;
        size_t FindSharedVertexAttributeLayerIndexByNameID(uint32 nameID) const;

        /**
         * Removes all vertex attributes for all vertices.
         * The previously allocated attributes will be deleted from memory automatically.
         */
        void RemoveAllVertexAttributeLayers();

        //---------------------------------------------------

        /**
         * Checks for an intersection between the mesh and a given ray.
         * @param transform The transformation of the mesh.
         * @param ray The ray to test with.
         * @result Returns true when an intersection has occurred, otherwise false.
         */
        bool Intersects(const Transform& transform, const MCore::Ray& ray);

        /**
         * Check for an intersection between the mesh a given ray, and calculate the closest intersection point.
         * If you do NOT need to know the intersection point, use the other Intersects method, because that one is faster, since it doesn't need to calculate
         * the closest intersection point.
         * The intersection point returned is in object space.
         * @param transform The transformation of the mesh.
         * @param ray The ray to test with.
         * @param outIntersect A pointer to the vector to store the intersection point in, in case of a collision (nullptr allowed).
         * @param outBaryU A pointer to a float in which the method will store the barycentric U coordinate, to be used to interpolate values on the triangle (nullptr allowed).
         * @param outBaryV A pointer to a float in which the method will store the barycentric V coordinate, to be used to interpolate values on the triangle (nullptr allowed).
         * @param outIndices A pointer to an array of 3 integers, which will contain the 3 vertex indices of the closest intersecting triangle. Even on polygon meshes with polygons of more than 3 vertices three indices are returned. In that case the indices represent a sub-triangle inside the polygon.
         *                   A value of nullptr is allowed, which will skip storing the resulting triangle indices.
         * @result Returns true when an intersection has occurred, otherwise false.
         */
        bool Intersects(const Transform& transform, const MCore::Ray& ray, AZ::Vector3* outIntersect, float* outBaryU = nullptr, float* outBaryV = nullptr, uint32* outIndices = nullptr);

        //---------------------------------------------------

        /**
         * Gather all bones influencing a specified face into a specified array.
         * The outBones array will be cleared when it enters the method.
         * @param startIndexOfFace The start index of the first vertex of the face. So not the vertex number, but the offset in the index array of this mesh.
         * @param outBones The array to store the pointers to the bones in. Any existing array contents will be cleared when it enters the method.
         * @param actor The actor to search the bones in.
         */
        void GatherBonesForFace(uint32 startIndexOfFace, AZStd::vector<Node*>& outBones, Actor* actor);

        /**
         * Calculates the maximum number of bone influences for a given face.
         * This is calculated by for each vertex checking the number of bone influences, and take the maximum of that amount.
         * @param startIndexOfFace The start index of the first vertex of the face. So not the vertex number, but the offset in the index array of this mesh.
         * @result The maximum number of influences of the given face. This will be 0 for non-softskinned objects.
         */
        uint32 CalcMaxNumInfluencesForFace(uint32 startIndexOfFace) const;

        /**
         * Calculates the maximum number of bone influences.
         * This is calculated by for each vertex checking the number of bone influences, and take the maximum of that amount.
         * @result The maximum number of influences. This will be 0 for non-softskinned objects.
         */
        size_t CalcMaxNumInfluences() const;

        /**
         * Calculates the maximum number of bone influences.
         * This is calculated by for each vertex checking the number of bone influences, and take the maximum of that amount.
         * @param vertexCounts Reference to an interger array, which will be filled by this function with vertex counts for specific number of influences.
         *                     array[0] e.g. holds the number of vertices which have no influences, so non-skinned; array[4] holds the number of vertices
         *                     which are effected by 4 bones.
         * @result The maximum number of influences. This will be 0 for non-softskinned objects.
         */
        size_t CalcMaxNumInfluences(AZStd::vector<size_t>& outVertexCounts) const;

        /**
         * Extract a list of positions of the original vertices.
         * For a cube, which normally might have 32 vertices, it will result in 8 positions.
         * @param outPoints The output array to store the points in. The array will be automatically resized.
         */
        void ExtractOriginalVertexPositions(AZStd::vector<AZ::Vector3>& outPoints) const;

        /**
         * Clone the mesh.
         * @result A pointer to a mesh that contains exactly the same data as this mesh.
         */
        Mesh* Clone();

        /**
         * Remove all empty submeshes.
         * A submesh is either empty if it has no vertices or no indices or a combination of both.
         * @param onlyRemoveOnZeroVertsAndTriangles Only remove when both the number of vertices and number of indices/triangles are zero.
         * @result Returns the number of removed submeshes.
         */
        size_t RemoveEmptySubMeshes(bool onlyRemoveOnZeroVertsAndTriangles = true);

        /**
         * Calculate the axis aligned bounding box of this mesh, after transforming the positions with the provided transform.
         * @param outBoundingBox The bounding box that will contain the bounds after executing this method.
         * @param Transform The transformation to transform all vertex positions with.
         * @param vertexFrequency This is the for loop increase counter value. A value of 1 means every vertex will be processed
         *                        while a value of 2 means every second vertex, etc. The value must be 1 or higher.
         */
        void CalcAabb(AZ::Aabb* outBoundingBox, const Transform& transform, uint32 vertexFrequency = 1);

    
        /**
         * Debug log information.
         * Information will be logged using LOGLEVEL_DEBUG.
         */
        void Log();

        /**
         * Check if this mesh is a pure triangle mesh.
         * Please keep in mind that this method is quite heavy to process as it iterates over all polygons to check if the vertex count equals to 3 for all polygons in the worst case.
         * @result Returns true in case this mesh contains only triangles, otherwise false is returned.
         */
        bool CheckIfIsTriangleMesh() const;

        /**
         * Check if this mesh is a pure quad mesh.
         * Please keep in mind that this method is quite heavy to process as it iterates over all polygons to check if the vertex count equals to 4 for all polygons in the worst case.
         * @result Returns true in case this mesh contains only quads, otherwise false is returned.
         */
        bool CheckIfIsQuadMesh() const;

        /**
         * Scale all positional data.
         * This is a slow operation and is used to convert between different unit systems (cm, meters, etc).
         * @param scaleFactor The scale factor to scale the current data by.
         */
        void Scale(float scaleFactor);

        /**
         * Get the total number of unique joints that impact this mesh
         */
        uint16 GetNumUniqueJoints() const;

        /**
         * Set the total number of unique joints that impact this mesh
         */
        void SetNumUniqueJoints(uint16 numUniqueJoints);

        /**
         * Get the highest id of all the jointId's used by this mesh
         */
        uint16 GetHighestJointIndex() const;

        /**
         * Get the highest id of all the jointId's used by this mesh
         */
        void SetHighestJointIndex(uint16 highestJointIndex);

        MCORE_INLINE bool GetIsCollisionMesh() const            { return m_isCollisionMesh; }
        void SetIsCollisionMesh(bool isCollisionMesh)           { m_isCollisionMesh = isCollisionMesh; }

        template<AttributeType TType> 
        void CreateVertexAttribute(const AZStd::vector<typename AttributeTrait<TType>::TargetType>& buffer, bool original) {
            size_t index = static_cast<size_t>(TType);
            m_vertexAttributeLayer[index] = AZStd::make_unique<VertexAttributeLayerBuffer<typename AttributeTrait<TType>::TargetType>>(TType, buffer, original);
        }

        VertexAttributes& GetVertexAttribute(AttributeType attrType) {
            size_t index = static_cast<size_t>(attrType);
            return m_vertexAttributeLayer[index];
        }

        AZStd::span<VertexAttributes> GetVertexAttributes() {
            return m_vertexAttributeLayer;
        }

        template<AttributeType TType>
        VertexAttributeLayerBuffer<typename AttributeTrait<TType>::TargetType>* GetVertexAttribute() const {
            size_t index = static_cast<size_t>(TType);
            if(auto attrib = AZStd::get_if<AZStd::unique_ptr<VertexAttributeLayerBuffer<typename AttributeTrait<TType>::TargetType>>>(&m_vertexAttributeLayer[index])) {
                return {attrib->get()};
            }
            return nullptr;
        }

        template<AttributeType TType>
        VertexAttributeLayerBuffer<typename AttributeTrait<TType>::TargetType>* GetVertexAttribute() {
            return const_cast<VertexAttributeLayerBuffer<typename AttributeTrait<TType>::TargetType>*>(const_cast<const Mesh*>(this)->GetVertexAttribute<TType>());
        }

    protected:

        AZStd::vector<SubMesh*> m_subMeshes;         /**< The collection of sub meshes. */
        uint32*                 m_indices;           /**< The array of indices, which define the faces. */
        uint8*                  m_polyVertexCounts;  /**< The number of vertices for each polygon, where the length of this array equals the number of polygons. */
        uint32                  m_numPolygons;       /**< The number of polygons in this mesh. */
        uint32                  m_numOrgVerts;       /**< The number of original vertices. */
        uint32                  m_numVertices;       /**< Number of vertices. */
        uint32                  m_numIndices;        /**< Number of indices. */
        uint16                  m_numUniqueJoints;   /**< Number of unique joints*/
        uint16                  m_highestJointIndex;    /**< The highest id of all the joints used by this mesh*/
        bool                    m_isCollisionMesh;   /**< Is this mesh a collision mesh? */

        /**
         * The array of shared vertex attribute layers.
         * The number of attributes in each shared layer will be equal to the value returned by Mesh::GetNumOrgVertices().
         */
        AZStd::vector< VertexAttributeLayer* >   m_sharedVertexAttributes;

        /**
         * The array of non-shared vertex attribute layers.
         * The number of attributes in each shared layer will be equal to the value returned by Mesh::GetNumVertices().
         */
        AZStd::array<VertexAttributes, NumAttributes> m_vertexAttributeLayer ;

        /**
         * Default constructor.
         */
        Mesh();

        /**
         * Constructor which allocates mesh data.
         * Please keep in mind that this does not create and add any layers for position, normal, tangent, uv data etc.
         * @param numVerts The number of vertices to allocate.
         * @param numIndices The number of indices to allocate.
         * @param numPolygons The number of polygons to allocate.
         * @param numOrgVerts The number of original vertices.
         * @param isCollisionMesh Set to true if this mesh is a collision mesh.
         */
        Mesh(uint32 numVerts, uint32 numIndices, uint32 numPolygons, uint32 numOrgVerts, bool isCollisionMesh);

        /**
         * Destructor.
         * Automatically releases all data.
         */
        ~Mesh();
    };

    // include inline code
#include "Mesh.inl"
} // namespace EMotionFX
