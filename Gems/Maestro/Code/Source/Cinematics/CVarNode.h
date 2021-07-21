/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYMOVIE_CVARNODE_H
#define CRYINCLUDE_CRYMOVIE_CVARNODE_H
#pragma once

#include "AnimNode.h"

class CAnimCVarNode
    : public CAnimNode
{
public:
    AZ_CLASS_ALLOCATOR(CAnimCVarNode, AZ::SystemAllocator, 0);
    AZ_RTTI(CAnimCVarNode, "{9059B454-EE73-4865-9B76-8C8430E3BB82}", CAnimNode);

    CAnimCVarNode(const int id);
    CAnimCVarNode();

    //////////////////////////////////////////////////////////////////////////
    // Overrides from CAnimNode
    //////////////////////////////////////////////////////////////////////////
    void SetName(const char* name);
    void Animate(SAnimContext& ec);
    void CreateDefaultTracks();
    void OnReset();
    void OnResume();

    virtual unsigned int GetParamCount() const;
    virtual CAnimParamType GetParamType(unsigned int nIndex) const;

    int GetDefaultKeyTangentFlags() const override;

    static void Reflect(AZ::ReflectContext* context);

protected:
    virtual bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const;

private:
    float m_value;
};

#endif // CRYINCLUDE_CRYMOVIE_CVARNODE_H

