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
    //! The QuadMeshType specifies the property of the heightfield quad.
    enum class QuadMeshType : uint8_t
    {
        SubdivideUpperLeftToBottomRight, //!< Subdivide the quad, from upper left to bottom right |\|, into two triangles.
        SubdivideBottomLeftToUpperRight, //!< Subdivide the quad, from bottom left to upper right |/|, into two triangles.
        Hole    //!< The quad should be treated as a hole in the heightfield.
    };

    struct HeightMaterialPoint
    {
        float m_height{ 0.0f }; //!< Holds the height of this point in the heightfield relative to the heightfield entity location.
        QuadMeshType m_quadMeshType{ QuadMeshType::SubdivideUpperLeftToBottomRight }; //!< By default, create two triangles like this |\|, where this point is in the upper left corner.
        uint8_t m_materialIndex{ 0 }; //!< The surface material index for the upper left corner of this quad.
        uint16_t m_padding{ 0 }; //!< available for future use.
    };

    //! An interface to provide heightfield values.
    class HeightfieldProviderRequests
        : public AZ::ComponentBus
    {
    public:
        //! Returns the distance between each height in the map.
        //! @return Vector containing Column Spacing, Rows Spacing.
        virtual AZ::Vector2 GetHeightfieldGridSpacing() const = 0;

        //! Returns the height field gridsize.
        //! @param numColumns contains the size of the grid in the x direction.
        //! @param numRows contains the size of the grid in the y direction.
        virtual void GetHeightfieldGridSize(int32_t& numColumns, int32_t& numRows) const = 0;

        //! Returns the height field min and max height bounds.
        //! @param minHeightBounds contains the minimum height that the heightfield can contain.
        //! @param maxHeightBounds contains the maximum height that the heightfield can contain.
        virtual void GetHeightfieldHeightBounds(float& minHeightBounds, float& maxHeightBounds) const = 0;

        //! Returns the AABB of the heightfield.
        //! This is provided separately from the shape AABB because the heightfield might choose to modify the AABB bounds.
        //! @return AABB of the heightfield.
        virtual AZ::Aabb GetHeightfieldAabb() const = 0;

        //! Returns the world transform for the heightfield.
        //! This is provided separately from the entity transform because the heightfield might want to clear out the rotation or scale.
        //! @return world transform that should be used with the heightfield data.
        virtual AZ::Transform GetHeightfieldTransform() const = 0;

        //! Returns the list of materials used by the height field.
        //! @return returns a vector of all materials.
        virtual AZStd::vector<MaterialId> GetMaterialList() const = 0;

        //! Returns the list of heights used by the height field.
        //! @return the rows*columns vector of the heights.
        virtual AZStd::vector<float> GetHeights() const = 0;

        //! Returns the list of heights and materials used by the height field.
        //! @return the rows*columns vector of the heights and materials.
        virtual AZStd::vector<Physics::HeightMaterialPoint> GetHeightsAndMaterials() const = 0;
    };

    using HeightfieldProviderRequestsBus = AZ::EBus<HeightfieldProviderRequests>;

    //! Broadcasts notifications when heightfield data changes - heightfield providers implement HeightfieldRequests bus.
    class HeightfieldProviderNotifications
        : public AZ::ComponentBus
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //! Called whenever the heightfield data changes.
        //! @param the AABB of the area of data that changed.
        virtual void OnHeightfieldDataChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion)
        {
        }

    protected:
        ~HeightfieldProviderNotifications() = default;
    };

    using HeightfieldProviderNotificationBus = AZ::EBus<HeightfieldProviderNotifications>;
} // namespace Physics
