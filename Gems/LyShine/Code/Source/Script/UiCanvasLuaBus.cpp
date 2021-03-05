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
#include "LyShine_precompiled.h"
#include "UiCanvasLuaBus.h"
#include "UiCanvasComponent.h"
#include <LyShine/Bus/UiElementBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(UiCanvasLuaProxy, "{AE6EE082-AD58-480A-8B53-E98B79F91368}");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

AZ::EntityId UiCanvasLuaProxy::FindElementById(LyShine::ElementId id)
{
    AZ::EntityId entityId;

    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
        "UiCanvasLuaBus:FindElementById is deprecated. Please use UiCanvasBus:FindElementByName instead\n");

    // Forward the call to UiCanvasBus and return its result
    AZ::Entity* element = nullptr;
    EBUS_EVENT_ID_RESULT(element, m_targetEntity, UiCanvasBus, FindElementById, id);
    if (element)
    {
        entityId = element->GetId();
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
            "UiCanvasLuaProxy:FindElementById: Couldn't find element with Id: %d\n",
            id);
    }

    return entityId;
}

AZ::EntityId UiCanvasLuaProxy::FindElementByName(const LyShine::NameType& name)
{
    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
        "UiCanvasLuaBus:FindElementByName is deprecated. Please use UiCanvasBus:FindElementByName instead\n");

    AZ::EntityId entityId;

    AZ::Entity* element = nullptr;
    EBUS_EVENT_ID_RESULT(element, m_targetEntity, UiCanvasBus, FindElementByName, name);
    if (element)
    {
        entityId = element->GetId();
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
            "UiCanvasLuaProxy:FindElementByName: Couldn't find element with name: %s\n",
            name.c_str());
    }

    return entityId;
}

bool UiCanvasLuaProxy::GetEnabled()
{
    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
        "UiCanvasLuaBus:GetEnabled is deprecated. Please use UiCanvasBus:GetEnabled instead\n");

    bool enabled = false;
    EBUS_EVENT_ID_RESULT(enabled, m_targetEntity, UiCanvasBus, GetEnabled);
    return enabled;
}

void UiCanvasLuaProxy::SetEnabled(bool enabled)
{
    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
        "UiCanvasLuaBus:SetEnabled is deprecated. Please use UiCanvasBus:SetEnabled instead\n");


    EBUS_EVENT_ID(m_targetEntity, UiCanvasBus, SetEnabled, enabled);
}

void UiCanvasLuaProxy::BusConnect(AZ::EntityId entityId)
{
    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
        "UiCanvasLuaProxy:BusConnect is deprecated. Please use the UiCanvasBus instead of the UiCanvasLuaBus and UiCanvasLuaProxy\n");

    AZ::Entity* canvasEntity = nullptr;
    EBUS_EVENT_RESULT(canvasEntity, AZ::ComponentApplicationBus, FindEntity, entityId);

    if (canvasEntity)
    {
        m_targetEntity = entityId;

        // Use this object to handle UiCanvasLuaBus calls for the given entity
        UiCanvasLuaBus::Handler::BusConnect(m_targetEntity);
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "UiCanvasLuaProxy:BusConnect: Canvas entity not found by ID\n");
    }
}

AZ::EntityId UiCanvasLuaProxy::LoadCanvas(const char* canvasFilename)
{
    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
        "UiCanvasLuaProxy:LoadCanvas is deprecated. Please use UiCanvasManagerBus:LoadCanvas instead\n");

    return gEnv->pLyShine->LoadCanvas(canvasFilename);
}

void UiCanvasLuaProxy::UnloadCanvas(AZ::EntityId canvasEntityId)
{
    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
        "UiCanvasLuaProxy:UnloadCanvas is deprecated. Please use UiCanvasManagerBus:UnloadCanvas instead\n");

    // Make sure that the entity has a canvas component
    AZ::Entity* canvasEntity = nullptr;
    EBUS_EVENT_RESULT(canvasEntity, AZ::ComponentApplicationBus, FindEntity, canvasEntityId);
    if (canvasEntity)
    {
        UiCanvasComponent* canvasComponent = canvasEntity->FindComponent<UiCanvasComponent>();
        if (canvasComponent)
        {
            gEnv->pLyShine->ReleaseCanvas(canvasEntityId, false);
        }
        else
        {
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "UiCanvasLuaProxy:UnloadCanvas: Canvas entity does not have a canvas component\n");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiCanvasLuaProxy::Reflect(AZ::ReflectContext* context)
{
    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiCanvasLuaBus>("UiCanvasLuaBus")
            ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
            ->Event("FindElementById", &UiCanvasLuaBus::Events::FindElementById)
            ->Event("FindElementByName", &UiCanvasLuaBus::Events::FindElementByName)
            ->Event("GetEnabled", &UiCanvasLuaBus::Events::GetEnabled)
            ->Event("SetEnabled", &UiCanvasLuaBus::Events::SetEnabled);

        behaviorContext->Class<UiCanvasLuaProxy>()
            ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
            ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
            ->Method("LoadCanvas", &UiCanvasLuaProxy::LoadCanvas)
            ->Method("UnloadCanvas", &UiCanvasLuaProxy::UnloadCanvas)
            ->Method("BusConnect", &UiCanvasLuaProxy::BusConnect)
        ;
    }
}
