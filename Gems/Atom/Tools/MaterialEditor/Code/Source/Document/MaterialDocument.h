/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/Document/MaterialDocumentRequestBus.h>

#include <AtomToolsFramework/DynamicProperty/DynamicProperty.h>

namespace MaterialEditor
{
    /**
     * MaterialDocument provides an API for modifying and saving material document properties.
     */
    class MaterialDocument
        : public MaterialDocumentRequestBus::Handler
        , private AZ::TickBus::Handler
        , private AZ::Data::AssetBus::MultiHandler
        , private AzToolsFramework::AssetSystemBus::Handler
    {
    public:
        AZ_RTTI(MaterialDocument, "{DBA269AE-892B-415C-8FA1-166B94B0E045}");
        AZ_CLASS_ALLOCATOR(MaterialDocument, AZ::SystemAllocator, 0);
        AZ_DISABLE_COPY(MaterialDocument);

        MaterialDocument();
        virtual ~MaterialDocument();

        const AZ::Uuid& GetId() const;

        ////////////////////////////////////////////////////////////////////////
        // MaterialDocumentRequestBus::Handler implementation
        AZStd::string_view GetAbsolutePath() const override;
        AZStd::string_view GetRelativePath() const override;
        AZ::Data::Asset<AZ::RPI::MaterialAsset> GetAsset() const override;
        AZ::Data::Instance<AZ::RPI::Material> GetInstance() const override;
        const AZ::RPI::MaterialSourceData* GetMaterialSourceData() const override;
        const AZ::RPI::MaterialTypeSourceData* GetMaterialTypeSourceData() const override;
        const AZStd::any& GetPropertyValue(const AZ::Name& propertyFullName) const override;
        const AtomToolsFramework::DynamicProperty& GetProperty(const AZ::Name& propertyFullName) const override;
        bool IsPropertyGroupVisible(const AZ::Name& propertyGroupFullName) const override;
        void SetPropertyValue(const AZ::Name& propertyFullName, const AZStd::any& value) override;
        bool Open(AZStd::string_view loadPath) override;
        bool Rebuild() override;
        bool Save() override;
        bool SaveAsCopy(AZStd::string_view savePath) override;
        bool SaveAsChild(AZStd::string_view savePath) override;
        bool Close() override;
        bool IsOpen() const override;
        bool IsModified() const override;
        bool IsSavable() const override;
        bool CanUndo() const override;
        bool CanRedo() const override;
        bool Undo() override;
        bool Redo() override;
        bool BeginEdit() override;
        bool EndEdit() override;
        ////////////////////////////////////////////////////////////////////////

    private:

        // Predicate for evaluating properties
        using PropertyFilterFunction = AZStd::function<bool(const AtomToolsFramework::DynamicProperty&)>;

        // Map of document's properties
        using PropertyMap = AZStd::unordered_map<AZ::Name, AtomToolsFramework::DynamicProperty>;

        // Map of raw property values for undo/redo comparison and storage
        using PropertyValueMap = AZStd::unordered_map<AZ::Name, AZStd::any>;
        
        // Map of document's property group visibility flags
        using PropertyGroupVisibilityMap = AZStd::unordered_map<AZ::Name, bool>;

        // Function to be bound for undo and redo
        using UndoRedoFunction = AZStd::function<void()>;

        // A pair of functions, where first is the undo operation and second is the redo operation
        using UndoRedoFunctionPair = AZStd::pair<UndoRedoFunction, UndoRedoFunction>;

        // Container for all of the active undo and redo functions and state
        using UndoRedoHistory = AZStd::vector<UndoRedoFunctionPair>;

        ////////////////////////////////////////////////////////////////////////
        // AZ::TickBus interface implementation
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        ////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::AssetSystemBus::Handler overrides...
        void SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid sourceUUID) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetBus::Router overrides...
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        //////////////////////////////////////////////////////////////////////////

        bool SavePropertiesToSourceData(AZ::RPI::MaterialSourceData& sourceData, PropertyFilterFunction propertyFilter) const;

        bool OpenInternal(AZStd::string_view loadPath);

        void Recompile();

        void Clear();

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

        // Unique id of this material document
        AZ::Uuid m_id = AZ::Uuid::CreateRandom();

        // Underlying material asset
        AZ::Data::Asset<AZ::RPI::MaterialAsset> m_materialAsset;

        // Material instance being edited
        AZ::Data::Instance<AZ::RPI::Material> m_materialInstance;

        // Relative path to the material source file
        AZStd::string m_relativePath;

        // Absolute path to the material source file
        AZStd::string m_absolutePath;

        // Asset used to open document
        AZ::Data::AssetId m_sourceAssetId;

        // Set of assets that can trigger a document reload
        AZStd::unordered_set<AZ::Data::AssetId> m_dependentAssetIds;

        // Track if document saved itself last to skip external modification notification
        bool m_saveTriggeredInternally = false;

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

        // Variables needed for tracking the undo and redo state of this document

        // State of property values prior to an edit, used for restoration during undo
        PropertyValueMap m_propertyValuesBeforeEdit;

        // Container of undo commands
        UndoRedoHistory m_undoHistory;

        // The current position in the undo redo history
        int m_undoHistoryIndex = 0;

        AZStd::any m_invalidValue;
        
        AtomToolsFramework::DynamicProperty m_invalidProperty;
    };
} // namespace MaterialEditor
