/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Document/MaterialDocumentRequestBus.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h>
#include <AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h>
#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>
#include <Window/MaterialInspector/MaterialInspector.h>

namespace MaterialEditor
{
    MaterialInspector::MaterialInspector(QWidget* parent)
        : AtomToolsFramework::InspectorWidget(parent)
    {
        m_windowSettings = AZ::UserSettings::CreateFind<MaterialEditorWindowSettings>(
            AZ::Crc32("MaterialEditorWindowSettings"), AZ::UserSettings::CT_GLOBAL);

        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect();
    }

    MaterialInspector::~MaterialInspector()
    {
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
        AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
    }

    void MaterialInspector::Reset()
    {
        m_documentPath.clear();
        m_documentId = AZ::Uuid::CreateNull();
        m_groups = {};

        AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
        AtomToolsFramework::InspectorWidget::Reset();
    }

    bool MaterialInspector::ShouldGroupAutoExpanded(const AZStd::string& groupName) const
    {
        auto stateItr = m_windowSettings->m_inspectorCollapsedGroups.find(GetGroupSaveStateKey(groupName));
        return stateItr == m_windowSettings->m_inspectorCollapsedGroups.end();
    }

    void MaterialInspector::OnGroupExpanded(const AZStd::string& groupName)
    {
        m_windowSettings->m_inspectorCollapsedGroups.erase(GetGroupSaveStateKey(groupName));
    }

    void MaterialInspector::OnGroupCollapsed(const AZStd::string& groupName)
    {
        m_windowSettings->m_inspectorCollapsedGroups.insert(GetGroupSaveStateKey(groupName));
    }

    void MaterialInspector::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        AddGroupsBegin();

        m_documentId = documentId;

        bool isOpen = false;
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(isOpen, m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::IsOpen);

        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(m_documentPath, m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::GetAbsolutePath);

        if (!m_documentId.IsNull() && isOpen)
        {
            // Create the top group for displaying overview info about the material
            AddOverviewGroup();
            // Create groups for displaying editable UV names
            AddUvNamesGroup();
            // Create groups for displaying editable properties
            AddPropertiesGroup();

            AtomToolsFramework::InspectorRequestBus::Handler::BusConnect(m_documentId);
        }

