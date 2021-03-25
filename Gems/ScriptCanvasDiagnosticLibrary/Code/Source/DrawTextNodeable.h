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

#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Core/NodeableNode.h>
#include <ScriptCanvas/CodeGen/NodeableCodegen.h>

#include <ScriptCanvasDiagnosticLibrary/DebugDrawBus.h>
#if 0
#include <Source/DrawTextNodeable.generated.h>

struct IRenderer;

namespace AZ
{
    class Color;
    class Vector2;
}

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Debug
        {
            class DrawTextNodeable
                : public ScriptCanvas::Nodeable
                , public ScriptCanvasDiagnostics::DebugDrawBus::Handler
                , public AZ::TickBus::Handler
            {
                NodeDefinition(DrawTextNodeable, "Draw Text", "Displays text on the viewport.",
                    NodeTags::Icon("Editor/Icons/ScriptCanvas/DrawTextNode.png"),
                    NodeTags::Category("Utilities/Debug"),
                    NodeTags::Version(0)
                );

            public:
                DrawTextNodeable();
                virtual ~DrawTextNodeable();

                // TickBus
                void OnTick(float deltaTime, AZ::ScriptTimePoint) override;

                // DebugDrawBus
                void OnDebugDraw(IRenderer*) override;

                InputMethod("Show", "Shows the debug text on the viewport")
                void Show(AZStd::string text, AZ::Vector2 position, AZ::Color color, float duration, float scale, Data::BooleanType centered, Data::BooleanType editorOnly);
                DataInput(AZStd::string, "Text", "", "The text to display on screen.", SlotTags::DisplayGroup("Show"));
                DataInput(AZ::Vector2, "Position", AZ::Vector2(20.f, 20.f), "Position of the text on-screen in normalized coordinates [0-1].", SlotTags::DisplayGroup("Show"));
                DataInput(AZ::Color, "Color", AZ::Color(1.f, 1.f, 1.f, 1.f), "Color of the text.", SlotTags::DisplayGroup("Show"));
                DataInput(float, "Duration", 0.f, "If greater than zero, the text will disappear after this duration (in seconds).", SlotTags::DisplayGroup("Show"));
                DataInput(float, "Scale", 1.f, "Scales the font size of the text.", SlotTags::DisplayGroup("Show"));
                DataInput(Data::BooleanType, "Centered", false, "Centers the text around the specfied coordinates.", SlotTags::DisplayGroup("Show"));
                DataInput(Data::BooleanType, "Editor Only", false, "Only displays this text if the game is being run in-editor, not on launchers.", SlotTags::DisplayGroup("Show"));

                InputMethod("Hide", "Removed the debug text from the viewport")
                void Hide();

            protected:
                void OnDeactivate() override;

            private:
                AZStd::string m_text;
                AZ::Vector2 m_position;
                AZ::Color m_color;
                float m_duration;
                float m_scale;
                bool m_centered;
                bool m_editorOnly;

                bool m_isEditor = false;

            };
        }
    }
}
#endif 
