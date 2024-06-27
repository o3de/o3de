/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Reflect/Image/Image.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h>
#include <Atom/Feature/SkyBox/SkyboxConstants.h>

namespace AZ
{
    namespace Render
    {
        // default SunParameters is set to radius of earth's sun, distance from sun -> earth, and cos(angularDiameter) of the sun
        struct SunParameters
        {
            float m_radius = PhysicalSunRadius;                          // Sun physical radius, unit is millions of km
            float m_distance = PhysicalSunDistance;                      // Sun distance to planet, unit is millions of km
            float m_cosAngularDiameter = PhysicalSunCosAngularDiameter;  // Cosine angular diameter of the sun, unit is radians
        };

        struct HosekSky
        {
            AZ::Vector3 a;   // Darkening or brightening of the horizon. Negative is brighter relative to the zenith luminance.
            AZ::Vector3 b;   // Smoothness of the gradient that is caused by darkening or brightening of the horizon. Higher values result in a more gradual transition.
            AZ::Vector3 c;   // Added in the extended formula due to the complication arose by anisotropic term, not exist in the original Perez formula.
            AZ::Vector3 d;   // Relative intensity of the area near the sun. Higher values result in higher luminance.
            AZ::Vector3 e;   // The width of the region described above by D is modulated by E. Higher values result in a more gradual transition.
            AZ::Vector3 f;   // Relative intensity of back-scattered light - in other words, the light reflected back from the ground. Higher values result in more reflected light.
            AZ::Vector3 g;   // Relative intensity of the aureole (the area around the sun).
            AZ::Vector3 h;   // Size of the aureole.
            AZ::Vector3 i;   // Smooth gradient around zenith.
            AZ::Vector3 z;   // Absolute luminance at zenith.
        };

