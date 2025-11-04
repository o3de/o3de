/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace LmbrCentral
{
    /// Type ID for CapsuleShapeComponent
    inline constexpr AZ::TypeId CapsuleShapeComponentTypeId{ "{967EC13D-364D-4696-AB5C-C00CC05A2305}" };

    /// Type ID for EditorCapsuleShapeComponent
    inline constexpr AZ::TypeId EditorCapsuleShapeComponentTypeId{ "{06B6C9BE-3648-4DA2-9892-755636EF6E19}" };

    /// Configuration data for CapsuleShapeComponent
    class CapsuleShapeConfig
        : public ShapeComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(CapsuleShapeConfig, AZ::SystemAllocator);
        AZ_RTTI(CapsuleShapeConfig, "{00931AEB-2AD8-42CE-B1DC-FA4332F51501}", ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        void SetHeight(float height)
        {
            AZ_WarningOnce("LmbrCentral", false, "SetHeight Deprecated - Please use m_height");
            m_height = height;
        }

        float GetHeight() const
        {
            AZ_WarningOnce("LmbrCentral", false, "GetHeight Deprecated - Please use m_height");
            return m_height;
        }

        void SetRadius(float radius)
        {
            AZ_WarningOnce("LmbrCentral", false, "SetRadius Deprecated - Please use m_radius");
            m_radius = radius;
        }

        float GetRadius() const
        {
            AZ_WarningOnce("LmbrCentral", false, "GetRadius Deprecated - Please use m_radius");
            return m_radius;
        }

        float m_height = 1.0f; ///< The end to end height of capsule, this includes the cylinder and both caps.
        float m_radius = 0.25f; ///< The radius of this capsule.
        AZ::Vector3 m_translationOffset = AZ::Vector3::CreateZero(); ///< Translation offset from the entity position.
    };

    struct CapsuleInternalEndPoints
    {
        AZ::Vector3 m_begin;
        AZ::Vector3 m_end;
    };

    /// Services provided by the Capsule Shape Component
    class CapsuleShapeComponentRequests 
        : public AZ::ComponentBus
    {
    public:
        virtual const CapsuleShapeConfig& GetCapsuleConfiguration() const = 0;

        /// @brief Returns the end to end height of the capsule, this includes the cylinder and both caps.
        virtual float GetHeight() const = 0;

        /// @brief Returns the radius of the capsule.
        virtual float GetRadius() const = 0;

        /// @brief Returns the base and top points of the capsule, corresponding to the center points of the cap spheres.
        virtual CapsuleInternalEndPoints GetCapsulePoints() const = 0;

        /// @brief Sets the end to end height of capsule, this includes the cylinder and both caps.
        /// @param height new height of the capsule.
        virtual void SetHeight(float height) = 0;

        /// @brief Sets radius of the capsule.
        /// @param radius new radius of the capsule.
        virtual void SetRadius(float radius) = 0;
    };

    // Bus to service the Capsule Shape component event group
    using CapsuleShapeComponentRequestsBus = AZ::EBus<CapsuleShapeComponentRequests>;

} // namespace LmbrCentral
