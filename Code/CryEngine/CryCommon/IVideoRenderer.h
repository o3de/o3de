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

#include <ITexture.h>

namespace AZ
{
    // General purpose video 'rendering' solution that abstracts video data into textures
    // and data to update those textures with.
    namespace VideoRenderer
    {
        enum Constants
        {
            MaxInputTextureCount = 4,
        };

        struct VideoTextureDesc
        {
            CryFixedStringT<64> m_name; // fixed string to avoid dll string copying issues
            uint32              m_width{ 4 };
            uint32              m_height{ 4 };
            ETEX_Format         m_format{ eTF_Unknown };
            uint32              m_used{ 0 };
        };

        // Full description of video texture resources for the renderer to create.
        struct VideoTexturesDesc
        {
            VideoTextureDesc m_outputTextureDesc;
            VideoTextureDesc m_inputTextureDescs[MaxInputTextureCount];
        };

        // Full set of textures created from the VideoTexturesDesc provided to the renderer.
        struct VideoTextures
        {
            uint32  m_outputTextureId{ 0 };
            uint32  m_inputTextureIds[MaxInputTextureCount]{ 0 };
        };

        struct VideoUpdateData
        {
            struct VideoTextureUpdateData
            {
                // Data to update the texture with, can be null.
                const void* m_data{ nullptr };

                // Format of above data, required for format conversions if needed.
                ETEX_Format m_dataFormat{ eTF_Unknown };
            }
            m_inputTextureData[MaxInputTextureCount];
        };

        // Set of data to update and render a frame of video textures.
        // Everything should be passed through by value except for the update data, which should be double buffered at the source.
        struct DrawArguments
        {
            // Set of textures to draw with.
            VideoTextures   m_textures;

            // Set of data to update the above textures with if set.
            VideoUpdateData m_updateData;

            // Flag to indicate that we want to draw to the backbuffer.
            uint32          m_drawingToBackbuffer{ 0 };

            // Payload information for reference. Useful for debugging.
            uint32          m_frameReference{ 0 };

            // Scale applied to each texture.
            Vec4            m_textureScales[MaxInputTextureCount]{};

            // Value added to final composited texture.
            Vec4            m_colorAdjustment{ ZERO };
        };

        // Video Rendering interface to provide callbacks from the Render Thread
        struct IVideoRenderer
        {
            // Called from the Render Thread to request the description of the video textures.
            virtual bool   GetVideoTexturesDesc(AZ::VideoRenderer::VideoTexturesDesc& videoTexturesDesc) const = 0;
            // Called from the Render Thread to get the set of video textures that were previously created. Used at cleanup time.
            virtual bool   GetVideoTextures(AZ::VideoRenderer::VideoTextures& videoTextures) const = 0;

            // Called from the Render Thread to provide the video textures it created from the VideoTexturesDesc.
            virtual bool   NotifyTexturesCreated(const AZ::VideoRenderer::VideoTextures& videoTextures) = 0;
            // CAlled from the Render Thread to notify the video manager that its textures were destroyed.
            virtual bool   NotifyTexturesDestroyed() = 0;
        };
    }
}
