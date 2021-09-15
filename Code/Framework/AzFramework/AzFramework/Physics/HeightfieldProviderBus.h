/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzFramework/Physics/Material.h>

namespace Physics
{
    class FlagAndUint7
    {
    public:
        FlagAndUint7()
            : m_data(0)
        {
        }

        FlagAndUint7(const uint8_t data, bool flag = false)
        {
            AZ_Assert(data < 0x80, "Value is greater than 0x80 in FlagAndUint7");
            m_data = flag ? (data & 0x7f) | 0x80 : data & 0x7f;
        }

        FlagAndUint7 operator=(uint8_t data)
        {
            AZ_Assert(data < 0x80, "Value is greater than 0x80 in FlagAndUint7");
            m_data = (m_data & 0x80) | (data & 0x7f);
            return m_data;
        }

        operator uint8_t() const { return m_data & 0x7f; }

        void SetFlag() { m_data |= 0x80; }
        void ClearFlag() { m_data &= 0x7f; }
        bool Flag() const { return m_data & 0x80; }

    private:
        uint8_t m_data;
    };

    struct HeightMaterialPoint
    {
        int16_t m_height;
        FlagAndUint7 m_materialIndex0; // Flag is a tesselation flag specifying which way the triangles go for this quad
        FlagAndUint7 m_materialIndex1; // Flag is "reserved"

        static inline constexpr uint8_t HoleMaterial = 0x7F;
    };

    //! An interface to provide heightfield values.
    class HeightfieldProviderRequests
        : public AZ::ComponentBus
    {
    public:
        /// Returns the distance between each height in the map.
        /// @return Vector containing Column Spacing, Rows Spacing.
        virtual AZ::Vector2 GetHeightfieldGridSpacing() const = 0;

        /// Returns the height field gridsize.
        /// @param numColumns contains the size of the grid in the x direction.
        /// @param numRows contains the size of the grid in the y direction.
        virtual void GetHeightfieldGridSize(int32_t& numColumns, int32_t& numRows) const = 0;
      
        /// Returns the list of materials used by the height field.
        /// @param materialList contains a vector of all materials.
        virtual void GetMaterialList(AZStd::vector<MaterialId>& materialList) const = 0;

        /// Returns the list of heights used by the height field.
        /// @return the rows*columns vector of the heights.
        virtual AZStd::vector<int16_t> GetHeights() const = 0;

        /// Returns the list of heights and materials used by the height field.
        /// @param heights contains the rows*columns vector of the heights and materials.
        virtual AZStd::vector<Physics::HeightMaterialPoint> GetHeightsAndMaterials() const = 0;

        /// Returns the scale used by the height field.
        /// @return heightScale contains the scale of the height values returned by GetHeights.
        virtual float GetScale() const = 0;

        /// Updates values in the height field.
        /// @param dirtyRegion contains the axis aligned bounding box that will be updated.
        /// @param heights contains all the new heights for the height field.
        virtual AZStd::vector<int16_t> UpdateHeights(const AZ::Aabb& dirtyRegion) const = 0;

        /// Updates values in the height field.
        /// @param dirtyRegion contains the axis aligned bounding box that will be updated.
        /// @param heights contains all the new heights and materials for the height field.
        virtual AZStd::vector<HeightMaterialPoint> UpdateHeightsAndMaterials(const AZ::Aabb& dirtyRegion) const = 0;
    };

    using HeightfieldProviderRequestsBus = AZ::EBus<HeightfieldProviderRequests>;

    //! Broadcasts notifications when heightfield data changes - heightfield providers implement HeightfieldRequests bus.
    class HeightfieldProviderNotifications
        : public AZ::ComponentBus
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        virtual void OnHeightfieldDataChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion)
        {
        }

    protected:
        ~HeightfieldProviderNotifications() = default;
    };

    using HeightfieldProviderNotificationBus = AZ::EBus<HeightfieldProviderNotifications>;
} // namespace Physics
