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

            MetaData(TemplateId tid, const char* source)
            {
                SetTemplateId(tid);
                SetSource(source);
            }

            TemplateId GetTemplateId()
            {
                return m_tid;
            }

            AZStd::string GetSource()
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

            Node(TemplateId tid, const char* source)
            {
                SetMetaData(tid, source);
            }

            MetaData* GetMetaDataPtr()
            {
                return m_metaDataPtr;
            }

            ~Node()
            {
                delete GetMetaDataPtr();
            }
        private:
            void SetMetaData(TemplateId tid, const char* source)
            {
                m_metaDataPtr = aznew MetaData(tid, source);
            }

            MetaData* m_metaDataPtr;
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
                }
                else
                {
                    m_root = child;
                }
            }

            ~DirectedGraph()
            {
                auto delete_node = [](Node* node) { delete node; };
                AZStd::for_each(m_nodes.begin(), m_nodes.end(), delete_node);
            }

        private:
            NodeSet m_nodes;
            ChildrenMap m_children;
            Node* m_root;
        };
    } // namespace Utils
} // namespace PrefabDependencyViewer
