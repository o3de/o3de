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
#include <Window/Inspector/MaterialCanvasInspector.h>

namespace MaterialCanvas
{
    MaterialCanvasInspector::MaterialCanvasInspector(const AZ::Crc32& toolId, QWidget* parent)
        : AtomToolsFramework::InspectorWidget(parent)
        , m_toolId(toolId)
    {
        m_windowSettings = AZ::UserSettings::CreateFind<MaterialCanvasMainWindowSettings>(
            AZ::Crc32("MaterialCanvasMainWindowSettings"), AZ::UserSettings::CT_GLOBAL);

        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
    }

    MaterialCanvasInspector::~MaterialCanvasInspector()
    {
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
    }

    void MaterialCanvasInspector::Reset()
    {
        m_documentPath.clear();
        m_documentId = AZ::Uuid::CreateNull();
        m_activeProperty = {};

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
        }

        AddGroupsEnd();
    }

    AZ::Crc32 MaterialCanvasInspector::GetGroupSaveStateKey(const AZStd::string& groupName) const
    {
        return AZ::Crc32(AZStd::string::format("MaterialCanvasInspector::PropertyGroup::%s::%s", m_documentPath.c_str(), groupName.c_str()));
    }

    bool MaterialCanvasInspector::IsInstanceNodePropertyModifed(const AzToolsFramework::InstanceDataNode* node) const
    {
        const auto property = AtomToolsFramework::FindAncestorInstanceDataNodeByType<AtomToolsFramework::DynamicProperty>(node);
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

    void MaterialCanvasInspector::OnDocumentObjectInfoChanged(
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

    void MaterialCanvasInspector::BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode)
    {
        // This function is called before every single property change whether it's a button click or dragging a slider. We only want to
        // begin tracking undo state for the first change in the sequence, when the user begins to drag the slider.
        const auto property = AtomToolsFramework::FindAncestorInstanceDataNodeByType<AtomToolsFramework::DynamicProperty>(pNode);
        if (!m_activeProperty && property)
        {
            m_activeProperty = property;
            AtomToolsFramework::AtomToolsDocumentRequestBus::Event(
                m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::BeginEdit);
        }
    }

    void MaterialCanvasInspector::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode)
    {
        // If tracking has started and editing has completed then we can stop tracking undue state for this sequence of changes.
        const auto property = AtomToolsFramework::FindAncestorInstanceDataNodeByType<AtomToolsFramework::DynamicProperty>(pNode);
        if (m_activeProperty && m_activeProperty == property)
        {
            AtomToolsFramework::AtomToolsDocumentRequestBus::Event(
                m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::EndEdit);
            m_activeProperty = {};
        }
    }
} // namespace MaterialCanvas

#include <Window/Inspector/moc_MaterialCanvasInspector.cpp>
