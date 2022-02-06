/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemSettings.h>
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#endif

namespace MaterialEditor
{
    //! Provides controls for viewing and editing settings.
    class SettingsWidget
        : public AtomToolsFramework::InspectorWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(SettingsWidget, AZ::SystemAllocator, 0);

        explicit SettingsWidget(QWidget* parent = nullptr);
        ~SettingsWidget() override;

        void Populate();

    private:
        void AddDocumentSystemSettingsGroup();

        // AtomToolsFramework::InspectorRequestBus::Handler overrides...
        void Reset() override;

        AZStd::intrusive_ptr<AtomToolsFramework::AtomToolsDocumentSystemSettings> m_documentSystemSettings;
    };
} // namespace MaterialEditor
