/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "ProgressIndicatorPage.h"
#include "Gallery/ui_ProgressIndicatorPage.h"

ProgressIndicatorPage::ProgressIndicatorPage(QWidget* parent)
: QWidget(parent)
, ui(new Ui::ProgressIndicatorPage)
{
    ui->setupUi(this);

    ui->spinner->SetIsBusy(true);
    ui->spinner->SetBusyIconSize(14);

    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(60);
    ui->progressBar->setTextVisible(false);

    QString exampleText = R"(

QProgressBar docs: <a href="http://doc.qt.io/qt-5/qprogressbar.html">http://doc.qt.io/qt-5/qprogressbar.html</a><br/>

<pre>
#include &lt;QProgressBar&gt;
#include &lt;QCoreApplication&gt;
#include &lt;QEventLoop&gt;

QProgressBar* progressBar;

// Assuming you've created a QProgressBar already (either in code or via .ui file):

// Set the integer range
progressBar->setRange(0, 100);

// Update the progress
progressBar->setValue(56);

// Hide the progress text, as specified by the UX spec - it defaults to on, so has to be turned off
// Note that it can also be set from the .ui file, from Qt Designer or Creator
progressBar->setTextVisible(false);

// An example of modal progress (meaning to update the progress bar without allowing the user to interact with anything:
int start = 0;
int end = 100;
progressBar->setRange(start, end);

for (int i = start; i &lt; end; i++)
{
    printf("doing some work");

    progressBar->setValue(i);

    // show the progress to the user by updating the Qt framework
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

ProgressIndicatorPage::~ProgressIndicatorPage()
{
}

#include "Gallery/moc_ProgressIndicatorPage.cpp"