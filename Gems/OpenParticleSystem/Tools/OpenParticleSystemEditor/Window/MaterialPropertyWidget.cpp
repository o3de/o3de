/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Window/MaterialPropertyWidget.h>

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>

#include <AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h>
#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>
#include <AtomToolsFramework/Util/Util.h>

#include <AzFramework/StringFunc/StringFunc.h>

namespace OpenParticleSystemEditor
{
    AZStd::unordered_set<AZ::u32> MaterialPropertyWidget::s_expandedGroups;

    MaterialPropertyWidget::MaterialPropertyWidget(QWidget* parent)
        : AtomToolsFramework::InspectorWidget(parent)
    {
        SetGroupSettingsPrefix("/O3DE/OpenParticleSystem/MaterialPropertyWidget");
    }

    MaterialPropertyWidget::~MaterialPropertyWidget()
    {
        UnloadMaterial();
    }

    bool MaterialPropertyWidget::IsLoaded() const
    {
        return m_materialAsset.IsReady() && m_materialTypeAsset.IsReady();
    }

    void MaterialPropertyWidget::Reset()
    {
        m_groups.clear();
        AtomToolsFramework::InspectorWidget::Reset();
    }

    bool MaterialPropertyWidget::ShouldGroupAutoExpanded(const AZStd::string& groupName) const
    {
        // Inverted from the base class: collapsed unless the user opened this group earlier in the session.
        return s_expandedGroups.find(AZ::Crc32(groupName)) != s_expandedGroups.end();
    }

    void MaterialPropertyWidget::OnGroupExpanded(const AZStd::string& groupName)
    {
        s_expandedGroups.insert(AZ::Crc32(groupName));
        AtomToolsFramework::InspectorWidget::OnGroupExpanded(groupName);
    }

    void MaterialPropertyWidget::OnGroupCollapsed(const AZStd::string& groupName)
    {
        s_expandedGroups.erase(AZ::Crc32(groupName));
        AtomToolsFramework::InspectorWidget::OnGroupCollapsed(groupName);
    }

    void MaterialPropertyWidget::SetDetail(OpenParticle::ParticleSourceData::DetailInfo* detail)
    {
        const AZ::Data::AssetId newAssetId = detail ? detail->m_material.GetId() : AZ::Data::AssetId();

        // Same emitter and same material - the layout is still valid, so only refresh the values.
        // This keeps group expansion state and avoids rebuilding the whole property editor on every click.
        if (m_detail == detail && m_materialAssetId == newAssetId && IsLoaded())
        {
            RefreshValues();
            return;
        }

        m_detail = detail;

        UnloadMaterial();

        if (m_detail == nullptr || !newAssetId.IsValid())
        {
            setVisible(false);
            return;
        }

        if (!LoadMaterial())
        {
            setVisible(false);
            return;
        }

        setVisible(true);
        Populate();
        LoadOverridesFromDetail();
    }

    void MaterialPropertyWidget::RefreshValues()
    {
        if (!IsLoaded())
        {
            return;
        }

        LoadOverridesFromDetail();
    }

    bool MaterialPropertyWidget::LoadMaterial()
    {
        m_materialAssetId = m_detail->m_material.GetId();

        auto materialAssetOutcome = AZ::RPI::AssetUtils::LoadAsset<AZ::RPI::MaterialAsset>(m_materialAssetId);
        if (!materialAssetOutcome)
        {
            AZ_Warning("MaterialPropertyWidget", false, "Failed to load the material assigned to this emitter.");
            UnloadMaterial();
            return false;
        }

        m_materialAsset = materialAssetOutcome.GetValue();
        m_materialTypeAsset = m_materialAsset->GetMaterialTypeAsset();

        // The property layout that ships in the material type product asset only carries ids and raw types.
        // Display names, descriptions, groups, ranges and enum labels all live in the .materialtype source
        // file, so that is what we load to build a UI that matches the mesh material inspector.
        const AZStd::string materialTypeSourcePath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(m_materialTypeAsset.GetId());
        if (materialTypeSourcePath.empty())
        {
            AZ_Warning("MaterialPropertyWidget", false, "Could not locate the source material type for this emitter's material.");
            UnloadMaterial();
            return false;
        }

        auto materialTypeOutcome = AZ::RPI::MaterialUtils::LoadMaterialTypeSourceData(materialTypeSourcePath);
        if (!materialTypeOutcome.IsSuccess())
        {
            AZ_Warning("MaterialPropertyWidget", false, "Failed to load material type source data: %s", materialTypeSourcePath.c_str());
            UnloadMaterial();
            return false;
        }

        m_materialTypeSourceData = materialTypeOutcome.TakeValue();
        return true;
    }

