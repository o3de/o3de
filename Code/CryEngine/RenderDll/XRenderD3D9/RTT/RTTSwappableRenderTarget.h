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

#include "RenderContextConfig.h"

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

class CTexture;

namespace AzRTT
{
    class SwappableRenderTarget
    {
    public:
        SwappableRenderTarget();
        SwappableRenderTarget(CTexture** texture);
        SwappableRenderTarget(const SwappableRenderTarget& a);

        virtual ~SwappableRenderTarget();

        /**
        * Return true if original texture and copy are swapped
        * @return true if the textures are swapped
        */
        bool IsSwapped() const;

        /**
        * Return true if the original texture and copy exist and are valid
        * @return true if the original texture and copy exist and are valid
        */
        bool IsValid() const;

        /**
        * Revert the swap state
        */
        void Revert();

        /**
        * Swap the original texture and the copy 
        */
        void Swap();

        /**
        * Create a render target copy object. 
        * @param id the RenderContextId this copy belongs to
        */
        void CreateRenderTargetCopy(RenderContextId id);

        /**
        * Create a render target copy object with some common differences.
        * We provide the width, height and scale here to make it easier to resize later.
        * The texture copy will have the dimensions x: width/scale, y: height/scale
        * @param width the width of the original 
        * @param height the height of the original
        * @param scale the scale to divide the original width and height by
        * @param id the RenderContextId this copy belongs to
        */
        void CreateRenderTargetCopy(uint32_t width, uint32_t height, uint32_t scale, RenderContextId id, ETEX_Format format = eTF_Unknown);

        /**
        * Resize the render target using the existing scale 
        * The texture copy will have the dimensions x: width/m_scale, y: height/m_scale
        * @param width the width of the original in pixels
        * @param height the height of the original in pixels
        */
        void Resize(uint32_t width, uint32_t height);

    private:
        /**
        * Helper method to get a texture by id. 
        * @return the texture or nullptr if not found 
        */
        CTexture* GetTextureById(int id) const;

        /**
        * Release resources. 
        */
        void Release();

        /**
        * Get a name to use for the render target copy with custom prefix and suffix 
        * Ex. If textureName was "original" copy will be $RTToriginal_1234-5678-90AB-CDEF
        * @param textureName the original texture's name 
        * @return the texture name of the copy 
        */
        AZStd::string GetRenderTargetCopyName(const char* textureName);

        CTexture** m_originalTexture;
        CTexture* m_rtt;

        // texture resource IDs used to detect invalid pointers
        int m_rttId;
        int m_originalTextureId;

        //! the ID of the render context that owns this texture, used for name generation
        RenderContextId m_renderContextId;

        // store the scale factor so we can resize easily
        uint32_t m_scale;
        uint32_t m_width;
        uint32_t m_height;
        bool m_swapped;
    };
}

#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED