/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/RTTI/RTTI.h>

#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AtomToolsFramework/Document/AtomToolsDocument.h>
#include <Document/MaterialDocumentRequestBus.h>

namespace MaterialEditor
{
    /**
     * MaterialDocument provides an API for modifying and saving material document properties.
     */
    class MaterialDocument
        : public AtomToolsFramework::AtomToolsDocument
        , public MaterialDocumentRequestBus::Handler
        , private AZ::TickBus::Handler
    {
    public:
        AZ_RTTI(MaterialDocument, "{DBA269AE-892B-415C-8FA1-166B94B0E045}");
        AZ_CLASS_ALLOCATOR(MaterialDocument, AZ::SystemAllocator, 0);
        AZ_DISABLE_COPY(MaterialDocument);

        MaterialDocument();
        virtual ~MaterialDocument();

        // AtomToolsFramework::AtomToolsDocument overrides...
        const AZStd::any& GetPropertyValue(const AZ::Name& propertyId) const override;
        const AtomToolsFramework::DynamicProperty& GetProperty(const AZ::Name& propertyId) const override;
        bool IsPropertyGroupVisible(const AZ::Name& propertyGroupFullName) const override;
        void SetPropertyValue(const AZ::Name& propertyId, const AZStd::any& value) override;
        bool Open(AZStd::string_view loadPath) override;
        bool Save() override;
        bool SaveAsCopy(AZStd::string_view savePath) override;
        bool SaveAsChild(AZStd::string_view savePath) override;
        bool IsOpen() const override;
        bool IsModified() const override;
        bool IsSavable() const override;
        bool BeginEdit() override;
        bool EndEdit() override;

        // MaterialDocumentRequestBus::Handler overrides...
        AZ::Data::Asset<AZ::RPI::MaterialAsset> GetAsset() const override;
        AZ::Data::Instance<AZ::RPI::Material> GetInstance() const override;
        const AZ::RPI::MaterialSourceData* GetMaterialSourceData() const override;
        const AZ::RPI::MaterialTypeSourceData* GetMaterialTypeSourceData() const override;

    private:

        // Predicate for evaluating properties
        using PropertyFilterFunction = AZStd::function<bool(const AtomToolsFramework::DynamicProperty&)>;

        // Map of document's properties
        using PropertyMap = AZStd::unordered_map<AZ::Name, AtomToolsFramework::DynamicProperty>;

        // Map of raw property values for undo/redo comparison and storage
        using PropertyValueMap = AZStd::unordered_map<AZ::Name, AZStd::any>;
        
        // Map of document's property group visibility flags
        using PropertyGroupVisibilityMap = AZStd::unordered_map<AZ::Name, bool>;

        // AZ::TickBus overrides...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        bool SaveSourceData(AZ::RPI::MaterialSourceData& sourceData, PropertyFilterFunction propertyFilter) const;

        // AtomToolsFramework::AtomToolsDocument overrides...
        void Clear() override;

        bool ReopenRecordState() override;
        bool ReopenRestoreState() override;

        void Recompile();

        void RestorePropertyValues(const PropertyValueMap& propertyValues);

        struct EditorMaterialFunctorResult
        {
            AZStd::unordered_set<AZ::Name> m_updatedProperties;
            AZStd::unordered_set<AZ::Name> m_updatedPropertyGroups;
        };

        // Run editor material functor to update editor metadata.
        // @param dirtyFlags indicates which properties have changed, and thus which MaterialFunctors need to be run.
        // @return names for the set of properties and groups that have been changed or need update.
        EditorMaterialFunctorResult RunEditorMaterialFunctors(AZ::RPI::MaterialPropertyFlags dirtyFlags);

        // Underlying material asset
        AZ::Data::Asset<AZ::RPI::MaterialAsset> m_materialAsset;

        // Material instance being edited
        AZ::Data::Instance<AZ::RPI::Material> m_materialInstance;

        // If material instance value(s) were modified, do we need to recompile on next tick?
        bool m_compilePending = false;

        // Collection of all material's properties
        PropertyMap m_properties;
        
        // Collection of all material's property groups
        PropertyGroupVisibilityMap m_propertyGroupVisibility;

        // Material functors that run in editor. See MaterialFunctor.h for details.
        AZStd::vector<AZ::RPI::Ptr<AZ::RPI::MaterialFunctor>> m_editorFunctors;

        // Source data for material type
        AZ::RPI::MaterialTypeSourceData m_materialTypeSourceData;

        // Source data for material
        AZ::RPI::MaterialSourceData m_materialSourceData;

        // State of property values prior to an edit, used for restoration during undo
        PropertyValueMap m_propertyValuesBeforeEdit;

        // State of property values prior to reopen
        PropertyValueMap m_propertyValuesBeforeReopen;
    };
} // namespace MaterialEditor
