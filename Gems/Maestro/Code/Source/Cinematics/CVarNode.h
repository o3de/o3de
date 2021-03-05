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

    static void Reflect(AZ::SerializeContext* serializeContext);

protected:
    virtual bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const;

private:
    float m_value;
};

#endif // CRYINCLUDE_CRYMOVIE_CVARNODE_H

