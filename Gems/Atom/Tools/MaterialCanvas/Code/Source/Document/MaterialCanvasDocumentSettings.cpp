/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <Document/MaterialCanvasDocumentSettings.h>

namespace MaterialCanvas
{
    void MaterialCanvasDocumentSettings::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MaterialCanvasDocumentSettings, AZ::UserSettings>()
                ->Version(1)
                ->Field("defaultMaterialTypeName", &MaterialCanvasDocumentSettings::m_defaultMaterialTypeName)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<MaterialCanvasDocumentSettings>(
                    "MaterialCanvasDocumentSettings", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialCanvasDocumentSettings::m_defaultMaterialTypeName, "Default Material Type Name", "")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<MaterialCanvasDocumentSettings>("MaterialCanvasDocumentSettings")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialcanvas")
                ->Constructor()
                ->Constructor<const MaterialCanvasDocumentSettings&>()
                ->Property("defaultMaterialTypeName", BehaviorValueProperty(&MaterialCanvasDocumentSettings::m_defaultMaterialTypeName))
                ;
        }
    }
} // namespace MaterialCanvas
