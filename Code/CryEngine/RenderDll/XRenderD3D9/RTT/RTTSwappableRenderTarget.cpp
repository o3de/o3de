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

#include "RenderDll_precompiled.h"
#include "RTTSwappableRenderTarget.h"
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/std/string/conversions.h>

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

namespace AzRTT 
{
    const int INVALID_RENDER_TARGET = -1;

    SwappableRenderTarget::SwappableRenderTarget()
        : m_originalTexture(nullptr)
        , m_rtt(nullptr)
        , m_scale(1)
        , m_rttId(INVALID_RENDER_TARGET)
        , m_originalTextureId(INVALID_RENDER_TARGET)
        , m_renderContextId(AzRTT::RenderContextId::CreateNull())
        , m_swapped(false)
        , m_width(0)
        , m_height(0)
    {
    }

    SwappableRenderTarget::SwappableRenderTarget(CTexture** texture)
        : m_originalTexture(texture)
        , m_rtt(nullptr)
        , m_scale(1)
        , m_rttId(INVALID_RENDER_TARGET)
        , m_originalTextureId(INVALID_RENDER_TARGET)
        , m_renderContextId(AzRTT::RenderContextId::CreateNull())
        , m_swapped(false)
        , m_width(0)
        , m_height(0)
    {
    }

    SwappableRenderTarget::SwappableRenderTarget(const SwappableRenderTarget& a)
        : m_originalTexture(a.m_originalTexture)
        , m_rtt(a.m_rtt)
        , m_scale(a.m_scale)
        , m_rttId(a.m_rttId)
        , m_originalTextureId(a.m_originalTextureId)
        , m_renderContextId(a.m_renderContextId)
        , m_swapped(a.m_swapped)
        , m_width(a.m_width)
        , m_height(a.m_height)
    {
    }

    SwappableRenderTarget::~SwappableRenderTarget()
    {
        Revert();
        Release();
    }

    void SwappableRenderTarget::Release()
    {
        if (m_rttId != INVALID_RENDER_TARGET)
        {
            ITexture* renderTarget = GetTextureById(m_rttId);
            if (renderTarget)
            {
                if (renderTarget->GetFlags() & FT_DONT_RELEASE)
                {
                    SAFE_RELEASE_FORCE(renderTarget);
                }
                else
                {
                    SAFE_RELEASE(renderTarget);
                }
            }
        }
        m_rtt = nullptr;
    }

    bool SwappableRenderTarget::IsSwapped() const
    {
        return m_swapped;
    }

    bool SwappableRenderTarget::IsValid() const
    {
        ITexture* rtt = GetTextureById(m_rttId);
        ITexture* originalTexture = GetTextureById(m_originalTextureId);

        // in order to be valid the original texture and swap texture must exist 
        // in a swapped, or not swapped state.
        return m_originalTexture && *m_originalTexture && m_rtt && rtt && originalTexture && 
            originalTexture->GetDevTexture() && rtt->GetDevTexture() &&
            ((*m_originalTexture == originalTexture && m_rtt == rtt) || (*m_originalTexture == rtt && m_rtt == originalTexture));
    }

    void SwappableRenderTarget::Revert()
    {
        if (IsSwapped())
        {
            Swap();
        }
    }

    CTexture* SwappableRenderTarget::GetTextureById(int id) const
    {
        // can't use CTexture::GetById because it will return the default 
        // texture if this id is invalid, so manually search for the resource 
        if (id == INVALID_RENDER_TARGET)
        {
            return nullptr;
        }

        bool addReference = false;
        return static_cast<CTexture*>(CBaseResource::GetResource(CTexture::mfGetClassName(), id, addReference));
    }

    void SwappableRenderTarget::Swap()
    {
        if (!IsValid())
        {
            // Try to recover from a state where the renderer changed our texture 
            if (m_originalTexture && *m_originalTexture &&  
                m_originalTextureId != INVALID_RENDER_TARGET &&
                m_rtt && *m_originalTexture != m_rtt)
            {
                if (m_rtt == *m_originalTexture)
                {
                    // We cannot recover from this state because the renderer 
                    // changed a static texture, usually be referencing or 
                    // re-creating a texture by name (string) while a texture was
                    // swapped.  Usually this means the renderer will re-create 
                    // the texture every pass. 
                    AZ_Warning("SwappableRenderTarget", false, "SwappableRenderTarget %s has been re-created", m_rtt->GetName());
                }
                else
                {
                    // The renderer may have changed a texture size ($StereoR)
                    // or format. This can happen if the main camera is
                    // not using the same HDR settings as the RT or when a
                    // viewport is resized in the editor.
#ifdef _DEBUG
                    AZ_Warning("SwappableRenderTarget", false, "SwappableRenderTarget no longer valid, re-creating.");
#endif //ifdef _DEBUG

                    const ETEX_Format desiredFormat = (*m_originalTexture)->GetDstFormat();

                    // revert the swapped state
                    bool wasSwapped = m_swapped;
                    if (m_swapped)
                    {
                        AZStd::swap(*m_originalTexture, m_rtt);
                    }

                    // restore the original texture based on the ID
                    *m_originalTexture = GetTextureById(m_originalTextureId);

                    // release the render texture copy
                    Release();

                    // re-create the render target copy
                    CreateRenderTargetCopy(m_width, m_height, m_scale, m_renderContextId, desiredFormat);

                    // restore the swapped state
                    if (wasSwapped == m_swapped)
                    {
                        AZStd::swap(*m_originalTexture, m_rtt);
                        m_swapped = !m_swapped;
                    }
                }
            }
        }
        else
        {
            AZStd::swap(*m_originalTexture, m_rtt);
            m_swapped = !m_swapped;
        }
    }

