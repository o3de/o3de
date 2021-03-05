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

//! \brief Mirrors the UiCanvasBus for use in Lua.
class UiCanvasLuaInterface
    : public AZ::ComponentBus
{
public:

    virtual ~UiCanvasLuaInterface() {}

    // UiCanvasBus

    //! This flavor of FindElementById differs slightly from the UiCanvasBus version
    //! in that it returns an EntityId, which is a bit friendly for passing around in
    //! Lua.
    //! Use of the Element Id is discouraged as it will be deprecated soon.
    virtual AZ::EntityId FindElementById(LyShine::ElementId id) = 0;

    //! This flavor of FindElementByName differs slightly from the UiCanvasBus version
    //! in that it returns an EntityId, which is a bit friendly for passing around in
    //! Lua.
    virtual AZ::EntityId FindElementByName(const LyShine::NameType& name) = 0;

    virtual bool GetEnabled() = 0;

    virtual void SetEnabled(bool enabled) = 0;

    // ~UiCanvasBus
};

typedef AZ::EBus<UiCanvasLuaInterface> UiCanvasLuaBus;

//! \brief This component serves as the bridge between UiCanvasBus and UiCanvasLuaBus
class UiCanvasLuaProxy
    : public UiCanvasLuaBus::Handler
{
public:

    // UiCanvasLuaBus

    AZ::EntityId FindElementById(LyShine::ElementId id) override;
    AZ::EntityId FindElementByName(const LyShine::NameType& name) override;
    bool GetEnabled() override;
    void SetEnabled(bool enabled) override;

    // ~UiCanvasLuaBus

    //! \brief Adds this object as a handler for UiCanvasLuaBus
    void BusConnect(AZ::EntityId entityId);

    //! \brief Loads the canvas with the given filename
    AZ::EntityId LoadCanvas(const char* canvasFilename);

    //! \brief Unloads the canvas with the given canvas entity Id
    void UnloadCanvas(AZ::EntityId canvasEntityId);

    static void Reflect(AZ::ReflectContext* context);

private:

    AZ::EntityId m_targetEntity;
};
