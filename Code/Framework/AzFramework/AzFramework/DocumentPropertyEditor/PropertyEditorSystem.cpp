/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorSystem.h>

namespace AZ::DocumentPropertyEditor
{
    PropertyEditorSystem::PropertyEditorSystem()
    {
        Nodes::Reflect(this);

        AZ::Interface<PropertyEditorSystemInterface>::Register(this);
    }

    PropertyEditorSystem::~PropertyEditorSystem()
    {
        AZ::Interface<PropertyEditorSystemInterface>::Unregister(this);
    }

    void PropertyEditorSystem::RegisterNode(NodeMetadata metadata)
    {
        AddNameToCrcTable(metadata.m_name);
        m_nodeMetadata[metadata.m_name] = AZStd::move(metadata);
    }

    void PropertyEditorSystem::RegisterPropertyEditor(PropertyEditorMetadata metadata)
    {
        AZ_Assert(
            metadata.InheritsFrom<Nodes::PropertyEditor>(),
            "DPE RegisterPropertyEditor: Attempted to register a node (\"%s\") that is not derived from PropertyEditor as a property "
            "editor",
            metadata.m_name.GetCStr());
        RegisterNode(metadata);
    }

    void PropertyEditorSystem::RegisterAttribute(AttributeMetadata metadata)
    {
        AddNameToCrcTable(metadata.m_name);
        AZ::Name parentNodeName;
        if (metadata.m_node != nullptr)
        {
            parentNodeName = metadata.m_node->m_name;
        }
        else
        {
            AZ_Error(
                "PropertyEditorSystem", false, "Failed to register attribute, no parent Node specified: %s", metadata.m_name.GetCStr());
            return;
        }
        m_attributeMetadata[metadata.m_name][parentNodeName] = AZStd::move(metadata);
    }

    const NodeMetadata* PropertyEditorSystem::FindNode(AZ::Name name) const
    {
        if (auto nodeIt = m_nodeMetadata.find(name); nodeIt != m_nodeMetadata.end())
        {
            return &nodeIt->second;
        }
        return nullptr;
    }

    const PropertyEditorMetadata* PropertyEditorSystem::FindPropertyEditor(AZ::Name name) const
    {
        const PropertyEditorMetadata* result = FindNode(name);
        return result->InheritsFrom<Nodes::PropertyEditor>() ? result : nullptr;
    }

    const AttributeMetadata* PropertyEditorSystem::FindAttribute(AZ::Name name, const PropertyEditorMetadata* parent) const
    {
        if (auto attributeContainerIt = m_attributeMetadata.find(name); attributeContainerIt != m_attributeMetadata.end())
        {
            while (parent != nullptr)
            {
                if (auto attributeIt = attributeContainerIt->second.find(parent->m_name); attributeIt != attributeContainerIt->second.end())
                {
                    return &attributeIt->second;
                }
                parent = parent->m_parent;
            }
        }
        return nullptr;
    }

    void PropertyEditorSystem::AddNameToCrcTable(AZ::Name name)
    {
        m_crcToName[AZ::Crc32(name.GetStringView())] = AZStd::move(name);
    }

    AZ::Name PropertyEditorSystem::LookupNameFromId(AZ::Crc32 crc) const
    {
        auto crcIt = m_crcToName.find(crc);
        if (crcIt != m_crcToName.end())
        {
            return crcIt->second;
        }
        AZ_Assert(false, "No name found for CRC: " PRIu32, static_cast<AZ::u32>(crc));
        return {};
    }
} // namespace AZ::DocumentPropertyEditor
