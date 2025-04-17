/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/std/parallel/shared_mutex.h>
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
        AZ_CLASS_ALLOCATOR(PolygonPrismShape, AZ::SystemAllocator)
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
        AZ::Crc32 GetShapeType() const override { return AZ_CRC_CE("PolygonPrism"); }
        AZ::Aabb GetEncompassingAabb() const override;
        void GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) const override;
        bool IsPointInside(const AZ::Vector3& point) const override;
        float DistanceSquaredFromPoint(const AZ::Vector3& point) const override;
        bool IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) const override;

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

        void OnNonUniformScaleChanged(const AZ::Vector3& scale);
        const AZ::Vector3& GetCurrentNonUniformScale() const { return m_currentNonUniformScale; }

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
                const AZ::Vector3& currentNonUniformScale = AZ::Vector3::CreateOne()) override;

            friend PolygonPrismShape;

            AZ::Aabb m_aabb; ///< Aabb of polygon prism shape.
            AZStd::vector<AZ::Vector3> m_triangles; ///< Triangles comprising the polygon prism shape (for intersection testing).
        };

        // PolygonPrism has a complicated set of access patterns, so it needs specialized mutex logic to be able to use a shared_mutex
        // with this shape type. The shared_mutex is desirable because it allows the shape to be queried from multiple threads at once,
        // while still protecting reads from running concurrently with writes.
        // The complication is that the PolygonPrismShape has an underlying PolygonPrism, which is the actual data container.
        // PolygonPrism itself has optional callbacks that trigger when data is modified. The EditorPolygonPrismShapeComponent uses those
        // callbacks to query the PolygonPrismShape to refresh some data.
        // This means that when modifying data, the PolygonPrismShape can lock a unique_lock, but while it's holding that lock, callbacks
        // can trigger that try to read data on the same thread, which will try to lock a shared_lock.
        // The extra logic the PolygonPrism*LockGuard classes perform is that they track the thread that acquires a unique lock,
        // and they don't try to acquire or release a shared lock if one is requested on the same thread as the unique lock.

        class PolygonPrismSharedLockGuard
        {
        public:
            PolygonPrismSharedLockGuard(AZStd::shared_mutex& mutex, AZStd::thread::id& uniqueLockThreadId)
                : m_mutex(mutex)
            {
                if (uniqueLockThreadId != AZStd::this_thread::get_id())
                {
                    m_mutex.lock_shared();
                    m_unlockOnDestroy = true;
                }
            }

            ~PolygonPrismSharedLockGuard()
            {
                if (m_unlockOnDestroy)
                {
                    m_mutex.unlock_shared();
                }
            }

            // The handling of the intersection data cache within this shape is especially complex. It gets passed a pointer
            // to a shared mutex that will get promoted to a unique lock only if the the cache is actually getting updated.
            // However, we're managing our shared lock in a way where it might already be a unique lock, so we do the following:
            // - If our shared mutex has a shared lock, we'll pass it down to the intersection data cache as-is.
            // - If our shared mutex already has a unique lock, pass down a null pointer to the intersection data cache.
            AZStd::shared_mutex* GetMutexForIntersectionDataCache()
            {
                // If unlockOnDestroy is set, it's because we have a shared lock, so we'll pass our shared mutex
                // to the intersection data cache. Otherwise, we already have a unique, so pass down a nullptr to prevent
                // the intersection data cache from trying to manage it.
                return (m_unlockOnDestroy) ? &m_mutex : nullptr;
            }

        private:
            PolygonPrismSharedLockGuard(const PolygonPrismSharedLockGuard&) = delete;
            PolygonPrismSharedLockGuard& operator=(const PolygonPrismSharedLockGuard&) = delete;
            AZStd::shared_mutex& m_mutex;
            bool m_unlockOnDestroy = false;
        };

        class PolygonPrismUniqueLockGuard
        {
        public:
            PolygonPrismUniqueLockGuard(AZStd::shared_mutex& mutex, AZStd::thread::id& uniqueLockThreadId)
                : m_mutex(mutex)
                , m_uniqueLockThreadId(uniqueLockThreadId)
            {
                m_mutex.lock();
                m_uniqueLockThreadId = AZStd::this_thread::get_id();
            }

            ~PolygonPrismUniqueLockGuard()
            {
                m_uniqueLockThreadId = AZStd::thread::id();
                m_mutex.unlock();
            }

        private:
            PolygonPrismUniqueLockGuard(const PolygonPrismUniqueLockGuard&) = delete;
            PolygonPrismUniqueLockGuard& operator=(const PolygonPrismUniqueLockGuard&) = delete;
            AZStd::shared_mutex& m_mutex;
            AZStd::thread::id& m_uniqueLockThreadId;
        };

        AZ::PolygonPrismPtr m_polygonPrism; ///< Reference to the underlying polygon prism data.
        mutable PolygonPrismIntersectionDataCache m_intersectionDataCache; ///< Caches transient intersection data.
        AZ::Transform m_currentTransform = AZ::Transform::CreateIdentity(); ///< Caches the current transform for this shape.
        AZ::EntityId m_entityId; ///< Id of the entity the box shape is attached to.
        AZ::NonUniformScaleChangedEvent::Handler m_nonUniformScaleChangedHandler; ///< Responds to changes in non-uniform scale.
        AZ::Vector3 m_currentNonUniformScale = AZ::Vector3::CreateOne(); ///< Caches the current non-uniform scale.
        mutable AZStd::shared_mutex m_mutex; ///< Mutex to allow multiple readers but single writer for efficient thread safety
        mutable AZStd::thread::id m_uniqueLockThreadId; ///< Thread that currently owns the unique lock on the mutex, if any.
    };

    /// Generate mesh used for rendering top and bottom of PolygonPrism shape.
    void GeneratePolygonPrismMesh(
        const AZStd::vector<AZ::Vector2>& vertices, float height, const AZ::Vector3& nonUniformScale,
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
