/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#endif

namespace AtomToolsFramework
{
    //! This is a specialized inspector widget that populates itself by inspecting reflected document object info.
    //! Each element of an AtomToolsDocument object info vector will be displayed in a collapsible RPE group in the inspector.
    //! Property changes emitted from each RPE will be tracked and used to signal undo/redo events in the document.
    class AtomToolsDocumentInspector
        : public InspectorWidget
        , public AtomToolsDocumentNotificationBus::Handler
        , public AzToolsFramework::IPropertyEditorNotify
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(AtomToolsDocumentInspector, AZ::SystemAllocator);

        AtomToolsDocumentInspector(const AZ::Crc32& toolId, QWidget* parent = nullptr);
        ~AtomToolsDocumentInspector() override;

        //! Set the ID of the document that will be used to populate the inspector 
        void SetDocumentId(const AZ::Uuid& documentId);

        //! Set a prefix string for storing registry settings 
        void SetDocumentSettingsPrefix(const AZStd::string& prefix);

        // InspectorRequestBus::Handler overrides...
        void Reset() override;

    private:
        // AtomToolsDocumentNotificationBus::Handler implementation
        void OnDocumentObjectInfoChanged(const AZ::Uuid& documentId, const DocumentObjectInfo& objectInfo, bool rebuilt) override;
        void OnDocumentObjectInfoInvalidated(const AZ::Uuid& documentId) override;
        void OnDocumentModified(const AZ::Uuid& documentId) override;
        void OnDocumentCleared(const AZ::Uuid& documentId) override;
        void OnDocumentError(const AZ::Uuid& documentId) override;

        // AzToolsFramework::IPropertyEditorNotify overrides...
        void BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
        void AfterPropertyModified([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode) override;
        void SetPropertyEditingActive([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode) override {}
        void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode) override;
        void SealUndoStack() override {}
        void RequestPropertyContextMenu([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode, const QPoint&) override {}
        void PropertySelectionChanged([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode, bool) override {}

        void Populate();

        const AZ::Crc32 m_toolId = {};

        bool m_editInProgress = {};

        AZ::Uuid m_documentId = AZ::Uuid::CreateNull();

        AZStd::string m_documentSettingsPrefix = "/O3DE/AtomToolsFramework/AtomToolsDocumentInspector";
    };
} // namespace AtomToolsFramework
