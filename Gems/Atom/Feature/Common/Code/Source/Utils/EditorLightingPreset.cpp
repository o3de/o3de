/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#undef RC_INVOKED

#include <Atom/Feature/Utils/EditorLightingPreset.h>
#include <Atom/Feature/Utils/LightingPreset.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <Atom/RPI.Edit/Common/ColorUtils.h>

namespace AZ
{
    namespace Render
    {
        void EditorExposureControlConfig::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<ExposureControlConfig>(
                        "ExposureControlConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(Edit::UIHandlers::ComboBox,
                            &ExposureControlConfig::m_exposureControlType,
                            "Control Type",
                            "How to control a exposure value.")
                            ->EnumAttribute(ExposureControlConfig::ExposureControlType::ManualOnly, "Manual Only")
                            ->EnumAttribute(ExposureControlConfig::ExposureControlType::EyeAdaptation, "Eye Adaptation")

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &ExposureControlConfig::m_manualCompensationValue, "Manual compensation", "Manual exposure compensation value.")
                            ->Attribute(AZ::Edit::Attributes::Min, -16.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 16.0f)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &ExposureControlConfig::m_autoExposureMin, "Minimum Exposure", "Minimum exposure value for the auto exposure.")
                            ->Attribute(AZ::Edit::Attributes::Min, -16.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 16.0f)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &ExposureControlConfig::m_autoExposureMax, "Maximum Exposure", "Maximum exposure value for the auto exposure.")
                            ->Attribute(AZ::Edit::Attributes::Min, -16.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 16.0f)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &ExposureControlConfig::m_autoExposureSpeedUp, "Speed Up", "The speed at which auto exposure adapts to bright scenes.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.01)
                            ->Attribute(AZ::Edit::Attributes::Max, 10.0f)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &ExposureControlConfig::m_autoExposureSpeedDown, "Speed Down", "The speed at which auto exposure adapts to dark scenes.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.01)
                            ->Attribute(AZ::Edit::Attributes::Max, 10.0f)

                        ;
                }
            }
        }

        void EditorLightConfig::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<LightConfig>(
                        "LightConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &LightConfig::m_direction, "Direction", "")
                        ->DataElement(Edit::UIHandlers::Color, &LightConfig::m_color, "Color", "Color of the light")
                            ->Attribute("ColorEditorConfiguration", AZ::RPI::ColorUtils::GetLinearRgbEditorConfig())
                        ->DataElement(Edit::UIHandlers::Default, &LightConfig::m_intensity, "Intensity", "Intensity of the light in the set photometric unit.")

                        ->ClassElement(AZ::Edit::ClassElements::Group, "Shadow")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(Edit::UIHandlers::Default, &LightConfig::m_shadowFarClipDistance, "Shadow Far Clip", "Shadow specific far clip distance.")
                        ->DataElement(Edit::UIHandlers::ComboBox, &LightConfig::m_shadowmapSize, "Shadowmap Size", "Width/Height of shadowmap")
                            ->EnumAttribute(ShadowmapSize::Size256, " 256")
                            ->EnumAttribute(ShadowmapSize::Size512, " 512")
                            ->EnumAttribute(ShadowmapSize::Size1024, "1024")
                            ->EnumAttribute(ShadowmapSize::Size2048, "2048")
                        ->DataElement(Edit::UIHandlers::Slider, &LightConfig::m_shadowCascadeCount, "Cascade Count", "Number of cascades")
                            ->Attribute(Edit::Attributes::Min, 1)
                            ->Attribute(Edit::Attributes::Max, Shadow::MaxNumberOfCascades)
                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &LightConfig::m_enableShadowDebugColoring, "Enable Debug Coloring?",
                            "Enable coloring to see how cascades places 0:red, 1:green, 2:blue, 3:yellow.")
                        ;
                }
            }
        }

        void EditorLightingPreset::Reflect(AZ::ReflectContext* context)
        {
            EditorExposureControlConfig::Reflect(context);
            EditorLightConfig::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<LightingPreset>(
                        "LightingPreset", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &LightingPreset::m_iblDiffuseImageAsset, "IBL Diffuse Image Asset", "IBL diffuse image asset reference")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &LightingPreset::m_iblSpecularImageAsset, "IBL Specular Image Asset", "IBL specular image asset reference")
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &LightingPreset::m_iblExposure, "IBL exposure", "IBL exposure")
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -5.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 5.0f)
                            ->Attribute(AZ::Edit::Attributes::Min, -20.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &LightingPreset::m_skyboxImageAsset, "Skybox Image Asset", "Skybox image asset reference")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &LightingPreset::m_alternateSkyboxImageAsset, "Skybox Image Asset (Alt)", "Alternate skybox image asset reference")
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &LightingPreset::m_skyboxExposure, "Skybox Exposure", "Skybox exposure")
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -5.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 5.0f)
                            ->Attribute(AZ::Edit::Attributes::Min, -20.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &LightingPreset::m_shadowCatcherOpacity, "Shadow Catcher Opacity", "Shadow catcher opacity")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &LightingPreset::m_exposure, "Exposure", "Exposure")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &LightingPreset::m_lights, "Lights", "Lights")
                            ->Attribute(AZ::Edit::Attributes::ClearNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                            ->Attribute(AZ::Edit::Attributes::AddNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                            ->Attribute(AZ::Edit::Attributes::RemoveNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ;
                }
            }
        }
    } // namespace Render
} // namespace AZ
