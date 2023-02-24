/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/PropertyEditorSystemInterface.h>

namespace AZ::DocumentPropertyEditor
{
    class PropertyEditorSystem : public PropertyEditorSystemInterface
    {
    public:
        AZ_RTTI(PropertyEditorSystem, "{5DD4E43F-B17C-463E-8D4C-5A1E22DD452D}", PropertyEditorSystemInterface);
        AZ_CLASS_ALLOCATOR(PropertyEditorSystem, AZ::OSAllocator);

        PropertyEditorSystem();
        ~PropertyEditorSystem();

        void RegisterNode(NodeMetadata metadata) override;
        void RegisterPropertyEditor(PropertyEditorMetadata metadata) override;
        void RegisterNodeAttribute(const NodeMetadata* node, const AttributeDefinitionInterface* attribute) override;

        const NodeMetadata* FindNode(AZ::Name name) const override;
        const PropertyEditorMetadata* FindPropertyEditor(AZ::Name name) const override;
        const AttributeDefinitionInterface* FindNodeAttribute(AZ::Name name, const PropertyEditorMetadata* parent) const override;
        void EnumerateRegisteredAttributes(AZ::Name name, const AZStd::function<void(const AttributeDefinitionInterface&)>& enumerateCallback) const override;
        AZ::Name LookupNameFromId(AZ::Crc32 crc) const override;

        /*! returns whether the ed_DebugDPE CVar indicates that the DPE should print additional info / error messages to
         *  the console and spawn a DPEDebugWindow per DPE instance */
        static bool DPEDebugEnabled();

    private:
        void AddNameToCrcTable(AZ::Name name);

        mutable AZStd::unordered_map<AZ::Crc32, AZ::Name> m_crcToName;
        AZStd::unordered_map<AZ::Name, NodeMetadata> m_nodeMetadata;
        AZStd::unordered_map<AZ::Name, AZStd::unordered_multimap<AZ::Name, const AttributeDefinitionInterface*>> m_attributeMetadata;
    };
} // namespace AZ::DocumentPropertyEditor
