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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_IMAGESPACESHAFTS_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_IMAGESPACESHAFTS_H
#pragma once

#include "OpticsElement.h"

class CTexture;
class CD3D9Renderer;

class ImageSpaceShafts
    : public COpticsElement
{
protected:
    static CTexture* m_pOccBuffer;
    static CTexture* m_pDraftBuffer;

protected:
    bool m_bHighQualityMode;
    bool m_bTexDirty;
    _smart_ptr<CTexture> m_pGoboTex;

    virtual void InitTextures();

public:

    ImageSpaceShafts(const char* name)
        : COpticsElement(name)
        , m_bTexDirty(true)
        , m_bHighQualityMode(false)
    {
        m_Color.a = 1.f;
        SetSize(0.7f);
    }

    EFlareType GetType() { return eFT_ImageSpaceShafts; }

    void InitEditorParamGroups(AZStd::vector<FuncVariableGroup>& groups);
    void Render(CShader* shader, Vec3 vSrcWorldPos, Vec3 vSrcProjPos, SAuxParams& aux);
    void Load(IXmlNode* pNode);

    bool IsHighQualityMode() const { return m_bHighQualityMode; }
    void SetHighQualityMode(bool b) { m_bHighQualityMode = b; m_bTexDirty = true; }

    CTexture* GetGoboTex();
    void SetGoboTex(CTexture* tex) { m_pGoboTex = tex; }
};

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_IMAGESPACESHAFTS_H
