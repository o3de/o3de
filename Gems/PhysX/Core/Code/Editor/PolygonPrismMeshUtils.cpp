/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/PolygonPrismMeshUtils.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Math/Vector3.h>

namespace PolygonPrismMeshUtils
{
    AZ::Vector3 CalculateAngles(p2t::Triangle& triangle)
    {
        // Calculate the internal angles of the triangle from the edge lengths using the law of cosines
        const float e0 = static_cast<float>((*triangle.GetPoint(1) - *triangle.GetPoint(0)).Length());
        const float e1 = static_cast<float>((*triangle.GetPoint(2) - *triangle.GetPoint(1)).Length());
        const float e2 = static_cast<float>((*triangle.GetPoint(0) - *triangle.GetPoint(2)).Length());

        const float angle0 = acosf(AZ::GetClamp((e0 * e0 + e2 * e2 - e1 * e1) / (2.0f * e0 * e2), -1.0f, 1.0f));
        const float angle1 = acosf(AZ::GetClamp((e0 * e0 + e1 * e1 - e2 * e2) / (2.0f * e0 * e1), -1.0f, 1.0f));

        return AZ::Vector3(angle0, angle1, AZ::Constants::Pi - angle0 - angle1);
    }

    bool InternalEdgeCompare::operator()(const InternalEdge& left, const InternalEdge& right) const
    {
        return left.m_minAngle > right.m_minAngle;
    }

    void Mesh2D::CreateFromPoly2Tri(const std::vector<p2t::Triangle*>& triangles)
    {
        const int numTriangles = static_cast<int>(triangles.size());
        m_faces.resize(numTriangles);
        m_halfEdges.resize(3 * numTriangles);

        AZStd::unordered_map<p2t::Triangle*, int> triangleIndexMap;

        for (int faceIndex = 0; faceIndex < numTriangles; faceIndex++)
        {
            p2t::Triangle* triangle = triangles[faceIndex];
            triangleIndexMap.emplace(AZStd::make_pair(triangle, faceIndex));

            // The index of the first half-edge in this face
            const int firstHalfEdgeIndex = 3 * faceIndex;

            // Populate the face data
            m_faces[faceIndex].m_edge = &m_halfEdges[firstHalfEdgeIndex];
            m_faces[faceIndex].m_numEdges = 3;

            // Populate the half-edge data, apart from the twin pointers which will require a second pass
            const AZ::Vector3 angles = CalculateAngles(*triangle);
            for (int edgeIndex = 0; edgeIndex < 3; edgeIndex++)
            {
                const int nextIndex = (edgeIndex + 1) % 3;
                const int prevIndex = (edgeIndex + 2) % 3;

                HalfEdge* halfEdge = &m_halfEdges[firstHalfEdgeIndex + edgeIndex];
                halfEdge->m_face = &m_faces[faceIndex];
                const p2t::Point* point = triangle->GetPoint(edgeIndex);
                halfEdge->m_origin = AZ::Vector2(static_cast<float>(point->x), static_cast<float>(point->y));
                halfEdge->m_prev = &m_halfEdges[firstHalfEdgeIndex + prevIndex];
                halfEdge->m_next = &m_halfEdges[firstHalfEdgeIndex + nextIndex];
                halfEdge->m_prevAngle = angles.GetElement(edgeIndex);
                halfEdge->m_nextAngle = angles.GetElement(nextIndex);
            }
        }

        // Figure out twin half-edges, and populate the queue of internal edges to consider for removal
        for (int faceIndex = 0; faceIndex < numTriangles; faceIndex++)
        {
            p2t::Triangle* triangle = triangles[faceIndex];
            for (int edgeIndex = 0; edgeIndex < 3; edgeIndex++)
            {
                HalfEdge* halfEdge = &m_halfEdges[3 * faceIndex + edgeIndex];
                if (halfEdge->m_visited)
                {
                    // We have already visited this half-edge when considering its twin, so there is nothing to do
                    continue;
                }

                p2t::Triangle* twinFace = triangle->NeighborCCW(*triangle->GetPoint(edgeIndex));
                if (twinFace == nullptr)
                {
                    // This half-edge doesn't have a twin, so it is an external edge and there is nothing to do
                    continue;
                }

                auto triangleIndexPair = triangleIndexMap.find(twinFace);
                if (triangleIndexPair == triangleIndexMap.end())
                {
                    // Poly2tri can have triangles outside of the polygon, so it is possible for them not to be
                    // found in the face map, but in this case we should do nothing
                    continue;
                }

                const int twinFaceIndex = triangleIndexPair->second;
                if (twinFaceIndex >= numTriangles)
                {
                    AZ_Error("PolygonPrismMeshUtils", false, "Invalid triangle index.");
                    continue;
                }

                const int nextIndex = (edgeIndex + 1) % 3;
                const int twinEdgeIndex = twinFace->Index(triangle->GetPoint(nextIndex));

                HalfEdge* twinHalfEdge = &m_halfEdges[3 * twinFaceIndex + twinEdgeIndex];
                halfEdge->m_twin = twinHalfEdge;
                halfEdge->m_visited = true;
                twinHalfEdge->m_twin = halfEdge;
                twinHalfEdge->m_visited = true;

                InternalEdge internalEdge;
                internalEdge.m_edges[0] = halfEdge;
                internalEdge.m_edges[1] = twinHalfEdge;
                internalEdge.m_minAngle = AZStd::GetMin(
                    AZStd::GetMin(halfEdge->m_prevAngle, halfEdge->m_nextAngle),
                    AZStd::GetMin(twinHalfEdge->m_prevAngle, twinHalfEdge->m_nextAngle));
                m_edgeQueue.push(internalEdge);
            }
        }
    }

