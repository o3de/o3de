/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <Viewport/MaterialCanvasViewportSettings.h>

namespace MaterialCanvas
{
    void MaterialCanvasViewportSettings::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MaterialCanvasViewportSettings>()
                ->Version(3)
                ->Field("enableGrid", &MaterialCanvasViewportSettings::m_enableGrid)
                ->Field("enableShadowCatcher", &MaterialCanvasViewportSettings::m_enableShadowCatcher)
                ->Field("enableAlternateSkybox", &MaterialCanvasViewportSettings::m_enableAlternateSkybox)
                ->Field("fieldOfView", &MaterialCanvasViewportSettings::m_fieldOfView)
                ->Field("displayMapperOperationType", &MaterialCanvasViewportSettings::m_displayMapperOperationType)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<MaterialCanvasViewportSettings>(
                    "MaterialCanvasViewportSettings", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialCanvasViewportSettings::m_enableGrid, "Enable Grid", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialCanvasViewportSettings::m_enableShadowCatcher, "Enable Shadow Catcher", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialCanvasViewportSettings::m_enableAlternateSkybox, "Enable Alternate Skybox", "")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &MaterialCanvasViewportSettings::m_fieldOfView, "Field Of View", "")
                        ->Attribute(AZ::Edit::Attributes::Min, 60.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 120.0f)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MaterialCanvasViewportSettings::m_displayMapperOperationType, "Display Mapper Type", "")
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
            behaviorContext->Class<MaterialCanvasViewportSettings>("MaterialCanvasViewportSettings")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialcanvas")
                ->Constructor()
                ->Constructor<const MaterialCanvasViewportSettings&>()
                ->Property("enableGrid", BehaviorValueProperty(&MaterialCanvasViewportSettings::m_enableGrid))
                ->Property("enableShadowCatcher", BehaviorValueProperty(&MaterialCanvasViewportSettings::m_enableShadowCatcher))
                ->Property("enableAlternateSkybox", BehaviorValueProperty(&MaterialCanvasViewportSettings::m_enableAlternateSkybox))
                ->Property("fieldOfView", BehaviorValueProperty(&MaterialCanvasViewportSettings::m_fieldOfView))
                ->Property("displayMapperOperationType", BehaviorValueProperty(&MaterialCanvasViewportSettings::m_displayMapperOperationType))
                ;
        }
    }
} // namespace MaterialCanvas
