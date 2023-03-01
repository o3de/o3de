/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/DocumentEditor/ToolsDocumentInspector.h>
#include <AzToolsFramework/DocumentEditor/ToolsDocumentRequestBus.h>
#include <AzToolsFramework/DocumentEditor/Inspector/InspectorPropertyGroupWidget.h>

namespace AzToolsFramework
{
    ToolsDocumentInspector::ToolsDocumentInspector(const AZ::Crc32& toolId, QWidget* parent)
        : InspectorWidget(parent)
        , m_toolId(toolId)
    {
        ToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
    }

    ToolsDocumentInspector::~ToolsDocumentInspector()
    {
        ToolsDocumentNotificationBus::Handler::BusDisconnect();
    }

    void ToolsDocumentInspector::SetDocumentId(const AZ::Uuid& documentId)
    {
        m_documentId = documentId;
        Populate();
    }

    void ToolsDocumentInspector::SetDocumentSettingsPrefix(const AZStd::string& prefix)
    {
        m_documentSettingsPrefix = prefix;
    }

    void ToolsDocumentInspector::Reset()
    {
        m_editInProgress = false;
        InspectorWidget::Reset();
    }

    void ToolsDocumentInspector::OnDocumentObjectInfoChanged(
        [[maybe_unused]] const AZ::Uuid& documentId, const DocumentObjectInfo& objectInfo, bool rebuilt)
    {
        if (m_documentId == documentId)
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
    }

    void ToolsDocumentInspector::OnDocumentObjectInfoInvalidated(const AZ::Uuid& documentId)
    {
        if (m_documentId == documentId)
        {
            Populate();
        }
    }

    void ToolsDocumentInspector::OnDocumentModified(const AZ::Uuid& documentId)
    {
        if (m_documentId == documentId && !m_editInProgress)
        {
            RefreshAll();
        }
    }

    void ToolsDocumentInspector::OnDocumentCleared(const AZ::Uuid& documentId)
    {
        if (m_documentId == documentId)
        {
            Reset();
        }
    }

    void ToolsDocumentInspector::OnDocumentError(const AZ::Uuid& documentId)
    {
        if (m_documentId == documentId)
        {
            Reset();
        }
    }

    void ToolsDocumentInspector::BeforePropertyModified([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode)
    {
        if (!m_editInProgress)
        {
            m_editInProgress = true;
            ToolsDocumentRequestBus::Event(m_documentId, &ToolsDocumentRequestBus::Events::BeginEdit);
            ToolsDocumentNotificationBus::Event(m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentModified, m_documentId);
        }
    }

    void ToolsDocumentInspector::AfterPropertyModified([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode)
    {
        if (m_editInProgress)
        {
            ToolsDocumentNotificationBus::Event(m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentModified, m_documentId);
        }
    }

    void ToolsDocumentInspector::SetPropertyEditingComplete([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode)
    {
        if (m_editInProgress)
        {
            ToolsDocumentNotificationBus::Event(m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentModified, m_documentId);
            ToolsDocumentRequestBus::Event(m_documentId, &ToolsDocumentRequestBus::Events::EndEdit);
            m_editInProgress = false;
        }
    }

    void ToolsDocumentInspector::Populate()
    {
        AddGroupsBegin();

        ToolsDocumentRequestBus::Event(
            m_documentId,
            [&](ToolsDocumentRequests* documentRequests)
            {
                // Create a unique settings prefix string per document using a CRC of the document path
                const AZStd::string groupSettingsPrefix = m_documentSettingsPrefix +
                    AZStd::string::format("/%08x/GroupSettings", aznumeric_cast<AZ::u32>(AZ::Crc32(documentRequests->GetAbsolutePath())));
                SetGroupSettingsPrefix(groupSettingsPrefix);

                // This will automatically expose all document contents to an inspector with a collapsible group per object.
                // In the case of the material editor, this will be one inspector group per property group.
                for (auto& objectInfo : documentRequests->GetObjectInfo())
                {
                    // Passing in same main and comparison instance to enable custom value comparison
                    const AZ::Crc32 groupSaveStateKey(
                        AZStd::string::format("%s/%s", groupSettingsPrefix.c_str(), objectInfo.m_name.c_str()));
                    auto propertyGroupWidget = new InspectorPropertyGroupWidget(
                        objectInfo.m_objectPtr,
                        objectInfo.m_objectPtr,
                        objectInfo.m_objectType,
                        this,
                        this,
                        groupSaveStateKey,
                        {},
                        objectInfo.m_nodeIndicatorFunction,
                        0);

                    AddGroup(objectInfo.m_name, objectInfo.m_displayName, objectInfo.m_description, propertyGroupWidget);
                    SetGroupVisible(objectInfo.m_name, objectInfo.m_visible);
                }

                InspectorRequestBus::Handler::BusConnect(m_documentId);
            });

        AddGroupsEnd();
    }
} // namespace AzToolsFramework
