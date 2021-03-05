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

#ifndef __RENDERGL_RENDERTEXTURE_H
#define __RENDERGL_RENDERTEXTURE_H

#include "TextureCache.h"
#include <MCore/Source/Color.h>
#include "RenderGLConfig.h"


namespace RenderGL
{
    class RENDERGL_API RenderTexture
        : public Texture
    {
        MCORE_MEMORYOBJECTCATEGORY(RenderTexture, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERING);

    public:
        /**
         * Default constructor.
         */
        RenderTexture();

        /**
         * Destructor.
         */
        ~RenderTexture();

        /**
         * Activate the render texture.
         */
        void Activate();

        /**
         * Clear the render texture using the given color.
         * @param color The new background color of the render texture. Default value is black.
         */
        void Clear(const MCore::RGBAColor& color = MCore::RGBAColor(0.0f, 0.0f, 0.0f, 0.0f));

        /**
         * Deactivate the render texture.
         */
        void Deactivate();

        void Render();

        AZ::u32 GetDepthBuffer() const                                  { return mDepthBuffer; }
        int32 GetFormat() const                                         { return mFormat; }

        /**
         *    Formats:   GL_RGBA32F_ARB
         *               GL_RGBA16F_ARB
         *               GL_RGBA8
         */
        bool Init(int32 format, uint32 width, uint32 height, AZ::u32 depthBuffer = 0);

    private:
        int32   mFormat;        /*< . */
        uint32  mPrevHeight;    /*< . */
        uint32  mPrevWidth;     /*< . */
        AZ::u32 mFrameBuffer;
        AZ::u32 mDepthBuffer;
    };
}

#endif