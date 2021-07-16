/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RHI.Reflect/Size.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Image/Image.h>

namespace AZ
{
    namespace Render
    {
        class DiffuseProbeGrid;

        using DiffuseProbeGridHandle = AZStd::shared_ptr<DiffuseProbeGrid>;

        enum class DiffuseProbeGridMode : uint8_t
        {
            RealTime,
            Baked,
            AutoSelect
        };

        enum class DiffuseProbeGridTextureNotificationType
        {
            None,
            Ready,
            Error
        };

        struct DiffuseProbeGridTexture
        {
            const AZStd::shared_ptr<AZStd::vector<uint8_t>> m_data;
            RHI::Format m_format;
            RHI::Size m_size;
        };

        static const char* DiffuseProbeGridIrradianceFileName = "Irradiance_lutrgba16.dds";
        static const char* DiffuseProbeGridDistanceFileName = "Distance_lutrg32f.dds";
        static const char* DiffuseProbeGridRelocationFileName = "Relocation_lutrgba16f.dds";
        static const char* DiffuseProbeGridClassificationFileName = "Classification_lutr32f.dds";

        using DiffuseProbeGridBakeTexturesCallback = AZStd::function<void(
            DiffuseProbeGridTexture irradianceTexture,
            DiffuseProbeGridTexture distanceTexture,
            DiffuseProbeGridTexture relocationTexture,
            DiffuseProbeGridTexture classificationTexture)>;

        struct DiffuseProbeGridBakedTextures
        {
            // irradiance and distance images can be used directly 
            Data::Instance<RPI::Image> m_irradianceImage;
            AZStd::string m_irradianceImageRelativePath;

            Data::Instance<RPI::Image> m_distanceImage;
            AZStd::string m_distanceImageRelativePath;

            // relocation and classification images need to be recreated as RW textures
            RHI::ImageDescriptor m_relocationImageDescriptor;
            AZStd::array_view<uint8_t> m_relocationImageData;
            AZStd::string m_relocationImageRelativePath;

            RHI::ImageDescriptor m_classificationImageDescriptor;
            AZStd::array_view<uint8_t> m_classificationImageData;
            AZStd::string m_classificationImageRelativePath;
        };

        // DiffuseProbeGridFeatureProcessorInterface provides an interface to the feature processor for code outside of Atom
        class DiffuseProbeGridFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::DiffuseProbeGridFeatureProcessorInterface, "{6EF4F226-D473-4D50-8884-D407E4D145F4}");

            virtual DiffuseProbeGridHandle AddProbeGrid(const AZ::Transform& transform, const AZ::Vector3& extents, const AZ::Vector3& probeSpacing) = 0;
            virtual void RemoveProbeGrid(DiffuseProbeGridHandle& handle) = 0;
            virtual bool IsValidProbeGridHandle(const DiffuseProbeGridHandle& probeGrid) const = 0;
            virtual bool ValidateExtents(const DiffuseProbeGridHandle& probeGrid, const AZ::Vector3& newExtents) = 0;
            virtual void SetExtents(const DiffuseProbeGridHandle& probeGrid, const AZ::Vector3& extents) = 0;
            virtual void SetTransform(const DiffuseProbeGridHandle& probeGrid, const AZ::Transform& transform) = 0;
            virtual bool ValidateProbeSpacing(const DiffuseProbeGridHandle& probeGrid, const AZ::Vector3& newSpacing) = 0;
            virtual void SetProbeSpacing(const DiffuseProbeGridHandle& probeGrid, const AZ::Vector3& probeSpacing) = 0;
            virtual void SetViewBias(const DiffuseProbeGridHandle& probeGrid, float viewBias) = 0;
            virtual void SetNormalBias(const DiffuseProbeGridHandle& probeGrid, float normalBias) = 0;
            virtual void SetAmbientMultiplier(const DiffuseProbeGridHandle& probeGrid, float ambientMultiplier) = 0;
            virtual void Enable(const DiffuseProbeGridHandle& probeGrid, bool enable) = 0;
            virtual void SetGIShadows(const DiffuseProbeGridHandle& probeGrid, bool giShadows) = 0;
            virtual void SetUseDiffuseIbl(const DiffuseProbeGridHandle& probeGrid, bool useDiffuseIbl) = 0;
            virtual void SetMode(const DiffuseProbeGridHandle& probeGrid, DiffuseProbeGridMode mode) = 0;
            virtual void SetBakedTextures(const DiffuseProbeGridHandle& probeGrid, const DiffuseProbeGridBakedTextures& bakedTextures) = 0;

            virtual void BakeTextures(
                const DiffuseProbeGridHandle& probeGrid,
                DiffuseProbeGridBakeTexturesCallback callback,
                const AZStd::string& irradianceTextureRelativePath,
                const AZStd::string& distanceTextureRelativePath,
                const AZStd::string& relocationTextureRelativePath,
                const AZStd::string& classificationTextureRelativePath) = 0;

            // check for and retrieve a new baked texture asset (does not apply to hot-reloaded assets, only initial bakes)
            virtual bool CheckTextureAssetNotification(
                const AZStd::string& relativePath,
                Data::Asset<RPI::StreamingImageAsset>& outTextureAsset,
                DiffuseProbeGridTextureNotificationType& outNotificationType) = 0;

            virtual bool AreBakedTexturesReferenced(
                const AZStd::string& irradianceTextureRelativePath,
                const AZStd::string& distanceTextureRelativePath,
                const AZStd::string& relocationTextureRelativePath,
                const AZStd::string& classificationTextureRelativePath) = 0;

        };
    } // namespace Render
} // namespace AZ
