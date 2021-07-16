/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StyleSheetPage.h"
#include <AzQtComponents/Gallery/ui_StyleSheetPage.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

#include <AzQtComponents/Components/StyleHelpers.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QDir>
namespace Example
{
    Widget::Widget(QWidget* parent)
        : QWidget(parent)
        , m_drawSimple(false)
        , m_tearEnabled(false)
    {
        // Call StyleHelpers::repolishWhenPropertyChanges to ensure the widget is repolished when
        // drawSimple changes.
        AzQtComponents::StyleHelpers::repolishWhenPropertyChanges(this, &Widget::drawSimpleChanged);
        AzQtComponents::StyleHelpers::repolishWhenPropertyChanges(this, &Widget::tearEnabledChanged);

        auto layout = new QHBoxLayout(this);
        auto label = new QLabel(this);
        label->setObjectName("icon");
        layout->addWidget(label);
    }

    Widget::~Widget()
    {

    }

    void Widget::setDrawSimple(bool drawSimple)
    {
        if (drawSimple == m_drawSimple)
        {
            return;
        }

        m_drawSimple = drawSimple;
        emit drawSimpleChanged(m_drawSimple);
    }

    void Widget::setTearEnabled(bool tearEnabled)
    {
        if (tearEnabled == m_tearEnabled)
        {
            return;
        }

        m_tearEnabled = tearEnabled;
        emit tearEnabledChanged(m_tearEnabled);
    }
}

StyleSheetPage::StyleSheetPage(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::StyleSheetPage)
{
    ui->setupUi(this);
    ui->qrcLabel->setText("Styled using qrc: \":/Gallery/StyleSheetPage.qss\"");
    ui->absolutePathLabel->setText("Style using: \"<path/to>/StyleSheetPage.qss\"");
    ui->noPrefixLabel->setText("Styled without a prefix: \"StyleSheetPage.qss\"");
    ui->exampleLabel->setText("Styled using a prefix: \"gallery:StyleSheetPage.qss\"");

    // Add demo tabs to segmentBar
    for (int i = 0; i < 4; i++)
    {
        ui->segmentBar->addTab(QStringLiteral("Segment Bar %1").arg(i));
    }

    // Add demo filters to filteredSearchWidget
    const auto fruitList = {"Apple", "Orange", "Pear", "Banana"};
    const auto category = tr("Fruit");
    for (const auto fruit : fruitList)
    {
        ui->filteredSearchWidget->AddTypeFilter(category, fruit);
    }
    ui->filteredSearchWidget->SetFilterState(0, true);
    ui->filteredSearchWidget->SetFilterState(1, true);

    // pathOnDisk is specified relative to the engine root directory
    const auto pathOnDisk = QStringLiteral("Code/Framework/AzQtComponents/AzQtComponents/Gallery");
    const auto qrcPath = QStringLiteral(":/Gallery");

    // Setting the style sheet using both methods allows faster style iteration speed for
    // developers. The style will be loaded from a Qt Resource file if Editor is installed, but
    // developers with the file on disk will be able to modify the style and have it automatically
    // reloaded.
    AZ::IO::FixedMaxPath engineRootPath;
    if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
    {
        settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
    }
    AzQtComponents::StyleManager::addSearchPaths("gallery", pathOnDisk, qrcPath, engineRootPath);

    // The following label shows the intended use:
    AzQtComponents::StyleManager::setStyleSheet(ui->exampleWidget, "gallery:StyleSheetPage.qss");

    // The following labels have their style sheet set specifically for testing.

    // Style sheets can be set on a widget from disk or Qt Resource files.
    AzQtComponents::StyleManager::setStyleSheet(ui->qrcLabel, ":/Gallery/StyleSheetPage.qss");
    AzQtComponents::StyleManager::setStyleSheet(ui->absolutePathLabel, pathOnDisk + QDir::separator() + "StyleSheetPage.qss");

    // Style sheets without a prefix will still be loaded if they are found in a search path:
    AzQtComponents::StyleManager::setStyleSheet(ui->noPrefixLabel, "StyleSheetPage.qss");

    // Setup Example::Widget
    connect(ui->drawSimpleToggle, &QPushButton::clicked, ui->widget, &Example::Widget::setDrawSimple);
    connect(ui->tearEnabledToggle, &QPushButton::clicked, ui->widget, &Example::Widget::setTearEnabled);
    connect(ui->drawSimpleToggle, &QPushButton::clicked, ui->titleBar, &AzQtComponents::TitleBar::setDrawSimple);
    connect(ui->tearEnabledToggle, &QPushButton::clicked, ui->titleBar, &AzQtComponents::TitleBar::setTearEnabled);
    QFile styleSheetFile("gallery:ExampleWidget.qss");
    if (styleSheetFile.open(QFile::ReadOnly))
    {
        // QWidget::setStyleSheet should NOT be used to set stylesheets on widgets, as they will
        // survive the switch between UI 1.0 and UI 2.0. However, for this example/test case that is
        // exactly what is needed. Note that this style sheet will not be automatically reloaded if
        // the file is changed.
        ui->widget->setStyleSheet(styleSheetFile.readAll());
    }

    QString exampleText = R"(
<p>Qt Style Sheet docs:
<a href="http://doc.qt.io/qt-5/stylesheet.html">http://doc.qt.io/qt-5/stylesheet.html</a></p>

<p>AzQtComponents::StyleManager::addSearchPaths must be used to specify the prefix of the style
sheet and the paths to search for the prefix.</p>

<p>StyleManager::setStyleSheet can then be used to add a style sheet to a particular widget.</p>

Say a .qrc file exists with the following entry:
<pre>
&lt;qresource prefix="/Gallery"&gt;
    &lt;file&gt;StyleSheetLabel.qss&lt;/file&gt;
    &lt;file&gt;StyleSheetPage.qss&lt;/file&gt;
    &lt;file&gt;StyleSheetView.qss&lt;/file&gt;
&lt;/qresource&gt;
</pre>

StyleSheetPage.qss simply imports the other style sheets:
<pre>
@import "StyleSheetView.qss";
@import "StyleSheetLabel.qss";
</pre>

StyleSheetCache extends the Qt Style Sheet format by including a simple preprocessor that
allows other style sheets to be imported.
<pre>
#include &lt;AzQtComponents/Components/StyleManager.h&gt;

// To add a search prefix:
// pathOnDisk is specified relative to the engine root directory
const auto pathOnDisk = QStringLiteral("Code/Framework/AzQtComponents/AzQtComponents/Gallery");
const auto qrcPath = QStringLiteral(":/Gallery");

// Specifying the path to the file on disk and the qrc prefix of the file in this order means
// that the style will be loaded from disk if it exists, otherwise the style in the Qt Resource
// file will be used. Styles loaded from disk will be automatically watched and re-applied if
// changes are detected, allowing much faster style iteration.
AzQtComponents::StyleManager::addSearchPaths("gallery", pathOnDisk, qrcPath);

// To add a style to a widget:
auto widget = new QWidget();
AzQtComponents::StyleManager::setStyleSheet(widget, "gallery:StyleSheetPage.qss");
</pre>
)";

    ui->exampleText->setHtml(exampleText);
}

StyleSheetPage::~StyleSheetPage()
{
}

#include "Gallery/moc_StyleSheetPage.cpp"
