/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYMOVIE_SCRIPTVARNODE_H
#define CRYINCLUDE_CRYMOVIE_SCRIPTVARNODE_H
#pragma once

#include "AnimNode.h"

class CAnimScriptVarNode
    : public CAnimNode
{
public:
    AZ_CLASS_ALLOCATOR(CAnimScriptVarNode, AZ::SystemAllocator, 0);
    AZ_RTTI(CAnimScriptVarNode, "{D93FC866-A158-4C00-AB03-29DC7D3CCCFF}", CAnimNode);

    CAnimScriptVarNode(const int id);
    CAnimScriptVarNode();

    //////////////////////////////////////////////////////////////////////////
    // Overrides from CAnimNode
    //////////////////////////////////////////////////////////////////////////
    void Animate(SAnimContext& ec);
    void CreateDefaultTracks();
    void OnReset();
    void OnResume();

    //////////////////////////////////////////////////////////////////////////
    virtual unsigned int GetParamCount() const;
    virtual CAnimParamType GetParamType(unsigned int nIndex) const;
    virtual bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const;

    static void Reflect(AZ::ReflectContext* context);

private:
    float m_value;
};

#endif // CRYINCLUDE_CRYMOVIE_SCRIPTVARNODE_H
