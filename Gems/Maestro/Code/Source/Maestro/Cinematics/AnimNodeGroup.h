/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Anim Node Group

#pragma once

#include <IMovieSystem.h>
#include "AnimNode.h"

namespace Maestro
{

    //////////////////////////////////////////////////////////////////////////
    class CAnimNodeGroup : public CAnimNode
    {
    public:
        AZ_CLASS_ALLOCATOR(CAnimNodeGroup, AZ::SystemAllocator);
        AZ_RTTI(CAnimNodeGroup, "{6BDA5C06-7C15-4622-9550-68368E84D653}", CAnimNode);

        CAnimNodeGroup();
        explicit CAnimNodeGroup(const int id);

        CAnimParamType GetParamType(unsigned int nIndex) const override;

        static void Reflect(AZ::ReflectContext* context);
    };

} // namespace Maestro
