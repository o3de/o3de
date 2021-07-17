/* Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzToolsFramework/Prefab/Template/Template.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/vector.h>

namespace PrefabDependencyViewer
{
    namespace Utils
    {
        class Node;
        using NodeSet = AZStd::unordered_set<Node*>;
        using ChildrenMap = AZStd::unordered_map<Node*, NodeSet>;
        using TemplateId = AzToolsFramework::Prefab::TemplateId;

        struct MetaData
        {
        public:
            AZ_CLASS_ALLOCATOR(MetaData, AZ::SystemAllocator, 0);

            MetaData() = default;

            MetaData(TemplateId tid, const char* source)
            {
                SetTemplateId(tid);
                SetSource(source);
            }

            TemplateId GetTemplateId()
            {
                return m_tid;
            }

            const char* GetSource()
            {
                return m_source;
            }
        private:
            void SetTemplateId(const TemplateId tid)
            {
                m_tid = tid;
            }
            void SetSource(const char* source)
            {
                m_source = source;
            }

            TemplateId m_tid;
            const char* m_source;
        };

        class Node
        {
        public:
            AZ_CLASS_ALLOCATOR(Node, AZ::SystemAllocator, 0);

            Node(TemplateId tid, const char* source, Node* parent = nullptr)
            {
                SetMetaData(tid, source);
                SetParent(parent);
            }

            MetaData GetMetaData()
            {
                return m_metaData;
            }

            Node* GetParent()
            {
                return m_parent;
            }

            // In the future want to be able to edit the tree.
            void SetParent(Node* parent)
            {
                m_parent = parent;
            }

        private:
            void SetMetaData(TemplateId tid, const char* source)
            {
                m_metaData = MetaData(tid, source);
            }

            MetaData m_metaData;
            Node* m_parent;
        };

        class DirectedGraph
        {
        public:
            DirectedGraph()
            {
                m_nodes = NodeSet();
                m_children = ChildrenMap();
                m_root = nullptr;
            }

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
                    m_root = child;
                }
            }

            Node* GetRoot() const
            {
                return m_root;
            }

            NodeSet GetChildren(Node* parent) const
            {
                AZ_Assert(parent, "Nullptr can't have children");

                auto iter = m_children.find(parent);
                if (iter != m_children.end())
                {
                    return iter->second;
                }

                return NodeSet();
            }

            DirectedGraph(const DirectedGraph& rhs)
            {
                AZStd::stack<AZStd::pair<Node*, Node*>> stack;
                stack.push(AZStd::make_pair(rhs.m_root, nullptr));

                while (!stack.empty())
                {
                    AZStd::pair<Node*, Node*> pair = stack.top();
                    Node* rhsNode = pair.first;
                    Node* parent = pair.second;

                    stack.pop();

                    MetaData cpyMetaData = rhsNode->GetMetaData();
                    Node* copy = aznew Node(cpyMetaData.GetTemplateId(),
                                            cpyMetaData.GetSource(),
                                            parent);

                    AddNode(copy);
                    AddChild(parent, copy);

                    for (Node* child : rhs.GetChildren(rhsNode))
                    {
                        stack.push(AZStd::make_pair(child, copy));
                    }
                }
            }

            ~DirectedGraph()
            {
                auto delete_node = [](Node* node) { delete node; };
                AZStd::for_each(m_nodes.begin(), m_nodes.end(), delete_node);
            }

            AZStd::vector<int> countNodesAtEachLevel(int& widestLevelSize) const
            {
                /** Directed Graph can't have cycles because of the
                non-circular nature of the Prefab. */
                AZStd::vector<int> count;
                using pair = AZStd::pair<int, Node*>;
                AZStd::queue<pair> queue;
                queue.push(AZStd::make_pair(0, m_root));


                while (!queue.empty())
                {
                    pair p = queue.front();
                    int level = p.first;
                    Node* currNode = p.second;

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
                            queue.push(AZStd::make_pair(level + 1, node));
                    }
                }

                // Check if the last level was the widest.
                widestLevelSize = AZStd::max(widestLevelSize, count[count.size() - 1]);
                return count;
            }
        private:
            NodeSet m_nodes;
            ChildrenMap m_children;
            Node* m_root;
        };
    } // namespace Utils
} // namespace PrefabDependencyViewer
