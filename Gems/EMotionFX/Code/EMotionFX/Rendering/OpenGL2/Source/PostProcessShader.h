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

#ifndef __RENDERGL_POSTPROCESS_SHADER_H
#define __RENDERGL_POSTPROCESS_SHADER_H

#include "GLSLShader.h"
#include "RenderTexture.h"


namespace RenderGL
{
    class RENDERGL_API PostProcessShader
        : public GLSLShader
    {
        MCORE_MEMORYOBJECTCATEGORY(PostProcessShader, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERING);

    public:
        PostProcessShader();
        ~PostProcessShader();

        void ActivateRT(RenderTexture* target);
        void ActivateRT(Texture* source, RenderTexture* target);
        void ActivateFromGBuffer(RenderTexture* target);

        void Deactivate() override;

        bool Init(const char* filename);
        void Render();

    private:

        RenderTexture* mRT;
    };
}

#endif
