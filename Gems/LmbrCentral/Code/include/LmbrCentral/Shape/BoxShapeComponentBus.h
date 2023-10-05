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
    /// Type ID for the BoxShapeComponent
    inline constexpr AZ::TypeId BoxShapeComponentTypeId{ "{5EDF4B9E-0D3D-40B8-8C91-5142BCFC30A6}" };

    /// Type ID for the EditorBoxShapeComponent
    inline constexpr AZ::TypeId EditorBoxShapeComponentTypeId{ "{2ADD9043-48E8-4263-859A-72E0024372BF}" };

    /// Type ID for the BoxShapeConfig
    inline constexpr AZ::TypeId BoxShapeConfigTypeId{ "{F034FBA2-AC2F-4E66-8152-14DFB90D6283}" };

    /// Type ID for the AxisAlignedBoxShapeComponent
    inline constexpr AZ::TypeId AxisAlignedBoxShapeComponentTypeId{ "{641D817E-1BC6-406A-BBB2-218541808E45}" };

    /// Type ID for the EditorAxisAlignedBoxShapeComponent
    inline constexpr AZ::TypeId EditorAxisAlignedBoxShapeComponentTypeId{ "{8C027DF6-E157-4159-9BF8-F1B925466F1F}" };

    /// Configuration data for BoxShapeComponent
    class BoxShapeConfig
        : public ShapeComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(BoxShapeConfig, AZ::SystemAllocator)
        AZ_RTTI(BoxShapeConfig, BoxShapeConfigTypeId, ShapeComponentConfig)

        static void Reflect(AZ::ReflectContext* context);

        BoxShapeConfig() = default;
        explicit BoxShapeConfig(const AZ::Vector3& dimensions) : m_dimensions(dimensions) {}

        void SetDimensions(const AZ::Vector3& dimensions)
        {
            AZ_WarningOnce("LmbrCentral", false, "SetDimensions Deprecated - Please use m_dimensions");
            m_dimensions = dimensions;
        }

        AZ::Vector3 GetDimensions() const
        {
            AZ_WarningOnce("LmbrCentral", false, "GetDimensions Deprecated - Please use m_dimensions");
            return m_dimensions;
        }

        AZ::Vector3 m_dimensions = AZ::Vector3::CreateOne(); ///< Stores the dimensions of the box along each axis.
        AZ::Vector3 m_translationOffset = AZ::Vector3::CreateZero(); ///< Translation offset from the entity position.
    };

    /// Services provided by the Box Shape Component
    class BoxShapeComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual const BoxShapeConfig& GetBoxConfiguration() const = 0;

        /// @brief Gets dimensions for the Box Shape
        /// @return Vector3 indicating dimensions along the x,y & z axis
        virtual AZ::Vector3 GetBoxDimensions() const = 0;

        /// @brief Sets new dimensions for the Box Shape
        /// @param newDimensions Vector3 indicating new dimensions along the x,y & z axis
        virtual void SetBoxDimensions(const AZ::Vector3& newDimensions) = 0;

        /// Returns true if the object type is axis-aligned box shape, otherwise false (regardless of orientation).
        virtual bool IsTypeAxisAligned() const = 0;
    };

    // Bus to service the Box Shape component event group
    using BoxShapeComponentRequestsBus = AZ::EBus<BoxShapeComponentRequests>;

} // namespace LmbrCentral
