/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>

#include <Atom/Feature/DisplayMapper/DisplayMapperFeatureProcessorInterface.h>
#include <ACES/Aces.h>
#include <Atom/Feature/DisplayMapper/DisplayMapperConfigurationDescriptor.h>

#include <Atom/RHI/DeviceImageView.h>
#include <Atom/RHI/ImagePool.h>

#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>

namespace AZ
{
    namespace Render
    {
        /**
         * The ACES display mapper parameters.
         * These parameters are input to the display mapper shader on the DisplayMapperPass.
         */
        struct DisplayMapperParameters
        {
            // Bit flag for control the ODT shader behavior
            int m_OutputDisplayTransformFlags;
            // The ODT output mode specify by OutputDeviceTransformMode
            int m_OutputDisplayTransformMode;
            // Reference white and black luminance values
            float m_cinemaLimits[2];
            // Color transformation matrix from XYZ to the display's color primaries
            SegmentedSplineParamsC9 m_acesSplineParams;
            // ACES spline parameters
            Matrix3x3 m_XYZtoDisplayPrimaries;
            // Gamma adjustment to be applied to compensate for the condition of the viewing environment.
            // Note that ACES uses a value of 0.9811 for adjusting from dark to dim surrounding.
            float m_surroundGamma;
            // Optional gamma value that is applied as basic gamma curve OETF
            float m_gamma;
        };

        /**
         *  The ACES display mapper feature processor.
         *  This class create display mapper shader input parameters by using
         *  the ACES reference implementation.
         */
        class AcesDisplayMapperFeatureProcessor final
            : public DisplayMapperFeatureProcessorInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(AcesDisplayMapperFeatureProcessor, AZ::SystemAllocator)
            enum OutputDeviceTransformFlags
            {
                AlterSurround = 0x1, // Apply gamma adjustment to compensate for dim surround
                ApplyDesaturation = 0x2, // Apply desaturation to compensate for luminance difference
                ApplyCATD60toD65 = 0x4, // Apply Color appearance transform (CAT) from ACES white point to assumed observer adapted white point
            };

            AZ_RTTI(AZ::Render::AcesDisplayMapperFeatureProcessor, "{995C2B93-8B08-4313-89B0-02394F90F1B8}", AZ::Render::DisplayMapperFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            AcesDisplayMapperFeatureProcessor() = default;
            virtual ~AcesDisplayMapperFeatureProcessor() = default;

            // FeatureProcessor
            void Activate() override;
            void Deactivate() override;
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;
            void Render(const FeatureProcessor::RenderPacket& packet) override;

            static OutputDeviceTransformType GetOutputDeviceTransformType(RHI::Format bufferFormat);
            static void GetAcesDisplayMapperParameters(DisplayMapperParameters* displayMapperParameters, OutputDeviceTransformType odtType);
            static ShaperParams GetShaperParameters(ShaperPresetType shaperPreset, float customMinEv = 0.0f, float customMaxEv = 0.0f);
            static void GetDefaultDisplayMapperConfiguration(DisplayMapperConfigurationDescriptor& config);

            // DisplayMapperFeatureProcessorInteface overrides...
            void GetOwnedLut(DisplayMapperLut& displayMapperLut, const AZ::Name& lutName) override;
            void GetDisplayMapperLut(DisplayMapperLut& displayMapperLut) override;
            void GetLutFromAssetLocation(DisplayMapperAssetLut& displayMapperAssetLut, const AZStd::string& assetPath) override;
            void GetLutFromAssetId(DisplayMapperAssetLut& displayMapperAssetLut, const AZ::Data::AssetId) override;
            void RegisterDisplayMapperConfiguration(const DisplayMapperConfigurationDescriptor& config) override;
            void UnregisterDisplayMapperConfiguration() override;
            const DisplayMapperConfigurationDescriptor* GetDisplayMapperConfiguration() override;

        private:
            AcesDisplayMapperFeatureProcessor(const AcesDisplayMapperFeatureProcessor&) = delete;
            static void ApplyLdrOdtParameters(DisplayMapperParameters* pOutParameters);
            static void ApplyHdrOdtParameters(DisplayMapperParameters* pOutParameters, const OutputDeviceTransformType& odtType);

            enum OutputDeviceTransformMode {
                Srgb = 0,
                PerceptualQuantizer,
                Ldr,
            };

            static constexpr const char* FeatureProcessorName = "AcesDisplayMapperFeatureProcessor";

            static const int ImagePoolBudget = 1 << 20; // 1 Megabyte

            // LUTs that are baked through shaders
            RHI::Ptr<RHI::ImagePool> m_displayMapperImagePool;
            AZStd::unordered_map<AZ::Name, DisplayMapperLut>            m_ownedLuts;
            // LUTs loaded from assets
            AZStd::unordered_map<AZStd::string, DisplayMapperAssetLut>  m_assetLuts;
            // DisplayMapper configurations per scene
            AZStd::optional<DisplayMapperConfigurationDescriptor> m_displayMapperConfiguration;

            void InitializeImagePool();
            // Initialize a LUT image with the given name.
            void InitializeLutImage(const AZ::Name& lutName);
        };
    } // namespace Render
} // namespace AZ
