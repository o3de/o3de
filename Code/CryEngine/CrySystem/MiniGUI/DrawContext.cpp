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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "CrySystem_precompiled.h"
#include "DrawContext.h"
#include <ISystem.h>
#include <IRenderer.h>
#include <IRenderAuxGeom.h>

MINIGUI_BEGIN

//////////////////////////////////////////////////////////////////////////
CDrawContext::CDrawContext(SMetrics* pMetrics)
{
    m_currentStackLevel = 0;
    m_x = 0;
    m_y = 0;
    m_pMetrics = pMetrics;
    m_color = ColorB(0, 0, 0, 0);
    m_defaultZ = 0.0f;
    m_pAuxRender = gEnv->pRenderer->GetIRenderAuxGeom();

    m_frameWidth = (float)gEnv->pRenderer->GetWidth();
    m_frameHeight = (float)gEnv->pRenderer->GetHeight();
}

//////////////////////////////////////////////////////////////////////////
void CDrawContext::SetColor(ColorB color)
{
    m_color = color;
}

//////////////////////////////////////////////////////////////////////////
void CDrawContext::DrawLine(float x0, float y0, float x1, float y1, float thickness /*= 1.0f */)
{
    m_pAuxRender->DrawLine(Vec3(m_x + x0, m_y + y0, m_defaultZ), m_color, Vec3(m_x + x1, m_y + y1, m_defaultZ), m_color, thickness);
}

//////////////////////////////////////////////////////////////////////////
void CDrawContext::DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2)
{
    m_pAuxRender->DrawTriangle(Vec3(m_x + x0, m_y + y0, m_defaultZ), m_color, Vec3(m_x + x1, m_y + y1, m_defaultZ), m_color, Vec3(m_x + x2, m_y + y2, m_defaultZ), m_color);
}

//////////////////////////////////////////////////////////////////////////
void CDrawContext::DrawRect(const Rect& rc)
{
    m_pAuxRender->DrawTriangle(Vec3(m_x + rc.left, m_y + rc.top, m_defaultZ), m_color, Vec3(m_x + rc.left, m_y + rc.bottom, m_defaultZ), m_color, Vec3(m_x + rc.right, m_y + rc.top, m_defaultZ), m_color);
    m_pAuxRender->DrawTriangle(Vec3(m_x + rc.left, m_y + rc.bottom, m_defaultZ), m_color, Vec3(m_x + rc.right, m_y + rc.bottom, m_defaultZ), m_color, Vec3(m_x + rc.right, m_y + rc.top, m_defaultZ), m_color);
}

//////////////////////////////////////////////////////////////////////////
void CDrawContext::DrawFrame(const Rect& rc, ColorB lineColor, ColorB solidColor, float thickness)
{
    ColorB prevColor = m_color;

    SetColor(solidColor);
    DrawRect(rc);

    SetColor(lineColor);

    uint32 curFlags = m_pAuxRender->GetRenderFlags().m_renderFlags;
    m_pAuxRender->SetRenderFlags(curFlags | e_DrawInFrontOn);
    DrawLine(rc.left, rc.top, rc.right, rc.top, thickness);
    DrawLine(rc.right, rc.top, rc.right, rc.bottom, thickness);
    DrawLine(rc.left, rc.top, rc.left, rc.bottom, thickness);
    DrawLine(rc.left, rc.bottom, rc.right, rc.bottom, thickness);
    m_pAuxRender->SetRenderFlags(curFlags);

    m_color = prevColor;
}

//////////////////////////////////////////////////////////////////////////
void CDrawContext::StartDrawing()
{
    uint32 width = gEnv->pRenderer->GetWidth();
    uint32 height = gEnv->pRenderer->GetHeight();

    gEnv->pRenderer->Set2DMode(width, height, m_backupSceneMatrices);

    m_prevRenderFlags = m_pAuxRender->GetRenderFlags().m_renderFlags;
    m_pAuxRender->SetRenderFlags(e_Mode3D | e_AlphaBlended | e_FillModeSolid | e_CullModeBack | e_DepthWriteOff | e_DepthTestOff);
}

//////////////////////////////////////////////////////////////////////////
void CDrawContext::StopDrawing()
{
    // Restore old flags that where set before our draw context.
    m_pAuxRender->SetRenderFlags(m_prevRenderFlags);

    gEnv->pRenderer->Unset2DMode(m_backupSceneMatrices);
}

//////////////////////////////////////////////////////////////////////////
void CDrawContext::DrawString(float x, float y, float font_size, ETextAlign align, const char* format, ...)
{
    //text will be off screen
    if (y > m_frameHeight || x > m_frameWidth)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    SDrawTextInfo ti;
    ti.xscale = ti.yscale = font_size / 12.0f; // font size in pixels to text scale.

    ti.flags = eDrawText_Monospace | eDrawText_2D | eDrawText_FixedSize | eDrawText_IgnoreOverscan;
    if (align == eTextAlign_Left)
    {
    }
    else if (align == eTextAlign_Right)
    {
        ti.flags |= eDrawText_Right;
    }
    else if (align == eTextAlign_Center)
    {
        ti.flags |= eDrawText_Center;
    }
    ti.color[0] = (float)m_color.r / 255.0f;
    ti.color[1] = (float)m_color.g / 255.0f;
    ti.color[2] = (float)m_color.b / 255.0f;
    ti.color[3] = (float)m_color.a / 255.0f;
    gEnv->pRenderer->DrawTextQueued(Vec3(m_x + x, m_y + y, m_defaultZ), ti, format, args);
    va_end(args);
}

//////////////////////////////////////////////////////////////////////////
void CDrawContext::PushClientRect(const Rect& rc)
{
    m_currentStackLevel++;
    assert(m_currentStackLevel < MAX_ORIGIN_STACK);
    m_clientRectStack[m_currentStackLevel] = rc;
    m_x += rc.left;
    m_y += rc.top;
}

//////////////////////////////////////////////////////////////////////////
void CDrawContext::PopClientRect()
{
    if (m_currentStackLevel > 0)
    {
        Rect& rc = m_clientRectStack[m_currentStackLevel];
        m_x -= rc.left;
        m_y -= rc.top;

        m_currentStackLevel--;
    }
}
MINIGUI_END
