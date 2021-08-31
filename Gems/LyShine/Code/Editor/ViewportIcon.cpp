/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include <LyShine/Draw2d.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

float ViewportIcon::m_dpiScaleFactor = 1.0f;

ViewportIcon::ViewportIcon(const char* textureFilename)
{
    m_image = CDraw2d::LoadTexture(textureFilename);
}

ViewportIcon::~ViewportIcon()
{
}

AZ::Vector2 ViewportIcon::GetTextureSize() const
{
    if (m_image)
    {
        AZ::RHI::Size size = m_image->GetDescriptor().m_size;
        AZ::Vector2 scaledSize(static_cast<float>(size.m_width), static_cast<float>(size.m_height));
        if (m_applyDpiScaleFactorToSize)
        {
            scaledSize *= m_dpiScaleFactor;
        }
        return scaledSize;
    }

    return AZ::Vector2(0.0f, 0.0f);
}

void ViewportIcon::DrawImageAligned(Draw2dHelper& draw2d, AZ::Vector2& pivot, float opacity)
{
    draw2d.DrawImageAligned(m_image,
        pivot,
        GetTextureSize(),
        IDraw2d::HAlign::Center,
        IDraw2d::VAlign::Center,
        opacity);
}

void ViewportIcon::DrawImageTiled(Draw2dHelper& draw2d, CDraw2d::VertexPosColUV* verts)
{
    // Use default blending and rounding modes
    IDraw2d::Rounding rounding = IDraw2d::Rounding::Nearest;
    draw2d.DrawQuad(m_image, verts, rounding);
}

void ViewportIcon::DrawAxisAlignedBoundingBox(Draw2dHelper& draw2d, AZ::Vector2 bound0, AZ::Vector2 bound1)
{
    AZ::Color dottedColor(1.0f, 1.0f, 1.0f, 1.0f);

    const float pixelLengthForDottedLineTexture = (1.0f / 8.0f);
    float endTexCoordU = fabsf((bound1.GetX() - bound0.GetX()) * pixelLengthForDottedLineTexture);
    float endTexCoordV = fabsf((bound1.GetY() - bound0.GetY()) * pixelLengthForDottedLineTexture);

    CDraw2d::VertexPosColUV verts[2];
    {
        verts[0].color = dottedColor;
        verts[1].color = dottedColor;
    }

    // bound0
    //      A----B
    //      |    |
    //      C----D
    //           bound1
    //
    // Draw line segment A -> B.
    {
        verts[0].position = AZ::Vector2(bound0.GetX(), bound0.GetY());
        verts[1].position = AZ::Vector2(bound1.GetX(), bound0.GetY());
        verts[0].uv = AZ::Vector2(0.0f, 0.5f);
        verts[1].uv = AZ::Vector2(endTexCoordU, 0.5f);

        draw2d.DrawLineTextured(m_image, verts);
    }

    // bound0
    //      A----B
    //      |    |
    //      C----D
    //           bound1
    //
    // Draw line segment A -> C.
    {
        verts[0].position = AZ::Vector2(bound0.GetX(), bound0.GetY());
        verts[1].position = AZ::Vector2(bound0.GetX(), bound1.GetY());
        verts[0].uv = AZ::Vector2(0.0f, 0.5f);
        verts[1].uv = AZ::Vector2(endTexCoordV, 0.5f);

        draw2d.DrawLineTextured(m_image, verts);
    }

    // bound0
    //      A----B
    //      |    |
    //      C----D
    //           bound1
    //
    // Draw line segment C -> D.
    {
        verts[0].position = AZ::Vector2(bound0.GetX(), bound1.GetY());
        verts[1].position = AZ::Vector2(bound1.GetX(), bound1.GetY());
        verts[0].uv = AZ::Vector2(0.0f, 0.5f);
        verts[1].uv = AZ::Vector2(endTexCoordU, 0.5f);

        draw2d.DrawLineTextured(m_image, verts);
    }

    // bound0
    //      A----B
    //      |    |
    //      C----D
    //           bound1
    //
    // Draw line segment B -> D.
    {
        verts[0].position = AZ::Vector2(bound1.GetX(), bound0.GetY());
        verts[1].position = AZ::Vector2(bound1.GetX(), bound1.GetY());
        verts[0].uv = AZ::Vector2(0.0f, 0.5f);
        verts[1].uv = AZ::Vector2(endTexCoordV, 0.5f);

        draw2d.DrawLineTextured(m_image, verts);
    }
}

