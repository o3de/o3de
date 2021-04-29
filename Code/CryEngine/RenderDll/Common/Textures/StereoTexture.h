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

/*
    This stereo texture does two things:
    1) Extends the base CTexture class in order to provide information about 
       the actual underlying textures.
    2) Stores two underlying CTextures and returns their IDs based on whether
       the system is presenting to the left eye or the right eye.
*/

#include "Texture.h"
#include "IStereoRenderer.h"
#include "../Common/CommonRender.h"
#include <AzCore/std/containers/vector.h>

class CStereoTexture 
    : public CTexture
{
public:
    CStereoTexture(const char* szName, ETEX_Format format, int nFlags);
    virtual ~CStereoTexture() = default;
    
    //Based on which eye the renderer currently wants to use this apply
    //either the left texture or the right texture
    void Apply(int nTUnit, int nState = -1, int nTexMatSlot = EFTT_UNKNOWN, int nSUnit = -1, SResourceView::KeyType nResViewKey = SResourceView::DefaultView, EHWShaderClass eHWSC = eHWSC_Pixel) override;
    
    AZStd::vector<CTexture*> m_textures;
};
