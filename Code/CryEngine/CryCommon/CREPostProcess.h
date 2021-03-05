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

#ifndef CRYINCLUDE_CRYCOMMON_CREPOSTPROCESS_H
#define CRYINCLUDE_CRYCOMMON_CREPOSTPROCESS_H
#pragma once


class CREPostProcess
    : public CRendElementBase
{
    friend class CD3D9Renderer;

public:

    CREPostProcess();
    virtual ~CREPostProcess();

    virtual void mfPrepare(bool bCheckOverflow);
    virtual bool mfDraw(CShader* ef, SShaderPass* sfm);

    // Use for setting numeric values, vec4 (colors, position, vectors, wtv), strings
    virtual int mfSetParameter(const char* pszParam, float fValue, bool bForceValue = false) const;
    virtual int mfSetParameterVec4(const char* pszParam, const Vec4& pValue, bool bForceValue = false) const;
    virtual int mfSetParameterString(const char* pszParam, const char* pszArg) const;

    virtual void mfGetParameter(const char* pszParam, float& fValue) const;
    virtual void mfGetParameterVec4(const char* pszParam, Vec4& pValue) const;
    virtual void mfGetParameterString(const char* pszParam, const char*& pszArg) const;

    virtual int32 mfGetPostEffectID(const char* pPostEffectName) const;

    // Reset all post processing effects
    virtual void Reset(bool bOnSpecChange = false);
    virtual void mfReset()
    {
        Reset();
    }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
};

#endif // CRYINCLUDE_CRYCOMMON_CREPOSTPROCESS_H
