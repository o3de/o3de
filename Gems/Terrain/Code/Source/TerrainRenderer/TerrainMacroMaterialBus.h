/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Color.h>
#include <Atom/RHI.Reflect/Size.h>
#include <Atom/RPI.Reflect/Image/Image.h>

namespace Terrain
{
    struct MacroMaterialData final
    {
        AZ_TYPE_INFO(MacroMaterialData, "{DC68E20A-3251-4E4E-8BC7-F6A2521FEF46}");

        static void Reflect(AZ::ReflectContext* context);

        AZ::EntityId m_entityId;
        AZ::Aabb m_bounds = AZ::Aabb::CreateNull();

        AZ::Data::Instance<AZ::RPI::Image> m_colorImage;
        AZ::Data::Instance<AZ::RPI::Image> m_normalImage;
        bool m_normalFlipX{ false };
        bool m_normalFlipY{ false };
        float m_normalFactor{ 0.0f };
        int32_t m_priority{ 0 };
    };

    /**
    * Request terrain macro material data.
    */
    class TerrainMacroMaterialRequests
        : public AZ::ComponentBus
    {
    public:
        static void Reflect(AZ::ReflectContext* context);
    
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        using MutexType = AZStd::recursive_mutex;
        ////////////////////////////////////////////////////////////////////////

        virtual ~TerrainMacroMaterialRequests() = default;

        //! Get the terrain macro material and the region that it covers.
        virtual MacroMaterialData GetTerrainMacroMaterialData() = 0;

        //! Get the macro color image size in pixels
        //! @return Number of pixels in the image width, height, and depth
        virtual AZ::RHI::Size GetMacroColorImageSize() const
        {
            return { 0, 0, 0 };
        }

        //! Get the number of macro color pixels per meter in world space.
        //! @return The number of pixels in the X and Y direction in one world space meter.
        virtual AZ::Vector2 GetMacroColorImagePixelsPerMeter() const
        {
            return AZ::Vector2(0.0f, 0.0f);
        }
    };

    using TerrainMacroMaterialRequestBus = AZ::EBus<TerrainMacroMaterialRequests>;
    
    /**
    * Notifications for when the terrain macro material data changes.
    */
    class TerrainMacroMaterialNotifications : public AZ::EBusTraits
    {
    public:
        //! Allow multiple listeners to the notification bus.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //! Notifications are broadcast to everyone and don't require subscribing to a specific ID or address.
        //! This is because the systems that care about this information wouldn't know which entity IDs to listen to until after
        //! it received a "macro material created" event, which is one of the events sent out on this bus.
        //! So instead, all the events include which entity ID they affect, but don't require subscribing to specific entity IDs.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! Notify any listeners that a new macro material has been created.
        //! @param macroMaterialEntity The Entity ID for the entity containing the macro material.
        //! @param macroMaterial The data for the newly-created macro material.
        virtual void OnTerrainMacroMaterialCreated(
            [[maybe_unused]] AZ::EntityId macroMaterialEntity,
            [[maybe_unused]] const MacroMaterialData& macroMaterial)
        {
        }

        //! Notify any listeners that the macro material data changed.
        //! @param macroMaterialEntity The Entity ID for the entity containing the macro material.
        //! @param macroMaterial The data for the changed macro material. (This data contains the new changes)
        virtual void OnTerrainMacroMaterialChanged(
            [[maybe_unused]] AZ::EntityId macroMaterialEntity, [[maybe_unused]] const MacroMaterialData& macroMaterial)
        {
        }

        //! Notify any listeners that the region affected by the macro material has changed (presumably by moving the transform or the box)
        //! @param macroMaterialEntity The Entity ID for the entity containing the macro material.
        //! @param oldRegion The previous region covered by the macro material.
        //! @param newRegion The new region covered by the macro material.
        virtual void OnTerrainMacroMaterialRegionChanged(
            [[maybe_unused]] AZ::EntityId macroMaterialEntity,
            [[maybe_unused]] const AZ::Aabb& oldRegion,
            [[maybe_unused]] const AZ::Aabb& newRegion)
        {
        }

