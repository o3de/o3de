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

        using NodeList = AZStd::vector<AZStd::pair<AZ::Uuid, AZStd::string>>;

        AZStd::unordered_map<AZ::Uuid, NodeList> m_nodeMap;
    };

    NodeRegistry* GetNodeRegistry();
    void ResetNodeRegistry();
    static const char* s_nodeRegistryName = "ScriptCanvasNodeRegistry";

    namespace Library
    {
        struct LibraryDefinition
        {
            AZ_RTTI(LibraryDefinition, "{C7A74062-1577-4925-897F-BB7600D2016D}");

            virtual ~LibraryDefinition() = default;

            static const NodeRegistry::NodeList GetNodes(const AZ::Uuid& libraryType)
            {
                NodeRegistry* registry = GetNodeRegistry();
                const auto& libraryIterator = registry->m_nodeMap.find(libraryType);
                if (libraryIterator != registry->m_nodeMap.end())
                {
                    const NodeRegistry::NodeList& nodeTypes = libraryIterator->second;
                    return nodeTypes;
                }

                return NodeRegistry::NodeList();
            }

            static bool HasNode(const AZ::Uuid& libraryId, const AZ::Uuid& nodeId)
            {
                NodeRegistry::NodeList nodes = GetNodes(libraryId);
                for (const auto& node : nodes)
                {
                    if (node.first == nodeId)
                    {
                        return true;
                    }
                }
                return false;
            }
        };

        struct CustomLibrary
            : public LibraryDefinition
        {
            AZ_RTTI(CustomLibrary, "{C8AF36B8-90B4-4DFE-949F-A6A52ED8AA2E}", LibraryDefinition);

            static void Reflect(AZ::ReflectContext*);

            ~CustomLibrary() override = default;
        };
    } // namespace Library

} // namespace ScriptCanvas
