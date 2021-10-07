/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Viewport/MaterialViewportSettings.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace MaterialEditor
{
    void MaterialViewportSettings::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MaterialViewportSettings, AZ::UserSettings>()
                ->Version(1)
                ->Field("enableGrid", &MaterialViewportSettings::m_enableGrid)
                ->Field("enableShadowCatcher", &MaterialViewportSettings::m_enableShadowCatcher)
                ->Field("enableAlternateSkybox", &MaterialViewportSettings::m_enableAlternateSkybox)
                ->Field("fieldOfView", &MaterialViewportSettings::m_fieldOfView)
                ->Field("displayMapperOperationType", &MaterialViewportSettings::m_displayMapperOperationType)
                ->Field("selectedModelPresetName", &MaterialViewportSettings::m_selectedModelPresetName)
                ->Field("selectedLightingPresetName", &MaterialViewportSettings::m_selectedLightingPresetName)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<MaterialViewportSettings>(
                    "MaterialViewportSettings", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialViewportSettings::m_enableGrid, "Enable Grid", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialViewportSettings::m_enableShadowCatcher, "Enable Shadow Catcher", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialViewportSettings::m_enableAlternateSkybox, "Enable Alternate Skybox", "")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &MaterialViewportSettings::m_fieldOfView, "Field Of View", "")
                        ->Attribute(AZ::Edit::Attributes::Min, 60.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 120.0f)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MaterialViewportSettings::m_displayMapperOperationType, "Display Mapper Type", "")
                    ->EnumAttribute(AZ::Render::DisplayMapperOperationType::Aces, "Aces")
                    ->EnumAttribute(AZ::Render::DisplayMapperOperationType::AcesLut, "AcesLut")
                    ->EnumAttribute(AZ::Render::DisplayMapperOperationType::Passthrough, "Passthrough")
                    ->EnumAttribute(AZ::Render::DisplayMapperOperationType::GammaSRGB, "GammaSRGB")
                    ->EnumAttribute(AZ::Render::DisplayMapperOperationType::Reinhard, "Reinhard")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<MaterialViewportSettings>("MaterialViewportSettings")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialeditor")
                ->Constructor()
                ->Constructor<const MaterialViewportSettings&>()
                ->Property("enableGrid", BehaviorValueProperty(&MaterialViewportSettings::m_enableGrid))
                ->Property("enableShadowCatcher", BehaviorValueProperty(&MaterialViewportSettings::m_enableShadowCatcher))
                ->Property("enableAlternateSkybox", BehaviorValueProperty(&MaterialViewportSettings::m_enableAlternateSkybox))
                ->Property("fieldOfView", BehaviorValueProperty(&MaterialViewportSettings::m_fieldOfView))
                ->Property("displayMapperOperationType", BehaviorValueProperty(&MaterialViewportSettings::m_displayMapperOperationType))
                ;
        }
    }
} // namespace MaterialEditor
