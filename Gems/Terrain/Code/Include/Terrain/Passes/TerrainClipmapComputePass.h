/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Reflect/Pass/ComputePassData.h>
#include <Atom/RPI.Reflect/Pass/PassDescriptor.h>

namespace Terrain
{
    class TerrainClipmapGenerationPass
        : public AZ::RPI::ComputePass
    {
        AZ_RPI_PASS(TerrainClipmapGenerationPass);

    public:
        AZ_RTTI(Terrain::TerrainClipmapGenerationPass, "{BD504E93-87F4-484E-A17A-E337C3F2279C}", AZ::RPI::ComputePass);
        AZ_CLASS_ALLOCATOR(Terrain::TerrainClipmapGenerationPass, AZ::SystemAllocator, 0);
        virtual ~TerrainClipmapGenerationPass() = default;

        static AZ::RPI::Ptr<TerrainClipmapGenerationPass> Create(const AZ::RPI::PassDescriptor& descriptor);

        void InitializeInternal() override;
        void FrameBeginInternal(FramePrepareParams params) override;
        void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
        void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

    private:
        TerrainClipmapGenerationPass(const AZ::RPI::PassDescriptor& descriptor);

        AZ::RHI::ShaderInputImageIndex m_macroColorClipmapsIndex;
        AZ::RHI::ShaderInputImageIndex m_macroNormalClipmapsIndex;
        AZ::RHI::ShaderInputImageIndex m_detailColorClipmapsIndex;
        AZ::RHI::ShaderInputImageIndex m_detailNormalClipmapsIndex;
        AZ::RHI::ShaderInputImageIndex m_detailHeightClipmapsIndex;
        AZ::RHI::ShaderInputImageIndex m_detailMiscClipmapsIndex;

        bool m_needsUpdate = true;
    };
} // namespace Terrain
