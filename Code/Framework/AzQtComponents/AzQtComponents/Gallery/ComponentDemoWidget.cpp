/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ComponentDemoWidget.h"
#include "AzQtComponents/Gallery/ui_ComponentDemoWidget.h"
#include "AssetBrowserFolderPage.h"
#include "BreadCrumbsPage.h"
#include "BrowseEditPage.h"
#include "ButtonPage.h"
#include "CardPage.h"
#include "CheckBoxPage.h"
#include "ColorLabelPage.h"
#include "ColorPickerPage.h"
#include "ComboBoxPage.h"
#include "DragAndDropPage.h"
#include "FilteredSearchWidgetPage.h"
#include "GradientSliderPage.h"
#include "HyperlinkPage.h"
#include "LineEditPage.h"
#include "MenuPage.h"
#include "ProgressIndicatorPage.h"
#include "RadioButtonPage.h"
#include "ReflectedPropertyEditorPage.h"
#include "ScrollBarPage.h"
#include "SegmentControlPage.h"
#include "SliderComboPage.h"
#include "SliderPage.h"
#include "SplitterPage.h"
#include "SpinBoxPage.h"
#include "StyledDockWidgetPage.h"
#include "StyleSheetPage.h"
#include "SvgLabelPage.h"
#include "TableViewPage.h"
#include "TabWidgetPage.h"
#include "ToggleSwitchPage.h"
#include "ToolBarPage.h"
#include "TreeViewPage.h"
#include "TypographyPage.h"

#include <QAction>
#include <QMap>
#include <QMenu>
#include <QMenuBar>
#include <QSettings>

#include <cctype>

const QString g_pageIndexSettingKey = QStringLiteral("ComponentDemoWidgetPage");

ComponentDemoWidget::ComponentDemoWidget(QWidget* parent)
: QMainWindow(parent)
, ui(new Ui::ComponentDemoWidget)
{
    ui->setupUi(this);

    setupMenuBar();

    QMap<QString, QWidget*> sortedPages;

    sortedPages.insert("Breadcrumbs", new BreadCrumbsPage(this));
    sortedPages.insert("Browse Edit", new BrowseEditPage(this));
    sortedPages.insert("Button", new ButtonPage(this));
    sortedPages.insert("Card", new CardPage(this));
    sortedPages.insert("Checkbox", new CheckBoxPage(this));
    sortedPages.insert("Color Label", new ColorLabelPage(this));
    sortedPages.insert("Color Picker", new ColorPickerPage(this));
    sortedPages.insert("Combo Box", new ComboBoxPage(this));
    sortedPages.insert("Drag and Drop", new DragAndDropPage(this));
    sortedPages.insert("Filtered Search Widget", new FilteredSearchWidgetPage(this));
    sortedPages.insert("Gradient Slider", new GradientSliderPage(this));
    sortedPages.insert("Hyperlink", new HyperlinkPage(this));
    sortedPages.insert("Line Edit", new LineEditPage(this));
    sortedPages.insert("Menu", new MenuPage(this));
    sortedPages.insert("Progress Indicator", new ProgressIndicatorPage(this));
    sortedPages.insert("Radio Button", new RadioButtonPage(this));
    sortedPages.insert("Reflected Property Editor", new ReflectedPropertyEditorPage(this));
    sortedPages.insert("Scrollbar", new ScrollBarPage(this));
    sortedPages.insert("Segment Control", new SegmentControlPage(this));
    sortedPages.insert("Slider", new SliderPage(this));
    sortedPages.insert("Slider Combo", new SliderComboPage(this));

    SpinBoxPage* spinBoxPage = new SpinBoxPage(this);
    sortedPages.insert("Spin Box", spinBoxPage);

    sortedPages.insert("Splitter", new SplitterPage(this));
    sortedPages.insert("Styled Dock Widget", new StyledDockWidgetPage(this));
    sortedPages.insert("Stylesheet", new StyleSheetPage(this));
    sortedPages.insert("SVG Label", new SvgLabelPage(this));
    sortedPages.insert("Tab Widget", new TabWidgetPage(this));
    sortedPages.insert("Table View", new TableViewPage(this));
    sortedPages.insert("Toggle Switch", new ToggleSwitchPage(this));
    sortedPages.insert("Toolbar", new ToolBarPage(this));
    sortedPages.insert("Tree View", new TreeViewPage(this));
    sortedPages.insert("Typography", new TypographyPage(this));

    sortedPages.insert("AssetBrowserFolder", new AssetBrowserFolderPage(this));

    for (const auto& title : sortedPages.keys())
    {
        addPage(sortedPages.value(title), title);
    }

    connect(ui->demoSelector, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), ui->demoWidgetStack, [this, spinBoxPage](int newIndex) {
        QString demoSelectorText = ui->demoSelector->currentText();
        ui->demoWidgetStack->setCurrentIndex(newIndex);

        QSettings settings;
        settings.setValue(g_pageIndexSettingKey, demoSelectorText);

        m_editMenu->clear();
        if (ui->demoWidgetStack->currentWidget() == spinBoxPage)
        {
            QAction* undo = spinBoxPage->getUndoStack().createUndoAction(m_editMenu);
            undo->setShortcut(QKeySequence::Undo);
            m_editMenu->addAction(undo);

            QAction* redo = spinBoxPage->getUndoStack().createRedoAction(m_editMenu);
            redo->setShortcut(QKeySequence::Redo);
            m_editMenu->addAction(redo);
        }
        else
        {
            createEditMenuPlaceholders();
        }
    });

    QSettings settings;
    QString valueString = settings.value(g_pageIndexSettingKey, 0).toString();
    bool convertedToInt = false;
    int savedIndex = valueString.toInt(&convertedToInt);
    if (!convertedToInt)
    {
        savedIndex = ui->demoSelector->findText(valueString);
    }

    ui->demoSelector->setCurrentIndex(savedIndex);
}

ComponentDemoWidget::~ComponentDemoWidget()
{
}

void ComponentDemoWidget::addPage(QWidget* widget, const QString& title)
{
    ui->demoSelector->addItem(title);
    ui->demoWidgetStack->addWidget(widget);
}

void ComponentDemoWidget::setupMenuBar()
{
    auto fileMenu = menuBar()->addMenu("&File");

    QAction* refreshAction = fileMenu->addAction("Refresh Stylesheet");
    QObject::connect(refreshAction, &QAction::triggered, this, &ComponentDemoWidget::refreshStyle);
    fileMenu->addSeparator();

    m_editMenu = menuBar()->addMenu("&Edit");
    createEditMenuPlaceholders();

#if defined(Q_OS_MACOS)
    QAction* quitAction = fileMenu->addAction("&Quit");
#else
    QAction* quitAction = fileMenu->addAction("E&xit");
#endif
    quitAction->setShortcut(QKeySequence::Quit);
    QObject::connect(quitAction, &QAction::triggered, this, &QMainWindow::close);
}

void ComponentDemoWidget::createEditMenuPlaceholders()
{
    QAction* undo = m_editMenu->addAction("&Undo");
    undo->setDisabled(true);
    undo->setShortcut(QKeySequence::Undo);

    QAction* redo = m_editMenu->addAction("&Redo");
    redo->setDisabled(true);
    redo->setShortcut(QKeySequence::Redo);
}

#include "Gallery/moc_ComponentDemoWidget.cpp"
