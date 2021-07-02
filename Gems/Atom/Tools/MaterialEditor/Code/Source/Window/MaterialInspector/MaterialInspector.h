/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>

#include <AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h>
#include <AtomToolsFramework/Inspector/InspectorWidget.h>

#include <Atom/Document/MaterialDocumentNotificationBus.h>
#include <Atom/Window/MaterialEditorWindowSettings.h>
#endif

namespace MaterialEditor
{
    //! Provides controls for viewing and editing a material document settings.
    //! The settings can be divided into cards, with each one showing a subset of properties.
    class MaterialInspector
        : public AtomToolsFramework::InspectorWidget
        , public MaterialDocumentNotificationBus::Handler
        , public AzToolsFramework::IPropertyEditorNotify
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(MaterialInspector, AZ::SystemAllocator, 0);

        explicit MaterialInspector(QWidget* parent = nullptr);
        ~MaterialInspector() override;

        // AtomToolsFramework::InspectorRequestBus::Handler overrides...
        void Reset() override;

    protected:
        bool ShouldGroupAutoExpanded(const AZStd::string& groupNameId) const override;
        void OnGroupExpanded(const AZStd::string& groupNameId) override;
        void OnGroupCollapsed(const AZStd::string& groupNameId) override;

    private:
        AZ::Crc32 GetGroupSaveStateKey(const AZStd::string& groupNameId) const;
        bool CompareInstanceNodeProperties(
            const AzToolsFramework::InstanceDataNode* source, const AzToolsFramework::InstanceDataNode* target) const;

        void AddOverviewGroup();
        void AddUvNamesGroup();
        void AddPropertiesGroup();

        // MaterialDocumentNotificationBus::Handler implementation
        void OnDocumentOpened(const AZ::Uuid& documentId) override;
        void OnDocumentPropertyValueModified(const AZ::Uuid& documentId, const AtomToolsFramework::DynamicProperty& property) override;
        void OnDocumentPropertyConfigModified(const AZ::Uuid& documentId, const AtomToolsFramework::DynamicProperty& property) override;
        void OnDocumentPropertyGroupVisibilityChanged(const AZ::Uuid& documentId, const AZ::Name& groupId, bool visible) override;

        // AzToolsFramework::IPropertyEditorNotify overrides...
        void BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
        void AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
        void SetPropertyEditingActive([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode) override {}
        void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode) override;
        void SealUndoStack() override {}
        void RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode*, const QPoint&) override {}
        void PropertySelectionChanged(AzToolsFramework::InstanceDataNode*, bool) override {}

        // Tracking the property that is activiley being edited in the inspector
        const AtomToolsFramework::DynamicProperty* m_activeProperty = nullptr;

        AZ::Uuid m_documentId = AZ::Uuid::CreateNull();
        AZStd::string m_documentPath;
        AZStd::unordered_map<AZStd::string, AtomToolsFramework::DynamicPropertyGroup> m_groups;
        AZStd::intrusive_ptr<MaterialEditorWindowSettings> m_windowSettings;
    };
} // namespace MaterialEditor
