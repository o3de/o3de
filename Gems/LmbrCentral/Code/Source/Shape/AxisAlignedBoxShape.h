/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/AxisAlignedBoxShapeComponentBus.h>

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace LmbrCentral
{
    struct ShapeDrawParams;

    class AxisAlignedBoxShape
        : public ShapeComponentRequestsBus::Handler
        , public AxisAlignedBoxShapeComponentRequestsBus::Handler
        , public AZ::TransformNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(AxisAlignedBoxShape, AZ::SystemAllocator, 0)
        AZ_RTTI(AxisAlignedBoxShape, "{CFDC96C5-287A-4033-8D7D-BA9331C13F25}")

        AxisAlignedBoxShape();

        static void Reflect(AZ::ReflectContext* context);

        void Activate(AZ::EntityId entityId);
        void Deactivate();
        void InvalidateCache(InvalidateShapeCacheReason reason);

        // ShapeComponentRequestsBus::Handler
        AZ::Crc32 GetShapeType() override { return AZ_CRC_CE("AxisAlignedBox"); }
        AZ::Aabb GetEncompassingAabb() override;
        void GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) override;
        bool IsPointInside(const AZ::Vector3& point) override;
        float DistanceSquaredFromPoint(const AZ::Vector3& point) override;
        AZ::Vector3 GenerateRandomPointInside(AZ::RandomDistributionType randomDistribution) override;
        bool IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) override;

        // BoxShapeComponentRequestBus::Handler
        AxisAlignedBoxShapeConfig GetBoxConfiguration() override { return m_boxShapeConfig; }
        AZ::Vector3 GetBoxDimensions() override { return m_boxShapeConfig.m_dimensions; }
        void SetBoxDimensions(const AZ::Vector3& dimensions) override;

        // AZ::TransformNotificationBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        void OnNonUniformScaleChanged(const AZ::Vector3& scale);
        const AZ::Vector3& GetCurrentNonUniformScale() const { return m_currentNonUniformScale; }

        const AxisAlignedBoxShapeConfig& GetBoxConfiguration() const { return m_boxShapeConfig; }
        void SetBoxConfiguration(const AxisAlignedBoxShapeConfig& boxShapeConfig) { m_boxShapeConfig = boxShapeConfig; }
        const AZ::Transform& GetCurrentTransform() const { return m_currentTransform; }

        void SetDrawColor(const AZ::Color& color) { m_boxShapeConfig.SetDrawColor(color); }

    protected:

        friend class EditorAxisAlignedBoxShapeComponent;        
        AxisAlignedBoxShapeConfig& ModifyConfiguration() { return m_boxShapeConfig; }

    private:
        /// Runtime data - cache potentially expensive operations.
        class BoxIntersectionDataCache
            : public IntersectionTestDataCache<AxisAlignedBoxShapeConfig>
        {
            void UpdateIntersectionParamsImpl(
                const AZ::Transform& currentTransform, const AxisAlignedBoxShapeConfig& configuration,
                const AZ::Vector3& currentNonUniformScale = AZ::Vector3::CreateOne()) override;

            friend AxisAlignedBoxShape;

            AZ::Aabb m_aabb; ///< Aabb representing this Box (including the effects of scale).
            AZ::Vector3 m_currentPosition; ///< Position of the Box.
            AZ::Vector3 m_scaledDimensions; ///< Dimensions of Box (including entity scale and non-uniform scale).
        };

        AxisAlignedBoxShapeConfig m_boxShapeConfig; ///< Underlying box configuration.
        BoxIntersectionDataCache m_intersectionDataCache; ///< Caches transient intersection data.
        AZ::Transform m_currentTransform; ///< Caches the current transform for the entity on which this component lives.
        AZ::EntityId m_entityId; ///< Id of the entity the box shape is attached to.
        AZ::NonUniformScaleChangedEvent::Handler m_nonUniformScaleChangedHandler; ///< Responds to changes in non-uniform scale.
        AZ::Vector3 m_currentNonUniformScale = AZ::Vector3::CreateOne(); ///< Caches the current non-uniform scale.
    };

    void DrawBoxShape(
        const ShapeDrawParams& shapeDrawParams, const AxisAlignedBoxShapeConfig& boxShapeConfig,
        AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Vector3& nonUniformScale = AZ::Vector3::CreateOne());

} // namespace LmbrCentral