        AddGroupsEnd();
    }

    AZ::Crc32 MaterialInspector::GetGroupSaveStateKey(const AZStd::string& groupName) const
    {
        return AZ::Crc32(AZStd::string::format("MaterialInspector::PropertyGroup::%s::%s", m_documentPath.c_str(), groupName.c_str()));
    }

    bool MaterialInspector::IsInstanceNodePropertyModifed(const AzToolsFramework::InstanceDataNode* node) const
    {
        const AtomToolsFramework::DynamicProperty* property = AtomToolsFramework::FindDynamicPropertyForInstanceDataNode(node);
        return property && !AtomToolsFramework::ArePropertyValuesEqual(property->GetValue(), property->GetConfig().m_parentValue);
    }

    const char* MaterialInspector::GetInstanceNodePropertyIndicator(const AzToolsFramework::InstanceDataNode* node) const
    {
        if (IsInstanceNodePropertyModifed(node))
        {
            return ":/Icons/changed_property.svg";
        }
        return ":/Icons/blank.png";
    }

    void MaterialInspector::AddOverviewGroup()
    {
        const AZ::RPI::MaterialTypeSourceData* materialTypeSourceData = nullptr;
        MaterialDocumentRequestBus::EventResult(
            materialTypeSourceData, m_documentId, &MaterialDocumentRequestBus::Events::GetMaterialTypeSourceData);

        const AZStd::string groupName = "overview";
        const AZStd::string groupDisplayName = "Overview";
        const AZStd::string groupDescription = materialTypeSourceData->m_description;
        auto& group = m_groups[groupName];

        AtomToolsFramework::DynamicProperty property;
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
            property, m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::GetProperty, AZ::Name("overview.materialType"));
        group.m_properties.push_back(property);

        property = {};
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
            property, m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::GetProperty, AZ::Name("overview.parentMaterial"));
        group.m_properties.push_back(property);

        // Passing in same group as main and comparison instance to enable custom value comparison for highlighting modified properties
        auto propertyGroupWidget = new AtomToolsFramework::InspectorPropertyGroupWidget(
            &group, &group, group.TYPEINFO_Uuid(), this, this, GetGroupSaveStateKey(groupName), {},
            [this](const auto node) { return GetInstanceNodePropertyIndicator(node); }, 0);
        AddGroup(groupName, groupDisplayName, groupDescription, propertyGroupWidget);
    }

    void MaterialInspector::AddUvNamesGroup()
    {
        AZ::Data::Asset<AZ::RPI::MaterialAsset> materialAsset;
        MaterialDocumentRequestBus::EventResult(materialAsset, m_documentId, &MaterialDocumentRequestBus::Events::GetAsset);

        const AZStd::string groupName = UvGroupName;
        const AZStd::string groupDisplayName = "UV Sets";
        const AZStd::string groupDescription = "UV set names in this material, which can be renamed to match those in the model.";
        auto& group = m_groups[groupName];

        const auto& uvNameMap = materialAsset->GetMaterialTypeAsset()->GetUvNameMap();
        group.m_properties.reserve(uvNameMap.size());

        for (const auto& uvNamePair : uvNameMap)
        {
            AtomToolsFramework::DynamicProperty property;
            AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                property, m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::GetProperty,
                AZ::RPI::MaterialPropertyId(groupName, uvNamePair.m_shaderInput.ToString()).GetFullName());
            group.m_properties.push_back(property);

            property.SetValue(property.GetConfig().m_parentValue);
        }

        // Passing in same group as main and comparison instance to enable custom value comparison for highlighting modified properties
        auto propertyGroupWidget = new AtomToolsFramework::InspectorPropertyGroupWidget(
            &group, &group, group.TYPEINFO_Uuid(), this, this, GetGroupSaveStateKey(groupName), {},
            [this](const auto node) { return GetInstanceNodePropertyIndicator(node); }, 0);
        AddGroup(groupName, groupDisplayName, groupDescription, propertyGroupWidget);
    }

    void MaterialInspector::AddPropertiesGroup()
    {
        const AZ::RPI::MaterialTypeSourceData* materialTypeSourceData = nullptr;
        MaterialDocumentRequestBus::EventResult(
            materialTypeSourceData, m_documentId, &MaterialDocumentRequestBus::Events::GetMaterialTypeSourceData);

        for (const auto& groupDefinition : materialTypeSourceData->GetGroupDefinitionsInDisplayOrder())
        {
            const AZStd::string& groupName = groupDefinition.m_name;
            const AZStd::string& groupDisplayName = !groupDefinition.m_displayName.empty() ? groupDefinition.m_displayName : groupName;
            const AZStd::string& groupDescription =
                !groupDefinition.m_description.empty() ? groupDefinition.m_description : groupDisplayName;
            auto& group = m_groups[groupName];

            const auto& propertyLayout = materialTypeSourceData->m_propertyLayout;
            const auto& propertyListItr = propertyLayout.m_properties.find(groupName);
            if (propertyListItr != propertyLayout.m_properties.end())
            {
                group.m_properties.reserve(propertyListItr->second.size());
                for (const auto& propertyDefinition : propertyListItr->second)
                {
                    AtomToolsFramework::DynamicProperty property;
                    AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                        property, m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::GetProperty,
                        AZ::RPI::MaterialPropertyId(groupName, propertyDefinition.m_name).GetFullName());
                    group.m_properties.push_back(property);
                }
            }

            // Passing in same group as main and comparison instance to enable custom value comparison for highlighting modified properties
            auto propertyGroupWidget = new AtomToolsFramework::InspectorPropertyGroupWidget(
                &group, &group, group.TYPEINFO_Uuid(), this, this, GetGroupSaveStateKey(groupName), {},
                [this](const auto node) { return GetInstanceNodePropertyIndicator(node); }, 0);
            AddGroup(groupName, groupDisplayName, groupDescription, propertyGroupWidget);
            
            bool isGroupVisible = false;
            AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                isGroupVisible, m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::IsPropertyGroupVisible, AZ::Name{groupName});
            SetGroupVisible(groupName, isGroupVisible);
        }
    }

    void MaterialInspector::OnDocumentPropertyValueModified(const AZ::Uuid& documentId, const AtomToolsFramework::DynamicProperty& property)
    {
        for (auto& groupPair : m_groups)
        {
            for (auto& reflectedProperty : groupPair.second.m_properties)
            {
                if (reflectedProperty.GetId() == property.GetId())
                {
                    if (!AtomToolsFramework::ArePropertyValuesEqual(reflectedProperty.GetValue(), property.GetValue()))
                    {
                        reflectedProperty.SetValue(property.GetValue());
                        AtomToolsFramework::InspectorRequestBus::Event(
                            documentId, &AtomToolsFramework::InspectorRequestBus::Events::RefreshGroup, groupPair.first);
                    }
                    return;
                }
            }
        }
    }

    void MaterialInspector::OnDocumentPropertyConfigModified(const AZ::Uuid&, const AtomToolsFramework::DynamicProperty& property)
    {
        for (auto& groupPair : m_groups)
        {
            for (auto& reflectedProperty : groupPair.second.m_properties)
            {
                if (reflectedProperty.GetId() == property.GetId())
                {
                    // Visibility changes require the entire reflected property editor tree for this group to be rebuilt
                    if (reflectedProperty.GetVisibility() != property.GetVisibility())
                    {
                        reflectedProperty.SetConfig(property.GetConfig());
                        RebuildGroup(groupPair.first);
                    }
                    else
                    {
                        reflectedProperty.SetConfig(property.GetConfig());
                        RefreshGroup(groupPair.first);
                    }
                    return;
                }
            }
        }
    }
    
    void MaterialInspector::OnDocumentPropertyGroupVisibilityChanged(const AZ::Uuid&, const AZ::Name& groupId, bool visible)
    {
        SetGroupVisible(groupId.GetStringView(), visible);
    }

    void MaterialInspector::BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode)
    {
        // For some reason the reflected property editor notifications are not symmetrical
        // This function is called continuously anytime a property changes until the edit has completed
        // Because of that, we have to track whether or not we are continuing to edit the same property to know when editing has started and
        // ended
        const AtomToolsFramework::DynamicProperty* property = AtomToolsFramework::FindDynamicPropertyForInstanceDataNode(pNode);
        if (property)
        {
            if (m_activeProperty != property)
            {
                m_activeProperty = property;
                AtomToolsFramework::AtomToolsDocumentRequestBus::Event(m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::BeginEdit);
            }
        }
    }

    void MaterialInspector::AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode)
    {
        const AtomToolsFramework::DynamicProperty* property = AtomToolsFramework::FindDynamicPropertyForInstanceDataNode(pNode);
        if (property)
        {
            if (m_activeProperty == property)
            {
                AtomToolsFramework::AtomToolsDocumentRequestBus::Event(
                    m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::SetPropertyValue, property->GetId(), property->GetValue());
            }
        }
    }

    void MaterialInspector::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode)
    {
        // As above, there are symmetrical functions on the notification interface for when editing begins and ends and has been completed
        // but they are not being called following that pattern. when this function executes the changes to the property are ready to be
        // committed or reverted
        const AtomToolsFramework::DynamicProperty* property = AtomToolsFramework::FindDynamicPropertyForInstanceDataNode(pNode);
        if (property)
        {
            if (m_activeProperty == property)
            {
                AtomToolsFramework::AtomToolsDocumentRequestBus::Event(
                    m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::SetPropertyValue, property->GetId(), property->GetValue());

                AtomToolsFramework::AtomToolsDocumentRequestBus::Event(m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::EndEdit);
                m_activeProperty = nullptr;
            }
        }
    }
} // namespace MaterialEditor

#include <Window/MaterialInspector/moc_MaterialInspector.cpp>