        //! Notify any listeners that the macro material has been destroyed.
        //! @param macroMaterialEntity The Entity ID for the entity containing the macro material.
        virtual void OnTerrainMacroMaterialDestroyed([[maybe_unused]] AZ::EntityId macroMaterialEntity)
        {
        }
    };
    using TerrainMacroMaterialNotificationBus = AZ::EBus<TerrainMacroMaterialNotifications>;

    class ImageTileBuffer;

    using PixelIndex = AZStd::pair<int16_t, int16_t>;

    //! EBus that can be used to modify the image data for a Terrain Macro Color texture.
    //! The following APIs are the low-level image modification APIs that enable image modifications at the per-pixel level.
    class TerrainMacroColorModifications : public AZ::ComponentBus
    {
    public:
        // Overrides the default AZ::EBusTraits handler policy to allow only one listener per entity.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Start an image modification session.
        //! This will create a modification buffer that contains an uncompressed copy of the current macro color image data.
        virtual void StartMacroColorImageModification() = 0;

        //! Finish an image modification session.
        //! Clean up any helper structures used during image modification.
        virtual void EndMacroColorImageModification() = 0;

        //! Given a list of world positions, return a list of pixel indices into the image.
        //! @param positions The list of world positions to query
        //! @param outIndices [out] The list of output PixelIndex values giving the (x,y) pixel coordinates for each world position.
        virtual void GetMacroColorPixelIndicesForPositions(
            AZStd::span<const AZ::Vector3> positions, AZStd::span<PixelIndex> outIndices) const = 0;

        //! Get the image pixel values at a list of positions.
        //! @param positions The list of world positions to query
        //! @param outValues [out] The list of output values. This list is expected to be the same size as the positions list.
        virtual void GetMacroColorPixelValuesByPosition(AZStd::span<const AZ::Vector3> positions, AZStd::span<AZ::Color> outValues) const = 0;

        //! Get the image pixel values at a list of pixel indices.
        //! @param positions The list of pixel indices to query
        //! @param outValues [out] The list of output values. This list is expected to be the same size as the positions list.
        virtual void GetMacroColorPixelValuesByPixelIndex(AZStd::span<const PixelIndex> indices, AZStd::span<AZ::Color> outValues) const = 0;

        //! Start a series of pixel modifications.
        //! This will track all of the pixels modified so that they can be updated once at the end.
        virtual void StartMacroColorPixelModifications() = 0;

        //! End a series of pixel modifications.
        //! This will notify that the series of pixel modifications have ended, so buffer refreshes can now happen.
        virtual void EndMacroColorPixelModifications() = 0;

        //! Given a list of pixel indices, set those pixels to the given values.
        //! @param indicdes The list of pixel indices to set the values for.
        //! @param values The list of values to set. This list is expected to be the same size as the positions list.
        virtual void SetMacroColorPixelValuesByPixelIndex(AZStd::span<const PixelIndex> indices, AZStd::span<const AZ::Color> values) = 0;
    };

    using TerrainMacroColorModificationBus = AZ::EBus<TerrainMacroColorModifications>;

    //! EBus that notifies about the current state of Terrain Macro Color modifications
    class TerrainMacroColorModificationNotifications : public AZ::ComponentBus
    {
    public:
        //! Notify any listeners that a brush stroke has started on the macro color image.
        virtual void OnTerrainMacroColorBrushStrokeBegin() {}

        //! Notify any listeners that a brush stroke has ended on the macro color image.
        //! @param changedDataBuffer A pointer to the ImageTileBuffer containing the changed data. The buffer will be deleted
        //! after this notification unless a listener keeps a copy of the pointer (for undo/redo, for example).
        //! @param dirtyRegion The AABB defining the world space region affected by the brush stroke.
        virtual void OnTerrainMacroColorBrushStrokeEnd(
            [[maybe_unused]] AZStd::shared_ptr<ImageTileBuffer> changedDataBuffer, [[maybe_unused]] const AZ::Aabb& dirtyRegion)
        {
        }
    };

    using TerrainMacroColorModificationNotificationBus = AZ::EBus<TerrainMacroColorModificationNotifications>;
}
