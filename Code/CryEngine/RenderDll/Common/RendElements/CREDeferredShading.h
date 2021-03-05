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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Deferred shading processing render element


#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_CREDEFERREDSHADING_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_CREDEFERREDSHADING_H
#pragma once



class CREDeferredShading
    : public CRendElementBase
{
    friend class CD3D9Renderer;
    friend class CGLRenderer;

public:

    // constructor/destructor
    CREDeferredShading();

    virtual ~CREDeferredShading();

    // prepare screen processing
    virtual void mfPrepare(bool bCheckOverflow);
    // render screen processing
    virtual bool mfDraw(CShader* ef, SShaderPass* sfm);

    // begin screen processing
    virtual void mfActivate(int iProcess);
    // reset
    virtual void mfReset(void);

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
};

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_CREDEFERREDSHADING_H
