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
    class TerrainMacroTextureClipmapGenerationPass : public AZ::RPI::ComputePass
    {
        AZ_RPI_PASS(TerrainMacroTextureClipmapGenerationPass);

    public:
        AZ_RTTI(Terrain::TerrainMacroTextureClipmapGenerationPass, "{BD504E93-87F4-484E-A17A-E337C3F2279C}", AZ::RPI::ComputePass);
        AZ_CLASS_ALLOCATOR(Terrain::TerrainMacroTextureClipmapGenerationPass, AZ::SystemAllocator, 0);
        virtual ~TerrainMacroTextureClipmapGenerationPass() = default;

        static AZ::RPI::Ptr<TerrainMacroTextureClipmapGenerationPass> Create(const AZ::RPI::PassDescriptor& descriptor);

        void BuildInternal() override;
        void InitializeInternal() override;
        void FrameBeginInternal(FramePrepareParams params) override;
        void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

    private:
        TerrainMacroTextureClipmapGenerationPass(const AZ::RPI::PassDescriptor& descriptor);

        //! Update ClipmapData every frame when view point is changed.
        void UpdateClipmapData();

        static constexpr uint32_t ClipmapStackSize = 5;
        static constexpr uint32_t ClipmapSizeWidth = 1024;
        static constexpr uint32_t ClipmapSizeHeight = 1024;

        struct ClipmapData
        {
            //! The 2D xy-plane view position where the main camera is.
            //! 0,1: previous; 2,3: current.
            AZStd::array<float, 4> m_viewPosition;

            // 2D xy-plane world bounds defined by the terrain.
            // 0,1: min; 2,3: max.
            AZStd::array<float, 4> m_worldBounds;

            //! The max range that the clipmap is covering.
            AZStd::array<float, 2> m_maxRenderSize;

            //! The size of a single clipmap.
            AZStd::array<float, 2> m_clipmapSize;

            //! Clipmap centers in normalized UV coordinates [0, 1].
            // 0,1: previous clipmap centers; 2,3: current clipmap centers.
            // They are used for toroidal addressing and may move each frame based on the view point movement.
            // The move distance is scaled differently in each layer.
            AZStd::array<AZStd::array<float, 4>, ClipmapStackSize> m_clipmapCenters;

            //! A list of reciprocal the clipmap scale [s],
            //! where 1 pixel in the current layer of clipmap represents s meters.
            //! Fast lookup list to avoid redundant calculation in shaders.
            AZStd::array<AZStd::array<float, 4>, ClipmapStackSize> m_clipmapScaleInv;
        };

        ClipmapData m_clipmapData;

        AZ::Data::Instance<AZ::RPI::AttachmentImage> m_macroColorClipmaps;

        AZ::RHI::ShaderInputNameIndex m_clipmapDataIndex = "m_clipmapData";
    };
} // namespace Terrain
