/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BrowseEditPage.h"
#include <AzQtComponents/Gallery/ui_BrowseEditPage.h>

#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <QIntValidator>
#include <QInputDialog>
#include <QMessageBox>

BrowseEditPage::BrowseEditPage(QWidget* parent)
: QWidget(parent)
, ui(new Ui::BrowseEditPage)
{
    ui->setupUi(this);

    QIcon icon(":/stylesheet/img/question.png");

    ui->browseEditEnabled->setAttachedButtonIcon(icon);
    ui->browseEditEnabled->setLineEditReadOnly(true);
    ui->browseEditEnabled->setPlaceholderText("Some placeholder");
    connect(ui->browseEditEnabled, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, [this]() {
        QString text = QInputDialog::getText(this, "Text Entry", "Enter a number, or some invalid text");
        ui->browseEditEnabled->setText(text);
    });

    ui->browseEditReadOnlyClearButton->setLineEditReadOnly(true);
    ui->browseEditReadOnlyClearButton->setClearButtonEnabled(true);
    ui->browseEditReadOnlyClearButton->setText(QStringLiteral("Text to clear"));
    connect(ui->browseEditReadOnlyClearButton, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, [this]() {
        QString text = QInputDialog::getText(this, "Text Entry", "Enter some text");
        ui->browseEditReadOnlyClearButton->setText(text);
    });

    ui->browseEditClearButton->setClearButtonEnabled(true);

    ui->browseEditDisabled->setLineEditReadOnly(true);
    ui->browseEditDisabled->setEnabled(false);
    ui->browseEditDisabled->setAttachedButtonIcon(icon);
    ui->browseEditDisabled->setText("Text");
    connect(ui->browseEditDisabled, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, [this]() {
        QString text = QInputDialog::getText(this, "Text Entry", "Enter a number, or some invalid text");
        ui->browseEditDisabled->setText(text);
    });

    AzQtComponents::BrowseEdit::applyDropTargetStyle(ui->browseEditValidDropTarget, true);
    AzQtComponents::BrowseEdit::applyDropTargetStyle(ui->browseEditInvalidDropTarget, false);

    ui->browseEditNumberReadOnly->setLineEditReadOnly(true);
    ui->browseEditNumberReadOnly->setValidator(new QIntValidator(this));
    ui->browseEditNumberReadOnly->setAttachedButtonIcon(icon);
    ui->browseEditNumberReadOnly->setToolTip("Click the attached button to enter a number. Put random characters in to put the control into error state.");
    ui->browseEditNumberReadOnly->setErrorToolTip("The control only accepts numbers!");
    ui->browseEditNumberReadOnly->setText("Nan");
    connect(ui->browseEditNumberReadOnly, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, [this]() {
        // pop up a dialog box and set the line edit based on the user's input
        QString text = QInputDialog::getText(this, "Text Entry", "Enter a number, or some invalid text");
        ui->browseEditNumberReadOnly->setText(text);
    });

    ui->browseEditNumberNonReadOnly->setValidator(new QIntValidator(this));
    ui->browseEditNumberNonReadOnly->setAttachedButtonIcon(icon);
    ui->browseEditNumberNonReadOnly->setToolTip("Click the attached button to enter a number. Put random characters in to put the control into error state.");
    ui->browseEditNumberNonReadOnly->setErrorToolTip("The control only accepts numbers!");
    connect(ui->browseEditNumberNonReadOnly, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, [this]() {
        // pop up a dialog box and set the line edit based on the user's input
        QMessageBox::critical(this, "title", "Button attach worked!");
    });

    QString exampleText = R"(

A Browse Edit is a widget which adds a button to a Line Edit. It can be used to add additional helpers or dialogs to a Line Edit, for example providing simpler means of selecting a path.<br/>

<pre>
#include &lt;AzQtComponents/Components/Widgets/BrowseEdit.h&gt;

// Assuming you've created a BrowseEdit already (either in code or via .ui file):

// Change the Icon on the button
QIcon icon(":/stylesheet/img/question.png");
browseEdit->setAttachedButtonIcon(icon);

// Make the LineEdit read only, forcing the value to be changed via the button
browseEdit->setLineEditReadOnly(true);

// Set a placeholder text
browseEdit->setPlaceholderText("Some placeholder");

// Enable the clear button on the LineEdit
browseEdit->setClearButtonEnabled(true);

// Connect a behavior to the button - for example, a dialog that inserts the text
connect(browseEdit, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, [this]() {
    QString text = QInputDialog::getText(this, "Text Entry", "Enter a value");
    browseEdit->setText(text);
});
</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

BrowseEditPage::~BrowseEditPage()
{
}

#include "Gallery/moc_BrowseEditPage.cpp"
