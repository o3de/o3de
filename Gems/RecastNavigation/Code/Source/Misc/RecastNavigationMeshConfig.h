/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/EditContext.h>

namespace RecastNavigation
{
    //! See @rcConfig from Recast library for documentation on these parameters.
    //! @Reflect method provides that documentation as tooltips in the Editor.
    //!
    //! @note Pay attention to the units of the properties in this configuration,
    //! some of them are in world units (regular O3DE world space) and others are
    //! in Recast voxels, which are defined by @m_cellSize. 1 voxel = 1 @m_cellSize in world units.
    class RecastNavigationMeshConfig final
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(RecastNavigationMeshConfig, AZ::SystemAllocator)
        AZ_RTTI(RecastNavigationMeshConfig, "{472141BB-7C36-4D2C-A508-78DB0D3C07F2}");

        static void Reflect(AZ::ReflectContext* context);

        float m_tileSize = 32.f;
        int m_borderSize = 20;
        float m_cellSize = 0.4f;
        float m_cellHeight = 0.4f;

        float m_agentMaxSlope = 45.0f;
        float m_agentHeight = 2.0f;
        float m_agentRadius = 0.6f;
        float m_agentMaxClimb = 0.9f;

        float m_edgeMaxError = 1.3f;
        float m_edgeMaxLen = 12.0f;

        int m_maxVerticesPerPoly = 6;
        float m_detailSampleDist = 6.0f;
        float m_detailSampleMaxError = 1.0f;

        int m_regionMinSize = 8;
        int m_regionMergeSize = 20;

        bool m_filterLowHangingObstacles = true;
        bool m_filterLedgeSpans = true;
        bool m_filterWalkableLowHeightSpans = true;

        //! If enabled, draw the navigation mesh in the game.
        bool m_enableDebugDraw = false;

        //! If enabled, previews the navigation mesh in the Editor's viewport.
        bool m_enableEditorPreview = false;
    };

} // namespace RecastNavigation
