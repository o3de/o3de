/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>

namespace AZ
{
    class ReflectContext;
    void VertexContainerReflect(ReflectContext* context);

    using IndexFunction = AZStd::function<void(size_t)>;
    using VoidFunction = AZStd::function<void()>;
    using BoolFunction = AZStd::function<void(bool)>;

    /// A wrapper around a AZStd::vector of either Vector2 or Vector3s.
    /// Provides an interface to access and modify the container.
    template <typename Vertex>
    class VertexContainer
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        VertexContainer() = default;
        VertexContainer(
            const IndexFunction& addCallback, const IndexFunction& removeCallback,
            const IndexFunction& updateCallback, const VoidFunction& setCallback,
            const VoidFunction& clearCallback);
        ~VertexContainer() = default;

        /// Add vertex at end of container.
        /// @param vertex new vertex position (local space).
        void AddVertex(const Vertex& vertex);
        /// Update position of vertex in container.
        /// @param index index of current vertex to update.
        /// @param vertex new vertex position (local space).
        /// @return was the index in range.
        bool UpdateVertex(size_t index, const Vertex& vertex);
        /// Insert vertex before index in container.
        /// @param index index of vertex to insert before.
        /// @param vertex new vertex position (local space).
        /// @return was the index in range.
        bool InsertVertex(size_t index, const Vertex& vertex);
        /// Remove vertex at index in container.
        /// @param index index of vertex to remove.
        /// @return was the index in range.
        bool RemoveVertex(size_t index);
        /// Set all vertices.
        /// @param vertices new vertices to set.
        template<typename Vertices>
        void SetVertices(Vertices&& vertices);
        /// Remove all vertices.
        void Clear();

        /// Get vertex at index.
        /// @return was the index in range.
        bool GetVertex(size_t index, Vertex& vertex) const;
        /// Get last vertex.
        /// @return if vector is empty return false.
        bool GetLastVertex(Vertex& vertex) const;
        /// Number of vertices in the container.
        size_t Size() const;
        /// Is the container empty or not.
        bool Empty() const;
        /// Immutable reference to container of vertices.
        const AZStd::vector<Vertex>& GetVertices() const;
        /// Unsafe access to vertices (index must be checked to be in range before use).
        const Vertex& operator[](size_t index) const;

        /// Provide callbacks for this container
        /// Useful if you could not provide callbacks at construction or
        /// you need to re-supply callbacks after deserialization
        void SetCallbacks(
            const IndexFunction& addCallback, const IndexFunction& removeCallback,
            const IndexFunction& updateCallback, const VoidFunction& setCallback,
            const VoidFunction& clearCallback);

        static void Reflect(ReflectContext* context);

    private:
        AZStd::vector<Vertex> m_vertices; ///< Vertices (positions).

        IndexFunction m_addCallback = nullptr; ///< Callback for when a vertex is added.
        IndexFunction m_removeCallback = nullptr; ///< Callback for when a vertex is removed.
        IndexFunction m_updateCallback = nullptr; ///< Callback for when a vertex is updated/modified.
        VoidFunction m_setCallback = nullptr; ///< Callback for when a all vertices are set.
        VoidFunction m_clearCallback = nullptr; ///< Callback for when a all vertices are cleared.

        /// Internal function called when a new vertex is added from the property grid (AZ::Edit::Attributes::AddNotify) - default to Vector3(0,0,0) if empty, else previous last vertex position.
        void AddNotify();
        /// Internal function called when a vertex is removed from the property grid (AZ::Edit::Attributes::RemoveNotify).
        void RemoveNotify(size_t index) const;
        /// Internal function called when a vertex is modified in the property grid (AZ::Edit::Attributes::ChangeNotify).
        void UpdateNotify(size_t index) const;
    };

    AZ_TYPE_INFO_TEMPLATE(VertexContainer, "{39A06CC2-D2C4-4803-A2D1-0E674A61EF4E}", AZ_TYPE_INFO_TYPENAME);

} // namespace AZ

#include <AzCore/Math/Internal/VertexContainer.inl>
