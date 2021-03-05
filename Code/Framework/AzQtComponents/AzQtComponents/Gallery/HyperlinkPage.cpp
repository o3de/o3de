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

#include "HyperlinkPage.h"
#include <Gallery/ui_HyperlinkPage.h>

#include <QLabel>
#include <AzQtComponents/Components/Style.h>

HyperlinkPage::HyperlinkPage(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::HyperlinkPage)
{
    ui->setupUi(this);

    QString exampleText = R"(

QLabel docs: <a href="http://doc.qt.io/qt-5/qlabel.html">http://doc.qt.io/qt-5/qlabel.html</a><br/>

<pre>
#include &lt;QLabel&gt;

QLabel* label;

// Assuming you've created a QLabel already (either in code or via .ui file):

// To add a hyperlink, use the &lt;a&gt; HTML tag in the text
label->setText("&lt;a href=\"#\"&gt;This is a hyperlink.&lt;/a&gt;");

// Note: Hyperlinks on this page are all set to # and are not meant to trigger a browser to open a webpage.

/*
  Note that QLabel text display has two modes: Qt::PlainText and Qt::RichText.
  By default, it auto-selects the mode based on the text. If the text has HTML in it
  then it'll use Qt::RichText. You need to use Qt::RichText to support hyperlinks.
  Hyperlinks can be used wherever Qt supports drawing rich text.
  Some widgets (such as QTextEdit) support RichText but cannot provide any useful
  way to interact with hyperlinks.
*/

</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

#include <Gallery/moc_HyperlinkPage.cpp>
