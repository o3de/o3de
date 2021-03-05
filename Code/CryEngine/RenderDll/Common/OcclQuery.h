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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_OCCLQUERY_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_OCCLQUERY_H
#pragma once

class COcclusionQuery
{
public:
    COcclusionQuery()
        : m_nVisSamples(~0)
        , m_nCheckFrame(0)
        , m_nDrawFrame(0)
        , m_nOcclusionID(0)
    {
    }

    ~COcclusionQuery()
    {
        Release();
    }

    void Create();
    void Release();

    void BeginQuery();
    void EndQuery();

    uint32 GetVisibleSamples(bool bAsynchronous);

    int GetDrawFrame() const
    {
        return m_nDrawFrame;
    }

    bool IsReady();

    bool IsCreated() const { return m_nOcclusionID != 0; }

private:

    int m_nVisSamples;
    int m_nCheckFrame;
    int m_nDrawFrame;

    UINT_PTR m_nOcclusionID; // this will carry a pointer D3DQuery, so it needs to be 64-bit on Windows 64
};


#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_OCCLQUERY_H
