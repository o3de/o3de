/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Debug/EventTrace.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Asset/AssetSystemBus.h>

#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

#include <PostProcess/DepthOfField/DepthOfFieldSettings.h>
#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcessing/DepthOfFieldCompositePass.h>
#include <PostProcessing/DepthOfFieldBokehBlurPass.h>
#include <PostProcessing/DepthOfFieldMaskPass.h>
#include <PostProcessing/DepthOfFieldPencilMap.h>
#include <PostProcessing/DepthOfFieldReadBackFocusDepthPass.h>

namespace AZ
{
    namespace Render
    {
        namespace DepthOfField
        {
            // Parameters for each quality are listed here.
            struct Quality
            {
                uint32_t sampleRadialDivision2 = 0;
                uint32_t sampleRadialDivision4 = 0;
                uint32_t sampleRadialDivision8 = 0;
            };

            static constexpr Quality QualitySet[DepthOfField::QualityLevelMax] =
            {
                // It is the radial division count of blur kernel.
                {2, 3, 4},
                {4, 4, 4}
            };
        }

        DepthOfFieldSettings::DepthOfFieldSettings(PostProcessFeatureProcessor* featureProcessor)
            : PostProcessBase(featureProcessor)
        {
            LoadPencilMap();
            m_pencilMapIndex = GetSceneSrg()->FindShaderInputImageIndex(Name("m_dofPencilMap"));

            // Get default 
            auto viewSrg = GetDefaultViewSrg();
            AZ_Assert(viewSrg, "DepthOfFieldSettings : Failed to get the default render pipeline's default viewSrg.");

            m_passListWithHashOfDivisionNumber.Insert(Name("FrontblurDivision2"), AZ::RHI::Handle<uint32_t>(2));
            m_passListWithHashOfDivisionNumber.Insert(Name("BackblurDivision2"), AZ::RHI::Handle<uint32_t>(2));
            m_passListWithHashOfDivisionNumber.Insert(Name("MaskDivision2"), AZ::RHI::Handle<uint32_t>(2));
            m_passListWithHashOfDivisionNumber.Insert(Name("FrontblurDivision4"), AZ::RHI::Handle<uint32_t>(4));
            m_passListWithHashOfDivisionNumber.Insert(Name("BackblurDivision4"), AZ::RHI::Handle<uint32_t>(4));
            m_passListWithHashOfDivisionNumber.Insert(Name("MaskDivision4"), AZ::RHI::Handle<uint32_t>(4));
            m_passListWithHashOfDivisionNumber.Insert(Name("FrontblurDivision8"), AZ::RHI::Handle<uint32_t>(8));
            m_passListWithHashOfDivisionNumber.Insert(Name("BackblurDivision8"), AZ::RHI::Handle<uint32_t>(8));
            m_passListWithHashOfDivisionNumber.Insert(Name("MaskDivision8"), AZ::RHI::Handle<uint32_t>(8));
        }

        void DepthOfFieldSettings::LoadPencilMap()
        {
            m_pencilMap = RPI::LoadStreamingTexture(PencilMap::TextureFilePath);
            if (!m_pencilMap)
            {
                AZ_Error("DepthOfFieldSettings", false, "Failed to find or create an image instance from image asset '%s'", PencilMap::TextureFilePath);
            }
        }

        void DepthOfFieldSettings::OnConfigChanged()
        {
            m_parentSettings->OnConfigChanged();
        }

        void DepthOfFieldSettings::ApplySettingsTo(DepthOfFieldSettings* target, float alpha) const
        {
            AZ_Assert(target != nullptr, "DepthOfFieldSettings::ApplySettingsTo called with nullptr as argument.");

            // Auto-gen code to blend individual params based on their override value onto target settings
#define OVERRIDE_TARGET target
#define OVERRIDE_ALPHA alpha
#include <Atom/Feature/ParamMacros/StartOverrideBlend.inl>
#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef OVERRIDE_TARGET
#undef OVERRIDE_ALPHA
        }

        void DepthOfFieldSettings::Simulate(float deltaTime)
        {
            m_deltaTime = deltaTime;
            UpdatePencilMapTexture();

            if (m_cameraEntityId.IsValid() && m_enabled)
            {
                UpdateCameraParameters();
                UpdateAutoFocusDepth(m_enabled);
                UpdateBlendFactor();
            }
        }

