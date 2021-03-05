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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_STARS_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_STARS_H
#pragma once
class CStars
{
public:
    CStars();
    ~CStars();

    void Render(bool bUseMoon);

private:
    bool LoadData();

private:
    uint32 m_numStars;
    _smart_ptr<IRenderMesh> m_pStarMesh;
    CShader* m_pShader;
};

#endif  // #ifndef _STARS_H_
