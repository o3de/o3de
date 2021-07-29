/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>

#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Template/Template.h>

#include <MetaData.h>
#include <Node.h>

namespace PrefabDependencyViewer::Utils
{
    class Node;
    using NodeSet = AZStd::unordered_set<Node*>;
    using ChildrenMap = AZStd::unordered_map<Node*, NodeSet>;
    using TemplateId = AzToolsFramework::Prefab::TemplateId;

    class DirectedGraph
    {
    public:
        DirectedGraph() = default;

        void AddNode(Node* node)
        {
            m_nodes.insert(node);
        }

        void AddChild(Node* parent, Node* child)
        {
            if (parent)
            {
                m_children[parent].insert(child);
                child->SetParent(parent);
            }
            else
            {
                if (m_root)
                {
                    AZ_Assert(false, "Prefab Dependency Viewer - Memory leak in the graph because the root was already set.");
                }

                m_root = child;
            }
        }

        Node* GetRoot() const
        {
            return m_root;
        }

        AZStd::optional<NodeSet> GetChildren(Node* parent) const
        {
            AZ_Assert(parent, "Nullptr can't have children");

            const auto iter = m_children.find(parent);
            if (iter != m_children.end())
            {
                return iter->second;
            }

            return AZStd::nullopt;
        }

        DirectedGraph(const DirectedGraph& rhs)
        {
            AZStd::stack<AZStd::pair<Node*, Node*>> stack;
            stack.push(AZStd::make_pair(rhs.m_root, nullptr));

            while (!stack.empty())
            {
                auto [rhsNode, parent] = stack.top();
                stack.pop();

                MetaData cpyMetaData = rhsNode->GetMetaData();
                Node* copy = aznew Node(cpyMetaData.GetTemplateId(),
                                        cpyMetaData.GetSource(),
                                        parent);

                AddNode(copy);
                AddChild(parent, copy);

                AZStd::optional<NodeSet> children = rhs.GetChildren(rhsNode);
                if (AZStd::nullopt != children && children.has_value())
                {
                    for (Node* child : children.value())
                    {
                        stack.push(AZStd::make_pair(child, copy));
                    }
                }
            }
        }

        ~DirectedGraph()
        {
            auto delete_node = [](Node* node) { delete node; };
            AZStd::for_each(m_nodes.begin(), m_nodes.end(), delete_node);
        }

        AZStd::tuple<AZStd::vector<int>, int> countNodesAtEachLevel() const
        {
            /** Directed Graph can't have cycles because of the
            non-circular nature of the Prefab. */
            int widestLevelSize = 0;
            AZStd::vector<int> count;

            using pair = AZStd::pair<int, Node*>;
            AZStd::queue<pair> queue;
            queue.emplace(0, m_root);

            while (!queue.empty())
            {
                auto [level, currNode] = queue.front();
                queue.pop();

                if (count.size() <= level)
                {
                    // Started a new level so check if the previous level
                    // was any bigger then the widest level size seen so far.
                    int prevLevelCount = level != 0 ? count[level - 1] : 0;
                    widestLevelSize = AZStd::max(widestLevelSize, prevLevelCount);
                    count.push_back(1);
                }
                else
                {
                    ++count[level];
                }

                auto it = m_children.find(currNode);
                if (it != m_children.end())
                {
                    NodeSet children = it->second;
                    for (Node* node : children)
                    {
                        queue.emplace(level + 1, node);
                    }
                }
            }

            // Check if the last level was the widest.
            widestLevelSize = AZStd::max(widestLevelSize, count[count.size() - 1]);
            return AZStd::make_tuple(count, widestLevelSize);
        }
    private:
        NodeSet m_nodes;
        ChildrenMap m_children;
        Node* m_root = nullptr;
    };
}
