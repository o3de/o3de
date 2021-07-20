/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ToggleSwitchPage.h"
#include <AzQtComponents/Gallery/ui_ToggleSwitchPage.h>

#include <AzQtComponents/Components/Widgets/CheckBox.h>

#include <QCheckBox>

ToggleSwitchPage::ToggleSwitchPage(QWidget* parent)
: QWidget(parent)
, ui(new Ui::ToggleSwitchPage)
{
    ui->setupUi(this);

    QString exampleText = R"(

A ToggleSwitch is just a QCheckBox with a css class of "ToggleSwitch".<br/>
ToggleSwitches do not support the partial state that QCheckBoxes do, but other than that, they are functionally the same.<br/>
<br/>
QCheckBox docs: <a href="http://doc.qt.io/qt-5/qcheckbox.html">http://doc.qt.io/qt-5/qcheckbox.html</a><br/>

<pre>
#include &lt;QCheckBox&gt;
#include &lt;AzQtComponents/Components/Widgets/CheckBox.h&gt;

QCheckBox* toggleSwitch;

// Assuming you've created a QCheckBox already (either in code or via .ui file), make it into a ToggleSwitch like this:
CheckBox::applyToggleSwitchStyle(toggleSwitch);

// Disable it like this:
toggleSwitch->setEnabled(false);

// set the switch to the "on" state
toggleSwitch->setChecked(true);

// set the switch to the "off" state
toggleSwitch->setChecked(false);

</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

ToggleSwitchPage::~ToggleSwitchPage()
{
}

#include "Gallery/moc_ToggleSwitchPage.cpp"
