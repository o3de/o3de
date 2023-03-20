/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzCore/Console/IConsole.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorSystem.h>

AZ_CVAR(
    bool,
    ed_DebugDPE,
    false,
    nullptr,
    AZ::ConsoleFunctorFlags::DontReplicate | AZ::ConsoleFunctorFlags::DontDuplicate,
    "If set, prints extra debug info from the DPE to the console. Instances of the DPE also spawn a DPE debug view window, which allows "
    "users inspect the hierarchy and the DOM in pseudo XML format");

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

    void PropertyEditorSystem::RegisterNodeAttribute(const NodeMetadata* node, const AttributeDefinitionInterface* attribute)
    {
        AddNameToCrcTable(attribute->GetName());
        AZ::Name parentNodeName;
        if (node != nullptr)
        {
            parentNodeName = node->m_name;
        }
        else
        {
            AZ_Error(
                "PropertyEditorSystem",
                false,
                "Failed to register attribute, no parent Node specified: %s",
                attribute->GetName().GetCStr());
            return;
        }

        // Prevent duplication of (parentNodeName, attribute) pairs in case the same attribute definition
        // was registered multiple times for the same node
        if (m_attributeMetadata[attribute->GetName()].contains(parentNodeName))
        {
            auto relatedAttributes = m_attributeMetadata[attribute->GetName()].equal_range(parentNodeName);
            for (auto iter = relatedAttributes.first; iter != relatedAttributes.second; ++iter)
            {
                if (iter->second == attribute)
                {
                    AZ_Warning(
                        "PropertyEditorSystem",
                        false,
                        "Detected attempt to re-register a registered attribute definition interface for attribute %s",
                        attribute->GetName().GetCStr());
                    return;
                }
            }
        }

        m_attributeMetadata[attribute->GetName()].insert({ parentNodeName, attribute });
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

    const AttributeDefinitionInterface* PropertyEditorSystem::FindNodeAttribute(AZ::Name name, const PropertyEditorMetadata* parent) const
    {
        if (auto attributeContainerIt = m_attributeMetadata.find(name); attributeContainerIt != m_attributeMetadata.end())
        {
            while (parent != nullptr)
            {
                if (auto attributeIt = attributeContainerIt->second.find(parent->m_name); attributeIt != attributeContainerIt->second.end())
                {
                    return attributeIt->second;
                }
                parent = parent->m_parent;
            }
        }
        return nullptr;
    }

    void PropertyEditorSystem::EnumerateRegisteredAttributes(
        AZ::Name name, const AZStd::function<void(const AttributeDefinitionInterface&)>& enumerateCallback) const
    {
        if (auto attributeContainerIt = m_attributeMetadata.find(name); attributeContainerIt != m_attributeMetadata.end())
        {
            for (auto attributeIt = attributeContainerIt->second.begin(); attributeIt != attributeContainerIt->second.end(); ++attributeIt)
            {
                enumerateCallback(*attributeIt->second);
            }
        }
    }

    bool PropertyEditorSystem::DPEDebugEnabled()
    {
        bool dpeDebugEnabled = false;
        if (auto* console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            console->GetCvarValue("ed_DebugDPE", dpeDebugEnabled);
        }
        return dpeDebugEnabled;
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

        if (DPEDebugEnabled())
        {
            AZ_Warning("DPE", false, "No name found for CRC, falling back to CRC value: %" PRIu32, static_cast<AZ::u32>(crc));
        }
        AZ::Name hashName(AZStd::fixed_string<16>::format("%" PRIu32, static_cast<AZ::u32>(crc)));
        m_crcToName[crc] = hashName;
        return hashName;
    }
} // namespace AZ::DocumentPropertyEditor
