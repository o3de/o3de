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

#include "precompiled.h"
#include "DrawText.h"

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <ScriptCanvas/Core/GraphBus.h>

#include <CryCommon/platform.h>
#include <CryCommon/Cry_Math.h>
#include <CryCommon/Cry_Color.h>

#include <CryCommon/IRenderer.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Debug
        {
            void DrawTextNode::OnInputSignal(const SlotId& slotId)
            {
                if (slotId == GetSlotId("Show"))
                {
                    ScriptCanvasDiagnostics::DebugDrawBus::Handler::BusConnect(this);

                    m_duration = DrawTextNodeProperty::GetDuration(this);
                    if (m_duration > 0.f && !AZ::TickBus::Handler::BusIsConnected())
                    {
                        AZ::TickBus::Handler::BusConnect();
                    }
                }
                else if (slotId == GetSlotId("Hide"))
                {
                    AZ::TickBus::Handler::BusDisconnect();
                    ScriptCanvasDiagnostics::DebugDrawBus::Handler::BusDisconnect();
                }

                SignalOutput(GetSlotId("Out"));

            }

            void DrawTextNode::OnInputChanged(const Datum&, const SlotId& slotId)
            {
                ScriptCanvas::SlotId textSlotId = DrawTextNodeProperty::GetTextSlotId(this);
                if (slotId == textSlotId)
                {
                    m_text = DrawTextNodeProperty::GetText(this);
                }
            }

            void DrawTextNode::OnTick(float deltaTime, AZ::ScriptTimePoint)
            {
                m_duration -= deltaTime;
                if (m_duration <= 0.f)
                {
                    ScriptCanvasDiagnostics::DebugDrawBus::Handler::BusDisconnect();
                    AZ::TickBus::Handler::BusDisconnect();
                }
            }

            void DrawTextNode::OnDebugDraw(IRenderer* renderer)
            {
                if (!renderer)
                {
                    return;
                }

                if (m_text.empty())
                {
                    m_text = DrawTextNodeProperty::GetText(this);
                }

                if (m_text.empty())
                {
                    return;
                }

                bool editorOnly = DrawTextNodeProperty::GetEditorOnly(this);

                bool isEditor = false;
                ScriptCanvasDiagnostics::SystemRequestBus::BroadcastResult(isEditor, &ScriptCanvasDiagnostics::SystemRequests::IsEditor);

                if (editorOnly && !isEditor)
                {
                    return;
                }

                AZ::Vector2 position = DrawTextNodeProperty::GetPosition(this);

                // Get correct coordinates
                float x = position.GetX();
                float y = position.GetY();

                if (x < 1.f || y < 1.f)
                {
                    int screenX, screenY, screenWidth, screenHeight;
                    renderer->GetViewport(&screenX, &screenY, &screenWidth, &screenHeight);
                    if (x < 1.f)
                    {
                        x *= (float)screenWidth;
                    }
                    if (y < 1.f)
                    {
                        y *= (float)screenHeight;
                    }
                }

                SDrawTextInfo ti;
                ti.xscale = ti.yscale = DrawTextNodeProperty::GetScale(this);
                ti.flags = eDrawText_2D | eDrawText_FixedSize;

                const AZ::Color& color = DrawTextNodeProperty::GetColor(this);
                ti.color[0] = color.GetR();
                ti.color[1] = color.GetG();
                ti.color[2] = color.GetB();
                ti.color[3] = color.GetA();

                ti.flags |= DrawTextNodeProperty::GetCentered(this) ? eDrawText_Center | eDrawText_CenterV : 0;

                renderer->DrawTextQueued(Vec3(x, y, 0.5f), ti, m_text.c_str());
            }
        }
    }
}
