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
#pragma once

#include <LyShine/Bus/UiCanvasBus.h>
#include <AzCore/Component/Component.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! \brief Defines the Lua-specific variant of UiCanvasNotificationBus
class UiCanvasNotificationLuaInterface
    : public AZ::ComponentBus
{
public:

    virtual ~UiCanvasNotificationLuaInterface() {}

    // UiCanvasNotificationBus

    virtual void OnAction(AZ::EntityId entityId, const char* actionName) = 0;

    // ~UiCanvasNotificationBus
};

typedef AZ::EBus<UiCanvasNotificationLuaInterface> UiCanvasNotificationLuaBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! \brief Listens for UiCanvasNotificationBus actions and forwards the calls to the Lua-specific version
//!
//! For this to work, the Lua script must create a UiCanvasNotificationLuaProxy object and call BusConnect,
//! passing the entity ID of the entity they want to listen for action notifications from. For example:
//! self.uiCanvasNotificationLuaProxy = UiCanvasNotificationLuaProxy();
//! self.uiCanvasNotificationLuaProxy:BusConnect(canvasEntityId);
class UiCanvasNotificationLuaProxy
    : public UiCanvasNotificationBus::Handler
{
public: // member functions

    UiCanvasNotificationLuaProxy();
    ~UiCanvasNotificationLuaProxy() override;

    // UiCanvasNotificationBus

    void OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName) override;

    // ~UiCanvasNotificationBus

    //! \brief Connects to the given entity's canvas notifications to forward to Lua
    void BusConnect(AZ::EntityId entityId);

    static void Reflect(AZ::ReflectContext* context);

private: // members

    AZ::EntityId m_targetEntity;
};
