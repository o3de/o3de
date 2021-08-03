/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
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
                        ->Version(1)
                        ->Field("HairLightingModel", &HairGlobalSettings::m_hairLightingModel)
                        ;

                    if (auto editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<HairGlobalSettings>("Hair Global Settings", "Shared settings across all hair components")
                            ->DataElement(
                                AZ::Edit::UIHandlers::ComboBox, &HairGlobalSettings::m_hairLightingModel, "Hair Lighting Model",
                                "Determines which lighting equation to use")
                            ->EnumAttribute(Hair::HairLightingModel::GGX, "GGX")
                            ->EnumAttribute(Hair::HairLightingModel::Marschner, "Marschner")
                            ->EnumAttribute(Hair::HairLightingModel::Kajiya, "Kajiya")
                            ;
                    }
                }
            }
        } // namespace Hair
    } // namespace Render
} // namespace AZ
