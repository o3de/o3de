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
#include <AzFramework/Translation/TranslationDef.h>

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
                        editContext->Class<HairGlobalSettings>(QT_TRANSLATE_NOOP("AtomTressFX", "Hair Global Settings"), QT_TRANSLATE_NOOP("AtomTressFX", "Shared settings across all hair components"))
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairGlobalSettings::m_enableShadows, QT_TRANSLATE_NOOP("AtomTressFX", "Enable Shadows"), QT_TRANSLATE_NOOP("AtomTressFX", "Enable shadows for hair."))
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairGlobalSettings::m_enableDirectionalLights, QT_TRANSLATE_NOOP("AtomTressFX", "Enable Directional Lights"), QT_TRANSLATE_NOOP("AtomTressFX", "Enable directional lights for hair."))
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairGlobalSettings::m_enablePunctualLights, QT_TRANSLATE_NOOP("AtomTressFX", "Enable Punctual Lights"), QT_TRANSLATE_NOOP("AtomTressFX", "Enable punctual lights for hair."))
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairGlobalSettings::m_enableAreaLights, QT_TRANSLATE_NOOP("AtomTressFX", "Enable Area Lights"), QT_TRANSLATE_NOOP("AtomTressFX", "Enable area lights for hair."))
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairGlobalSettings::m_enableIBL, QT_TRANSLATE_NOOP("AtomTressFX", "Enable IBL"), QT_TRANSLATE_NOOP("AtomTressFX", "Enable imaged-based lighting for hair."))
                            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &HairGlobalSettings::m_hairLightingModel, QT_TRANSLATE_NOOP("AtomTressFX", "Hair Lighting Model"), QT_TRANSLATE_NOOP("AtomTressFX", "Determines which lighting equation to use"))
                                ->EnumAttribute(Hair::HairLightingModel::GGX, QT_TRANSLATE_NOOP("AtomTressFX", "GGX"))
                                ->EnumAttribute(Hair::HairLightingModel::Marschner, QT_TRANSLATE_NOOP("AtomTressFX", "Marschner"))
                                ->EnumAttribute(Hair::HairLightingModel::Kajiya, QT_TRANSLATE_NOOP("AtomTressFX", "Kajiya"))
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairGlobalSettings::m_enableMarschner_R, QT_TRANSLATE_NOOP("AtomTressFX", "Enable Marschner R"), QT_TRANSLATE_NOOP("AtomTressFX", "Enable Marschner R."))
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairGlobalSettings::m_enableMarschner_TRT, QT_TRANSLATE_NOOP("AtomTressFX", "Enable Marschner TRT"), QT_TRANSLATE_NOOP("AtomTressFX", "Enable Marschner TRT."))
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairGlobalSettings::m_enableMarschner_TT, QT_TRANSLATE_NOOP("AtomTressFX", "Enable Marschner TT"), QT_TRANSLATE_NOOP("AtomTressFX", "Enable Marschner TT."))
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairGlobalSettings::m_enableLongtitudeCoeff, QT_TRANSLATE_NOOP("AtomTressFX", "Enable Longitude"), QT_TRANSLATE_NOOP("AtomTressFX", "Enable Longitude Contribution"))
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairGlobalSettings::m_enableAzimuthCoeff, QT_TRANSLATE_NOOP("AtomTressFX", "Enable Azimuth"), QT_TRANSLATE_NOOP("AtomTressFX", "Enable Azimuth Contribution"))
                            ;
                    }
                }
            }
        } // namespace Hair
    } // namespace Render
} // namespace AZ
