/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
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
        m_activeProperty = {};

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
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
            isOpen, m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::IsOpen);

        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
            m_documentPath, m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::GetAbsolutePath);

        if (!m_documentId.IsNull() && isOpen)
        {
            // This will automatically expose all document contents to an inspector with a collapsible group per object. In the case of the
            // material editor, this will be one inspector group per property group.
            AZStd::vector<AtomToolsFramework::DocumentObjectInfo> objects;
            AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                objects, m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::GetObjectInfo);

            for (auto& objectInfo : objects)
            {
                // Passing in same main and comparison instance to enable custom value comparison for highlighting modified properties
                auto propertyGroupWidget = new AtomToolsFramework::InspectorPropertyGroupWidget(
                    objectInfo.m_objectPtr, objectInfo.m_objectPtr, objectInfo.m_objectType, this, this,
                    GetGroupSaveStateKey(objectInfo.m_name), {},
                    [this](const auto node) { return GetInstanceNodePropertyIndicator(node); }, 0);

                AddGroup(objectInfo.m_name, objectInfo.m_displayName, objectInfo.m_description, propertyGroupWidget);
                SetGroupVisible(objectInfo.m_name, objectInfo.m_visible);
            }

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

    void MaterialInspector::OnDocumentObjectInfoChanged(
        [[maybe_unused]] const AZ::Uuid& documentId, const AtomToolsFramework::DocumentObjectInfo& objectInfo, bool rebuilt)
    {
        SetGroupVisible(objectInfo.m_name, objectInfo.m_visible);
        if (rebuilt)
        {
            RebuildGroup(objectInfo.m_name);
        }
        else
        {
            RefreshGroup(objectInfo.m_name);
        }
    }

    void MaterialInspector::BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode)
    {
        // This function is called before every single property change whether it's a button click or dragging a slider. We only want to
        // begin tracking undo state for the first change in the sequence, when the user begins to drag the slider.
        const AtomToolsFramework::DynamicProperty* property = AtomToolsFramework::FindDynamicPropertyForInstanceDataNode(pNode);
        if (!m_activeProperty && property)
        {
            m_activeProperty = property;
            AtomToolsFramework::AtomToolsDocumentRequestBus::Event(
                m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::BeginEdit);
        }
    }

    void MaterialInspector::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode)
    {
        // If tracking has started and editing has completed then we can stop tracking undue state for this sequence of changes.
        const AtomToolsFramework::DynamicProperty* property = AtomToolsFramework::FindDynamicPropertyForInstanceDataNode(pNode);
        if (m_activeProperty && m_activeProperty == property)
        {
            AtomToolsFramework::AtomToolsDocumentRequestBus::Event(
                m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::EndEdit);
            m_activeProperty = {};
        }
    }
} // namespace MaterialEditor

#include <Window/MaterialInspector/moc_MaterialInspector.cpp>
