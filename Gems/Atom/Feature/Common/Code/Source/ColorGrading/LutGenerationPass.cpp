/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma optimize("", off)

#include <ColorGrading/LutGenerationPass.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>

#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

namespace AZ
{
    namespace Render
    {

        RPI::Ptr<LutGenerationPass> LutGenerationPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<LutGenerationPass> pass = aznew LutGenerationPass(descriptor);
            return AZStd::move(pass);
        }

         LutGenerationPass::LutGenerationPass(const RPI::PassDescriptor& descriptor)
            : HDRColorGradingPass(descriptor)
        {
        }

        void LutGenerationPass::BuildInternal()
        {
            auto scene = GetScene();
            if (scene)
            {
                AcesDisplayMapperFeatureProcessor* dmfp = scene->GetFeatureProcessor<AcesDisplayMapperFeatureProcessor>();
                if (dmfp)
                {
                    // load the image assets
                    auto assetId = AZ::RPI::AssetUtils::GetAssetIdForProductPath(
                        "lookuptables/lut_identitylinear_16x16x16.azasset", AZ::RPI::AssetUtils::TraceLevel::Error);
                    AZ_Assert(assetId.IsValid(), "LUT Asset is not valid.");
                    dmfp->GetLutFromAssetId(m_colorGradingLut, assetId);

                    // set srg image index
                    m_shaderResourceGroup->SetImageView(m_identityLut16x16x16Index, m_colorGradingLut.m_lutStreamingImage->GetImageView());

                    // Get the output image attachment
                    RPI::Ptr<RPI::PassAttachment> attachment = FindOwnedAttachment(Name{ "ColorGradingLut" });
                    RHI::ImageDescriptor& imageDescriptor = attachment->m_descriptor.m_image;
                    imageDescriptor.m_size = RHI::Size(
                        m_colorGradingLut.m_lutStreamingImage->GetDescriptor().m_size.m_width*m_colorGradingLut.m_lutStreamingImage->GetDescriptor().m_size.m_width,
                        m_colorGradingLut.m_lutStreamingImage->GetDescriptor().m_size.m_height,
                        1);
                }
            }

            HDRColorGradingPass::BuildInternal();
        }

        void LutGenerationPass::InitializeInternal()
        {
            HDRColorGradingPass::InitializeInternal();

            m_identityLut16x16x16Index.Reset();
            m_identityLut32x32x32Index.Reset();
            m_identityLut64x64x64Index.Reset();

            //// load the image assets
            //DisplayMapperAssetLut m_colorGradingLut;
            //auto assetId = AZ::RPI::AssetUtils::GetAssetIdForProductPath("lookuptables/lut_identitylinear_16x16x16.azasset", AZ::RPI::AssetUtils::TraceLevel::Error);
            //AcesDisplayMapperFeatureProcessor* dmfp = GetScene()->GetFeatureProcessor<AcesDisplayMapperFeatureProcessor>();
            //dmfp->GetLutFromAssetId(m_colorGradingLut, assetId);

            //// set srg image index
            //m_shaderResourceGroup->SetImageView(m_identityLut16x16x16Index, m_colorGradingLut.m_lutStreamingImage->GetImageView());
        }

        void LutGenerationPass::FrameBeginInternal(FramePrepareParams params)
        {
            HDRColorGradingPass::FrameBeginInternal(params);
        }

        void LutGenerationPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            HDRColorGradingPass::BuildCommandListInternal(context);
        }

        bool LutGenerationPass::IsEnabled() const
        {
            //const auto* colorGradingSettings = GetHDRColorGradingSettings();
            //return colorGradingSettings ? colorGradingSettings->GetGenerateLut() : false;
            return true;
        }
    } // namespace Render
} // namespace AZ

#pragma optimize("", on)
