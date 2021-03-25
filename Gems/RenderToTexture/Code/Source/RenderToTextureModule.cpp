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

#include "RenderToTexture_precompiled.h"

#include "RenderToTextureModule.h"
#include "RenderToTextureComponent.h"

#if defined(RENDER_TO_TEXTURE_EDITOR)
#include "EditorRenderToTextureComponent.h"
#endif

namespace RenderToTexture
{
    RenderToTextureModule::RenderToTextureModule()
        : CryHooksModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
            RenderToTextureComponent::CreateDescriptor(),
#if defined(RENDER_TO_TEXTURE_EDITOR)
            EditorRenderToTextureComponent::CreateDescriptor(),
#endif
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
        });
    }

    void RenderToTextureModule::OnCrySystemCVarRegistry()
    {
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
        REGISTER_CVAR_DEV_ONLY(rtt_aa, -1, 0, "Override AA mode used by all render to texture components.");
        REGISTER_CVAR_DEV_ONLY(rtt_dof, -1, 0, "Override Depth of Field mode used by all render to texture components.");
        REGISTER_CVAR_DEV_ONLY(rtt_motionblur, -1, 0, "Override MotionBlur mode used by all render to texture components.");
        REGISTER_CVAR_DEV_ONLY(rtt_maxFPS, -1.0f, 0, "Override the maxfps setting.  -1 don't override, 0 disable fps limiting, 1+ limit to this amount  .");
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    }
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_RenderToTexture, RenderToTexture::RenderToTextureModule)
