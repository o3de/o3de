/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <Window/MaterialCanvasMainWindowSettings.h>

namespace MaterialCanvas
{
    void MaterialCanvasMainWindowSettings::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MaterialCanvasMainWindowSettings, AZ::UserSettings>()
                ->Version(1)
                ->Field("mainWindowState", &MaterialCanvasMainWindowSettings::m_mainWindowState)
                ->Field("inspectorCollapsedGroups", &MaterialCanvasMainWindowSettings::m_inspectorCollapsedGroups)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<MaterialCanvasMainWindowSettings>(
                    "MaterialCanvasMainWindowSettings", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<MaterialCanvasMainWindowSettings>("MaterialCanvasMainWindowSettings")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialcanvas")
                ->Constructor()
                ->Constructor<const MaterialCanvasMainWindowSettings&>()
                ;
        }
    }
} // namespace MaterialCanvas
