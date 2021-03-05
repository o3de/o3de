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

#include <AzCore/Component/TickBus.h>

#include <ScriptCanvas/CodeGen/CodeGen.h>
#include <ScriptCanvas/Core/Node.h>

#include <Source/DrawText.generated.h>

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Color.h>

#include <ScriptCanvasDiagnosticLibrary/DebugDrawBus.h>

struct IRenderer;

namespace AZ
{
    class Color;
    class Vector2;
}

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Debug
        {
            class DrawTextNode
                : public Node
                , ScriptCanvasDiagnostics::DebugDrawBus::Handler
                , AZ::TickBus::Handler
            {
            public:
                ScriptCanvas_Node(DrawTextNode,
                    ScriptCanvas_Node::Name("Draw Text", "Displays text on the viewport.")
                    ScriptCanvas_Node::Uuid("{AA209CEC-3813-4DC2-85A9-DE8B7A905CD6}")
                    ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/DrawText.png")
                    ScriptCanvas_Node::Category("Utilities/Debug")
                    ScriptCanvas_Node::Version(1));

                // DebugDrawBus
                void OnDebugDraw(IRenderer*) override;
                //

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("Show", "Shows the debug text on the viewport"));
                ScriptCanvas_In(ScriptCanvas_In::Name("Hide", "Removed the debug text from the viewport"));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", ""));

                // Properties
                ScriptCanvas_Property(AZStd::string,
                    ScriptCanvas_Property::Name("Text", "The text to display on screen.")
                    ScriptCanvas_Property::Input
                );

                ScriptCanvas_PropertyWithDefaults(AZ::Vector2, AZ::Vector2(20.f, 20.f), 
                    ScriptCanvas_Property::Name("Position", "Position of the text on-screen in normalized coordinates [0-1].")
                    ScriptCanvas_Property::Input
                );

                ScriptCanvas_PropertyWithDefaults(AZ::Color, AZ::Color(1.f, 1.f, 1.f, 1.f),
                    ScriptCanvas_Property::Name("Color", "Color of the text.")
                    ScriptCanvas_Property::Input
                );

                ScriptCanvas_PropertyWithDefaults(float, 0.f,
                    ScriptCanvas_Property::Name("Duration", "If greater than zero, the text will disappear after this duration (in seconds).")
                    ScriptCanvas_Property::Input
                );

                ScriptCanvas_PropertyWithDefaults(float, 1.f,
                    ScriptCanvas_Property::Name("Scale", "Scales the font size of the text.")
                    ScriptCanvas_Property::Input
                );

                ScriptCanvas_PropertyWithDefaults(bool, false,
                    ScriptCanvas_Property::Name("Centered", "Centers the text around the specfied coordinates.")
                    ScriptCanvas_Property::Input
                );

                ScriptCanvas_PropertyWithDefaults(bool, false,
                    ScriptCanvas_Property::Name("Editor Only", "Only displays this text if the game is being run in-editor, not on launchers.")
                    ScriptCanvas_Property::Input
                );

            protected:

                void OnInputSignal(const SlotId& slotId) override;
                void OnInputChanged(const Datum& input, const SlotId& slotID) override;

                void OnTick(float deltaTime, AZ::ScriptTimePoint) override;

            private:

                AZStd::string m_text;
                float m_duration;

            };
        }
    }
}
