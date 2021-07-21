/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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

    static void Reflect(AZ::ReflectContext* context);
};

#endif // CRYINCLUDE_CRYMOVIE_ANIMNODEGROUP_H
