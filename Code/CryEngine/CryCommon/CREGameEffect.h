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
#ifndef _CREGameEffect_
#define _CREGameEffect_

#pragma once

//==================================================================================================
// Name: IREGameEffect
// Desc: Interface for game effect render elements, designed to be instantiated in game code, and
//           called from the CREGameEffect within the engine. This then allows render elements
//       to be created in game code as well as in the engine.
// Author: James Chilvers
//==================================================================================================
struct IREGameEffect
{
    virtual ~IREGameEffect(){}

    virtual void    mfPrepare(bool bCheckOverflow) = 0;
    virtual bool    mfDraw(CShader* ef, SShaderPass* sfm, CRenderObject* renderObj) = 0;
};//------------------------------------------------------------------------------------------------

//==================================================================================================
// Name: CREGameEffect
// Desc: Render element that uses the IREGameEffect interface for its functionality
// Author: James Chilvers
//==================================================================================================
class CREGameEffect
    : public CRendElementBase
{
public:

    CREGameEffect();
    ~CREGameEffect();

    // CRendElementBase interface
    void  mfPrepare(bool bCheckOverflow);
    bool  mfDraw(CShader* ef, SShaderPass* sfm);

    // CREGameEffect interface
    inline void                     SetPrivateImplementation(IREGameEffect* pImpl) { m_pImpl = pImpl; }
    inline IREGameEffect* GetPrivateImplementation() const { return m_pImpl; }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
private:

    IREGameEffect* m_pImpl; // Implementation of of render element
};//------------------------------------------------------------------------------------------------

#endif // #ifndef _CREGameEffect_
