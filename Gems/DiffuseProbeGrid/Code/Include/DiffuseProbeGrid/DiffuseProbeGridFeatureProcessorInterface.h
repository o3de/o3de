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

        enum class DiffuseProbeGridNumRaysPerProbe : uint32_t
        {
            NumRaysPerProbe_144,
            NumRaysPerProbe_288,
            NumRaysPerProbe_432,
            NumRaysPerProbe_576,
            NumRaysPerProbe_720,
            NumRaysPerProbe_864,
            NumRaysPerProbe_1008,
        
            Count
        };

        struct DiffuseProbeGridNumRaysPerProbeEntry
        {
            DiffuseProbeGridNumRaysPerProbe m_enum;
            uint32_t m_rayCount;
            const char* m_supervariant;
            uint32_t m_index;
        };

        #define DECLARE_DiffuseProbeGridNumRaysPerProbeEntry(numRaysPerProbe) \
            { DiffuseProbeGridNumRaysPerProbe::NumRaysPerProbe_##numRaysPerProbe, numRaysPerProbe, "NumRaysPerProbe" #numRaysPerProbe, aznumeric_cast<uint32_t>(DiffuseProbeGridNumRaysPerProbe::NumRaysPerProbe_##numRaysPerProbe) }

        static const DiffuseProbeGridNumRaysPerProbeEntry DiffuseProbeGridNumRaysPerProbeArray[aznumeric_cast<uint32_t>(DiffuseProbeGridNumRaysPerProbe::Count)] =
        {
            DECLARE_DiffuseProbeGridNumRaysPerProbeEntry(144),
            DECLARE_DiffuseProbeGridNumRaysPerProbeEntry(288),
            DECLARE_DiffuseProbeGridNumRaysPerProbeEntry(432),
            DECLARE_DiffuseProbeGridNumRaysPerProbeEntry(576),
            DECLARE_DiffuseProbeGridNumRaysPerProbeEntry(720),
            DECLARE_DiffuseProbeGridNumRaysPerProbeEntry(864),
            DECLARE_DiffuseProbeGridNumRaysPerProbeEntry(1008)
        };
        static const uint32_t DiffuseProbeGridNumRaysPerProbeArraySize = RHI::ArraySize(DiffuseProbeGridNumRaysPerProbeArray);

        static constexpr const char* DiffuseProbeGridIrradianceFileName = "Irradiance_lutrgba16f.dds";
        static constexpr const char* DiffuseProbeGridDistanceFileName = "Distance_lutrg32f.dds";
        static constexpr const char* DiffuseProbeGridProbeDataFileName = "ProbeData_lutrgba16f.dds";

        enum class DiffuseProbeGridTransparencyMode : uint8_t
        {
            Full,
            ClosestOnly,
            None
        };

        constexpr float DefaultDiffuseProbeGridSpacing = 2.0f;
        constexpr float DefaultDiffuseProbeGridExtents = 8.0f;
        constexpr float DefaultDiffuseProbeGridAmbientMultiplier = 1.0f;
        constexpr float DefaultDiffuseProbeGridEmissiveMultiplier = 1.0f;
        constexpr float DefaultDiffuseProbeGridViewBias = 0.2f;
        constexpr float DefaultDiffuseProbeGridNormalBias = 0.1f;
        constexpr float DefaultVisualizationSphereRadius = 0.5f;
        constexpr DiffuseProbeGridNumRaysPerProbe DefaultDiffuseProbeGridNumRaysPerProbe = DiffuseProbeGridNumRaysPerProbe::NumRaysPerProbe_288;
        constexpr DiffuseProbeGridTransparencyMode DefaultDiffuseProbeGridTransparencyMode = DiffuseProbeGridTransparencyMode::ClosestOnly;

        using DiffuseProbeGridBakeTexturesCallback = AZStd::function<void(
            DiffuseProbeGridTexture irradianceTexture,
            DiffuseProbeGridTexture distanceTexture,
            DiffuseProbeGridTexture probeDataTexture)>;

        struct DiffuseProbeGridBakedTextures
        {
            // irradiance and distance images can be used directly 
            Data::Instance<RPI::Image> m_irradianceImage;
            AZStd::string m_irradianceImageRelativePath;

            Data::Instance<RPI::Image> m_distanceImage;
            AZStd::string m_distanceImageRelativePath;

            Data::Instance<RPI::Image> m_probeDataImage;
            AZStd::string m_probeDataImageRelativePath;
        };

        // DiffuseProbeGridFeatureProcessorInterface provides an interface to the feature processor for code outside of Atom
        class DiffuseProbeGridFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::DiffuseProbeGridFeatureProcessorInterface, "{6EF4F226-D473-4D50-8884-D407E4D145F4}", AZ::RPI::FeatureProcessor);

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
            virtual void SetNumRaysPerProbe(const DiffuseProbeGridHandle& probeGrid, DiffuseProbeGridNumRaysPerProbe numRaysPerProbe) = 0;
            virtual void SetAmbientMultiplier(const DiffuseProbeGridHandle& probeGrid, float ambientMultiplier) = 0;
            virtual void Enable(const DiffuseProbeGridHandle& probeGrid, bool enable) = 0;
            virtual void SetGIShadows(const DiffuseProbeGridHandle& probeGrid, bool giShadows) = 0;
            virtual void SetUseDiffuseIbl(const DiffuseProbeGridHandle& probeGrid, bool useDiffuseIbl) = 0;
            virtual void SetMode(const DiffuseProbeGridHandle& probeGrid, DiffuseProbeGridMode mode) = 0;
            virtual void SetScrolling(const DiffuseProbeGridHandle& probeGrid, bool scrolling) = 0;
            virtual void SetEdgeBlendIbl(const DiffuseProbeGridHandle& probeGrid, bool edgeBlendIbl) = 0;
            virtual void SetFrameUpdateCount(const DiffuseProbeGridHandle& probeGrid, uint32_t frameUpdateCount) = 0;
            virtual void SetTransparencyMode(const DiffuseProbeGridHandle& probeGrid, DiffuseProbeGridTransparencyMode transparencyMode) = 0;
            virtual void SetEmissiveMultiplier(const DiffuseProbeGridHandle& probeGrid, float emissiveMultiplier) = 0;
            virtual void SetBakedTextures(const DiffuseProbeGridHandle& probeGrid, const DiffuseProbeGridBakedTextures& bakedTextures) = 0;
            virtual void SetVisualizationEnabled(const DiffuseProbeGridHandle& probeGrid, bool visualizationEnabled) = 0;
            virtual void SetVisualizationShowInactiveProbes(const DiffuseProbeGridHandle& probeGrid, bool visualizationShowInactiveProbes) = 0;
            virtual void SetVisualizationSphereRadius(const DiffuseProbeGridHandle& probeGrid, float visualizationSphereRadius) = 0;

            virtual bool CanBakeTextures() = 0;
            virtual void BakeTextures(
                const DiffuseProbeGridHandle& probeGrid,
                DiffuseProbeGridBakeTexturesCallback callback,
                const AZStd::string& irradianceTextureRelativePath,
                const AZStd::string& distanceTextureRelativePath,
                const AZStd::string& probeDataTextureRelativePath) = 0;

            // check for and retrieve a new baked texture asset (does not apply to hot-reloaded assets, only initial bakes)
            virtual bool CheckTextureAssetNotification(
                const AZStd::string& relativePath,
                Data::Asset<RPI::StreamingImageAsset>& outTextureAsset,
                DiffuseProbeGridTextureNotificationType& outNotificationType) = 0;

            virtual bool AreBakedTexturesReferenced(
                const AZStd::string& irradianceTextureRelativePath,
                const AZStd::string& distanceTextureRelativePath,
                const AZStd::string& probeDataTextureRelativePath) = 0;

        };
    } // namespace Render
} // namespace AZ
