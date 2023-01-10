/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCanvasNotificationLuaBus.h"
#include <LyShine/Bus/UiElementBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(UiCanvasNotificationLuaProxy, "{9AD2B735-23AA-44F8-A51F-5F9A6BA25224}");
}

// BehaviorContext UiCanvasNotificationLuaBus forwarder
class BehaviorUiCanvasNotificationLuaBusHandler
    : public UiCanvasNotificationLuaBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(BehaviorUiCanvasNotificationLuaBusHandler, "{452E5C9A-CFEA-4F91-A6E8-CF427F8D56EF}", AZ::SystemAllocator, OnAction);

    void OnAction(AZ::EntityId entityId, const char* actionName) override
    {
        Call(FN_OnAction, entityId, actionName);
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasNotificationLuaProxy::UiCanvasNotificationLuaProxy()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasNotificationLuaProxy::~UiCanvasNotificationLuaProxy()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasNotificationLuaProxy::OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName)
{
    // Forward the entity's UiCanvasNotification OnAction event to the Lua-wrapped bus (this
    // will execute the Lua script's OnAction).
    UiCanvasNotificationLuaBus::Event(m_targetEntity, &UiCanvasNotificationLuaBus::Events::OnAction, entityId, actionName.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasNotificationLuaProxy::BusConnect(AZ::EntityId entityId)
{
    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
        "UiCanvasNotificationLuaProxy:BusConnect is deprecated. Please use the UiCanvasNotificationBus directly instead\n");

    AZ::Entity* canvasEntity = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(canvasEntity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);

    if (canvasEntity)
    {
        m_targetEntity = entityId;

        // This helper component will receive the OnAction broadcast for the entity ID
        // on its UiCanvasNotificationBus. Then, the OnAction will be forwarded to the
        // Lua-wrapped bus, and ultimately end up calling the Lua script.
        UiCanvasNotificationBus::Handler::BusConnect(m_targetEntity);
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "UiCanvasNotificationLuaProxy:BusConnect: Canvas entity not found by ID\n");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiCanvasNotificationLuaProxy::Reflect(AZ::ReflectContext* context)
{
    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiCanvasNotificationLuaBus>("UiCanvasNotificationLuaBus")
            ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
            ->Handler<BehaviorUiCanvasNotificationLuaBusHandler>();

        behaviorContext->Class<UiCanvasNotificationLuaProxy>()
            ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
            ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
            ->Method("BusConnect", &UiCanvasNotificationLuaProxy::BusConnect)
        ;
    }
}
