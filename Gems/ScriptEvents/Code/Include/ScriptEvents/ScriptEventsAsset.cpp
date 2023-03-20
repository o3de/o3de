/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptEventsAsset.h"
#include "ScriptEventDefinition.h"
#include "ScriptEventsBus.h"

#include <AzCore/Serialization/EditContext.h>

namespace ScriptEvents
{
    void ScriptEventsAsset::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ScriptEventsAsset>()
                ->Version(1)
                ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                ->Field("m_definition", &ScriptEventsAsset::m_definition)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ScriptEventsAsset>("Script Events Asset", "")
                    ->DataElement(0, &ScriptEventsAsset::m_definition, "Definition", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    void ScriptEventsAssetPtr::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext * serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ScriptEventsAssetPtr>()
                ;
        }
    }

    void ScriptEventAssetRuntimeHandler::InitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset, bool loadStageSucceeded, bool isReload)
    {
        AssetHandler::InitAsset(asset, loadStageSucceeded, isReload);

        if (loadStageSucceeded && !isReload)
        {
            const ScriptEvents::ScriptEvent& definition = asset.GetAs<ScriptEventsAsset>()->m_definition;
            AZStd::intrusive_ptr<Internal::ScriptEventRegistration> scriptEvent;
            ScriptEvents::ScriptEventBus::BroadcastResult(scriptEvent, &ScriptEvents::ScriptEventRequests::RegisterScriptEvent, asset.GetId(), definition.GetVersion());
        }
    }

    ScriptEventAssetRuntimeHandler::ScriptEventAssetRuntimeHandler
        ( const char* displayName
        , const char* group
        , const char* extension
        , const AZ::Uuid& componentTypeId /*= AZ::Uuid::CreateNull()*/
        , AZ::SerializeContext* serializeContext /*= nullptr*/)
        : AzFramework::GenericAssetHandler<ScriptEventsAsset>(displayName, group, extension, componentTypeId, serializeContext)
    {}
}
