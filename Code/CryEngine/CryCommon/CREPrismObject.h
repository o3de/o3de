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
#ifndef _CREPRISMOBJECT_
#define _CREPRISMOBJECT_

#pragma once

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)

class CREPrismObject
    : public CRendElementBase
{
public:
    CREPrismObject();

    virtual ~CREPrismObject() {}
    virtual void mfPrepare(bool bCheckOverflow);
    virtual bool mfDraw(CShader* ef, SShaderPass* sfm);

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

    Vec3 m_center;
};

#endif // EXCLUDE_DOCUMENTATION_PURPOSE

#endif // _CREPRISMOBJECT_
