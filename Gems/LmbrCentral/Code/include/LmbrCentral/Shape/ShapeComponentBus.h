/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBusSharedDispatchTraits.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzFramework/Viewport/ViewportColors.h>

namespace AZ
{
    class BehaviorContext;
    enum class RandomDistributionType : AZ::u32;
}
namespace LmbrCentral
{
    /// Reason shape cache should be recalculated.
    enum class InvalidateShapeCacheReason
    {
        TransformChange, ///< The cache is invalid because the transform of the entity changed.
        ShapeChange ///< The cache is invalid because the shape configuration/properties changed.
    };

    /// Feature flag for work in progress on shape component translation offsets (see https://github.com/o3de/sig-simulation/issues/26).
    constexpr AZStd::string_view ShapeComponentTranslationOffsetEnabled = "/Amazon/Preferences/EnableShapeComponentTranslationOffset";

    /// Helper function for checking whether feature flag for in progress shape component translation offsets is enabled.
    /// See https://github.com/o3de/sig-simulation/issues/26 for more details.
    inline bool IsShapeComponentTranslationEnabled()
    {
        bool isShapeComponentTranslationEnabled = false;

        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(isShapeComponentTranslationEnabled, ShapeComponentTranslationOffsetEnabled);
        }

        return isShapeComponentTranslationEnabled;
    }

    /// Wrapper for cache of data used for intersection tests
    template <typename ShapeConfiguration>
    class IntersectionTestDataCache
    {
    public:
        virtual ~IntersectionTestDataCache() = default;

        /// @brief Updates the intersection data cache to reflect the current state of the shape.
        /// @param currentTransform The current Transform of the entity.
        /// @param configuration The specific configuration of a shape.
        /// @param sharedMutex Optional pointer to a shared_mutex for the shape that is expected to be lock_shared on both entry and exit.
        ///        It will be promoted to a unique lock temporarily if the cache currently needs to be updated.
        /// @param currentNonUniformScale (Optional) The current non-uniform scale of the entity (if supported by the shape).
        void UpdateIntersectionParams(
            const AZ::Transform& currentTransform, const ShapeConfiguration& configuration,
            AZStd::shared_mutex* sharedMutex,
            [[maybe_unused]] const AZ::Vector3& currentNonUniformScale = AZ::Vector3::CreateOne())
        {
            // does the cache need updating
            if (m_cacheStatus > ShapeCacheStatus::Current)
            {
                if (sharedMutex)
                {
                    sharedMutex->unlock_shared();
                    sharedMutex->lock();
                }
                UpdateIntersectionParamsImpl(currentTransform, configuration, currentNonUniformScale); // shape specific cache update
                m_cacheStatus = ShapeCacheStatus::Current; // mark cache as up to date
                if (sharedMutex)
                {
                    sharedMutex->unlock();
                    sharedMutex->lock_shared();
                }
            }
        }

        /// Mark the cache as needing to be updated.
        void InvalidateCache(const InvalidateShapeCacheReason reason)
        {
            switch (reason)
            {
            case InvalidateShapeCacheReason::TransformChange:
                if (m_cacheStatus < ShapeCacheStatus::ObsoleteTransformChange)
                {
                    m_cacheStatus = ShapeCacheStatus::ObsoleteTransformChange;
                }
                break;
            case InvalidateShapeCacheReason::ShapeChange:
                if (m_cacheStatus < ShapeCacheStatus::ObsoleteShapeChange)
                {
                    m_cacheStatus = ShapeCacheStatus::ObsoleteShapeChange;
                }
                break;
            default:
                break;
            }
        }

    protected:
        /// Derived shape specific implementation of cache update (called from UpdateIntersectionParams).
        virtual void UpdateIntersectionParamsImpl(
            const AZ::Transform& currentTransform, const ShapeConfiguration& configuration,
            const AZ::Vector3& nonUniformScale = AZ::Vector3::CreateOne()) = 0;

        /// State of shape cache - should the internal shape cache be recalculated, or is it up to date.
        enum class ShapeCacheStatus
        {
            Current, ///< Cache is up to date.
            ObsoleteTransformChange, ///< The cache is invalid because the transform of the entity changed.
            ObsoleteShapeChange ///< The cache is invalid because the shape configuration/properties changed.
        };

        /// Expose read only cache status to derived IntersectionTestDataCache if different
        /// logic want to hapoen based on the cache status (shape/transform).
        ShapeCacheStatus CacheStatus() const
        {
            return m_cacheStatus;
        }

    private:
        ShapeCacheStatus m_cacheStatus = ShapeCacheStatus::Current; ///< The current state of the shape cache.
    };

    struct ShapeComponentGeneric
    {
        static void Reflect(AZ::ReflectContext* context);
    };

    /// Services provided by the Shape Component
    class ShapeComponentRequests : public AZ::EBusSharedDispatchTraits<ShapeComponentRequests>
    {
    public:
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        /// @brief Returns the type of shape that this component holds
        /// @return Crc32 indicating the type of shape
        virtual AZ::Crc32 GetShapeType() = 0;

        /// @brief Returns an AABB that encompasses this entire shape
        /// @return AABB that encompasses the shape
        virtual AZ::Aabb GetEncompassingAabb() = 0;

        /**
        * @brief Returns the local space bounds of a shape and its world transform
        * @param transform AZ::Transform outparam containing the shape transform
        * @param bounds AZ::Aabb outparam containing an untransformed tight fitting bounding box according to the shape parameters
        */
        virtual void GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) = 0;

        /// @brief Checks if a given point is inside a shape or outside it
        /// @param point Vector3 indicating the point to be tested
        /// @return bool indicating whether the point is inside or out
        virtual bool IsPointInside(const AZ::Vector3& point) = 0;

        /// @brief Returns the min distance a given point is from the shape
        /// @param point Vector3 indicating point to calculate distance from
        /// @return float indicating distance point is from shape
        virtual float DistanceFromPoint(const AZ::Vector3& point)
        {
            return sqrtf(DistanceSquaredFromPoint(point));
        }

        /// @brief Returns the min squared distance a given point is from the shape
        /// @param point Vector3 indicating point to calculate square distance from
        /// @return float indicating square distance point is from shape
        virtual float DistanceSquaredFromPoint(const AZ::Vector3& point) = 0;

        /// @brief Returns a random position inside the volume.
        /// @param randomDistribution An enum representing the different random distributions to use.
        virtual AZ::Vector3 GenerateRandomPointInside(AZ::RandomDistributionType /*randomDistribution*/)
        {
            AZ_Warning("ShapeComponentRequests", false, "GenerateRandomPointInside not implemented");
            return AZ::Vector3::CreateZero();
        }

        /// @brief Returns if a ray is intersecting the shape.
        virtual bool IntersectRay(const AZ::Vector3& /*src*/, const AZ::Vector3& /*dir*/, float& /*distance*/)
        {
            AZ_Warning("ShapeComponentRequests", false, "IntersectRay not implemented");
            return false;
        }

        /// Get the translation offset for the shape relative to its entity.
        virtual AZ::Vector3 GetTranslationOffset() const
        {
            AZ_WarningOnce("ShapeComponentRequests", !IsShapeComponentTranslationEnabled(), "GetTranslationOffset not implemented");
            return AZ::Vector3::CreateZero();
        }

        /// Set the translation offset for the shape relative to its entity.
        virtual void SetTranslationOffset([[maybe_unused]] const AZ::Vector3& translationOffset)
        {
            AZ_WarningOnce("ShapeComponentRequests", !IsShapeComponentTranslationEnabled(), "SetTranslationOffset not implemented");
        }

        virtual ~ShapeComponentRequests() = default;
    };

    // Bus to service the Shape component requests event group
    using ShapeComponentRequestsBus = AZ::EBus<ShapeComponentRequests>;

    /// Notifications sent by the shape component.
    class ShapeComponentNotifications : public AZ::ComponentBus
    {
    public:
        //! allows multiple threads to call shape requests
        using MutexType = AZStd::recursive_mutex;

        enum class ShapeChangeReasons
        {
            TransformChanged,
            ShapeChanged
        };

        /// @brief Informs listeners that the shape component has been updated (The shape was modified)
        /// @param changeReason Informs listeners of the reason for this shape change (transform change, the shape dimensions being altered)
        virtual void OnShapeChanged(ShapeChangeReasons changeReason) = 0;
    };

    // Bus to service Shape component notifications event group
    using ShapeComponentNotificationsBus = AZ::EBus<ShapeComponentNotifications>;

    /**
    * Common properties of how shape debug drawing can be rendererd.
    */
    struct ShapeDrawParams
    {
        AZ::Color m_shapeColor; ///< Color of underlying shape.
        AZ::Color m_wireColor; ///< Color of wireframe edges of shapes.
        bool m_filled; ///< Whether the shape should be rendered filled, or wireframe only.
    };

    class ShapeComponentConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(ShapeComponentConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(ShapeComponentConfig, "{32683353-0EF5-4FBC-ACA7-E220C58F60F5}", AZ::ComponentConfig);

        static void Reflect(AZ::ReflectContext* context);

        ShapeComponentConfig() = default;
        virtual ~ShapeComponentConfig() = default;

        void SetDrawColor(const AZ::Color& drawColor)
        {
            m_drawColor = drawColor;
        }

        const AZ::Color& GetDrawColor() const
        {
            return m_drawColor;
        }

        void SetIsFilled(bool isFilled)
        {
            m_filled = isFilled;
        }

        bool IsFilled() const
        {
            return m_filled;
        }

        ShapeDrawParams GetDrawParams() const
        {
            ShapeDrawParams drawParams;
            drawParams.m_shapeColor = GetDrawColor();
            drawParams.m_wireColor = AzFramework::ViewportColors::WireColor;
            drawParams.m_filled = IsFilled();

            return drawParams;
        }

    private:

        AZ::Color m_drawColor   = AzFramework::ViewportColors::DeselectedColor;
        bool      m_filled      = true;
    };
} // namespace LmbrCentral
