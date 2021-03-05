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

#include <LyShine/Bus/UiElementBus.h>
#include <AzCore/Component/Component.h>

//! \brief Mirrors the UiElementBus for use in Lua.
class UiElementLuaInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiElementLuaInterface() {}

    virtual bool IsEnabled() = 0;

    virtual void SetIsEnabled(bool isEnabled) = 0;
};

typedef AZ::EBus<UiElementLuaInterface> UiElementLuaBus;

//! \brief This component serves as the bridge between UiElementBus and UiElementLuaBus
class UiElementLuaProxy
    : public UiElementLuaBus::Handler
{
public: // member functions

    // UiElementLuaInterface

    bool IsEnabled() override;
    void SetIsEnabled(bool isEnabled) override;

    // ~UiElementLuaInterface

    //! \brief Adds this object as a handler for UiElementLuaBus
    void BusConnect(AZ::EntityId entityId);

public: // static member functions

    static void Reflect(AZ::ReflectContext* context);

private: // data

    AZ::EntityId m_targetEntity;
};
