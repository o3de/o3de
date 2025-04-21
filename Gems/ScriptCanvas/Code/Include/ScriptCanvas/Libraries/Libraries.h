/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <ScriptCanvas/Libraries/ScriptCanvasNodeRegistry.h>


namespace AZ
{
    class ReflectContext;
    class ComponentDescriptor;
}

namespace ScriptCanvas
{
    void InitLibraries();

    void ResetLibraries();

    void ReflectLibraries(AZ::ReflectContext*);

    AZStd::vector<AZ::ComponentDescriptor*> GetLibraryDescriptors();

    //! Deprecated, following is to support backward compatibility
    AZ::EnvironmentVariable<NodeRegistry> GetNodeRegistry();

    namespace Library
    {
        struct LibraryDefinition
        {
            AZ_RTTI(LibraryDefinition, "{C7A74062-1577-4925-897F-BB7600D2016D}");

            virtual ~LibraryDefinition() = default;

            static const NodeRegistry::NodeList GetNodes(const AZ::Uuid& libraryType)
            {
                AZ_Warning("ScriptCanvas", false, "LibraryDefinition is deprecated, please migrate to autogen node registry.");
                NodeRegistry& registry = (*GetNodeRegistry());
                const auto& libraryIterator = registry.m_nodeMap.find(libraryType);
                if (libraryIterator != registry.m_nodeMap.end())
                {
                    const NodeRegistry::NodeList& nodeTypes = libraryIterator->second;
                    return nodeTypes;
                }

                return NodeRegistry::NodeList();
            }

            static bool HasNode(const AZ::Uuid& libraryId, const AZ::Uuid& nodeId)
            {
                AZ_Warning("ScriptCanvas", false, "LibraryDefinition is deprecated, please migrate to autogen node registry.");
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

        template<typename NodeGroup, typename NodeType>
        void AddNodeToRegistry(NodeRegistry& nodeRegistry, const AZStd::string_view& nameOverride = {})
        {
            AZ_Warning("ScriptCanvas", false, "LibraryDefinition is deprecated, please migrate to autogen node registry.");
            auto& nodes = nodeRegistry.m_nodeMap[AZ::AzTypeInfo<NodeGroup>::Uuid()];
            nodes.push_back({ AZ::AzTypeInfo<NodeType>::Uuid(), nameOverride.empty() ? AZ::AzTypeInfo<NodeType>::Name() : nameOverride });
        }
    }
}
