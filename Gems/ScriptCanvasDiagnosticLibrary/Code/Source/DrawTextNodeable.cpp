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
#include "DrawTextNodeable.h"

#if 0

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Utils/SerializationUtils.h>

#include <CryCommon/platform.h>
#include <CryCommon/Cry_Math.h>
#include <CryCommon/Cry_Color.h>

#include <CryCommon/IRenderer.h>

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Debug
        {
            DrawTextNodeable::DrawTextNodeable()
            {
                ScriptCanvasDiagnostics::SystemRequestBus::BroadcastResult(m_isEditor, &ScriptCanvasDiagnostics::SystemRequests::IsEditor);
            }

            DrawTextNodeable::~DrawTextNodeable()
            {
                Hide();
            }

            void DrawTextNodeable::OnDeactivate()
            {
                Hide();
            }

            void DrawTextNodeable::Show(AZStd::string text, AZ::Vector2 position, AZ::Color color, float duration, float scale, Data::BooleanType centered, Data::BooleanType editorOnly)
            {
                if (text.empty())
                {
                    return;
                }

                if (editorOnly && !m_isEditor)
                {
                    return;
                }

                ScriptCanvasDiagnostics::DebugDrawBus::Handler::BusConnect(this);

                m_text = text;
                m_position = position;
                m_color = color;
                m_duration = duration;
                m_scale = scale;
                m_centered = centered;
                m_editorOnly = editorOnly;

                if (m_duration > 0.f && !AZ::TickBus::Handler::BusIsConnected())
                {
                    AZ::TickBus::Handler::BusConnect();
                }
            }

            void DrawTextNodeable::Hide()
            {
                AZ::TickBus::Handler::BusDisconnect();
                ScriptCanvasDiagnostics::DebugDrawBus::Handler::BusDisconnect();
            }

            void DrawTextNodeable::OnTick(float deltaTime, AZ::ScriptTimePoint)
            {
                m_duration -= deltaTime;
                if (m_duration <= 0.f)
                {
                    ScriptCanvasDiagnostics::DebugDrawBus::Handler::BusDisconnect();
                    AZ::TickBus::Handler::BusDisconnect();
                }
            }

            void DrawTextNodeable::OnDebugDraw(IRenderer* renderer)
            {
                if (!renderer)
                {
                    return;
                }

                // Get correct coordinates
                float x = m_position.GetX();
                float y = m_position.GetY();

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
                ti.xscale = ti.yscale = m_scale;
                ti.flags = eDrawText_2D | eDrawText_FixedSize;

                ti.color[0] = m_color.GetR();
                ti.color[1] = m_color.GetG();
                ti.color[2] = m_color.GetB();
                ti.color[3] = m_color.GetA();

                ti.flags |= m_centered ? eDrawText_Center | eDrawText_CenterV : 0;

                renderer->DrawTextQueued(Vec3(x, y, 0.5f), ti, m_text.c_str());
            }
        }
    }
}

#include <Source/DrawTextNodeable.generated.cpp>


#endif
