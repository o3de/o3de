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
#include "UiElementLuaBus.h"
#include <LyShine/Bus/UiElementBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(UiElementLuaProxy, "{77FDAF4D-23B5-4004-8679-14AA1BBC7B5E}");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

bool UiElementLuaProxy::IsEnabled()
{
    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
        "UiElementLuaProxy:IsEnabled is deprecated. Please use UiElementBus:IsEnabled instead\n");

    bool isEnabled = false;
    EBUS_EVENT_ID_RESULT(isEnabled, m_targetEntity, UiElementBus, IsEnabled);
    return isEnabled;
}

void UiElementLuaProxy::SetIsEnabled(bool isEnabled)
{
    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
        "UiElementLuaProxy:SetIsEnabled is deprecated. Please use UiElementBus:SetIsEnabled instead\n");

    EBUS_EVENT_ID(m_targetEntity, UiElementBus, SetIsEnabled, isEnabled);
}

void UiElementLuaProxy::BusConnect(AZ::EntityId entityId)
{
    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
        "UiElementLuaProxy:BusConnect is deprecated. Please use the UiElement bus directly instead\n");

    AZ::Entity* elementEntity = nullptr;
    EBUS_EVENT_RESULT(elementEntity, AZ::ComponentApplicationBus, FindEntity, entityId);

    if (elementEntity)
    {
        m_targetEntity = entityId;

        // Use this object to handle UiElementLuaBus calls for the given entity
        UiElementLuaBus::Handler::BusConnect(m_targetEntity);
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "UiElementLuaProxy:BusConnect: Element entity not found by ID\n");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiElementLuaProxy::Reflect(AZ::ReflectContext* context)
{
    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->Class<UiElementLuaProxy>("UiElementLuaProxy")
            ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
            ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
            ->Method("BusConnect", &UiElementLuaProxy::BusConnect)
            ->Method("IsEnabled", &UiElementLuaProxy::IsEnabled)
            ->Method("SetIsEnabled", &UiElementLuaProxy::SetIsEnabled)
        ;
    }
}
