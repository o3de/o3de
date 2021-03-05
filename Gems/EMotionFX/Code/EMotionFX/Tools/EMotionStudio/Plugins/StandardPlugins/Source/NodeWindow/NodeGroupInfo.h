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

#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/ReflectContext.h>

namespace  EMotionFX
{
    class Actor;
    class NodeGroup;
}

namespace EMStudio
{
    class NodeGroupInfo final
    {
    public:
        AZ_RTTI(NodeGroupInfo, "{DBB0784B-6F32-4C2B-B56E-7C606875FEDD}")
        AZ_CLASS_ALLOCATOR_DECL

        NodeGroupInfo() {}
        NodeGroupInfo(EMotionFX::Actor* actor, EMotionFX::NodeGroup* nodeGroup);
        ~NodeGroupInfo() = default;

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::string                m_name;
        AZStd::vector<AZStd::string> m_nodes;
    };
} // namespace EMStudio
