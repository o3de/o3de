/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
                ->Field("defaultMaterialTypeName", &MaterialDocumentSettings::m_defaultMaterialTypeName)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<MaterialDocumentSettings>(
                    "MaterialDocumentSettings", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialDocumentSettings::m_defaultMaterialTypeName, "Default Material Type Name", "")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<MaterialDocumentSettings>("MaterialDocumentSettings")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialeditor")
                ->Constructor()
                ->Constructor<const MaterialDocumentSettings&>()
                ->Property("defaultMaterialTypeName", BehaviorValueProperty(&MaterialDocumentSettings::m_defaultMaterialTypeName))
                ;
        }
    }
} // namespace MaterialEditor
