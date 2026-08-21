/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>

#include <AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h>
#include <AtomToolsFramework/Inspector/InspectorWidget.h>

#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>

#include <Serializer/ParticleSourceData.h>

namespace OpenParticleSystemEditor
{
    //! Shows the full property layout of the material assigned to one emitter and lets the user override
    //! individual values for that emitter only.
    //!
    //! This is the particle editor equivalent of the mesh Material Component's property inspector. Because
    //! every emitter builds its own AZ::RPI::Material instance at runtime, two emitters can point at the same
    //! .material asset and still carry different values; this widget is what authors those differences.
    //!
    //! Edited values are written into DetailInfo::m_materialOverrides. The owning inspector is responsible
    //! for syncing that map back onto the emitter and notifying the document so the viewport updates.
    class MaterialPropertyWidget
        : public AtomToolsFramework::InspectorWidget
        , public AzToolsFramework::IPropertyEditorNotify
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(MaterialPropertyWidget, AZ::SystemAllocator, 0);

        explicit MaterialPropertyWidget(QWidget* parent = nullptr);
        ~MaterialPropertyWidget() override;

        //! Points the widget at an emitter. Pass nullptr to clear it.
        //! Rebuilds the whole property layout if the assigned material asset changed since the last call.
        void SetDetail(OpenParticle::ParticleSourceData::DetailInfo* detail);

        // AtomToolsFramework::InspectorRequestBus::Handler overrides...
        void Reset() override;

    protected:
        // AtomToolsFramework::InspectorWidget overrides...
        //! Every group starts collapsed. A full material layout has more groups than fit on screen, and the
        //! base class default of "expanded unless previously collapsed" means opening this on a PBR material
        //! dumps hundreds of rows at once.
        bool ShouldGroupAutoExpanded(const AZStd::string& groupName) const override;
        void OnGroupExpanded(const AZStd::string& groupName) override;
        void OnGroupCollapsed(const AZStd::string& groupName) override;

    Q_SIGNALS:
        //! Raised whenever an override value changes. `editingFinished` is false while a slider or colour
        //! picker is still being dragged, so listeners can throttle expensive work such as saving.
        void OnMaterialPropertyChanged(bool editingFinished);

    private:
        //! Re-reads values out of the detail's override map without rebuilding the layout.
        void RefreshValues();

        //! True once a material and its material type source data have been loaded successfully.
        bool IsLoaded() const;

        // AzToolsFramework::IPropertyEditorNotify overrides...
        void BeforePropertyModified(AzToolsFramework::InstanceDataNode* node) override;
        void AfterPropertyModified(AzToolsFramework::InstanceDataNode* node) override;
        void SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* node) override;
        void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* node) override;
        void SealUndoStack() override;

        //! Loads the material asset and the material type source data that describes its property layout.
        //! The source data is what carries display names, groups, ranges and enum labels; the runtime
        //! layout alone would only give us ids and raw types.
        bool LoadMaterial();
        void UnloadMaterial();

        //! Builds one DynamicPropertyGroup per material property group and adds it to the inspector.
        void Populate();

        //! Copies the current override map (falling back to the material asset's own values) into the
        //! dynamic properties so the UI reflects what will actually render.
        void LoadOverridesFromDetail();

        //! Writes one edited property back into DetailInfo::m_materialOverrides. Values that match the
        //! assigned material are removed rather than stored, so the map only ever holds real differences.
        void SaveOverrideToDetail(const AtomToolsFramework::DynamicProperty& property);

        AZ::Crc32 GetGroupSaveStateKey(const AZStd::string& groupName) const;
        bool IsInstanceNodePropertyModified(const AzToolsFramework::InstanceDataNode* node) const;
        const char* GetInstanceNodePropertyIndicator(const AzToolsFramework::InstanceDataNode* node) const;

        OpenParticle::ParticleSourceData::DetailInfo* m_detail = nullptr;

        AZ::Data::AssetId m_materialAssetId;
        AZ::Data::Asset<AZ::RPI::MaterialAsset> m_materialAsset;
        AZ::Data::Asset<AZ::RPI::MaterialTypeAsset> m_materialTypeAsset;
        AZ::RPI::MaterialTypeSourceData m_materialTypeSourceData;

        //! Ordered so the groups always appear in the same sequence between rebuilds.
        AZStd::map<AZStd::string, AtomToolsFramework::DynamicPropertyGroup> m_groups;

        //! Guards against re-entrancy while LoadOverridesFromDetail is pushing values into the editors.
        bool m_updatingUi = false;

        //! Groups the user has opened, so expansion survives switching emitters or reopening the dialog.
        //! Deliberately per session rather than persisted: which groups matter changes with the material.
        static AZStd::unordered_set<AZ::u32> s_expandedGroups;
    };
} // namespace OpenParticleSystemEditor
