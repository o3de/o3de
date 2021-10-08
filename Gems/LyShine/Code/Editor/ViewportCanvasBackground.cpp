/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <LyShine/UiSerializeHelpers.h>
#include "EditorCommon.h"

ViewportCanvasBackground::ViewportCanvasBackground()
    : m_canvasBackground(new ViewportIcon("Editor/Icons/Viewport/Canvas_Background.tif"))
{
}

ViewportCanvasBackground::~ViewportCanvasBackground()
{
}

void ViewportCanvasBackground::Draw(Draw2dHelper& draw2d, const AZ::Vector2& canvasSize, float canvasToViewportScale, const AZ::Vector3& canvasToViewportTranslation)
{
    // Determine size of canvas on-screen by applying the current canvas-to-viewport scale
    const int scaledCanvasWidth = aznumeric_cast<int>(canvasSize.GetX() * canvasToViewportScale);
    const int scaledCanvasHeight = aznumeric_cast<int>(canvasSize.GetY() * canvasToViewportScale);

    // Take on-screen canvas panning/translation into account
    const float xCanvasPanOffset = canvasToViewportTranslation.GetX();
    const float yCanvasPanOffset = canvasToViewportTranslation.GetY();

    AZ::Vector2 topLeft(xCanvasPanOffset, yCanvasPanOffset);
    AZ::Vector2 topRight(xCanvasPanOffset + scaledCanvasWidth, yCanvasPanOffset);
    AZ::Vector2 bottomRight(xCanvasPanOffset + scaledCanvasWidth, yCanvasPanOffset + scaledCanvasHeight);
    AZ::Vector2 bottomLeft(xCanvasPanOffset, yCanvasPanOffset + scaledCanvasHeight);

    // points are a clockwise quad
    static const unsigned int quadVertCount = 4;
    AZ::Vector2 positions[quadVertCount] =
    {
        topLeft,
        topRight,
        bottomRight,
        bottomLeft
    };

    // scale UV's so that one texel is one pixel on screen
    AZ::Vector2 textureSize(m_canvasBackground->GetTextureSize());
    AZ::Vector2 rectSize(aznumeric_cast<float>(scaledCanvasWidth), aznumeric_cast<float>(scaledCanvasHeight));
    AZ::Vector2 uvScale(rectSize.GetX() / textureSize.GetX(), rectSize.GetY() / textureSize.GetY());

    // now draw the same as Stretched but with UV's adjusted
    const AZ::Vector2 uvs[4] = { AZ::Vector2(0, 0), AZ::Vector2(uvScale.GetX(), 0), AZ::Vector2(uvScale.GetX(), uvScale.GetY()), AZ::Vector2(0, uvScale.GetY()) };
    AZ::Color colorWhite(1.0f, 1.0f, 1.0f, 1.0f);
    CDraw2d::VertexPosColUV verts[4];
    for (int i = 0; i < 4; ++i)
    {
        verts[i].position = positions[i];
        verts[i].color = colorWhite;
        verts[i].uv = uvs[i];
    }

    m_canvasBackground->DrawImageTiled(draw2d, verts);
}
