/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
                ->Field("mainWindowState", &MaterialEditorWindowSettings::m_mainWindowState)
                ->Field("inspectorCollapsedGroups", &MaterialEditorWindowSettings::m_inspectorCollapsedGroups)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<MaterialEditorWindowSettings>(
                    "MaterialEditorWindowSettings", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
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
                ;
        }
    }
} // namespace MaterialEditor
