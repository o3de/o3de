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
    /// Type ID of CylinderShapeComponent
    inline constexpr AZ::TypeId CylinderShapeComponentTypeId{ "{B0C6AA97-E754-4E33-8D32-33E267DB622F}" };

    /// Type ID of EditorCylinderShapeComponent
    inline constexpr AZ::TypeId EditorCylinderShapeComponentTypeId{ "{D5FC4745-3C75-47D9-8C10-9F89502487DE}" };

    /// Configuration data for CylinderShapeComponent
    class CylinderShapeConfig
        : public ShapeComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(CylinderShapeConfig, AZ::SystemAllocator);
        AZ_RTTI(CylinderShapeConfig, "{53254779-82F1-441E-9116-81E1FACFECF4}", ComponentConfig);
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

        float m_height = 1.f; ///< The height of this cylinder.
        float m_radius = 0.5f; ///< The radius of this cylinder.
    };

    /// Services provided by the Cylinder Shape Component.
    class CylinderShapeComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual const CylinderShapeConfig& GetCylinderConfiguration() const = 0;

        /// @brief Returns the end to end height of the cylinder.
        virtual float GetHeight() const = 0;

        /// @brief Returns the radius of the cylinder.
        virtual float GetRadius() const = 0;
        
        /// @brief Sets height of the cylinder
        /// @param height new height of the cylinder
        virtual void SetHeight(float height) = 0;

        /// @brief Sets radius of the cylinder
        /// @param radius new radius of the cylinder
        virtual void SetRadius(float radius) = 0;
    };

    // Bus to service the Cylinder Shape component event group
    using CylinderShapeComponentRequestsBus = AZ::EBus<CylinderShapeComponentRequests>;

} // namespace LmbrCentral
