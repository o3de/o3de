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

#include <Atom/Document/MaterialDocumentSettings.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace MaterialEditor
{
    void MaterialDocumentSettings::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MaterialDocumentSettings, AZ::UserSettings>()
                ->Version(1)
                ->Field("showReloadDocumentPrompt", &MaterialDocumentSettings::m_showReloadDocumentPrompt)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<MaterialDocumentSettings>(
                    "MaterialDocumentSettings", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialDocumentSettings::m_showReloadDocumentPrompt, "Show Reload Document Prompt", "")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<MaterialDocumentSettings>("MaterialDocumentSettings")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "render")
                ->Constructor()
                ->Constructor<const MaterialDocumentSettings&>()
                ->Property("showReloadDocumentPrompt", BehaviorValueProperty(&MaterialDocumentSettings::m_showReloadDocumentPrompt))
                ;
        }
    }
} // namespace MaterialEditor