void ViewportIcon::Draw(Draw2dHelper& draw2d, AZ::Vector2 anchorPos, const AZ::Matrix4x4& transform, float iconRot, AZ::Color color) const
{
    AZ::Vector2 iconSize = GetTextureSize();

    // the icon images are authored with the "point" of the anchor in the center for all icons currently
    const AZ::Vector2 originRatio(0.5f, 0.5f);
    float iconOriginX = iconSize.GetX() * originRatio.GetX();
    float iconOriginY = iconSize.GetY() * originRatio.GetY();

    AZ::Vector2 tl(anchorPos.GetX() - iconOriginX, anchorPos.GetY() - iconOriginY);
    UiTransformInterface::RectPoints iconPoints;
    iconPoints.TopLeft() = tl;
    iconPoints.TopRight() = AZ::Vector2(tl.GetX() + iconSize.GetX(), tl.GetY());
    iconPoints.BottomLeft() = AZ::Vector2(tl.GetX(), tl.GetY() + iconSize.GetY());
    iconPoints.BottomRight() = AZ::Vector2(tl.GetX() + iconSize.GetX(), tl.GetY() + iconSize.GetY());

    // apply the rotation that rotates the anchor icon to point in the correct direction
    AZ::Vector3 pivot3(anchorPos.GetX(), anchorPos.GetY(), 0);
    float rotRad = DEG2RAD(iconRot);
    AZ::Matrix4x4 moveToPivotSpaceMat = AZ::Matrix4x4::CreateTranslation(-pivot3);
    AZ::Matrix4x4 rotMat = AZ::Matrix4x4::CreateRotationZ(rotRad);
    AZ::Matrix4x4 moveFromPivotSpaceMat = AZ::Matrix4x4::CreateTranslation(pivot3);
    AZ::Matrix4x4 newTransform = transform * moveFromPivotSpaceMat * rotMat * moveToPivotSpaceMat;

    CDraw2d::VertexPosColUV verts[4];
    // points are a clockwise quad
    static const AZ::Vector2 uvs[4] = {
        AZ::Vector2(0.0f, 0.0f), AZ::Vector2(1.0f, 0.0f), AZ::Vector2(1.0f, 1.0f), AZ::Vector2(0.0f, 1.0f)
    };
    for (int i = 0; i < 4; ++i)
    {
        verts[i].color = color;
        verts[i].uv = uvs[i];

        AZ::Vector3 point3(iconPoints.pt[i].GetX(), iconPoints.pt[i].GetY(), 0.0f);
        point3 = newTransform * point3;
        verts[i].position = AZ::Vector2(point3.GetX(), point3.GetY());
    }

    // in order to align the anchor icon correctly we do want rotation, shearing and negative scale
    // in the transform to affect the icon, but we do not want its size to be affected.
    // So we fix up the transformed points so that it has the correct icon width and height in viewport space.
    if (transform.GetElement(0, 0) != 1.0f || transform.GetElement(1, 1) != 1.0f || transform.GetElement(2, 2) != 1.0f)
    {
        AZ::Vector2 widthVec = verts[1].position - verts[0].position;
        AZ::Vector2 heightVec = verts[3].position - verts[0].position;

        AZ::Vector2 originPos = verts[0].position + widthVec * originRatio.GetX() + heightVec * originRatio.GetY();

        // adjust both vectors to be of the desired length (iconW and iconH)
        // Avoid a divide by zero. We could compare with 0.0f here and that would avoid a divide
        // by zero. However comparing with FLT_EPSILON also avoids the rare case of an overflow.
        // FLT_EPSILON is small enough to be considered equivalent to zero in this application.
        float widthVecLength = widthVec.GetLength();
        float heightVecLength = heightVec.GetLength();
        widthVec *= (fabsf(widthVecLength) > FLT_EPSILON) ? iconSize.GetX() / widthVecLength : 0.0f;
        heightVec *= (fabsf(heightVecLength) > FLT_EPSILON) ? iconSize.GetY() / heightVecLength : 0.0f;

        verts[0].position = originPos - widthVec * originRatio.GetX()        - heightVec * originRatio.GetY();
        verts[1].position = originPos + widthVec * (1.0f - originRatio.GetX())    - heightVec * originRatio.GetY();
        verts[2].position = originPos + widthVec * (1.0f - originRatio.GetX())    + heightVec * (1.0f - originRatio.GetY());
        verts[3].position = originPos - widthVec * originRatio.GetX()        + heightVec * (1.0f - originRatio.GetY());
    }

    draw2d.DrawQuad(m_image, verts);
}

