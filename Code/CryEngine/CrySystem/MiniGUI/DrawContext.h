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

// Description : DrawContext helper class for MiniGUI


#ifndef CRYINCLUDE_CRYSYSTEM_MINIGUI_DRAWCONTEXT_H
#define CRYINCLUDE_CRYSYSTEM_MINIGUI_DRAWCONTEXT_H
#pragma once


#include "ICryMiniGUI.h"
#include <Cry_Color.h>

struct IRenderAuxGeom;

MINIGUI_BEGIN

enum ETextAlign
{
    eTextAlign_Left,
    eTextAlign_Right,
    eTextAlign_Center
};

//////////////////////////////////////////////////////////////////////////
// Context of MiniGUI drawing.
//////////////////////////////////////////////////////////////////////////
class CDrawContext
{
public:
    CDrawContext(SMetrics* pMetrics);

    // Must be called before any drawing happens
    void StartDrawing();
    // Must be called after all drawing have been complete.
    void StopDrawing();

    void PushClientRect(const Rect& rc);
    void PopClientRect();

    SMetrics& Metrics() { return *m_pMetrics; }
    void SetColor(ColorB color);

    void DrawLine(float x0, float y0, float x1, float y1, float thickness = 1.0f);
    void DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2);
    void DrawRect(const Rect& rc);
    void DrawFrame(const Rect& rc, ColorB lineColor, ColorB solidColor, float thickness = 1.0f);

    void DrawString(float x, float y, float font_size, ETextAlign align, const char* format, ...);


protected:
    SMetrics* m_pMetrics;

    ColorB m_color;
    float m_defaultZ;
    IRenderAuxGeom* m_pAuxRender;
    uint32 m_prevRenderFlags;

    enum
    {
        MAX_ORIGIN_STACK = 16
    };

    int m_currentStackLevel;
    float m_x, m_y; // Reference X,Y positions
    Rect m_clientRectStack[MAX_ORIGIN_STACK];

    float m_frameWidth;
    float m_frameHeight;

private:
    TransformationMatrices m_backupSceneMatrices;
};

MINIGUI_END

#endif // CRYINCLUDE_CRYSYSTEM_MINIGUI_DRAWCONTEXT_H
