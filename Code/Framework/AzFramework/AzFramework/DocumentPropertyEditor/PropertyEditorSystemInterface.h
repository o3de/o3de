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
    //! System interface for the Document Property Editor.
    //! The Property Editor System provides mechanisms for registering
    //! adapter elemenets (nodes / property editors and their attributes)
    //! with schema information for validation.
    //! This system also provides a CRC -> Name mapping for interoperation
    //! with the legacy reflection system.
    class PropertyEditorSystemInterface
    {
    public:
        AZ_RTTI(PropertyEditorSystemInterface, "{4F30FC7E-28F3-4D75-9F8E-D818FDBBE984}");

        virtual ~PropertyEditorSystemInterface() = default;

        //! Registers a Node, an adapter element that maps to a UI representation.
        virtual void RegisterNode(NodeMetadata metadata) = 0;
        //! Registers a PropertyEditor definition defined as the Type of a PropertyEditor node.
        //! Attributes may be associated with specific PropertyEditors.
        virtual void RegisterPropertyEditor(PropertyEditorMetadata metadata) = 0;
        //! Registers an attribute definition that can registered to a Node or PropertyEditor.
        //! Attributes have a name and a type that can be used to validate and extract the contens of a given node.
        virtual void RegisterAttribute(AttributeMetadata metadata) = 0;

        //! Look up the metadata associated with a Node. Returns null if a node definition isn't found.
        virtual const NodeMetadata* FindNode(AZ::Name name) const = 0;
        //! Looks up the metadata associated with a PropertyEditor. Equivalent to FindNode, but this will validate that
        //! the node is a property editor.
        virtual const PropertyEditorMetadata* FindPropertyEditor(AZ::Name name) const = 0;
        //! Looks up attribute metadata registered to a given property editor (or one of its parents)
        //! based on an attribute name. Returns null if no attribute is found.
        virtual const AttributeMetadata* FindAttribute(AZ::Name name, const PropertyEditorMetadata* parent = nullptr) const = 0;
        //! For a given CRC, looks up an equivalent Name.
        //! The pool of valid CRCs is registered from registered Node, PropertyEditor, and Attribute names.
        virtual AZ::Name LookupNameFromId(AZ::Crc32 crc) const = 0;

        //! Register a node from a given NodeDefinition struct.
        template<typename NodeDefinition>
        void RegisterNode()
        {
            RegisterNode(NodeMetadata::FromType<NodeDefinition>());
        }

        //! Register a node from a given NodeDefinition struct and its parent NodeDefinition.
        template<typename NodeDefinition, typename ParentNodeDefinition>
        void RegisterNode()
        {
            const AZ::Name parentName = GetNodeName<ParentNodeDefinition>();
            const NodeMetadata* parent = FindNode(parentName);
            AZ_Assert(parent != nullptr, "DPE RegisterNode: No node definiton found for parent \"%s\"", parentName.GetCStr());
            RegisterNode(NodeMetadata::FromType<NodeDefinition>(parent));
        }

        //! Register a property editor given a PropertyEditorDefinition.
        template<typename DerivedPropertyEditor>
        void RegisterPropertyEditor(const PropertyEditorMetadata* parent = nullptr)
        {
            if (parent == nullptr)
            {
                parent = FindPropertyEditor(GetNodeName<Nodes::PropertyEditor>());
            }
            RegisterPropertyEditor(PropertyEditorMetadata::FromType<DerivedPropertyEditor>(parent));
        }

        //! Register a property editor given a PropertyEditorDefinition and its parent PropertyEditorDefinition.
        template<typename PropertyEditor, typename ParentPropertyEditor>
        void RegisterPropertyEditor()
        {
            const AZ::Name parentName = GetNodeName<ParentPropertyEditor>();
            const PropertyEditorMetadata* parent = FindPropertyEditor(parentName);
            AZ_Assert(parent != nullptr, "DPE RegisterNode: No property editor definiton found for parent \"%s\"", parentName.GetCStr());
            RegisterPropertyEditor(PropertyEditorMetadata::FromType<PropertyEditor>(parent));
        }

        //! Registers an attribute based on an AttributeDefinition to a given node or property editor.
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
