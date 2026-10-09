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
#include <QCursor>
#include <QDebug>
#include <QDir>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QImage>
#include <QFontInfo>
#include <QMap>
#include <QMenu>
#include <QMenuBar>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QSplitter>
#include <QTextEdit>
#include <QTextStream>
#include <QTimer>
#include <QTreeView>

#include <cctype>
#include <memory>

namespace
{
    // Capture-time frame pinning (see ComponentDemoWidget::pinCaptureLayout).
    // Every gallery page is a QHBoxLayout [exampleText | demo]; the layout hands
    // the spare width out by sizeHint, which legitimately differs between the
    // qss and flattened styling modes and would otherwise shift every widget.
    constexpr int kCaptureWindowWidth = 1600;
    constexpr int kCaptureWindowHeight = 1100;
    constexpr int kCaptureExampleTextWidth = 480;

    // Modal-dialog watchdog: poll for the dialog instead of a single fixed delay,
    // so a slow dialog is still captured and a missing one cannot hang the run.
    constexpr int kDialogPollIntervalMs = 100;
    constexpr int kDialogPollAttempts = 60;

    // The styling of a page plays out over many event-loop turns (style sheet
    // polish, re-polish, deferred layout, timers). A capture is only taken after a
    // minimum dwell AND once two consecutive grabs are identical, so the recorded
    // frame is the final user-side view, never an intermediate state.
    constexpr int kSettleMinDwellMs = 3000;
    constexpr int kSettleProbeIntervalMs = 500;
    constexpr int kSettleMaxMs = 10000;

    void pumpEventsFor(int milliseconds)
    {
        QEventLoop loop;
        QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
        loop.exec();
    }
}

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

    // Same frame in every styling mode and on every machine.
    window()->showNormal();
    window()->resize(kCaptureWindowWidth, kCaptureWindowHeight);
    // Park the pointer outside the window so no widget captures a hover state.
    if (const QScreen* screen = window()->screen())
    {
        const QRect screenRect = screen->geometry();
        QCursor::setPos(screenRect.right() - 1, screenRect.bottom() - 1);
    }
    QCoreApplication::processEvents();
    QCoreApplication::processEvents();

    for (int pageIndex = 0; pageIndex < ui->demoSelector->count(); ++pageIndex)
    {
        ui->demoSelector->setCurrentIndex(pageIndex);
        pinCaptureLayout(ui->demoWidgetStack->currentWidget());

        // Deterministic state for every base capture: no popup left open by a
        // previous interaction (its list view would keep keyboard focus), the main
        // window active, the page selector focused.
        for (int attempt = 0; attempt < 8; ++attempt)
        {
            QWidget* popup = QApplication::activePopupWidget();
            if (!popup)
            {
                break;
            }
            popup->close();
            QCoreApplication::processEvents();
        }
        window()->activateWindow();
        window()->raise();
        ui->demoSelector->setFocus(Qt::OtherFocusReason);

        // Let the whole styling chain play out before recording (see kSettle*).
        settleForCapture();

        QString pageName = ui->demoSelector->itemText(pageIndex).toLower();
        for (int charIndex = 0; charIndex < pageName.size(); ++charIndex)
        {
            if (!pageName.at(charIndex).isLetterOrNumber())
            {
                pageName[charIndex] = QLatin1Char('-');
            }
        }

        const QString baseName = QStringLiteral("%1/%2_%3")
                                     .arg(outputDirPath)
                                     .arg(pageIndex, 2, 10, QLatin1Char('0'))
                                     .arg(pageName);

        const QPixmap pageGrab = window()->grab();
        pageGrab.save(baseName + QStringLiteral(".png"));
        dumpPageGeometry(ui->demoWidgetStack->currentWidget(), baseName + QStringLiteral("__geometry.txt"));

        capturePageInteractions(outputDirPath, pageIndex, pageName);
    }
}

void ComponentDemoWidget::settleForCapture()
{
    pumpEventsFor(kSettleMinDwellMs);

    QImage previous = window()->grab().toImage();
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < kSettleMaxMs)
    {
        pumpEventsFor(kSettleProbeIntervalMs);
        const QImage current = window()->grab().toImage();
        if (current == previous)
        {
            return;
        }
        previous = current;
    }
    qWarning() << "captureAllPages: the window did not settle within" << kSettleMaxMs << "ms";
}

