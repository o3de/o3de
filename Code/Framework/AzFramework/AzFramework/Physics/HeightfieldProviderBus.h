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
#include <AzFramework/Physics/Material/PhysicsMaterialAsset.h>
#include <AzCore/EBus/EBusSharedDispatchTraits.h>

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
        HeightMaterialPoint(
            float height = 0.0f, QuadMeshType type = QuadMeshType::SubdivideUpperLeftToBottomRight, uint8_t index = 0)
            : m_height(height)
            , m_quadMeshType(type)
            , m_materialIndex(index)
            , m_padding(0)
        {
        }

        ~HeightMaterialPoint() = default;

        static void Reflect(AZ::ReflectContext* context);

        AZ_TYPE_INFO(HeightMaterialPoint, "{DF167ED4-24E6-4F7B-8AB7-42622F7DBAD3}");
        float m_height{ 0.0f }; //!< Holds the height of this point in the heightfield relative to the heightfield entity location.
        QuadMeshType m_quadMeshType{ QuadMeshType::Hole }; //!< By default, create an empty point.
        uint8_t m_materialIndex{ 0 }; //!< The surface material index for the upper left corner of this quad.
        uint16_t m_padding{ 0 }; //!< available for future use.
    };

    using UpdateHeightfieldSampleFunction = AZStd::function<void(size_t column, size_t row, const Physics::HeightMaterialPoint& point)>;
    using UpdateHeightfieldCompleteFunction = AZStd::function<void()>;

    //! An interface to provide heightfield values.
    //! This EBus supports multiple concurrent requests from different threads.
    class HeightfieldProviderRequests : public AZ::EBusSharedDispatchTraits<HeightfieldProviderRequests>
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::EntityId BusIdType;

        static void Reflect(AZ::ReflectContext* context);

        //! Returns the distance between each height in the map.
        //! @return Vector containing Column Spacing, Rows Spacing.
        virtual AZ::Vector2 GetHeightfieldGridSpacing() const = 0;

        //! Returns the height field gridsize.
        //! @param numColumns contains the size of the grid in the x direction.
        //! @param numRows contains the size of the grid in the y direction.
        virtual void GetHeightfieldGridSize(size_t& numColumns, size_t& numRows) const = 0;

        //! Returns the height field gridsize columns.
        //! @return the size of the grid in the x direction.
        virtual AZ::u64 GetHeightfieldGridColumns() const = 0;

        //! Returns the height field gridsize rows.
        //! @return the size of the grid in the y direction.
        virtual AZ::u64 GetHeightfieldGridRows() const = 0;

        //! Returns the height field min and max height bounds.
        //! @param minHeightBounds contains the minimum height that the heightfield can contain.
        //! @param maxHeightBounds contains the maximum height that the heightfield can contain.
        virtual void GetHeightfieldHeightBounds(float& minHeightBounds, float& maxHeightBounds) const = 0;

        //! Returns the height field min height bounds.
        //! @return the minimum height that the heightfield can contain.
        virtual float GetHeightfieldMinHeight() const = 0;

        //! Returns the height field max height bounds.
        //! @return the maximum height that the heightfield can contain.
        virtual float GetHeightfieldMaxHeight() const = 0;

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
        virtual AZStd::vector<AZ::Data::Asset<Physics::MaterialAsset>> GetMaterialList() const = 0;

        //! Returns the list of heights used by the height field.
        //! @return the rows*columns vector of the heights.
        virtual AZStd::vector<float> GetHeights() const = 0;

        //! Returns the list of heights and materials used by the height field.
        //! @return the rows*columns vector of the heights and materials.
        virtual AZStd::vector<Physics::HeightMaterialPoint> GetHeightsAndMaterials() const = 0;

        //! Return the specific heightfield row/column data that exists inside a given AABB region.
        //! @param region The input region to get row/column data for
        //! @param startColumn [out] The starting heightfield column index for that region
        //! @param startRow [out] The starting heightfield row index for that region
        //! @param numColumns [out] The number of columns that exist within the region
        //! @param numRows [out] The number of rows that exist within the region
        virtual void GetHeightfieldIndicesFromRegion(
            const AZ::Aabb& region, size_t& startColumn, size_t& startRow, size_t& numColumns, size_t& numRows) const = 0;

        //! Updates the list of heights and materials within the region.
        virtual void UpdateHeightsAndMaterials(
            const UpdateHeightfieldSampleFunction& updateHeightsMaterialsCallback,
            size_t startColumn, size_t startRow, size_t numColumns, size_t numRows) const = 0;

        //! Asynchronously updates the list of heights and materials within the region.
        virtual void UpdateHeightsAndMaterialsAsync(
            const UpdateHeightfieldSampleFunction& updateHeightsMaterialsCallback,
            const UpdateHeightfieldCompleteFunction& updateHeightsCompleteCallback,
            size_t startColumn,
            size_t startRow,
            size_t numColumns,
            size_t numRows) const = 0;
    };

    using HeightfieldProviderRequestsBus = AZ::EBus<HeightfieldProviderRequests>;

    //! Broadcasts notifications when heightfield data changes - heightfield providers implement HeightfieldRequests bus.
    class HeightfieldProviderNotifications
        : public AZ::ComponentBus
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        enum class HeightfieldChangeMask : AZ::u8
        {
            None = 0,
            Settings = (1 << 0),
            HeightData = (1 << 1),
            SurfaceData = (1 << 2),
            SurfaceMapping = (1 << 3)
        };

        //! Called whenever the heightfield data changes.
        //! @param the AABB of the area of data that changed.
        virtual void OnHeightfieldDataChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion, 
            [[maybe_unused]] Physics::HeightfieldProviderNotifications::HeightfieldChangeMask changeMask)
        {
        }

    protected:
        ~HeightfieldProviderNotifications() = default;
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(HeightfieldProviderNotifications::HeightfieldChangeMask)

    using HeightfieldProviderNotificationBus = AZ::EBus<HeightfieldProviderNotifications>;
} // namespace Physics