        void DepthOfFieldSettings::SetValuesToViewSrg(AZ::Data::Instance<RPI::ShaderResourceGroup> viewSrg)
        {
            viewSrg->SetConstant(m_cameraParametersIndex, m_configurationToViewSRG.m_cameraParameters);
            viewSrg->SetConstant(m_pencilMapTexcoordToCocRadiusIndex, m_configurationToViewSRG.m_pencilMapTexcoordToCocRadius);
            viewSrg->SetConstant(m_pencilMapFocusPointTexcoordUIndex, m_configurationToViewSRG.m_pencilMapFocusPointTexcoordU);
            viewSrg->SetConstant(m_cocToScreenRatioIndex, m_configurationToViewSRG.m_cocToScreenRatio);
        }

        void DepthOfFieldSettings::UpdatePencilMapTexture() const
        {
            GetSceneSrg()->SetImage(m_pencilMapIndex, m_pencilMap);
        }

        void DepthOfFieldSettings::UpdateCameraParameters()
        {
            // get camera parameters
            float viewFovRadian = 0.0f;
            float viewWidth = 0.0f;
            float viewHeight = 0.0f;
            float viewNear = 0.0f;
            float viewFar = 0.0f;
            Camera::CameraRequestBus::EventResult(viewFovRadian, m_cameraEntityId, &Camera::CameraRequestBus::Events::GetFovRadians);
            Camera::CameraRequestBus::EventResult(viewWidth, m_cameraEntityId, &Camera::CameraRequestBus::Events::GetFrustumWidth);
            Camera::CameraRequestBus::EventResult(viewHeight, m_cameraEntityId, &Camera::CameraRequestBus::Events::GetFrustumHeight);
            Camera::CameraRequestBus::EventResult(viewNear, m_cameraEntityId, &Camera::CameraRequestBus::Events::GetNearClipDistance);
            Camera::CameraRequestBus::EventResult(viewFar, m_cameraEntityId, &Camera::CameraRequestBus::Events::GetFarClipDistance);
            if (m_viewFovRadian != viewFovRadian
                || m_viewWidth != viewWidth
                || m_viewHeight != viewHeight
                || m_viewNear != viewNear
                || m_viewFar != viewFar)
            {
                m_viewFovRadian = viewFovRadian;
                m_viewWidth = viewWidth;
                m_viewHeight = viewHeight;
                m_viewNear = viewNear;
                m_viewFar = viewFar;
            }
        }

