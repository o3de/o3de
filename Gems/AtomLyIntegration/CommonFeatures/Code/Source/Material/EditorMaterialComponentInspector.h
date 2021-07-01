/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h>
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/function/function_base.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#include <Material/EditorMaterialComponentUtil.h>
#endif

namespace AZ
{
    namespace Render
    {
        namespace EditorMaterialComponentInspector
        {
            using PropertyChangedCallback = AZStd::function<void(const MaterialPropertyOverrideMap&)>;

            class MaterialPropertyInspector
                : public AtomToolsFramework::InspectorWidget
                , public AzToolsFramework::IPropertyEditorNotify
            {
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR(MaterialPropertyInspector, AZ::SystemAllocator, 0);

                explicit MaterialPropertyInspector(
                    const AZStd::string& slotName, const AZ::Data::AssetId& assetId, PropertyChangedCallback propertyChangedCallback,
                    QWidget* parent = nullptr);
                ~MaterialPropertyInspector() override;

                bool LoadMaterial();

                // AtomToolsFramework::InspectorRequestBus::Handler overrides...
                void Reset() override;

                void Populate();

                void SetOverrides(const MaterialPropertyOverrideMap& propertyOverrideMap);

                bool SaveMaterial() const;
                bool SaveMaterialToSource() const;
                bool HasMaterialSource() const;
                bool HasMaterialParentSource() const;
                void OpenMaterialSourceInEditor() const;
                void OpenMaterialParentSourceInEditor() const;
                const EditorMaterialComponentUtil::MaterialEditData& GetEditData() const;

            private:

                // AzToolsFramework::IPropertyEditorNotify overrides...
                void BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
                void AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
                void SetPropertyEditingActive([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode) override {}
                void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode) override;
                void SealUndoStack() override {}
                void RequestPropertyContextMenu([[maybe_unused]] AzToolsFramework::InstanceDataNode*, const QPoint&) override {}
                void PropertySelectionChanged([[maybe_unused]] AzToolsFramework::InstanceDataNode*, bool) override {}

                void AddDetailsGroup();
                void AddUvNamesGroup();
                void RunPropertyChangedCallback();
                void RunEditorMaterialFunctors();
                void UpdateMaterialInstanceProperty(const AtomToolsFramework::DynamicProperty& property);

                // Tracking the property that is actively being edited in the inspector
                const AtomToolsFramework::DynamicProperty* m_activeProperty = {};

                AZStd::string m_slotName;
                AZ::Data::AssetId m_materialAssetId = {};
                EditorMaterialComponentUtil::MaterialEditData m_editData;
                PropertyChangedCallback m_propertyChangedCallback = {};
                AZ::Data::Instance<AZ::RPI::Material> m_materialInstance = {};
                AZStd::vector<AZ::RPI::Ptr<AZ::RPI::MaterialFunctor>> m_editorFunctors = {};
                AZ::RPI::MaterialPropertyFlags m_dirtyPropertyFlags = {};
                AZStd::unordered_map<AZStd::string, AtomToolsFramework::DynamicPropertyGroup> m_groups = {};
            };

            bool OpenInspectorDialog(
                const AZStd::string& slotName, const AZ::Data::AssetId& assetId, MaterialPropertyOverrideMap propertyOverrideMap,
                PropertyChangedCallback propertyChangedCallback);
        } // namespace EditorMaterialComponentInspector
    } // namespace Render
} // namespace AZ
