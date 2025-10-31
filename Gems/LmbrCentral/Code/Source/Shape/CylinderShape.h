/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/CylinderShapeComponentBus.h>

namespace AZ
{
    enum class RandomDistributionType : AZ::u32;
}

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace LmbrCentral
{
    struct ShapeDrawParams;

    class CylinderShape
        : public ShapeComponentRequestsBus::Handler
        , public CylinderShapeComponentRequestsBus::Handler
        , public AZ::TransformNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(CylinderShape, AZ::SystemAllocator)
        AZ_RTTI(CylinderShape, "{B45EFEF2-631F-43D3-B538-A3FE68350231}")

        static void Reflect(AZ::ReflectContext* context);

        void Activate(AZ::EntityId entityId);
        void Deactivate();
        void InvalidateCache(InvalidateShapeCacheReason reason);

        // ShapeComponentRequestsBus::Handler
        AZ::Crc32 GetShapeType() const override { return AZ_CRC_CE("Cylinder"); }
        bool IsPointInside(const AZ::Vector3& point) const override;
        float DistanceSquaredFromPoint(const AZ::Vector3& point) const override;
        AZ::Aabb GetEncompassingAabb() const override;
        void GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) const override;
        AZ::Vector3 GenerateRandomPointInside(AZ::RandomDistributionType randomDistribution) const override;
        bool IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) const override;

        // CylinderShapeComponentRequestsBus::Handler
        const CylinderShapeConfig& GetCylinderConfiguration() const override { return m_cylinderShapeConfig; }
        void SetHeight(float height) override;
        void SetRadius(float radius) override;
        float GetHeight() const override;
        float GetRadius() const override;

        // AZ::TransformNotificationBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        void SetCylinderConfiguration(const CylinderShapeConfig& cylinderShapeConfig) { m_cylinderShapeConfig = cylinderShapeConfig; }
        const AZ::Transform& GetCurrentTransform() const { return m_currentTransform; }

    protected:

        friend class EditorCylinderShapeComponent;
        CylinderShapeConfig& ModifyConfiguration() { return m_cylinderShapeConfig; }

    private:
        /// Runtime data - cache potentially expensive operations.
        class CylinderIntersectionDataCache
            : public IntersectionTestDataCache<CylinderShapeConfig>
        {
            void UpdateIntersectionParamsImpl(
                const AZ::Transform& currentTransform, const CylinderShapeConfig& configuration,
                const AZ::Vector3& currentNonUniformScale = AZ::Vector3::CreateOne()) override;

            friend class CylinderShape;
            
            AZ::Vector3 m_baseCenterPoint; ///< The center point of the cylinder.
            AZ::Vector3 m_axisVector; ///< A vector along the axis of this cylinder scaled to the height of the cylinder.
            float m_height = 0.0f; ///< Height of the cylinder (including entity scale).
            float m_radius = 0.0f; ///< Radius of the cylinder (including entity scale).
        };

        CylinderShapeConfig m_cylinderShapeConfig; ///< Underlying cylinder configuration.
        mutable CylinderIntersectionDataCache m_intersectionDataCache; ///< Caches transient intersection data.
        AZ::Transform m_currentTransform; ///< Caches the current World transform.
        AZ::EntityId m_entityId; ///< The Id of the entity the shape is attached to.
        mutable AZStd::shared_mutex m_mutex; ///< Mutex to allow multiple readers but single writer for efficient thread safety
    };

    void DrawCylinderShape(
        const ShapeDrawParams& shapeDrawParams, const CylinderShapeConfig& cylinderShapeConfig,
        AzFramework::DebugDisplayRequests& debugDisplay);

} // namespace LmbrCentral