        void DepthOfFieldSettings::UpdateBlendFactor()
        {
            float focusDistance = 0.0f;
            if (m_enableAutoFocus)
            {
                focusDistance = m_viewNear + m_normalizedFocusDistanceForAutoFocus * (m_viewFar - m_viewNear);
                focusDistance = GetClamp(focusDistance, m_viewNear, m_viewFar);
            }
            else
            {
                focusDistance = GetClamp(m_focusDistance, m_viewNear, m_viewFar);
            }

            m_configurationToViewSRG.m_cameraParameters[0] = m_viewFar;
            m_configurationToViewSRG.m_cameraParameters[1] = m_viewNear;
            m_configurationToViewSRG.m_cameraParameters[2] = focusDistance;
            m_viewAspectRatio = m_viewWidth / m_viewHeight;

            float cameraSensorDiagonalLength = PencilMap::EIS_35mm_DiagonalLength;
            float imageSensorHeight = cameraSensorDiagonalLength / sqrt(m_viewAspectRatio * m_viewAspectRatio + 1);
            float verticalTanHalfFov = tanf(m_viewFovRadian * 0.5f);

            // focalLength : Focusing distance of lens
            float focalLength = focusDistance * imageSensorHeight / (verticalTanHalfFov * 2.0f * focusDistance + imageSensorHeight);

            float cocToRatio = (focalLength * (focalLength / m_fNumber)) / (focusDistance - focalLength);
            m_configurationToViewSRG.m_cocToScreenRatio = cocToRatio / imageSensorHeight;

            // Ratio of filter diameter to screen, vertical reference.
            constexpr float ScreenApertureDiameter = 0.005f;

            // The diameter ratio of the reduced buffer compared to the next larger buffer.
            constexpr float DiameterDivisionScaleRatio = 4.0f;
            float screenApertureDiameterDivision2 = ScreenApertureDiameter * DiameterDivisionScaleRatio;
            float screenApertureDiameterDivision4 = screenApertureDiameterDivision2 * DiameterDivisionScaleRatio;
            float screenApertureDiameterDivision8 = screenApertureDiameterDivision4 * DiameterDivisionScaleRatio;

            // coc0Ratio : speed of blur end. The smaller the value, blur ends faster and changes suddenly
            // coc1Ratio : speed of blur start. The higher the value, blur starts later and changes suddenly
            constexpr float Coc0RatioBack = 0.51f;
            constexpr float Coc1RatioBack = 0.61f;
            constexpr float Coc0RatioFront = 1.0f;
            constexpr float Coc1RatioFront = 1.0f;

            float scaledDiameter = ScreenApertureDiameter * 0.25f;

            // This is the conversion factor for calculating the blend ratio from DofFactor.

            // coc0 : Confusion circle diameter screen ratio
            // coc1 : Confusion circle diameter screen ratio of one lower blur level;
            float backCoc0_Division2 = screenApertureDiameterDivision2 * Coc0RatioBack + scaledDiameter;
            float backCoc0_Division4 = screenApertureDiameterDivision4 * Coc0RatioBack + scaledDiameter;
            float backCoc0_Division8 = screenApertureDiameterDivision8 * Coc0RatioBack + scaledDiameter;

            float backCoc1_Division2 = ScreenApertureDiameter * Coc1RatioBack + scaledDiameter;
            float backCoc1_Division4 = screenApertureDiameterDivision2 * Coc1RatioBack + scaledDiameter;
            float backCoc1_Division8 = screenApertureDiameterDivision4 * Coc1RatioBack + scaledDiameter;

            float frontCoc0_Division2 = screenApertureDiameterDivision2 * Coc0RatioFront + scaledDiameter;
            float frontCoc0_Division4 = screenApertureDiameterDivision4 * Coc0RatioFront + scaledDiameter;
            float frontCoc0_Division8 = screenApertureDiameterDivision8 * Coc0RatioFront + scaledDiameter;

            float frontCoc1_Division2 = ScreenApertureDiameter * Coc1RatioFront + scaledDiameter;
            float frontCoc1_Division4 = screenApertureDiameterDivision2 * Coc1RatioFront + scaledDiameter;
            float frontCoc1_Division8 = screenApertureDiameterDivision4 * Coc1RatioFront + scaledDiameter;

            m_configurationToViewSRG.m_backBlendFactorDivision2[0] = m_configurationToViewSRG.m_cocToScreenRatio / (backCoc0_Division2 - backCoc1_Division2);
            m_configurationToViewSRG.m_backBlendFactorDivision2[1] = -backCoc1_Division2 / (backCoc0_Division2 - backCoc1_Division2);
            m_configurationToViewSRG.m_frontBlendFactorDivision2[0] = -m_configurationToViewSRG.m_cocToScreenRatio / (frontCoc0_Division2 - frontCoc1_Division2);
            m_configurationToViewSRG.m_frontBlendFactorDivision2[1] = -frontCoc1_Division2 / (frontCoc0_Division2 - frontCoc1_Division2);

            m_configurationToViewSRG.m_backBlendFactorDivision4[0] = m_configurationToViewSRG.m_cocToScreenRatio / (backCoc0_Division4 - backCoc1_Division4);
            m_configurationToViewSRG.m_backBlendFactorDivision4[1] = -backCoc1_Division4 / (backCoc0_Division4 - backCoc1_Division4);
            m_configurationToViewSRG.m_frontBlendFactorDivision4[0] = -m_configurationToViewSRG.m_cocToScreenRatio / (frontCoc0_Division4 - frontCoc1_Division4);
            m_configurationToViewSRG.m_frontBlendFactorDivision4[1] = -frontCoc1_Division4 / (frontCoc0_Division4 - frontCoc1_Division4);

            m_configurationToViewSRG.m_backBlendFactorDivision8[0] = m_configurationToViewSRG.m_cocToScreenRatio / (backCoc0_Division8 - backCoc1_Division8);
            m_configurationToViewSRG.m_backBlendFactorDivision8[1] = -backCoc1_Division8 / (backCoc0_Division8 - backCoc1_Division8);
            m_configurationToViewSRG.m_frontBlendFactorDivision8[0] = -m_configurationToViewSRG.m_cocToScreenRatio / (frontCoc0_Division8 - frontCoc1_Division8);
            m_configurationToViewSRG.m_frontBlendFactorDivision8[1] = -frontCoc1_Division8 / (frontCoc0_Division8 - frontCoc1_Division8);

            // max: radius x 2.0
            // min: radius x 0.5
            // Determine the maximum and minimum radius values so that the blurs in the front and back buffers are connected smoothly.
            m_maxBokehRadiusDivision2 = screenApertureDiameterDivision2;
            m_minBokehRadiusDivision2 = screenApertureDiameterDivision2 * 0.25f;
            m_maxBokehRadiusDivision4 = screenApertureDiameterDivision4;
            m_minBokehRadiusDivision4 = screenApertureDiameterDivision4 * 0.25f;
            m_maxBokehRadiusDivision8 = screenApertureDiameterDivision8;
            m_minBokehRadiusDivision8 = screenApertureDiameterDivision8 * 0.25f;

            // The ratio of the texcoord U of the pencil map to circle of confusion radius.
            // experimentally adjusted value.
            constexpr float PencilMapTexcoordToCocRadiusScale = 5.0f;

            float pencilMapTexcoordToCocRadius =
                PencilMapTexcoordToCocRadiusScale * m_fNumber * sqrt(m_viewFovRadian * 2.0f)
                / (focalLength / (focusDistance - focalLength) + 1.0f);

            m_configurationToViewSRG.m_pencilMapTexcoordToCocRadius = pencilMapTexcoordToCocRadius;
            m_configurationToViewSRG.m_pencilMapFocusPointTexcoordU = PencilMap::PencilMapFocusPointTexcoordU;
        }

