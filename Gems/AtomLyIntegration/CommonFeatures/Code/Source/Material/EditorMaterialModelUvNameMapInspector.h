/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialAssignment.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

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
            using MaterialModelUvOverrideMapChangedCallBack = AZStd::function<void(const RPI::MaterialModelUvOverrideMap&)>;

            class MaterialModelUvNameMapInspector
                : public AtomToolsFramework::InspectorWidget
                , public AzToolsFramework::IPropertyEditorNotify
            {
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR(MaterialModelUvNameMapInspector, AZ::SystemAllocator);

                explicit MaterialModelUvNameMapInspector(
                    const AZ::Data::AssetId& assetId,
                    const RPI::MaterialModelUvOverrideMap& matModUvOverrides,
                    const AZStd::unordered_set<AZ::Name>& modelUvNames,
                    MaterialModelUvOverrideMapChangedCallBack matModUvOverrideMapChangedCallBack,
                    QWidget* parent = nullptr
                );
                ~MaterialModelUvNameMapInspector() override;

                //! AtomToolsFramework::InspectorRequestBus::Handler overrides...
                void Reset() override;

                void Populate();

                void SetUvNameMap(const RPI::MaterialModelUvOverrideMap& matModUvPairs);

            private:

                //! AzToolsFramework::IPropertyEditorNotify overrides...
                void BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
                void AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
                void SetPropertyEditingActive([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode) override {}
                void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode) override;
                void SealUndoStack() override {}
                void RequestPropertyContextMenu([[maybe_unused]] AzToolsFramework::InstanceDataNode*, const QPoint&) override {}
                void PropertySelectionChanged([[maybe_unused]] AzToolsFramework::InstanceDataNode*, bool) override {}

                void ResetModelUvNameIndices();
                void SetModelUvNames(const AZStd::unordered_set<AZ::Name>& modelUvNames);

                RPI::MaterialModelUvOverrideMap m_matModUvOverrides;
                AZStd::vector<uint32_t> m_modelUvNameIndices;  //! Used to index enum values and translate to UV pairs.
                RPI::MaterialUvNameMap m_materialUvNames;
                AZStd::vector<AZStd::string> m_modelUvNames; //! Used as enum values.
                MaterialModelUvOverrideMapChangedCallBack m_matModUvOverrideMapChangedCallBack;

                AtomToolsFramework::DynamicPropertyGroup m_group;
                const AtomToolsFramework::DynamicProperty* m_activeProperty = nullptr;
            };

            bool OpenInspectorDialog(
                const AZ::Data::AssetId& assetId,
                const RPI::MaterialModelUvOverrideMap& matModUvOverrides,
                const AZStd::unordered_set<AZ::Name>& modelUvNames,
                MaterialModelUvOverrideMapChangedCallBack matModUvOverrideMapChangedCallBack);
        } // namespace EditorMaterialComponentInspector
    } // namespace Render
} // namespace AZ
