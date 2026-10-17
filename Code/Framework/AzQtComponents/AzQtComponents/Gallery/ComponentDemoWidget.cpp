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

#include <AzQtComponents/Components/Widgets/Card.h>

#include <QAction>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QDockWidget>
#include <QMap>
#include <QMenu>
#include <QMenuBar>
#include <QPixmap>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QTreeView>

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

void ComponentDemoWidget::captureAllPages(const QString& outputDirPath)
{
    QDir().mkpath(outputDirPath);

    for (int pageIndex = 0; pageIndex < ui->demoSelector->count(); ++pageIndex)
    {
        ui->demoSelector->setCurrentIndex(pageIndex);

        // let polish, layout and paint settle before grabbing
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();

        QString pageName = ui->demoSelector->itemText(pageIndex).toLower();
        for (int charIndex = 0; charIndex < pageName.size(); ++charIndex)
        {
            if (!pageName.at(charIndex).isLetterOrNumber())
            {
                pageName[charIndex] = QLatin1Char('-');
            }
        }

        const QPixmap pageGrab = window()->grab();
        pageGrab.save(QStringLiteral("%1/%2_%3.png")
                          .arg(outputDirPath)
                          .arg(pageIndex, 2, 10, QLatin1Char('0'))
                          .arg(pageName));

        capturePageInteractions(outputDirPath, pageIndex, pageName);
    }
}

void ComponentDemoWidget::selectPage(const QString& pageNameOrIndex)
{
    bool isNumber = false;
    const int numericIndex = pageNameOrIndex.toInt(&isNumber);
    if (isNumber && numericIndex >= 0 && numericIndex < ui->demoSelector->count())
    {
        ui->demoSelector->setCurrentIndex(numericIndex);
        return;
    }

    for (int pageIndex = 0; pageIndex < ui->demoSelector->count(); ++pageIndex)
    {
        if (ui->demoSelector->itemText(pageIndex).contains(pageNameOrIndex, Qt::CaseInsensitive))
        {
            ui->demoSelector->setCurrentIndex(pageIndex);
            return;
        }
    }
}

void ComponentDemoWidget::capturePageInteractions(const QString& outputDirPath, int pageIndex, const QString& pageName)
{
    QWidget* page = ui->demoWidgetStack->currentWidget();
    if (!page)
    {
        return;
    }

    const auto settle = []()
    {
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
    };
    const auto capturePath = [outputDirPath, pageIndex, pageName](const QString& suffix)
    {
        return QStringLiteral("%1/%2_%3__%4.png")
            .arg(outputDirPath)
            .arg(pageIndex, 2, 10, QLatin1Char('0'))
            .arg(pageName)
            .arg(suffix);
    };

    // Expanded state: open every tree and card so collapsed-only content is captured
    bool expandedAnything = false;
    for (QTreeView* treeView : page->findChildren<QTreeView*>())
    {
        treeView->expandAll();
        expandedAnything = true;
    }
    for (AzQtComponents::Card* card : page->findChildren<AzQtComponents::Card*>())
    {
        card->setExpanded(true);
        expandedAnything = true;
    }
    if (expandedAnything)
    {
        settle();
        window()->grab().save(capturePath(QStringLiteral("expanded")));
    }

    // Combo popups: open each visible combo on the page and grab the popup window
    int comboIndex = 0;
    for (QComboBox* combo : page->findChildren<QComboBox*>())
    {
        if (!combo->isVisible() || combo->count() == 0)
        {
            continue;
        }
        combo->showPopup();
        settle();
        if (QWidget* popup = QApplication::activePopupWidget())
        {
            popup->grab().save(capturePath(QStringLiteral("combo%1-popup").arg(comboIndex)));
        }
        combo->hidePopup();
        settle();
        if (++comboIndex >= 3)
        {
            break;
        }
    }

    // Modal dialog launchers (color picker etc.): a watchdog grabs and closes the
    // dialog while the button's exec() blocks
    int dialogIndex = 0;
    for (QPushButton* button : page->findChildren<QPushButton*>())
    {
        const QString buttonText = button->text().toLower();
        const bool opensDialog = buttonText.contains(QStringLiteral("picker"))
            || buttonText.contains(QStringLiteral("dialog"))
            || buttonText.contains(QStringLiteral("rgb"))
            || buttonText.contains(QStringLiteral("hsl"))
            || buttonText.contains(QStringLiteral("hsv"));
        if (!button->isVisible() || !opensDialog)
        {
            continue;
        }

        const QString dialogCapturePath = capturePath(QStringLiteral("dialog%1").arg(dialogIndex));
        QTimer::singleShot(900, this, [dialogCapturePath]()
        {
            if (QWidget* modal = QApplication::activeModalWidget())
            {
                modal->grab().save(dialogCapturePath);
                modal->close();
            }
        });
        button->click();
        settle();
        if (++dialogIndex >= 3)
        {
            break;
        }
    }

    // Dock widgets: float each one and grab its window, then re-dock
    int dockIndex = 0;
    for (QDockWidget* dockWidget : page->findChildren<QDockWidget*>())
    {
        if (!dockWidget->isVisible())
        {
            continue;
        }
        dockWidget->setFloating(true);
        settle();
        dockWidget->window()->grab().save(capturePath(QStringLiteral("dock%1-floating").arg(dockIndex)));
        dockWidget->setFloating(false);
        settle();
        if (++dockIndex >= 3)
        {
            break;
        }
    }
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

