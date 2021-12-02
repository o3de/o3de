/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __RENDERGL_POSTPROCESS_SHADER_H
#define __RENDERGL_POSTPROCESS_SHADER_H

#include <AzCore/IO/Path/Path_fwd.h>
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

        bool Init(AZ::IO::PathView filename);
        void Render();

    private:

        RenderTexture* m_rt;
    };
}

#endif
