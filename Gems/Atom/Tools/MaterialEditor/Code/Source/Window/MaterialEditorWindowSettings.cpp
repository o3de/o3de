/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Atom/Window/MaterialEditorWindowSettings.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace MaterialEditor
{
    void MaterialEditorWindowSettings::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MaterialEditorWindowSettings, AZ::UserSettings>()
                ->Version(1)
                ->Field("enableGrid", &MaterialEditorWindowSettings::m_enableGrid)
                ->Field("enableShadowCatcher", &MaterialEditorWindowSettings::m_enableShadowCatcher)
                ->Field("enableAlternateSkybox", &MaterialEditorWindowSettings::m_enableAlternateSkybox)
                ->Field("fieldOfView", &MaterialEditorWindowSettings::m_fieldOfView)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<MaterialEditorWindowSettings>(
                    "MaterialEditorWindowSettings", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialEditorWindowSettings::m_enableGrid, "Enable Grid", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialEditorWindowSettings::m_enableShadowCatcher, "Enable Shadow Catcher", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialEditorWindowSettings::m_enableAlternateSkybox, "Enable Alternate Skybox", "")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &MaterialEditorWindowSettings::m_fieldOfView, "Field Of View", "")
                        ->Attribute(AZ::Edit::Attributes::Min, 60.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 120.0f)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<MaterialEditorWindowSettings>("MaterialEditorWindowSettings")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "render")
                ->Constructor()
                ->Constructor<const MaterialEditorWindowSettings&>()
                ->Property("enableGrid", BehaviorValueProperty(&MaterialEditorWindowSettings::m_enableGrid))
                ->Property("enableShadowCatcher", BehaviorValueProperty(&MaterialEditorWindowSettings::m_enableShadowCatcher))
                ->Property("enableAlternateSkybox", BehaviorValueProperty(&MaterialEditorWindowSettings::m_enableAlternateSkybox))
                ->Property("fieldOfView", BehaviorValueProperty(&MaterialEditorWindowSettings::m_fieldOfView))
                ;
        }
    }
} // namespace MaterialEditor
