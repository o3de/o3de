/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

#include <MetaData.h>

namespace PrefabDependencyViewer::Utils
{
    using TemplateId = AzToolsFramework::Prefab::TemplateId;

    /**
     * Node is a building block for the DirectedGraph. It stores
     * generic MetaData about Prefab or an Asset. It also keeps
     * track of it's parent which helps with adding connections
     * in the GraphCanvas UI.
     */
    class Node
    {
    public:
        AZ_CLASS_ALLOCATOR(Node, AZ::SystemAllocator, 0);

        Node(TemplateId tid, AZStd::string source, Node* parent = nullptr)
            : m_metaData(tid, source)
            , m_parent(parent)
        {
        }

        MetaData& GetMetaData()
        {
            return m_metaData;
        }

        Node* GetParent()
        {
            return m_parent;
        }

        void SetParent(Node* parent)
        {
            m_parent = parent;
        }

    private:
        MetaData m_metaData;
        Node* m_parent;
    };
} // namespace PrefabDependencyViewer::Utils
