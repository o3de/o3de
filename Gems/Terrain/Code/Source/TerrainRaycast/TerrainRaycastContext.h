/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Render/IntersectorInterface.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Terrain
{
    class TerrainSystem;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class TerrainRaycastContext : public AzFramework::RenderGeometry::IntersectorBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] terrainSystem The terrain system that owns this terrain raycast context
        TerrainRaycastContext(TerrainSystem& terrainSystem);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(TerrainRaycastContext);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~TerrainRaycastContext();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the terrain raycast context's entity context id
        //! \return The terrain raycast context's entity context id
        inline AzFramework::EntityContextId GetEntityContextId() const { return m_entityContextId; }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::RenderGeometry::RayIntersect
        AzFramework::RenderGeometry::RayResult RayIntersect(const AzFramework::RenderGeometry::RayRequest& ray) override;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // RenderGeometry::IntersectorBus inherits from RenderGeometry::IntersectionNotifications,
        // so we must override the following pure virtual functions. We could potentially implement
        // them using TerrainSystem::m_registeredAreas, but right now that would not serve a purpose.
        ///@{
        //! Unused pure virtual override
        void OnEntityConnected(AZ::EntityId) override {}
        void OnEntityDisconnected(AZ::EntityId) override {}
        void OnGeometryChanged(AZ::EntityId) override {}
        ///@}

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        TerrainSystem& m_terrainSystem; //!< Terrain system that owns this terrain raycast context
        AzFramework::EntityContextId m_entityContextId; //!< This object's entity context id
    };
} // namespace Terrain
