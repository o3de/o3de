/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
    class ComponentDescriptor;
} // namespace AZ

namespace ScriptCanvas
{
    struct NodeRegistry final
    {
        AZ_TYPE_INFO(NodeRegistry, "{C1613BD5-3104-44E4-98FE-A917A90B2014}");

        static NodeRegistry* GetInstance();
        static void ResetInstance();

        //! Collection to store ScriptCanvas derived node uuid
        AZStd::vector<AZ::Uuid> m_nodes;

        //! Deprecated field, keep it for backward compatible
        using NodeList = AZStd::vector<AZStd::pair<AZ::Uuid, AZStd::string>>;
        AZStd::unordered_map<AZ::Uuid, NodeList> m_nodeMap;
    };

    static constexpr const char* s_nodeRegistryName = "ScriptCanvasNodeRegistry";
} // namespace ScriptCanvas
