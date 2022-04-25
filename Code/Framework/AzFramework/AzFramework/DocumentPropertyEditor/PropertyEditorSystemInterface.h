/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/DocumentSchema.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>

namespace AZ::DocumentPropertyEditor
{
    class PropertyEditorSystemInterface
    {
    public:
        AZ_RTTI(PropertyEditorSystemInterface, "{4F30FC7E-28F3-4D75-9F8E-D818FDBBE984}");

        virtual ~PropertyEditorSystemInterface() = default;

        virtual void RegisterNode(NodeMetadata metadata) = 0;
        virtual void RegisterPropertyEditor(PropertyEditorMetadata metadata) = 0;
        virtual void RegisterAttribute(AttributeMetadata metadata) = 0;

        virtual const NodeMetadata* FindNode(AZ::Name name) const = 0;
        virtual const PropertyEditorMetadata* FindPropertyEditor(AZ::Name name) const = 0;
        virtual const AttributeMetadata* FindAttribute(AZ::Name name, const PropertyEditorMetadata* parent = nullptr) const = 0;
        virtual AZ::Name LookupNameFromCrc(AZ::Crc32 crc) const = 0;

        template<typename NodeDefinition>
        void RegisterNode()
        {
            RegisterNode(NodeMetadata::FromType<NodeDefinition>());
        }

        template<typename NodeDefinition, typename ParentNodeDefinition>
        void RegisterNode()
        {
            const AZ::Name parentName = GetNodeName<ParentNodeDefinition>();
            const NodeMetadata* parent = FindNode(parentName);
            AZ_Assert(parent != nullptr, "DPE RegisterNode: No node definiton found for parent \"%s\"", parentName.GetCStr());
            RegisterNode(NodeMetadata::FromType<NodeDefinition>(parent));
        }

        template<typename DerivedPropertyEditor>
        void RegisterPropertyEditor(const PropertyEditorMetadata* parent = nullptr)
        {
            if (parent == nullptr)
            {
                parent = FindPropertyEditor(GetNodeName<Nodes::PropertyEditor>());
            }
            RegisterPropertyEditor(PropertyEditorMetadata::FromType<DerivedPropertyEditor>(parent));
        }

        template<typename PropertyEditor, typename ParentPropertyEditor>
        void RegisterPropertyEditor()
        {
            const AZ::Name parentName = GetNodeName<ParentPropertyEditor>();
            const PropertyEditorMetadata* parent = FindPropertyEditor(parentName);
            AZ_Assert(parent != nullptr, "DPE RegisterNode: No property editor definiton found for parent \"%s\"", parentName.GetCStr());
            RegisterPropertyEditor(PropertyEditorMetadata::FromType<PropertyEditor>(parent));
        }

        template<typename NodeType, typename AttributeDefinition>
        void RegisterAttribute(const AttributeDefinition& definition)
        {
            const AZ::Name nodeName = GetNodeName<NodeType>();
            const NodeMetadata* node = FindNode(nodeName);
            AZ_Assert(node != nullptr, "DPE RegisterAttribute: No node definition found for parent \"%s\"", nodeName.GetCStr());
            RegisterAttribute(AttributeMetadata::FromDefinition(definition, node));
        }
    };
} // namespace AZ::DocumentPropertyEditor
