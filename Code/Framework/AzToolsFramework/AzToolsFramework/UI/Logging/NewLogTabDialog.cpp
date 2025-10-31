/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzToolsFramework/UI/LegacyFramework/Core/EditorFrameworkAPI.h>

#include "NewLogTabDialog.h"
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <AzToolsFramework/UI/Logging/ui_NewLogTabDialog.h>
AZ_POP_DISABLE_WARNING
#include <QPushButton>
#include <QLineEdit>
#include <QDialogButtonBox>

namespace AzToolsFramework
{
    namespace LogPanel
    {
        NewLogTabDialog::NewLogTabDialog(QWidget* pParent)
            : QDialog(pParent)
            , uiManager(new Ui::newLogDialog)
        {
            uiManager->setupUi(this);

            // hardwired into the incoming message filter, "All" will take any source's message
            uiManager->windowNameBox->addItem("All");

            AZStd::vector<AZStd::string> knownWindows;
            LegacyFramework::LogComponentAPI::Bus::Broadcast(&LegacyFramework::LogComponentAPI::Bus::Events::EnumWindowTypes, knownWindows);

            if (knownWindows.size() > 0)
            {
                for (AZStd::vector<AZStd::string>::iterator it = knownWindows.begin(); it != knownWindows.end(); ++it)
                {
                    uiManager->windowNameBox->addItem(it->c_str());
                }
            }
            else
            {
                uiManager->windowNameBox->hide();
                uiManager->windowNameBoxLabel->hide();
            }

            QPushButton* ok = uiManager->buttonBox->button(QDialogButtonBox::Ok);
            ok->setEnabled(false);
            connect(uiManager->tabNameEdit, &QLineEdit::textChanged, this, [this, ok]() {
                QString text = uiManager->tabNameEdit->text();
                bool okEnabled = text.length() > 0;
                ok->setEnabled(okEnabled);

                if (!okEnabled)
                {
                    ok->setToolTip("Please enter a valid tab name before saving");
                }
                else
                {
                    ok->setToolTip("");
                }
            });
        }

        NewLogTabDialog::~NewLogTabDialog()
        {
            // defined in here so that ui_NewLogTabDialog.h doesn't have to be included in the header file
        }

        void NewLogTabDialog::attemptAccept()
        {
            m_windowName = uiManager->windowNameBox->currentText();
            m_tabName = uiManager->tabNameEdit->text();
            m_textFilter = uiManager->filterEdit->text();
            m_checkNormal = uiManager->checkNormal->isChecked();
            m_checkWarning = uiManager->checkWarning->isChecked();
            m_checkError = uiManager->checkError->isChecked();
            m_checkDebug = uiManager->checkDebug->isChecked();

            if (m_tabName.isEmpty())
            {
                uiManager->tabNameEdit->setFocus();
                QApplication::beep();
                return;
            }

            emit accept();
        }

    } // namespace LogPanel
} // namespace AzToolsFramework

#include "UI/Logging/moc_NewLogTabDialog.cpp"
