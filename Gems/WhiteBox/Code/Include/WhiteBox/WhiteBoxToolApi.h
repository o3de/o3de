/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class Transform;
}

namespace AZ::IO
{
    class GenericStream;
}

namespace WhiteBox
{
    //! Opaque handle to a WhiteBoxMesh.
    //! @note All Api calls require a white box mesh pointer as the first argument.
    struct WhiteBoxMesh;

    //! Wrapper class to provide type safe index/handle.
    //! @note Make use of the template tag technique (phantom types)
    //! to provide type safe variants of all handles.
    template<typename Tag>
    class GenericHandle
    {
    public:
        GenericHandle() = default;
        explicit GenericHandle(const int index)
            : m_index(index)
        {
        }

        bool IsValid() const
        {
            return m_index >= 0;
        }

        int Index() const
        {
            return m_index;
        }

        bool operator==(const GenericHandle& handle) const
        {
            return m_index == handle.m_index;
        }

        bool operator!=(const GenericHandle& handle) const
        {
            return !operator==(handle);
        }

        bool operator<(const GenericHandle& handle) const
        {
            return m_index < handle.m_index;
        }

    private:
        int m_index = -1;
    };

    namespace Api
    {
        //! Unique identifier for a vertex in the mesh.
        using VertexHandle = GenericHandle<struct VertexHandleTag>;
        //! Unique identifier for a face (triangle) in the mesh.
        using FaceHandle = GenericHandle<struct FaceHandleTag>;
        //! Unique identifier for an edge in the mesh.
        using EdgeHandle = GenericHandle<struct EdgeHandleTag>;
        //! Unique identifier for a halfedge in the mesh.
        using HalfedgeHandle = GenericHandle<struct HalfedgeHandleTag>;

        //! Alias for a collection of vertex handles.
        using VertexHandles = AZStd::vector<VertexHandle>;
        //! Alias for a collection of multiple vertex handle lists.
        using VertexHandlesCollection = AZStd::vector<VertexHandles>;
        //! Alias for a collection of multiple vertex position lists.
        using VertexPositionsCollection = AZStd::vector<AZStd::vector<AZ::Vector3>>;
        //! Alias for a collection of face handles.
        using FaceHandles = AZStd::vector<FaceHandle>;
        //! Alias for a collection of edge handles.
        using EdgeHandles = AZStd::vector<EdgeHandle>;
        //! Alias for a collection of multiple edge handle lists.
        using EdgeHandlesCollection = AZStd::vector<EdgeHandles>;
        //! Alias for a collection of halfedge handles.
        using HalfedgeHandles = AZStd::vector<HalfedgeHandle>;
        //! Alias for a collection of multiple halfedge handle lists.
        using HalfedgeHandlesCollection = AZStd::vector<HalfedgeHandles>;
        //! Alias for position of face vertices.
        using Face = AZStd::array<AZ::Vector3, 3>;
        //! Alias for a collection of faces.
        using Faces = AZStd::vector<Face>;

        //! Underlying representation of the White Box mesh (serialized halfedge data).
        using WhiteBoxMeshStream = AZStd::vector<AZ::u8>;

        //! Represents the vertex handles to be used to form a new face.
        struct FaceVertHandles
        {
            AZStd::array<VertexHandle, 3> m_vertexHandles;
        };

        //! Alias for a collection of multiple face vert handles.
        using FaceVertHandlesList = AZStd::vector<FaceVertHandles>;

        //! Alias for a collection of multiple face vert handle lists.
        using FaceVertHandlesCollection = AZStd::vector<FaceVertHandlesList>;

        //! A type safe way to ask for either the first of second halfedge handle from an edge handle.
        enum class EdgeHalfedge
        {
            First,
            Second
        };

        //! A wrapper to group both 'user' edges and 'mesh' edges.
        //! A 'user' edge is associated with a polygon and the user can interact with it (a logical edge).
        //! A 'mesh' edge is an edge the user cannot currently interact with. It can turn into a user edge
        //! by clicking it in edge activation mode. A 'mesh' edge is an interior edge of a polygon.
        struct EdgeTypes
        {
            EdgeHandles m_user;
            EdgeHandles m_mesh;
        };

        //! A polygon handle is an internal grouping of face handles a user can select and interact with.
        //! @note A polygon handle can consist of 1-N face handles (commonly two).
        struct PolygonHandle
        {
            FaceHandles m_faceHandles;

            bool operator==(const PolygonHandle& handle) const
            {
                return m_faceHandles == handle.m_faceHandles;
            }

            bool operator!=(const PolygonHandle& handle) const
            {
                return !(operator==(handle));
            }
        };

        using PolygonHandles = AZStd::vector<PolygonHandle>; //!< Alias for a collection of polygon handles.

        //! Stores the before and after polygon handles potentially created during a polygon append (impression).
        struct RestoredPolygonHandlePair
        {
            PolygonHandle m_before;
            PolygonHandle m_after;
        };

        //!< Alias for a collection of restored polygon handle pairs.
        using RestoredPolygonHandlePairs = AZStd::vector<RestoredPolygonHandlePair>;

        //! Stores all relevant created/modified polygon handles from an append operation.
        struct AppendedPolygonHandles
        {
            PolygonHandle m_appendedPolygonHandle; //!< The primary new polygon handle that was created
                                                   //!< (usually the one being interacted with).
            RestoredPolygonHandlePairs m_restoredPolygonHandles; //!< A collection of the connected polygon handles to
                                                                 //!< the primary polygon handle that may have been
                                                                 //!< deleted and then re-added.
        };

        //! Custom deleter for WhiteBoxMesh opaque pointer.
        class WhiteBoxMeshDeleter
        {
        public:
            void operator()(WhiteBoxMesh* whiteBox)
            {
                DestroyWhiteBoxMesh(whiteBox);
            }

        private:
            static void DestroyWhiteBoxMesh(WhiteBoxMesh* whiteBox);
        };

        //! Alias for an owning pointer to the opaque WhiteBoxMesh type.
        using WhiteBoxMeshPtr = AZStd::unique_ptr<WhiteBoxMesh, WhiteBoxMeshDeleter>;

        //! Return an owning pointer to a newly created WhiteBoxMesh.
        //! @note `delete` does not need to be called as the memory will automatically be reclaimed when
        //! the WhiteBoxMeshPtr goes out of scope (or reset() is explicitly called).
        WhiteBoxMeshPtr CreateWhiteBoxMesh();

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Handles

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Mesh - handle operations/queries that relate to the entire mesh.
        //
        //! Return all vertex handles in the mesh.
        VertexHandles MeshVertexHandles(const WhiteBoxMesh& whiteBox);

