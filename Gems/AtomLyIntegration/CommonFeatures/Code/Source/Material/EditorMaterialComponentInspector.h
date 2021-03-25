/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Asset/AssetCommon.h>
#include <Atom/Feature/Material/MaterialAssignment.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>

#include <AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h>
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
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

                explicit MaterialPropertyInspector(const AZ::Data::AssetId& assetId, PropertyChangedCallback propertyChangedCallback, QWidget* parent = nullptr);
                ~MaterialPropertyInspector() override;

                bool LoadMaterial();

                // AtomToolsFramework::InspectorRequestBus::Handler overrides...
                void Reset() override;

                void Populate();

                void SetOverrides(const MaterialPropertyOverrideMap& propertyOverrideMap);

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

                AZ::Data::AssetId m_materialAssetId = {};
                MaterialPropertyOverrideMap m_materialPropertyOverrideMap = {};
                PropertyChangedCallback m_propertyChangedCallback = {};
                AZ::Data::Asset<AZ::RPI::MaterialAsset> m_materialAsset = {};
                AZ::Data::Asset<AZ::RPI::MaterialTypeAsset> m_materialTypeAsset = {};
                AZ::Data::Asset<AZ::RPI::MaterialAsset> m_parentMaterialAsset = {};
                AZ::Data::Instance<AZ::RPI::Material> m_materialInstance = {};
                AZ::RPI::MaterialTypeSourceData m_materialTypeSourceData;
                AZStd::vector<AZ::RPI::Ptr<AZ::RPI::MaterialFunctor>> m_editorFunctors = {};
                AZ::RPI::MaterialPropertyFlags m_dirtyPropertyFlags = {};
                AZStd::unordered_map<AZStd::string, AtomToolsFramework::DynamicPropertyGroup> m_groups = {};
            };

            bool OpenInspectorDialog(const AZ::Data::AssetId& assetId, MaterialPropertyOverrideMap propertyOverrideMap, PropertyChangedCallback propertyChangedCallback);
        } // namespace EditorMaterialComponentInspector
    } // namespace Render
} // namespace AZ
