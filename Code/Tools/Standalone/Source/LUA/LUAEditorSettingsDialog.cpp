/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Script/ScriptAsset.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <QtWidgets/QVBoxLayout>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzToolsFramework/UI/LegacyFramework/Core/EditorFrameworkAPI.h>

#include "LUAEditorSettingsDialog.hxx"
#include "LUAEditorMainWindow.hxx"
#include "LUAEditorContextMessages.h"
#include "LUAEditorBlockState.h"

#include <Source/LUA/ui_LUAEditorSettingsDialog.h>

namespace
{
}

namespace LUAEditor
{
    extern AZ::Uuid ContextID;

    LUAEditorSettingsDialog::LUAEditorSettingsDialog(QWidget* parent)
        : QDialog(parent)
    {
        m_gui = azcreate(Ui::LUAEditorSettingsDialog, ());
        m_gui->setupUi(this);

        AZ::SerializeContext* context = nullptr;
        {
            EBUS_EVENT_RESULT(context, AZ::ComponentApplicationBus, GetSerializeContext);
            AZ_Assert(context, "We should have a valid context!");
        }


        AZStd::intrusive_ptr<SyntaxStyleSettings> syntaxStyleSettings = AZ::UserSettings::CreateFind<SyntaxStyleSettings>(AZ_CRC("LUA Editor Text Settings", 0xb6e15565), AZ::UserSettings::CT_GLOBAL);

        // Store a copy to revert if needed.
        m_originalSettings = *syntaxStyleSettings;

        m_gui->propertyEditor->Setup(context, nullptr, true, 420);
        m_gui->propertyEditor->AddInstance(syntaxStyleSettings.get(), syntaxStyleSettings->RTTI_GetType());

        m_gui->propertyEditor->setObjectName("m_gui->propertyEditor");
        m_gui->propertyEditor->setMinimumHeight(500);
        m_gui->propertyEditor->setMaximumHeight(1000);
        m_gui->propertyEditor->SetSavedStateKey(AZ_CRC("LuaIDE_SyntaxStyleSettings"));

        setModal(false);

        m_gui->propertyEditor->InvalidateAll();
        m_gui->propertyEditor->ExpandAll();

        m_gui->propertyEditor->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        layout->addWidget(m_gui->propertyEditor);
        layout->addWidget(m_gui->cancelButton);
        layout->addWidget(m_gui->saveButton);
        layout->addWidget(m_gui->saveCloseButton);

        setLayout(layout);
    }

    void LUAEditorSettingsDialog::OnSave()
    {
        EBUS_EVENT(AZ::UserSettingsComponentRequestBus, Save);

        EBUS_EVENT(LUAEditorMainWindowMessages::Bus, Repaint);
    }

    void LUAEditorSettingsDialog::OnSaveClose()
    {
        OnSave();
        close();
    }

    void LUAEditorSettingsDialog::OnCancel()
    {
        AZStd::intrusive_ptr<SyntaxStyleSettings> syntaxStyleSettings = AZ::UserSettings::CreateFind<SyntaxStyleSettings>(AZ_CRC("LUA Editor Text Settings", 0xb6e15565), AZ::UserSettings::CT_GLOBAL);

        // Revert the stored copy, no changes will be stored.
        *syntaxStyleSettings = m_originalSettings;

        EBUS_EVENT(LUAEditorMainWindowMessages::Bus, Repaint);

        close();
    }

    LUAEditorSettingsDialog::~LUAEditorSettingsDialog()
    {
        m_gui->propertyEditor->ClearInstances();
        azdestroy(m_gui);
    }

    void LUAEditorSettingsDialog::keyPressEvent(QKeyEvent* event)
    {
        if (event->key() == Qt::Key_Escape)
        {
            OnCancel();
            return;
        }
        else
        if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
        {
            OnSaveClose();
            return;
        }
    }
}//namespace LUAEditor

#include <Source/LUA/moc_LUAEditorSettingsDialog.cpp>
