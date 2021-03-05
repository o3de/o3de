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
// code gen
#include <Source/InputNode.generated.h>

// script canvas
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/CodeGen/CodeGen.h>

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
        ScriptCanvas_Node(InputNode,
            ScriptCanvas_Node::Uuid("{0B0AC61B-4BBA-42BF-BDCD-DAF2D3CA41A8}")
            ScriptCanvas_Node::Name("Input Handler")
            ScriptCanvas_Node::Description("Handle processed input events found in input binding assets")
            ScriptCanvas_Node::Icon("Editor/Icons/Components/InputConfig.png")
            ScriptCanvas_Node::Category("Gameplay/Input")
            ScriptCanvas_Node::GraphEntryPoint(true)
        );
    public:
        InputNode() = default;
        ~InputNode() override = default;
        InputNode(const InputNode&) = default;
        InputNode& operator=(const InputNode&) = default;

        // Outputs
        ScriptCanvas_Out(ScriptCanvas_Out::Name("Pressed", "Signaled when the input event begins."));
        ScriptCanvas_Out(ScriptCanvas_Out::Name("Held", "Signaled while the input event is active."));
        ScriptCanvas_Out(ScriptCanvas_Out::Name("Released", "Signaled when the input event ends."));

        // Data
        ScriptCanvas_Property(AZStd::string, ScriptCanvas_Property::Name("Event Name", "The input event name as defined in an inputbinding asset.  Example 'Fireball'") ScriptCanvas_Property::Input);
        AZStd::string m_eventName;

        ScriptCanvas_Property(float, ScriptCanvas_Property::Name("Value", "The current value from the input.") ScriptCanvas_Property::Visibility(true) ScriptCanvas_Property::Output ScriptCanvas_Property::OutputStorageSpec);
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
} // namespace StartingPointInput
