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
#include <LyShine/UiBase.h>

namespace Multiplayer
{
    AZ::EntityId LoadCanvas(const char* canvasName);    

    void ReleaseCanvas(const AZ::EntityId& canvasId);

    void SetElementEnabled(const AZ::EntityId& canvasID, const char* elementName, bool enabled);   

    bool IsElementEnabled(const AZ::EntityId& canvasID, const char* elementName);

    void SetElementInputEnabled(const AZ::EntityId& canvasID, const char* elementName, bool enabled);

    void SetElementText(const AZ::EntityId& canvasID, const char* elementName, const char* text);

    LyShine::StringType GetElementText(const AZ::EntityId& canvasID, const char* elementName);

    void SetCheckBoxState(const AZ::EntityId& canvasID, const char* elementName, bool value);

    bool GetCheckBoxState(const AZ::EntityId& canvasID, const char* elementName);

    const char* GetConsoleVarValue(const char* param);

    bool GetGetConsoleVarBoolValue(const char* param);
 
    void SetConsoleVarValue(const char* param, const char* value);

    bool SetGetConsoleVarBoolValue(const char* param, bool value);
}
