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

class ViewportIcon
{
public:

    ViewportIcon(const char* textureFilename);
    virtual ~ViewportIcon();

    AZ::Vector2 GetTextureSize() const;

    void DrawImageAligned(Draw2dHelper& draw2d, AZ::Vector2& pivot, float opacity);

    void DrawImageTiled(Draw2dHelper& draw2d, IDraw2d::VertexPosColUV* verts, float opacity);

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

private:

    ITexture* m_texture;
};
