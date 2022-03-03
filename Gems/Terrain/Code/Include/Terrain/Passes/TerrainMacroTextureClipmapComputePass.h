/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Reflect/Pass/ComputePassData.h>
#include <Atom/RPI.Reflect/Pass/PassDescriptor.h>
#include <AZCore/Math/Vector4.h>
#include <AZCore/Math/Vector3.h>

namespace Terrain
{
    class TerrainMacroTextureClipmapGenerationPass
        : public AZ::RPI::ComputePass
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

        void CreateClipmaps();
        void UpdateClipmapData();

        static constexpr uint32_t ClipmapStackSize = 4;

        AZStd::array<AZ::Vector4, ClipmapStackSize + 1> m_previousClipmapCenters; // +1 for the first layer of the pyramid
        AZStd::array<AZ::Vector4, ClipmapStackSize + 1> m_currentClipmapCenters;

        AZStd::array<AZ::Data::Instance<AZ::RPI::AttachmentImage>, ClipmapStackSize> m_clipmapStacks;
        AZ::Data::Instance<AZ::RPI::AttachmentImage> m_clipmapPyramid;

        AZ::Vector3 m_previousViewPosition;
        AZ::Vector3 m_currentViewPosition;

        uint32_t m_fullUpdateFlag = 0;

        AZ::RHI::ShaderInputNameIndex m_colorClipmapStackIndex = "m_macroColorClipmaps";
        AZ::RHI::ShaderInputNameIndex m_colorClipmapPyramidIndex = "m_macroColorPyramid";

        AZ::RHI::ShaderInputNameIndex m_currentViewPositionIndex = "m_currentViewPosition";
        AZ::RHI::ShaderInputNameIndex m_previousViewPositionIndex = "m_previousViewPosition";
        AZ::RHI::ShaderInputNameIndex m_worldBoundsMinIndex = "m_worldBoundsMin";
        AZ::RHI::ShaderInputNameIndex m_worldBoundsMaxIndex = "m_worldBoundsMax";
        AZ::RHI::ShaderInputNameIndex m_currentClipmapCentersIndex = "m_currentClipmapCenters";
        AZ::RHI::ShaderInputNameIndex m_previousClipmapCentersIndex = "m_previousClipmapCenters";
        AZ::RHI::ShaderInputNameIndex m_fullUpdateFlagIndex = "m_fullUpdateFlag";
    };
} // namespace AZ::Render
