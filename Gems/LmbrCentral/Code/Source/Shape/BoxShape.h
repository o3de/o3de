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
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace LmbrCentral
{
    struct ShapeDrawParams;

    class BoxShape
        : public ShapeComponentRequestsBus::Handler
        , public BoxShapeComponentRequestsBus::Handler
        , public AZ::TransformNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(BoxShape, AZ::SystemAllocator)
        AZ_RTTI(BoxShape, "{36D1BA94-13CF-433F-B1FE-28BEBBFE20AA}")

        BoxShape();

        static void Reflect(AZ::ReflectContext* context);

        virtual void Activate(AZ::EntityId entityId);
        void Deactivate();
        void InvalidateCache(InvalidateShapeCacheReason reason);

        // ShapeComponentRequestsBus::Handler
        AZ::Crc32 GetShapeType() const override { return AZ_CRC_CE("Box"); }
        AZ::Aabb GetEncompassingAabb() const override;
        void GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) const override;
        bool IsPointInside(const AZ::Vector3& point) const override;
        float DistanceSquaredFromPoint(const AZ::Vector3& point) const override;
        AZ::Vector3 GenerateRandomPointInside(AZ::RandomDistributionType randomDistribution) const override;
        bool IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) const override;
        AZ::Vector3 GetTranslationOffset() const override;
        void SetTranslationOffset(const AZ::Vector3& translationOffset) override;

        // BoxShapeComponentRequestBus::Handler
       const BoxShapeConfig& GetBoxConfiguration() const override { return m_boxShapeConfig; }
        AZ::Vector3 GetBoxDimensions() const override { return m_boxShapeConfig.m_dimensions; }
        void SetBoxDimensions(const AZ::Vector3& dimensions) override;
        bool IsTypeAxisAligned() const override;

        // AZ::TransformNotificationBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        void OnNonUniformScaleChanged(const AZ::Vector3& scale);
        const AZ::Vector3& GetCurrentNonUniformScale() const { return m_currentNonUniformScale; }

        void SetBoxConfiguration(const BoxShapeConfig& boxShapeConfig) { m_boxShapeConfig = boxShapeConfig; }
        const AZ::Transform& GetCurrentTransform() const { return m_currentTransform; }

        void SetDrawColor(const AZ::Color& color) { m_boxShapeConfig.SetDrawColor(color); }

        BoxShapeConfig& ModifyConfiguration() { return m_boxShapeConfig; }

    protected:
        /// Runtime data - cache potentially expensive operations.
        class BoxIntersectionDataCache
            : public IntersectionTestDataCache<BoxShapeConfig>
        {
            void UpdateIntersectionParamsImpl(
                const AZ::Transform& currentTransform, const BoxShapeConfig& configuration,
                const AZ::Vector3& currentNonUniformScale = AZ::Vector3::CreateOne()) override;

            friend BoxShape;
            friend class AxisAlignedBoxShape;

            AZ::Aabb m_aabb; ///< Aabb representing this Box (including the effects of scale).
            AZ::Obb m_obb; ///< Obb representing this Box (including the effects of scale).
            AZ::Vector3 m_currentPosition; ///< Position of the Box.
            AZ::Vector3 m_scaledDimensions; ///< Dimensions of Box (including entity scale and non-uniform scale).
            bool m_axisAligned = true; ///< Indicates whether the box is axis or object aligned.
        };

        mutable BoxIntersectionDataCache m_intersectionDataCache; ///< Caches transient intersection data.
        AZ::Transform m_currentTransform; ///< Caches the current transform for the entity on which this component lives.
        AZ::EntityId m_entityId; ///< Id of the entity the box shape is attached to.
        AZ::NonUniformScaleChangedEvent::Handler m_nonUniformScaleChangedHandler; ///< Responds to changes in non-uniform scale.
        AZ::Vector3 m_currentNonUniformScale = AZ::Vector3::CreateOne(); ///< Caches the current non-uniform scale.
        BoxShapeConfig m_boxShapeConfig; ///< Underlying box configuration.
        mutable AZStd::shared_mutex m_mutex; ///< Mutex to allow multiple readers but single writer for efficient thread safety
    };

    void DrawBoxShape(
        const ShapeDrawParams& shapeDrawParams, const BoxShapeConfig& boxShapeConfig,
        AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Vector3& nonUniformScale = AZ::Vector3::CreateOne());

} // namespace LmbrCentral
