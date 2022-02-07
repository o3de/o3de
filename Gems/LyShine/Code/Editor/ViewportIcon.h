/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Image/Image.h>

class ViewportIcon
{
public:

    ViewportIcon(const char* textureFilename);
    virtual ~ViewportIcon();

    AZ::Vector2 GetTextureSize() const;

    void DrawImageAligned(Draw2dHelper& draw2d, AZ::Vector2& pivot, float opacity);

    void DrawImageTiled(Draw2dHelper& draw2d, CDraw2d::VertexPosColUV* verts);

    void Draw(Draw2dHelper& draw2d, AZ::Vector2 anchorPos, const AZ::Matrix4x4& transform, float iconRot = 0.0f, AZ::Color color = AZ::Color(1.0f, 1.0f, 1.0f, 1.0f)) const;

    void DrawAxisAlignedBoundingBox(Draw2dHelper& draw2d, AZ::Vector2 bound0, AZ::Vector2 bound1);

    // draw two orthogonal lines that form an L shape from the anchor pos to the target pos on the element
    void DrawAnchorLines(Draw2dHelper& draw2d, AZ::Vector2 anchorPos, AZ::Vector2 targetPos, const AZ::Matrix4x4& transform,
        bool xFirst, bool xText, bool yText);

    // the distance line is the segment of the anchor lines that has the distance displayed on it
    void DrawDistanceLine(Draw2dHelper& draw2d, AZ::Vector2 start, AZ::Vector2 end, float displayDistance, const char* suffix = nullptr);

    // draw two orthogonal lines that form an L or T shape from the two anchors to the target pos on the element
    void DrawAnchorLinesSplit(Draw2dHelper& draw2d, AZ::Vector2 anchorPos1, AZ::Vector2 anchorPos2,
        AZ::Vector2 targetPos, const AZ::Matrix4x4& transform, bool horizSplit, const char* suffix = nullptr);

    // draw a distance line given untransformed points
    void DrawDistanceLineWithTransform(Draw2dHelper& draw2d, AZ::Vector2 sourcePos, AZ::Vector2 targetPos, const AZ::Matrix4x4& transform,
        float value, const char* suffix = nullptr);

    // Draw a rectangle around an element using this icon's texture, The height of the texture is the
    // width of the border (but the texture can have alpha at edges to make it thinner).
    void DrawElementRectOutline(Draw2dHelper& draw2d, AZ::EntityId entityId, AZ::Color color);

    // Set whether to apply high resolution dpi scaling to the icon size
    void SetApplyDpiScaleFactorToSize(bool apply) { m_applyDpiScaleFactorToSize = apply; }

    // Get whether to apply high resolution dpi scaling to the icon size
    bool GetApplyDpiScaleFactorToSize() { return m_applyDpiScaleFactorToSize; }

    // Set scale factor
    static void SetDpiScaleFactor(float scale) { m_dpiScaleFactor = scale; }

    // Get scale factor
    static float GetDpiScaleFactor() { return m_dpiScaleFactor; }

private:
    AZ::Data::Instance<AZ::RPI::Image> m_image;
    bool m_applyDpiScaleFactorToSize = true;

    static float m_dpiScaleFactor;
};
