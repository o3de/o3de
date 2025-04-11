/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "EMotionFXConfig.h"
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/string/string.h>
#include <MCore/Source/RefCounted.h>
#include "VertexAttributeLayer.h"
#include "Transform.h"

#include <MCore/Source/Vector.h>
#include <AzCore/std/containers/vector.h>
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
    class EMFX_API Mesh final
        : public MCore::RefCounted
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
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
         * Recalculates the vertex normals.
         * @param useDuplicates Setting this to true will cause hard borders at vertices that have been duplicated. Setting to false (default) will use the original mesh vertex positions only
         *                      which results in the mesh being fully smoothed. On a cube however that would not work well, as it won't preserve the hard face normals, but it will smooth the normals.
         *                      Most of the time on character you would want to set this to false though, to prevent seams from showing up in areas between the neck and body.
         */
        void CalcNormals(bool useDuplicates = false);

        /**
         * Calculates the tangent vectors which can be used for per pixel lighting.
         * These are vectors also known as S and T, used in per pixel lighting techniques, like bumpmapping.
         * You can calculate the bitangent for the given tangent by taking the cross product between the
         * normal and the tangent for the given vertex. These three vectors (normal, tangent and bitangent) are
         * used to build a matrix which transforms the lights into tangent space.
         * You can only call this method after you have passed all other vertex and index data to the mesh.
         * So after all that has been initialized, call this method to calculate these vectors automatically.
         * If you specify a UV set that doesn't exist, this method will fall back to UV set 0.
         * @param uvLayer[in] The UV texture coordinate layer number. This is not the vertex attribute layer number, but the UV layer number.
         *                    This means that 0 means the first UV set, and 1 would mean the second UV set, etc.
         *                    When the UV layer doesn't exist, this method does nothing.
         * @param storeBitangents Set to true when you want the mesh to store its bitangents in its own layer. This will generate a bitangents layer inside the mesh. False will no do this.
         * @result Returns true on success, or false on failure. A failure can happen when no UV data can be found.
         */
        bool CalcTangents(uint32 uvLayer = 0, bool storeBitangents = false);

        /**
         * Calculates the tangent and bitangent for a given triangle.
         * @param posA The position of the first vertex.
         * @param posB The position of the second vertex.
         * @param posC The position of the third vertex.
         * @param uvA The texture coordinate of the first vertex.
         * @param uvB The texture coordinate of the second vertex.
         * @param uvC The texture coordinate of the third vertex.
         * @param outTangent A pointer to the vector to store the calculated tangent vector.
         * @param outBitangent A pointer to the vector to store the calculated bitangent vector (calculated using the gradients).
         */
        static void CalcTangentAndBitangentForFace(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC,
            const AZ::Vector2& uvA,  const AZ::Vector2& uvB,  const AZ::Vector2& uvC,
            AZ::Vector3* outTangent, AZ::Vector3* outBitangent);

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
         * Extract the number of UV coordinate layers.
         * It is recommended NOT to put this function inside a loop, because it is not very fast.
         * @result The number of UV layers/sets currently present inside this mesh.
         */
        size_t CalcNumUVLayers() const;

        /**
         * Calculate the number of vertex attribute layers of the given type.
         * It is recommended NOT to put this function inside a loop, because it is not very fast.
         * @param[in] type The type of the vertex attribute layer to count.
         * @result The number of layers/sets currently present inside this mesh.
         */
        size_t CalcNumAttributeLayers(uint32 type) const;

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

        /**
         * Get the vertex attribute data of a given layer.
         * The number of arrays inside the array returned equals the value returned by GetNumVertices().
         * @param layerNr The layer number to get the attributes from. Must be below the value returned by GetNumVertexAttributeLayers().
         * @result A pointer to the array of vertex attributes. You can typecast this pointer if you know the type of the vertex attributes.
         */
        VertexAttributeLayer* GetVertexAttributeLayer(size_t layerNr);

        /**
         * Adds a new layer of vertex attributes.
         * The data will automatically be deleted from memory on destruction of the mesh.
         * @param layer The layer to add. The array must contain GetNumVertices() elements.
         */
        void AddVertexAttributeLayer(VertexAttributeLayer* layer);

        /**
         * Reserve space for the given amount of vertex attribute layers.
         * This just pre-allocates array data to prevent reallocs.
         * @param numLayers The number of layers to reserve space for.
         */
        void ReserveVertexAttributeLayerSpace(uint32 numLayers);

        /**
         * Find and return the non-shared vertex attribute layer of a given type.
         * If you like to find the first layer of the given type, the occurence parameter must be set to a value of 0.
         * If you like to find the second layer of this type, the occurence parameter must be set to a value of of 1, etc.
         * This function will return nullptr when the layer cannot be found.
         * @param layerTypeID the layer type ID to search for.
         * @param occurrence The occurence to search for. Set to 0 when you want the first layer of this type, set to 1 if you
         *                  want the second layer of the given type, etc.
         * @result The vertex attribute layer index number that you can pass to GetSharedVertexAttributeLayer. A value of MCORE_INVALIDINDEX32 os returned
         *         when no result could be found.
         */
        size_t FindVertexAttributeLayerNumber(uint32 layerTypeID, size_t occurrence = 0) const;

        size_t FindVertexAttributeLayerNumberByName(uint32 layerTypeID, const char* name) const;
        VertexAttributeLayer* FindVertexAttributeLayerByName(uint32 layerTypeID, const char* name) const;


        /**
         * Find and return the non-shared vertex attribute layer of a given type.
         * If you like to find the first layer of the given type, the occurence parameter must be set to a value of 0.
         * If you like to find the second layer of this type, the occurence parameter must be set to a value of of 1, etc.
         * This function will return nullptr when the layer cannot be found.
         * @param layerTypeID the layer type ID to search for.
         * @param occurence The occurence to search for. Set to 0 when you want the first layer of this type, set to 1 if you
         *                  want the second layer of the given type, etc.
         * @result A pointer to the vertex attribute layer, or nullptr when none could be found.
         */
        VertexAttributeLayer* FindVertexAttributeLayer(uint32 layerTypeID, size_t occurence = 0) const;

        size_t FindVertexAttributeLayerIndexByName(const char* name) const;
        size_t FindVertexAttributeLayerIndexByNameString(const AZStd::string& name) const;
        size_t FindVertexAttributeLayerIndexByNameID(uint32 nameID) const;

        size_t FindSharedVertexAttributeLayerIndexByName(const char* name) const;
        size_t FindSharedVertexAttributeLayerIndexByNameString(const AZStd::string& name) const;
        size_t FindSharedVertexAttributeLayerIndexByNameID(uint32 nameID) const;

        /**
         * Removes all vertex attributes for all vertices.
         * The previously allocated attributes will be deleted from memory automatically.
         */
        void RemoveAllVertexAttributeLayers();

        /**
         * Remove a layer of vertex attributes.
         * Automatically deletes the data from memory.
         * @param layerNr The layer number to remove, must be below the value returned by GetNumVertexAttributeLayers().
         */
        void RemoveVertexAttributeLayer(size_t layerNr);

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
         * Swap two vertices. This will swap all data elements, such as position and normals.
         * Not only it will swap the basic data like position and normals, but also all vertex attributes.
         * This method is used by the  meshes support inside GetEMotionFX().
         * @param vertexA The first vertex number.
         * @param vertexB The second vertex number.
         * @note This does NOT update the index buffer!
         */
        void SwapVertex(uint32 vertexA, uint32 vertexB);

        /**
         * Remove a specific range of vertices from the mesh.
         * This also might change all pointers to vertex and index data returned by this mesh and its submeshes.
         * This automatically also adjusts the index buffer as well.
         * Please note that the specified end vertex will also be removed!
         * @param startVertexNr The vertex number to start removing from.
         * @param endVertexNr The last vertex number to remove, this value must be bigger or equal than the start vertex number.
         * @param changeIndexBuffer If this is set to true, the index buffer will be modified on such a way that all vertex
         *                          number indexed by the index buffer, which are above the startVertexNr parameter value,
         *                          will be decreased by the amount of removed vertices.
         * @param removeEmptySubMeshes When set to true (default) submeshes that become empty will be removed from memory.
         */
        void RemoveVertices(uint32 startVertexNr, uint32 endVertexNr, bool changeIndexBuffer = true, bool removeEmptySubMeshes = false);

        /**
         * Remove all empty submeshes.
         * A submesh is either empty if it has no vertices or no indices or a combination of both.
         * @param onlyRemoveOnZeroVertsAndTriangles Only remove when both the number of vertices and number of indices/triangles are zero.
         * @result Returns the number of removed submeshes.
         */
        size_t RemoveEmptySubMeshes(bool onlyRemoveOnZeroVertsAndTriangles = true);

        /**
         * Find specific current vertex data in the mesh. This contains the vertex data after mesh deformers have been
         * applied to them. If you want the vertex data before any mesh deformers have been applied to it, use the
         * method named FindOriginalVertexData instead.
         * Here are some examples to get common vertex data:
         *
         * <pre>
         * Vector3* positions = (Vector3*)mesh->FindVertexData( Mesh::ATTRIB_POSITIONS );   // the positions
         * Vector3* normals   = (Vector3*)mesh->FindVertexData( Mesh::ATTRIB_NORMALS   );   // the normals
         * Vector4* tangents  = (Vector4*)mesh->FindVertexData( Mesh::ATTRIB_TANGENTS  );   // first set of tangents, can be nullptr, and note the Vector4!
         * AZ::Vector2* uvCoords  = static_cast<AZ::Vector2*>(mesh->FindVertexData( Mesh::ATTRIB_UVCOORDS  ));  // the first set of UVs, can be nullptr
         * AZ::Vector2* uvCoords2 = static_cast<AZ::Vector2*>(mesh->FindVertexData( Mesh::ATTRIB_UVCOORDS, 1 ));    // the second set of UVs, can be nullptr
         * </pre>
         *
         * @param layerID The layer type ID to get the information from.
         * @param occurrence The layer number to get. Where 0 means the first layer, 1 means the second, etc. This is used
         *                   when there are multiple layers of the same type. An example is a mesh having multiple UV layers.
         * @result A void pointer to the layer data. You have to typecast yourself.
         */
        void* FindVertexData(uint32 layerID, size_t occurrence = 0) const;

        void* FindVertexDataByName(uint32 layerID, const char* name) const;

        /**
         * Find specific original vertex data in the mesh.
         * The difference between the original vertex data and current vertex data as returned by FindVertexData is that the
         * original vertex data is the data stored in the base pose, before any mesh deformers are being applied to it.
         * Here are some examples to get common vertex data:
         *
         * <pre>
         * Vector3* positions = (Vector3*)mesh->FindOriginalVertexData( Mesh::ATTRIB_POSITIONS );   // the positions
         * Vector3* normals   = (Vector3*)mesh->FindOriginalVertexData( Mesh::ATTRIB_NORMALS   );   // the normals
         * Vector4* tangents  = (Vector4*)mesh->FindOriginalVertexData( Mesh::ATTRIB_TANGENTS  );   // first set of tangents, can be nullptr, and note the Vector4!
         * AZ::Vector2* uvCoords  = static_cast<AZ::Vector2*>(mesh->FindOriginalVertexData( Mesh::ATTRIB_UVCOORDS  ));  // the first set of UVs, can be nullptr
         * AZ::Vector2* uvCoords2 = static_cast<AZ::Vector2*>(mesh->FindOriginalVertexData( Mesh::ATTRIB_UVCOORDS, 1 ));    // the second set of UVs, can be nullptr
         * </pre>
         *
         * @param layerID The layer type ID to get the information from.
         * @param occurrence The layer number to get. Where 0 means the first layer, 1 means the second, etc. This is used
         *                   when there are multiple layers of the same type. An example is a mesh having multiple UV layers.
         * @result A void pointer to the layer data. You have to typecast yourself.
         */
        void* FindOriginalVertexData(uint32 layerID, size_t occurrence = 0) const;

        void* FindOriginalVertexDataByName(uint32 layerID, const char* name) const;

        /**
         * Calculate the axis aligned bounding box of this mesh, after transforming the positions with the provided transform.
         * @param outBoundingBox The bounding box that will contain the bounds after executing this method.
         * @param Transform The transformation to transform all vertex positions with.
         * @param vertexFrequency This is the for loop increase counter value. A value of 1 means every vertex will be processed
         *                        while a value of 2 means every second vertex, etc. The value must be 1 or higher.
         */
        void CalcAabb(AZ::Aabb* outBoundingBox, const Transform& transform, uint32 vertexFrequency = 1);

        /**
         * The mesh type used to indicate if a mesh is either static, like a cube or building, cpu deformed, if it needs to be processed on the CPU, or GPU deformed if it can be processed fully on the GPU.
         */
        enum EMeshType
        {
            MESHTYPE_STATIC         = 0,    /**< Static rigid mesh, like a cube or building (can still be position/scale/rotation animated though). */
            MESHTYPE_CPU_DEFORMED   = 1,    /**< Deformed on the CPU. */
            MESHTYPE_GPU_DEFORMED   = 2     /**< Deformed on the GPU. */
        };

        /**
         * Check for a given mesh how we categorize it. A mesh can be either static, like a cube or building, dynamic if it has mesh deformers which
         * have to be processed on the CPU like a morphing deformer or gpu skinned if they only have a skinning deformer applied.
         * There are additional criteria like the maximum number of influences and the maximum number of bones per submesh to make it fit to different hardware specifications.
         * @param lodLevel The geometry LOD level of the mesh to check.
         * @param actor The actor where this mesh belongs to.
         * @param nodeIndex The index of the node that has this mesh.
         * @param forceCPUSkinning If true the function will never return MESHTYPE_GPUSKINNED which means that no hardware processing will be used.
         * @param maxInfluences The maximum number of influences per vertex that can be processed on hardware. If there will be more influences the mesh will be processed in software which will be very slow.
         * @param maxBonesPerSubMesh The maximum number of bones per submesh can be processed on hardware. If there will be more bones per submesh the mesh will be processed in software which will be very slow.
         * @return The mesh type meaning if the given mesh is static like a cube or building or if is deformed by the GPU or CPU.
         */
        EMeshType ClassifyMeshType(size_t lodLevel, Actor* actor, size_t nodeIndex, bool forceCPUSkinning, uint32 maxInfluences, uint32 maxBonesPerSubMesh) const;

        /**
         * Debug log information.
         * Information will be logged using LOGLEVEL_DEBUG.
         */
        void Log();

        /**
         * Convert the 32-bit vertex indices to 16-bit index values.
         * Do not call this function more than once per mesh. The number of indices will stay the same
         * while the size of the index buffer will be half as big as before.
         * When calling GetIndices() make sure to type-cast the returned pointer to an uint16*.
         * @return True in case all indices have been ported correctly, false if any of the indices was out of the 16-bit range.
         */
        bool ConvertTo16BitIndices();

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
         * Calculate how many triangles this mesh has.
         * In case the mesh contains polygons of more than 3 vertices, triangulation will be taken into account.
         * @result The number of triangles that are needed to draw this mesh.
         */
        uint32 CalcNumTriangles() const;

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
        AZStd::vector< VertexAttributeLayer* >   m_vertexAttributes;

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