void ViewportIcon::DrawAnchorLines(Draw2dHelper& draw2d, AZ::Vector2 anchorPos, AZ::Vector2 targetPos, const AZ::Matrix4x4& transform,
    bool xFirst, bool xText, bool yText)
{
    AZ::Vector2 cornerPos = (xFirst) ? AZ::Vector2(targetPos.GetX(), anchorPos.GetY()) : AZ::Vector2(anchorPos.GetX(), targetPos.GetY());

    AZ::Vector3 start3 = EntityHelpers::MakeVec3(anchorPos);
    AZ::Vector3 end3 = EntityHelpers::MakeVec3(targetPos);
    AZ::Vector3 corner3 = EntityHelpers::MakeVec3(cornerPos);

    start3 = transform * start3;
    corner3 = transform * corner3;
    end3 = transform * end3;

    AZ::Vector2 start2(start3.GetX(), start3.GetY());
    AZ::Vector2 corner2(corner3.GetX(), corner3.GetY());
    AZ::Vector2 end2(end3.GetX(), end3.GetY());

    AZ::Color solidColor(1.0f, 1.0f, 1.0f, 0.2f);

    if ((xFirst && xText) || (!xFirst && yText))
    {
        float displayDistance = (xFirst) ? cornerPos.GetX() - anchorPos.GetX() : cornerPos.GetY() - anchorPos.GetY();
        DrawDistanceLine(draw2d, start2, corner2, displayDistance);
    }
    else
    {
        draw2d.DrawLine(start2, corner2, solidColor);
    }

    if ((!xFirst && xText) || (xFirst && yText))
    {
        float displayDistance = (!xFirst) ? targetPos.GetX() - cornerPos.GetX() : targetPos.GetY() - cornerPos.GetY();
        DrawDistanceLine(draw2d, corner2, end2, displayDistance);
    }
    else
    {
        draw2d.DrawLine(corner2, end2, solidColor);
    }
}

void ViewportIcon::DrawDistanceLine(Draw2dHelper& draw2d, AZ::Vector2 start, AZ::Vector2 end, float displayDistance, const char* suffix)
{
    // draw a dotted line with the distance displayed on it

    AZ::Color dottedColor(1.0f, 1.0f, 1.0f, 1.0f);

    float length = AZ::Vector2(end - start).GetLength();
    const float pixelLengthForDottedLineTexture = 8.0f;
    float endTexCoordU = length / pixelLengthForDottedLineTexture;

    CDraw2d::VertexPosColUV verts[2];

    verts[0].position = start;
    verts[0].color = dottedColor;
    verts[0].uv = AZ::Vector2(0.0f, 0.5f);

    verts[1].position = end;
    verts[1].color = dottedColor;
    verts[1].uv = AZ::Vector2(endTexCoordU, 0.5f);

    draw2d.DrawLineTextured(m_image, verts);

    // Now draw the text rotated to match the angle of the line and slightly offset from the center point

    // first swap the start end such that the line always goes left to right
    if (start.GetX() == end.GetX())
    {
        if (start.GetY() < end.GetY())
        {
            std::swap(start, end);
        }
    }
    else if (start.GetX() > end.GetX())
    {
        std::swap(start, end);
    }

    // get the angle of the line (will always be > (-90 < angle <= 90)
    float rotRad = atan2f(end.GetY() - start.GetY(), end.GetX() - start.GetX());
    float rotation = RAD2DEG(rotRad);

    // offset the bottom center of the text from the line by a fixed offset,
    // we rotate the offset to match line angle
    const float offsetDist = 2.0f;
    AZ::Vector2 textOffset(offsetDist * sinf(rotRad), -offsetDist * cosf(rotRad));

    // position for text is the midpoint of the line plus the offset
    AZ::Vector2 textPos = (start + end) * 0.5f + textOffset;

    const size_t bufSize = 32;
    char textBuf[bufSize];
    azsnprintf(textBuf, bufSize, "%.2f%s", displayDistance, suffix ? suffix : "");

    draw2d.SetTextAlignment(IDraw2d::HAlign::Center, IDraw2d::VAlign::Bottom);
    draw2d.SetTextRotation(rotation);
    draw2d.DrawText(textBuf, textPos, 16.0f * ViewportIcon::GetDpiScaleFactor(), 1.0f);
}