    void Mesh2D::CreateFromSimpleConvexPolygon(const AZStd::vector<AZ::Vector2>& vertices)
    {
        const int numVertices = static_cast<int>(vertices.size());
        m_faces.resize(1);
        m_halfEdges.resize(numVertices);

        m_faces[0].m_edge = &m_halfEdges[0];
        m_faces[0].m_numEdges = numVertices;

        for (int edgeIndex = 0; edgeIndex < numVertices; edgeIndex++)
        {
            const int nextIndex = (edgeIndex + 1) % numVertices;
            const int prevIndex = (edgeIndex + numVertices - 1) % numVertices;

            m_halfEdges[edgeIndex].m_face = &m_faces[0];
            m_halfEdges[edgeIndex].m_origin = vertices[edgeIndex];
            m_halfEdges[edgeIndex].m_prev = &m_halfEdges[prevIndex];
            m_halfEdges[edgeIndex].m_next = &m_halfEdges[nextIndex];
        }
    }

    void Mesh2D::RemoveInternalEdge(const InternalEdge& internalEdge)
    {
        HalfEdge* halfEdge0 = internalEdge.m_edges[0];
        HalfEdge* halfEdge1 = internalEdge.m_edges[1];
        Face* faceToKeep = halfEdge0->m_face;
        Face* faceToRemove = halfEdge1->m_face;
        const float newAngle0 = halfEdge0->m_prevAngle + halfEdge1->m_nextAngle;
        const float newAngle1 = halfEdge0->m_nextAngle + halfEdge1->m_prevAngle;
        const int newNumEdges = halfEdge0->m_face->m_numEdges + halfEdge1->m_face->m_numEdges - 2;

        faceToKeep->m_numEdges = newNumEdges;
        faceToRemove->m_removed = true;

        // Make sure the face doesn't point to the half-edge that will be removed.
        if (faceToKeep->m_edge == internalEdge.m_edges[0])
        {
            faceToKeep->m_edge = internalEdge.m_edges[0]->m_next;
        }

        // Make all the half-edges in the face that will be removed point to the kept face instead.
        HalfEdge* currentEdge = internalEdge.m_edges[1];
        for (int edgeIndex = 0; edgeIndex < faceToRemove->m_numEdges; edgeIndex++)
        {
            currentEdge->m_face = faceToKeep;
            currentEdge = currentEdge->m_next;
        }

        // Update the previous and next half-edge pointers and angles for the 4 half-edges adjacent
        // to the edge that will be removed.
        halfEdge0->m_prev->m_dirty = true;
        halfEdge0->m_next->m_dirty = true;
        halfEdge1->m_prev->m_dirty = true;
        halfEdge1->m_next->m_dirty = true;

        halfEdge0->m_prev->m_next = halfEdge1->m_next;
        halfEdge0->m_next->m_prev = halfEdge1->m_prev;
        halfEdge1->m_prev->m_next = halfEdge0->m_next;
        halfEdge1->m_next->m_prev = halfEdge0->m_prev;

        halfEdge0->m_prev->m_nextAngle = newAngle0;
        halfEdge0->m_next->m_prevAngle = newAngle1;
        halfEdge1->m_prev->m_nextAngle = newAngle1;
        halfEdge1->m_next->m_prevAngle = newAngle0;
    }

