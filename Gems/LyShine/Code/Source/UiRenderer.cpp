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
#include "UiRenderer.h"

#include <LyShine/IDraw2d.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiRenderer::UiRenderer()
    : m_baseState(GS_DEPTHFUNC_LEQUAL)
    , m_stencilRef(0)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiRenderer::~UiRenderer()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::BeginUiFrameRender()
{
    m_renderer = gEnv->pRenderer;

    // we are rendering at the end of the frame, after tone mapping, so we should be writing sRGB values
    m_renderer->SetSrgbWrite(true);

#ifndef _RELEASE
    if (m_debugTextureDataRecordLevel > 0)
    {
        m_texturesUsedInFrame.clear();
    }
#endif
    
    // Various platform drivers expect all texture slots used in the shader to be bound
    BindNullTexture();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::EndUiFrameRender()
{
    // We never want to leave a texture bound that could get unloaded before the next render
    // So bind the global white texture for all the texture units we use.
    BindNullTexture();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::BeginCanvasRender()
{
    m_baseState = GS_NODEPTHTEST;
    
    m_stencilRef = 0;

    // Set default starting state
    IRenderer* renderer = gEnv->pRenderer;

    renderer->SetCullMode(R_CULL_DISABLE);
    renderer->SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
    renderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::EndCanvasRender()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiRenderer::GetBaseState()
{
    return m_baseState;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::SetBaseState(int state)
{
    m_baseState = state;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32 UiRenderer::GetStencilRef()
{
    return m_stencilRef;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::SetStencilRef(uint32 stencilRef)
{
    m_stencilRef = stencilRef;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::IncrementStencilRef()
{
    ++m_stencilRef;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::DecrementStencilRef()
{
    --m_stencilRef;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::SetTexture(ITexture* texture, int texUnit, bool clamp)
{
    if (!texture)
    {
        texture = m_renderer->GetWhiteTexture();
    }
    else
    {
        texture->SetClamp(clamp);
    }

    m_renderer->SetTexture(texture->GetTextureID(), texUnit);

#ifndef _RELEASE
    if (m_debugTextureDataRecordLevel > 0)
    {
        m_texturesUsedInFrame.insert(texture);
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::BindNullTexture()
{
    // Bind the global white texture for all the texture units we use
    const int MaxTextures = 16;
    int whiteTexId = m_renderer->GetWhiteTextureId();
    for (int texUnit = 0; texUnit < MaxTextures; ++texUnit)
    {
        m_renderer->SetTexture(whiteTexId, texUnit);
    }
}

#ifndef _RELEASE

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::DebugSetRecordingOptionForTextureData(int recordingOption)
{
    m_debugTextureDataRecordLevel = recordingOption;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRenderer::DebugDisplayTextureData(int recordingOption)
{
    if (recordingOption > 0)
    {
        // compute the total area of all the textures, also create a vector that we can sort by area
        AZStd::vector<ITexture*> textures;
        int totalArea = 0;
        int totalDataSize = 0;
        for (ITexture* texture : m_texturesUsedInFrame)
        {
            int area = texture->GetWidth() * texture->GetHeight();
            int dataSize = texture->GetDataSize();
            totalArea += area;
            totalDataSize += dataSize;

            textures.push_back(texture);
        }

        // sort the vector by data size
        std::sort( textures.begin( ), textures.end( ), [ ]( const ITexture* lhs, const ITexture* rhs )
        {
            return lhs->GetDataSize() > rhs->GetDataSize();
        });

        IDraw2d* draw2d = Draw2dHelper::GetDraw2d();

        // setup to render lines of text for the debug display
        draw2d->BeginDraw2d(false);

        float xOffset = 20.0f;
        float yOffset = 20.0f;
        float xSpacing = 20.0f;

        int blackTexture = gEnv->pRenderer->GetBlackTextureId();
        float textOpacity = 1.0f;
        float backgroundRectOpacity = 0.75f;
        const float lineSpacing = 20.0f;

        const AZ::Vector3 white(1,1,1);
        const AZ::Vector3 red(1,0.3f,0.3f);
        const AZ::Vector3 blue(0.3f,0.3f,1);

        int xDim, yDim;
        if (totalArea > 2048 * 2048)
        {
            xDim = 4096;
            yDim = totalArea / 4096;
        }
        else
        {
            xDim = 2048;
            yDim = totalArea / 2048;
        }

        float totalDataSizeMB = static_cast<float>(totalDataSize) / (1024.0f * 1024.0f);

        // local function to write a line of text (with a background rect) and increment Y offset
        AZStd::function<void(const char*, const AZ::Vector3&)> WriteLine = [&](const char* buffer, const AZ::Vector3& color)
        {
            IDraw2d::TextOptions textOptions = draw2d->GetDefaultTextOptions();
            textOptions.color = color;
            AZ::Vector2 textSize = draw2d->GetTextSize(buffer, 16, &textOptions);
            AZ::Vector2 rectTopLeft = AZ::Vector2(xOffset - 2, yOffset);
            AZ::Vector2 rectSize = AZ::Vector2(textSize.GetX() + 4, lineSpacing);
            draw2d->DrawImage(blackTexture, rectTopLeft, rectSize, backgroundRectOpacity);
            draw2d->DrawText(buffer, AZ::Vector2(xOffset, yOffset), 16, textOpacity, &textOptions);
            yOffset += lineSpacing;
        };

        int numTexturesUsedInFrame = m_texturesUsedInFrame.size();
        char buffer[200];
        sprintf_s(buffer, "There are %d unique UI textures rendered in this frame, the total texture area is %d (%d x %d), total data size is %d (%.2f MB)",
            numTexturesUsedInFrame, totalArea, xDim, yDim, totalDataSize, totalDataSizeMB);
        WriteLine(buffer, white);
        sprintf_s(buffer, "Dimensions   Data Size   Format Texture name");
        WriteLine(buffer, blue);

        for (ITexture* texture : textures)
        {
            sprintf_s(buffer, "%4d x %4d, %9d %8s %s",
                texture->GetWidth(), texture->GetHeight(), texture->GetDataSize(), texture->GetFormatName(), texture->GetName());
            WriteLine(buffer, white);
        }

        draw2d->EndDraw2d();
    }
}

#endif
