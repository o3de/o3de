/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>

#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace LmbrCentral
{
    /// Type ID for the AxisAlignedBoxShapeComponent
    static const AZ::Uuid AxisAlignedBoxShapeComponentTypeId = "{641D817E-1BC6-406A-BBB2-218541808E45}";

    /// Type ID for the EditorAxisAlignedBoxShapeComponent
    static const AZ::Uuid EditorAxisAlignedBoxShapeComponentTypeId = "{8C027DF6-E157-4159-9BF8-F1B925466F1F}";

    /// Configuration data for AxisAlignedBoxShapeComponent
    class AxisAlignedBoxShapeConfig
        : public ShapeComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(AxisAlignedBoxShapeConfig, AZ::SystemAllocator, 0)
        AZ_RTTI(AxisAlignedBoxShapeConfig, "{3D882524-35C7-41D7-A5D3-79D8E2E49906}", ShapeComponentConfig)

        static void Reflect(AZ::ReflectContext* context);

        AxisAlignedBoxShapeConfig() = default;
        explicit AxisAlignedBoxShapeConfig(const AZ::Vector3& dimensions) : m_dimensions(dimensions) {}

        AZ::Vector3 m_dimensions = AZ::Vector3::CreateOne(); ///< Stores the dimensions of the box along each axis.        
    };

    /// Services provided by the Axis Aligned Box Shape Component
    class AxisAlignedBoxShapeComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual AxisAlignedBoxShapeConfig GetBoxConfiguration() = 0;

        /// @brief Gets dimensions for the Box Shape
        /// @return Vector3 indicating dimensions along the x,y & z axis
        virtual AZ::Vector3 GetBoxDimensions() = 0;

        /// @brief Sets new dimensions for the Box Shape
        /// @param newDimensions Vector3 indicating new dimensions along the x,y & z axis
        virtual void SetBoxDimensions(const AZ::Vector3& newDimensions) = 0;
    };

    // Bus to service the Box Shape component event group
    using AxisAlignedBoxShapeComponentRequestsBus = AZ::EBus<AxisAlignedBoxShapeComponentRequests>;

} // namespace LmbrCentral
