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
#include <ACES/Aces.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>
#include <DisplayMapper/DisplayMapperFullScreenPass.h>

namespace AZ
{
    namespace Render
    {
        /**
         * This pass is used to apply a shaper function and lookup table to the input image attachment.
         * The coordinates on the lookup table are computed by taking the color values of the input image
         * and translating it via a shaper function.
         * Right now, preset shaper functions based of ACES 1.0.3 are being used.
         */
        class ApplyShaperLookupTablePass
            : public DisplayMapperFullScreenPass
        {
            AZ_RPI_PASS(AcesOutputTransformLutPass);

        public:
            AZ_RTTI(ApplyShaperLookupTablePass, "{5C76BE12-307A-4595-91CE-AAA13ED6368C}", DisplayMapperFullScreenPass);
            AZ_CLASS_ALLOCATOR(ApplyShaperLookupTablePass, SystemAllocator);
            virtual ~ApplyShaperLookupTablePass();

            /// Creates a AcesOutputTransformLutPass
            static RPI::Ptr<ApplyShaperLookupTablePass> Create(const RPI::PassDescriptor& descriptor);

            void SetShaperParameters(const ShaperParams& shaperParams);
            void SetLutAssetId(const AZ::Data::AssetId& assetId);
            const AZ::Data::AssetId& GetLutAssetId() const;

        protected:
            explicit ApplyShaperLookupTablePass(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void InitializeInternal() override;

            RHI::ShaderInputImageIndex m_shaderInputLutImageIndex;

            RHI::ShaderInputConstantIndex m_shaderShaperTypeIndex;
            RHI::ShaderInputConstantIndex m_shaderShaperBiasIndex;
            RHI::ShaderInputConstantIndex m_shaderShaperScaleIndex;

            void SetupFrameGraphDependenciesCommon(RHI::FrameGraphInterface frameGraph);
            void CompileResourcesCommon(const RHI::FrameGraphCompileContext& context);

        private:

            // Scope producer functions...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            void UpdateShaperSrg();
            void AcquireLutImage();
            void ReleaseLutImage();

            DisplayMapperAssetLut       m_lutResource;
            AZ::Data::AssetId           m_lutAssetId;

            ShaperParams m_shaperParams;

            bool m_needToReloadLut = true;
        };
    }   // namespace Render
}   // namespace AZ
