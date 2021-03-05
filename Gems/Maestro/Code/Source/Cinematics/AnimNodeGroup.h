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

// Description : Anim Node Group

#ifndef CRYINCLUDE_CRYMOVIE_ANIMNODEGROUP_H
#define CRYINCLUDE_CRYMOVIE_ANIMNODEGROUP_H

#pragma once

#include "IMovieSystem.h"
#include "AnimNode.h"

//////////////////////////////////////////////////////////////////////////
class CAnimNodeGroup
    : public CAnimNode
{
public:
    AZ_CLASS_ALLOCATOR(CAnimNodeGroup, AZ::SystemAllocator, 0);
    AZ_RTTI(CAnimNodeGroup, "{6BDA5C06-7C15-4622-9550-68368E84D653}", CAnimNode);

    CAnimNodeGroup();
    CAnimNodeGroup(const int id);

    virtual CAnimParamType GetParamType(unsigned int nIndex) const;

    static void Reflect(AZ::SerializeContext* serializeContext);
};

#endif // CRYINCLUDE_CRYMOVIE_ANIMNODEGROUP_H