    void SwappableRenderTarget::CreateRenderTargetCopy(RenderContextId id)
    {
        if (m_originalTexture && *m_originalTexture)
        {
            const uint32_t scale = 1;
            CreateRenderTargetCopy((*m_originalTexture)->GetWidth(), (*m_originalTexture)->GetHeight(), scale, id);
        }
    }

    void SwappableRenderTarget::CreateRenderTargetCopy(uint32_t width, uint32_t height, uint32_t scale, RenderContextId id, ETEX_Format formatOverride)
    {
        m_scale = scale;
        m_width = width;
        m_height = height;
        m_renderContextId = id;

        if (m_originalTexture && *m_originalTexture)
        {
            const CTexture* originalTexture = *m_originalTexture;
            m_originalTextureId = originalTexture->GetID();
            m_swapped = false;

            // allow overriding the format which can change based on the render pass settings
            const ETEX_Format format = formatOverride == eTF_Unknown ? originalTexture->GetDstFormat() : formatOverride;
            const ETEX_Type texType  = originalTexture->GetTexType();
            const uint32_t flags     = originalTexture->GetFlags() | FT_DONT_STREAM | FT_USAGE_RENDERTARGET;
            const ColorF clearColor  = originalTexture->GetClearColor();
            const int customId       = originalTexture->GetCustomID();
            const int mips           = originalTexture->GetNumMips();

            // slice support check necessary to avoid __debugbreak() in StreamGetNumSlices
            bool supportsSlices     = texType == eTT_2D || texType == eTT_2DArray || texType == eTT_Cube;
            const int numSlices     = supportsSlices ? originalTexture->StreamGetNumSlices() : 1;

            const uint32_t scaledWidth  = width / m_scale;
            const uint32_t scaledHeight = height / m_scale;

            AZ_Assert(scaledWidth != 0 && scaledHeight != 0, "Invalid scaled width/height for render target copy.");

            const AZStd::string name = GetRenderTargetCopyName(originalTexture->GetName());

            if (originalTexture->GetDevTexture())
            {
                if (numSlices > 1)
                {
                    m_rtt = CTexture::CreateTextureArray(name.c_str(), eTT_2D, scaledWidth, scaledHeight, numSlices, mips, flags, format, customId);
                }
                else
                {
                    m_rtt = CTexture::CreateRenderTarget(name.c_str(), scaledWidth, scaledHeight, clearColor, eTT_2D, flags, format, customId);
                }
            }
            else
            {
                m_rtt = CTexture::CreateTextureObject(name.c_str(), scaledWidth, scaledHeight, numSlices, eTT_2D, flags, format, customId);
            }

            if (m_rtt)
            {
                m_rtt->SetClearColor(clearColor);
                m_rttId = m_rtt->GetID();
            }
            else
            {
                m_rttId = INVALID_RENDER_TARGET;
            }
        }
    }

    AZStd::string SwappableRenderTarget::GetRenderTargetCopyName(const char* textureName)
    {
        AZ_Assert(textureName, "Invalid texture name for render target copy");

        // prepend $RTT so we can match these easily when swapping shader samplers and to identify when debugging
        const AZStd::string prefix("$RTT");

        AZStd::string name(textureName);
        AzFramework::StringFunc::Replace(name, "$", prefix.c_str(), true, false);

        // append the RenderContextId after a slash because some textures end in numbers so this 
        // makes the name more readable when debugging
        name += AZStd::string("_"); 
        name += m_renderContextId.ToString<AZStd::string>();

        return name;
    }

    void SwappableRenderTarget::Resize(uint32_t width, uint32_t height)
    {
        if (m_rttId != INVALID_RENDER_TARGET)
        {
            m_width = width;
            m_height = height;
            gEnv->pRenderer->ResizeRenderTarget(m_rttId, width / m_scale, height / m_scale);
        }
    }
}

#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED