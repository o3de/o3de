/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <Atom/Document/MaterialDocumentSettings.h>
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#endif

namespace MaterialEditor
{
    //! Provides controls for viewing and editing settings.
    class SettingsWidget
        : public AtomToolsFramework::InspectorWidget
        , private AzToolsFramework::IPropertyEditorNotify
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(SettingsWidget, AZ::SystemAllocator, 0);

        explicit SettingsWidget(QWidget* parent = nullptr);
        ~SettingsWidget() override;

        void Populate();

    private:
        void AddDocumentGroup();

        // AtomToolsFramework::InspectorRequestBus::Handler overrides...
        void Reset() override;

        // AzToolsFramework::IPropertyEditorNotify overrides...
        void BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
        void AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
        void SetPropertyEditingActive([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode) override {}
        void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode) override;
        void SealUndoStack() override {}
        void RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode*, const QPoint&) override {}
        void PropertySelectionChanged(AzToolsFramework::InstanceDataNode*, bool) override {}

        AZStd::intrusive_ptr<MaterialDocumentSettings> m_documentSettings;
    };
} // namespace MaterialEditor
