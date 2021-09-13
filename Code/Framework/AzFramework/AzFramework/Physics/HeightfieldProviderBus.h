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
    struct HeightMaterialPoint
    {
        int16_t m_height;
        uint8_t m_materialIndex0; // Top bit is a tesselation flag specifying which way the triangles go for this quad
        uint8_t m_materialIndex1; // Top bit is "reserved"

        static inline constexpr uint8_t HoleMaterial = 0x7F;
    };

    //! An interface to provide heightfield values.
    class HeightfieldProviderRequests
        : public AZ::ComponentBus
    {
    public:
        virtual AZ::Vector2 GetHeightfieldGridSpacing() = 0;
        virtual void GetHeightfieldGridSize(int32_t& width, int32_t& height) = 0;
        virtual void GetMaterialList(AZStd::vector<MaterialId>& materialList) = 0;
        virtual void GetHeights(AZStd::vector<int16_t>& heights) = 0;
        virtual void GetHeightsAsFloats(AZStd::vector<float>& heights) = 0;
        virtual void GetHeightsAsUint8(AZStd::vector<uint8_t>& heights) = 0;

        virtual void GetHeightsAndMaterials(AZStd::vector<HeightMaterialPoint>& heights) = 0;

        virtual void UpdateHeights(const AZ::Aabb& dirtyRegion, AZStd::vector<int16_t>& heights) = 0;
        virtual void UpdateHeightsAsFloats(const AZ::Aabb& dirtyRegion, AZStd::vector<float>& heights) = 0;
        virtual void UpdateHeightsAsUint8(const AZ::Aabb& dirtyRegion, AZStd::vector<uint8_t>& heights) = 0;

        virtual void UpdateHeightsAndMaterials(const AZ::Aabb& dirtyRegion, AZStd::vector<HeightMaterialPoint>& heights) = 0;
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

        virtual void RefreshHeightfield()
        {
        }

    protected:
        ~HeightfieldProviderNotifications() = default;
    };

    using HeightfieldProviderNotificationBus = AZ::EBus<HeightfieldProviderNotifications>;

} // namespace Physics