        class SkyBoxFeatureProcessor final
            : public SkyBoxFeatureProcessorInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(SkyBoxFeatureProcessor, AZ::SystemAllocator)
            AZ_RTTI(AZ::Render::SkyBoxFeatureProcessor, "{CB7D1F95-2A02-4152-86F1-BB29DC802CF7}", AZ::Render::SkyBoxFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            SkyBoxFeatureProcessor();
            virtual ~SkyBoxFeatureProcessor() = default;

            //! FeatureProcessor
            void Activate() override;
            void Deactivate() override;
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;
            void Render(const FeatureProcessor::RenderPacket& packet) override;

            // SkyBoxFeatureProcessorInterface overrides ...
            void Enable(bool enable) override;
            bool IsEnabled() override;
            void SetSkyboxMode(SkyBoxMode mode) override;
            void SetFogSettings(const SkyBoxFogSettings& fogSettings) override;
            void SetFogEnabled(bool enable) override;
            bool IsFogEnabled() override;
            void SetFogColor(const AZ::Color& color) override;
            void SetFogTopHeight(float topHeight) override;
            void SetFogBottomHeight(float bottomHeight) override;

            void SetCubemapRotationMatrix(AZ::Matrix4x4 matrix) override;
            void SetCubemap(Data::Instance<RPI::Image> cubemap) override;
            void SetCubemapExposure(float exposure) override;

            void SetSunPosition(SunPosition sunPosition) override;
            void SetSunPosition(float azimuth, float altitude) override;
            void SetTurbidity(int turbidity) override;
            void SetSkyIntensity(float intensity, PhotometricUnit unit) override;
            void SetSunIntensity(float intensity, PhotometricUnit unit) override;
            void SetSunRadiusFactor(float factor) override;

        private:
            static constexpr const char* FeatureProcessorName = "SkyBoxFeatureProcessor";

            // Parameters definition written above
            struct PhysicalSkyData
            {
                AZStd::array<float, 4> m_physicalSkyParameterA;
                AZStd::array<float, 4> m_physicalSkyParameterB;
                AZStd::array<float, 4> m_physicalSkyParameterC;
                AZStd::array<float, 4> m_physicalSkyParameterD;
                AZStd::array<float, 4> m_physicalSkyParameterE;
                AZStd::array<float, 4> m_physicalSkyParameterF;
                AZStd::array<float, 4> m_physicalSkyParameterG;
                AZStd::array<float, 4> m_physicalSkyParameterH;
                AZStd::array<float, 4> m_physicalSkyParameterI;
                AZStd::array<float, 4> m_physicalSkyParameterZ;

                AZStd::array<float, 4> m_physicalSkySunParameters;
                AZStd::array<float, 4> m_physicalSkySunDirection;
                AZStd::array<float, 4> m_physicalSkySunRGB;
                AZStd::array<float, 4> m_physicalSkyAndSunIntensity;
            };

            SkyBoxFeatureProcessor(const SkyBoxFeatureProcessor&) = delete;

            void LoadDefaultCubeMap();
            void InitBuffer();
            HosekSky ComputeHosekSky();
            //! Sun color is based on Preetham's paper
            //! https://www.cs.utah.edu/~shirley/papers/sunsky/sunsky.pdf
            AZ::Vector4 ComputeSunRGB();

            //! Sample function for the look up table
            double SampleLUT(const double* dataset, size_t stride, int turbidity, float albedo, float inverseAltitude);
            double EvaluateSpline(const double* spline, size_t stride, double value);
            AZ::Vector3 ComputeSpherical(float altitude, float azimuth) const;

            //! Irradiance to CIE XYZ
            //! http://jcgt.org/published/0002/02/01/paper.pdf
            AZ::Vector3 EvaluateCIEXYZ(int lambda);

            //--------------------------------------------------------------------------
            //! Evaluates the Perez (extended) formula to find the luminance, in RGB, of the sky
            //! https://cgg.mff.cuni.cz/projects/SkylightModelling/HosekWilkie_SkylightModel_SIGGRAPH2012_Preprint_lowres.pdf
            //!
            //! \param cosTheta Viewing angle on Y axis in radians
            //! \param gamma    Angle, in radians, between view direction and sun direction
            //! \param cosGamma Float dot product of view direction and sun direction
            //! \param hosek    Sky model parameters
            //--------------------------------------------------------------------------
            AZ::Vector3 EvaluateHosek(float cosTheta, float gamma, float cosGamma,const HosekSky& hosek);

            AZ::Vector3 Vector3Pow(AZ::Vector3 a, AZ::Vector3 b);
            AZ::Vector3 Vector3Exp(AZ::Vector3 a);

            Data::Instance<RPI::Buffer> m_buffer;
            PhysicalSkyData m_physicalSkyData;

            RHI::ShaderInputNameIndex m_skyboxEnableIndex = "m_enable";
            RHI::ShaderInputNameIndex m_physicalSkyBufferIndex = "m_physicalSkyData";
            RHI::ShaderInputNameIndex m_physicalSkyIndex = "m_physicalSky";
            RHI::ShaderInputNameIndex m_cubemapIndex = "m_skyboxCubemap";
            RHI::ShaderInputNameIndex m_cubemapRotationMatrixIndex = "m_cubemapRotationMatrix";
            RHI::ShaderInputNameIndex m_cubemapExposureIndex = "m_cubemapExposure";
            RHI::ShaderInputNameIndex m_fogEnableIndex = "m_fogEnable";
            RHI::ShaderInputNameIndex m_fogColorIndex = "m_fogColor";
            RHI::ShaderInputNameIndex m_fogTopHeightIndex = "m_fogTopHeight";
            RHI::ShaderInputNameIndex m_fogBottomHeightIndex = "m_fogBottomHeight";

            bool m_skyNeedUpdate = true;
            bool m_sunNeedUpdate = true;
            bool m_mapBuffer = true;

            bool m_enable = false;
            SkyBoxMode m_skyboxMode = SkyBoxMode::None;
            SkyBoxFogSettings m_fogSettings;
            Data::Instance<RPI::ShaderResourceGroup> m_sceneSrg = nullptr;

            Data::Instance<RPI::Image> m_cubemapTexture = nullptr;
            float m_cubemapExposure = 0;
            AZ::Matrix4x4 m_cubemapRotationMatrix = AZ::Matrix4x4::CreateIdentity();

            int m_turbidity = 1;            // A measure of the aerosol content in the air, it is not linearly interpolated as a float due to numerical instability, but rather treated as integer steps
            SunPosition m_sunPosition;      // Sun position in the Sky( Azimuth, Altitude)
            SunParameters m_sunParameters;  // Sun physical parameters
            AZ::Vector3 m_sunDirection;
            PhotometricValue m_sunIntensity;
            PhotometricValue m_skyIntensity;

            Data::Instance<RPI::Image> m_defaultCubemapTexture;
        };
    } // namespace Render
} // namespace AZ
