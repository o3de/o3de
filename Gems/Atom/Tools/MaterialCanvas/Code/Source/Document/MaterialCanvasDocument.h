/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AtomToolsFramework/Document/AtomToolsDocument.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>
#include <Document/MaterialCanvasDocumentRequestBus.h>

namespace MaterialCanvas
{
    //! MaterialCanvasDocument
    class MaterialCanvasDocument
        : public AtomToolsFramework::AtomToolsDocument
        , public MaterialCanvasDocumentRequestBus::Handler
    {
    public:
        AZ_RTTI(MaterialCanvasDocument, "{DBA269AE-892B-415C-8FA1-166B94B0E045}");
        AZ_CLASS_ALLOCATOR(MaterialCanvasDocument, AZ::SystemAllocator, 0);
        AZ_DISABLE_COPY(MaterialCanvasDocument);

        MaterialCanvasDocument();
        virtual ~MaterialCanvasDocument();

        // AtomToolsFramework::AtomToolsDocument overrides...
        const AZStd::any& GetPropertyValue(const AZ::Name& propertyId) const override;
        const AtomToolsFramework::DynamicProperty& GetProperty(const AZ::Name& propertyId) const override;
        bool IsPropertyGroupVisible(const AZ::Name& propertyGroupFullName) const override;
        void SetPropertyValue(const AZ::Name& propertyId, const AZStd::any& value) override;
        bool IsModified() const override;
        bool IsSavable() const override;
        bool BeginEdit() override;
        bool EndEdit() override;

    private:
        // Predicate for evaluating properties
        using PropertyFilterFunction = AZStd::function<bool(const AtomToolsFramework::DynamicProperty&)>;

        // Map of document's properties
        using PropertyMap = AZStd::unordered_map<AZ::Name, AtomToolsFramework::DynamicProperty>;

        // Map of raw property values for undo/redo comparison and storage
        using PropertyValueMap = AZStd::unordered_map<AZ::Name, AZStd::any>;

        // Map of document's property group visibility flags
        using PropertyGroupVisibilityMap = AZStd::unordered_map<AZ::Name, bool>;

        // AtomToolsFramework::AtomToolsDocument overrides...
        void Clear() override;

        void RestorePropertyValues(const PropertyValueMap& propertyValues);

        // If material instance value(s) were modified, do we need to recompile on next tick?
        bool m_compilePending = false;

        // Collection of all material's properties
        PropertyMap m_properties;

        // Collection of all material's property groups
        PropertyGroupVisibilityMap m_propertyGroupVisibility;

        // State of property values prior to an edit, used for restoration during undo
        PropertyValueMap m_propertyValuesBeforeEdit;
    };
} // namespace MaterialCanvas
