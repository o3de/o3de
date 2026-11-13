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

#include <QVBoxLayout>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzToolsFramework/UI/LegacyFramework/Core/EditorFrameworkAPI.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/Elements/Foundations.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/StyleManager.h>

#include "LUAEditorSettingsDialog.hxx"
#include "LUAEditorMainWindow.hxx"
#include "LUAEditorContextMessages.h"
#include "LUAEditorBlockState.h"

#include <Source/LUA/ui_LUAEditorSettingsDialog.h>

#include <QLabel>
#include <QPushButton>

namespace
{
}

namespace LUAEditor
{
    extern AZ::Uuid ContextID;

    // ----------------------------------------------------------------------------------
    // Flattened GUI: the size of the property editor's labels
    //
    // LUAEditorSettingsDialog.ui gave the property editor a sheet of one rule, `QLabel{font-size: 13px;}`. A rule without a name
    // reaches every label of its class below the widget that has the sheet, so it sized the name label of each property row, the
    // default label of a container and the label ElidingLabel measures its hint with (and a label with a class tag too: an own
    // sheet outranks the tiers of Text.qss). Everything else of those labels came from the application sheet: the family and the
    // weight of the label rule (Text.qss:91, BaseStyleSheet.qss:19-20), and the boxes of a property row (ReflectedPropertyEditor.qss,
    // carried by the Global box elements of PropertyEditorElements). The legacy GUI sets the sheet after setupUi; the flattened GUI
    // sets none, and this element sizes the labels.
    //
    // The labels of the colour picker a colour row opens are reached too, as the sheet reached them. The picker is a StyledDialog,
    // which makes its WindowDecorationWrapper with the colour row as parent (StyledDialog.cpp:17, PropertyColorCtrl.cpp:190), and
    // WindowDecorationWrapper::setGuest re-parents the picker to that same wrapper (:165): the editor stays in its chain for the
    // picker's whole life, and a child window inherits the sheet. The wrapper's title bar label is sized the same way. The picker's
    // labels have this element's family and weight (no tier tag; the palette cards, all of them PaletteCardBase, keep the normal
    // weight of ColorPicker.qss:58). The one that stays at 12px is the title of a modified palette (orange): its Global element
    // (PickerModifiedPaletteTitle) is registered first and wins, an open item of the picker's own elements.
    //
    // Not reached, as the sheet did not reach it: a tooltip (the sheet style names it QToolTip, not QLabel; it is a window with no
    // parent).
    // ----------------------------------------------------------------------------------

    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins files into one translation
    //! unit, where anonymous namespaces merge.
    namespace LUAEditorSettingsDialogFlatInternal
    {
        //! The sheet the form gave the property editor (LUAEditorSettingsDialog.ui:32, the form before the flattening), which the
        //! legacy GUI sets
        constexpr const char* PropertyEditorSheet = "QLabel{font-size: 13px;}\n";

        //! font-size: LUAEditorSettingsDialog.ui:32 (the form before the flattening). TOKENIZE: SettingsLabelSize
        constexpr int LabelSize = 13;

        //! `QLabel{font-size: 13px;}` over the label rule's family (Text.qss:91 `QLabel`) and the weight of the '*' rule
        //! (BaseStyleSheet.qss:20). No label below this editor has a class tag of Text.qss, whose tiers would give another weight.
        const AzQtComponents::TextElement PropertyEditorLabel{
            .m_name = "LuaIDE.Text.SettingsPropertyEditorLabel",
            .m_family = AzQtComponents::Foundations::Label::Family,
            .m_pixelSize = LabelSize,
        };

        //! A label below the property editor of a settings dialog (`QLabel` in a sheet of the editor's own), the labels of a colour
        //! picker a row opens included (see above). The dialog has the one editor, and the editor is the one widget of the dialog that
        //! holds labels.
        bool isPropertyEditorLabel(const QWidget* widget)
        {
            using AzQtComponents::Selectors::withinClass;
            return withinClass(widget, "AzToolsFramework::ReflectedPropertyEditor")
                && withinClass(widget, "LUAEditor::LUAEditorSettingsDialog");
        }
    } // namespace LUAEditorSettingsDialogFlatInternal

    void LUAEditorSettingsDialog::RegisterFlatElements()
    {
        static bool registered = false;
        if (!registered)
        {
            registered = true;
            namespace Flat = LUAEditorSettingsDialogFlatInternal;
            AzQtComponents::TextElement::registerElement(
                &Flat::PropertyEditorLabel, &QLabel::staticMetaObject, &Flat::isPropertyEditorLabel);
        }
    }

    LUAEditorSettingsDialog::LUAEditorSettingsDialog(QWidget* parent)
        : QDialog(parent)
    {
        m_gui = azcreate(Ui::LUAEditorSettingsDialog, ());
        m_gui->setupUi(this);

        // The property editor's sheet from the form, for the legacy GUI only. The flattened GUI sets none, and its text element sizes
        // the labels (RegisterFlatElements).
        using AzQtComponents::StyleManager;
        StyleManager::setLegacyStyleSheet(
            m_gui->propertyEditor, QString::fromLatin1(LUAEditorSettingsDialogFlatInternal::PropertyEditorSheet));
        if (StyleManager::stylesheetsDisabled())
        {
            RegisterFlatElements();
        }

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
        AZStd::intrusive_ptr<SyntaxStyleSettings> syntaxStyleSettings = AZ::UserSettings::CreateFind<SyntaxStyleSettings>(AZ_CRC_CE("LUA Editor Text Settings"), AZ::UserSettings::CT_GLOBAL);

        m_originalSettings = *syntaxStyleSettings;

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

        if (*syntaxStyleSettings != m_originalSettings)
        {
            *syntaxStyleSettings = m_originalSettings;
            LUAEditorMainWindowMessages::Bus::Broadcast(&LUAEditorMainWindowMessages::Bus::Events::Repaint);
        }

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