        //! Return all edge handles in the mesh.
        //! @note this includes interior edges of a polygon.
        EdgeHandles MeshEdgeHandles(const WhiteBoxMesh& whiteBox);
        //! Return edge handles decomposed into 'user' edges and 'mesh' edges.
        //! @note: 'user' edges are those associated with a polygon and that can
        //! be interacted with, 'mesh' edges are interior edges. It is possible for
        //! edges to transition between 'user' and 'mesh' with show/hide operations.
        EdgeTypes MeshUserEdgeHandles(const WhiteBoxMesh& whiteBox);

        //! Return all face handles in the mesh.
        FaceHandles MeshFaceHandles(const WhiteBoxMesh& whiteBox);

        //! Return all polygon handles in the mesh.
        PolygonHandles MeshPolygonHandles(const WhiteBoxMesh& whiteBox);

        //! Return all unique edges bordering the polygons in the mesh.
        EdgeHandles MeshPolygonEdgeHandles(const WhiteBoxMesh& whiteBox);

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Face - handle operations/queries that relate to a single face.
        //
        //! Return all halfedge handles corresponding to a given face.
        //! @note In all cases the number of halfedge handles returned for a face should be three.
        HalfedgeHandles FaceHalfedgeHandles(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);

        //! Return all edge handles that belong to a face.
        EdgeHandles FaceEdgeHandles(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);

        //! Return all vertex handles corresponding to a given face.
        //! @note In all cases the number of vertex handles returned for a face should be three.
        VertexHandles FaceVertexHandles(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);

