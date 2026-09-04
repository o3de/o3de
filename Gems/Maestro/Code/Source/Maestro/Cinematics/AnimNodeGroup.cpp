/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Serialization/SerializeContext.h>
#include "AnimNodeGroup.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimParamType.h"

namespace Maestro
{

    CAnimNodeGroup::CAnimNodeGroup()
        : CAnimNodeGroup(0)
    {
    }

    CAnimNodeGroup::CAnimNodeGroup(const int id)
        : CAnimNode(id, AnimNodeType::Group)
    {
        SetFlags(GetFlags() | eAnimNodeFlags_CanChangeName);
    }

    CAnimParamType CAnimNodeGroup::GetParamType([[maybe_unused]] unsigned int nIndex) const
    {
        return AnimParamType::Invalid;
    }

    void CAnimNodeGroup::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CAnimNodeGroup, CAnimNode>()->Version(1);
        }
    }

} // namespace Maestro
