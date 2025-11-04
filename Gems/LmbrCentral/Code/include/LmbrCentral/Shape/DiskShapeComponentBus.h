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

namespace AZ
{
    class Vector3;
}

namespace LmbrCentral
{
    //! Type ID for DiskShapeComponent
    inline constexpr AZ::TypeId DiskShapeComponentTypeId{ "{A3E6BE21-29B7-46AA-8B0E-1D8372DADA3F}" };

    //! Type ID for EditorDiskShapeComponent
    inline constexpr AZ::TypeId EditorDiskShapeComponentTypeId{ "{5CD2459F-9D51-4FA3-9D35-D1A2C65ED272}" };

    //! Configuration data for DiskShapeComponent
    class DiskShapeConfig
        : public ShapeComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(DiskShapeConfig, AZ::SystemAllocator);
        AZ_RTTI(DiskShapeConfig, "{24EC2919-F198-4871-8404-F6DE8A16275E}", ShapeComponentConfig);
        
        static void Reflect(AZ::ReflectContext* context);

        DiskShapeConfig() = default;
        explicit DiskShapeConfig(float radius) : m_radius(radius) {}

        float m_radius = 0.5f;
    };

    //! Services provided by the Disk Shape Component.
    class DiskShapeComponentRequests 
        : public AZ::ComponentBus
    {
    public:
        virtual const DiskShapeConfig& GetDiskConfiguration() const = 0;

        //! @brief Returns the radius for the disk shape component.
        virtual float GetRadius() const = 0;

        //! @brief Sets the radius for the disk shape component.
        //! @param radius new Radius of the disk shape.
        virtual void SetRadius(float radius) = 0;

        //! @brief Convenience function that returns the facing normal for the disk determined by the transform component.
        virtual const AZ::Vector3& GetNormal() const = 0;

    };

    // Bus to service the Disk Shape component event group
    using DiskShapeComponentRequestBus = AZ::EBus<DiskShapeComponentRequests>;

} // namespace LmbrCentral
