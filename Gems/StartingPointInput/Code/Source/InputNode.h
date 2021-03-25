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

#include <Source/InputNode.generated.h>

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>

#include <StartingPointInput/InputEventNotificationBus.h>

#include <AzCore/RTTI/TypeInfo.h>

namespace StartingPointInput
{
    //////////////////////////////////////////////////////////////////////////
    /// Input handles raw input from any source and outputs Pressed, Held, and Released input events
    class InputNode
        : public ScriptCanvas::Node
        , protected InputEventNotificationBus::Handler
    {

    public:

        SCRIPTCANVAS_NODE(InputNode);

        InputNode() = default;
        ~InputNode() override = default;
        InputNode(const InputNode&) = default;
        InputNode& operator=(const InputNode&) = default;

        AZStd::string m_eventName;
        float m_value;

        //////////////////////////////////////////////////////////////////////////
        /// ScriptCanvas_Node
        void OnInputChanged(const ScriptCanvas::Datum& input, const ScriptCanvas::SlotId& slotId) override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        /// Node
        void OnPostActivate() override;
        void OnDeactivate() override;

        //////////////////////////////////////////////////////////////////////////
        /// InputEventNotificationBus::Handler
        void OnPressed(float value) override;
        void OnHeld(float value) override;
        void OnReleased(float value) override;
    };
}
