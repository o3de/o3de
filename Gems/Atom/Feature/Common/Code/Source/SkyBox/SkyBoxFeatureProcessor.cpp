/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkyBox/SkyBoxFeatureProcessor.h>

#include <AzCore/Debug/EventTrace.h>

#include <AzFramework/Asset/AssetSystemBus.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Atom/RPI.Public/ColorManagement/TransformColor.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

#include <Atom/Feature/SkyBox/SkyBoxLUT.h>

namespace AZ
{
    namespace Render
    {
        const double* physicalSkyLUTRGB[] =
        {
            PhysicalSkyLUT::RGB1,
            PhysicalSkyLUT::RGB2,
            PhysicalSkyLUT::RGB3
        };

        const double* physicalSkyLUTRGBRad[] =
        {
            PhysicalSkyLUT::RGBRad1,
            PhysicalSkyLUT::RGBRad2,
            PhysicalSkyLUT::RGBRad3
        };

        void SkyBoxFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<SkyBoxFeatureProcessor, FeatureProcessor>()
                    ->Version(1);
            }
        }

        SkyBoxFeatureProcessor::SkyBoxFeatureProcessor()
            :SkyBoxFeatureProcessorInterface()
        {
            m_skyIntensity = PhotometricValue(PhysicalSkyDefaultIntensity, AZ::Color::CreateOne(), PhotometricUnit::Ev100Luminance);
            m_sunIntensity = PhotometricValue(PhysicalSunDefaultIntensity, AZ::Color::CreateOne(), PhotometricUnit::Ev100Luminance);
        }

        void SkyBoxFeatureProcessor::Activate()
        {
            InitBuffer();

            // Load default cubemap
            // This is assigned when the skybox is disabled or removed from the scene to prevent a Vulkan TDR.
            // [GFX-TODO][ATOM-4181] This can be removed after Vulkan is changed to automatically handle this issue.
            LoadDefaultCubeMap();

            m_cubemapTexture = m_defaultCubemapTexture;

            // Find the relevant indices in the scene srg
            m_sceneSrg = GetParentScene()->GetShaderResourceGroup();

            m_skyboxEnableIndex.Reset();
            m_physicalSkyBufferIndex.Reset();
            m_physicalSkyIndex.Reset();
            m_cubemapIndex.Reset();
            m_cubemapRotationMatrixIndex.Reset();
            m_cubemapExposureIndex.Reset();
            m_fogEnableIndex.Reset();
            m_fogColorIndex.Reset();
            m_fogTopHeightIndex.Reset();
            m_fogBottomHeightIndex.Reset();

            if (m_buffer)
            {
                m_sceneSrg->SetBufferView(m_physicalSkyBufferIndex, m_buffer->GetBufferView());
            }
        }

        void SkyBoxFeatureProcessor::Deactivate()
        {
            m_buffer = nullptr;
            m_cubemapTexture = m_defaultCubemapTexture;
            m_sceneSrg = nullptr;
        }

        void SkyBoxFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "SkyBoxFeatureProcessor: Simulate");
            AZ_UNUSED(packet);

            m_sceneSrg->SetConstant(m_skyboxEnableIndex, m_enable);

            if (m_enable)
            {
                switch (m_skyboxMode)
                {
                    case SkyBoxMode::PhysicalSky:
                    {
                         if (m_skyNeedUpdate)
                         {
                             HosekSky skyParameters = ComputeHosekSky();
                             skyParameters.a.StoreToFloat4(m_physicalSkyData.m_physicalSkyParameterA.data());
                             skyParameters.b.StoreToFloat4(m_physicalSkyData.m_physicalSkyParameterB.data());
                             skyParameters.c.StoreToFloat4(m_physicalSkyData.m_physicalSkyParameterC.data());
                             skyParameters.d.StoreToFloat4(m_physicalSkyData.m_physicalSkyParameterD.data());
                             skyParameters.e.StoreToFloat4(m_physicalSkyData.m_physicalSkyParameterE.data());
                             skyParameters.f.StoreToFloat4(m_physicalSkyData.m_physicalSkyParameterF.data());
                             skyParameters.g.StoreToFloat4(m_physicalSkyData.m_physicalSkyParameterG.data());
                             skyParameters.h.StoreToFloat4(m_physicalSkyData.m_physicalSkyParameterH.data());
                             skyParameters.i.StoreToFloat4(m_physicalSkyData.m_physicalSkyParameterI.data());
                             skyParameters.z.StoreToFloat4(m_physicalSkyData.m_physicalSkyParameterZ.data());
                         }

                         if (m_sunNeedUpdate)
                         {
                             AZ::Vector4 sunRGB = ComputeSunRGB();
                             sunRGB.StoreToFloat4(m_physicalSkyData.m_physicalSkySunRGB.data());
                         }

                         if (m_skyNeedUpdate || m_sunNeedUpdate || m_mapBuffer)
                         {
                             m_sunDirection.StoreToFloat4(m_physicalSkyData.m_physicalSkySunDirection.data());

                             m_physicalSkyData.m_physicalSkySunParameters[0] = m_sunParameters.m_radius;
                             m_physicalSkyData.m_physicalSkySunParameters[1] = m_sunParameters.m_distance;
                             m_physicalSkyData.m_physicalSkySunParameters[2] = m_sunParameters.m_cosAngularDiameter;
                             m_physicalSkyData.m_physicalSkySunParameters[3] = 0.0f;

                             m_skyIntensity.ConvertToPhotometricUnit(PhotometricUnit::Nit);
                             m_sunIntensity.ConvertToPhotometricUnit(PhotometricUnit::Nit);
                             AZ::Vector4 artistParams = AZ::Vector4(m_skyIntensity.GetIntensity(), m_sunIntensity.GetIntensity(), 0.0f, 0.0f);
                             artistParams.StoreToFloat4(m_physicalSkyData.m_physicalSkyAndSunIntensity.data());

                             if (m_buffer)
                             {
                                 m_buffer->UpdateData(&m_physicalSkyData, sizeof(PhysicalSkyData));
                             }
                             m_skyNeedUpdate = false;
                             m_sunNeedUpdate = false;
                             m_mapBuffer = false;
                         }

                        m_sceneSrg->SetConstant(m_fogEnableIndex, m_fogSettings.m_enable);
                        if (m_fogSettings.m_enable)
                        {
                            m_sceneSrg->SetConstant(m_fogTopHeightIndex, m_fogSettings.m_topHeight);
                            m_sceneSrg->SetConstant(m_fogBottomHeightIndex, m_fogSettings.m_bottomHeight);
                            m_sceneSrg->SetConstant(m_fogColorIndex, m_fogSettings.m_color);
                        }

                        m_sceneSrg->SetConstant(m_physicalSkyIndex, true);
                        break;
                    }
                    case SkyBoxMode::Cubemap:
                    {
                        // set texture
                        m_sceneSrg->SetImage(m_cubemapIndex, m_cubemapTexture);
                        // set cubemap rotation matrix
                        m_sceneSrg->SetConstant(m_cubemapRotationMatrixIndex, m_cubemapRotationMatrix);
                        // set exposure
                        m_sceneSrg->SetConstant(m_cubemapExposureIndex, m_cubemapExposure);

                        m_sceneSrg->SetConstant(m_physicalSkyIndex, false);
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        void SkyBoxFeatureProcessor::Render(const FeatureProcessor::RenderPacket& packet)
        {
             AZ_TRACE_METHOD();
             AZ_UNUSED(packet);
        }

        void SkyBoxFeatureProcessor::InitBuffer()
        {
            const uint32_t byteCount = sizeof(PhysicalSkyData);

            RPI::CommonBufferDescriptor desc;
            desc.m_poolType = RPI::CommonBufferPoolType::Constant;
            desc.m_bufferName = "SkyboxBuffer";
            desc.m_byteCount = byteCount;
            desc.m_elementSize = byteCount;
            desc.m_bufferData = &m_physicalSkyData;

            m_buffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        }

        void SkyBoxFeatureProcessor::LoadDefaultCubeMap()
        {
            const constexpr char* DefaultCubeMapPath = "textures/default/default_skyboxcm.dds.streamingimage";
            m_defaultCubemapTexture = RPI::LoadStreamingTexture(DefaultCubeMapPath);
            AZ_Assert(m_defaultCubemapTexture, "Failed to load default cubemap");
        }

        void SkyBoxFeatureProcessor::Enable(bool enable)
        {
            m_enable = enable;
        }

        bool SkyBoxFeatureProcessor::IsEnabled()
        {
            return m_enable;
        }

        void SkyBoxFeatureProcessor::SetCubemapRotationMatrix(AZ::Matrix4x4 matrix)
        {
            m_cubemapRotationMatrix = matrix;
        }

        void SkyBoxFeatureProcessor::SetCubemap(Data::Instance<RPI::Image> cubemap)
        {
            m_cubemapTexture = cubemap.get() ? cubemap : m_defaultCubemapTexture;
        }

        void SkyBoxFeatureProcessor::SetCubemapExposure(float exposure)
        {
            m_cubemapExposure = exposure;
        }

        void SkyBoxFeatureProcessor::SetSkyboxMode(SkyBoxMode mode)
        {
            m_skyboxMode = mode;
        }

        void SkyBoxFeatureProcessor::SetFogSettings(const SkyBoxFogSettings& fogSettings)
        {
            m_fogSettings = fogSettings;
        }

        void SkyBoxFeatureProcessor::SetFogEnabled(bool enable)
        {
            m_fogSettings.m_enable = enable;
        }

        bool SkyBoxFeatureProcessor::IsFogEnabled()
        {
            return m_fogSettings.m_enable;
        }

        void SkyBoxFeatureProcessor::SetFogColor(const AZ::Color& color)
        {
            m_fogSettings.m_color = color;
        }

        void SkyBoxFeatureProcessor::SetFogTopHeight(float topHeight)
        {
            m_fogSettings.m_topHeight = topHeight;
        }

        void SkyBoxFeatureProcessor::SetFogBottomHeight(float bottomHeight)
        {
            m_fogSettings.m_bottomHeight = bottomHeight;
        }

        void SkyBoxFeatureProcessor::SetSunPosition(SunPosition sunPosition)
        {
            m_skyNeedUpdate = true;
            m_sunNeedUpdate = true;

            m_sunPosition = sunPosition;
        }

        void SkyBoxFeatureProcessor::SetSunPosition(float azimuth, float altitude)
        {
            m_skyNeedUpdate = true;
            m_sunNeedUpdate = true;

            m_sunPosition.m_azimuth = azimuth;
            m_sunPosition.m_altitude = altitude;
        }

        void SkyBoxFeatureProcessor::SetTurbidity(int turbidity)
        {
            m_skyNeedUpdate = true;
            m_sunNeedUpdate = true;

            m_turbidity = turbidity;
        }
        
        void SkyBoxFeatureProcessor::SetSkyIntensity(float intensity, PhotometricUnit unit)
        {
            m_mapBuffer = true;

            m_skyIntensity.ConvertToPhotometricUnit(unit);
            m_skyIntensity.SetIntensity(intensity);
        }

        void SkyBoxFeatureProcessor::SetSunIntensity(float intensity, PhotometricUnit unit)
        {
            m_mapBuffer = true;

            m_sunIntensity.ConvertToPhotometricUnit(unit);
            m_sunIntensity.SetIntensity(intensity);
        }

        void SkyBoxFeatureProcessor::SetSunRadiusFactor(float factor)
        {
            m_sunNeedUpdate = true;

            m_sunParameters.m_radius = PhysicalSunRadius * factor;
            m_sunParameters.m_cosAngularDiameter = cosf(atanf((m_sunParameters.m_radius * 2.0f) / (m_sunParameters.m_distance * 2.0f)) * 2.0f);
        }

        AZ::Vector3 SkyBoxFeatureProcessor::ComputeSpherical(float altitude, float azimuth) const
        {
            return AZ::Vector3(cos(altitude) * cos(azimuth), sin(altitude), sin(azimuth) * cos(altitude));
        }

        AZ::Vector3 SkyBoxFeatureProcessor::Vector3Pow(AZ::Vector3 a, AZ::Vector3 b)
        {
            AZ::Vector3 result;
            result.SetX(pow(a.GetX(), b.GetX()));
            result.SetY(pow(a.GetY(), b.GetY()));
            result.SetZ(pow(a.GetZ(), b.GetZ()));

            return result;
        }

        AZ::Vector3 SkyBoxFeatureProcessor::Vector3Exp(AZ::Vector3 a)
        {
            AZ::Vector3 result;
            result.SetX(exp(a.GetX()));
            result.SetY(exp(a.GetY()));
            result.SetZ(exp(a.GetZ()));

            return result;
        }

        AZ::Vector3 SkyBoxFeatureProcessor::EvaluateHosek(float cosTheta, float gamma, float cosGamma, const HosekSky& hosek)
        {
            AZ::Vector3  chi = AZ::Vector3(1 + cosGamma * cosGamma) /
                Vector3Pow(AZ::Vector3(1.0f) + hosek.h * hosek.h - 2.0f * cosGamma * hosek.h, AZ::Vector3(1.5f));

            return (AZ::Vector3(1.0f) + hosek.a * Vector3Exp(hosek.b / (cosTheta + 0.01f))) * (hosek.c + hosek.d * Vector3Exp(hosek.e * gamma) + hosek.f * (cosGamma * cosGamma) +
                hosek.g * chi + hosek.i * sqrt(AZ::GetMax(0.0f, cosTheta)));
        }

        double SkyBoxFeatureProcessor::EvaluateSpline(const double* spline, size_t stride, double value)
        {
            return
                1 * pow(1 - value, 5) * spline[0 * stride] +
                5 * pow(1 - value, 4) * pow(value, 1) * spline[1 * stride] +
                10 * pow(1 - value, 3) * pow(value, 2) * spline[2 * stride] +
                10 * pow(1 - value, 2) * pow(value, 3) * spline[3 * stride] +
                5 * pow(1 - value, 1) * pow(value, 4) * spline[4 * stride] +
                1 * pow(value, 5) * spline[5 * stride];
        }

        double SkyBoxFeatureProcessor::SampleLUT(const double* dataset, size_t stride, int turbidity, float albedo, float inverseAltitude)
        {
            // Splines are functions of elevation ^ 1/3
            double elevationK = pow(AZ::GetMax(0.0, 1.0 - inverseAltitude / AZ::Constants::HalfPi), 1.0 / 3.0);

            // table has values for turbidity 1..10
            int turbidity0 = AZ::GetMax(1, AZ::GetMin(turbidity, 10));
            int turbidity1 = AZ::GetMin(turbidity0 + 1, 10);
            double turbidityK = AZ::GetMax(0.0f, AZ::GetMin(static_cast<float>(turbidity - turbidity0), 1.0f));

            const double* datasetA0 = dataset;
            const double* datasetA1 = dataset + stride * 6 * 10;

            double a0t0 = EvaluateSpline(datasetA0 + stride * 6 * (turbidity0 - 1), stride, elevationK);
            double a1t0 = EvaluateSpline(datasetA1 + stride * 6 * (turbidity0 - 1), stride, elevationK);
            double a0t1 = EvaluateSpline(datasetA0 + stride * 6 * (turbidity1 - 1), stride, elevationK);
            double a1t1 = EvaluateSpline(datasetA1 + stride * 6 * (turbidity1 - 1), stride, elevationK);

            return
                a0t0 * (1 - albedo) * (1 - turbidityK) +
                a1t0 * albedo * (1 - turbidityK) +
                a0t1 * (1 - albedo) * turbidityK +
                a1t1 * albedo * turbidityK;
        }

        HosekSky SkyBoxFeatureProcessor::ComputeHosekSky()
        {
            // Valid turbidity values are in the range 1..10
            m_turbidity = AZ::GetMax(AZ::GetMin(m_turbidity, 10), 1); // to avoid silent crash

            AZ::Vector3 a, b, c, d, e, f, g, h, i, z;
            HosekSky result;
            float inverseAltitude = AZ::Constants::HalfPi - m_sunPosition.m_altitude;

            // Currently, we don't have an easy way to get this ground albedo value, so it's hard coded at zero
            float albedo = 0.0f;

            // Fill each 3 component vector of the parameters with data from the dataset
            for (int it = 0; it < 3; ++it)
            {
                a.SetElement(it, static_cast<float>(SampleLUT(physicalSkyLUTRGB[it] + 0, 9, m_turbidity, albedo, inverseAltitude)));
                b.SetElement(it, static_cast<float>(SampleLUT(physicalSkyLUTRGB[it] + 1, 9, m_turbidity, albedo, inverseAltitude)));
                c.SetElement(it, static_cast<float>(SampleLUT(physicalSkyLUTRGB[it] + 2, 9, m_turbidity, albedo, inverseAltitude)));
                d.SetElement(it, static_cast<float>(SampleLUT(physicalSkyLUTRGB[it] + 3, 9, m_turbidity, albedo, inverseAltitude)));
                e.SetElement(it, static_cast<float>(SampleLUT(physicalSkyLUTRGB[it] + 4, 9, m_turbidity, albedo, inverseAltitude)));
                f.SetElement(it, static_cast<float>(SampleLUT(physicalSkyLUTRGB[it] + 5, 9, m_turbidity, albedo, inverseAltitude)));
                g.SetElement(it, static_cast<float>(SampleLUT(physicalSkyLUTRGB[it] + 6, 9, m_turbidity, albedo, inverseAltitude)));

                // H and I are swapped in dataset
                h.SetElement(it, static_cast<float>(SampleLUT(physicalSkyLUTRGB[it] + 8, 9, m_turbidity, albedo, inverseAltitude)));
                i.SetElement(it, static_cast<float>(SampleLUT(physicalSkyLUTRGB[it] + 7, 9, m_turbidity, albedo, inverseAltitude)));

                z.SetElement(it, static_cast<float>(SampleLUT(physicalSkyLUTRGBRad[it], 1, m_turbidity, albedo, inverseAltitude)));
            }

            m_sunDirection = ComputeSpherical(m_sunPosition.m_altitude, m_sunPosition.m_azimuth);

            // In the following block of code we get a "normalized" value representing sun altitude angle
            float sunAmount = fmodf((m_sunDirection.GetY() / AZ::Constants::HalfPi), 4.0f);

            if (sunAmount > 2.0)
            {
                sunAmount = 0.0;
            }
            else if (sunAmount > 1.0)
            {
                sunAmount = 2.0f - sunAmount;
            }
            else if (sunAmount < -1.0)
            {
                sunAmount = -2.0f - sunAmount;
            }

            float normalizedSunY = 0.6f + 0.45f * sunAmount;

            result = { a, b, c, d, e, f, g, h, i, z };
            AZ::Vector3 S = EvaluateHosek(static_cast<float>(cos(inverseAltitude)), 0.0f, 1.0f, result) * z;

            // dividing z by the luminance of S
            z /= S.Dot(AZ::Vector3(0.2126, 0.7152, 0.0722));
            z *= normalizedSunY;

            result.z = z;
            return result;
        }

        AZ::Vector4 SkyBoxFeatureProcessor::ComputeSunRGB()
        {
            AZ::Vector4 result = AZ::Vector4(0.0f);

            // Relative air mass, in this case, means that zenith = 1
            const float inverseAlt = AZ::Constants::HalfPi - m_sunPosition.m_altitude;
            const float relativeAirMass = 1.0f / (static_cast<float>(cos(inverseAlt)) + 0.15f / static_cast<float>(pow(93.885f - AZ::RadToDeg(inverseAlt), 1.253f)));

            // ratio of small to large particle sizes (0:4, usually 1.3)
            const float alpha = 1.3f;

            // amount of aerosols present
            const float beta = 0.04608f * static_cast<float>(m_turbidity) - 0.04586f;

            // amount of ozone in cm(NTP)
            const float ozoneL = 0.35f; // centimeters

            // precipitable water vapor in centimeters
            const float w = 2.0f; // centimeters

            const float solidAngle = AZ::Constants::TwoPi * (1.0f - m_sunParameters.m_cosAngularDiameter);
            AZ::Vector3 cieXYZ = AZ::Vector3(0.0f);

            const int wavelengthStep = 10;
            const int lambdaMin = 380;
            const int lambdaMax = 750;
            for (int lambda = lambdaMin; lambda <= lambdaMax; lambda += wavelengthStep)
            {
                int idx = (lambda - lambdaMin) / wavelengthStep;
                AZ::Vector4 data = PhysicalSkyLUT::Spectral[idx];

                // space radiance figures are in cm^-2, we need cm^-1
                const float spaceRadiance = data.GetX() * 10.0f;
                const float koLambda = data.GetY();
                const float kwLambda = data.GetZ();
                const float kgLambda = data.GetW();

                const float rayleighScattering = static_cast<float>(exp(-relativeAirMass * 0.008735f * static_cast<float>(pow(static_cast<float>(lambda) / 1000.0f, -4.08f))));
                const float aerosolScattering = static_cast<float>(exp(-relativeAirMass * beta * static_cast<float>(pow(static_cast<float>(lambda) / 1000.0f, -alpha))));
                const float ozoneAbsorption = static_cast<float>(exp(-relativeAirMass * koLambda * ozoneL));
                const float mixedGasAbsorption = static_cast<float>(exp(-1.41f * kgLambda * relativeAirMass / static_cast<float>(pow(1.0f + 118.93f * kgLambda * relativeAirMass, 0.45f))));
                const float waterAbsorption = static_cast<float>(exp(-0.2385f * kwLambda * w * relativeAirMass / static_cast<float>(pow(1.0f + 20.07f * kwLambda * w * relativeAirMass, 0.45f))));

                // Multiply all the scattering coefficients to attain spectral radiance
                float spectralRadiance = spaceRadiance *
                    rayleighScattering *
                    aerosolScattering *
                    ozoneAbsorption *
                    mixedGasAbsorption *
                    waterAbsorption;

                float spectralIrradiance = spectralRadiance * solidAngle;

                // Now we can go from irradiance => CIE XYZ
                // First get the matching function for our wavelength (lambda)
                AZ::Vector3 matchingFunction = EvaluateCIEXYZ(lambda);

                // Integrate over wavelengths to collect colour information
                cieXYZ += AZ::Vector3(spectralIrradiance) * matchingFunction;
            }

            cieXYZ /= (lambdaMax - lambdaMin) / wavelengthStep;

            // Go from cieXYZ to sRGB
            result.SetX(cieXYZ.GetX() * 3.2404542f + cieXYZ.GetY() * -1.15371385f + cieXYZ.GetZ() * -0.4985314f);
            result.SetY(cieXYZ.GetX() * -0.9692660f + cieXYZ.GetY() * 1.8760108f + cieXYZ.GetZ() * 0.0415560f);
            result.SetZ(cieXYZ.GetX() * 0.0556434f + cieXYZ.GetY() * -0.2040259f + cieXYZ.GetZ() * 1.0572252f);

            result.Normalize();

            return AZ::RPI::TransformColor(Color::CreateFromVector3(result.GetAsVector3()), AZ::RPI::ColorSpaceId::LinearSRGB, AZ::RPI::ColorSpaceId::ACEScg).GetAsVector4();
        }

        AZ::Vector3 SkyBoxFeatureProcessor::EvaluateCIEXYZ(int lambda)
        {
            // Opting for the easy analytical single - lobe fit
            // Fitting function computed from 1964 CIE-standard xyz functions
            // which are fitted for a 10-degree field of view.
            // While using the 1931 standard is more common, it only uses a 2-degree
            // field of view for its tests, making it less optimal for graphics,
            // where a monitor takes up much more than 2 degrees in typical viewing situations
            AZ::Vector3 result;

            // Values taken from Wyman/Sloan/Shirley paper titled "Simple Analytic Approximations to the CIE XYZ Color Matching Functions"
            // x, y, and z all have absolute errors below 3% and root mean square errors around 0.016
            // If more precision is needed, consider a different function
            float valX[4] = { 0.4f, 1014.0f, -0.02f, -570.f };

            float smallLobe = valX[0] * expf(-1250.0f * powf(logf((aznumeric_cast<float>(lambda) - valX[3]) / valX[1]), 2));

            float val2X[4] = { 1.13f, 234.f, -0.001345f, -1.799f };
            float bigLobe = val2X[0] * expf(-val2X[1] * powf(logf((1338.0f - aznumeric_cast<float>(lambda)) / 743.5f), 2));

            result.SetX(smallLobe + bigLobe);

            float valY[4] = { 1.011f, 556.1f, 46.14f, 0.0f };
            result.SetY(valY[0] * expf(-0.5f* powf((aznumeric_cast<float>(lambda) - valY[1]) / valY[2], 2)) + valY[3]);

            float valZ[4] = { 2.06f, 180.4f, 0.125f, 266.0f };
            result.SetZ(valZ[0] * expf(-32.0f* powf(logf((aznumeric_cast<float>(lambda) - valZ[3]) / valZ[1]), 2)));

            return result;
        }
    } // namespace Render
} // namespace AZ
