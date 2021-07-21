/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LineEditPage.h"
#include <AzQtComponents/Gallery/ui_LineEditPage.h>

#include <QValidator>
#include <QDoubleValidator>

#include <AzQtComponents/Components/Widgets/LineEdit.h>
#include <AzQtComponents/Components/Widgets/Text.h>

class ErrorValidator : public QValidator
{
public:
    ErrorValidator(QObject* parent) : QValidator(parent) {}

    QValidator::State validate([[maybe_unused]] QString& input, [[maybe_unused]] int& pos) const override
    {
        return QValidator::Invalid;
    }
};

LineEditPage::LineEditPage(QWidget* parent)
: QWidget(parent)
, ui(new Ui::LineEditPage)
{
    ui->setupUi(this);

    const auto titles = {ui->lineEditTitle, ui->searchBoxTitle, ui->examplesTitle};
    for (auto title : titles)
    {
        AzQtComponents::Text::addTitleStyle(title);
    }

    AzQtComponents::LineEdit::applyDropTargetStyle(ui->validDropTargetLineEdit, true);
    AzQtComponents::LineEdit::applyDropTargetStyle(ui->invalidDropTargetLineEdit, false);

    ui->withData->setToolTip("<b></b>This will be a very very long sentence with an end but not for a long time, spanning an entire screen. I said, spanning an entire screen. No really. An entire screen. A very long, very wide screen.");

    ui->error->setText("Error");
    ui->error->setValidator(new ErrorValidator(ui->error));

    auto validator = new QDoubleValidator(ui->longNumber);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setTop(4.0);
    validator->setBottom(3.0);
    ui->longNumber->setValidator(validator);
    AzQtComponents::LineEdit::setErrorMessage(ui->longNumber, QStringLiteral("Value must be between 3.0 and 4.0"));
    ui->longNumber->setClearButtonEnabled(true);

    const auto searchBoxes = {ui->searchBox, ui->emptySearchBox};
    for (auto searchBox : searchBoxes)
    {
        AzQtComponents::LineEdit::applySearchStyle(searchBox);
        searchBox->setPlaceholderText("Search...");
    }

    QString exampleText = R"(

QLineEdit docs: <a href="http://doc.qt.io/qt-5/qcombobox.html">http://doc.qt.io/qt-5/qlineedit.html</a><br/>

<pre>
#include &lt;AzQtComponents/Components/Widgets/LineEdit.h&gt;
#include &lt;QLineEdit&gt;
#include &lt;QDebug&gt;

QLineEdit* lineEdit;

// Assuming you've created a QLineEdit already (either in code or via .ui file):

// to set the text:
lineEdit->setText("Filled in data");

// to set hint text:
lineEdit->setPlaceholderText("Hint text");

// to disable line edits
lineEdit->setDisabled(true);

// to style a line edit with the search icon on the left:
AzQtComponents::LineEdit::applySearchStyle(ui->searchBox);

// to indicate errors, set a QValidator subclass on the QLineEdit object:
lineEdit->setValidator(new CustomValidator(lineEdit));

// to set the error message
AzQtComponents::LineEdit::setErrorMessage(lineEdit, QStringLiteral("Value must be between 3.0 and 4.0"));

// You may also set external error state on the line edit that is OR'ed with validators and mask
AzQtComponents::LineEdit::setExternalError(lineEdit, true);

// You can also use pre-made validators for QDoubleValidator, QIntValidator, QRegularExpressionValidator

// To listen for text changes:
connect(lineEdit, &QLineEdit::textChanged, this, [](const QString& newText){
    qDebug() &lt;&lt; QString("Text changed: %1").arg(newText);
});

connect(lineEdit, &QLineEdit::textEdited, this, [](const QString& newText){
    // only happens when the user changed the text, not when lineEdit->setText("some text"); is called
    qDebug() &lt;&lt; QString("Text edited: %1").arg(newText);
});

connect(lineEdit, &QLineEdit::editingFinished, this, [lineEdit](){
    // this happens when the focus leaves the line edit or the user hits the enter key
    qDebug() &lt;&lt; QString("Editing finished. New text: %1").arg(lineEdit->text());
});

// To show the line edit as a valid drop target:
AzQtComponents::LineEdit::applyDropTargetStyle(lineEdit, true);

// To show the line edit as an invalid drop target:
AzQtComponents::LineEdit::applyDropTargetStyle(lineEdit, false);

// To clear the drop target style:
AzQtComponents::LineEdit::removeDropTargetStyle(lineEdit);
</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

LineEditPage::~LineEditPage()
{
}

#include "Gallery/moc_LineEditPage.cpp"