        // [GFX TODO][ATOM-3035]This function is temporary and will change with improvement to the draw list tag system
        void DepthOfFieldSettings::UpdateAutoFocusDepth(bool enabled)
        {            
            const Name TemplateNameReadBackFocusDepth = Name("DepthOfFieldReadBackFocusDepthTemplate");
            // [GFX TODO][ATOM-4908] multiple camera should be distingushed.
            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithTemplateName(TemplateNameReadBackFocusDepth, GetParentScene());
            RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this, enabled](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
                {
                    auto* dofPass = azrtti_cast<AZ::Render::DepthOfFieldReadBackFocusDepthPass*>(pass);
                    if (enabled)
                    {
                        m_normalizedFocusDistanceForAutoFocus = dofPass->GetNormalizedFocusDistanceForAutoFocus();
                    }
                    return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                });
        }

        void DepthOfFieldSettings::SetCameraEntityId(EntityId cameraEntityId)
        {
            m_cameraEntityId = cameraEntityId;
        }

        void DepthOfFieldSettings::SetEnabled(bool enabled)
        {
            m_enabled = enabled && m_cameraEntityId.IsValid();
        }

        void DepthOfFieldSettings::SetQualityLevel(uint32_t qualityLevel)
        {
            m_qualityLevel = qualityLevel;
            m_sampleRadialDivision2 = DepthOfField::QualitySet[qualityLevel].sampleRadialDivision2;
            m_sampleRadialDivision4 = DepthOfField::QualitySet[qualityLevel].sampleRadialDivision4;
            m_sampleRadialDivision8 = DepthOfField::QualitySet[qualityLevel].sampleRadialDivision8;
        }

        void DepthOfFieldSettings::SetApertureF(float apertureF)
        {
            m_apertureF = apertureF;
            UpdateFNumber();
        }

        void DepthOfFieldSettings::UpdateFNumber()
        {
            // convert from [0, 1] to [1/256 - 1/0.12]
            constexpr float Min = DepthOfField::ApertureFMin;
            constexpr float Max = DepthOfField::ApertureFMax;
            float apertureF = 1.0f / Max + (1.0f / Min - 1.0f / Max) * m_apertureF;
            // convert from [1/256 - 1/0.12] to [256 - 0.12]
            m_fNumber = 1.0f / apertureF;
        }

        void DepthOfFieldSettings::SetFNumber([[maybe_unused]] float fNumber)
        {
            // FNumber is inferred from ApertureF
        }

        void DepthOfFieldSettings::SetFocusDistance(float focusDistance)
        {
            m_focusDistance = focusDistance;
        }

        void DepthOfFieldSettings::SetEnableAutoFocus(bool enableAutoFocus)
        {
            m_enableAutoFocus = enableAutoFocus;
        }

        void DepthOfFieldSettings::SetAutoFocusScreenPosition(Vector2 screenPosition)
        {
            m_autoFocusScreenPosition = screenPosition;
        }

        void DepthOfFieldSettings::SetAutoFocusSensitivity(float sensitivity)
        {
            m_autoFocusSensitivity = sensitivity;
        }

        void DepthOfFieldSettings::SetAutoFocusSpeed(float speed)
        {
            m_autoFocusSpeed = speed;
        }

        void DepthOfFieldSettings::SetAutoFocusDelay(float delay)
        {
            m_autoFocusDelay = delay;
        }

        void DepthOfFieldSettings::SetEnableDebugColoring(bool enabled)
        {
            m_enableDebugColoring = enabled;
        }

        AZ::RHI::Handle<uint32_t> DepthOfFieldSettings::GetSplitSizeForPass(const Name& passName) const
        {
            return m_passListWithHashOfDivisionNumber.Find(passName);
        }

    } // namespace Render
} // namespace AZ