void ComponentDemoWidget::pinCaptureLayout(QWidget* page)
{
    if (!page)
    {
        return;
    }

    if (auto* exampleText = page->findChild<QTextEdit*>(QStringLiteral("exampleText")))
    {
        exampleText->setFixedWidth(kCaptureExampleTextWidth);
    }

    const QList<QSplitter*> splitters = page->findChildren<QSplitter*>();
    for (QSplitter* splitter : splitters)
    {
        const QList<int> natural = splitter->sizes();
        int total = 0;
        for (int size : natural)
        {
            total += size;
        }
        if (natural.size() < 2 || total <= 0)
        {
            continue;
        }
        QList<int> even;
        for (int index = 0; index < natural.size(); ++index)
        {
            even.append(total / natural.size());
        }
        splitter->setSizes(even);
    }
}

void ComponentDemoWidget::dumpPageGeometry(QWidget* page, const QString& filePath)
{
    if (!page)
    {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        return;
    }

    QTextStream out(&file);

    // Frame line: lets the comparer attribute differences to the window shell,
    // the example text panel, or the demo area (all in logical px; dpr scales
    // them to the grabbed pixmap).
    const QWidget* frame = window();
    const QPoint pageOrigin = page->mapTo(window(), QPoint(0, 0));
    QRect exampleRect;
    if (auto* exampleText = page->findChild<QTextEdit*>(QStringLiteral("exampleText")))
    {
        exampleRect = QRect(exampleText->mapTo(window(), QPoint(0, 0)), exampleText->size());
    }
    out << "# frame window " << frame->width() << ' ' << frame->height()
        << " dpr " << frame->devicePixelRatioF()
        << " page " << pageOrigin.x() << ' ' << pageOrigin.y() << ' ' << page->width() << ' ' << page->height()
        << " example " << exampleRect.x() << ' ' << exampleRect.y() << ' ' << exampleRect.width() << ' ' << exampleRect.height()
        << '\n';
    // Determinism diagnostics: where the window sits, what has focus, what is hovered.
    const QRect screenRect = frame->screen() ? frame->screen()->geometry() : QRect();
    const QWidget* focus = QApplication::focusWidget();
    const QWidget* hovered = QApplication::widgetAt(QCursor::pos());
    const auto describe = [](const QWidget* w)
    {
        return w ? QString::fromLatin1("%1/%2").arg(QString::fromLatin1(w->metaObject()->className()), w->objectName()) : QStringLiteral("none");
    };
    out << "# state screen " << screenRect.x() << ' ' << screenRect.y() << ' ' << screenRect.width() << ' ' << screenRect.height()
        << " windowPos " << frame->pos().x() << ' ' << frame->pos().y()
        << " cursor " << QCursor::pos().x() << ' ' << QCursor::pos().y()
        << " focus " << describe(focus) << " hovered " << describe(hovered) << '\n';
    out << "# class\tobjectName\tx\ty\tw\th\tsizeHintW\tsizeHintH\tminHintW\tminHintH\tvisible"
           "\tfont\tcontentsMargins\n";

    // Whole window, window-relative coordinates: the shell (menu bar, page selector,
    // status area) is compared exactly like the page content. Widgets of the OTHER
    // (hidden) gallery pages are skipped: they are not laid out and would only add
    // noise to the geometry comparison.
    const QList<QWidget*> widgets = window()->findChildren<QWidget*>();
    for (QWidget* widget : widgets)
    {
        if (ui->demoWidgetStack->isAncestorOf(widget) && widget != page && !page->isAncestorOf(widget))
        {
            continue;
        }
        const QPoint topLeft = widget->mapTo(window(), QPoint(0, 0));
        const QSize hint = widget->sizeHint();
        const QSize minHint = widget->minimumSizeHint();
        const QFont font = widget->font();
        const QMargins margins = widget->contentsMargins();
        out << widget->metaObject()->className() << '\t' << widget->objectName() << '\t'
            << topLeft.x() << '\t' << topLeft.y() << '\t' << widget->width() << '\t' << widget->height() << '\t'
            << hint.width() << '\t' << hint.height() << '\t' << minHint.width() << '\t' << minHint.height() << '\t'
            << (widget->isVisible() ? 1 : 0) << '\t'
            << font.family() << '=' << QFontInfo(font).family() << '/' << font.pixelSize() << "px/"
            << font.pointSizeF() << "pt/" << font.weight() << '\t'
            << margins.left() << ',' << margins.top() << ',' << margins.right() << ',' << margins.bottom() << '\n';
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

    // Every interaction state gets the same dwell + frame-stable wait as the base capture.
    const auto settle = [this]()
    {
        settleForCapture();
    };
    const auto capturePath = [outputDirPath, pageIndex, pageName](const QString& suffix)
    {
        return QStringLiteral("%1/%2_%3__%4.png")
            .arg(outputDirPath)
            .arg(pageIndex, 2, 10, QLatin1Char('0'))
            .arg(pageName)
            .arg(suffix);
    };

    // Toggled state: open every tree and flip every card, so the state the base
    // capture did not show (collapsed cards are the default-expanded ones) is
    // captured too. Card::setExpanded is a no-op when already in that state.
    bool toggledAnything = false;
    for (QTreeView* treeView : page->findChildren<QTreeView*>())
    {
        treeView->expandAll();
        toggledAnything = true;
    }
    for (AzQtComponents::Card* card : page->findChildren<AzQtComponents::Card*>())
    {
        card->setExpanded(!card->isExpanded());
        toggledAnything = true;
    }
    if (toggledAnything)
    {
        settle();
        window()->grab().save(capturePath(QStringLiteral("toggled")));
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
        // The popup can take a few event-loop turns to become active; retry briefly.
        QWidget* popup = QApplication::activePopupWidget();
        for (int attempt = 0; !popup && attempt < 20; ++attempt)
        {
            QCoreApplication::processEvents();
            popup = QApplication::activePopupWidget();
        }
        if (popup)
        {
            popup->grab().save(capturePath(QStringLiteral("combo%1-popup").arg(comboIndex)));
        }
        else
        {
            qWarning() << "captureAllPages: combo popup did not appear on page" << pageName << "combo" << comboIndex;
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
        auto* watchdog = new QTimer(this);
        watchdog->setInterval(kDialogPollIntervalMs);
        auto attempts = std::make_shared<int>(0);
        connect(watchdog, &QTimer::timeout, this, [watchdog, attempts, dialogCapturePath, settle]()
        {
            ++(*attempts);
            if (QWidget* modal = QApplication::activeModalWidget())
            {
                settle();
                // window() includes the WindowDecorationWrapper chrome the dialog was wrapped in
                modal->window()->grab().save(dialogCapturePath);
                modal->close();
                watchdog->stop();
                watchdog->deleteLater();
            }
            else if (*attempts >= kDialogPollAttempts)
            {
                qWarning() << "captureAllPages: no modal dialog appeared for" << dialogCapturePath;
                watchdog->stop();
                watchdog->deleteLater();
            }
        });
        watchdog->start();
        button->click();
        settle();
        if (++dialogIndex >= 3)
        {
            break;
        }
    }

    // Dock widgets: the StyledDockWidget page builds its dock main window lazily
    // behind a "Show main window" button, as its own top-level window (wrapped by
    // WindowDecorationWrapper), so docks are searched in every top-level window,
    // not just the page. Grab the host, then float each dock, grab, re-dock.
    QWidget* dockHost = nullptr;
    for (QPushButton* button : page->findChildren<QPushButton*>())
    {
        if (button->isVisible() && button->text().contains(QStringLiteral("main window"), Qt::CaseInsensitive))
        {
            button->click();
            settle();
            break;
        }
    }
    for (QWidget* top : QApplication::topLevelWidgets())
    {
        if (top != window() && top->isVisible() && !top->findChildren<QDockWidget*>().isEmpty())
        {
            dockHost = top;
            break;
        }
    }
    QWidget* dockSearchRoot = dockHost ? dockHost : page;
    if (dockHost)
    {
        dockHost->grab().save(capturePath(QStringLiteral("dockmain")));
    }

    int dockIndex = 0;
    for (QDockWidget* dockWidget : dockSearchRoot->findChildren<QDockWidget*>())
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

    if (dockHost)
    {
        dockHost->close();
        settle();
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

