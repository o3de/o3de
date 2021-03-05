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

#include "Multiplayer_precompiled.h"
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiInteractableBus.h>
#include <LyShine/Bus/UiTextBus.h>
#include <LyShine/Bus/UiTextInputBus.h>
#include <LyShine/Bus/UiCheckboxBus.h>
#include <LyShine/Bus/UiCanvasManagerBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <IConsole.h>
#include <Source/Canvas/MultiplayerCanvasHelper.h>

namespace Multiplayer
{
    AZ::EntityId LoadCanvas(const char* canvasName)
    {
        AZ::EntityId canvasEntityId;
        EBUS_EVENT_RESULT(canvasEntityId, UiCanvasManagerBus, LoadCanvas, canvasName);
        return canvasEntityId;
    }

    void ReleaseCanvas(const AZ::EntityId& canvasId)
    {
        if (canvasId.IsValid())
        {
            EBUS_EVENT(UiCanvasManagerBus, UnloadCanvas, canvasId);
        }
    }

    void SetElementEnabled(const AZ::EntityId& canvasID, const char* elementName, bool enabled)
    {
        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element, canvasID, UiCanvasBus, FindElementByName, elementName);

        if (element != nullptr)
        {
            EBUS_EVENT_ID(element->GetId(), UiElementBus, SetIsEnabled, enabled);
        }
    }

    bool IsElementEnabled(const AZ::EntityId& canvasID, const char* elementName)
    {
        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element, canvasID, UiCanvasBus, FindElementByName, elementName);
        bool enabled  = false;
        if (element != nullptr)
        {
            EBUS_EVENT_ID_RESULT(enabled, element->GetId(), UiElementBus, IsEnabled);
        }
        return enabled;
    }

    void SetElementInputEnabled(const AZ::EntityId& canvasID, const char* elementName, bool enabled)
    {
        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element, canvasID, UiCanvasBus, FindElementByName, elementName);

        if (element != nullptr)
        {
            EBUS_EVENT_ID(element->GetId(), UiInteractableBus, SetIsHandlingEvents, enabled);
        }
    }

    void SetElementText(const AZ::EntityId& canvasID, const char* elementName, const char* text)
    {
        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element, canvasID, UiCanvasBus, FindElementByName, elementName);

        if (element != nullptr)
        {
            if (UiTextInputBus::FindFirstHandler(element->GetId()))
            {
                EBUS_EVENT_ID(element->GetId(), UiTextInputBus, SetText, text);
            }
            else if (UiTextBus::FindFirstHandler(element->GetId()))
            {
                EBUS_EVENT_ID(element->GetId(), UiTextBus, SetText, text);
            }
        }
    }

    LyShine::StringType GetElementText(const AZ::EntityId& canvasID, const char* elementName)
    {
        LyShine::StringType retVal;
        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element, canvasID, UiCanvasBus, FindElementByName, elementName);

        if (element != nullptr)
        {
            if (UiTextInputBus::FindFirstHandler(element->GetId()))
            {
                EBUS_EVENT_ID_RESULT(retVal, element->GetId(), UiTextInputBus, GetText);
            }
            else if (UiTextBus::FindFirstHandler(element->GetId()))
            {
                EBUS_EVENT_ID_RESULT(retVal, element->GetId(), UiTextBus, GetText);
            }
        }

        return retVal;
    }

    void SetCheckBoxState(const AZ::EntityId& canvasID, const char* elementName, bool value)
    {
        bool retVal = false;;
        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element, canvasID, UiCanvasBus, FindElementByName, elementName);
        if (element != nullptr)
        {
            EBUS_EVENT_ID(element->GetId(), UiCheckboxBus, SetState, value);
        }
    }

    bool GetCheckBoxState(const AZ::EntityId& canvasID, const char* elementName)
    {
        bool retVal = false;;
        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element, canvasID, UiCanvasBus, FindElementByName, elementName);
        if (element != nullptr)
        {
            EBUS_EVENT_ID_RESULT(retVal, element->GetId(), UiCheckboxBus, GetState);
        }
        return retVal;
    }

    const char* GetConsoleVarValue(const char* param)
    {
        ICVar* cvar = gEnv->pConsole->GetCVar(param);

        if (cvar)
        {
            return cvar->GetString();
        }

        return "";
    }


    bool GetGetConsoleVarBoolValue(const char* param)
    {
        bool value = false;
        ICVar* cvar = gEnv->pConsole->GetCVar(param);

        if (cvar)
        {
            if (cvar->GetI64Val())
            {
                value = true;
            }
        }

        return value;
    }

    void SetConsoleVarValue(const char* param, const char* value)
    {
        ICVar* cvar = gEnv->pConsole->GetCVar(param);

        if (cvar)
        {
            cvar->Set(value);
        }
    }

    bool SetGetConsoleVarBoolValue(const char* param, bool value)
    {
        ICVar* cvar = gEnv->pConsole->GetCVar(param);

        if (cvar)
        {
           value ? cvar->Set(1) : cvar->Set(0);
        }

        return value;
    }
}
