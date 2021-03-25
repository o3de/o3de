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
            //! Uses the DebugDrawBus to display on-screen debug text
            class DrawTextNode
                : public Node
                , ScriptCanvasDiagnostics::DebugDrawBus::Handler
                , AZ::TickBus::Handler
            {

            public:

                SCRIPTCANVAS_NODE(DrawTextNode);

                // DebugDrawBus...
                void OnDebugDraw(IRenderer*) override;
                //

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
