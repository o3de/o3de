/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/string/string.h>

namespace EMotionFX
{
    class ActorInstance;
}

namespace EMStudio
{
    class NodeGroupInfo;

    class ActorInfo final
    {
    public:
        AZ_RTTI(ActorInfo, "{72F3E145-5308-49D5-9509-320AA2D5EAF1}")
        AZ_CLASS_ALLOCATOR_DECL

        ActorInfo() {}
        ActorInfo(const EMotionFX::ActorInstance* actorInstance);
        ~ActorInfo() = default;

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::string                m_name;
        AZStd::string                m_unitType;
        AZ::u64                      m_nodeCount;
        AZStd::vector<NodeGroupInfo> m_nodeGroups;
        unsigned int                 m_totalVertices;
        unsigned int                 m_totalIndices;
    };
} // namespace EMStudio
