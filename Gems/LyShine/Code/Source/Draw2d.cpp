/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <IRenderer.h> // for SVF_P3F_C4B_T2F which will be removed in a coming PR

#include <LyShine/Draw2d.h>
#include "LyShinePassDataBus.h"

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzFramework/Font/FontInterface.h>

#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

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
CDraw2d::CDraw2d(AZ::RPI::ViewportContextPtr viewportContext)
    : m_deferCalls(false)
    , m_viewportContext(viewportContext)
{
    // These default options are set here and never change. They are stored so that if a null options
    // structure is passed into the draw functions then this default one can be used instead

    m_defaultTextOptions.fontName = "default";
    m_defaultTextOptions.effectIndex = 0;
    m_defaultTextOptions.color.Set(1.0f, 1.0f, 1.0f);
    m_defaultTextOptions.horizontalAlignment = HAlign::Left;
    m_defaultTextOptions.verticalAlignment = VAlign::Top;
    m_defaultTextOptions.dropShadowOffset.Set(0.0f, 0.0f);
    m_defaultTextOptions.dropShadowColor.Set(0.0f, 0.0f, 0.0f, 0.0f);
    m_defaultTextOptions.rotation = 0.0f;
    m_defaultTextOptions.depthTestEnabled = false;

    AZ::Render::Bootstrap::NotificationBus::Handler::BusConnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CDraw2d::~CDraw2d()
{
    AZ::Render::Bootstrap::NotificationBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::OnBootstrapSceneReady(AZ::RPI::Scene* bootstrapScene)
{
    // At this point the RPI is ready for use

    // Load the shader to be used for 2d drawing
    const char* shaderFilepath = "Shaders/SimpleTextured.azshader";
    AZ::Data::Instance<AZ::RPI::Shader> shader = AZ::RPI::LoadCriticalShader(shaderFilepath);

    // Set scene to be associated with the dynamic draw context
    AZ::RPI::Scene* scene = nullptr;
    if (m_viewportContext)
    {
        // Use scene associated with the specified viewport context
        scene = m_viewportContext->GetRenderScene().get();
    }
    else
    {
        // No viewport context specified, use main scene
        scene = bootstrapScene;
    }
    AZ_Assert(scene != nullptr, "Attempting to create a DynamicDrawContext for a viewport context that has not been associated with a scene yet.");

    // Create and initialize a DynamicDrawContext for 2d drawing

    // Get the pass for the dynamic draw context to render to
    AZ::RPI::RasterPass* uiCanvasPass = nullptr;
    AZ::RPI::SceneId sceneId = scene->GetId();
    LyShinePassRequestBus::EventResult(uiCanvasPass, sceneId, &LyShinePassRequestBus::Events::GetUiCanvasPass);

    m_dynamicDraw = AZ::RPI::DynamicDrawInterface::Get()->CreateDynamicDrawContext();
    AZ::RPI::ShaderOptionList shaderOptions;
    shaderOptions.push_back(AZ::RPI::ShaderOption(AZ::Name("o_useColorChannels"), AZ::Name("true")));
    shaderOptions.push_back(AZ::RPI::ShaderOption(AZ::Name("o_clamp"), AZ::Name("false")));
    m_dynamicDraw->InitShaderWithVariant(shader, &shaderOptions);
    m_dynamicDraw->InitVertexFormat(
        { {"POSITION", AZ::RHI::Format::R32G32B32_FLOAT},
        {"COLOR", AZ::RHI::Format::B8G8R8A8_UNORM},
        {"TEXCOORD0", AZ::RHI::Format::R32G32_FLOAT} });
    m_dynamicDraw->AddDrawStateOptions(AZ::RPI::DynamicDrawContext::DrawStateOptions::PrimitiveType
        | AZ::RPI::DynamicDrawContext::DrawStateOptions::BlendMode
        | AZ::RPI::DynamicDrawContext::DrawStateOptions::DepthState);
    if (uiCanvasPass)
    {
        m_dynamicDraw->SetOutputScope(uiCanvasPass);
    }
    else
    {
        // Render target support is disabled
        m_dynamicDraw->SetOutputScope(scene);
    }
    m_dynamicDraw->EndInit();

    // Cache draw srg input indices for later use
    static const char textureIndexName[] = "m_texture";
    static const char worldToProjIndexName[] = "m_worldToProj";
    AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> drawSrg = m_dynamicDraw->NewDrawSrg();
    const AZ::RHI::ShaderResourceGroupLayout* layout = drawSrg->GetLayout();
    m_shaderData.m_imageInputIndex = layout->FindShaderInputImageIndex(AZ::Name(textureIndexName));
    AZ_Error("Draw2d", m_shaderData.m_imageInputIndex.IsValid(), "Failed to find shader input constant %s.",
        textureIndexName);
    m_shaderData.m_viewProjInputIndex = layout->FindShaderInputConstantIndex(AZ::Name(worldToProjIndexName));
    AZ_Error("Draw2d", m_shaderData.m_viewProjInputIndex.IsValid(), "Failed to find shader input constant %s.",
        worldToProjIndexName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Draw a textured quad with the top left corner at the given position.
void CDraw2d::DrawImage(AZ::Data::Instance<AZ::RPI::Image> image, AZ::Vector2 position, AZ::Vector2 size, float opacity,
    float rotation, const AZ::Vector2* pivotPoint, const AZ::Vector2* minMaxTexCoords,
    ImageOptions* imageOptions)
{
    ImageOptions* actualImageOptions = (imageOptions) ? imageOptions : &m_defaultImageOptions;

    AZ::Color color = AZ::Color::CreateFromVector3AndFloat(actualImageOptions->color, opacity);
    AZ::u32 packedColor = PackARGB8888(color);

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

    quad.m_image = image;

    // add the blendMode flags to the base state
    quad.m_renderState = actualImageOptions->m_renderState;

    // apply rotation if requested
    if (rotation != 0.0f)
    {
        AZ::Vector2 pivot = (pivotPoint) ? *pivotPoint : quad.m_points[0];
        RotatePointsAboutPivot(quad.m_points, 4, pivot, rotation);
    }

    DrawOrDeferQuad(&quad);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::DrawImageAligned(AZ::Data::Instance<AZ::RPI::Image> image, AZ::Vector2 position, AZ::Vector2 size,
    HAlign horizontalAlignment, VAlign verticalAlignment,
    float opacity, float rotation, const AZ::Vector2* minMaxTexCoords, ImageOptions* imageOptions)
{
    AZ::Vector2 alignedPosition = Align(position, size, horizontalAlignment, verticalAlignment);

    DrawImage(image, alignedPosition, size, opacity, rotation, &position, minMaxTexCoords, imageOptions);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::DrawQuad(AZ::Data::Instance<AZ::RPI::Image> image, VertexPosColUV* verts, Rounding pixelRounding,
    const CDraw2d::RenderState& renderState)
{
    // define quad
    DeferredQuad quad;
    for (int i = 0; i < 4; ++i)
    {
        quad.m_points[i] = Draw2dHelper::RoundXY(verts[i].position, pixelRounding);
        quad.m_texCoords[i] = verts[i].uv;
        quad.m_packedColors[i] = PackARGB8888(verts[i].color);
    }
    quad.m_image = image;

    // add the blendMode flags to the base state
    quad.m_renderState = renderState;

    DrawOrDeferQuad(&quad);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::DrawLine(AZ::Vector2 start, AZ::Vector2 end, AZ::Color color, Rounding pixelRounding,
    const CDraw2d::RenderState& renderState)
{
    auto image = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::White);

    // define line
    uint32 packedColor = PackARGB8888(color);

    DeferredLine line;
    line.m_image = image;

    line.m_points[0] = Draw2dHelper::RoundXY(start, pixelRounding);
    line.m_points[1] = Draw2dHelper::RoundXY(end, pixelRounding);

    line.m_texCoords[0] = AZ::Vector2(0, 0);
    line.m_texCoords[1] = AZ::Vector2(1, 1);

    line.m_packedColors[0] = packedColor;
    line.m_packedColors[1] = packedColor;

    // add the blendMode flags to the base state
    line.m_renderState = renderState;

    DrawOrDeferLine(&line);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::DrawLineTextured(AZ::Data::Instance<AZ::RPI::Image> image, VertexPosColUV* verts, Rounding pixelRounding,
    const CDraw2d::RenderState& renderState)
{
    // define line
    DeferredLine line;
    line.m_image = image;

    for (int i = 0; i < 2; ++i)
    {
        line.m_points[i] = Draw2dHelper::RoundXY(verts[i].position, pixelRounding);
        line.m_texCoords[i] = verts[i].uv;
        line.m_packedColors[i] = PackARGB8888(verts[i].color);
    }

    // add the blendMode flags to the base state
    line.m_renderState = renderState;

    DrawOrDeferLine(&line);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE: The alignment does not take account of the drop shadow. i.e. the non-shadow text is centered
// and the drop shadow text is offset from that.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::DrawText(const char* textString, AZ::Vector2 position, float pointSize, float opacity, TextOptions* textOptions)
{
    TextOptions* actualTextOptions = (textOptions) ? textOptions : &m_defaultTextOptions;

    AzFramework::FontId fontId = AzFramework::InvalidFontId;
    AzFramework::FontQueryInterface* fontQueryInterface = AZ::Interface<AzFramework::FontQueryInterface>::Get();
    if (fontQueryInterface)
    {
        fontId = fontQueryInterface->GetFontId(actualTextOptions->fontName);
    }

    // render the drop shadow, if needed
    if ((actualTextOptions->dropShadowColor.GetA() > 0.0f) &&
        (actualTextOptions->dropShadowOffset.GetX() || actualTextOptions->dropShadowOffset.GetY()))
    {
        // calculate the drop shadow pos and render it
        AZ::Vector2 dropShadowPosition(position + actualTextOptions->dropShadowOffset);
        DrawTextInternal(textString, fontId, actualTextOptions->effectIndex,
            dropShadowPosition, pointSize, actualTextOptions->dropShadowColor,
            actualTextOptions->rotation,
            actualTextOptions->horizontalAlignment, actualTextOptions->verticalAlignment,
            actualTextOptions->depthTestEnabled);
    }

    // draw the text string
    AZ::Color textColor = AZ::Color::CreateFromVector3AndFloat(actualTextOptions->color, opacity);
    DrawTextInternal(textString, fontId, actualTextOptions->effectIndex,
        position, pointSize, textColor,
        actualTextOptions->rotation,
        actualTextOptions->horizontalAlignment, actualTextOptions->verticalAlignment,
        actualTextOptions->depthTestEnabled);
}

void CDraw2d::DrawRectOutlineTextured(AZ::Data::Instance<AZ::RPI::Image> image,
    UiTransformInterface::RectPoints points,
    AZ::Vector2 rightVec,
    AZ::Vector2 downVec,
    AZ::Color color,
    uint32_t lineThickness)
{
    // since the rect can be transformed we have to add the offsets by multiplying them
    // by unit vectors parallel with the edges of the rect. However, the rect could be
    // zero width and/or height so we can't use "points" to compute these unit vectors.
    // So we instead get two transformed unit vectors and then normalize them
    rightVec.NormalizeSafe();
    downVec.NormalizeSafe();

    // calculate the transformed width and height of the rect
    // (in case it is smaller than the texture height)
    AZ::Vector2 widthVec = points.TopRight() - points.TopLeft();
    AZ::Vector2 heightVec = points.BottomLeft() - points.TopLeft();
    float rectWidth = widthVec.GetLength();
    float rectHeight = heightVec.GetLength();

    if (lineThickness == 0 && image)
    {
        lineThickness = image->GetDescriptor().m_size.m_height;
    }

    if (lineThickness == 0)
    {
        AZ_Assert(false, "Attempting to draw a rect outline with of zero thickness.");
        return;
    }

    // the outline is centered on the element rect so half the outline is outside
    // the rect and half is inside the rect
    float offset = aznumeric_cast<float>(lineThickness);
    float outerOffset = -offset * 0.5f;
    float innerOffset = offset * 0.5f;
    float outerV = 0.0f;
    float innerV = 1.0f;

    // if the rect is small there may not be space for the half of the outline that
    // is inside the rect. If this is the case reduce the innerOffset so the inner
    // points are coincident. Adjust the UVs according to keep a 1-1 texel to pixel ratio.
    float minDimension = min(rectWidth, rectHeight);
    if (innerOffset > minDimension * 0.5f)
    {
        float oldInnerOffset = innerOffset;
        innerOffset = minDimension * 0.5f;
        // note oldInnerOffset can't be zero because of early return if lineThickness is zero
        innerV = 0.5f + 0.5f * innerOffset / oldInnerOffset;
    }

    DeferredRectOutline rectOutline;

    // fill out the 8 verts to define the 2 rectangles - outer and inner
    // The vertices are in the order of outer rect then inner rect. e.g.:
    //  0        1
    //     4  5
    //     6  7
    //  2        3
    //
    // four verts of outer rect
    rectOutline.m_verts2d[0] = points.pt[0] + rightVec * outerOffset + downVec * outerOffset;
    rectOutline.m_verts2d[1] = points.pt[1] - rightVec * outerOffset + downVec * outerOffset;
    rectOutline.m_verts2d[2] = points.pt[3] + rightVec * outerOffset - downVec * outerOffset;
    rectOutline.m_verts2d[3] = points.pt[2] - rightVec * outerOffset - downVec * outerOffset;
    // four verts of inner rect
    rectOutline.m_verts2d[4] = points.pt[0] + rightVec * innerOffset + downVec * innerOffset;
    rectOutline.m_verts2d[5] = points.pt[1] - rightVec * innerOffset + downVec * innerOffset;
    rectOutline.m_verts2d[6] = points.pt[3] + rightVec * innerOffset - downVec * innerOffset;
    rectOutline.m_verts2d[7] = points.pt[2] - rightVec * innerOffset - downVec * innerOffset;

    // and define the UV coordinates for these 8 verts
    rectOutline.m_uvs[0] = AZ::Vector2(0.0f, outerV);
    rectOutline.m_uvs[1] = AZ::Vector2(1.0f, outerV);
    rectOutline.m_uvs[2] = AZ::Vector2(1.0f, outerV);
    rectOutline.m_uvs[3] = AZ::Vector2(0.0f, outerV);
    rectOutline.m_uvs[4] = AZ::Vector2(0.0f, innerV);
    rectOutline.m_uvs[5] = AZ::Vector2(1.0f, innerV);
    rectOutline.m_uvs[6] = AZ::Vector2(1.0f, innerV);
    rectOutline.m_uvs[7] = AZ::Vector2(0.0f, innerV);

    rectOutline.m_image = image;
    rectOutline.m_color = color;

    DrawOrDeferRectOutline(&rectOutline);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// TBD should the size include the offset of the drop shadow (if any)?
////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 CDraw2d::GetTextSize(const char* textString, float pointSize, TextOptions* textOptions)
{
    AzFramework::FontDrawInterface* fontDrawInterface = nullptr;
    AzFramework::FontQueryInterface* fontQueryInterface = AZ::Interface<AzFramework::FontQueryInterface>::Get();
    if (fontQueryInterface)
    {
        TextOptions* actualTextOptions = (textOptions) ? textOptions : &m_defaultTextOptions;
        AzFramework::FontId fontId = fontQueryInterface->GetFontId(actualTextOptions->fontName);
        fontDrawInterface = fontQueryInterface->GetFontDrawInterface(fontId);
    }
    if (!fontDrawInterface)
    {
        return AZ::Vector2(0.0f, 0.0f);
    }

    // Set up draw parameters
    AzFramework::TextDrawParameters drawParams;
    drawParams.m_drawViewportId = GetViewportContext()->GetId();
    drawParams.m_position = AZ::Vector3(0.0f, 0.0f, 1.0f);
    drawParams.m_effectIndex = 0;
    drawParams.m_textSizeFactor = pointSize;
    drawParams.m_scale = AZ::Vector2(1.0f, 1.0f);
    drawParams.m_lineSpacing = 1.0f;
    drawParams.m_monospace = false;
    drawParams.m_depthTest = false;
    drawParams.m_virtual800x600ScreenSize = false;
    drawParams.m_scaleWithWindow = false;
    drawParams.m_multiline = true;

    AZ::Vector2 textSize = fontDrawInterface->GetTextSize(drawParams, textString);
    return textSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float CDraw2d::GetViewportWidth() const
{
    auto windowContext = GetViewportContext()->GetWindowContext();
    const AZ::RHI::Viewport& viewport = windowContext->GetViewport();
    const float viewWidth = viewport.m_maxX - viewport.m_minX;
    return viewWidth;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float CDraw2d::GetViewportHeight() const
{
    auto windowContext = GetViewportContext()->GetWindowContext();
    const AZ::RHI::Viewport& viewport = windowContext->GetViewport();
    const float viewHeight = viewport.m_maxY - viewport.m_minY;
    return viewHeight;
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
    // Draw and delete the deferred primitives
    AZ::RPI::ViewportContextPtr viewportContext = GetViewportContext();
    for (auto primIter : m_deferredPrimitives)
    {
        primIter->Draw(m_dynamicDraw, m_shaderData, viewportContext);
        delete primIter;
    }

    // clear the list of deferred primitives
    m_deferredPrimitives.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::SetDeferPrimitives(bool deferPrimitives)
{
    m_deferCalls = deferPrimitives;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CDraw2d::GetDeferPrimitives()
{
    return m_deferCalls;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::SetSortKey(int64_t key)
{
    m_dynamicDraw->SetSortKey(key);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 CDraw2d::Align(AZ::Vector2 position, AZ::Vector2 size,
    HAlign horizontalAlignment, VAlign verticalAlignment)
{
    AZ::Vector2 result = AZ::Vector2::CreateZero();
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
AZ::Data::Instance<AZ::RPI::Image> CDraw2d::LoadTexture(const AZStd::string& pathName)
{
    // The file may not be in the AssetCatalog at this point if it is still processing or doesn't exist on disk.
    // Use GenerateAssetIdTEMP instead of GetAssetIdByPath so that it will return a valid AssetId anyways
    AZ::Data::AssetId streamingImageAssetId;
    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
        streamingImageAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GenerateAssetIdTEMP,
        pathName.c_str());
    streamingImageAssetId.m_subId = AZ::RPI::StreamingImageAsset::GetImageAssetSubId();

    auto streamingImageAsset = AZ::Data::AssetManager::Instance().FindOrCreateAsset<AZ::RPI::StreamingImageAsset>(streamingImageAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
    AZ::Data::Instance<AZ::RPI::Image> image = AZ::RPI::StreamingImage::FindOrCreate(streamingImageAsset);
    if (!image)
    {
        AZ_Error("Draw2d", false, "Failed to find or create an image instance from image asset '%s'", pathName.c_str());
    }

    return image;
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
void CDraw2d::DrawTextInternal(const char* textString, AzFramework::FontId fontId, unsigned int effectIndex,
    AZ::Vector2 position, float pointSize, AZ::Color color, float rotation,
    HAlign horizontalAlignment, VAlign verticalAlignment, bool depthTestEnabled)
{
    // FFont.cpp uses the alpha value of the color to decide whether to use the color, if the alpha value is zero
    // (in a ColorB format) then the color set via SetColor is ignored and it usually ends up drawing with an alpha of 1.
    // This is not what we want so in this case do not draw at all.
    if (AZ::IsClose(color.GetA(), 0.0f))
    {
        return;
    }

    // Convert Draw2d alignment to text alignment
    AzFramework::TextHorizontalAlignment hAlignment = AzFramework::TextHorizontalAlignment::Left;
    switch (horizontalAlignment)
    {
    case HAlign::Left:
        hAlignment = AzFramework::TextHorizontalAlignment::Left;
        break;
    case HAlign::Center:
        hAlignment = AzFramework::TextHorizontalAlignment::Center;
        break;
    case HAlign::Right:
        hAlignment = AzFramework::TextHorizontalAlignment::Right;
        break;
    default:
        AZ_Assert(false, "Attempting to draw text with unsupported horizontal alignment.");
        break;
    }

    AzFramework::TextVerticalAlignment vAlignment = AzFramework::TextVerticalAlignment::Top;
    switch (verticalAlignment)
    {
    case VAlign::Top:
        vAlignment = AzFramework::TextVerticalAlignment::Top;
        break;
    case VAlign::Center:
        vAlignment = AzFramework::TextVerticalAlignment::Center;
        break;
    case VAlign::Bottom:
        vAlignment = AzFramework::TextVerticalAlignment::Bottom;
        break;
    default:
        AZ_Assert(false, "Attempting to draw text with unsupported vertical alignment.");
        break;
    }

    // Set up draw parameters for font interface
    AzFramework::TextDrawParameters drawParams;
    drawParams.m_drawViewportId = GetViewportContext()->GetId();
    drawParams.m_position = AZ::Vector3(position.GetX(), position.GetY(), 1.0f);
    drawParams.m_color = color;
    drawParams.m_effectIndex = effectIndex;
    drawParams.m_textSizeFactor = pointSize;
    drawParams.m_scale = AZ::Vector2(1.0f, 1.0f);
    drawParams.m_lineSpacing = 1.0f; //!< Spacing between new lines, as a percentage of m_scale.
    drawParams.m_hAlign = hAlignment;
    drawParams.m_vAlign = vAlignment;
    drawParams.m_monospace = false;
    drawParams.m_depthTest = depthTestEnabled;
    drawParams.m_virtual800x600ScreenSize = false;
    drawParams.m_scaleWithWindow = false;
    drawParams.m_multiline = true;

    if (rotation != 0.0f)
    {
        // rotate around the position (if aligned to center will rotate about center etc)
        float rotRad = DEG2RAD(rotation);
        AZ::Vector3 pivot(position.GetX(), position.GetY(), 0.0f);
        AZ::Matrix3x4 moveToPivotSpaceMat = AZ::Matrix3x4::CreateTranslation(-pivot);
        AZ::Matrix3x4 rotMat = AZ::Matrix3x4::CreateRotationZ(rotRad);
        AZ::Matrix3x4 moveFromPivotSpaceMat = AZ::Matrix3x4::CreateTranslation(pivot);

        drawParams.m_transform = moveFromPivotSpaceMat * rotMat * moveToPivotSpaceMat;
        drawParams.m_useTransform = true;
    }

    DeferredText newText;
    newText.m_drawParameters = drawParams;
    newText.m_fontId = fontId;
    newText.m_string = textString;

    DrawOrDeferTextString(&newText);
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
        quad->Draw(m_dynamicDraw, m_shaderData, GetViewportContext());
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
        line->Draw(m_dynamicDraw, m_shaderData, GetViewportContext());
    }
}

void CDraw2d::DrawOrDeferTextString(const DeferredText* text)
{
    if (m_deferCalls)
    {
        DeferredText* newText = new DeferredText;
        *newText = *text;
        m_deferredPrimitives.push_back(newText);
    }
    else
    {
        text->Draw(m_dynamicDraw, m_shaderData, GetViewportContext());
    }
}

void CDraw2d::DrawOrDeferRectOutline(const DeferredRectOutline* rectOutline)
{
    if (m_deferCalls)
    {
        DeferredRectOutline* newRectOutline = new DeferredRectOutline;
        *newRectOutline = *rectOutline;
        m_deferredPrimitives.push_back(newRectOutline);
    }
    else
    {
        rectOutline->Draw(m_dynamicDraw, m_shaderData, GetViewportContext());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::RPI::ViewportContextPtr CDraw2d::GetViewportContext() const
{
    if (!m_viewportContext)
    {
        // Return the default viewport context
        auto viewContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        return viewContextManager->GetDefaultViewportContext();
    }

    // Return the user specified viewport context
    return m_viewportContext;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CDraw2d::DeferredQuad
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::DeferredQuad::Draw(AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> dynamicDraw,
    const Draw2dShaderData& shaderData,
    AZ::RPI::ViewportContextPtr viewportContext) const
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

    // Set up per draw SRG
    AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> drawSrg = dynamicDraw->NewDrawSrg();

    // Set texture
    const AZ::RHI::ImageView* imageView = m_image ? m_image->GetImageView() : nullptr;
    if (!imageView)
    {
        // Default to white texture
        auto image = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::White);
        imageView = image->GetImageView();
    }

    if (imageView)
    {
        drawSrg->SetImageView(shaderData.m_imageInputIndex, imageView, 0);
    }

    // Set projection matrix
    auto windowContext = viewportContext->GetWindowContext();
    const AZ::RHI::Viewport& viewport = windowContext->GetViewport();
    const float viewX = viewport.m_minX;
    const float viewY = viewport.m_minY;
    const float viewWidth = viewport.m_maxX - viewport.m_minX;
    const float viewHeight = viewport.m_maxY - viewport.m_minY;
    const float zf = viewport.m_minZ;
    const float zn = viewport.m_maxZ;
    AZ::Matrix4x4 modelViewProjMat;
    AZ::MakeOrthographicMatrixRH(modelViewProjMat, viewX, viewX + viewWidth, viewY + viewHeight, viewY, zn, zf);
    drawSrg->SetConstant(shaderData.m_viewProjInputIndex, modelViewProjMat);

    drawSrg->Compile();

    // Add the primitive to the dynamic draw context for drawing
    dynamicDraw->SetPrimitiveType(AZ::RHI::PrimitiveTopology::TriangleList);
    dynamicDraw->SetDepthState(m_renderState.m_depthState);
    dynamicDraw->SetTarget0BlendState(m_renderState.m_blendState);
    dynamicDraw->DrawLinear(vertices, NUM_VERTS, drawSrg);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CDraw2d::DeferredLine
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::DeferredLine::Draw(AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> dynamicDraw,
    const Draw2dShaderData& shaderData,
    AZ::RPI::ViewportContextPtr viewportContext) const
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

    // Set up per draw SRG
    AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> drawSrg = dynamicDraw->NewDrawSrg();

    // Set texture
    const AZ::RHI::ImageView* imageView = m_image ? m_image->GetImageView() : nullptr;
    if (!imageView)
    {
        // Default to white texture
        auto image = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::White);
        imageView = image->GetImageView();
    }

    if (imageView)
    {
        drawSrg->SetImageView(shaderData.m_imageInputIndex, imageView, 0);
    }

    // Set projection matrix
    auto windowContext = viewportContext->GetWindowContext();
    const AZ::RHI::Viewport& viewport = windowContext->GetViewport();
    const float viewX = viewport.m_minX;
    const float viewY = viewport.m_minY;
    const float viewWidth = viewport.m_maxX - viewport.m_minX;
    const float viewHeight = viewport.m_maxY - viewport.m_minY;
    const float zf = viewport.m_minZ;
    const float zn = viewport.m_maxZ;
    AZ::Matrix4x4 modelViewProjMat;
    AZ::MakeOrthographicMatrixRH(modelViewProjMat, viewX, viewX + viewWidth, viewY + viewHeight, viewY, zn, zf);
    drawSrg->SetConstant(shaderData.m_viewProjInputIndex, modelViewProjMat);

    drawSrg->Compile();

    // Add the primitive to the dynamic draw context for drawing
    dynamicDraw->SetPrimitiveType(AZ::RHI::PrimitiveTopology::LineList);
    dynamicDraw->SetDepthState(m_renderState.m_depthState);
    dynamicDraw->SetTarget0BlendState(m_renderState.m_blendState);
    dynamicDraw->DrawLinear(vertices, NUM_VERTS, drawSrg);
}

void CDraw2d::DeferredRectOutline::Draw(AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> dynamicDraw,
    const Draw2dShaderData& shaderData,
    AZ::RPI::ViewportContextPtr viewportContext) const
{
    // Create the 8 verts in the right vertex format for the dynamic draw context
    SVF_P3F_C4B_T2F vertices[NUM_VERTS];
    const float z = 1.0f;   // depth test disabled, if writing Z this will write at far plane
    uint32 packedColor = (m_color.GetA8() << 24) | (m_color.GetR8() << 16) | (m_color.GetG8() << 8) | m_color.GetB8();
    for (int i = 0; i < NUM_VERTS; ++i)
    {
        vertices[i].xyz = Vec3(m_verts2d[i].GetX(), m_verts2d[i].GetY(), z);
        vertices[i].color.dcolor = packedColor;
        vertices[i].st = Vec2(m_uvs[i].GetX(), m_uvs[i].GetY());
    }

    // The indices are for four quads (one for each side of the rect).
    // The quads are drawn using a triangle list (simpler than a tri-strip)
    // We draw each quad in the same order that the image component draws quads to
    // maximize chances of things lining up so each quad is drawn as two triangles:
    // top-left, top-right, bottom-left / bottom-left, top-right, bottom-right
    // e.g. for a quad like this:
    //
    // 0   1
    //  |/|
    // 2   3
    //
    // The two triangles would be 0,1,2 and 2,1,3
    //
    static constexpr int32 NUM_INDICES = 24;
    uint16 indices[NUM_INDICES] =
    {
        0, 1, 4,    4, 1, 5,    // top quad
        6, 7, 2,    2, 7, 3,    // bottom quad
        0, 4, 2,    2, 4, 6,    // left quad
        5, 1, 7,    1, 7, 3,    // right quad
    };

    // Set up per draw SRG
    AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> drawSrg = dynamicDraw->NewDrawSrg();

    // Set texture
    const AZ::RHI::ImageView* imageView = m_image ? m_image->GetImageView() : nullptr;
    if (!imageView)
    {
        // Default to white texture
        auto image = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::White);
        imageView = image->GetImageView();
    }

    if (imageView)
    {
        drawSrg->SetImageView(shaderData.m_imageInputIndex, imageView, 0);
    }

    // Set projection matrix
    auto windowContext = viewportContext->GetWindowContext();
    const AZ::RHI::Viewport& viewport = windowContext->GetViewport();
    const float viewX = viewport.m_minX;
    const float viewY = viewport.m_minY;
    const float viewWidth = viewport.m_maxX - viewport.m_minX;
    const float viewHeight = viewport.m_maxY - viewport.m_minY;
    const float zf = viewport.m_minZ;
    const float zn = viewport.m_maxZ;
    AZ::Matrix4x4 modelViewProjMat;
    AZ::MakeOrthographicMatrixRH(modelViewProjMat, viewX, viewX + viewWidth, viewY + viewHeight, viewY, zn, zf);
    drawSrg->SetConstant(shaderData.m_viewProjInputIndex, modelViewProjMat);

    drawSrg->Compile();

    // Add the primitive to the dynamic draw context for drawing
    dynamicDraw->SetPrimitiveType(AZ::RHI::PrimitiveTopology::TriangleList);
    dynamicDraw->DrawIndexed(vertices, NUM_VERTS, indices, NUM_INDICES, AZ::RHI::IndexFormat::Uint16, drawSrg);

}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CDraw2d::DeferredText
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDraw2d::DeferredText::Draw([[maybe_unused]] AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> dynamicDraw,
    [[maybe_unused]] const Draw2dShaderData& shaderData,
    [[maybe_unused]] AZ::RPI::ViewportContextPtr viewportContext) const
{
    AzFramework::FontDrawInterface* fontDrawInterface = nullptr;
    AzFramework::FontQueryInterface* fontQueryInterface = AZ::Interface<AzFramework::FontQueryInterface>::Get();
    if (fontQueryInterface)
    {
        fontDrawInterface = fontQueryInterface->GetFontDrawInterface(m_fontId);
        if (fontDrawInterface)
        {
            fontDrawInterface->DrawScreenAlignedText2d(m_drawParameters, m_string.c_str());
        }
    }
}
