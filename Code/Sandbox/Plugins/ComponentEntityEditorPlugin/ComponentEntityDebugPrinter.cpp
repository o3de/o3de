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
#include "ComponentEntityEditorPlugin_precompiled.h"
#include "ComponentEntityDebugPrinter.h"

#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

ComponentEntityDebugPrinter::ComponentEntityDebugPrinter()
{
    AZ::TickBus::Handler::BusConnect();
}

void ComponentEntityDebugPrinter::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
{
    if (!GetIEditor()->GetRenderer())
    {
        return;
    }

    ICVar* displayInfo = GetIEditor()->GetSystem()->GetIConsole()->GetCVar("r_DisplayInfo");
    if (!displayInfo || displayInfo->GetIVal() == 0)
    {
        return;
    }

    float x = 2.f;
    float y = 2.f;

    SDrawTextInfo textInfo;
    textInfo.xscale = 1.25f;
    textInfo.yscale = textInfo.xscale;
    textInfo.flags = eDrawText_2D | eDrawText_FixedSize | eDrawText_800x600 | eDrawText_Monospace;

    // Figure out whether we're querying the Game or Editor entity context
    AzFramework::EntityContextId entityContextId = AzFramework::EntityContextId::CreateNull();
    if (GetIEditor()->IsInGameMode())
    {
        AzFramework::GameEntityContextRequestBus::BroadcastResult(entityContextId, &AzFramework::GameEntityContextRequestBus::Events::GetGameEntityContextId);
    }
    else
    {
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(entityContextId, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetEditorEntityContextId);
    }
    if (entityContextId.IsNull())
    {
        return;
    }

    // Print the number of entities in the level
    AZ::SliceComponent* rootSlice = nullptr;
    AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(rootSlice, entityContextId,
        &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::GetRootSlice);
    if (rootSlice)
    {
        size_t numEntities = rootSlice->GetInstantiatedEntityCount();
        if (numEntities > 0)
        {
            GetIEditor()->GetRenderer()->DrawTextQueued(Vec3(x, y, 0), textInfo, AZStd::string::format("Entities: %zu", numEntities).c_str());
        }
    }
}