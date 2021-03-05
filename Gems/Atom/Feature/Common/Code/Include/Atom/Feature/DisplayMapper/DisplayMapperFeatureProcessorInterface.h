/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>

#include <Atom/Feature/DisplayMapper/DisplayMapperConfigurationDescriptor.h>

#include <Atom/RHI/ImagePool.h>
#include <Atom/RHI/ImageView.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>

namespace AZ
{
    namespace Render
    {
        struct DisplayMapperLut
        {
            RHI::Ptr<RHI::Image>        m_lutImage;
            RHI::Ptr<RHI::ImageView>    m_lutImageView;
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
            AZ_RTTI(AZ::Render::DisplayMapperFeatureProcessorInterface, "{FA57793A-1C7B-4B44-88C4-02AA431C468F}", FeatureProcessor);

            virtual void GetOwnedLut(DisplayMapperLut& displayMapperLut, const AZ::Name& lutName) = 0;
            virtual void GetDisplayMapperLut(DisplayMapperLut& displayMapperLut) = 0;
            virtual void GetLutFromAssetLocation(DisplayMapperAssetLut& displayMapperAssetLut, const AZStd::string& assetPath) = 0;
            virtual void GetLutFromAssetId(DisplayMapperAssetLut& displayMapperAssetLut, const AZ::Data::AssetId) = 0;
            virtual void RegisterDisplayMapperConfiguration(const DisplayMapperConfigurationDescriptor& config) = 0;
            virtual DisplayMapperConfigurationDescriptor GetDisplayMapperConfiguration() = 0;
        };
    } // namespace Render
} // namespace AZ
