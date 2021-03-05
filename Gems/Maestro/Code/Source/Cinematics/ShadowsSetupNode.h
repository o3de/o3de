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

// Description : CryMovie animation node for shadow settings


#ifndef CRYINCLUDE_CRYMOVIE_SHADOWSSETUPNODE_H
#define CRYINCLUDE_CRYMOVIE_SHADOWSSETUPNODE_H
#pragma once


#include "AnimNode.h"

class CShadowsSetupNode
    : public CAnimNode
{
public:
    AZ_CLASS_ALLOCATOR(CShadowsSetupNode, AZ::SystemAllocator, 0);
    AZ_RTTI(CShadowsSetupNode, "{419F9F77-FC64-43D1-ABCF-E78E90889DF8}", CAnimNode);

    CShadowsSetupNode();
    CShadowsSetupNode(const int id);
    static void Initialize();

    //-----------------------------------------------------------------------------
    //! Overrides from CAnimNode
    virtual void Animate(SAnimContext& ac);

    virtual void CreateDefaultTracks();

    virtual void OnReset();

    //-----------------------------------------------------------------------------
    //! Overrides from IAnimNode
    virtual unsigned int GetParamCount() const;
    virtual CAnimParamType GetParamType(unsigned int nIndex) const;

    static void Reflect(AZ::SerializeContext* serializeContext);

protected:
    virtual bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const;
};

#endif // CRYINCLUDE_CRYMOVIE_SHADOWSSETUPNODE_H