    size_t Mesh2D::ConvexMerge()
    {
        size_t numFacesRemoved = 0;

        while (!m_edgeQueue.empty())
        {
            const InternalEdge& internalEdge = m_edgeQueue.top();

            if (internalEdge.m_edges[0] != nullptr && internalEdge.m_edges[1] != nullptr)
            {
                // If either of the half-edges are marked dirty due to previously removing adjacent edges,
                // update the edge and place back into the queue.
                if (internalEdge.m_edges[0]->m_dirty || internalEdge.m_edges[1]->m_dirty)
                {
                    InternalEdge updatedInternalEdge(internalEdge);
                    updatedInternalEdge.m_minAngle = AZStd::GetMin(
                        AZStd::GetMin(internalEdge.m_edges[0]->m_prevAngle, internalEdge.m_edges[0]->m_nextAngle),
                        AZStd::GetMin(internalEdge.m_edges[1]->m_prevAngle, internalEdge.m_edges[1]->m_nextAngle));
                    internalEdge.m_edges[0]->m_dirty = false;
                    internalEdge.m_edges[1]->m_dirty = false;
                    m_edgeQueue.pop();
                    m_edgeQueue.push(updatedInternalEdge);
                }
                else
                {
                    // There are two conditions that need to be satisfied in order to allow removing this edge.
                    // First, the merged face should remain convex, i.e. the two new internal angles which would be
                    // created must both be less than 180 degrees.
                    // Secondly, the merged polygon must meet the PhysX vertex limit for convex meshes.
                    HalfEdge* halfEdge0 = internalEdge.m_edges[0];
                    HalfEdge* halfEdge1 = internalEdge.m_edges[1];
                    const float newAngle0 = halfEdge0->m_prevAngle + halfEdge1->m_nextAngle;
                    const float newAngle1 = halfEdge0->m_nextAngle + halfEdge1->m_prevAngle;
                    const int newNumEdges = halfEdge0->m_face->m_numEdges + halfEdge1->m_face->m_numEdges - 2;

                    // Allow angles very slightly larger than 180 degrees to avoid unnecessary splitting due to
                    // precision issues.
                    const float epsilon = 1e-5f;
                    const float maxAngle = AZ::Constants::Pi + epsilon;

                    if (newAngle0 < maxAngle &&
                        newAngle1 < maxAngle &&
                        newNumEdges <= MaxPolygonPrismEdges)
                    {
                        RemoveInternalEdge(internalEdge);
                        numFacesRemoved++;
                    }

                    m_edgeQueue.pop();
                }
            }

            else
            {
                AZ_Error("PhysX Shape Collider Component", false, "Invalid half-edge.");
                m_edgeQueue.pop();
            }
        }

        return numFacesRemoved;
    }

    const AZStd::vector<Face>& Mesh2D::GetFaces() const
    {
        return m_faces;
    }

    const AZStd::priority_queue<InternalEdge, AZStd::vector<InternalEdge>, InternalEdgeCompare>& Mesh2D::GetInternalEdges() const
    {
        return m_edgeQueue;
    }

    const AZStd::vector<AZ::Vector3>& Mesh2D::GetDebugDrawPoints(float height, const AZ::Vector3& nonUniformScale) const
    {
        if (m_debugDrawDirty)
        {
            m_debugDrawPoints.clear();

            for (const auto& face : m_faces)
            {
                if (!face.m_removed)
                {
                    int numEdges = face.m_numEdges;
                    PolygonPrismMeshUtils::HalfEdge* currentEdge = face.m_edge;
                    for (int edgeIndex = 0; edgeIndex < numEdges; edgeIndex++)
                    {
                        PolygonPrismMeshUtils::HalfEdge* nextEdge = currentEdge->m_next;
                        const auto v1 = nonUniformScale * AZ::Vector3(currentEdge->m_origin.GetX(), currentEdge->m_origin.GetY(), 0.0f);
                        const auto v2 = nonUniformScale * AZ::Vector3(currentEdge->m_origin.GetX(), currentEdge->m_origin.GetY(), height);
                        const auto v3 = nonUniformScale * AZ::Vector3(nextEdge->m_origin.GetX(), nextEdge->m_origin.GetY(), 0.0f);
                        const auto v4 = nonUniformScale * AZ::Vector3(nextEdge->m_origin.GetX(), nextEdge->m_origin.GetY(), height);

                        m_debugDrawPoints.insert(m_debugDrawPoints.end(), { v1, v2, v1, v3, v2, v4 });

                        currentEdge = currentEdge->m_next;
                    }
                }
            }

            m_debugDrawDirty = false;
        }

        return m_debugDrawPoints;
    }

    void Mesh2D::SetDebugDrawDirty()
    {
        m_debugDrawDirty = true;
    }

    void Mesh2D::Clear()
    {
        m_halfEdges.clear();
        m_faces.clear();
        m_edgeQueue = AZStd::priority_queue<InternalEdge, AZStd::vector<InternalEdge>, InternalEdgeCompare>();
        m_debugDrawDirty = true;
    }
} // namespace PolygonPrismMeshUtils
