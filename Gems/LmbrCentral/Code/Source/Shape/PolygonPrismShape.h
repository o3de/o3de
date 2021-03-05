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

#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace LmbrCentral
{
    struct ShapeDrawParams;

    /// Buffer to store triangles of top and bottom of Polygon Prism.
    struct PolygonPrismMesh
    {
        AZStd::vector<AZ::Vector3> m_triangles;
        AZStd::vector<AZ::Vector3> m_lines;
    };

    /// Configuration data for PolygonPrismShapeComponent.
    /// Internally represented as a vertex list with a height (extrusion) property.
    /// All vertices must lie on the same plane to form a specialized type of prism, a polygon prism.
    /// A Vector2 is used to enforce this.
    class PolygonPrismShape
        : private ShapeComponentRequestsBus::Handler
        , private PolygonPrismShapeComponentRequestBus::Handler
        , private AZ::FixedVerticesRequestBus<AZ::Vector2>::Handler
        , private AZ::VariableVerticesRequestBus<AZ::Vector2>::Handler
        , private AZ::TransformNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(PolygonPrismShape, AZ::SystemAllocator, 0)
        AZ_RTTI(PolygonPrismShape, "{BDB453DE-8A51-42D0-9237-13A9193BE724}")

        PolygonPrismShape();
        virtual ~PolygonPrismShape() = default;

        PolygonPrismShape(const PolygonPrismShape& other);
        PolygonPrismShape& operator=(const PolygonPrismShape& other);

        static void Reflect(AZ::ReflectContext* context);

        void Activate(AZ::EntityId entityId);
        void Deactivate();
        void InvalidateCache(InvalidateShapeCacheReason reason);

        // ShapeComponent::Handler
        AZ::Crc32 GetShapeType() override { return AZ_CRC("PolygonPrism", 0xd6b50036); }
        AZ::Aabb GetEncompassingAabb() override;
        void GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) override;
        bool IsPointInside(const AZ::Vector3& point) override;
        float DistanceSquaredFromPoint(const AZ::Vector3& point) override;
        bool IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) override;

        // PolygonShapeShapeComponentRequestBus::Handler
        AZ::PolygonPrismPtr GetPolygonPrism() override;
        void SetHeight(float height) override;
        bool GetVertex(size_t index, AZ::Vector2& vertex) const override;
        void AddVertex(const AZ::Vector2& vertex) override;
        bool UpdateVertex(size_t index, const AZ::Vector2& vertex) override;
        bool InsertVertex(size_t index, const AZ::Vector2& vertex) override;
        bool RemoveVertex(size_t index) override;
        void SetVertices(const AZStd::vector<AZ::Vector2>& vertices) override;
        void ClearVertices() override;
        size_t Size() const override;
        bool Empty() const override;

        // AZ::TransformNotificationBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        void ShapeChanged();

        AZ::PolygonPrismPtr GetPolygonPrism() const { return m_polygonPrism;  }
        const AZ::Transform& GetCurrentTransform() const { return m_currentTransform; }

    private:
        /// Runtime data - cache potentially expensive operations.
        class PolygonPrismIntersectionDataCache
            : public IntersectionTestDataCache<AZ::PolygonPrism>
        {
            void UpdateIntersectionParamsImpl(const AZ::Transform& currentTransform,
                const AZ::PolygonPrism& polygonPrism,
                const AZ::Vector3& currentNonUniformScale) override;

            friend PolygonPrismShape;

            AZ::Aabb m_aabb; ///< Aabb of polygon prism shape.
            AZStd::vector<AZ::Vector3> m_triangles; ///< Triangles comprising the polygon prism shape (for intersection testing).
        };

        AZ::PolygonPrismPtr m_polygonPrism; ///< Reference to the underlying polygon prism data.
        PolygonPrismIntersectionDataCache m_intersectionDataCache; ///< Caches transient intersection data.
        AZ::Transform m_currentTransform; ///< Caches the current transform for the entity on which this component lives.
        AZ::EntityId m_entityId; ///< Id of the entity the box shape is attached to.
    };

    /// Generate mesh used for rendering top and bottom of PolygonPrism shape.
    void GeneratePolygonPrismMesh(
        const AZStd::vector<AZ::Vector2>& vertices, float height,
        PolygonPrismMesh& polygonPrismMeshOut);

    void DrawPolygonPrismShape(
        const ShapeDrawParams& shapeDrawParams, const PolygonPrismMesh& polygonPrismMesh,
        AzFramework::DebugDisplayRequests& debugDisplay);

    /// Small set of util functions for PolygonPrism.
    namespace PolygonPrismUtil
    {
        /// Routine to calculate Aabb for orientated polygon prism shape
        AZ::Aabb CalculateAabb(const AZ::PolygonPrism& polygonPrism, const AZ::Transform& transform);

        /// Return if a point in world space is contained within a polygon prism shape
        bool IsPointInside(const AZ::PolygonPrism& polygonPrism, const AZ::Vector3& point, const AZ::Transform& transform);

        /// Return distance squared from point in world space from polygon prism shape
        float DistanceSquaredFromPoint(const AZ::PolygonPrism& polygonPrism, const AZ::Vector3& point, const AZ::Transform& transform);

        /// Return if a ray is intersecting the polygon prism.
        bool IntersectRay(
            AZStd::vector<AZ::Vector3> triangles, const AZ::Transform& worldFromLocal,
            const AZ::Vector3& src, const AZ::Vector3& dir, float& distance);
    }
} // namespace LmbrCentral
