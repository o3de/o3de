/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Rendering/HairGlobalSettings.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            void HairGlobalSettings::Reflect(ReflectContext* context)
            {
                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<HairGlobalSettings>()
                        ->Version(3)
                        ->Field("EnableShadows", &HairGlobalSettings::m_enableShadows)
                        ->Field("EnableDirectionalLights", &HairGlobalSettings::m_enableDirectionalLights)
                        ->Field("EnablePunctualLights", &HairGlobalSettings::m_enablePunctualLights)
                        ->Field("EnableAreaLights", &HairGlobalSettings::m_enableAreaLights)
                        ->Field("EnableIBL", &HairGlobalSettings::m_enableIBL)
                        ->Field("HairLightingModel", &HairGlobalSettings::m_hairLightingModel)
                        ->Field("EnableMarschner_R", &HairGlobalSettings::m_enableMarschner_R)
                        ->Field("EnableMarschner_TRT", &HairGlobalSettings::m_enableMarschner_TRT)
                        ->Field("EnableMarschner_TT", &HairGlobalSettings::m_enableMarschner_TT)
                        ->Field("EnableLongtitudeCoeff", &HairGlobalSettings::m_enableLongtitudeCoeff)
                        ->Field("EnableAzimuthCoeff", &HairGlobalSettings::m_enableAzimuthCoeff)
                        ;

                    if (auto editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<HairGlobalSettings>("Hair Global Settings", "Shared settings across all hair components")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairGlobalSettings::m_enableShadows, "Enable Shadows", "Enable shadows for hair.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairGlobalSettings::m_enableDirectionalLights, "Enable Directional Lights", "Enable directional lights for hair.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairGlobalSettings::m_enablePunctualLights, "Enable Punctual Lights", "Enable punctual lights for hair.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairGlobalSettings::m_enableAreaLights, "Enable Area Lights", "Enable area lights for hair.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairGlobalSettings::m_enableIBL, "Enable IBL", "Enable imaged-based lighting for hair.")
                            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &HairGlobalSettings::m_hairLightingModel, "Hair Lighting Model", "Determines which lighting equation to use")
                                ->EnumAttribute(Hair::HairLightingModel::GGX, "GGX")
                                ->EnumAttribute(Hair::HairLightingModel::Marschner, "Marschner")
                                ->EnumAttribute(Hair::HairLightingModel::Kajiya, "Kajiya")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairGlobalSettings::m_enableMarschner_R, "Enable Marschner R", "Enable Marschner R.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairGlobalSettings::m_enableMarschner_TRT, "Enable Marschner TRT", "Enable Marschner TRT.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairGlobalSettings::m_enableMarschner_TT, "Enable Marschner TT", "Enable Marschner TT.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairGlobalSettings::m_enableLongtitudeCoeff, "Enable Longtitude", "Enable Longtitude Contribution")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairGlobalSettings::m_enableAzimuthCoeff, "Enable Azimuth", "Enable Azimuth Contribution")
                            ;
                    }
                }
            }
        } // namespace Hair
    } // namespace Render
} // namespace AZ
