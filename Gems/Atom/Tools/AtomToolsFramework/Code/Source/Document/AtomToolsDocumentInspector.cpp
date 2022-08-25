/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentInspector.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h>

namespace AtomToolsFramework
{
    AtomToolsDocumentInspector::AtomToolsDocumentInspector(const AZ::Crc32& toolId, QWidget* parent)
        : InspectorWidget(parent)
        , m_toolId(toolId)
    {
        AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
    }

    AtomToolsDocumentInspector::~AtomToolsDocumentInspector()
    {
        AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
    }

    void AtomToolsDocumentInspector::SetDocumentId(const AZ::Uuid& documentId)
    {
        m_documentId = documentId;
        Populate();
    }

    void AtomToolsDocumentInspector::SetDocumentSettingsPrefix(const AZStd::string& prefix)
    {
        m_documentSettingsPrefix = prefix;
    }

    void AtomToolsDocumentInspector::Reset()
    {
        m_editInProgress = false;
        InspectorWidget::Reset();
    }

    void AtomToolsDocumentInspector::OnDocumentObjectInfoChanged(
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

    void AtomToolsDocumentInspector::OnDocumentObjectInfoInvalidated(const AZ::Uuid& documentId)
    {
        if (m_documentId == documentId)
        {
            Populate();
        }
    }

    void AtomToolsDocumentInspector::OnDocumentModified(const AZ::Uuid& documentId)
    {
        if (m_documentId == documentId && !m_editInProgress)
        {
            RefreshAll();
        }
    }

    void AtomToolsDocumentInspector::BeforePropertyModified([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode)
    {
        if (!m_editInProgress)
        {
            m_editInProgress = true;
            AtomToolsDocumentRequestBus::Event(m_documentId, &AtomToolsDocumentRequestBus::Events::BeginEdit);
            AtomToolsDocumentNotificationBus::Event(m_toolId, &AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_documentId);
        }
    }

    void AtomToolsDocumentInspector::AfterPropertyModified([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode)
    {
        if (m_editInProgress)
        {
            AtomToolsDocumentNotificationBus::Event(m_toolId, &AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_documentId);
        }
    }

    void AtomToolsDocumentInspector::SetPropertyEditingComplete([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode)
    {
        if (m_editInProgress)
        {
            AtomToolsDocumentNotificationBus::Event(m_toolId, &AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_documentId);
            AtomToolsDocumentRequestBus::Event(m_documentId, &AtomToolsDocumentRequestBus::Events::EndEdit);
            m_editInProgress = false;
        }
    }

    void AtomToolsDocumentInspector::Populate()
    {
        AddGroupsBegin();

        AtomToolsDocumentRequestBus::Event(
            m_documentId,
            [&](AtomToolsDocumentRequests* documentRequests)
            {
                if (documentRequests->IsOpen())
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
                }

                InspectorRequestBus::Handler::BusConnect(m_documentId);
            });

        AddGroupsEnd();
    }
} // namespace AtomToolsFramework
