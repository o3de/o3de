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
#include "StereoTexture.h"

#include <AzCore/std/string/string.h>

CStereoTexture::CStereoTexture(const char* szName, ETEX_Format format, int nFlags)
    : CTexture(nFlags)
{
    AZStd::string leftName = szName;
    AZStd::string rightName = szName;
    leftName = leftName.append("_Left");
    rightName = rightName.append("_Right");

    //Create internal textures
    m_textures = AZStd::vector<CTexture*>(STEREO_EYE_COUNT);

    //Create textures with no width or height; 
    //When the VideoPlaybackComponent creates those it can specify an accurate width & height
    m_textures[STEREO_EYE_LEFT] = CTexture::CreateTextureObject(leftName.c_str(), 0, 0, 1, eTT_2D, nFlags, format);
    m_textures[STEREO_EYE_RIGHT] = CTexture::CreateTextureObject(rightName.c_str(), 0, 0, 1, eTT_2D, nFlags, format);
}

void CStereoTexture::Apply(int nTUnit, int nState, int nTexMatSlot, int nSUnit, SResourceView::KeyType nResViewKey, EHWShaderClass eHWSC)
{
    int eye = gRenDev->m_CurRenderEye;
    if (eye < STEREO_EYE_COUNT)
    {
        m_textures[eye]->Apply(nTUnit, nState, nTexMatSlot, nSUnit, nResViewKey, eHWSC);
    }
    else
    {
        AZ_Assert(true, "Invalid eye provided for rendering");
    }
}
