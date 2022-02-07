/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <TerrainSystem/TerrainSystem.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Terrain
{
    class TerrainWorldDebuggerConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainWorldDebuggerConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainWorldDebuggerConfig, "{92686FA9-2C0B-47F1-8E2D-F2F302CDE5AA}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        bool m_drawWireframe{ true };
        bool m_drawWorldBounds{ true };
    };


    class TerrainWorldDebuggerComponent
        : public AZ::Component
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AzFramework::BoundsRequestBus::Handler
        , private AzFramework::Terrain::TerrainDataNotificationBus::Handler
    {
    public:
        template<typename, typename>
        friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(TerrainWorldDebuggerComponent, "{ECA1F4CB-5395-41FD-B6ED-FFD2C80096E2}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        TerrainWorldDebuggerComponent(const TerrainWorldDebuggerConfig& configuration);
        TerrainWorldDebuggerComponent() = default;
        ~TerrainWorldDebuggerComponent() override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // EntityDebugDisplayEventBus

        // Ideally this would use ViewportDebugDisplayEventBus::DisplayViewport, but that doesn't currently work in game mode,
        // so instead we use this plus the BoundsRequestBus with a large AABB to get ourselves rendered.
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        //////////////////////////////////////////////////////////////////////////
        // BoundsRequestBus
        AZ::Aabb GetWorldBounds() override;
        AZ::Aabb GetLocalBounds() override;

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Terrain::TerrainDataNotificationBus
        void OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask) override;

    private:

        // Cache our debug wireframe representation in "sectors" of data so that we can easily control how far out we draw
        // the wireframe representation in each direction.
        struct WireframeSector
        {
            AZ::Aabb m_aabb;
            AZStd::vector<AZ::Vector3> m_lineVertices;
        };

        // Each sector contains an N x N grid of squares that it will draw.  Since this is a count of the number of terrain grid points
        // in each direction, the actual world size will depend on the terrain grid resolution in each direction.
        static constexpr int32_t SectorSizeInGridPoints = 10;

        // For each grid point we will draw half a square (left-right, top-down), so we need 4 vertices for the two lines.
        static constexpr int32_t VerticesPerGridPoint = 4;

        // Pre-calculate the total number of vertices per sector.
        static constexpr int32_t VerticesPerSector =
            (SectorSizeInGridPoints * VerticesPerGridPoint) * (SectorSizeInGridPoints * VerticesPerGridPoint);

        // AuxGeom has limits to the number of lines it can draw in a frame, so we'll cap how many total sectors to draw.
        static constexpr int32_t MaxVerticesToDraw = 500000;
        static constexpr int32_t MaxSectorsToDraw = MaxVerticesToDraw / VerticesPerSector;

        void RefreshCachedWireframeGrid(const AZ::Aabb& dirtyRegion);

        TerrainWorldDebuggerConfig m_configuration;
        AZStd::vector<WireframeSector> m_wireframeSectors;
        AZ::Aabb m_wireframeBounds;
    };
}