void ViewportIcon::DrawAnchorLinesSplit(Draw2dHelper& draw2d, AZ::Vector2 anchorPos1, AZ::Vector2 anchorPos2,
    AZ::Vector2 targetPos, const AZ::Matrix4x4& transform, bool horizSplit, const char* suffix)
{
    AZ::Vector2 cornerPos = (horizSplit) ? AZ::Vector2(targetPos.GetX(), anchorPos1.GetY()) : AZ::Vector2(anchorPos1.GetX(), targetPos.GetY());

    AZ::Vector3 start1_3 = EntityHelpers::MakeVec3(anchorPos1);
    AZ::Vector3 start2_3 = EntityHelpers::MakeVec3(anchorPos2);
    AZ::Vector3 end3 = EntityHelpers::MakeVec3(targetPos);
    AZ::Vector3 corner3 = EntityHelpers::MakeVec3(cornerPos);

    start1_3 = transform * start1_3;
    start2_3 = transform * start2_3;
    corner3 = transform * corner3;
    end3 = transform * end3;

    AZ::Vector2 start1_2(start1_3.GetX(), start1_3.GetY());
    AZ::Vector2 start2_2(start2_3.GetX(), start2_3.GetY());
    AZ::Vector2 corner2(corner3.GetX(), corner3.GetY());
    AZ::Vector2 end2(end3.GetX(), end3.GetY());

    AZ::Color solidColor(1.0f, 1.0f, 1.0f, 0.2f);

    draw2d.DrawLine(start1_2, corner2, solidColor);
    draw2d.DrawLine(corner2, start2_2, solidColor);

    float displayDistance = (!horizSplit) ? targetPos.GetX() - cornerPos.GetX() : targetPos.GetY() - cornerPos.GetY();
    DrawDistanceLine(draw2d, corner2, end2, displayDistance, suffix);
}

void ViewportIcon::DrawDistanceLineWithTransform(Draw2dHelper& draw2d, AZ::Vector2 sourcePos, AZ::Vector2 targetPos, const AZ::Matrix4x4& transform,
    float value, const char* suffix)
{
    AZ::Vector3 start3 = EntityHelpers::MakeVec3(sourcePos);
    AZ::Vector3 end3 = EntityHelpers::MakeVec3(targetPos);

    start3 = transform * start3;
    end3 = transform * end3;

    AZ::Vector2 start2(start3.GetX(), start3.GetY());
    AZ::Vector2 end2(end3.GetX(), end3.GetY());

    AZ::Color solidColor(1.0f, 1.0f, 1.0f, 0.2f);
    DrawDistanceLine(draw2d, start2, end2, value, suffix);
}

void ViewportIcon::DrawElementRectOutline(Draw2dHelper& draw2d, AZ::EntityId entityId, AZ::Color color)
{
    // get the transformed rect for the element
    UiTransformInterface::RectPoints points;
    EBUS_EVENT_ID(entityId, UiTransformBus, GetViewportSpacePoints, points);

    // work out if we should snap to exact pixel
    AZ::EntityId canvasEntityId;
    EBUS_EVENT_ID_RESULT(canvasEntityId, entityId, UiElementBus, GetCanvasEntityId);
    bool isPixelAligned = true;
    EBUS_EVENT_ID_RESULT(isPixelAligned, canvasEntityId, UiCanvasBus, GetIsPixelAligned);
    IDraw2d::Rounding pixelRounding = isPixelAligned ? IDraw2d::Rounding::Nearest : IDraw2d::Rounding::None;

    // round the points to the nearest pixel if the canvas is set to do that for elements since
    // we want this outline to line up with the element
    for (int i = 0; i < 4; ++i)
    {
        points.pt[i] = Draw2dHelper::RoundXY(points.pt[i], pixelRounding);
    }

    // since the rect is transformed we have to add the offsets by multiplying them
    // by unit vectors parallel with the edges of the rect. However, the rect could be
    // zero width and/or height so we can't use "points" to compute these unit vectors.
    // So we instead get the transform matrix and transform two unit vectors
    // and then normalize them (they have to be re-normalized since the transform can scale them)
    AZ::Matrix4x4 transform;
    EBUS_EVENT_ID(entityId, UiTransformBus, GetTransformToViewport, transform);
    AZ::Vector3 rightVec3(1.0f, 0.0f, 0.0f);
    AZ::Vector3 downVec3(0.0f, 1.0f, 0.0f);
    rightVec3 = transform.Multiply3x3(rightVec3);
    downVec3 = transform.Multiply3x3(downVec3);
    AZ::Vector2 rightVec(rightVec3.GetX(), rightVec3.GetY());
    AZ::Vector2 downVec(downVec3.GetX(), downVec3.GetY());
    rightVec.NormalizeSafe();
    downVec.NormalizeSafe();

    uint32_t lineThickness = aznumeric_cast<uint32_t>(GetTextureSize().GetY());
    draw2d.DrawRectOutlineTextured(m_image, points, rightVec, downVec, color, lineThickness);
}
