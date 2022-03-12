/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "NewGraphDialog.h"

#include <QLineEdit>
#include <QPushButton>

#include "Editor/View/Dialogs/ui_NewGraphDialog.h"



namespace ScriptCanvasEditor
{
    NewGraphDialog::NewGraphDialog(const QString& title, const QString& text, QWidget* pParent /*=nullptr*/)
        : QDialog(pParent)
        , ui(new Ui::NewGraphDialog)
        , m_text(text)
    {
        ui->setupUi(this);

        setWindowTitle(title);

        QObject::connect(ui->GraphName, &QLineEdit::returnPressed, this, &NewGraphDialog::OnOK);
        QObject::connect(ui->GraphName, &QLineEdit::textChanged, this, &NewGraphDialog::OnTextChanged);
        QObject::connect(ui->ok, &QPushButton::clicked, this, &NewGraphDialog::OnOK);
        QObject::connect(ui->cancel, &QPushButton::clicked, this, &QDialog::reject);

        ui->ok->setEnabled(false);
    }

    void NewGraphDialog::OnTextChanged(const QString& text)
    {
        ui->ok->setEnabled(!text.isEmpty());
    }

    void NewGraphDialog::OnOK()
    {
        QString itemName = ui->GraphName->text();
        m_text = itemName.toLocal8Bit().constData();

        accept();
    }

    #include <Editor/View/Dialogs/moc_NewGraphDialog.cpp>
}