    void MaterialPropertyWidget::UnloadMaterial()
    {
        Reset();
        m_materialAssetId = {};
        m_materialAsset = {};
        m_materialTypeAsset = {};
        m_materialTypeSourceData = {};
    }

    void MaterialPropertyWidget::Populate()
    {
        AddGroupsBegin();

        m_materialTypeSourceData.EnumeratePropertyGroups(
            [this](const AZ::RPI::MaterialTypeSourceData::PropertyGroupStack& propertyGroupStack)
            {
                using namespace AZ::RPI;

                const MaterialTypeSourceData::PropertyGroup* propertyGroupDefinition = propertyGroupStack.back();
                MaterialNameContext groupNameContext = MaterialTypeSourceData::MakeMaterialNameContext(propertyGroupStack);

                AZStd::vector<AZStd::string> groupNameVector;
                AZStd::vector<AZStd::string> groupDisplayNameVector;
                groupNameVector.reserve(propertyGroupStack.size());
                groupDisplayNameVector.reserve(propertyGroupStack.size());

                for (const auto& nextGroup : propertyGroupStack)
                {
                    groupNameVector.push_back(nextGroup->GetName());
                    groupDisplayNameVector.push_back(
                        !nextGroup->GetDisplayName().empty() ? nextGroup->GetDisplayName() : nextGroup->GetName());
                }

                AZStd::string groupId;
                AzFramework::StringFunc::Join(groupId, groupNameVector.begin(), groupNameVector.end(), ".");

                auto& group = m_groups[groupId];
                group.m_name = groupId;
                AzFramework::StringFunc::Join(
                    group.m_displayName, groupDisplayNameVector.begin(), groupDisplayNameVector.end(), " | ");
                group.m_description = !propertyGroupDefinition->GetDescription().empty()
                    ? propertyGroupDefinition->GetDescription()
                    : group.m_displayName;

                group.m_properties.reserve(propertyGroupDefinition->GetProperties().size());
                for (const auto& propertyDefinition : propertyGroupDefinition->GetProperties())
                {
                    AtomToolsFramework::DynamicPropertyConfig propertyConfig;

                    // The id has to be assigned before conversion because it is folded into the description.
                    propertyConfig.m_id = propertyDefinition->GetName();
                    groupNameContext.ContextualizeProperty(propertyConfig.m_id);

                    AtomToolsFramework::ConvertToPropertyConfig(propertyConfig, *propertyDefinition);

                    const auto& propertyIndex = m_materialAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(propertyConfig.m_id);
                    if (propertyIndex.IsNull())
                    {
                        // Declared in the source data but absent from the built layout; nothing to override.
                        continue;
                    }

                    propertyConfig.m_groupName = group.m_name;
                    propertyConfig.m_groupDisplayName = group.m_displayName;
                    propertyConfig.m_showThumbnail = true;

                    // There is no parent material in this workflow. The emitter's overrides sit directly on top
                    // of the assigned material asset, so the asset's values act as both the reset target and the
                    // baseline the "modified" indicator compares against.
                    const auto& assetValue = m_materialAsset->GetPropertyValues()[propertyIndex.GetIndex()];
                    propertyConfig.m_defaultValue = AtomToolsFramework::ConvertToEditableType(assetValue);
                    propertyConfig.m_parentValue = propertyConfig.m_defaultValue;
                    propertyConfig.m_originalValue = propertyConfig.m_defaultValue;

                    group.m_properties.emplace_back(propertyConfig);
                }

                // A group whose properties were all filtered out would render as an empty header.
                if (group.m_properties.empty())
                {
                    m_groups.erase(groupId);
                    return true;
                }

                // Passing the same group as both instance and comparison instance enables the custom value
                // comparison that drives the "this value is overridden" indicator icon.
                auto propertyGroupWidget = new AtomToolsFramework::InspectorPropertyGroupWidget(
                    &group, &group, group.TYPEINFO_Uuid(), this, this, GetGroupSaveStateKey(group.m_name), {},
                    [this](const auto node)
                    {
                        return GetInstanceNodePropertyIndicator(node);
                    },
                    0);

                AddGroup(group.m_name, group.m_displayName, group.m_description, propertyGroupWidget);
                return true;
            });

        AddGroupsEnd();
    }

