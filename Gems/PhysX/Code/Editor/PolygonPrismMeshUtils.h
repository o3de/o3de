/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/Math/Vector2.h>
#include <poly2tri.h>

namespace PolygonPrismMeshUtils
{
    //! The largest number of edges a polygon prism can have if it is to be represented as a PhysX convex mesh.
    //! Convex meshes are limited to 255 edges and (polygonal) faces.  An n-sided polygon prism has n + 2 faces and
    //! 2n vertices, so the largest number of edges for a polygon prism which can be used with PhysX is 127.  Prisms
    //! with more edges will need to be decomposed into a collection of simpler prisms.
    static const int MaxPolygonPrismEdges = 127;

    //! Calculates the three internal angles in a triangle.
    AZ::Vector3 CalculateAngles(p2t::Triangle& triangle);

    struct HalfEdge;

    //! A face in a doubly connected edge list (a data structure for efficiently manipulating meshes).
    struct Face
    {
        HalfEdge* m_edge = nullptr; //!< One arbitrary half-edge in this face.
        int m_numEdges = 0; //!< The number of edges this face has.
        bool m_removed = false; //!< Marks if the face has been removed due to merging with another face.
    };

    //! A half-edge in a doubly connected edge list (a data structure for efficiently manipulating meshes).
    //! An edge connecting two adjoining faces in the mesh is represented as two oppositely directed half-edges,
    //! which each half-edge belonging to one of the faces, and owning a pointer to its twin in the other face.
    struct HalfEdge
    {
        Face* m_face = nullptr; //!< The face this half-edge belongs to.
        AZ::Vector2 m_origin = AZ::Vector2::CreateZero(); //!< The point where this half-edge meets the previous half-edge.
        HalfEdge* m_prev = nullptr; //!< The previous half-edge.
        HalfEdge* m_next = nullptr; //!< The next half-edge.
        HalfEdge* m_twin = nullptr; //!< The half-edge which shares this edge, or nullptr if this edge has no adjacent face.
        float m_prevAngle = 0.0f; //!< The internal angle between this half-edge and the previous half-edge.
        float m_nextAngle = 0.0f; //!< The internal angle between this half-edge and the next half-edge.
        bool m_visited = false; //!< Marks if the half edge has been visited during the process of matching up twin edges.
        bool m_dirty = false; //!< Marks if an update is required because an adjacent internal edge has been removed.
    };

    //! An internal edge (twinned pair of half-edges).
    //! An internal edge means an edge in the interior of the mesh, so that it has two connected faces, as opposed to
    //! an edge on the exterior of the mesh, which would only be connected to one face.  The smallest of the four
    //! internal angles between this edge and the adjacent edges of the two connected faces is used to prioritize
    //! which internal edges to remove when merging faces to produce a convex decomposition.
    struct InternalEdge
    {
        HalfEdge* m_edges[2] = { nullptr, nullptr }; //!< The two half-edges which together make up the internal edge.
        float m_minAngle = 0.0f; //!< The smallest of the four angles between this edge and adjacent edges.
    };

    //! Sorts internal edges so that the edges with small adjacent angles are considered for removal first.
    struct InternalEdgeCompare
    {
        bool operator()(const InternalEdge& left, const InternalEdge& right) const;
    };

    using InternalEdgePriorityQueue = AZStd::priority_queue<InternalEdge, AZStd::vector<InternalEdge>, InternalEdgeCompare>;

    //! A collection of Face and HalfEdge objects used to represent a 2d mesh.
    class Mesh2D
    {
    public:
        //! Populates this mesh from a set of triangles obtained from poly2tri.
        void CreateFromPoly2Tri(const std::vector<p2t::Triangle*>& triangles);

        //! Populates this mesh from a simple convex polygon.
        void CreateFromSimpleConvexPolygon(const AZStd::vector<AZ::Vector2>& vertices);

        //! Removes an internal edge of the mesh.
        //! The first of the two faces connected to the edge is updated in place to hold the merged face.
        //! The other face is marked as removed, but not deleted from the collection.
        void RemoveInternalEdge(const InternalEdge& internalEdge);

        //! Iteratively merges faces in the mesh if it is possible to do so while maintaining convexity.
        //! Internal edges of the mesh are considered for removal in an order based on 
        //! @return The number of faces which have been removed during the merging process.
        size_t ConvexMerge();

        const AZStd::vector<Face>& GetFaces() const;

        const InternalEdgePriorityQueue& GetInternalEdges() const;

        const AZStd::vector<AZ::Vector3>& GetDebugDrawPoints(float height, const AZ::Vector3& nonUniformScale) const;

        void SetDebugDrawDirty();

        void Clear();

    private:
        //! Together with m_faces, composes the doubly connected edge list representation of the decomposed polygon prism.
        AZStd::vector<HalfEdge> m_halfEdges;

        //! Together with m_halfEdges, composes the doubly connected edge list representation of the decomposed polygon prism.
        AZStd::vector<Face> m_faces;

        //! A queue used to remove internal edges in order based on eliminating small angles from the decomposition first.
        InternalEdgePriorityQueue m_edgeQueue;

        //! Used for caching debug draw vertices.
        mutable AZStd::vector<AZ::Vector3> m_debugDrawPoints;

        //! Used to track when to recalculate the cached debug draw vertices.
        mutable bool m_debugDrawDirty = true;
    };
} // namespace PolygonPrismMeshUtils
