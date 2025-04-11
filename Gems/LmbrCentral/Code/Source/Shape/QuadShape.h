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
#include <LmbrCentral/Shape/QuadShapeComponentBus.h>

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace LmbrCentral
{
    struct ShapeDrawParams;

    //! Provide QuadShape functionality.
    class QuadShape
        : public ShapeComponentRequestsBus::Handler
        , public QuadShapeComponentRequestBus::Handler
        , public AZ::TransformNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(QuadShape, AZ::SystemAllocator);
        AZ_RTTI(LmbrCentral::QuadShape, "{4DCA67DA-5CBB-4E6C-8DA2-2B8CB177A301}");

        QuadShape();

        static void Reflect(AZ::ReflectContext* context);

        void Activate(AZ::EntityId entityId);
        void Deactivate();
        void InvalidateCache(InvalidateShapeCacheReason reason);

        //! ShapeComponentRequestsBus overrides...
        AZ::Crc32 GetShapeType() const override { return AZ_CRC_CE("QuadShape"); }
        AZ::Aabb GetEncompassingAabb() const override;
        void GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) const override;
        bool IsPointInside(const AZ::Vector3& point) const  override;
        float DistanceSquaredFromPoint(const AZ::Vector3& point) const override;
        bool IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) const override;

        //! QuadShapeComponentRequestBus overrides...
        const QuadShapeConfig& GetQuadConfiguration() const override;
        void SetQuadWidth(float width) override;
        float GetQuadWidth() const override;
        void SetQuadHeight(float height) override;
        float GetQuadHeight() const override;
        const AZ::Quaternion& GetQuadOrientation() const override;

        //! AZ::TransformNotificationBus overrides...
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        void OnNonUniformScaleChanged(const AZ::Vector3& scale);
        const AZ::Vector3& GetCurrentNonUniformScale() const { return m_currentNonUniformScale; }

        void SetQuadConfiguration(const QuadShapeConfig& quadShapeConfig);
        const AZ::Transform& GetCurrentTransform() const;

        AZStd::array<AZ::Vector3, 4> GetLocalSpaceCorners();

    protected:

        friend class EditorQuadShapeComponent;
        ShapeComponentConfig& ModifyShapeComponent();

    private:
        //! Runtime data - cache potentially expensive operations.
        class QuadIntersectionDataCache
            : public IntersectionTestDataCache<QuadShapeConfig>
        {
            void UpdateIntersectionParamsImpl(
                const AZ::Transform& currentTransform, const QuadShapeConfig& configuration,
                const AZ::Vector3& currentNonUniformScale = AZ::Vector3::CreateOne()) override;

            friend class QuadShape;

            AZ::Vector3 m_position; //! Position of the center of the quad.
            AZ::Quaternion m_quaternion; //! quaternion of the quad.
            float m_scaledWidth = 1.0f; //! Width of the quad (including entity scale and non-uniform scale).
            float m_scaledHeight = 1.0f; //! Height of the quad (including entity scale and non-uniform scale).
        };

        QuadShapeConfig m_quadShapeConfig; //! Underlying quad configuration.
        mutable QuadIntersectionDataCache m_intersectionDataCache; //! Caches transient intersection data.
        AZ::Transform m_currentTransform; //! Caches the current world transform.
        AZ::EntityId m_entityId; //! The Id of the entity the shape is attached to.
        AZ::NonUniformScaleChangedEvent::Handler m_nonUniformScaleChangedHandler; ///< Responds to changes in non-uniform scale.
        AZ::Vector3 m_currentNonUniformScale = AZ::Vector3::CreateOne(); ///< Caches the current non-uniform scale.
        mutable AZStd::shared_mutex m_mutex; ///< Mutex to allow multiple readers but single writer for efficient thread safety
    };

    void DrawQuadShape(
        const ShapeDrawParams& shapeDrawParams, const QuadShapeConfig& quadShapeConfig,
        AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Vector3& nonUniformScale = AZ::Vector3::CreateOne());

} // namespace LmbrCentral
