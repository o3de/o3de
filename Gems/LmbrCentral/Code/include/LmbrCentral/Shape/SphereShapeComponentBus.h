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
    /// Type ID for SphereShapeComponent
    inline constexpr AZ::TypeId SphereShapeComponentTypeId{ "{E24CBFF0-2531-4F8D-A8AB-47AF4D54BCD2}" };

    /// Type ID for EditorSphereShapeComponent
    inline constexpr AZ::TypeId EditorSphereShapeComponentTypeId{ "{2EA56CBF-63C8-41D9-84D5-0EC2BECE748E}" };

    /// Configuration data for SphereShapeComponent
    class SphereShapeConfig
        : public ShapeComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(SphereShapeConfig, AZ::SystemAllocator);
        AZ_RTTI(SphereShapeConfig, "{4AADFD75-48A7-4F31-8F30-FE4505F09E35}", ShapeComponentConfig);
        
        static void Reflect(AZ::ReflectContext* context);

        SphereShapeConfig() = default;
        explicit SphereShapeConfig(float radius) : m_radius(radius) {}

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

        float m_radius = 0.5f;
        AZ::Vector3 m_translationOffset = AZ::Vector3::CreateZero(); ///< Translation offset from the entity position.
    };

    using SphereShapeConfiguration = SphereShapeConfig; ///< @deprecated Use SphereShapeConfig.

    /// Services provided by the Sphere Shape Component.
    class SphereShapeComponentRequests 
        : public AZ::ComponentBus
    {
    public:
        virtual const SphereShapeConfig& GetSphereConfiguration() const = 0;

        /// @brief Returns the radius for the sphere shape component.
        virtual float GetRadius() const = 0;

        /// @brief Sets the radius for the sphere shape component.
        /// @param radius new Radius of the sphere shape.
        virtual void SetRadius(float radius) = 0;
    };

    // Bus to service the Sphere Shape component event group
    using SphereShapeComponentRequestsBus = AZ::EBus<SphereShapeComponentRequests>;

} // namespace LmbrCentral
