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
#include "..\Common\RenderCapabilities.h"

namespace RenderCapabilities
{
    bool SupportsTextureViews()
    {
        return true;
    }

    bool SupportsStencilTextures()
    {
        return true;
    }

    bool SupportsPLSExtension()
    {
        return false;
    }

    FrameBufferFetchMask GetFrameBufferFetchCapabilities()
    {
        return FrameBufferFetchMask();
    }

    bool SupportsDepthClipping()
    {
        return true;
    }

    bool SupportsDualSourceBlending()
    {
        return true;
    }

    bool SupportsStructuredBuffer([[maybe_unused]] EShaderStage stage)
    {
        return true;
    }

    bool SupportsIndependentBlending()
    {
        return true;
    }
}
