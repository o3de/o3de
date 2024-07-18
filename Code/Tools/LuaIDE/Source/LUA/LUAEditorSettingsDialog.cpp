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

#include <QPushButton>

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
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(context, "We should have a valid context!");
        }


        AZStd::intrusive_ptr<SyntaxStyleSettings> syntaxStyleSettings = AZ::UserSettings::CreateFind<SyntaxStyleSettings>(AZ_CRC_CE("LUA Editor Text Settings"), AZ::UserSettings::CT_GLOBAL);

        // Store a copy to revert if needed.
        m_originalSettings = *syntaxStyleSettings;

        m_gui->propertyEditor->Setup(context, nullptr, true, 420);
        m_gui->propertyEditor->AddInstance(syntaxStyleSettings.get(), syntaxStyleSettings->RTTI_GetType());
        m_gui->propertyEditor->setObjectName("m_gui->propertyEditor");
        m_gui->propertyEditor->setMinimumHeight(500);
        m_gui->propertyEditor->setMaximumHeight(1000);
        m_gui->propertyEditor->SetSavedStateKey(AZ_CRC_CE("LuaIDE_SyntaxStyleSettings"));

        setModal(false);

        m_gui->propertyEditor->InvalidateAll();
        m_gui->propertyEditor->ExpandAll();
        m_gui->propertyEditor->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        auto* layout = new QVBoxLayout(this);
        layout->addWidget(m_gui->propertyEditor);

        auto* hlayout = new QHBoxLayout(this);
        hlayout->addWidget(m_gui->applyButton);
        hlayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Fixed));
        hlayout->addWidget(m_gui->saveButton);
        hlayout->addWidget(m_gui->saveCloseButton);
        hlayout->addWidget(m_gui->cancelButton);

        layout->addLayout(hlayout);

        QObject::connect(m_gui->saveButton, &QPushButton::clicked, this, &LUAEditorSettingsDialog::OnSave);
        QObject::connect(m_gui->saveCloseButton, &QPushButton::clicked, this, &LUAEditorSettingsDialog::OnSaveClose);
        QObject::connect(m_gui->cancelButton, &QPushButton::clicked, this, &LUAEditorSettingsDialog::OnCancel);
        QObject::connect(m_gui->applyButton, &QPushButton::clicked, this, &LUAEditorSettingsDialog::OnApply);

        setLayout(layout);
    }

    void LUAEditorSettingsDialog::OnSave()
    {
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequestBus::Events::Save);

        LUAEditorMainWindowMessages::Bus::Broadcast(&LUAEditorMainWindowMessages::Bus::Events::Repaint);
    }

    void LUAEditorSettingsDialog::OnSaveClose()
    {
        OnSave();
        close();
    }

    void LUAEditorSettingsDialog::OnCancel()
    {
        AZStd::intrusive_ptr<SyntaxStyleSettings> syntaxStyleSettings = AZ::UserSettings::CreateFind<SyntaxStyleSettings>(AZ_CRC_CE("LUA Editor Text Settings"), AZ::UserSettings::CT_GLOBAL);

        // Revert the stored copy, no changes will be stored.
        *syntaxStyleSettings = m_originalSettings;

        LUAEditorMainWindowMessages::Bus::Broadcast(&LUAEditorMainWindowMessages::Bus::Events::Repaint);

        close();
    }

    void LUAEditorSettingsDialog::OnApply()
    {
        LUAEditorMainWindowMessages::Bus::Broadcast(&LUAEditorMainWindowMessages::Bus::Events::Repaint);
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
        }
        else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
        {
            OnSaveClose();
        }
    }
}//namespace LUAEditor

#include <Source/LUA/moc_LUAEditorSettingsDialog.cpp>
