/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TypographyPage.h"
#include <AzQtComponents/Gallery/ui_TypographyPage.h>

#include <QLabel>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/Widgets/Text.h>

static QLabel* label(QGridLayout* colorGrid, int row, int column)
{
    return qobject_cast<QLabel*>(colorGrid->itemAtPosition(row, column)->widget());
}

TypographyPage::TypographyPage(QWidget* parent)
: QWidget(parent)
, ui(new Ui::TypographyPage)
{
    ui->setupUi(this);

    AzQtComponents::Text::addHeadlineStyle(ui->m_headlinePrimary);
    AzQtComponents::Text::addHeadlineStyle(ui->m_headlinePrimaryDescription);
    AzQtComponents::Text::addTitleStyle(ui->m_titlePrimary);
    AzQtComponents::Text::addTitleStyle(ui->m_titlePrimaryDescription);
    AzQtComponents::Text::addSubtitleStyle(ui->m_subTitlePrimary);
    AzQtComponents::Text::addSubtitleStyle(ui->m_subTitlePrimaryDescription);
    AzQtComponents::Text::addMenuStyle(ui->m_dataMenuPrimary);
    AzQtComponents::Text::addMenuStyle(ui->m_dataMenuPrimaryDescription);
    AzQtComponents::Text::addParagraphStyle(ui->m_paragraphPrimary);
    AzQtComponents::Text::addParagraphStyle(ui->m_paragraphPrimaryDescription);
    AzQtComponents::Text::addTooltipStyle(ui->m_tooltipPrimary);
    AzQtComponents::Text::addTooltipStyle(ui->m_tooltipPrimaryDescription);
    AzQtComponents::Text::addButtonStyle(ui->m_buttonPrimary);
    AzQtComponents::Text::addButtonStyle(ui->m_buttonPrimaryDescription);

    AzQtComponents::Text::addPrimaryStyle(ui->m_primaryColor);
    AzQtComponents::Text::addPrimaryStyle(ui->m_primaryColorDescription);
    AzQtComponents::Text::addSecondaryStyle(ui->m_secondaryColor);
    AzQtComponents::Text::addSecondaryStyle(ui->m_secondaryColorDescription);
    AzQtComponents::Text::addHighlightedStyle(ui->m_highlightedColor);
    AzQtComponents::Text::addHighlightedStyle(ui->m_highlightedColorDescription);
    AzQtComponents::Text::addBlackStyle(ui->m_blackColor);
    AzQtComponents::Text::addBlackStyle(ui->m_blackColorDescription);

    QGridLayout* colorGrid = ui->colorGrid;

    typedef void (*TextColorStyleCallback)(QLabel* QLabel);
    TextColorStyleCallback colorClassCallbacks[] = { &AzQtComponents::Text::addBlackStyle, &AzQtComponents::Text::addSecondaryStyle, &AzQtComponents::Text::addHighlightedStyle };
    for (int column = 0; column < 3; column++)
    {
        using namespace AzQtComponents;

        Text::addHeadlineStyle(label(colorGrid, 1, column));
        Text::addTitleStyle(label(colorGrid, 2, column));
        Text::addSubtitleStyle(label(colorGrid, 3, column));
        Text::addMenuStyle(label(colorGrid, 4, column));
        Text::addParagraphStyle(label(colorGrid, 6, column));
        Text::addTooltipStyle(label(colorGrid, 7, column));
        Text::addButtonStyle(label(colorGrid, 8, column));

        for (int row = 1; row < 9; row++)
        {
            colorClassCallbacks[column](label(colorGrid, row, column));
        }
    }

    QString exampleText = R"(

QLabel docs: <a href="http://doc.qt.io/qt-5/qlabel.html">http://doc.qt.io/qt-5/qlabel.html</a><br/>

<pre>
#include &lt;QLabel&gt;
#include &lt;AzQtComponents/Components/Widgets/Text.h&gt;

QLabel* label;

// Assuming you've created a QLabel already (either in code or via .ui file):

// To apply various styles, use one (and only one) of the following:
AzQtComponents::Text::addHeadlineStyle(label);
AzQtComponents::Text::addTitleStyle(label);
AzQtComponents::Text::addSubtitleStyle(label);
AzQtComponents::Text::addMenuStyle(label);
AzQtComponents::Text::addLabelStyle(label); // default style
AzQtComponents::Text::addParagraphStyle(label);
AzQtComponents::Text::addTooltipStyle(label);
AzQtComponents::Text::addButtonStyle(label);

// To set various colors, use one (and only one) of the following:
AzQtComponents::Text::addPrimaryStyle(label);
AzQtComponents::Text::addSecondaryStyle(label);
AzQtComponents::Text::addHighlightedStyle(label);
AzQtComponents::Text::addBlackStyle(label);

// One text style and one text color can be applied at the same time on the same label.

/*
  Note that QLabel text display has two modes: Qt::PlainText and Qt::RichText.
  By default, it auto-selects the mode based on the text. If the text has HTML in it
  then it'll use Qt::RichText.
  This matters because \n means newline in PlainText mode, but &lt;br&gt; means newline in
  RichText mode.
*/

</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

TypographyPage::~TypographyPage()
{
}

#include "Gallery/moc_TypographyPage.cpp"