        //! Return the polygon handle containing the given face.
        //! @note Calling this function from outside of the Api should always return a non-empty
        //! PolygonHandle however internally there may be occasions were certain invariants are
        //! temporarily broken and a FaceHandle may not yet have been added to a PolygonHandle.
        //! e.g. While splitting an Edge.
        PolygonHandle FacePolygonHandle(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Vertex - handle operations/queries that relate to a vertex
        //
        //! Return all outgoing halfedge handles from a given vertex.
        //! @note The outgoing halfedges may span multiple faces.
        HalfedgeHandles VertexOutgoingHalfedgeHandles(const WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Return all incoming halfedge handles to a given vertex.
        //! @note The incoming halfedges may span multiple faces.
        HalfedgeHandles VertexIncomingHalfedgeHandles(const WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Return all halfedge handles (incoming and outgoing) for a given vertex.
        //! @note The halfedges may span multiple faces.
        HalfedgeHandles VertexHalfedgeHandles(const WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Return all edge handles for a given vertex.
        //! @note The edge handles returned will include both 'user' and 'mesh' edges.
        EdgeHandles VertexEdgeHandles(const WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Return user edge handles for a given vertex.
        //! @note The edge handles returned will only include 'user' edges.
        EdgeHandles VertexUserEdgeHandles(const WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Side - handle operations/queries that relate to a side.
        // @note A 'side' is defined as a collection of faces that all share the same normal and are connected/adjacent.
        //
        //! Return all vertex handles that correspond with a given side.
        //! @note All vertex handles that lie on the same plane as the side.
        VertexHandles SideVertexHandles(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);

        //! Return all faces that correspond to a given side.
        FaceHandles SideFaceHandles(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);

        //! Return only the vertices that bound the edge of the side.
        //! @note No internal vertices will be returned.
        //! @note A vector of vectors is returned as there may be multiple vertex loops for a given side.
        VertexHandlesCollection SideBorderVertexHandles(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);

        //! Return only the halfedges that bound the edge of the side.
        //! @note No internal halfedges will be returned (that is any halfedges
        //! that have an opposite face handle with the same normal).
        //! @note A vector of vectors is returned as there may be multiple halfedge loops for a given side.
        HalfedgeHandlesCollection SideBorderHalfedgeHandles(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Halfedge - handle operations/queries that relate to a single halfedge.
        //
        //! Return the face handle that this halfedge belongs to.
        FaceHandle HalfedgeFaceHandle(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);

        //! Return the opposite halfedge handle of the passed in halfedge handle.
        //! @note Is it possible the returned halfedge handle may be invalid if the
        //! opposite halfedge does not belong to a face (this may happen with a 2d
        //! mesh when looking at a halfedge that bounds the mesh).
        HalfedgeHandle HalfedgeOppositeHalfedgeHandle(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);

        //! Return the opposite face handle of the passed in halfedge handle.
        //! @note It is possible the returned face handle may be invalid if the
        //! opposite halfedge does not belong to a face (this may happen with a 2d
        //! mesh when looking at a halfedge that bounds the mesh).
        FaceHandle HalfedgeOppositeFaceHandle(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);

        //! Return the vertex handle when following the direction/orientation of
        //! the halfedge handle and looking at what vertex it is pointing to.
        VertexHandle HalfedgeVertexHandleAtTip(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);

        //! Return the vertex handle when following the direction/orientation of
        //! the halfedge handle and looking at what vertex it is coming from.
        VertexHandle HalfedgeVertexHandleAtTail(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);

        //! Return the edge handle the halfedge handle is related to.
        EdgeHandle HalfedgeEdgeHandle(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);

        //! Return the next halfedge handle the halfedge is connected to.
        HalfedgeHandle HalfedgeHandleNext(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);

        //! Return the previous halfedge handle the halfedge is connected to.
        HalfedgeHandle HalfedgeHandlePrevious(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Edge - handle operations/queries that related to a single edge.
        //
        //! Return the two adjacent faces for a given edge.
        //! If the edge is on a border, only one face handle is returned.
        AZStd::vector<FaceHandle> EdgeFaceHandles(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);

        //! Return the first or second halfedge handle of a given edge handle.
        //! @param edgeHalfedge Used to ask for either the first or second halfedge of an edge.
        HalfedgeHandle EdgeHalfedgeHandle(
            const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle, EdgeHalfedge edgeHalfedge);

        //! Return either one or two halfedge handles for a given edge handle (depending on if
        //! the edge is a boundary or not).
        HalfedgeHandles EdgeHalfedgeHandles(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Polygon - handle operations/queries that relate to polygons.
        //
        //! Return all vertex handles that are associated with the polygon.
        VertexHandles PolygonVertexHandles(const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Return all vertex handles that bound the polygon (vertices at the edge).
        //! @note These will be ordered CCW.
        //! @note A vector of vectors is returned as there may be multiple vertex loops for a given polygon.
        VertexHandlesCollection PolygonBorderVertexHandles(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Return all vertex handles that bound the polygon (vertices at the edge).
        //! @note These will be ordered CCW.
        //! @note Return the vector of vectors as a flattened list. May contain multiple loops that are not connected.
        VertexHandles PolygonBorderVertexHandlesFlattened(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Return all halfedge handles associated with the given polygon (this includes interior edges).
        HalfedgeHandles PolygonHalfedgeHandles(const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Return all halfedge handles that border the polygon (synonymous with 'logical' edges).
        //! @note This excludes interior halfedges.
        //! @note A vector of vectors is returned as there may be multiple vertex loops for a given polygon.
        HalfedgeHandlesCollection PolygonBorderHalfedgeHandles(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Return all halfedge handles that border the polygon (synonymous with 'logical' edges).
        //! @note This excludes interior halfedges.
        //! @note Return the vector of vectors as a flattened list. May contain multiple loops that are not connected.
        HalfedgeHandles PolygonBorderHalfedgeHandlesFlattened(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Return all edge handles that border the polygon (synonymous with 'logical' edges).
        //! @note This excludes interior edges.
        //! @note A vector of vectors is returned as there may be multiple edge loops for a given polygon.
        EdgeHandlesCollection PolygonBorderEdgeHandles(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Return all edge handles that border the polygon.
        //! @note This excludes interior edges.
        //! @note Return the vector of vectors as a flattened list. May contain multiple loops that are not connected.
        EdgeHandles PolygonBorderEdgeHandlesFlattened(const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Values

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Mesh - value operations/queries that relate to the entire mesh.
        //
        //! Return how many faces the white box mesh has.
        AZ::u64 MeshFaceCount(const WhiteBoxMesh& whiteBox);

        //! Return how many vertices the white box mesh has.
        AZ::u64 MeshVertexCount(const WhiteBoxMesh& whiteBox);

        //! Return how many halfedges the white box mesh has.
        AZ::u64 MeshHalfedgeCount(const WhiteBoxMesh& whiteBox);

        //! Return all face value types in the mesh.
        //! @note Face is a collection for three vertices/positions.
        Faces MeshFaces(const WhiteBoxMesh& whiteBox);

        //! Return the positions of all vertices in the mesh.
        AZStd::vector<AZ::Vector3> MeshVertexPositions(const WhiteBoxMesh& whiteBox);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Vertex - value operations/queries that relate to vertices.
        //
        //! Return the vertex position of the requested vertex handle.
        //! @note A valid vertex handle must be provided. This function will fail if a vertex
        //! is passed that does not exist in the mesh.
        AZ::Vector3 VertexPosition(const WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Return all vertex positions corresponding to the collection of vertex handles passed in.
        //! @note All vertex handles in the collection must exist in the mesh. This function will fail
        //! if a vertex is passed that does not exist in the mesh.
        AZStd::vector<AZ::Vector3> VertexPositions(const WhiteBoxMesh& whiteBox, const VertexHandles& vertexHandles);

        //! Return if a vertex is hidden or not.
        bool VertexIsHidden(const WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Return if a vertex is isolated or not.
        //! @note A vertex is isolated if it has no connecting 'user' edges.
        bool VertexIsIsolated(const WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Return user edge vectors for a given vertex.
        //! @note The edge vectors returned will only include 'user' edges and will not be normalized.
        //! @note Any invalid (zero) edge vectors will be filtered out and not returned.
        AZStd::vector<AZ::Vector3> VertexUserEdgeVectors(const WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Return user edge axes for a given vertex.
        //! @note The edge axes returned will only include 'user' edges and will be normalized.
        //! @note Any invalid (zero) edge axes will be filtered out and not returned.
        AZStd::vector<AZ::Vector3> VertexUserEdgeAxes(const WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Face - value operations/queries that relate to faces.
        //
        //! Return the normal associated with the given face handle.
        //! @note The face handle passed in must be known to exist in the mesh otherwise the call will fail.
        AZ::Vector3 FaceNormal(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);

        //! Return all vertex positions for a given face handle.
        //! @note The face handle passed in must be known to exist in the mesh otherwise the call will fail.
        AZStd::vector<AZ::Vector3> FaceVertexPositions(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);

        //! Return positions of each face in the collection.
        //! Vertex positions for each face will be returned (not unique vertex positions for all faces).
        //! @note FacesPositions effectively returns a triangle list of vertex positions.
        //! @note The face handles passed in must be known to exist in the mesh otherwise the call will fail.
        AZStd::vector<AZ::Vector3> FacesPositions(const WhiteBoxMesh& whiteBox, const FaceHandles& faceHandles);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Halfedge - value operations/queries that relate to halfedges.
        //
        //! Return the texture coordinate (uv) associated with the given halfedge handle.
        AZ::Vector2 HalfedgeUV(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);

        //! Return the vertex position at the tip of the half edge.
        AZ::Vector3 HalfedgeVertexPositionAtTip(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);

        //! Return the vertex position at the tail of the half edge.
        AZ::Vector3 HalfedgeVertexPositionAtTail(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);

        //! Return if the halfedge is at a boundary (i.e. the halfedge only has one face associated with it).
        bool HalfedgeIsBoundary(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Edge - value operations/queries that relate to edges.
        //
        //! Return the vertex positions at the tip of each edge.
        AZStd::array<AZ::Vector3, 2> EdgeVertexPositions(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);

        //! Return the vertex handles at each end of the edge.
        AZStd::array<VertexHandle, 2> EdgeVertexHandles(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);

        //! Return the normalized axis of the edge.
        //! @note Internally uses the 'first' halfedge, direction will be from tail to tip.
        //! @note It is possible EdgeAxis will return a zero vector if the two edge vertices are at the same position.
        AZ::Vector3 EdgeAxis(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);

        //! Return the edge for the corresponding edge handle.
        //! @note The edge vector will not be normalized. It will be the length of the distance between the two
        //! vertices.
        //! @note It is possible EdgeVector will return a zero vector if the two edge vertices are at the same position.
        AZ::Vector3 EdgeVector(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);

        //! Return if the edge is at a boundary (i.e. the edge only has one halfedge associated with it).
        bool EdgeIsBoundary(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);

        //! Return all connected edges that have been merged through vertex hiding.
        EdgeHandles EdgeGrouping(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);

        //! Return if an edge is hidden or not.
        bool EdgeIsHidden(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Polygon - value operations/queries that relate to polygons.
        //
        //! Return the average normal of a polygon composed of several faces (average the normal across the faces).
        AZ::Vector3 PolygonNormal(const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Will return a transform aligned to the orientation of the polygon.
        //! @param pivot Most often will be the midpoint of the polygon but can be customized for other
        //! use cases such as non-uniform scaling.
        AZ::Transform PolygonSpace(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle, const AZ::Vector3& pivot);

        //! Will return a transform aligned to the direction of the edge.
        //! @param pivot Most often will be the midpoint of the edge but can be customized for other
        //! use cases such as non-uniform scaling.
        AZ::Transform EdgeSpace(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle, const AZ::Vector3& pivot);

        //! Return all vertex positions associated with the polygon handle.
        //! @note This is a helper which internally calls PolygonVertexHandles and then does a
        //! transformation from vertex handles to vertex positions.
        AZStd::vector<AZ::Vector3> PolygonVertexPositions(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Return all border vertex positions associated with the polygon handle.
        //! @note This is a helper which internally calls PolygonBorderVertexHandles and then does a
        //! transformation from vertex handles to vertex positions.
        //! @note A vector of vectors is returned as there may be multiple vertex loops for a given polygon.
        VertexPositionsCollection PolygonBorderVertexPositions(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Return positions of each face in the polygon.
        //! @note This call is a convenience wrapper for FacesPositions.
        //! @note PolygonFacesPositions effectively returns a triangle list of vertex positions.
        AZStd::vector<AZ::Vector3> PolygonFacesPositions(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Defaults

        //! Accepts a created but uninitialized white box and generates a cube for editing.
        //! @note A polygon will be created for each side of the cube.
        //! @return A vector of the polygon handles that were added.
        PolygonHandles InitializeAsUnitCube(WhiteBoxMesh& whiteBox);
        //! Accepts a created but uninitialized whiteBox and generates a quad for editing.
        //! @note A polygon will be created for the quad (it is currently only 1 sided, CCW winding order).
        //! @return The polygon handle that was added.
        PolygonHandle InitializeAsUnitQuad(WhiteBoxMesh& whiteBox);
        //! Accepts a created but uninitialized whiteBox and generates a triangle for editing.
        //! @note A polygon will be created for the triangle (it is currently only 1 sided, CCW winding order).
        //! @return The polygon handle that was added.
        PolygonHandle InitializeAsUnitTriangle(WhiteBoxMesh& whiteBox);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Mutations
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //
        //! Remove faces from the mesh.
        //! @note Collect all face handles to remove and call once per operation --
        //! do not call in a loop, as handles may be invalidated during garbage_collect.
        void RemoveFaces(WhiteBoxMesh& whiteBox, const FaceHandles& faceHandles);
        //! Delete vertices that belong to no face and compact the mesh (RemoveFaces keeps them).
        void RemoveIsolatedVertices(WhiteBoxMesh& whiteBox);
        //! Set the position of the provided vertex handle to the new vertex position.
        void SetVertexPosition(WhiteBoxMesh& whiteBox, VertexHandle vertexHandle, const AZ::Vector3& position);

        //! Set the position of the provided vertex handle to the new vertex position
        //! and immediately recalculate the UVs for the mesh.
        //! @note Internally calls SetVertexPosition and CalculatePlanarUVs.
        void SetVertexPositionAndUpdateUVs(
            WhiteBoxMesh& whiteBox, VertexHandle vertexHandle, const AZ::Vector3& position);

        //! Add a vertex to the mesh.
        //! @return A handle to the newly added vertex.
        //! @note This is a low level call that should not generally be used as it usually
        //! requires calling AddFace immediately after adding three consecutive vertex handles.
        VertexHandle AddVertex(WhiteBoxMesh& whiteBox, const AZ::Vector3& vertex);

        //! Add a face to the mesh.
        //! @return A handle to the newly added face.
        //! @note This is a low level call that should not generally be used as internally
        //! a Polygon should be created when one or more faces are added - prefer using the Append<> operations.
        FaceHandle AddFace(WhiteBoxMesh& whiteBox, VertexHandle v0, VertexHandle v1, VertexHandle v2);

        //! Create a Polygon from a list of FaceVertHandles.
        //! Each FaceVertHandle represents an individual Face in the Polygon (3 vertex handles forming a Face/Triangle).
        //! @return The newly added PolygonHandle.
        //! @note faceVertHandles should have at least one faceVertHandle.
        PolygonHandle AddPolygon(WhiteBoxMesh& whiteBox, const FaceVertHandlesList& faceVertHandles);

        //! Create a three sided (triangle) polygon from three VertexHandles.
        //! Each VertexHandle represents an individual Vertex in the Polygon (3 vertex handles forming a Face/Triangle).
        //! @return The newly added PolygonHandle.
        PolygonHandle AddTriPolygon(WhiteBoxMesh& whiteBox, VertexHandle vh0, VertexHandle vh1, VertexHandle vh2);

        //! Create four sided quad polygon (two triangles sharing an edge) from four VertexHandles.
        //! Each VertexHandle represents an individual Vertex in the Polygon (4 vertex handles forming
        //! two Faces in one Quad).
        //! @return The newly added PolygonHandle.
        PolygonHandle AddQuadPolygon(
            WhiteBoxMesh& whiteBox, VertexHandle vh0, VertexHandle vh1, VertexHandle vh2, VertexHandle vh3);

        //! Extrude a single polygon in the mesh.
        //! A lateral face will be created for each edge of the polygon that is extruded.
        //! TranslatePolygonAppend can be thought of as an extrusion along the normal of the polygon.
        //! @note distance must not be zero.
        PolygonHandle TranslatePolygonAppend(
            WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle, float distance);

        //! Extrude a single polygon in the mesh.
        //! A lateral face will be created for each edge of the polygon that is extruded.
        //! TranslatePolygonAppend can be thought of as an extrusion along the normal of the polygon.
        //! @note distance must not be zero.
        //! @note TranslatePolygonAppendAdvanced returns additional information including connected/neighboring
        //! polygons that were removed and then re-added if shared vertices were changed. This information is important
        //! for types that hold references to polygon handles that may change (e.g. Modifiers).
        AppendedPolygonHandles TranslatePolygonAppendAdvanced(
            WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle, float distance);

        //! Translate an edge in the mesh.
        //! Moves an edge specified by the displacement.
        void TranslateEdge(WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle, const AZ::Vector3& displacement);

        //! Translate an edge while at the same time duplicating it to create a total of three new polygons.
        //! One new polygon will be created aligned to the edge that was translated (most likely a quad)
        //! and two polygons at the top and bottom of the new edge will be inserted (most likely triangles).
        //! @note displacement must not be zero.
        EdgeHandle TranslateEdgeAppend(WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle, const AZ::Vector3& displacement);

        //! Duplicate and then scale a polygon in the mesh.
        //! @note A new polygon will be inserted into the mesh but will be scaled uniformly along
        //! the tangent of the face (orthogonal to the normal of the polygon/face).
        PolygonHandle ScalePolygonAppendRelative(
            WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle, float scale);

        //! Translate a polygon along its normal axis.
        //! @note distance can be positive or negative to move forward or backward respectively.
        void TranslatePolygon(WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle, float distance);

        //! Scale a polygon in-place.
        //! @note Scale will be applied relative to the existing scale of the polygon.
        //! @note All edges/vertices of a polygon will be scaled uniformly along the
        //! tangent of the face about the pivot.
        void ScalePolygonRelative(
            WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle, const AZ::Vector3& pivot, float scaleDelta);

        //! The supported CSG (constructive solid geometry) boolean operations.
        enum class BooleanOperation
        {
            Union, //!< The combined volume of both meshes (A + B).
            Subtraction, //!< The volume of the first mesh with the second mesh cut away (A - B).
            Intersection //!< Only the volume shared by both meshes (A & B).
        };

        //! Which CSG solver backend performs the boolean.
        enum class CsgSolver
        {
            //! Volumetric (Manifold library): robust, needs closed manifold inputs; inverted normals mean the complement.
            Manifold,
            //! Face-based BSP, like Blender's "Fast": tolerates open meshes and inward-facing rooms; less robust on slivers.
            Fast
        };

        //! Perform a CSG boolean operation between two white box meshes.
        //! The result replaces the contents of whiteBox; the operand mesh is unmodified.
        //! @param whiteBox The mesh to apply the boolean operation to (modified in place).
        //! @param operand The second operand (the 'tool' mesh), e.g. the volume to cut away in a subtraction.
        //! @param operandTransform Transform positioning operand in the local space of whiteBox
        //! (for two entities, worldFromLocalA.GetInverse() * worldFromLocalB).
        //! @param operation Which boolean operation to apply.
        //! @param solver Which CSG backend to use (Manifold by default; Fast/BSP for the face-based,
        //! brush-style path that handles shells and inverted normals). See CsgSolver.
        //! @return True if the operation succeeded, false otherwise (e.g. the meshes do not
        //! intersect, or one of them is not a closed manifold). whiteBox is unmodified on failure.
        //! @note With the Manifold solver both meshes must be closed (watertight). Coplanar faces in
        //! the result are merged into white box polygons, but UVs are recalculated so custom UVs are
        //! not preserved.
        bool ApplyMeshBoolean(
            WhiteBoxMesh& whiteBox, const WhiteBoxMesh& operand, const AZ::Transform& operandTransform,
            BooleanOperation operation, CsgSolver solver = CsgSolver::Manifold);

        //! Weld coincident vertices and regroup coplanar triangles (what CSG does to its inputs), so a
        //! non-manifold mesh stops blocking booleans. False if the mesh was empty.
        bool RepairMesh(WhiteBoxMesh& whiteBox);

        //! Collider triangles with coplanar faces merged and re-triangulated (far fewer triangles); regions
        //! that cannot be simplified keep their original triangles. False if the mesh was empty.
        bool BuildColliderTriangles(
            const WhiteBoxMesh& whiteBox, AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices);

        //! Recalculate all normals of each face in the mesh.
        void CalculateNormals(WhiteBoxMesh& whiteBox);

        //! Zero/clear all uvs.
        void ZeroUVs(WhiteBoxMesh& whiteBox);

        //! Calculate planar uvs for all halfedges (these are where the uvs are stored) in the mesh.
        //! @note This will produce a tiling effect across each side of the mesh.
        void CalculatePlanarUVs(WhiteBoxMesh& whiteBox);

        //! Hide an edge to merge two polygons that share the same edge.
        //! Return the handle to the merged polygon.
        PolygonHandle HideEdge(WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);

        //! Flip an edge within a quad to subdivide the quad across another diagonal.
        //! Return whether the edge was able to be flipped (operation may fail).
        bool FlipEdge(WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);

        //! Split an edge and subdivide any connected faces in two, the position
        //! is where to insert the new vertex.
        //! Return the vertex handle where the edge was split.
        //! @note If the edge is at a boundary only one face will be split.
        VertexHandle SplitEdge(WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle, const AZ::Vector3& position);

        //! Split a face by subdividing it into three new faces, the position
        //! is where to insert the new vertex.
        //! Return the vertex handle where the face was split.
        VertexHandle SplitFace(WhiteBoxMesh& whiteBox, FaceHandle faceHandle, const AZ::Vector3& position);

        //! Hide a vertex to merge the edges that share the same vertex.
        void HideVertex(WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Move an edge from a 'mesh' edge to a 'user' edge (a 'logical' edge).
        //! As a new polygon may not be immediately created, it is the caller's responsibility to
        //! store the edges that might form new polygons. When a new polygon is formed, the caller
        //! must remove the edges from the intermediate restoringEdgeHandles parameter.
        //! @return If successful, return the two newly formed polygons, otherwise an empty optional.
        //! @param restoringEdgeHandles Temporary buffer for edges that are being restored,
        //! must be maintained by the caller.
        AZStd::optional<AZStd::array<PolygonHandle, 2>> RestoreEdge(
            WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle, EdgeHandles& restoringEdgeHandles);

        //! Restore a vertex from its 'hidden' state and split any connected edges.
        void RestoreVertex(WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Attempt to restore a vertex from its 'hidden' state and split any connected edges.
        //! @note TryRestoreVertex will first check if there are any connected edges. If there are none, then the
        //! vertex will not be restored as it has no valid edges to connect with so cannot be interacted with.
        //! @return Returns if the vertex was restored or not.
        bool TryRestoreVertex(WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Removes all mesh data by clearing the mesh.
        void Clear(WhiteBoxMesh& whiteBox);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Helpers

        //! Calculate the midpoint of all vertices composing a polygon.
        AZ::Vector3 PolygonMidpoint(const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Calculate the midpoint of the two vertices composing an edge.
        AZ::Vector3 EdgeMidpoint(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);

        //! Calculate the midpoint of a face (three vertices).
        AZ::Vector3 FaceMidpoint(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);

        //! Calculate the midpoint of an arbitrary collection of vertices.
        AZ::Vector3 VerticesMidpoint(const WhiteBoxMesh& whiteBox, const VertexHandles& vertexHandles);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Serialization

        //! The result of attempting to deserialize a white box mesh from a white box mesh stream.
        enum class ReadResult
        {
            Full, //!< The white box mesh stream was full and was read into white box mesh (it is now initialized).
            Empty, //!< The white box mesh stream was empty so no white box mesh was loaded.
            Error //!< An error occurred while trying to deserialize white box mesh stream.
        };

        //! Take an input stream of bytes and create a white box mesh from the deserialized data.
        //! @return Will return ReadResult::Full if the white box mesh stream was filled with data and
        //! the white box mesh was initialized, ReadResult::Empty if white box mesh stream did not contain
        //! any data (white box mesh will be left empty) or ReadResult::Error if any error was encountered
        //! during deserialization.
        //! @note A white box mesh must have been created first.
        ReadResult ReadMesh(WhiteBoxMesh& whiteBox, const WhiteBoxMeshStream& input);

        //! Take an input stream and create a white box mesh from the deserialized data.
        //! @return Will return ReadResult::Full if the white box mesh stream was filled with data and
        //! the white box mesh was initialized, ReadResult::Empty if white box mesh stream did not contain
        //! any data (white box mesh will be left empty) or ReadResult::Error if any error was encountered
        //! during deserialization.
        //! @note The input stream must not skip white space characters (std::noskipws must be set on the stream).
        ReadResult ReadMesh(WhiteBoxMesh& whiteBox, std::istream& input);

        //! Take a white box mesh and write it out to a stream of bytes.
        //! @return Will return false if any error was encountered during serialization, true otherwise.
        //! Per-face material override. Invalid means inherit the entity's default.
        AZ::Data::AssetId FaceMaterial(const WhiteBoxMesh& whiteBox, FaceHandle face);
        //! Packed RGBA (R in the low byte); zero means no face-color override.
        AZ::u32 FacePaintColor(const WhiteBoxMesh& whiteBox, FaceHandle face);
        void SetFacePaintColor(WhiteBoxMesh& whiteBox, FaceHandle face, AZ::u32 color);
        void SetFaceMaterial(WhiteBoxMesh& whiteBox, FaceHandle face, const AZ::Data::AssetId& material);
        void SetPolygonMaterial(WhiteBoxMesh& whiteBox, const PolygonHandle& polygon, const AZ::Data::AssetId& material);

        //! Smoothing group bitmask (bit n = group n + 1); faces sharing a bit smooth across shared vertices, zero is flat.
        AZ::u32 FaceSmoothingGroups(const WhiteBoxMesh& whiteBox, FaceHandle face);
        void SetFaceSmoothingGroups(WhiteBoxMesh& whiteBox, FaceHandle face, AZ::u32 groups);
        //! Set, add (combine) or remove the bits on every face of the polygons.
        enum class SmoothingEdit
        {
            Set,
            Add,
            Remove
        };
        void SetPolygonSmoothingGroups(WhiteBoxMesh& whiteBox, const PolygonHandles& polygons, AZ::u32 groups, SmoothingEdit edit = SmoothingEdit::Set);
        //! Assign groups so neighbours within angleDegrees smooth together and sharper creases stay hard.
        void AutoSmoothPolygons(WhiteBoxMesh& whiteBox, const PolygonHandles& polygons, float angleDegrees);
        //! Per-corner normals in FaceHalfedgeHandles order: angle-weighted over faces sharing a smoothing group, else the face normal.
        AZStd::array<AZ::Vector3, 3> FaceCornerNormals(const WhiteBoxMesh& whiteBox, FaceHandle face);

        //! Vertex-blend layer weights (x: layer 2, y: layer 3). Unpainted vertices average their painted neighbours.
        AZ::Vector3 VertexBlend(const WhiteBoxMesh& whiteBox, VertexHandle vertex);
        bool VertexBlendPainted(const WhiteBoxMesh& whiteBox, VertexHandle vertex);
        void SetVertexBlend(WhiteBoxMesh& whiteBox, VertexHandle vertex, const AZ::Vector3& weights);
        //! Store every unpainted vertex's current (borrowed) weights as its own, so painting one vertex no longer spreads
        //! to unpainted neighbours through the averaging; a fresh mesh bakes to the base layer everywhere.
        void BakeVertexBlend(WhiteBoxMesh& whiteBox);
        //! Carry a vertex's painted weights to its copy in another mesh (an unpainted vertex stays unpainted).
        void CopyVertexBlend(WhiteBoxMesh& target, VertexHandle to, const WhiteBoxMesh& source, VertexHandle from);
        //! Whether any vertex has been painted, so callers can skip blend work on meshes that never used it.
        bool MeshHasVertexBlend(const WhiteBoxMesh& whiteBox);

        //! Which property Select Similar compares against the seed polygons.
        enum class SimilarBy
        {
            Material,
            Normal,
            Area,
            Sides,
            SmoothingGroups
        };
        //! Every polygon matching any seed on the chosen property (normal within toleranceDegrees, area within 5 percent). Seeds stay first.
        PolygonHandles FindSimilarPolygons(
            const WhiteBoxMesh& whiteBox, const PolygonHandles& seeds, SimilarBy similarBy, float toleranceDegrees = 1.0f);
        //! Edges matching any seed's length within 5 percent; vertices matching any seed's visible edge count.
        EdgeHandles FindSimilarEdges(const WhiteBoxMesh& whiteBox, const EdgeHandles& seeds);
        VertexHandles FindSimilarVertices(const WhiteBoxMesh& whiteBox, const VertexHandles& seeds);

        //! Split each polygon into quads (one per corner, meeting at its centre); shared borders split once. Transactional.
        bool SubdividePolygons(
            WhiteBoxMesh& whiteBox, const PolygonHandles& polygons, AZStd::string& error, PolygonHandles* created = nullptr);

        //! How a face's texture coordinates are generated each time UVs are recalculated.
        enum class UvProjectionMode : AZ::u32
        {
            World = 0,  //!< The original mapping: project along the face normal's dominant local axis.
            Planar = 1, //!< Project onto the face's own plane, so sloped faces do not stretch.
            Manual = 2  //!< Corner UVs are authored and kept through every edit; corners an edit creates follow the face's UV map.
        };

        //! The affine map a Manual face's UVs follow: uv = originUv + ((p - origin).uGradient, (p - origin).vGradient).
        //! It travels with the face's projection, so faces an edit creates from it get UVs that line up.
        struct ManualUvMap
        {
            AZ::Vector3 m_origin = AZ::Vector3::CreateZero();
            AZ::Vector2 m_originUv = AZ::Vector2::CreateZero();
            AZ::Vector3 m_uGradient = AZ::Vector3::CreateZero();
            AZ::Vector3 m_vGradient = AZ::Vector3::CreateZero();
            AZ::Vector3 m_normal = AZ::Vector3::CreateZero(); //!< Plane the map was taken on; zero when there is no map.

            bool IsValid() const { return !m_normal.IsZero(); }
            AZ::Vector2 Evaluate(const AZ::Vector3& position) const;
        };

        //! Per-face texture placement applied after projection (rotate, scale, offset); the default is the original mapping.
        struct UvProjection
        {
            UvProjectionMode m_mode = UvProjectionMode::World;
            AZ::Vector2 m_scale = AZ::Vector2(1.0f, 1.0f); //!< Texture repeats per metre along U and V.
            AZ::Vector2 m_offset = AZ::Vector2(0.0f, 0.0f);
            float m_rotationDegrees = 0.0f;
            ManualUvMap m_manual; //!< Manual mode only; not part of equality.

            bool IsDefault() const;
            bool operator==(const UvProjection& other) const;
            bool operator!=(const UvProjection& other) const { return !operator==(other); }
        };

        //! Freeze the polygons' current UVs as authored and switch them to Manual.
        void MakePolygonUvsManual(WhiteBoxMesh& whiteBox, const PolygonHandles& polygons);
        //! Author one corner's UV; its face becomes Manual first (keeping its other corners) if it was not.
        void SetHalfedgeManualUv(WhiteBoxMesh& whiteBox, HalfedgeHandle halfedge, const AZ::Vector2& uv);
        //! Whether a Manual face's corner holds an authored UV rather than waiting for one from the face's map.
        bool HalfedgeUvAuthored(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedge);

        //! Persistent per-face projection, honoured by every CalculatePlanarUVs call.
        UvProjection FaceUvProjection(const WhiteBoxMesh& whiteBox, FaceHandle face);
        void SetFaceUvProjection(WhiteBoxMesh& whiteBox, FaceHandle face, const UvProjection& projection);
        //! Set the projection on every face of the polygons and recalculate their UVs. Manual with a valid map transfers
        //! that map's UVs onto the polygons (paste); Manual without one freezes their current UVs.
        void SetPolygonUvProjection(WhiteBoxMesh& whiteBox, const PolygonHandles& polygons, const UvProjection& projection);
        //! Keep each polygon's mode and rotation but pick scale and offset so the texture spans it exactly once. Skips Manual faces.
        void FitPolygonUvProjection(WhiteBoxMesh& whiteBox, const PolygonHandles& polygons);
        //! Give every polygon the same tiling (repeats per metre on both axes), keeping its mode, rotation and texture anchor.
        //! Skips Manual faces.
        void NormalizePolygonTexelDensity(WhiteBoxMesh& whiteBox, const PolygonHandles& polygons, float repeatsPerMetre);
        //! Mean tiling over the polygons' faces (both axes), or one when there are none.
        float PolygonTexelDensity(const WhiteBoxMesh& whiteBox, const PolygonHandles& polygons);

        //! Rebuild the mesh from a triangle soup: coincident vertices weld, coplanar neighbours with one material become polygons.
        //! triangleMaterials is optional (one per triangle). Returns false and leaves the mesh untouched when nothing usable is left.
        bool BuildFromTriangles(
            WhiteBoxMesh& whiteBox, const AZStd::vector<AZ::Vector3>& positions, const AZStd::vector<AZ::u32>& indices,
            const AZStd::vector<AZ::Data::AssetId>& triangleMaterials = {});
        //! Copy material, paint and UV projection from one face to another, possibly in another mesh.
        void CopyFaceAttributes(WhiteBoxMesh& target, FaceHandle targetFace, const WhiteBoxMesh& source, FaceHandle sourceFace);
        //! Pairwise copy for many faces at once, looking each property up only once.
        void CopyFaceAttributes(
            WhiteBoxMesh& target, const FaceHandles& targetFaces, const WhiteBoxMesh& source, const FaceHandles& sourceFaces);

        bool WriteMesh(const WhiteBoxMesh& whiteBox, WhiteBoxMeshStream& output);

        //! Clones the white box mesh object into a new mesh.
        //! @return Will return null if any error was encountered during serialization, otherwise the cloned mesh.
        WhiteBoxMeshPtr CloneMesh(const WhiteBoxMesh& whiteBox);
        //! Copy a prepared mesh while preserving its topology handles and properties.
        void AssignMesh(WhiteBoxMesh& target, const WhiteBoxMesh& source);
        //! Transactional extrusion of connected boundary edges (or one solid edge using Sketch semantics).
        bool ExtrudeEdgeSelection(
            WhiteBoxMesh& mesh, const EdgeHandles& edges, const AZ::Vector3& offset,
            EdgeHandles& result, PolygonHandles& preview, AZStd::string& error);

        //! Extrude regions along their normals, or inset by a fraction (0, 1): convex planar regions scale, others get an even border.
        //! Preserves polygon divisions, face materials/paint and existing UVs. Transactional on failure.
        bool ExtrudeInsetRegions(
            WhiteBoxMesh& whiteBox, const PolygonHandles& selection, float amount, bool inset,
            PolygonHandles& result, AZStd::string& error, PolygonHandles* preview = nullptr);

        //! Bridge two boundary edges, open polygons or two facing caps with a connecting strip.
        //! Transactional: a failure returns a message and leaves the mesh unchanged.
        bool BridgeSelection(
            WhiteBoxMesh& whiteBox, const PolygonHandles& polygons, const EdgeHandles& edges, AZStd::string& error);

        //! Fill the planar hole bordered by the selected open edges with one polygon (concave allowed).
        //! Transactional: a failure returns a message and leaves the mesh unchanged.
        bool FillHole(
            WhiteBoxMesh& whiteBox, const EdgeHandles& edges, AZStd::string& error, PolygonHandle* filled = nullptr);

        //! Delete the selected polygons, keeping their vertices. Transactional.
        bool DeletePolygons(WhiteBoxMesh& whiteBox, const PolygonHandles& polygons, AZStd::string& error);

        //! As DeletePolygons, but at triangle granularity, so one triangle of a quad can go while
        //! its partner stays. Duplicate handles in the selection are ignored.
        bool DeleteFaces(WhiteBoxMesh& whiteBox, const FaceHandles& selection, AZStd::string& error);

        //! Merge a connected run of polygons into one by hiding every border they share. Transactional.
        bool MergePolygons(
            WhiteBoxMesh& whiteBox, const PolygonHandles& polygons, AZStd::string& error,
            PolygonHandle* merged = nullptr);

        //! Grow a polygon selection over neighbours facing the same way. Does not modify the mesh.
        PolygonHandles FindCoplanarRegion(
            const WhiteBoxMesh& whiteBox, const PolygonHandles& seeds, float toleranceDegrees = 1.0f);

        //! Grow a selection to the whole connected shell, crossing shared edges but not lone vertices.
        PolygonHandles FindLinkedPolygons(const WhiteBoxMesh& whiteBox, const PolygonHandles& seeds);
        EdgeHandles FindLinkedEdges(const WhiteBoxMesh& whiteBox, const EdgeHandles& seeds);
        VertexHandles FindLinkedVertices(const WhiteBoxMesh& whiteBox, const VertexHandles& seeds);

        //! Merge selected vertices at their average or the last selected one. Failure leaves the mesh unchanged.
        bool WeldVertices(WhiteBoxMesh& whiteBox, const VertexHandles& vertices, bool toLastSelected, AZStd::string& error);

        //! Cut an edge loop across a strip of convex quads at @p fraction along @p edge. Failure leaves the mesh unchanged.
        bool InsertEdgeLoop(WhiteBoxMesh& whiteBox, EdgeHandle edge, float fraction, AZStd::string& error);
        //! Expand edge seeds along regular quad loops, or along unambiguous open borders.
        //! Invalid handles and internal triangulation edges are ignored. Does not modify the mesh.
        EdgeHandles FindEdgeLoop(const WhiteBoxMesh& whiteBox, const EdgeHandles& seeds);
        //! Expand across opposite quad edges, stopping at non-quads and ambiguous topology.
        EdgeHandles FindEdgeRing(const WhiteBoxMesh& whiteBox, const EdgeHandles& seeds);

        //! Expand quads into the face strip through them; loop and ring are the two directions.
        PolygonHandles FindPolygonLoop(const WhiteBoxMesh& whiteBox, const PolygonHandles& seeds);
        PolygonHandles FindPolygonRing(const WhiteBoxMesh& whiteBox, const PolygonHandles& seeds);

        //! Grow one step (polygons sharing a corner, edges sharing an end, vertices one edge away); seeds stay first.
        PolygonHandles GrowPolygonSelection(const WhiteBoxMesh& whiteBox, const PolygonHandles& seeds);
        EdgeHandles GrowEdgeSelection(const WhiteBoxMesh& whiteBox, const EdgeHandles& seeds);
        VertexHandles GrowVertexSelection(const WhiteBoxMesh& whiteBox, const VertexHandles& seeds);
        //! The inverse of grow: drop every seed that touches an unselected neighbour. May return empty.
        PolygonHandles ShrinkPolygonSelection(const WhiteBoxMesh& whiteBox, const PolygonHandles& seeds);
        EdgeHandles ShrinkEdgeSelection(const WhiteBoxMesh& whiteBox, const EdgeHandles& seeds);
        VertexHandles ShrinkVertexSelection(const WhiteBoxMesh& whiteBox, const VertexHandles& seeds);

        //! One element type's worth of selection; conversion reads whichever member is filled.
        struct ElementSelection
        {
            PolygonHandles m_polygons;
            EdgeHandles m_edges;
            VertexHandles m_vertices;
        };
        //! Which element type a selection converts to.
        enum class SelectionElement
        {
            Vertex,
            Edge,
            Polygon
        };
        //! Convert to another element type: down takes every part, up takes what is enclosed (or touched when touching).
        ElementSelection ConvertSelection(
            const WhiteBoxMesh& whiteBox, const ElementSelection& source, SelectionElement target, bool touching);

        //! Move polygons into detached with all face attributes and UVs; refuses every face. Transactional.
        bool DetachPolygons(
            WhiteBoxMesh& whiteBox, const PolygonHandles& polygons, WhiteBoxMesh& detached, AZStd::string& error);

        //! Split polygons along straight cuts: a path in selection order, else polygons with two selected corners. Transactional.
        bool ConnectVertices(
            WhiteBoxMesh& whiteBox, const VertexHandles& vertices, AZStd::string& error, EdgeHandles* created = nullptr);

        //! Add a visible corner on a polygon border edge at fraction (0, 1) from its first endpoint; invalid on failure.
        VertexHandle InsertVertexOnEdge(WhiteBoxMesh& whiteBox, EdgeHandle edge, float fraction, AZStd::string& error);

        //! Bevel sharp edges by @p width with @p segments (1-32). Failure leaves the mesh unchanged.
        bool BevelEdges(WhiteBoxMesh& whiteBox, const EdgeHandles& edges, float width, int segments, AZStd::string& error, float profile = 0.5f);

        //! Insert 1-64 evenly spaced cuts as one transactional mesh edit. Slide [-1, 1] translates the group within its end margins.
        bool InsertEdgeLoops(WhiteBoxMesh& whiteBox, EdgeHandle edge, int count, AZStd::string& error, float slide = 0.0f);
        //! Read-only preview: pairs of local-space endpoints, using the same strip validation as insertion.
        bool PreviewEdgeLoops(
            WhiteBoxMesh& whiteBox, EdgeHandle edge, int count, AZStd::vector<AZ::Vector3>& lines, AZStd::string& error, float slide = 0.0f);

        //! A surface anchor in mesh-local coordinates. The face disambiguates overlapping surfaces.
        struct KnifePoint
        {
            FaceHandle m_face;
            AZ::Vector3 m_position = AZ::Vector3::CreateZero();
        };

        //! Cut the surface along the plane through two anchors and the view direction. Transactional;
        //! on success @p end references a resulting face, ready for the next segment.
        bool KnifeCut(
            WhiteBoxMesh& mesh, const KnifePoint& start, KnifePoint& end, const AZ::Vector3& viewDirection,
            AZStd::vector<AZ::Vector3>& lines, AZStd::string& error);

        //! Writes the white box mesh to an obj file at the specified path.
        //! @return Will return false if any error was encountered during serialization, true otherwise.
        bool SaveToObj(const WhiteBoxMesh& whiteBox, const AZStd::string& filePath);

        //! Writes the white box mesh to the io stream.
        //! @return Will return false if any error was encountered during serialization, true otherwise.
        bool SaveToWbm(const WhiteBoxMesh& whiteBox, AZ::IO::GenericStream& stream);

        //! Writes the white box mesh to a wbm file at the specified path.
        //! @return Will return false if any error was encountered during serialization, true otherwise.
        bool SaveToWbm(const WhiteBoxMesh& whiteBox, const AZStd::string& filePath);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Logging

        AZStd::string ToString(VertexHandle vertexHandle);
        AZStd::string ToString(FaceHandle faceHandle);
        AZStd::string ToString(EdgeHandle edgeHandle);
        AZStd::string ToString(HalfedgeHandle halfedgeHandle);
        AZStd::string ToString(const PolygonHandle& polygonHandle);
        AZStd::string ToString(const FaceVertHandles& faceVertHandles);
        AZStd::string ToString(const FaceVertHandlesList& faceVertHandlesList);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    } // namespace Api
} // namespace WhiteBox
