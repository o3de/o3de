/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
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
    UiCanvasBus::EventResult(element, m_targetEntity, &UiCanvasBus::Events::FindElementById, id);
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
    UiCanvasBus::EventResult(element, m_targetEntity, &UiCanvasBus::Events::FindElementByName, name);
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
    UiCanvasBus::EventResult(enabled, m_targetEntity, &UiCanvasBus::Events::GetEnabled);
    return enabled;
}

void UiCanvasLuaProxy::SetEnabled(bool enabled)
{
    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
        "UiCanvasLuaBus:SetEnabled is deprecated. Please use UiCanvasBus:SetEnabled instead\n");


    UiCanvasBus::Event(m_targetEntity, &UiCanvasBus::Events::SetEnabled, enabled);
}

void UiCanvasLuaProxy::BusConnect(AZ::EntityId entityId)
{
    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
        "UiCanvasLuaProxy:BusConnect is deprecated. Please use the UiCanvasBus instead of the UiCanvasLuaBus and UiCanvasLuaProxy\n");

    AZ::Entity* canvasEntity = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(canvasEntity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);

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

    return AZ::Interface<ILyShine>::Get()->LoadCanvas(canvasFilename);
}

void UiCanvasLuaProxy::UnloadCanvas(AZ::EntityId canvasEntityId)
{
    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
        "UiCanvasLuaProxy:UnloadCanvas is deprecated. Please use UiCanvasManagerBus:UnloadCanvas instead\n");

    // Make sure that the entity has a canvas component
    AZ::Entity* canvasEntity = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(canvasEntity, &AZ::ComponentApplicationBus::Events::FindEntity, canvasEntityId);
    if (canvasEntity)
    {
        UiCanvasComponent* canvasComponent = canvasEntity->FindComponent<UiCanvasComponent>();
        if (canvasComponent)
        {
            AZ::Interface<ILyShine>::Get()->ReleaseCanvas(canvasEntityId, false);
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
