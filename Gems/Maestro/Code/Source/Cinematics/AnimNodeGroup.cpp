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

#include "Maestro_precompiled.h"
#include <AzCore/Serialization/SerializeContext.h>
#include "AnimNodeGroup.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimParamType.h"

//////////////////////////////////////////////////////////////////////////
CAnimNodeGroup::CAnimNodeGroup()
    : CAnimNodeGroup(0)
{
}

//////////////////////////////////////////////////////////////////////////
CAnimNodeGroup::CAnimNodeGroup(const int id)
    : CAnimNode(id, AnimNodeType::Group) 
{ 
    SetFlags(GetFlags() | eAnimNodeFlags_CanChangeName);
}

CAnimParamType CAnimNodeGroup::GetParamType([[maybe_unused]] unsigned int nIndex) const
{
    return AnimParamType::Invalid;
}

//////////////////////////////////////////////////////////////////////////
void CAnimNodeGroup::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<CAnimNodeGroup, CAnimNode>()
        ->Version(1);
}
