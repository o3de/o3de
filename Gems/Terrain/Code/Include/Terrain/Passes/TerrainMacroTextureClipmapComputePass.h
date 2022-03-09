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

        struct ClipmapData
        {
            AZ::Vector3 m_previousViewPosition;
            AZ::Vector3 m_currentViewPosition;

            AZ::Vector3 m_worldBoundsMin;
            AZ::Vector3 m_worldBoundsMax;

            AZStd::array<float, 3> m_maxRenderSize;

            uint32_t m_fullUpdateFlag = 0;

            AZ::Vector4 m_clipmapCenters[ClipmapStackSize + 1]; // +1 for the first layer of the pyramid

            void SetPreviousClipmapCenter(const AZ::Vector2& clipmapCenter, uint32_t clipmapLevel);
            void SetCurrentClipmapCenter(const AZ::Vector2& clipmapCenter, uint32_t clipmapLevel);

            void SetMaxRenderSize(const AZ::Vector3& maxRenderSize);
        };

        ClipmapData m_clipmapData;

        bool m_clearFullUpdateFlag = true;

        AZStd::array<AZ::Data::Instance<AZ::RPI::AttachmentImage>, ClipmapStackSize> m_colorClipmapStacks;
        AZ::Data::Instance<AZ::RPI::AttachmentImage> m_colorClipmapPyramid;
        AZStd::array<AZ::Data::Instance<AZ::RPI::AttachmentImage>, ClipmapStackSize> m_normalClipmapStacks;
        AZ::Data::Instance<AZ::RPI::AttachmentImage> m_normalClipmapPyramid;

        AZ::RHI::ShaderInputNameIndex m_colorClipmapStackIndex = "m_macroColorClipmaps";
        AZ::RHI::ShaderInputNameIndex m_colorClipmapPyramidIndex = "m_macroColorPyramid";
        AZ::RHI::ShaderInputNameIndex m_normalClipmapStackIndex = "m_macroNormalClipmaps";
        AZ::RHI::ShaderInputNameIndex m_normalClipmapPyramidIndex = "m_macroNormalPyramid";

        AZ::RHI::ShaderInputNameIndex m_clipmapDataIndex = "m_clipmapData";
    };
} // namespace AZ::Render