    void MaterialPropertyWidget::LoadOverridesFromDetail()
    {
        if (m_detail == nullptr || !IsLoaded())
        {
            return;
        }

        m_updatingUi = true;

        // Material types can rename properties between versions. Migrate the stored ids first so the values
        // land on the right controls instead of silently disappearing.
        if (m_materialTypeAsset)
        {
            AZStd::vector<AZStd::pair<AZ::Name, AZ::Name>> renamedProperties;
            for (const auto& overridePair : m_detail->m_materialOverrides)
            {
                AZ::Name newName = overridePair.first;
                if (m_materialTypeAsset->ApplyPropertyRenames(newName))
                {
                    renamedProperties.emplace_back(overridePair.first, newName);
                }
            }
            for (const auto& [oldName, newName] : renamedProperties)
            {
                m_detail->m_materialOverrides[newName] = m_detail->m_materialOverrides[oldName];
                m_detail->m_materialOverrides.erase(oldName);
            }
        }

        for (auto& groupPair : m_groups)
        {
            auto& group = groupPair.second;
            for (auto& property : group.m_properties)
            {
                const auto& propertyConfig = property.GetConfig();
                const auto overrideItr = m_detail->m_materialOverrides.find(propertyConfig.m_id);

                // No override means show the value the material asset itself carries.
                const auto& editValue = overrideItr != m_detail->m_materialOverrides.end()
                    ? overrideItr->second
                    : propertyConfig.m_originalValue;

                // Round trip through the runtime type so values that arrived as an asset id or a loosely
                // typed number end up in the exact shape the control expects.
                const auto runtimeValue = AtomToolsFramework::ConvertToRuntimeType(editValue);
                property.SetValue(
                    runtimeValue.IsValid() ? AtomToolsFramework::ConvertToEditableType(runtimeValue) : editValue);
            }
        }

        m_updatingUi = false;

        RebuildAll();
    }

    void MaterialPropertyWidget::SaveOverrideToDetail(const AtomToolsFramework::DynamicProperty& property)
    {
        if (m_detail == nullptr)
        {
            return;
        }

        // Only store genuine differences. Dragging a slider back to the material's own value removes the
        // entry entirely, which keeps the .particle file clean and lets the emitter fall back to sharing
        // the material's value if the material is later edited.
        if (AtomToolsFramework::ArePropertyValuesEqual(property.GetValue(), property.GetConfig().m_originalValue))
        {
            m_detail->m_materialOverrides.erase(property.GetId());
        }
        else
        {
            m_detail->m_materialOverrides[property.GetId()] = property.GetValue();
        }
    }

    AZ::Crc32 MaterialPropertyWidget::GetGroupSaveStateKey(const AZStd::string& groupName) const
    {
        return AZ::Crc32(AZStd::string::format(
            "MaterialPropertyWidget::PropertyGroup::%s::%s", m_materialAssetId.ToString<AZStd::string>().c_str(), groupName.c_str()));
    }

    bool MaterialPropertyWidget::IsInstanceNodePropertyModified(const AzToolsFramework::InstanceDataNode* node) const
    {
        const auto property = AtomToolsFramework::FindAncestorInstanceDataNodeByType<AtomToolsFramework::DynamicProperty>(node);
        return property && !AtomToolsFramework::ArePropertyValuesEqual(property->GetValue(), property->GetConfig().m_originalValue);
    }

    const char* MaterialPropertyWidget::GetInstanceNodePropertyIndicator(const AzToolsFramework::InstanceDataNode* node) const
    {
        if (IsInstanceNodePropertyModified(node))
        {
            return ":/Icons/changed_property.svg";
        }
        return ":/Icons/blank.png";
    }

    void MaterialPropertyWidget::BeforePropertyModified([[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
    }

    void MaterialPropertyWidget::AfterPropertyModified(AzToolsFramework::InstanceDataNode* node)
    {
        if (m_updatingUi)
        {
            return;
        }

        const auto property = AtomToolsFramework::FindAncestorInstanceDataNodeByType<AtomToolsFramework::DynamicProperty>(node);
        if (property)
        {
            SaveOverrideToDetail(*property);

            // Fired continuously while dragging so the viewport tracks the edit live.
            Q_EMIT OnMaterialPropertyChanged(false);
        }
    }

    void MaterialPropertyWidget::SetPropertyEditingActive([[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
    }

    void MaterialPropertyWidget::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* node)
    {
        if (m_updatingUi)
        {
            return;
        }

        const auto property = AtomToolsFramework::FindAncestorInstanceDataNodeByType<AtomToolsFramework::DynamicProperty>(node);
        if (property)
        {
            SaveOverrideToDetail(*property);

            // The edit has settled, so listeners can now do the expensive work such as marking the
            // document modified.
            Q_EMIT OnMaterialPropertyChanged(true);
        }
    }

    void MaterialPropertyWidget::SealUndoStack()
    {
    }
} // namespace OpenParticleSystemEditor
