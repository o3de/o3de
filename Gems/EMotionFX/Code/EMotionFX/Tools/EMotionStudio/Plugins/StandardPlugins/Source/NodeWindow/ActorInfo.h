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
        int                          m_nodeCount;
        AZStd::vector<NodeGroupInfo> m_nodeGroups;
        unsigned int                 m_totalVertices;
        unsigned int                 m_totalIndices;
    };
} // namespace EMStudio
