/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Reflect/Pass/PassDescriptor.h>
#include <TerrainRenderer/TerrainClipmapManager.h>

namespace Terrain
{
    //! The debug render pass to display a single clipmap at the bottom-right corner.
    //! It is located between the DebugOverlay pass and the UI pass.
    //! By slightly modifying the fullscreen pass, we can still use a single triangle
    //! but have a constant-sized texture in terms of viewport, whose size is adjustable by CVars.
    //! See TerrainClipmapDebugPass.cpp for CVars controls.
    class TerrainClipmapDebugPass : public AZ::RPI::FullscreenTrianglePass
    {
        AZ_RPI_PASS(TerrainClipmapDebugPass);

    public:
        AZ_RTTI(Terrain::TerrainClipmapDebugPass, "{BF1ED790-34BB-4E09-803B-09BF5BBFF0BD}", AZ::RPI::FullscreenTrianglePass);
        AZ_CLASS_ALLOCATOR(Terrain::TerrainClipmapDebugPass, AZ::SystemAllocator);
        virtual ~TerrainClipmapDebugPass() = default;

        static AZ::RPI::Ptr<TerrainClipmapDebugPass> Create(const AZ::RPI::PassDescriptor& descriptor);

        void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
        void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

        //! Return true when both the enable CVar and standard enable flag are true.
        bool IsEnabled() const override;
    private:
        TerrainClipmapDebugPass(const AZ::RPI::PassDescriptor& descriptor);
    };
}
