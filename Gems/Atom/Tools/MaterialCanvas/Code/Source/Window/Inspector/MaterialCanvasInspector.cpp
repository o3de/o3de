/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h>
#include <AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h>
#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>
#include <Document/MaterialCanvasDocumentRequestBus.h>
#include <Window/Inspector/MaterialCanvasInspector.h>

namespace MaterialCanvas
{
    MaterialCanvasInspector::MaterialCanvasInspector(QWidget* parent)
        : AtomToolsFramework::InspectorWidget(parent)
    {
        m_windowSettings = AZ::UserSettings::CreateFind<MaterialCanvasMainWindowSettings>(
            AZ::Crc32("MaterialCanvasMainWindowSettings"), AZ::UserSettings::CT_GLOBAL);

        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect();
    }

    MaterialCanvasInspector::~MaterialCanvasInspector()
    {
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
        AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
    }

    void MaterialCanvasInspector::Reset()
    {
        m_documentPath.clear();
        m_documentId = AZ::Uuid::CreateNull();
        m_groups = {};

        AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
        AtomToolsFramework::InspectorWidget::Reset();
    }

    bool MaterialCanvasInspector::ShouldGroupAutoExpanded(const AZStd::string& groupName) const
    {
        auto stateItr = m_windowSettings->m_inspectorCollapsedGroups.find(GetGroupSaveStateKey(groupName));
        return stateItr == m_windowSettings->m_inspectorCollapsedGroups.end();
    }

    void MaterialCanvasInspector::OnGroupExpanded(const AZStd::string& groupName)
    {
        m_windowSettings->m_inspectorCollapsedGroups.erase(GetGroupSaveStateKey(groupName));
    }

    void MaterialCanvasInspector::OnGroupCollapsed(const AZStd::string& groupName)
    {
        m_windowSettings->m_inspectorCollapsedGroups.insert(GetGroupSaveStateKey(groupName));
    }

    void MaterialCanvasInspector::OnDocumentOpened(const AZ::Uuid& documentId)
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
            // Create groups for displaying editable properties
            AddPropertiesGroup();

            AtomToolsFramework::InspectorRequestBus::Handler::BusConnect(m_documentId);
        }

        AddGroupsEnd();
    }

    AZ::Crc32 MaterialCanvasInspector::GetGroupSaveStateKey(const AZStd::string& groupName) const
    {
        return AZ::Crc32(AZStd::string::format("MaterialCanvasInspector::PropertyGroup::%s::%s", m_documentPath.c_str(), groupName.c_str()));
    }

    bool MaterialCanvasInspector::IsInstanceNodePropertyModifed(const AzToolsFramework::InstanceDataNode* node) const
    {
        const AtomToolsFramework::DynamicProperty* property = AtomToolsFramework::FindDynamicPropertyForInstanceDataNode(node);
        return property && !AtomToolsFramework::ArePropertyValuesEqual(property->GetValue(), property->GetConfig().m_parentValue);
    }

    const char* MaterialCanvasInspector::GetInstanceNodePropertyIndicator(const AzToolsFramework::InstanceDataNode* node) const
    {
        if (IsInstanceNodePropertyModifed(node))
        {
            return ":/Icons/changed_property.svg";
        }
        return ":/Icons/blank.png";
    }

    void MaterialCanvasInspector::AddOverviewGroup()
    {
        const AZStd::string groupName = "overview";
        const AZStd::string groupDisplayName = "Overview";
        const AZStd::string groupDescription = "";
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

    void MaterialCanvasInspector::AddPropertiesGroup()
    {
    }

    void MaterialCanvasInspector::OnDocumentPropertyValueModified(const AZ::Uuid& documentId, const AtomToolsFramework::DynamicProperty& property)
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

    void MaterialCanvasInspector::OnDocumentPropertyConfigModified(const AZ::Uuid&, const AtomToolsFramework::DynamicProperty& property)
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
    
    void MaterialCanvasInspector::OnDocumentPropertyGroupVisibilityChanged(const AZ::Uuid&, const AZ::Name& groupId, bool visible)
    {
        SetGroupVisible(groupId.GetStringView(), visible);
    }

    void MaterialCanvasInspector::BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode)
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

    void MaterialCanvasInspector::AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode)
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

    void MaterialCanvasInspector::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode)
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
} // namespace MaterialCanvas

#include <Window/Inspector/moc_MaterialCanvasInspector.cpp>
