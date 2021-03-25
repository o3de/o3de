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

#include "precompiled.h"

#include "ScriptEventsAsset.h"
#include "ScriptEventDefinition.h"
#include "ScriptEventsBus.h"

namespace ScriptEvents
{
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

    ScriptEventAssetRuntimeHandler::ScriptEventAssetRuntimeHandler(const char* displayName, const char* group, const char* extension, const AZ::Uuid& componentTypeId /*= AZ::Uuid::CreateNull()*/, AZ::SerializeContext* serializeContext /*= nullptr*/)
        : AzFramework::GenericAssetHandler<ScriptEventsAsset>(displayName, group, extension, componentTypeId, serializeContext)
    {}


}
