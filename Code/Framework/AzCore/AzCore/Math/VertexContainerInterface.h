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

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/VectorConversions.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    /**
     * Interface provided by a container of vertices of fixed length (example: array or fixed_vector).
     */
    template<typename Vertex>
    class FixedVertices
    {
    public:
        virtual ~FixedVertices() = default;

        /**
         * Get a vertex at a particular index.
         * @param index Index of vertex to access.
         * @param vertex Out parameter of vertex at index.
         * @return Was the vertex at the index provided able to be accessed.
         */
        virtual bool GetVertex(size_t index, Vertex& vertex) const = 0;

        /**
         * Update a vertex at a particular index.
         * @param index Index of vertex to update.
         * @param vertex New vertex position.
         * @return Was the vertex at the index provided able to be updated.
         */
        virtual bool UpdateVertex(size_t index, const Vertex& vertex) = 0;

        /**
         * How many vertices are there.
         */
        virtual size_t Size() const = 0;
    };

    /**
     * Interface provided by a container of vertices of variable length (example: vector or VertexContainer).
     */
    template<typename Vertex>
    class VariableVertices
        : public FixedVertices<Vertex>
    {
    public:
        /**
         * Add a vertex at the end of the container.
         * @param vertex New vertex position.
         */
        virtual void AddVertex(const Vertex& vertex) = 0;

        /**
         * Insert a vertex at a particular index.
         * @param index Index of vertex to insert before.
         * @param vertex New vertex position.
         * @return Was the vertex able to be inserted at the provided index.
         */
        virtual bool InsertVertex(size_t index, const Vertex& vertex) = 0;

        /**
         * Remove a vertex at a particular index.
         * @param index Index of vertex to remove.
         * @return Was the vertex able to be removed.
         */

        virtual bool RemoveVertex(size_t index) = 0;

        /**
         * Is the container empty.
         */
        virtual bool Empty() const = 0;

        /**
         * Set all vertices.
         * @param vertices New vertices to set.
         */
        virtual void SetVertices(const AZStd::vector<Vertex>& vertices) = 0;

        /**
         * Remove all vertices from the container.
         */
        virtual void ClearVertices() = 0;
    };

    /// EBus traits for vertex requests.
    class VertexRequests
        : public AZ::ComponentBus
    {
    public:
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
    };

    /// Type to inherit to implement FixedVertices interface.
    template<typename Vertex>
    using FixedVerticesRequestBus = EBus<FixedVertices<Vertex>, VertexRequests>;

    /// Type to inherit to implement VariableVertices interface.
    template<typename Vertex>
    using VariableVerticesRequestBus = EBus<VariableVertices<Vertex>, VertexRequests>;

    /**
     * Interface for vertex container notifications.
     */
    template<typename Vertex>
    class VertexContainerNotificationInterface
    {
    public:
        /**
         * Called when a new vertex is added.
         */
        virtual void OnVertexAdded(size_t index) = 0;

        /**
         * Called when a vertex is removed.
         */
        virtual void OnVertexRemoved(size_t index) = 0;

        /**
         * Called when a vertex is updated.
         */
        virtual void OnVertexUpdated(size_t index) = 0;

        /**
         * Called when a new set of vertices is set.
         */
        virtual void OnVerticesSet(const AZStd::vector<Vertex>& vertices) = 0;

        /**
         * Called when all vertices are cleared.
         */
        virtual void OnVerticesCleared() = 0;

    protected:
        ~VertexContainerNotificationInterface() = default;
    };

    // template helper to map a local/world space position into a vertex container
    // depending on if it is storing Vector2s or Vector3s.
    template<typename Vertex>
    Vertex AdaptVertexIn(const AZ::Vector3&) { return AZ::Vector3::CreateZero(); }

    template<>
    inline AZ::Vector3 AdaptVertexIn<AZ::Vector3>(const AZ::Vector3& vector) { return vector; }

    template<>
    inline AZ::Vector2 AdaptVertexIn<AZ::Vector2>(const AZ::Vector3& vector) { return Vector3ToVector2(vector); }

    // template helper to map a vertex (vector) from a vertex container to a local/world space position
    // depending on if the vertex container is storing Vector2s or Vector3s.
    template<typename Vertex>
    AZ::Vector3 AdaptVertexOut(const Vertex&) { return AZ::Vector3::CreateZero(); }

    template<>
    inline AZ::Vector3 AdaptVertexOut<AZ::Vector3>(const AZ::Vector3& vector) { return vector; }

    template<>
    inline AZ::Vector3 AdaptVertexOut<AZ::Vector2>(const AZ::Vector2& vector) { return Vector2ToVector3(vector); }

} // namespace AZ