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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_CRELENSOPTICS_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_CRELENSOPTICS_H
#pragma once

class CRELensOptics
    : public CRendElementBase
{
public:
    CRELensOptics(void);
    ~CRELensOptics(void);

    virtual bool mfCompile(CParserBin& Parser, SParserFrame& Frame);
    virtual void mfPrepare(bool bCheckOverflow);
    virtual bool mfDraw(CShader* ef, SShaderPass* sfm);
    virtual void mfExport([[maybe_unused]] struct SShaderSerializeContext& SC) {};
    virtual void mfImport([[maybe_unused]] struct SShaderSerializeContext& SC, [[maybe_unused]] uint32& offset) {};

    void ProcessGlobalAction();

    static void ClearResources();
};

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_CRELENSOPTICS_H
