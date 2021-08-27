/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CheckBoxPage.h"
#include <AzQtComponents/Gallery/ui_CheckBoxPage.h>

#include <AzQtComponents/Components/Widgets/CheckBox.h>

#include <QMenu>
#include <QCheckBox>

CheckBoxPage::CheckBoxPage(QWidget* parent)
: QWidget(parent)
, ui(new Ui::CheckBoxPage)
{
    ui->setupUi(this);

    // set the state for tri-state; can't do it in Qt Designer
    ui->partiallySelectedCheckBox->setCheckState(Qt::PartiallyChecked);
    ui->partiallySelectedCheckBoxDisabled->setCheckState(Qt::PartiallyChecked);

    QString exampleText = R"(

QCheckBox docs: <a href="http://doc.qt.io/qt-5/qcheckbox.html">http://doc.qt.io/qt-5/qcheckbox.html</a><br/>

<pre>
#include &lt;QCheckBox&gt;

QCheckBox* checkBox;

// Assuming you've created a QCheckBox already (either in code or via .ui file):

// To set a checkBox to checked, do one of the following:
checkBox->setCheckState(Qt::Checked);
checkBox->setChecked(true);

// To set a checkbox to unchecked, do one of the following:
checkBox->setCheckState(Qt::Unchecked);
checkBox->setChecked(false);

// To turn a checkBox into a tri-state checkbox, so it can be on, off or partial:
checkBox->setTristate(true);

// To set a checkBox to partially on:
checkBox->setCheckState(Qt::PartiallyChecked);

// To disable the checkBox
checkBox->setEnabled(false);

// Note that checkBoxes can be enabled/disabled and set to checked/unchecked from Qt Designer or Creator.
// Whether or not a checkBox can be tri-state can be set from those tools, but the tools unfortunately
// do not allow you to actually set the state to PartiallyChecked.

</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

CheckBoxPage::~CheckBoxPage()
{
}

#include "Gallery/moc_CheckBoxPage.cpp"
