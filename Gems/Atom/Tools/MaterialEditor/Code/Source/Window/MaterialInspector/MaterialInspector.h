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
#include <AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h>
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#include <Window/MaterialEditorWindowSettings.h>
#endif

namespace MaterialEditor
{
    //! Provides controls for viewing and editing document settings.
    class MaterialInspector
        : public AtomToolsFramework::InspectorWidget
        , public AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler
        , public AzToolsFramework::IPropertyEditorNotify
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(MaterialInspector, AZ::SystemAllocator, 0);

        MaterialInspector(const AZ::Crc32& toolId, QWidget* parent = nullptr);
        ~MaterialInspector() override;

        // AtomToolsFramework::InspectorRequestBus::Handler overrides...
        void Reset() override;

    protected:
        bool ShouldGroupAutoExpanded(const AZStd::string& groupName) const override;
        void OnGroupExpanded(const AZStd::string& groupName) override;
        void OnGroupCollapsed(const AZStd::string& groupName) override;

    private:
        AZ::Crc32 GetGroupSaveStateKey(const AZStd::string& groupName) const;
        bool IsInstanceNodePropertyModifed(const AzToolsFramework::InstanceDataNode* node) const;
        const char* GetInstanceNodePropertyIndicator(const AzToolsFramework::InstanceDataNode* node) const;

        // AtomToolsDocumentNotificationBus::Handler implementation
        void OnDocumentOpened(const AZ::Uuid& documentId) override;
        void OnDocumentObjectInfoChanged(
            const AZ::Uuid& documentId, const AtomToolsFramework::DocumentObjectInfo& objectInfo, bool rebuilt) override;

        // AzToolsFramework::IPropertyEditorNotify overrides...
        void BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
        void AfterPropertyModified([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode) override {}
        void SetPropertyEditingActive([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode) override {}
        void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode) override;
        void SealUndoStack() override {}
        void RequestPropertyContextMenu([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode, const QPoint&) override {}
        void PropertySelectionChanged([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode, bool) override {}

        const AZ::Crc32 m_toolId = {};

        // Tracking the property that is activiley being edited in the inspector
        const AtomToolsFramework::DynamicProperty* m_activeProperty = {};

        AZ::Uuid m_documentId = AZ::Uuid::CreateNull();
        AZStd::string m_documentPath;
        AZStd::intrusive_ptr<MaterialEditorWindowSettings> m_windowSettings;
    };
} // namespace MaterialEditor
