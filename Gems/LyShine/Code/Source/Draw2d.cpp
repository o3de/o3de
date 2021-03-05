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
#include "LyShine_precompiled.h"
#include "Draw2d.h"
#include "IFont.h"
#include "IRenderer.h"

#include <AzCore/Math/Matrix3x3.h>

static const int g_defaultBlendState = GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
static const int g_defaultBaseState = GS_NODEPTHTEST;

static const int g_2dModeNotStarted = -1;

////////////////////////////////////////////////////////////////////////////////////////////////////
// LOCAL STATIC FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
// Color to u32 => 0xAARRGGBB
static AZ::u32 PackARGB8888(const AZ::Color& color)
{
    return (color.GetA8() << 24) | (color.GetR8() << 16) | (color.GetG8() << 8) | color.GetB8();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
CDraw2d::CDraw2d()
    : m_deferCalls(false)
    , m_nestLevelAtWhichStarted2dMode(g_2dModeNotStarted)
{
    // These default options are set here and never change. They are stored so that if a null options
    // structure is passed into the draw functions then this default one can be used instead
    m_defaultImageOptions.blendMode = g_defaultBlendState;
    m_defaultImageOptions.color.Set(1.0f, 1.0f, 1.0f);
    m_defaultImageOptions.pixelRounding = Rounding::Nearest;
    m_defaultImageOptions.baseState = g_defaultBaseState;

    m_defaultTextOptions.font = (gEnv && gEnv->pCryFont != nullptr) ? gEnv->pCryFont->GetFont("default") : nullptr;
    m_defaultTextOptions.effectIndex = 0;
    m_defaultTextOptions.color.Set(1.0f, 1.0f, 1.0f);
    m_defaultTextOptions.horizontalAlignment = HAlign::Left;
    m_defaultTextOptions.verticalAlignment = VAlign::Top;
    m_defaultTextOptions.dropShadowOffset.Set(0.0f, 0.0f);
    m_defaultTextOptions.dropShadowColor.Set(0.0f, 0.0f, 0.0f, 0.0f);
    m_defaultTextOptions.rotation = 0.0f;
    m_defaultTextOptions.baseState = g_defaultBaseState;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CDraw2d::~CDraw2d()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::BeginDraw2d(bool deferCalls)
{
    IRenderer* renderer = gEnv->pRenderer;
    AZ::Vector2 viewportSize(
        static_cast<float>(renderer->GetOverlayWidth()),
        static_cast<float>(renderer->GetOverlayHeight()));
    BeginDraw2d(viewportSize, deferCalls);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::BeginDraw2d(AZ::Vector2 viewportSize, bool deferCalls)
{
    // So that nested calls to BeginDraw2d/EndDraw2d do not end 2D drawmode prematurely we only
    // switch to 2D mode once and do not do it again until the corresponding call to EndDraw2d
    // is processed.
    // It may seem overkill to allow nested calls rather than just asserting in that case. But
    // a) it is more flexible to do so
    // b) it can be useful to draw some debug primitives in deferred mode while rendering a
    //    canvas in non-deferred mode for example

    // Push the current state of the m_deferCalls onto a stack to support nested calls
    m_deferCallsFlagStack.push(m_deferCalls);

    m_deferCalls = deferCalls;

    // if this is the outermost call with non-deferred rendering then switch to 2D mode
    if (!m_deferCalls && m_nestLevelAtWhichStarted2dMode == g_2dModeNotStarted)
    {
        IRenderer* renderer = gEnv->pRenderer;

        renderer->SetCullMode(R_CULL_DISABLE);

        renderer->Set2DMode(static_cast<uint32>(viewportSize.GetX()), static_cast<uint32>(viewportSize.GetY()), m_backupSceneMatrices);

        renderer->SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
        renderer->SetState(g_defaultBlendState | g_defaultBaseState);

        // remember the nesting level that we turned on 2D mode so we can turn it off as
        // we unwind the stack
        m_nestLevelAtWhichStarted2dMode = m_deferCallsFlagStack.size();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::EndDraw2d()
{
    // if we are ending a non-deferred series of calls and we turned on 2D draw mode when we started
    // this series then turn it off.
    if (!m_deferCalls && m_nestLevelAtWhichStarted2dMode == m_deferCallsFlagStack.size())
    {
        IRenderer* renderer = gEnv->pRenderer;
        renderer->Unset2DMode(m_backupSceneMatrices);
        m_nestLevelAtWhichStarted2dMode = -1;
    }

    // unwind the nesting stack
    m_deferCalls = m_deferCallsFlagStack.top();
    m_deferCallsFlagStack.pop();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Draw a textured quad with the top left corner at the given position.
void CDraw2d::DrawImage(int texId, AZ::Vector2 position, AZ::Vector2 size, float opacity,
    float rotation, const AZ::Vector2* pivotPoint, const AZ::Vector2* minMaxTexCoords,
    ImageOptions* imageOptions)
{
    ImageOptions* actualImageOptions = (imageOptions) ? imageOptions : &m_defaultImageOptions;

    AZ::Color color = AZ::Color::CreateFromVector3AndFloat(actualImageOptions->color, opacity);
    AZ::u32 packedColor = PackARGB8888(color);

    int blendMode = actualImageOptions->blendMode;

    // Depending on the requested pixel rounding setting we may round position to an exact pixel
    AZ::Vector2 pos = Draw2dHelper::RoundXY(position, actualImageOptions->pixelRounding);

    // define quad (in clockwise order)
    DeferredQuad quad;
    quad.m_points[0].Set(pos.GetX(), pos.GetY());
    quad.m_points[1].Set(pos.GetX() + size.GetX(), pos.GetY());
    quad.m_points[2].Set(pos.GetX() + size.GetX(), pos.GetY() + size.GetY());
    quad.m_points[3].Set(pos.GetX(), pos.GetY() + size.GetY());

    quad.m_packedColors[0] = packedColor;
    quad.m_packedColors[1] = packedColor;
    quad.m_packedColors[2] = packedColor;
    quad.m_packedColors[3] = packedColor;

    if (minMaxTexCoords)
    {
        quad.m_texCoords[0] = AZ::Vector2(minMaxTexCoords[0].GetX(), minMaxTexCoords[0].GetY());
        quad.m_texCoords[1] = AZ::Vector2(minMaxTexCoords[1].GetX(), minMaxTexCoords[0].GetY());
        quad.m_texCoords[2] = AZ::Vector2(minMaxTexCoords[1].GetX(), minMaxTexCoords[1].GetY());
        quad.m_texCoords[3] = AZ::Vector2(minMaxTexCoords[0].GetX(), minMaxTexCoords[1].GetY());
    }
    else
    {
        quad.m_texCoords[0].Set(0.0f, 0.0f);
        quad.m_texCoords[1].Set(1.0f, 0.0f);
        quad.m_texCoords[2].Set(1.0f, 1.0f);
        quad.m_texCoords[3].Set(0.0f, 1.0f);
    }

    quad.m_texId = texId;

    // add the blendMode flags to the base state
    quad.m_state = blendMode | actualImageOptions->baseState;

    // apply rotation if requested
    if (rotation != 0.0f)
    {
        AZ::Vector2 pivot = (pivotPoint) ? *pivotPoint : quad.m_points[0];
        RotatePointsAboutPivot(quad.m_points, 4, pivot, rotation);
    }

    DrawOrDeferQuad(&quad);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::DrawImageAligned(int texId, AZ::Vector2 position, AZ::Vector2 size,
    HAlign horizontalAlignment, VAlign verticalAlignment,
    float opacity, float rotation, const AZ::Vector2* minMaxTexCoords, ImageOptions* imageOptions)
{
    AZ::Vector2 alignedPosition = Align(position, size, horizontalAlignment, verticalAlignment);

    DrawImage(texId, alignedPosition, size, opacity, rotation, &position, minMaxTexCoords, imageOptions);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::DrawQuad(int texId, VertexPosColUV* verts, int blendMode, Rounding pixelRounding, int baseState)
{
    int actualBlendMode = (blendMode == -1) ? g_defaultBlendState : blendMode;
    int actualBaseState = (baseState == -1) ? g_defaultBaseState : baseState;

    // define quad
    DeferredQuad quad;
    for (int i = 0; i < 4; ++i)
    {
        quad.m_points[i] = Draw2dHelper::RoundXY(verts[i].position, pixelRounding);
        quad.m_texCoords[i] = verts[i].uv;
        quad.m_packedColors[i] = PackARGB8888(verts[i].color);
    }
    quad.m_texId = texId;

    // add the blendMode flags to the base state
    quad.m_state = actualBlendMode | actualBaseState;

    DrawOrDeferQuad(&quad);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::DrawLine(AZ::Vector2 start, AZ::Vector2 end, AZ::Color color, int blendMode, Rounding pixelRounding, int baseState)
{
    int actualBaseState = (baseState == -1) ? g_defaultBaseState : baseState;

    IRenderer* renderer = gEnv->pRenderer;

    int texId = renderer->GetWhiteTextureId();

    int actualBlendMode = (blendMode == -1) ? g_defaultBlendState : blendMode;

    // define line
    uint32 packedColor = PackARGB8888(color);

    DeferredLine line;
    line.m_texId = texId;

    line.m_points[0] = Draw2dHelper::RoundXY(start, pixelRounding);
    line.m_points[1] = Draw2dHelper::RoundXY(end, pixelRounding);

    line.m_texCoords[0] = AZ::Vector2(0, 0);
    line.m_texCoords[1] = AZ::Vector2(1, 1);

    line.m_packedColors[0] = packedColor;
    line.m_packedColors[1] = packedColor;

    // add the blendMode flags to the base state
    line.m_state = actualBlendMode | actualBaseState;

    DrawOrDeferLine(&line);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::DrawLineTextured(int texId, VertexPosColUV* verts, int blendMode, Rounding pixelRounding, int baseState)
{
    int actualBaseState = (baseState == -1) ? g_defaultBaseState : baseState;

    IRenderer* renderer = gEnv->pRenderer;

    int actualBlendMode = (blendMode == -1) ? g_defaultBlendState : blendMode;

    // define line
    DeferredLine line;
    line.m_texId = texId;

    for (int i = 0; i < 2; ++i)
    {
        line.m_points[i] = Draw2dHelper::RoundXY(verts[i].position, pixelRounding);
        line.m_texCoords[i] = verts[i].uv;
        line.m_packedColors[i] = PackARGB8888(verts[i].color);
    }

    // add the blendMode flags to the base state
    line.m_state = actualBlendMode | actualBaseState;

    DrawOrDeferLine(&line);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE: The alignment does not take account of the drop shadow. i.e. the non-shadow text is centered
// and the drop shadow text is offset from that.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::DrawText(const char* textString, AZ::Vector2 position, float pointSize, float opacity, TextOptions* textOptions)
{
    TextOptions* actualTextOptions = (textOptions) ? textOptions : &m_defaultTextOptions;

    // render the drop shadow, if needed
    if ((actualTextOptions->dropShadowColor.GetA() > 0.0f) &&
        (actualTextOptions->dropShadowOffset.GetX() || actualTextOptions->dropShadowOffset.GetY()))
    {
        // calculate the drop shadow pos and render it
        AZ::Vector2 dropShadowPosition(position + actualTextOptions->dropShadowOffset);
        DrawTextInternal(textString, actualTextOptions->font, actualTextOptions->effectIndex,
            dropShadowPosition, pointSize, actualTextOptions->dropShadowColor,
            actualTextOptions->rotation,
            actualTextOptions->horizontalAlignment, actualTextOptions->verticalAlignment,
            actualTextOptions->baseState);
    }

    // draw the text string
    AZ::Color textColor = AZ::Color::CreateFromVector3AndFloat(actualTextOptions->color, opacity);
    DrawTextInternal(textString, actualTextOptions->font, actualTextOptions->effectIndex,
        position, pointSize, textColor,
        actualTextOptions->rotation,
        actualTextOptions->horizontalAlignment, actualTextOptions->verticalAlignment,
        actualTextOptions->baseState);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// TBD should the size include the offset of the drop shadow (if any)?
////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 CDraw2d::GetTextSize(const char* textString, float pointSize, TextOptions* textOptions)
{
    TextOptions* actualTextOptions = (textOptions) ? textOptions : &m_defaultTextOptions;

    if (!actualTextOptions->font)
    {
        return AZ::Vector2(0.0f, 0.0f);
    }

    STextDrawContext fontContext;
    fontContext.SetEffect(actualTextOptions->effectIndex);
    fontContext.SetSizeIn800x600(false);
    fontContext.SetSize(vector2f(pointSize, pointSize));

    Vec2 textSize = actualTextOptions->font->GetTextSize(textString, true, fontContext);
    return AZ::Vector2(textSize.x, textSize.y);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float CDraw2d::GetViewportWidth() const
{
    IRenderer* renderer = gEnv->pRenderer;
    return (float)renderer->GetOverlayWidth();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float CDraw2d::GetViewportHeight() const
{
    IRenderer* renderer = gEnv->pRenderer;
    return (float)renderer->GetOverlayHeight();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const CDraw2d::ImageOptions& CDraw2d::GetDefaultImageOptions() const
{
    return m_defaultImageOptions;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const CDraw2d::TextOptions& CDraw2d::GetDefaultTextOptions() const
{
    return m_defaultTextOptions;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::RenderDeferredPrimitives()
{
    IRenderer* renderer = gEnv->pRenderer;

    // Set up the 2D drawing state
    renderer->SetCullMode(R_CULL_DISABLE);

    renderer->Set2DMode(renderer->GetOverlayWidth(), renderer->GetOverlayHeight(), m_backupSceneMatrices);
    renderer->SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
    renderer->SetState(g_defaultBlendState | g_defaultBaseState);

    // Draw and delete the deferred primitives
    for (auto primIter : m_deferredPrimitives)
    {
        primIter->Draw();
        delete primIter;
    }

    // clear the list of deferred primitives
    m_deferredPrimitives.clear();

    // Reset the render state
    renderer->Unset2DMode(m_backupSceneMatrices);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 CDraw2d::Align(AZ::Vector2 position, AZ::Vector2 size,
    HAlign horizontalAlignment, VAlign verticalAlignment)
{
    AZ::Vector2 result;
    switch (horizontalAlignment)
    {
    case HAlign::Left:
        result.SetX(position.GetX());
        break;
    case HAlign::Center:
        result.SetX(position.GetX() - size.GetX() * 0.5f);
        break;
    case HAlign::Right:
        result.SetX(position.GetX() - size.GetX());
        break;
    }

    switch (verticalAlignment)
    {
    case VAlign::Top:
        result.SetY(position.GetY());
        break;
    case VAlign::Center:
        result.SetY(position.GetY() - size.GetY() * 0.5f);
        break;
    case VAlign::Bottom:
        result.SetY(position.GetY() - size.GetY());
        break;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::RotatePointsAboutPivot(AZ::Vector2* points, [[maybe_unused]] int numPoints, AZ::Vector2 pivot, float angle) const
{
    float angleRadians = DEG2RAD(angle);
    AZ::Matrix3x3 rotationMatrix = AZ::Matrix3x3::CreateRotationZ(angleRadians);

    for (int i = 0; i < 4; ++i)
    {
        AZ::Vector2 offset = points[i] - pivot;
        AZ::Vector3 offset3(offset.GetX(), offset.GetY(), 0.0f);
        offset3 = offset3 * rotationMatrix;
        offset.Set(offset3.GetX(), offset3.GetY());
        points[i] = pivot + offset;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::DrawTextInternal(const char* textString, IFFont* font, unsigned int effectIndex,
    AZ::Vector2 position, float pointSize, AZ::Color color, float rotation,
    HAlign horizontalAlignment, VAlign verticalAlignment, int baseState)
{
    if (!font)
    {
        return;
    }

    STextDrawContext fontContext;
    fontContext.SetEffect(effectIndex);
    fontContext.SetSizeIn800x600(false);
    fontContext.SetSize(vector2f(pointSize, pointSize));
    fontContext.SetColor(ColorF(color.GetR(), color.GetG(), color.GetB(), color.GetA()));
    fontContext.m_baseState = baseState;
    fontContext.SetOverrideViewProjMatrices(false);

    // FFont.cpp uses the alpha value of the color to decide whether to use the color, if the alpha value is zero
    // (in a ColorB format) then the color set via SetColor is ignored and it usually ends up drawing with an alpha of 1.
    // This is not what we want so in this case do not draw at all.
    if (!fontContext.IsColorOverridden())
    {
        return;
    }

    AZ::Vector2 alignedPosition;
    if (horizontalAlignment == HAlign::Left && verticalAlignment == VAlign::Top)
    {
        alignedPosition = position;
    }
    else
    {
        // we align based on the size of the default font effect, because we do not want the
        // text to move when the font effect is changed
        unsigned int fontEffectIndex = fontContext.m_fxIdx;
        fontContext.SetEffect(0);
        Vec2 textSize = font->GetTextSize(textString, true, fontContext);
        fontContext.SetEffect(fontEffectIndex);

        alignedPosition = Align(position, AZ::Vector2(textSize.x, textSize.y), horizontalAlignment, verticalAlignment);
    }

    int flags = 0;
    if (rotation != 0.0f)
    {
        // rotate around the position (if aligned to center will rotate about center etc)
        float rotRad = DEG2RAD(rotation);
        Vec3 pivot(position.GetX(), position.GetY(), 0.0f);
        Matrix34A moveToPivotSpaceMat = Matrix34A::CreateTranslationMat(-pivot);
        Matrix34A rotMat = Matrix34A::CreateRotationZ(rotRad);
        Matrix34A moveFromPivotSpaceMat = Matrix34A::CreateTranslationMat(pivot);

        Matrix34A transform = moveFromPivotSpaceMat * rotMat * moveToPivotSpaceMat;
        fontContext.SetTransform(transform);
        flags |= eDrawText_UseTransform;
    }

    // The font system uses these alignment flags to force text to be in the safe zone
    // depending on overscan etc
    if (horizontalAlignment == HAlign::Center)
    {
        flags |= eDrawText_Center;
    }
    else if (horizontalAlignment == HAlign::Right)
    {
        flags |= eDrawText_Right;
    }

    if (verticalAlignment == VAlign::Center)
    {
        flags |= eDrawText_CenterV;
    }
    else if (verticalAlignment == VAlign::Bottom)
    {
        flags |= eDrawText_Bottom;
    }

    fontContext.SetFlags(flags);

    if (m_deferCalls)
    {
        DeferredText* newText = new DeferredText;

        newText->m_fontContext = fontContext;
        newText->m_font = font;
        newText->m_position = alignedPosition;
        newText->m_string = textString;

        m_deferredPrimitives.push_back(newText);
    }
    else
    {
        font->DrawString(alignedPosition.GetX(), alignedPosition.GetY(), textString, true, fontContext);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::DrawOrDeferQuad(const DeferredQuad* quad)
{
    if (m_deferCalls)
    {
        DeferredQuad* newQuad = new DeferredQuad;
        *newQuad = *quad;
        m_deferredPrimitives.push_back(newQuad);
    }
    else
    {
        quad->Draw();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::DrawOrDeferLine(const DeferredLine* line)
{
    if (m_deferCalls)
    {
        DeferredLine* newLine = new DeferredLine;
        *newLine = *line;
        m_deferredPrimitives.push_back(newLine);
    }
    else
    {
        line->Draw();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CDraw2d::DeferredQuad
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::DeferredQuad::Draw() const
{
    const int32 NUM_VERTS = 6;

    const float z = 1.0f;   // depth test disabled, if writing Z this will write at far plane

    SVF_P3F_C4B_T2F vertices[NUM_VERTS];
    const int vertIndex[NUM_VERTS] = {
        0, 1, 3, 3, 1, 2
    };

    for (int i = 0; i < NUM_VERTS; ++i)
    {
        int j = vertIndex[i];
        vertices[i].xyz = Vec3(m_points[j].GetX(), m_points[j].GetY(), z);
        vertices[i].color.dcolor = m_packedColors[j];
        vertices[i].st = Vec2(m_texCoords[j].GetX(), m_texCoords[j].GetY());
    }

    IRenderer* renderer = gEnv->pRenderer;
    renderer->SetTexture(m_texId);

    // Set the render state, can't rely on this being right because font rendering changes it
    renderer->SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);

    // set the desired render state
    renderer->SetState(m_state);

    // This will end up using DrawPrimitive to render the quad
    renderer->DrawDynVB(vertices, nullptr, NUM_VERTS, 0, prtTriangleList);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CDraw2d::DeferredLine
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::DeferredLine::Draw() const
{
    const float z = 1.0f;   // depth test disabled, if writing Z this will write at far plane

    const int32 NUM_VERTS = 2;

    SVF_P3F_C4B_T2F vertices[NUM_VERTS];

    for (int i = 0; i < NUM_VERTS; ++i)
    {
        vertices[i].xyz = Vec3(m_points[i].GetX(), m_points[i].GetY(), z);
        vertices[i].color.dcolor = m_packedColors[i];
        vertices[i].st = Vec2(m_texCoords[i].GetX(), m_texCoords[i].GetY());
    }

    IRenderer* renderer = gEnv->pRenderer;
    renderer->SetTexture(m_texId);

    // Set the render state, can't rely on this being right because font rendering changes it
    renderer->SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
    renderer->SetState(m_state);

    // This will end up using DrawPrimitive to render the quad
    renderer->DrawDynVB(vertices, nullptr, NUM_VERTS, 0, prtLineList);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// CDraw2d::DeferredText
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::DeferredText::Draw() const
{
    m_font->DrawString(m_position.GetX(), m_position.GetY(), m_string.c_str(), true, m_fontContext);
}

