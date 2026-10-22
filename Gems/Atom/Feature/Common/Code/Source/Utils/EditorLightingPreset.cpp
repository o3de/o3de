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
#include <AzFramework/Translation/TranslationDef.h>

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
                            QT_TRANSLATE_NOOP("Atom::Feature", "Control Type"),
                            QT_TRANSLATE_NOOP("Atom::Feature", "How to control a exposure value."))
                            ->EnumAttribute(ExposureControlConfig::ExposureControlType::ManualOnly, QT_TRANSLATE_NOOP("Atom::Feature", "Manual Only"))
                            ->EnumAttribute(ExposureControlConfig::ExposureControlType::EyeAdaptation, QT_TRANSLATE_NOOP("Atom::Feature", "Eye Adaptation"))

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &ExposureControlConfig::m_manualCompensationValue, QT_TRANSLATE_NOOP("Atom::Feature", "Manual compensation"), QT_TRANSLATE_NOOP("Atom::Feature", "Manual exposure compensation value."))
                            ->Attribute(AZ::Edit::Attributes::Min, -16.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 16.0f)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &ExposureControlConfig::m_autoExposureMin, QT_TRANSLATE_NOOP("Atom::Feature", "Minimum Exposure"), QT_TRANSLATE_NOOP("Atom::Feature", "Minimum exposure value for the auto exposure."))
                            ->Attribute(AZ::Edit::Attributes::Min, -16.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 16.0f)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &ExposureControlConfig::m_autoExposureMax, QT_TRANSLATE_NOOP("Atom::Feature", "Maximum Exposure"), QT_TRANSLATE_NOOP("Atom::Feature", "Maximum exposure value for the auto exposure."))
                            ->Attribute(AZ::Edit::Attributes::Min, -16.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 16.0f)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &ExposureControlConfig::m_autoExposureSpeedUp, QT_TRANSLATE_NOOP("Atom::Feature", "Speed Up"), QT_TRANSLATE_NOOP("Atom::Feature", "The speed at which auto exposure adapts to bright scenes."))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.01)
                            ->Attribute(AZ::Edit::Attributes::Max, 10.0f)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &ExposureControlConfig::m_autoExposureSpeedDown, QT_TRANSLATE_NOOP("Atom::Feature", "Speed Down"), QT_TRANSLATE_NOOP("Atom::Feature", "The speed at which auto exposure adapts to dark scenes."))
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
                        ->DataElement(AZ::Edit::UIHandlers::Default, &LightConfig::m_direction, QT_TRANSLATE_NOOP("Atom::Feature", "Direction"), "")
                        ->DataElement(Edit::UIHandlers::Color, &LightConfig::m_color, QT_TRANSLATE_NOOP("Atom::Feature", "Color"), QT_TRANSLATE_NOOP("Atom::Feature", "Color of the light"))
                            ->Attribute("ColorEditorConfiguration", AZ::RPI::ColorUtils::GetLinearRgbEditorConfig())
                        ->DataElement(Edit::UIHandlers::Default, &LightConfig::m_intensity, QT_TRANSLATE_NOOP("Atom::Feature", "Intensity"), QT_TRANSLATE_NOOP("Atom::Feature", "Intensity of the light in the set photometric unit."))

                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("Atom::Feature", "Shadow"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(Edit::UIHandlers::Default, &LightConfig::m_shadowFarClipDistance, QT_TRANSLATE_NOOP("Atom::Feature", "Shadow Far Clip"), QT_TRANSLATE_NOOP("Atom::Feature", "Shadow specific far clip distance."))
                        ->DataElement(Edit::UIHandlers::ComboBox, &LightConfig::m_shadowmapSize, QT_TRANSLATE_NOOP("Atom::Feature", "Shadowmap Size"), QT_TRANSLATE_NOOP("Atom::Feature", "Width/Height of shadowmap"))
                            ->EnumAttribute(ShadowmapSize::Size256, QT_TRANSLATE_NOOP("Atom::Feature", " 256"))
                            ->EnumAttribute(ShadowmapSize::Size512, QT_TRANSLATE_NOOP("Atom::Feature", " 512"))
                            ->EnumAttribute(ShadowmapSize::Size1024, QT_TRANSLATE_NOOP("Atom::Feature", "1024"))
                            ->EnumAttribute(ShadowmapSize::Size2048, QT_TRANSLATE_NOOP("Atom::Feature", "2048"))
                        ->DataElement(Edit::UIHandlers::Slider, &LightConfig::m_shadowCascadeCount, QT_TRANSLATE_NOOP("Atom::Feature", "Cascade Count"), QT_TRANSLATE_NOOP("Atom::Feature", "Number of cascades"))
                            ->Attribute(Edit::Attributes::Min, 1)
                            ->Attribute(Edit::Attributes::Max, Shadow::MaxNumberOfCascades)
                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &LightConfig::m_enableShadowDebugColoring, QT_TRANSLATE_NOOP("Atom::Feature", "Enable Debug Coloring?"),
                            QT_TRANSLATE_NOOP("Atom::Feature", "Enable coloring to see how cascades places 0:red, 1:green, 2:blue, 3:yellow."))
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
                        ->DataElement(AZ::Edit::UIHandlers::Default, &LightingPreset::m_iblDiffuseImageAsset, QT_TRANSLATE_NOOP("Atom::Feature", "IBL Diffuse Image Asset"), QT_TRANSLATE_NOOP("Atom::Feature", "IBL diffuse image asset reference"))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &LightingPreset::m_iblSpecularImageAsset, QT_TRANSLATE_NOOP("Atom::Feature", "IBL Specular Image Asset"), QT_TRANSLATE_NOOP("Atom::Feature", "IBL specular image asset reference"))
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &LightingPreset::m_iblExposure, QT_TRANSLATE_NOOP("Atom::Feature", "IBL exposure"), QT_TRANSLATE_NOOP("Atom::Feature", "IBL exposure"))
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -5.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 5.0f)
                            ->Attribute(AZ::Edit::Attributes::Min, -20.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &LightingPreset::m_skyboxImageAsset, QT_TRANSLATE_NOOP("Atom::Feature", "Skybox Image Asset"), QT_TRANSLATE_NOOP("Atom::Feature", "Skybox image asset reference"))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &LightingPreset::m_alternateSkyboxImageAsset, QT_TRANSLATE_NOOP("Atom::Feature", "Skybox Image Asset (Alt)"), QT_TRANSLATE_NOOP("Atom::Feature", "Alternate skybox image asset reference"))
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &LightingPreset::m_skyboxExposure, QT_TRANSLATE_NOOP("Atom::Feature", "Skybox Exposure"), QT_TRANSLATE_NOOP("Atom::Feature", "Skybox exposure"))
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -5.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 5.0f)
                            ->Attribute(AZ::Edit::Attributes::Min, -20.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &LightingPreset::m_shadowCatcherOpacity, QT_TRANSLATE_NOOP("Atom::Feature", "Shadow Catcher Opacity"), QT_TRANSLATE_NOOP("Atom::Feature", "Shadow catcher opacity"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &LightingPreset::m_exposure, QT_TRANSLATE_NOOP("Atom::Feature", "Exposure"), QT_TRANSLATE_NOOP("Atom::Feature", "Exposure"))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &LightingPreset::m_lights, QT_TRANSLATE_NOOP("Atom::Feature", "Lights"), QT_TRANSLATE_NOOP("Atom::Feature", "Lights"))
                            ->Attribute(AZ::Edit::Attributes::ClearNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                            ->Attribute(AZ::Edit::Attributes::AddNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                            ->Attribute(AZ::Edit::Attributes::RemoveNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ;
                }
            }
        }
    } // namespace Render
} // namespace AZ
