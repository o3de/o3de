/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>

#include <Atom/Feature/DisplayMapper/DisplayMapperConfigurationDescriptor.h>

#include <Atom/RHI/ImagePool.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>

namespace AZ
{
    namespace Render
    {
        struct DisplayMapperLut
        {
            RHI::Ptr<RHI::Image> m_lutImage;
            RHI::Ptr<RHI::ImageView> m_lutImageView;
            RHI::ImageViewDescriptor    m_lutImageViewDescriptor = {};
        };

        struct DisplayMapperAssetLut
        {
            Data::Instance<RPI::StreamingImage>     m_lutStreamingImage;
        };

        //! The display mapper feature processor interface for setting and retreiving tonemapping settings, and handing LUTs.
        class DisplayMapperFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::DisplayMapperFeatureProcessorInterface, "{FA57793A-1C7B-4B44-88C4-02AA431C468F}", AZ::RPI::FeatureProcessor);

            virtual void GetOwnedLut(DisplayMapperLut& displayMapperLut, const AZ::Name& lutName) = 0;
            virtual void GetDisplayMapperLut(DisplayMapperLut& displayMapperLut) = 0;
            virtual void GetLutFromAssetLocation(DisplayMapperAssetLut& displayMapperAssetLut, const AZStd::string& assetPath) = 0;
            virtual void GetLutFromAssetId(DisplayMapperAssetLut& displayMapperAssetLut, const AZ::Data::AssetId) = 0;
            virtual void RegisterDisplayMapperConfiguration(const DisplayMapperConfigurationDescriptor& config) = 0;
            virtual void UnregisterDisplayMapperConfiguration() = 0;
            virtual const DisplayMapperConfigurationDescriptor* GetDisplayMapperConfiguration() = 0;
        };
    } // namespace Render
} // namespace AZ
