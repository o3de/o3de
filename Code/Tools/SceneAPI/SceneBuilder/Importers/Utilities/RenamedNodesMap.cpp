/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/conversions.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneBuilder/Importers/Utilities/RenamedNodesMap.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            bool RenamedNodesMap::SanitizeNodeName(AZStd::string& name, const Containers::SceneGraph& graph,
                Containers::SceneGraph::NodeIndex parentNode, const char* defaultName)
            {
                AZ_TraceContext("Node name", name);
                const AZStd::string originalNodeName(name);

                bool isNameUpdated = false;
                // Nodes can't have an empty name, except of the root, otherwise nodes can't be referenced.
                if (name.empty())
                {
                    name = defaultName;
                    isNameUpdated = true;
                }

                // The scene graph uses an arbitrary character (by default dot) to separate the names of the parents
                // therefore that character can't be used in the name.
                AZStd::replace_if(name.begin(), name.end(),
                    [&isNameUpdated](char c) -> bool
                {
                    if (c == Containers::SceneGraph::GetNodeSeperationCharacter())
                    {
                        isNameUpdated = true;
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }, '_');

                // Nodes under a particular parent have to be unique. Multiple nodes can share the same name, but they
                // can't reference the same parent in that case. This is to make sure the node can be quickly found as
                // the full path will be unique. To fix any issues, an index is appended.
                size_t index = 1;
                const size_t offset = name.length();
                while (graph.Find(parentNode, name).IsValid())
                {
                    // Remove the previously tried extension.
                    name.erase(offset, name.length() - offset);

                    name += ('_');
                    name += AZStd::to_string(aznumeric_cast<u64>(index));
                    index++;
                    isNameUpdated = true;
                }

                if (isNameUpdated)
                {
                    AZ_TraceContext("New node name", name);
                    AZ_TracePrintf(Utilities::WarningWindow, "The name of the node '%s' was invalid or conflicting and was updated to '%s'.",
                        originalNodeName.c_str(), name.c_str());
                }

                return isNameUpdated;
            }

            bool RenamedNodesMap::RegisterNode(const std::shared_ptr<SDKNode::NodeWrapper>& node, const Containers::SceneGraph& graph,
                Containers::SceneGraph::NodeIndex parentNode, const char* defaultName)
            {
                return node ? RegisterNode(*node, graph, parentNode, defaultName) : false;
            }

            bool RenamedNodesMap::RegisterNode(const std::shared_ptr<const SDKNode::NodeWrapper>& node, const Containers::SceneGraph& graph,
                Containers::SceneGraph::NodeIndex parentNode, const char* defaultName)
            {
                return node ? RegisterNode(*node, graph, parentNode, defaultName) : false;
            }

            bool RenamedNodesMap::RegisterNode(const SDKNode::NodeWrapper& node, const Containers::SceneGraph& graph,
                Containers::SceneGraph::NodeIndex parentNode, const char* defaultName)
            {
                AZStd::string name = node.GetName();
                if (SanitizeNodeName(name, graph, parentNode, defaultName))
                {
                    AZ_TraceContext("New node name", name);
                    
                    // Only register if the name is updated, otherwise the name in the source scene's node can be returned.
                    auto entry = m_idToName.find(node.GetUniqueId());
                    if (entry == m_idToName.end())
                    {
                        m_idToName.insert(AZStd::make_pair(node.GetUniqueId(), AZStd::move(name)));
                        return true;
                    }
                    else
                    {
                        AZ_TraceContext("Previous name", entry->second);
                        if (entry->second == name)
                        {
                            return true;
                        }
                        else
                        {
                            AZ_Assert(false, "Node has already been registered with a different name.");
                            return false;
                        }
                    }
                }
                else
                {
                    return true;
                }
            }
            
            const char* RenamedNodesMap::GetNodeName(const std::shared_ptr<SDKNode::NodeWrapper>& node) const
            {
                return node ? GetNodeName(*node) : "<invalid>";
            }

            const char* RenamedNodesMap::GetNodeName(const std::shared_ptr<const SDKNode::NodeWrapper>& node) const
            {
                return node ? GetNodeName(*node) : "<invalid>";
            }

            const char* RenamedNodesMap::GetNodeName(const AZStd::shared_ptr<SDKNode::NodeWrapper>& node) const
            {
                return node ? GetNodeName(*node) : "<invalid>";
            }

            const char* RenamedNodesMap::GetNodeName(const AZStd::shared_ptr<const SDKNode::NodeWrapper>& node) const
            {
                return node ? GetNodeName(*node) : "<invalid>";
            }

            const char* RenamedNodesMap::GetNodeName(const SDKNode::NodeWrapper& node) const
            {
                auto entry = m_idToName.find(node.GetUniqueId());
                if (entry != m_idToName.end())
                {
                    return entry->second.c_str();
                }
                else
                {
                    return node.GetName();
                }
            }
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
