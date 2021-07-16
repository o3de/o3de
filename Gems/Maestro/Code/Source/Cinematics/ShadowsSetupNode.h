/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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

    static void Reflect(AZ::ReflectContext* context);

protected:
    virtual bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const;
};

#endif // CRYINCLUDE_CRYMOVIE_SHADOWSSETUPNODE_H
