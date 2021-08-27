/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
