/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentSystemSettings.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AtomToolsFramework
{
    void AtomToolsDocumentSystemSettings::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AtomToolsDocumentSystemSettings, AZ::UserSettings>()
                ->Version(1)
                ->Field("showReloadDocumentPrompt", &AtomToolsDocumentSystemSettings::m_showReloadDocumentPrompt)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AtomToolsDocumentSystemSettings>(
                    "AtomToolsDocumentSystemSettings", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AtomToolsDocumentSystemSettings::m_showReloadDocumentPrompt, "Show Reload Document Prompt", "")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AtomToolsDocumentSystemSettings>("AtomToolsDocumentSystemSettings")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "atomtools")
                ->Constructor()
                ->Constructor<const AtomToolsDocumentSystemSettings&>()
                ->Property("showReloadDocumentPrompt", BehaviorValueProperty(&AtomToolsDocumentSystemSettings::m_showReloadDocumentPrompt))
                ;
        }
    }
} // namespace AtomToolsFramework
