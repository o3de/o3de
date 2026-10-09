/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <QtGlobal>

#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleHelpers.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Components/Widgets/DialogButtonBox.h>
#include <AzQtComponents/Components/Widgets/DragAndDrop.h>
#include <AzQtComponents/Components/Widgets/PushButton.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzQtComponents/Components/Widgets/RadioButton.h>
#include <AzQtComponents/Components/Widgets/ProgressBar.h>
#include <AzQtComponents/Components/Widgets/Slider.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/ColorPicker.h>
#include <AzQtComponents/Components/Widgets/Eyedropper.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/PaletteView.h>
#include <AzQtComponents/Components/Widgets/LineEdit.h>
#include <AzQtComponents/Components/Widgets/ComboBox.h>
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <AzQtComponents/Components/Widgets/BreadCrumbs.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <AzQtComponents/Components/Widgets/ScrollBar.h>
#include <AzQtComponents/Components/Widgets/StatusBar.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>
#include <AzQtComponents/Components/Widgets/TableView.h>
#include <AzQtComponents/Components/Widgets/TreeView.h>
#include <AzQtComponents/Components/Widgets/Menu.h>
#include <AzQtComponents/Components/Widgets/Text.h>
#include <AzQtComponents/Components/Widgets/ToolBar.h>
#include <AzQtComponents/Components/Widgets/ToolButton.h>
#include <AzQtComponents/Components/Widgets/VectorInput.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <AzQtComponents/Components/Widgets/AssetFolderThumbnailView.h>
#include <AzQtComponents/Components/Titlebar.h>
#include <AzQtComponents/Components/StyledBusyLabel.h>
#include <AzQtComponents/Utilities/TextUtilities.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
#include <QAbstractScrollArea>
#include <QApplication>
#include <QCheckBox>
#include <QDockWidget>
#include <QLayout>
#include <QPainterPath>
#include <QTabBar>
#include <QMenu>
#include <QAbstractItemView>
#include <QMainWindow>
#include <QScrollArea>
#include <QToolBar>
#include <QComboBox>
#include <QDebug>
#include <QFile>
#include <QFileSystemWatcher>
#include <QHeaderView>
#include <QKeyEvent>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QListView>
#include <QObject>
#include <QPainter>
#include <QPixmapCache>
#include <QProgressBar>
#include <QSvgRenderer>
#include <QPushButton>
#include <QRadioButton>
#include <QScopedValueRollback>
#include <QSet>
#include <QSettings>
#include <QStyleOptionToolButton>
#include <QTableView>
#include <QTextEdit>
#include <QToolButton>
AZ_POP_DISABLE_WARNING

#include <limits>
#include <QListWidget>

namespace AzQtComponents
{
    // Add this css class to QTreeView's if you want to have absolute control of styling
    // in stylesheets. If you don't add this class, the expand arrows will ALWAYS paint.
    // See ::drawPrimitive below for more info.
    static QString g_treeViewDisableDefaultArrorPainting = QStringLiteral("DisableArrowPainting");

    static const char g_removeAllStylingProperty[] = {"RemoveAllStyling"};

    // Constant for the docking drop zone hotspot color when hovered over
    static const QColor g_dropZoneColorOnHover(23, 163, 205);


    // Private data structure
    struct Style::Data
    {
        QPalette palette;

        PushButton::Config pushButtonConfig;
        RadioButton::Config radioButtonConfig;
        CheckBox::Config checkBoxConfig;
        ProgressBar::Config progressBarConfig;
        Slider::Config sliderConfig;
        Card::Config cardConfig;
        ColorPicker::Config colorPickerConfig;
        Eyedropper::Config eyedropperConfig;
        PaletteView::Config paletteViewConfig;
        LineEdit::Config lineEditConfig;
        ComboBox::Config comboBoxConfig;
        BrowseEdit::Config browseEditConfig;
        BreadCrumbs::Config breadCrumbsConfig;
        SpinBox::Config spinBoxConfig;
        ScrollBar::Config scrollBarConfig;
        TabWidget::Config tabWidgetConfig;
        TableView::Config tableViewConfig;
        Text::Config textConfig;
        FilteredSearchWidget::Config filteredSearchWidgetConfig;
        AssetFolderThumbnailView::Config assetFolderThumbnailViewConfig;
        TitleBar::Config titleBarConfig;
        Menu::Config menuConfig;
        ToolButton::Config toolButtonConfig;
        DockBarButton::Config dockBarButtonConfig;
        StatusBar::Config statusBarConfig;
        DragAndDrop::Config dragAndDropConfig;
        ToolBar::Config toolBarConfig;
        TreeView::Config treeViewConfig;

        QFileSystemWatcher watcher;

        QSet<QObject*> widgetsToRepolishOnReload;
    };

    // Local template function to load config data from .ini files
    template <typename ConfigType, typename WidgetType>
    void loadConfig(Style* style, QFileSystemWatcher* watcher, ConfigType* config, const QString& path)
    {
        QString fullPath = QStringLiteral("AzQtComponentWidgets:%1").arg(path);
        ConfigHelpers::loadConfig<ConfigType, WidgetType>(watcher, config, fullPath, style, std::bind(&Style::settingsReloaded, style));
    }

    Style::DrawWidgetSentinel::DrawWidgetSentinel(const QWidget* widgetAboutToDraw)
        : m_style(qobject_cast<const Style*>(widgetAboutToDraw->style()))
        , m_lastDrawWidget(m_style ? m_style->m_drawControlWidget : nullptr)
    {
        if (m_style)
        {
            m_style->m_drawControlWidget = widgetAboutToDraw;
        }
    }

    Style::DrawWidgetSentinel::~DrawWidgetSentinel()
    {
        if (m_style)
        {
            m_style->m_drawControlWidget = m_lastDrawWidget.data();
        }
    }

    Style::Style(QStyle* style)
        : QProxyStyle(style)
        , m_data(new Style::Data)
    {
        SpinBox::initializeWatcher();
        LineEdit::initializeWatcher();
        ScrollBar::initializeWatcher();
        ComboBox::initializeWatcher();
        TreeView::initializeWatcher();

        // set up settings watchers
        loadConfig<PushButton::Config, PushButton>(this, &m_data->watcher, &m_data->pushButtonConfig, "PushButtonConfig.ini");
        loadConfig<RadioButton::Config, RadioButton>(this, &m_data->watcher, &m_data->radioButtonConfig, "RadioButtonConfig.ini");
        loadConfig<CheckBox::Config, CheckBox>(this, &m_data->watcher, &m_data->checkBoxConfig, "CheckBoxConfig.ini");
        loadConfig<ProgressBar::Config, ProgressBar>(this, &m_data->watcher, &m_data->progressBarConfig, "ProgressBarConfig.ini");
        loadConfig<Slider::Config, Slider>(this, &m_data->watcher, &m_data->sliderConfig, "SliderConfig.ini");
        loadConfig<Card::Config, Card>(this, &m_data->watcher, &m_data->cardConfig, "CardConfig.ini");
        loadConfig<ColorPicker::Config, ColorPicker>(this, &m_data->watcher, &m_data->colorPickerConfig, "ColorPickerConfig.ini");
        loadConfig<Eyedropper::Config, Eyedropper>(this, &m_data->watcher, &m_data->eyedropperConfig, "EyedropperConfig.ini");
        loadConfig<PaletteView::Config, PaletteView>(this, &m_data->watcher, &m_data->paletteViewConfig, "ColorPicker/PaletteViewConfig.ini");
        loadConfig<LineEdit::Config, LineEdit>(this, &m_data->watcher, &m_data->lineEditConfig, "LineEditConfig.ini");
        loadConfig<ComboBox::Config, ComboBox>(this, &m_data->watcher, &m_data->comboBoxConfig, "ComboBoxConfig.ini");
        loadConfig<BrowseEdit::Config, BrowseEdit>(this, &m_data->watcher, &m_data->browseEditConfig, "BrowseEditConfig.ini");
        loadConfig<BreadCrumbs::Config, BreadCrumbs>(this, &m_data->watcher, &m_data->breadCrumbsConfig, "BreadCrumbsConfig.ini");
        loadConfig<SpinBox::Config, SpinBox>(this, &m_data->watcher, &m_data->spinBoxConfig, "SpinBoxConfig.ini");
        loadConfig<ScrollBar::Config, ScrollBar>(this, &m_data->watcher, &m_data->scrollBarConfig, "ScrollBarConfig.ini");
        loadConfig<TabWidget::Config, TabWidget>(this, &m_data->watcher, &m_data->tabWidgetConfig, "TabWidgetConfig.ini");
        loadConfig<TableView::Config, TableView>(this, &m_data->watcher, &m_data->tableViewConfig, "TableViewConfig.ini");
        loadConfig<Text::Config, Text>(this, &m_data->watcher, &m_data->textConfig, "TextConfig.ini");
        loadConfig<FilteredSearchWidget::Config, FilteredSearchWidget>(this, &m_data->watcher, &m_data->filteredSearchWidgetConfig, "FilteredSearchWidgetConfig.ini");
        loadConfig<AssetFolderThumbnailView::Config, AssetFolderThumbnailView>(this, &m_data->watcher, &m_data->assetFolderThumbnailViewConfig, "AssetFolderThumbnailViewConfig.ini");
        loadConfig<TitleBar::Config, TitleBar>(this, &m_data->watcher, &m_data->titleBarConfig, "TitleBarConfig.ini");
        loadConfig<Menu::Config, Menu>(this, &m_data->watcher, &m_data->menuConfig, "MenuConfig.ini");
        loadConfig<ToolButton::Config, ToolButton>(this, &m_data->watcher, &m_data->toolButtonConfig, "ToolButtonConfig.ini");
        loadConfig<DockBarButton::Config, DockBarButton>(this, &m_data->watcher, &m_data->dockBarButtonConfig, "DockBarButtonConfig.ini");
        loadConfig<StatusBar::Config, StatusBar>(this, &m_data->watcher, &m_data->statusBarConfig, "StatusBarConfig.ini");
        loadConfig<DragAndDrop::Config, DragAndDrop>(this, &m_data->watcher, &m_data->dragAndDropConfig, "DragAndDropConfig.ini");
        loadConfig<ToolBar::Config, ToolBar>(this, &m_data->watcher, &m_data->toolBarConfig, "ToolBarConfig.ini");
        loadConfig<TreeView::Config, TreeView>(this, &m_data->watcher, &m_data->treeViewConfig, "TreeViewConfig.ini");

        VectorElement::initStaticVars(m_data->spinBoxConfig.labelSize);
        Slider::initStaticVars(m_data->sliderConfig.verticalToolTipOffset, m_data->sliderConfig.horizontalToolTipOffset);
    }

    Style::~Style()
    {
        SpinBox::uninitializeWatcher();
        LineEdit::uninitializeWatcher();
        ScrollBar::uninitializeWatcher();
        ComboBox::uninitializeWatcher();
        TreeView::uninitializeWatcher();
    }

    QIcon Style::icon(const QString& name)
    {
        const QString filePath = QStringLiteral(":/stylesheet/img/UI20/toolbar/%1.svg").arg(name);
        if (QFile::exists(filePath))
        {
            return QIcon(filePath);
        }

        qWarning() << "Style::icon: Couldn't find " << filePath;
        return {};
    }

    QColor Style::dropZoneColorOnHover()
    {
        return g_dropZoneColorOnHover;
    }


    QSize Style::sizeFromContents(QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget) const
    {
        // SegmentBar buttons carry flagToIgnore (hasStyle == false) - size them before
        // the gate. SegmentControl.qss: height 28, padding 16/16, min-width 68.
        if (StyleManager::stylesheetsDisabled() && type == CT_PushButton && widget
            && (hasClass(widget, QStringLiteral("TabOne")) || hasClass(widget, QStringLiteral("TabFirst"))
                || hasClass(widget, QStringLiteral("TabMiddle")) || hasClass(widget, QStringLiteral("TabLast"))))
        {
            // Legacy capture (page 20): 30 tall (28 + the 1px borders), 4px wider than this
            // formula gave for the leading segment, 3px for the others (margin-left -1);
            // vertical bars: 30 for the leading segment, 29 for the rest (margin-top -1).
            // TOKENIZE: SegmentHeight, SegmentPadding
            QSize segmentSize = QProxyStyle::sizeFromContents(type, option, size, widget);
            const bool leading = hasClass(widget, QStringLiteral("TabOne")) || hasClass(widget, QStringLiteral("TabFirst"));
            const QWidget* bar = widget->parentWidget();
            const bool verticalBar = bar && bar->property("orientation").toInt() == 2;
            segmentSize.setHeight((verticalBar && !leading) ? 29 : 30);
            segmentSize.setWidth(qMax(segmentSize.width() + 16, 70) + (leading ? 4 : 3));
            return segmentSize;
        }

        // BrowseEdit's line edit carries flagToIgnore (hasStyle == false); BrowseEdit.qss
        // QLineEdit { height: 16px; border-width: 0 }. TOKENIZE: BrowseEditLineEditHeight
        if (StyleManager::stylesheetsDisabled() && type == CT_LineEdit && widget && widget->parentWidget()
            && widget->parentWidget()->inherits("AzQtComponents::BrowseEdit"))
        {
            QSize lineEditSize = QProxyStyle::sizeFromContents(type, option, size, widget);
            lineEditSize.setHeight(16);
            return lineEditSize;
        }

        if (!hasStyle(widget))
        {
            return QProxyStyle::sizeFromContents(type, option, size, widget);
        }

        if (StyleManager::stylesheetsDisabled())
        {
            switch (type)
            {
                case CT_ItemViewItem:
                {
                    // qss-era item view row floor (24px)
                    QSize itemSize = QProxyStyle::sizeFromContents(type, option, size, widget);
                    itemSize.setHeight(qMax(itemSize.height(), 24));
                    return itemSize;
                }
                case CT_TabBarTab:
                {
                    // TabWidget.qss TabBar:tab { padding: 6px 21px 6px 9px; max-width: 200px },
                    // TabBar { min-height: 28px }: the legacy tab is 8px wider than Fusion's
                    // for the same text and the bar 31 tall (Gallery capture); dock tab bars
                    // follow the same rule, which is what makes their overflow caret appear.
                    // TOKENIZE: TabPadding, TabHeight, TabMaxWidth
                    QSize tabSize = QProxyStyle::sizeFromContents(type, option, size, widget);
                    tabSize.setHeight(31);
                    tabSize.setWidth(qMin(tabSize.width() + 8, 200));
                    return tabSize;
                }
                case CT_ComboBox:
                    if (qobject_cast<const QComboBox*>(widget))
                    {
                        return ComboBox::sizeFromContents(this, type, option, size, widget, m_data->comboBoxConfig);
                    }
                    break;
                case CT_LineEdit:
                {
                    // LineEdit.qss: height 16 + 2px border -> 20; SpinBox.qss line edits 19;
                    // editable-combo line edits 16. The legacy box is 2px wider than Fusion's.
                    // TOKENIZE: LineEditHeight, SpinBoxLineEditHeight, ComboBoxLineEditHeight
                    QSize lineEditSize = QProxyStyle::sizeFromContents(type, option, size, widget);
                    const QWidget* parent = widget ? widget->parentWidget() : nullptr;
                    int height = 20;
                    if (parent && parent->inherits("QAbstractSpinBox"))
                    {
                        height = 19;
                    }
                    else if (parent && parent->inherits("QComboBox"))
                    {
                        height = 16;
                    }
                    lineEditSize.setHeight(height);
                    lineEditSize.rwidth() += 2;
                    return lineEditSize;
                }
                case CT_SpinBox:
                {
                    // SpinBox.qss: height 16 + margin 2 + padding -> 23 (legacy capture, page 23)
                    // TOKENIZE: SpinBoxHeight
                    QSize spinSize = QProxyStyle::sizeFromContents(type, option, size, widget);
                    spinSize.setHeight(23);
                    return spinSize;
                }
                case CT_MenuBarItem:
                {
                    // MenuBar.qss: QMenuBar height 28 + border-bottom 2px = 30; ::item padding
                    // left/right 14px (legacy capture: bar 30 tall). TOKENIZE: MenuBarHeight, MenuBarItemPadding
                    return QSize(size.width() + 28, 30);
                }
                case CT_HeaderSection:
                {
                    // TableView.qss QHeaderView::section { padding-left: 7px }: vertical (row
                    // number) sections are text + 7 + 4 wide (legacy capture: 18 for one digit).
                    // TOKENIZE: HeaderSectionPadding
                    auto header = qobject_cast<const QHeaderView*>(widget);
                    auto headerOption = qstyleoption_cast<const QStyleOptionHeader*>(option);
                    if (header && headerOption && header->orientation() == Qt::Vertical)
                    {
                        QSize sectionSize = QProxyStyle::sizeFromContents(type, option, size, widget);
                        sectionSize.setWidth(headerOption->fontMetrics.horizontalAdvance(headerOption->text) + 11);
                        return sectionSize;
                    }
                    break;
                }
                default:
                    break;
            }
        }

        switch (type)
        {
            case QStyle::CT_PushButton:
                if (qobject_cast<const QPushButton*>(widget))
                {
                    return PushButton::sizeFromContents(this, type, option, size, widget, m_data->pushButtonConfig);
                }
                break;

            case QStyle::CT_ToolButton:
                if (qobject_cast<const QToolButton*>(widget))
                {
                    return ToolButton::sizeFromContents(this, type, option, size, widget, m_data->toolButtonConfig);
                }
                break;

            case QStyle::CT_CheckBox:
                if (qobject_cast<const QCheckBox*>(widget))
                {
                    return CheckBox::sizeFromContents(this, type, option, size, widget, m_data->checkBoxConfig);
                }
                break;

            case QStyle::CT_RadioButton:
                if (qobject_cast<const QRadioButton*>(widget))
                {
                    return RadioButton::sizeFromContents(this, type, option, size, widget, m_data->radioButtonConfig);
                }
                break;

            case QStyle::CT_ProgressBar:
                if (qobject_cast<const QProgressBar*>(widget))
                {
                    return ProgressBar::sizeFromContents(this, type, option, size, widget, m_data->progressBarConfig);
                }
                break;
            case QStyle::CT_ComboBox:
            {
                if (qobject_cast<const QComboBox*>(widget))
                {
                    return ComboBox::sizeFromContents(this, type, option, size, widget, m_data->comboBoxConfig);
                }
                break;
            }
            case QStyle::CT_HeaderSection:
            {
                const auto headerSize = TableView::sizeFromContents(this, type, option, size, widget, m_data->tableViewConfig);
                if (headerSize.isValid())
                {
                    return headerSize;
                }
                break;
            }
        }

        return QProxyStyle::sizeFromContents(type, option, size, widget);
    }

    void Style::drawControl(QStyle::ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
    {
        QScopedValueRollback<const QWidget*> rollbackDrawControl(m_drawControlWidget, widget);

        // SegmentBar buttons carry flagToIgnore (hasStyle == false), so their flattened
        // look must be handled BEFORE the gate.
        if (StyleManager::stylesheetsDisabled() && element == CE_PushButtonBevel && widget
            && (hasClass(widget, QStringLiteral("TabOne")) || hasClass(widget, QStringLiteral("TabFirst"))
                || hasClass(widget, QStringLiteral("TabMiddle")) || hasClass(widget, QStringLiteral("TabLast"))))
        {
            // SegmentControl.qss: flat #333333, hover #444444, selected #555555, #222222 border
            QColor segmentFill(0x33, 0x33, 0x33);
            if (option->state.testFlag(QStyle::State_On) || option->state.testFlag(QStyle::State_Sunken))
            {
                segmentFill = QColor(0x55, 0x55, 0x55);
            }
            else if (option->state.testFlag(QStyle::State_MouseOver))
            {
                segmentFill = QColor(0x44, 0x44, 0x44);
            }
            // SegmentControl.qss: seamless bar - each button has a 1px #222222 border
            // and non-first buttons carry margin-left:-1px so adjacent borders OVERLAP
            // into a single line. Mirror that by extending the border rect 1px left
            // for every non-leading segment.
            QRect borderRect = option->rect;
            const bool leadingSegment = hasClass(widget, QStringLiteral("TabOne")) || hasClass(widget, QStringLiteral("TabFirst"));
            if (!leadingSegment)
            {
                borderRect.adjust(-1, 0, 0, 0);
            }

            painter->fillRect(option->rect, segmentFill);
            painter->setPen(QColor(0x22, 0x22, 0x22));
            painter->drawRect(borderRect.adjusted(0, 0, -1, -1));
            return;
        }

        if (!hasStyle(widget))
        {
            QProxyStyle::drawControl(element, option, painter, widget);
            return;
        }

        prepPainter(painter);
        switch (element)
        {
            case CE_ShapedFrame:
            {
                // FilteredSearchWidget.qss tag chips: dark blocks rgb(46,46,46),
                // 1px #808080 border, 2px radius
                if (StyleManager::stylesheetsDisabled() && widget
                    && (widget->inherits("AzQtComponents::FilterCriteriaButton")
                        || widget->inherits("AzQtComponents::FilterTextButton")))
                {
                    painter->save();
                    painter->setRenderHint(QPainter::Antialiasing);
                    painter->setPen(QColor(0x80, 0x80, 0x80));
                    painter->setBrush(QColor(46, 46, 46));
                    painter->drawRoundedRect(QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5), 2.0, 2.0);
                    painter->restore();
                    return;
                }
                if (StyleManager::stylesheetsDisabled() && widget
                    && QString::fromLatin1(widget->metaObject()->className()) == QLatin1String("QFrame"))
                {
                    if (auto frameOption = qstyleoption_cast<const QStyleOptionFrame*>(option))
                    {
                        // BaseStyleSheet.qss `.QFrame[frameShape="1|2|4|5|6"] { background-color:
                        // #444444 }` (exact QFrame only): the sheet's box replaced the native
                        // frame, so Box / Panel / StyledPanel frames show no line (the Inspector
                        // asset details frame), and HLine / VLine draw a 1px line in the text
                        // colour the universal rule gave them. TOKENIZE: PlainFrameFace, PlainFrameLine
                        const int shape = frameOption->frameShape;
                        if (shape == QFrame::Box || shape == QFrame::Panel || shape == QFrame::StyledPanel)
                        {
                            painter->fillRect(option->rect, QColor(0x44, 0x44, 0x44));
                            return;
                        }
                        if (shape == QFrame::HLine || shape == QFrame::VLine)
                        {
                            const QRect r = option->rect;
                            const QRect line = (shape == QFrame::HLine)
                                ? QRect(r.left(), r.center().y(), r.width(), 1)
                                : QRect(r.center().x(), r.top(), 1, r.height());
                            painter->fillRect(r, QColor(0x44, 0x44, 0x44));
                            painter->fillRect(line, option->palette.color(QPalette::WindowText));
                            return;
                        }
                    }
                }
                if (StyleManager::stylesheetsDisabled() && widget && hasClass(widget, QStringLiteral("HorizontalSeparator")))
                {
                    // ColorPicker.qss .HorizontalSeparator: background #222222 with 1px #393a3c
                    // top and bottom lines. TOKENIZE: PickerSeparatorFill, PickerSeparatorLine
                    painter->fillRect(option->rect, QColor(0x22, 0x22, 0x22));
                    painter->fillRect(QRect(option->rect.left(), option->rect.top(), option->rect.width(), 1), QColor(0x39, 0x3A, 0x3C));
                    painter->fillRect(QRect(option->rect.left(), option->rect.bottom(), option->rect.width(), 1), QColor(0x39, 0x3A, 0x3C));
                    return;
                }
                if (StyleManager::stylesheetsDisabled() && widget && widget->parentWidget()
                    && widget->parentWidget()->inherits("AzQtComponents::VectorElement") && widget->inherits("QLabel"))
                {
                    // VectorInput.qss VectorElement[Coordinate=X|Y|Z|W] QLabel background
                    // #E25243 / #58BC61 / #4285F4 / #E57829, left corners 2px.
                    // TOKENIZE: VectorXColor, VectorYColor, VectorZColor, VectorWColor
                    const QString coordinate = widget->parentWidget()->property("Coordinate").toString();
                    QColor fill;
                    if (coordinate == QLatin1String("X")) fill = QColor(0xE2, 0x52, 0x43);
                    else if (coordinate == QLatin1String("Y")) fill = QColor(0x58, 0xBC, 0x61);
                    else if (coordinate == QLatin1String("Z")) fill = QColor(0x42, 0x85, 0xF4);
                    else if (coordinate == QLatin1String("W")) fill = QColor(0xE5, 0x78, 0x29);
                    if (fill.isValid())
                    {
                        painter->save();
                        painter->setRenderHint(QPainter::Antialiasing, true);
                        painter->setPen(Qt::NoPen);
                        painter->setBrush(fill);
                        painter->drawRoundedRect(QRectF(option->rect), 2.0, 2.0);
                        painter->drawRect(QRectF(option->rect).adjusted(2.0, 0.0, 0.0, 0.0)); // square right corners
                        painter->restore();
                    }
                    return;
                }
                if (BrowseEdit::drawFrame(this, option, painter, widget, m_data->browseEditConfig))
                {
                    return;
                }
            }
            break;

            case CE_Splitter:
            {
                // Splitter.qss: a 1px #222222 line (hover #1E70EB) in the middle of the 5px
                // handle; QMainWindow::separator (dock gaps): #111111 across the whole gap.
                // TOKENIZE: SplitterLineColor, SplitterHoverColor, DockSeparatorColor
                if (StyleManager::stylesheetsDisabled())
                {
                    if (qobject_cast<const QMainWindow*>(widget))
                    {
                        painter->fillRect(option->rect, QColor(0x11, 0x11, 0x11));
                        return;
                    }
                    const QColor line = option->state.testFlag(QStyle::State_MouseOver) ? QColor(0x1E, 0x70, 0xEB) : QColor(0x22, 0x22, 0x22);
                    if (option->state.testFlag(QStyle::State_Horizontal))
                    {
                        painter->fillRect(QRect(option->rect.center().x(), option->rect.top(), 1, option->rect.height()), line);
                    }
                    else
                    {
                        painter->fillRect(QRect(option->rect.left(), option->rect.center().y(), option->rect.width(), 1), line);
                    }
                    return;
                }
            }
            break;

            case CE_TabBarTabShape:
            {
                // TabWidget.qss: flat boxes, inactive #333333 / active #444444, #111111 border
                if (StyleManager::stylesheetsDisabled())
                {
                    const bool selected = option->state.testFlag(QStyle::State_Selected);
                    if (hasClass(widget, QStringLiteral("Secondary")))
                    {
                        // TabWidget.qss TabBar.Secondary::tab: #444444 (:hover +15% white),
                        // border-bottom 3px #333333 (:hover #444444, :selected #008DE5);
                        // .Bordered adds border-top 1px #111111.
                        // TOKENIZE: SecondaryTabFace, SecondaryTabUnderline, SecondaryTabSelectedUnderline
                        const bool hover = option->state.testFlag(QStyle::State_MouseOver);
                        painter->fillRect(option->rect, QColor(0x44, 0x44, 0x44));
                        if (hover && !selected)
                        {
                            painter->fillRect(option->rect, QColor(255, 255, 255, 38));
                        }
                        const QRect underline(option->rect.left(), option->rect.bottom() - 2, option->rect.width(), 3);
                        painter->fillRect(underline, selected ? QColor(0x00, 0x8D, 0xE5) : (hover ? QColor(0x44, 0x44, 0x44) : QColor(0x33, 0x33, 0x33)));
                        if (hasClass(widget, QStringLiteral("Bordered")))
                        {
                            painter->fillRect(QRect(option->rect.left(), option->rect.top(), option->rect.width(), 1), QColor(0x11, 0x11, 0x11));
                        }
                        return;
                    }
                    // TabWidget.qss TabBar:tab: #333333 with a 1px #111111 border; :selected
                    // #444444 whose bottom border is #444444 (flush with the page). The legacy
                    // tab box reaches the bottom of the tab bar (no bar backdrop under it).
                    // Dock tab bars (legacy capture): plain faces, no outline at all.
                    // TOKENIZE: TabFace, TabSelectedFace, TabBorder
                    QRect tabRect = option->rect;
                    if (widget)
                    {
                        tabRect.setBottom(widget->rect().bottom());
                    }
                    const QColor face = selected ? QColor(0x44, 0x44, 0x44) : QColor(0x33, 0x33, 0x33);
                    painter->fillRect(tabRect, face);
                    if (widget && widget->inherits("AzQtComponents::DockTabBar"))
                    {
                        return;
                    }
                    painter->setPen(QColor(0x11, 0x11, 0x11));
                    painter->drawRect(tabRect.adjusted(0, 0, -1, -1));
                    if (selected)
                    {
                        painter->fillRect(QRect(tabRect.left() + 1, tabRect.bottom(), tabRect.width() - 2, 1), face);
                    }
                    return;
                }
            }
            break;

            case CE_MenuBarEmptyArea:
            {
                // MenuBar.qss QMenuBar: window face with a 2px #111111 bottom border.
                // TOKENIZE: MenuBarBorderColor
                if (StyleManager::stylesheetsDisabled())
                {
                    painter->fillRect(option->rect, QColor(0x44, 0x44, 0x44));
                    painter->fillRect(QRect(option->rect.left(), option->rect.bottom() - 1, option->rect.width(), 2), QColor(0x11, 0x11, 0x11));
                    return;
                }
            }
            break;

            case CE_MenuBarItem:
            {
                // MenuBar.qss ::item:selected #555555, ::item:pressed #222222, white text; the
                // 2px #111111 bottom border runs under the items too.
                // TOKENIZE: MenuBarItemHoverColor, MenuBarItemPressedColor
                if (StyleManager::stylesheetsDisabled())
                {
                    if (auto menuBarItem = qstyleoption_cast<const QStyleOptionMenuItem*>(option))
                    {
                        const QRect box = option->rect.adjusted(0, 0, 0, -2);
                        const bool pressed = option->state.testFlag(QStyle::State_Sunken);
                        const bool selected = option->state.testFlag(QStyle::State_Selected) && option->state.testFlag(QStyle::State_Enabled);
                        painter->fillRect(option->rect, QColor(0x44, 0x44, 0x44));
                        if (pressed)
                        {
                            painter->fillRect(box, QColor(0x22, 0x22, 0x22));
                        }
                        else if (selected)
                        {
                            painter->fillRect(box, QColor(0x55, 0x55, 0x55));
                        }
                        painter->fillRect(QRect(option->rect.left(), option->rect.bottom() - 1, option->rect.width(), 2), QColor(0x11, 0x11, 0x11));
                        QPalette textPalette = option->palette;
                        textPalette.setColor(QPalette::ButtonText, QColor(0xFF, 0xFF, 0xFF));
                        drawItemText(painter, box, Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine,
                            textPalette, option->state.testFlag(QStyle::State_Enabled), menuBarItem->text, QPalette::ButtonText);
                        return;
                    }
                }
            }
            break;

            case CE_TabBarTabLabel:
            {
                // TabWidget.qss TabBar.Secondary::tab { color: #959595 } (:hover/:selected #FFFFFF)
                // TOKENIZE: SecondaryTabTextColor, SecondaryTabActiveTextColor
                if (StyleManager::stylesheetsDisabled() && hasClass(widget, QStringLiteral("Secondary")))
                {
                    if (auto tabOption = qstyleoption_cast<const QStyleOptionTab*>(option))
                    {
                        const bool lit = option->state.testFlag(QStyle::State_Selected) || option->state.testFlag(QStyle::State_MouseOver);
                        QStyleOptionTab labelOption = *tabOption;
                        labelOption.palette.setColor(QPalette::WindowText, lit ? QColor(0xFF, 0xFF, 0xFF) : QColor(0x95, 0x95, 0x95));
                        labelOption.palette.setColor(QPalette::ButtonText, labelOption.palette.color(QPalette::WindowText));
                        QProxyStyle::drawControl(CE_TabBarTabLabel, &labelOption, painter, widget);
                        return;
                    }
                }
            }
            break;

            case CE_DockWidgetTitle:
            {
                // QDockWidget.qss backup path: title strip #333333 (raw QDockWidgets in gems)
                if (StyleManager::stylesheetsDisabled())
                {
                    painter->fillRect(option->rect, QColor(0x33, 0x33, 0x33));
                    if (auto dockOption = qstyleoption_cast<const QStyleOptionDockWidget*>(option))
                    {
                        painter->setPen(QColor(Qt::white));
                        painter->drawText(option->rect.adjusted(8, 0, -8, 0), Qt::AlignVCenter | Qt::AlignLeft, dockOption->title);
                    }
                    return;
                }
            }
            break;

            case CE_PushButtonBevel:
            {
                if (qobject_cast<const QPushButton*>(widget))
                {
                    if (PushButton::drawPushButtonBevel(this, option, painter, widget, m_data->pushButtonConfig))
                    {
                        return;
                    }
                }
            }
            break;

            case CE_CheckBox:
            {
                if (qobject_cast<const QCheckBox*>(widget))
                {
                    if (CheckBox::drawCheckBox(this, option, painter, widget, m_data->checkBoxConfig))
                    {
                        return;
                    }
                }
            }
            break;

            case CE_CheckBoxLabel:
            {
                if (qobject_cast<const QCheckBox*>(widget))
                {
                    if (CheckBox::drawCheckBoxLabel(this, option, painter, widget, m_data->checkBoxConfig))
                    {
                        return;
                    }
                }
            }
            break;

            case CE_RadioButton:
            {
                if (qobject_cast<const QRadioButton*>(widget))
                {
                    if (RadioButton::drawRadioButton(this, option, painter, widget, m_data->radioButtonConfig))
                    {
                        return;
                    }
                }
            }
            break;

            case CE_RadioButtonLabel:
            {
                if (qobject_cast<const QRadioButton*>(widget))
                {
                    if (RadioButton::drawRadioButtonLabel(this, option, painter, widget, m_data->radioButtonConfig))
                    {
                        return;
                    }
                }
            }
            break;

            case CE_Header:
            {
                // Flatten: hard-fill header sections (TableView.qss #2d2d2d) - the
                // palette-only route was unreliable because Fusion blends Button
                if (StyleManager::stylesheetsDisabled() && widget && widget->inherits("QTableCornerButton"))
                {
                    // Legacy render: the QTableView corner button is solid black (user verdict,
                    // 2026-09-24 pair 19). TOKENIZE: TableCornerButtonColor
                    painter->fillRect(option->rect, QColor(Qt::black));
                    return;
                }
                if (StyleManager::stylesheetsDisabled() && qobject_cast<const QHeaderView*>(widget))
                {
                    // A header whose sections were made transparent by a local sheet
                    // (property "SectionBackground" = "transparent") shows the window colour.
                    const bool transparentSections = widget->property("SectionBackground").toString() == QLatin1String("transparent");
                    painter->fillRect(option->rect, transparentSections ? QColor(0x44, 0x44, 0x44) : QColor(0x2D, 0x2D, 0x2D));
                    painter->setPen(QColor(0x22, 0x22, 0x22));
                    painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
                    painter->drawLine(option->rect.topRight(), option->rect.bottomRight());
                    if (auto headerOption = qstyleoption_cast<const QStyleOptionHeader*>(option))
                    {
                        QStyleOptionHeader labelOption = *headerOption;
                        labelOption.rect = option->rect.adjusted(7, 0, -4, 0); // qss header pad-left 7
                        labelOption.palette.setColor(QPalette::ButtonText, QColor(0xCC, 0xCC, 0xCC));
                        QProxyStyle::drawControl(CE_HeaderLabel, &labelOption, painter, widget);
                    }
                    return;
                }
                if (qobject_cast<const QHeaderView*>(widget))
                {
                    if (TableView::drawHeader(this, option, painter, widget, m_data->tableViewConfig))
                    {
                        return;
                    }
                }
            }
            break;

            case CE_ComboBoxLabel:
            {
                if (qobject_cast<const QComboBox*>(widget))
                {
                    if (ComboBox::drawComboBoxLabel(this, option, painter, widget, m_data->comboBoxConfig))
                    {
                        return;
                    }
                }
            }
            break;

            case CE_ItemViewItem:
            {
                // For styling QTableView and QListView
                auto tableView = qobject_cast<const QTableView*>(widget);
                auto listView = qobject_cast<const QListView*>(widget);
                auto validListView = listView && !StyleHelpers::findParent<QComboBox>(listView);
                // For styling QTreeView (but not AzQtComponents::TableView)
                auto treeView = qobject_cast<const QTreeView*>(widget);
                auto validTreeView = treeView && !qobject_cast<const TableView*>(widget);

                auto itemOption = qstyleoption_cast<const QStyleOptionViewItem*>(option);

                if ((tableView || validListView) && itemOption)
                {
                    auto copy = *itemOption;
                    copy.styleObject = const_cast<QObject*>(qobject_cast<const QObject*>(widget));

                    // Need to stretch the focus rect to span all rows in a QTableView.
                    // Taking into account also possible headers.
                    if (tableView)
                    {
                        auto hHdr = tableView->horizontalHeader()->isVisible() ? tableView->horizontalHeader()->height() : 0;
                        auto vHdr = tableView->verticalHeader()->isVisible() ? tableView->verticalHeader()->width() : 0;

                        auto rowRect = copy.rect;
                        rowRect.setWidth(tableView->width());
                        rowRect.moveLeft(vHdr);
                        rowRect.adjust(0, hHdr, 0, hHdr);

                        copy.state.setFlag(QStyle::State_MouseOver, rowRect.contains(tableView->mapFromGlobal(QCursor::pos())));

                        // Draw focus frame rectangle in all cells belonging to the selected row.
                        // The focus frame rectangle is only drawn if QStyle::State_HasFocus is set,
                        // but we only get QStyle::State_Selected from the selection model, so we
                        // need to force QStyle::State_HasFocus whenever QStyle::State_Selected is set.
                        copy.state.setFlag(QStyle::State_HasFocus, tableView->hasFocus() && (copy.state & QStyle::State_Selected));
                    }

                    // QStyleSheetStyle seems to ignore the background color set by the model
                    painter->fillRect(copy.rect, copy.backgroundBrush);

                    return QProxyStyle::drawControl(element, &copy, painter, widget);
                }
                else if (validTreeView)
                {
                    if (TreeView::isBranchLinesEnabled(treeView) && qobject_cast<BranchDelegate*>(treeView->itemDelegate()) && option->state.testFlag(QStyle::State_Children))
                    {
                        auto copy = *itemOption;
                        copy.rect.adjust(treeView->indentation(), 0, 0, 0);
                        return QProxyStyle::drawControl(element, &copy, painter, widget);
                    }
                }
            }
            break;
        }

        return QProxyStyle::drawControl(element, option, painter, widget);
    }

    //////////////////////////////////////////////////////////////////////////
    // Flattened indicator painting (GUI flattening)
    //
    // Check box, toggle switch, expander and radio button indicators were
    // delivered by QSS "image:" rules (CheckBox.qss / RadioButton.qss) over
    // SVGs compiled into the AzQtComponents resources. In flattened mode the
    // style owns them: widget state maps to the same SVG assets, rendered
    // through the pixmap cache. Item view check indicators route through the
    // same PE_IndicatorCheckBox primitive.
    //////////////////////////////////////////////////////////////////////////
    static QPixmap flattenedIndicatorPixmap(const QString& svgPath, const QSize& size, qreal devicePixelRatio)
    {
        const QString cacheKey = QString::fromLatin1("O3DEFlatInd|%1|%2x%3|%4")
            .arg(svgPath).arg(size.width()).arg(size.height()).arg(devicePixelRatio);

        QPixmap pixmap;
        if (!QPixmapCache::find(cacheKey, &pixmap))
        {
            QSvgRenderer renderer(svgPath);
            pixmap = QPixmap(size * devicePixelRatio);
            pixmap.setDevicePixelRatio(devicePixelRatio);
            pixmap.fill(Qt::transparent);
            QPainter pixmapPainter(&pixmap);
            // Explicit logical bounds: render(painter) alone uses the pixmap's DEVICE
            // pixel size as logical bounds, drawing scale-factor times too large on
            // high-DPI displays (the clipped-indicator bug).
            renderer.render(&pixmapPainter, QRectF(QPointF(0.0, 0.0), QSizeF(size)));
            pixmapPainter.end();
            QPixmapCache::insert(cacheKey, pixmap);
        }
        return pixmap;
    }

    static void drawFlattenedIndicator(const QString& svgPath, QPainter* painter, const QRect& rect)
    {
        if (svgPath.isEmpty() || !rect.isValid())
        {
            return;
        }

        const qreal devicePixelRatio = painter->device() ? painter->device()->devicePixelRatio() : 1.0;
        painter->drawPixmap(rect, flattenedIndicatorPixmap(svgPath, rect.size(), devicePixelRatio));
    }

    static QString flattenedCheckBoxIndicatorPath(const QStyleOption* option, const QWidget* widget)
    {
        const bool enabled = option->state.testFlag(QStyle::State_Enabled);
        const bool focused = option->state.testFlag(QStyle::State_HasFocus);
        const bool checked = option->state.testFlag(QStyle::State_On);
        const bool partial = option->state.testFlag(QStyle::State_NoChange);

        // QCheckBox variants carried as style classes (see CheckBox.qss)
        if (widget && Style::hasClass(widget, QStringLiteral("ToggleSwitch")))
        {
            QString name = checked ? QStringLiteral("checked") : QStringLiteral("unchecked");
            if (!enabled)
            {
                name += QStringLiteral("-disabled");
            }
            else if (focused)
            {
                name += QStringLiteral("-focus");
            }
            return QStringLiteral(":/stylesheet/img/UI20/toggleswitch/%1.svg").arg(name);
        }

        if (widget && Style::hasClass(widget, QStringLiteral("Expander")))
        {
            QString name = checked ? QStringLiteral("caret-down") : QStringLiteral("caret-right");
            if (!enabled)
            {
                name += QStringLiteral("-disabled");
            }
            return QStringLiteral(":/Cards/img/UI20/Cards/%1.svg").arg(name);
        }

        QString name = partial ? QStringLiteral("partial-selected") : (checked ? QStringLiteral("on") : QStringLiteral("off"));
        if (!enabled)
        {
            name += QStringLiteral("-disabled");
        }
        else if (focused)
        {
            name += QStringLiteral("-focus");
        }
        return QStringLiteral(":/stylesheet/img/UI20/checkbox/%1.svg").arg(name);
    }

    static QString flattenedRadioButtonIndicatorPath(const QStyleOption* option)
    {
        const bool enabled = option->state.testFlag(QStyle::State_Enabled);
        const bool focused = option->state.testFlag(QStyle::State_HasFocus);

        QString name = option->state.testFlag(QStyle::State_On) ? QStringLiteral("checked") : QStringLiteral("unchecked");
        if (!enabled)
        {
            name += QStringLiteral("-disabled");
        }
        else if (focused)
        {
            name += QStringLiteral("-focus");
        }
        return QStringLiteral(":/stylesheet/img/UI20/radiobutton/%1.svg").arg(name);
    }

    //////////////////////////////////////////////////////////////////////////
    // Flattened scroll bar painting (GUI flattening)
    //
    // ScrollBar.qss: transparent 8px lane, rounded semi-transparent handle
    // (rgba 255,255,255,40%; DarkScrollBar class rgba 136,136,136,64%),
    // opaque lane fill on hover (#555555 light / #DCDCDC dark), arrow
    // buttons removed. The DarkScrollBar class lives on the owning
    // QAbstractScrollArea (Console, Python terminal).
    //////////////////////////////////////////////////////////////////////////
    static void drawFlattenedScrollBar(const Style* style, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget)
    {
        auto sliderOption = qstyleoption_cast<const QStyleOptionSlider*>(option);
        if (!sliderOption)
        {
            return;
        }

        const QWidget* scrollArea = widget ? widget->parentWidget() : nullptr;
        while (scrollArea && !qobject_cast<const QAbstractScrollArea*>(scrollArea))
        {
            scrollArea = scrollArea->parentWidget();
        }
        const bool dark = scrollArea && Style::hasClass(scrollArea, QStringLiteral("DarkScrollBar"));

        const QColor hoverFill = dark ? QColor(0xDC, 0xDC, 0xDC) : QColor(0x55, 0x55, 0x55);
        const QColor handleColor = dark ? QColor(136, 136, 136, 163) : QColor(255, 255, 255, 102);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        if (option->state & QStyle::State_MouseOver)
        {
            painter->fillRect(option->rect, hoverFill);
        }

        QRect handleRect = style->subControlRect(QStyle::CC_ScrollBar, option, QStyle::SC_ScrollBarSlider, widget);
        if (handleRect.isValid())
        {
            const bool horizontal = sliderOption->orientation == Qt::Horizontal;
            handleRect.adjust(horizontal ? 0 : 2, horizontal ? 2 : 0, horizontal ? 0 : -2, horizontal ? -2 : 0);
            painter->setPen(Qt::NoPen);
            painter->setBrush(handleColor);
            painter->drawRoundedRect(handleRect, 2, 2);
        }

        painter->restore();
    }

    void Style::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
    {
        if (element == PE_IndicatorDockWidgetResizeHandle)
        {
            // BaseStyleSheet.qss QMainWindow::separator { background-color: #111111 }
            // (:hover #1E70EB) -- Fusion would draw its light resize line instead.
            // TOKENIZE: DockSeparatorColor, DockSeparatorHoverColor
            if (StyleManager::stylesheetsDisabled())
            {
                painter->fillRect(option->rect,
                    option->state.testFlag(QStyle::State_MouseOver) ? QColor(0x1E, 0x70, 0xEB) : QColor(0x11, 0x11, 0x11));
                return;
            }

            // There is a bug in Qt where the option state Horizontal flag is
            // being set/unset incorrectly for some cases, particularly when you
            // have multiple dock widgets docked on the absolute edges, so we
            // can rely instead on the width/height relationship to determine
            // if the resize handle should be horizontal or vertical.

            // Here we just patch the QStyleOption and forward the drawing to its normal route
            if (auto optionHacked = const_cast<QStyleOption*>(option))
            {
                const bool isHorizontal = option->rect.width() > option->rect.height();
                // TODO for Qt >= 5.7: Simply replace with: optionHacked->state.setFlag(QStyle::State_Horizontal, isHorizontal);
                if (isHorizontal)
                {
                    optionHacked->state |= QStyle::State_Horizontal;
                }
                else
                {
                    optionHacked->state &= ~QStyle::State_Horizontal;
                }
            }

            return QProxyStyle::drawPrimitive(element, option, painter, widget);
        }

        if (!hasStyle(widget))
        {
            QProxyStyle::drawPrimitive(element, option, painter, widget);
            return;
        }

        prepPainter(painter);
        switch (element)
        {
            case PE_PanelLineEdit:
            {
                if (LineEdit::drawFrame(this, option, painter, widget, m_data->lineEditConfig))
                {
                    return;
                }
            }
            break;

            case PE_FrameFocusRect:
            {
                // We're not passing the widget parameter to TableView because for some reason QTreeView doesn't
                // use this parameter when calling QStyle::drawPrimitive to draw PE_FrameFocusRect (so it's always nullptr)
                if (TableView::drawFrameFocusRect(this, option, painter, m_data->tableViewConfig))
                {
                    return;
                }
                else if (qobject_cast<const QPushButton*>(widget) || qobject_cast<const QToolButton*>(widget))
                {
                    if (PushButton::drawPushButtonFocusRect(this, option, painter, widget, m_data->pushButtonConfig))
                    {
                        return;
                    }
                }
            }
            break;

            case PE_PanelButtonTool:
            {
                if (PushButton::drawPushButtonBevel(this, option, painter, widget, m_data->pushButtonConfig))
                {
                    return;
                }
            }
            break;

            case PE_IndicatorArrowDown:
            {
                if (qobject_cast<const QComboBox*>(widget) && ComboBox::drawIndicatorArrow(this, option, painter, widget, m_data->comboBoxConfig))
                {
                    return;
                }
                else if (PushButton::drawIndicatorArrowDown(this, option, painter, widget, m_data->pushButtonConfig))
                {
                    return;
                }
                else if (ToolButton::drawIndicatorArrowDown(this, option, painter, widget, m_data->toolButtonConfig))
                {
                    return;
                }
            }
            break;

            case PE_IndicatorItemViewItemDrop:
            {
                if (PaletteView::drawDropIndicator(this, option, painter, widget, m_data->paletteViewConfig))
                {
                    return;
                }
                else if (DragAndDrop::drawDropIndicator(this, option, painter, widget, m_data->dragAndDropConfig))
                {
                    return;
                }
            }
            break;

            case PE_IndicatorBranch:
            {
                // Flatten FIRST: the BranchDelegate path below delegates to raw Fusion
                // baseStyle() (massive native arrows); in flatten mode the caret SVGs
                // own this for every tree. TableView hides branches (qss).
                if (StyleManager::stylesheetsDisabled() && !qobject_cast<const TableView*>(widget))
                {
                    if (option->state.testFlag(QStyle::State_Children))
                    {
                        // Natural caret svg sizes (TableView.qss draws them unscaled:
                        // closed 4x8, open 8x4)
                        const bool open = option->state.testFlag(QStyle::State_Open);
                        QRect glyphRect = open ? QRect(0, 0, 8, 4) : QRect(0, 0, 4, 8);
                        glyphRect.moveCenter(option->rect.center());
                        drawFlattenedIndicator(open ? QStringLiteral(":/TreeView/open.svg")
                                                    : QStringLiteral(":/TreeView/closed.svg"),
                            painter, glyphRect);
                    }
                    return;
                }
                if (TreeView::drawBranchIndicator(this, option, painter, widget, m_data->treeViewConfig))
                {
                    return;
                }
            }
            break;

            case PE_IndicatorItemViewItemCheck:
            {
                if (StyleManager::stylesheetsDisabled())
                {
                    // Combo box popups have NO check indicators (upstream combo design:
                    // current row is pre-highlighted instead)
                    for (const QWidget* ancestor = widget; ancestor; ancestor = ancestor->parentWidget())
                    {
                        if (ancestor->inherits("QComboBoxPrivateContainer"))
                        {
                            return;
                        }
                    }
                    // Item view check boxes share the CheckBox.qss indicator images
                    drawFlattenedIndicator(flattenedCheckBoxIndicatorPath(option, widget), painter, option->rect);
                    return;
                }
            }
            break;

            case PE_IndicatorTabClose:
            {
                if (StyleManager::stylesheetsDisabled())
                {
                    if (option->state.testFlag(QStyle::State_MouseOver))
                    {
                        painter->fillRect(option->rect, QColor(0x44, 0x44, 0x44));
                    }
                    QRect glyphRect(0, 0, 12, 12);
                    glyphRect.moveCenter(option->rect.center());
                    drawFlattenedIndicator(QStringLiteral(":/Application/titlebar-close.svg"), painter, glyphRect);
                    return;
                }
            }
            break;

            case PE_FrameDockWidget:
            {
                // StyledDockWidget.qss: no frame when docked; 1px black when floating
                if (StyleManager::stylesheetsDisabled())
                {
                    const QDockWidget* dockWidget = qobject_cast<const QDockWidget*>(widget);
                    if (dockWidget && dockWidget->isFloating())
                    {
                        painter->setPen(QColor(Qt::black));
                        painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
                    }
                    return;
                }
            }
            break;

            case PE_IndicatorCheckBox:
            {
                if (StyleManager::stylesheetsDisabled())
                {
                    if (qobject_cast<const QMenu*>(widget))
                    {
                        // Fusion asks for a check BOX on checkable menu items; the sheet left the
                        // base style's plain check MARK (white, no box). TOKENIZE: MenuCheckMark
                        QProxyStyle::drawPrimitive(PE_IndicatorMenuCheckMark, option, painter, widget);
                        return;
                    }
                    if (widget && widget->inherits("AzToolsFramework::EntityOutlinerCheckBox"))
                    {
                        // EntityOutliner.qss: the outliner's visibility / lock toggles are check
                        // boxes whose indicator image is picked by objectName + state (eye / lock
                        // glyphs, hover variants, "Mixed" variants on a rounded rgba(0,0,0,80) pad);
                        // 16x16 image centred in an 18px box, margin-top 3 (lock_on, lock hover and
                        // lock overridden-unchecked: 1). Legacy at rest shows the small dots.
                        // TOKENIZE: OutlinerVisibilityIcons, OutlinerLockIcons, OutlinerMixedPad
                        const QString name = widget->objectName();
                        const bool checked = option->state.testFlag(QStyle::State_On);
                        const bool visibility = name.startsWith(QLatin1String("Visibility"));
                        const bool hover = name.endsWith(QLatin1String("Hover"));
                        const bool overridden = name.contains(QLatin1String("Overridden"));
                        const bool mixed = name.contains(QLatin1String("Mixed"));
                        QString image;
                        int marginTop = 3;
                        if (visibility)
                        {
                            if (hover)
                            {
                                image = checked ? QStringLiteral("visibility_default_hover") : QStringLiteral("visibility_on_hover");
                            }
                            else if (overridden)
                            {
                                image = checked ? QStringLiteral("visibility_default_transparent") : QStringLiteral("visibility_on_transparent");
                            }
                            else
                            {
                                image = checked ? QStringLiteral("visibility_default") : QStringLiteral("visibility_on");
                            }
                        }
                        else
                        {
                            if (hover)
                            {
                                image = checked ? QStringLiteral("lock_on_hover") : QStringLiteral("lock_default_hover");
                                marginTop = 1;
                            }
                            else if (overridden)
                            {
                                image = checked ? QStringLiteral("lock_on_transparent") : QStringLiteral("lock_default_transparent");
                                marginTop = checked ? 3 : 1;
                            }
                            else
                            {
                                image = checked ? QStringLiteral("lock_on") : QStringLiteral("lock_default");
                                marginTop = checked ? 1 : 3;
                            }
                        }
                        const QRect box(widget->rect().left(), widget->rect().top() + marginTop, 18, 16);
                        if (mixed)
                        {
                            painter->save();
                            painter->setRenderHint(QPainter::Antialiasing, true);
                            painter->setPen(Qt::NoPen);
                            painter->setBrush(QColor(0, 0, 0, 80));
                            painter->drawRoundedRect(box, 5, 5);
                            painter->restore();
                        }
                        drawFlattenedIndicator(QStringLiteral(":/Outliner/%1.svg").arg(image), painter, QRect(box.left() + 1, box.top(), 16, 16));
                        return;
                    }
                    drawFlattenedIndicator(flattenedCheckBoxIndicatorPath(option, widget), painter, option->rect);
                    return;
                }
            }
            break;

            case PE_PanelToolBar:
            {
                // ToolBar.qss QToolBar { background-color: #444444; border: 0 none };
                // MainWindow QToolBar { border-bottom: 2px solid #111111 }. TOKENIZE: ToolBarBottomBorder
                if (StyleManager::stylesheetsDisabled())
                {
                    painter->fillRect(option->rect, QColor(0x44, 0x44, 0x44));
                    const QWidget* parent = widget ? widget->parentWidget() : nullptr;
                    if (parent && QString::fromLatin1(parent->metaObject()->className()) == QLatin1String("MainWindow"))
                    {
                        painter->fillRect(QRect(option->rect.left(), option->rect.bottom() - 1, option->rect.width(), 2), QColor(0x11, 0x11, 0x11));
                    }
                    return;
                }
            }
            break;

            case PE_IndicatorToolBarSeparator:
            {
                // ToolBar.qss QToolBar::separator { background-color: #111111; width: 1px;
                // margin: 4px 0px }  TOKENIZE: ToolBarSeparatorColor
                if (StyleManager::stylesheetsDisabled())
                {
                    const QRect line = option->state.testFlag(QStyle::State_Horizontal)
                        ? option->rect.adjusted(0, 4, 0, -4)
                        : option->rect.adjusted(4, 0, -4, 0);
                    painter->fillRect(line, QColor(0x11, 0x11, 0x11));
                    return;
                }
            }
            break;

            case PE_FrameStatusBarItem:
            {
                // Status bar items carry no frame (PE_FrameDockWidget is handled above).
                if (StyleManager::stylesheetsDisabled())
                {
                    return;
                }
            }
            break;

            case PE_FrameTabBarBase:
            {
                // TabWidget.qss: qproperty-drawBase false; TabBar.Secondary.Bordered adds a 1px
                // #111111 left + top line. TOKENIZE: TabBarBorderColor
                if (StyleManager::stylesheetsDisabled() && widget && widget->inherits("AzQtComponents::TabBar"))
                {
                    if (hasClass(widget, QStringLiteral("Secondary")) && hasClass(widget, QStringLiteral("Bordered")))
                    {
                        painter->fillRect(QRect(option->rect.left(), option->rect.top(), 1, option->rect.height()), QColor(0x11, 0x11, 0x11));
                        painter->fillRect(QRect(option->rect.left(), option->rect.top(), option->rect.width(), 1), QColor(0x11, 0x11, 0x11));
                    }
                    return;
                }
            }
            break;

            case PE_PanelMenu:
            {
                if (StyleManager::stylesheetsDisabled())
                {
                    // Menu.qss QMenu { background-color: #222222 }, no frame. TOKENIZE: MenuSurfaceColor
                    painter->fillRect(option->rect, QColor(0x22, 0x22, 0x22));
                    return;
                }
            }
            break;

            case PE_FrameMenu:
            {
                if (StyleManager::stylesheetsDisabled())
                {
                    return;
                }
            }
            break;

            case PE_PanelButtonCommand:
            {
                // Fusion sinks a panel behind the icon of a checked menu item; the sheet drew none.
                if (StyleManager::stylesheetsDisabled() && qobject_cast<const QMenu*>(widget))
                {
                    return;
                }
            }
            break;

            case PE_FrameTabWidget:
            {
                // TabWidget.qss: TabWidget:pane { border: 0 none }; .Secondary:pane adds a 2px
                // #2B2B2B top line. TOKENIZE: SecondaryTabPaneLine
                if (StyleManager::stylesheetsDisabled() && widget && widget->inherits("AzQtComponents::TabWidget"))
                {
                    if (hasClass(widget, QStringLiteral("Secondary")))
                    {
                        painter->fillRect(QRect(option->rect.left(), option->rect.top(), option->rect.width(), 2), QColor(0x2B, 0x2B, 0x2B));
                        if (hasClass(widget, QStringLiteral("Bordered")))
                        {
                            // TabWidget.Secondary.Bordered { border: 1px solid #111111 }
                            painter->setPen(QColor(0x11, 0x11, 0x11));
                            painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
                        }
                    }
                    return;
                }
            }
            break;

            case PE_IndicatorRadioButton:
            {
                if (StyleManager::stylesheetsDisabled())
                {
                    // The focus variant carries its own 18px ring; grow the 16px rect so the ring is not cropped
                    QRect indicatorRect = option->rect;
                    if (option->state.testFlag(QStyle::State_HasFocus) && option->state.testFlag(QStyle::State_Enabled))
                    {
                        indicatorRect.adjust(-1, -1, 1, 1);
                    }
                    drawFlattenedIndicator(flattenedRadioButtonIndicatorPath(option), painter, indicatorRect);
                    return;
                }
            }
            break;

            case PE_PanelStatusBar:
            {
                if (StatusBar::drawPanelStatusBar(this, option, painter, widget, m_data->statusBarConfig))
                {
                    return;
                }
            }
            break;

            case PE_PanelItemViewRow:
            {
                /** HACK
                 * For TableView, we want the first row to use the alternate color, not the second one.
                 * The "right" way to do this would be to invert the `background-color` and
                 * `alternate-background-color` properties in the stylesheet, but if we do this:
                 *
                 * - the widget won't inherit the background color from the base stylesheet;
                 * - the widget background outside the list will be filled with a light shade of gray instead
                 *   of the global background color, which is not what we want.
                 *
                 * So here we just flip the Alternate flag before letting the base style fill the item background.
                 **/
                if (auto* tableView = qobject_cast<const TableView*>(widget))
                {
                    if (tableView->alternatingRowColors())
                    {
                        if (auto itemOption = qstyleoption_cast<const QStyleOptionViewItem*>(option))
                        {
                            auto copy = *itemOption;
                            copy.features ^= QStyleOptionViewItem::Alternate;
                            return QProxyStyle::drawPrimitive(element, &copy, painter, widget);
                        }
                    }
                }
            }
            break;
        }

        return QProxyStyle::drawPrimitive(element, option, painter, widget);
    }

    void Style::drawComplexControl(QStyle::ComplexControl element, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget) const
    {
        if (!hasStyle(widget))
        {
            QProxyStyle::drawComplexControl(element, option, painter, widget);
            return;
        }

        prepPainter(painter);

        switch (element)
        {
            case CC_SpinBox:
                if (SpinBox::drawSpinBox(this, option, painter, widget, m_data->spinBoxConfig))
                {
                    return;
                }
                break;

            case CC_Slider:
                if (auto sliderOption = qstyleoption_cast<const QStyleOptionSlider*>(option))
                {
                    if (Slider::drawSlider(this, sliderOption, painter, widget, m_data->sliderConfig))
                    {
                        return;
                    }
                }
                break;

            case CC_ToolButton:
                if (DockBarButton::drawDockBarButton(this, option, painter, widget, m_data->dockBarButtonConfig))
                {
                    return;
                }
                if (ToolButton::drawToolButton(this, option, painter, widget, m_data->toolButtonConfig))
                {
                    return;
                }
                break;

            case CC_ComboBox:
                // Flatten: ComboBox::drawComboBox owns the ComboBox.qss box for every combo.
                if (ComboBox::drawComboBox(this, option, painter, widget, m_data->comboBoxConfig))
                {
                    return;
                }
                break;

            case CC_ScrollBar:
                if (StyleManager::stylesheetsDisabled())
                {
                    drawFlattenedScrollBar(this, option, painter, widget);
                    return;
                }
                if (ScrollBar::drawScrollBar(this, option, painter, widget, m_data->scrollBarConfig))
                {
                    return;
                }
                break;
        }

        return QProxyStyle::drawComplexControl(element, option, painter, widget);
    }

    void Style::drawDragIndicator(const QStyleOption* option, QPainter* painter, const QWidget* widget) const
    {
        DragAndDrop::drawDragIndicator(this, option, painter, widget, m_data->dragAndDropConfig);
    }

    QPixmap Style::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap& pixmap, const QStyleOption* option) const
    {
        if ((qobject_cast<const TableView*>(m_drawControlWidget) ||
            qobject_cast<const QListWidget*>(m_drawControlWidget)) && iconMode == QIcon::Mode::Selected)
        {
            return QProxyStyle::generatedIconPixmap(QIcon::Mode::Active, pixmap, option);
        }

        QPixmap generatedPixmap;
        generatedPixmap = Card::generatedIconPixmap(iconMode, pixmap, option, m_drawControlWidget, m_data->cardConfig);
        if (!generatedPixmap.isNull())
        {
            return generatedPixmap;
        }

        generatedPixmap = PushButton::generatedIconPixmap(iconMode, pixmap, option, m_data->pushButtonConfig);
        if (!generatedPixmap.isNull())
        {
            return generatedPixmap;
        }

        return QProxyStyle::generatedIconPixmap(iconMode, pixmap, option);
    }

    QRect Style::subControlRect(ComplexControl control, const QStyleOptionComplex* option, SubControl subControl, const QWidget* widget) const
    {
        if (!hasStyle(widget))
        {
            return QProxyStyle::subControlRect(control, option, subControl, widget);
        }

        switch (control)
        {
            case CC_ComboBox:
            {
                if (auto comboBoxOption = qstyleoption_cast<const QStyleOptionComboBox*>(option))
                {
                    switch (subControl)
                    {
                    case SC_ComboBoxListBoxPopup:
                    {
                        QRect r = ComboBox::comboBoxListBoxPopupRect(this, comboBoxOption, widget, m_data->comboBoxConfig);
                        if (!r.isNull())
                        {
                            return r;
                        }
                    }
                    break;

                    default:
                        break;
                    }
                }
            }
            break;
            case CC_ScrollBar:
            {
                if (StyleManager::stylesheetsDisabled())
                {
                    if (auto sliderOption = qstyleoption_cast<const QStyleOptionSlider*>(option))
                    {
                        switch (subControl)
                        {
                            case SC_ScrollBarAddLine:
                            case SC_ScrollBarSubLine:
                                // ScrollBar.qss removes both arrow buttons (0px)
                                return QRect();

                            case SC_ScrollBarGroove:
                                return option->rect;

                            case SC_ScrollBarSlider:
                            {
                                const bool horizontal = sliderOption->orientation == Qt::Horizontal;
                                const int trackLength = horizontal ? option->rect.width() : option->rect.height();
                                const int sliderMin = 32; // matches PM_ScrollBarSliderMin above
                                const int range = sliderOption->maximum - sliderOption->minimum;
                                int sliderLength = range <= 0
                                    ? trackLength
                                    : (sliderOption->pageStep * trackLength) / (range + sliderOption->pageStep);
                                sliderLength = qBound(sliderMin, sliderLength, trackLength);
                                const int sliderPos = QStyle::sliderPositionFromValue(
                                    sliderOption->minimum, sliderOption->maximum, sliderOption->sliderPosition,
                                    trackLength - sliderLength, sliderOption->upsideDown);
                                return horizontal
                                    ? QRect(option->rect.x() + sliderPos, option->rect.y(), sliderLength, option->rect.height())
                                    : QRect(option->rect.x(), option->rect.y() + sliderPos, option->rect.width(), sliderLength);
                            }

                            default:
                                break;
                        }
                    }
                }
            }
            break;

            case CC_Slider:
            {
                if (auto sliderOption = qstyleoption_cast<const QStyleOptionSlider*>(option))
                {
                    switch (subControl)
                    {
                        case SC_SliderHandle:
                        {
                            QRect r = Slider::sliderHandleRect(this, sliderOption, widget, m_data->sliderConfig);
                            if (!r.isNull())
                            {
                                return r;
                            }
                        }
                        break;

                        case SC_SliderGroove:
                        {
                            QRect r = Slider::sliderGrooveRect(this, sliderOption, widget, m_data->sliderConfig);
                            if (!r.isNull())
                            {
                                return r;
                            }
                        }
                        break;

                        default:
                            break;
                    }
                }
            }
            break;

            case CC_SpinBox:
            {
                switch (subControl)
                {
                    case SC_SpinBoxEditField:
                    {
                        QRect r;
                        auto spinBox = qobject_cast<const SpinBox*>(widget);
                        auto doubleSpinBox = qobject_cast<const DoubleSpinBox*>(widget);
                        auto vectorElement = qobject_cast<const VectorElement*>(widget->parent());

                        if (StyleManager::stylesheetsDisabled() && (spinBox || doubleSpinBox))
                        {
                            // Flatten: one box for plain and vector spin boxes (SpinBox.qss +
                            // VectorInput.qss resolved inside SpinBox::editFieldRect).
                            r = SpinBox::editFieldRect(this, option, widget, m_data->spinBoxConfig);
                        }
                        else if (vectorElement && doubleSpinBox)
                        {
                            r = VectorElement::editFieldRect(this, option, widget, m_data->spinBoxConfig);
                        }
                        else if (spinBox || doubleSpinBox)
                        {
                            r = SpinBox::editFieldRect(this, option, widget, m_data->spinBoxConfig);
                        }

                        if (!r.isNull())
                        {
                            return r;
                        }
                    }
                    break;

                    default:
                        break;
                }
            }
            break;

            case CC_ToolButton:
            {
                return ToolButton::subControlRect(this, option, subControl, widget, m_data->toolButtonConfig);
            }
        }

        return QProxyStyle::subControlRect(control, option, subControl, widget);
    }

    //////////////////////////////////////////////////////////////////////////
    // Flattened label box (GUI flattening)
    //
    // Text.qss gives every QLabel a vertical margin per typography class. It
    // reached Qt as the label's frame box (SE_ShapedFrameContents), which is
    // also what switches on QLabel's automatic half-'x' text indent, so it is
    // reproduced at that contact point rather than as a contents margin.
    // TOKENIZE: TextLabelMargin (per class)
    //////////////////////////////////////////////////////////////////////////
    static QRect flattenedLabelFrameContents(const QStyleOption* option, const QWidget* widget)
    {
        if (!option || !widget || !widget->inherits("QLabel"))
        {
            return QRect();
        }

        const QWidget* parent = widget->parentWidget();
        if (parent && (parent->inherits("AzQtComponents::FilterCriteriaButton") || parent->inherits("AzQtComponents::FilterTextButton")))
        {
            // FilteredSearchWidget.qss: chip labels carry no margin except #tagIcon
            // margin-right 4 and #tagText margin-left/right 4. TOKENIZE: TagLabelMargin
            if (widget->objectName() == QLatin1String("tagIcon"))
            {
                return option->rect.adjusted(0, 0, -4, 0);
            }
            if (widget->objectName() == QLatin1String("tagText"))
            {
                return option->rect.adjusted(4, 0, -4, 0);
            }
            return option->rect;
        }
        if (parent && parent->inherits("AzQtComponents::VectorElement"))
        {
            // VectorInput.qss: VectorElement QLabel { margin-top: 2px; margin-bottom: 2px }
            return option->rect.adjusted(0, 2, 0, -2);
        }
        if (widget->objectName() == QLatin1String("title") && parent && parent->inherits("AzQtComponents::TitleBar"))
        {
            // TitleBar.qss #title { margin-left: 10px; margin-right: 16px } + the QLabel 1px
            return option->rect.adjusted(10, 1, -16, -1);
        }

        int margin = 1; // .Label, QLabel
        if (Style::hasClass(widget, QStringLiteral("Headline")) || Style::hasClass(widget, QStringLiteral("Paragraph")))
        {
            margin = 4;
        }
        else if (Style::hasClass(widget, QStringLiteral("Title")))
        {
            margin = 7;
        }
        else if (Style::hasClass(widget, QStringLiteral("Subtitle")))
        {
            margin = 8;
        }
        else if (Style::hasClass(widget, QStringLiteral("Menu")) || Style::hasClass(widget, QStringLiteral("Button")))
        {
            margin = 6;
        }
        else if (Style::hasClass(widget, QStringLiteral("Tooltip")))
        {
            margin = 2;
        }
        return option->rect.adjusted(0, margin, 0, -margin);
    }

    QRect Style::subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget) const
    {
        if (!hasStyle(widget))
        {
            return QProxyStyle::subElementRect(element, option, widget);
        }

        switch (element)
        {
            case SE_ShapedFrameContents:
            {
                if (StyleManager::stylesheetsDisabled())
                {
                    // The qss box (margin/border/padding) of a QFrame reached Qt through
                    // this element; the flattened boxes come back through the same door.
                    QRect r = Card::shapedFrameContents(option, widget);
                    if (r.isNull() && widget && widget->inherits("AzQtComponents::BrowseEdit"))
                    {
                        // BrowseEdit.qss: margin 2px. TOKENIZE: BrowseEditMargin
                        r = option->rect.adjusted(2, 2, -2, -2);
                    }
                    if (r.isNull() && widget
                        && (widget->inherits("AzQtComponents::FilterCriteriaButton") || widget->inherits("AzQtComponents::FilterTextButton")
                            || widget->inherits("AzQtComponents::SelectionCountButton")))
                    {
                        // FilteredSearchWidget.qss chips: border 1px. TOKENIZE: TagBorderWidth
                        r = option->rect.adjusted(1, 1, -1, -1);
                    }
                    if (r.isNull() && widget && widget->inherits("AzQtComponents::SegmentBar"))
                    {
                        // SegmentControl.qss SegmentBar { margin-bottom: 15px }. TOKENIZE: SegmentBarBottomMargin
                        r = option->rect.adjusted(0, 0, 0, -15);
                    }
                    if (r.isNull())
                    {
                        r = flattenedLabelFrameContents(option, widget);
                    }
                    if (!r.isNull())
                    {
                        return r;
                    }
                }
            }
            break;
            case SE_ItemViewItemText:       // intentional fall-through
            case SE_ItemViewItemDecoration: // intentional fall-through
            case SE_ItemViewItemFocusRect:
            {
                if (StyleManager::stylesheetsDisabled() && element == SE_ItemViewItemText
                    && widget && StyleHelpers::findParent<QComboBox>(widget))
                {
                    // ComboBox.qss QComboBox QAbstractItemView::item { padding-left: 24px;
                    // padding-right: 24px } (room for the delegate's check mark).
                    // TOKENIZE: ComboBoxPopupItemPadding
                    return QProxyStyle::subElementRect(element, option, widget).adjusted(24, 0, -24, 0);
                }
                auto optionItemView = qstyleoption_cast<const QStyleOptionViewItem*>(option);
                if (qobject_cast<const TableView*>(widget) && optionItemView)
                {
                    QRect r = TableView::itemViewItemRect(this, element, optionItemView, widget, m_data->tableViewConfig);
                    if (!r.isNull())
                    {
                        return r;
                    }
                }
            }
            break;
            case SE_LineEditContents:
            {
                QRect r = LineEdit::lineEditContentsRect(this, element, option, widget, m_data->lineEditConfig);
                if (!r.isNull())
                {
                    return r;
                }
            }
            break;
            case SE_TreeViewDisclosureItem:
            {
                auto treeView = static_cast<const QTreeView*>(widget);
                if (TreeView::isBranchLinesEnabled(treeView) && qobject_cast<BranchDelegate*>(treeView->itemDelegate()))
                {
                    auto copy = *option;
                    copy.rect.adjust(treeView->indentation(), 0, treeView->indentation(), 0);
                    return QProxyStyle::subElementRect(element, &copy, widget);
                }
            }
            break;
        }

        return QProxyStyle::subElementRect(element, option, widget);
    }

    int Style::pixelMetric(QStyle::PixelMetric metric, const QStyleOption* option,
        const QWidget* widget) const
    {
        if (!hasStyle(widget))
        {
            return QProxyStyle::pixelMetric(metric, option, widget);
        }

        if (StyleManager::stylesheetsDisabled())
        {
            // Indicator footprints from CheckBox.qss / RadioButton.qss (16x16; toggle switch 32x16)
            switch (metric)
            {
                case PM_IndicatorWidth:
                    return (widget && hasClass(widget, QStringLiteral("ToggleSwitch"))) ? 32 : 16;
                case PM_IndicatorHeight:
                case PM_ExclusiveIndicatorWidth:
                case PM_ExclusiveIndicatorHeight:
                    return 16;
                case PM_MenuHMargin:
                    // Menu.qss QMenu padding: 4px 2px (horizontal component)
                    return 2;
                case PM_MenuVMargin:
                    // Menu.qss QMenu padding: 4px 2px (vertical component)
                    return 4;
                case PM_ScrollBarExtent:
                    // ScrollBar.qss QScrollBar:vertical width / :horizontal height
                    return 8;
                case PM_ScrollBarSliderMin:
                    // ScrollBar.qss handle min-height/min-width
                    return 32;
                case PM_SplitterWidth:
                    // Legacy capture: the handle widget is 5px wide and Splitter.qss draws a
                    // 1px #222222 line in it (CE_Splitter). TOKENIZE: SplitterHandleWidth
                    return 5;
                case PM_ToolBarSeparatorExtent:
                    // ToolBar.qss QToolBar::separator { width: 1px }  TOKENIZE: ToolBarSeparatorWidth
                    return 1;
                case PM_MenuBarVMargin:
                case PM_MenuBarHMargin:
                case PM_MenuBarItemSpacing:
                case PM_MenuBarPanelWidth:
                    // MenuBar.qss QMenuBar { margin: 0; spacing: 0 }; the item box carries the
                    // padding (CT_MenuBarItem) and the 2px bottom border (CE_MenuBar*).
                    return 0;
                case PM_ToolBarIconSize:
                    // ToolBar.qss qproperty-iconSize tiers: default 16, IconLarge 20,
                    // MainToolBar 20, MainToolBar+IconLarge 32
                    if (widget && hasClass(widget, QStringLiteral("MainToolBar")))
                    {
                        return hasClass(widget, QStringLiteral("IconLarge")) ? 32 : 20;
                    }
                    return (widget && hasClass(widget, QStringLiteral("IconLarge"))) ? 20 : 16;
                default:
                    break;
            }
        }

        switch (metric)
        {
            case QStyle::PM_ButtonMargin:
            {
                int margin = ToolButton::buttonMargin(this, option, widget, m_data->toolButtonConfig);
                if (margin != -1)
                {
                    return margin;
                }

                margin = PushButton::buttonMargin(this, option, widget, m_data->pushButtonConfig);
                if (margin != -1)
                {
                    return margin;
                }

                break;
            }

            case QStyle::PM_LayoutLeftMargin:
            case QStyle::PM_LayoutTopMargin:
            case QStyle::PM_LayoutRightMargin:
            case QStyle::PM_LayoutBottomMargin:
                return 5;

            case QStyle::PM_LayoutHorizontalSpacing:
            case QStyle::PM_LayoutVerticalSpacing:
                return 3;

            case QStyle::PM_HeaderDefaultSectionSizeVertical:
                return 24;

            case QStyle::PM_DefaultFrameWidth:
            {
                if (auto button = qobject_cast<const QToolButton*>(widget))
                {
                    if (button->popupMode() == QToolButton::MenuButtonPopup)
                    {
                        return 0;
                    }
                }

                break;
            }

            case QStyle::PM_ButtonIconSize:
            {
                int size = ToolButton::buttonIconSize(this, option, widget, m_data->toolButtonConfig);
                if (size != -1)
                {
                    return size;
                }
                return 24;
                break;
            }

            case QStyle::PM_ToolBarItemSpacing:
            {
                int spacing = ToolBar::itemSpacing(this, option, widget, m_data->toolBarConfig);
                if (spacing != -1)
                {
                    return spacing;
                }
                break;
            }

            case QStyle::PM_DockWidgetSeparatorExtent:
                return 3;
                break;

            case QStyle::PM_SliderThickness:
            case QStyle::PM_SliderControlThickness: // used by qCommonStyle::subControlRect()
            {
                int thickness = Slider::sliderThickness(this, option, widget, m_data->sliderConfig);
                if (thickness != -1)
                {
                    return thickness;
                }
                break;
            }

            case QStyle::PM_SliderLength:
            {
                int length = Slider::sliderLength(this, option, widget, m_data->sliderConfig);
                if (length != -1)
                {
                    return length;
                }
                break;
            }

            case QStyle::PM_TitleBarHeight:
            {
                const int height = TitleBar::titleBarHeight(this, option, widget, m_data->titleBarConfig, m_data->tabWidgetConfig);
                if (height != -1)
                {
                    return height;
                }
                break;
            }

            case QStyle::PM_TabCloseIndicatorWidth:
            case QStyle::PM_TabCloseIndicatorHeight:
            {
                return TabBar::closeButtonSize(this, option, widget, m_data->tabWidgetConfig);
                break;
            }

            case QStyle::PM_MenuButtonIndicator:
            {
                int size = ToolButton::menuButtonIndicatorWidth(this, option, widget, m_data->toolButtonConfig);
                if (size != -1)
                {
                    return size;
                }
                size = PushButton::menuButtonIndicatorWidth(this, option, widget, m_data->pushButtonConfig);
                if (size != -1)
                {
                    return size;
                }
                break;
            }

            case QStyle::PM_SubMenuOverlap:
            {
                const int overlap = Menu::subMenuOverlap(this, option, widget, m_data->menuConfig);
                if (overlap != std::numeric_limits<int>::lowest())
                {
                    return overlap;
                }
                break;
            }

            case QStyle::PM_ToolBarExtensionExtent:
            {
                int retval{ 12 };
                return retval;
            }

            default:
                break;
        }

        return QProxyStyle::pixelMetric(metric, option, widget);
    }

    // Defined with the application-polish section further down.
    static QPalette buildO3DEBasePalette();
    static QPalette buildO3DEMenuPalette(const QPalette& basePalette);

    //////////////////////////////////////////////////////////////////////////
    // Flattened typography (GUI flattening)
    //
    // Text.qss set per-class font sizes and colors on QLabel via the
    // Text::add*Style/add*Color class tags. The app-wide Open Sans 12px font
    // already covers Label/Menu/Paragraph/Tooltip; only the larger tiers and
    // the color classes need explicit owners here.
    //////////////////////////////////////////////////////////////////////////
    static void applyFlattenedTypographyStyle(QWidget* widget)
    {
        if (!widget->inherits("QLabel"))
        {
            return;
        }

        int pixelSize = 0;
        if (Style::hasClass(widget, QStringLiteral("Headline")))
        {
            pixelSize = 24;
        }
        else if (Style::hasClass(widget, QStringLiteral("Title")))
        {
            pixelSize = 18;
        }
        else if (Style::hasClass(widget, QStringLiteral("Subtitle")))
        {
            pixelSize = 16;
        }

        // Text.qss font weights: Headline / Subtitle 250, Button 570 (others 400)
        // TOKENIZE: HeadlineWeight, SubtitleWeight, ButtonTextWeight
        int weight = 0;
        if (Style::hasClass(widget, QStringLiteral("Headline")) || Style::hasClass(widget, QStringLiteral("Subtitle")))
        {
            weight = 250;
        }
        else if (Style::hasClass(widget, QStringLiteral("Button")))
        {
            weight = 570;
        }

        if (pixelSize > 0 || weight > 0)
        {
            QFont classFont = widget->font();
            if (pixelSize > 0)
            {
                classFont.setPixelSize(pixelSize);
            }
            if (weight > 0)
            {
                classFont.setWeight(static_cast<QFont::Weight>(weight));
            }
            widget->setFont(classFont);
        }

        // The class tags arrive after construction; re-read the label's frame box
        // (flattenedLabelFrameContents) now that they are known.
        if (auto frame = qobject_cast<QFrame*>(widget))
        {
            frame->setFrameStyle(frame->frameStyle());
        }

        QColor textColor;
        if (Style::hasClass(widget, QStringLiteral("secondaryText")))
        {
            textColor = QColor(0x88, 0x88, 0x88);
        }
        else if (Style::hasClass(widget, QStringLiteral("highlightedText")))
        {
            textColor = QColor(0x44, 0xB2, 0xF8);
        }
        else if (Style::hasClass(widget, QStringLiteral("blackText")))
        {
            textColor = QColor(0x00, 0x00, 0x00);
        }

        if (textColor.isValid())
        {
            QPalette labelPalette = widget->palette();
            labelPalette.setColor(QPalette::WindowText, textColor);
            labelPalette.setColor(QPalette::Text, textColor);
            widget->setPalette(labelPalette);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Flattened text-entry palette (GUI flattening)
    //
    // Mirrors the BaseStyleSheet.qss rule that gave text-entry widgets a
    // light background with black text (QLineEdit/QSpinBox/QDoubleSpinBox/
    // QComboBox: #CCCCCC + black; QTextEdit/QPlainTextEdit: #E9E9E9 +
    // #545454). Applied per widget class because the app-wide palette keeps
    // QPalette::Base dark for the item views.
    //////////////////////////////////////////////////////////////////////////
    static void applyFlattenedTextEntryPalette(QWidget* widget)
    {
        const bool isLineEntry = widget->inherits("QLineEdit") || widget->inherits("QAbstractSpinBox") || widget->inherits("QComboBox");
        const bool isTextEdit = widget->inherits("QTextEdit") || widget->inherits("QPlainTextEdit");

        if (!isLineEntry && !isTextEdit)
        {
            return;
        }

        // The line edit inside a BrowseEdit or a spin box has no field of its own in the
        // sheet (transparent over the parent's box); BrowseEdit::polish / SpinBox::polish own it.
        const QWidget* parent = widget->parentWidget();
        if (widget->inherits("QLineEdit") && parent
            && (parent->inherits("AzQtComponents::BrowseEdit") || parent->inherits("QAbstractSpinBox")))
        {
            return;
        }

        QPalette entryPalette = widget->palette();
        const QColor base = isTextEdit ? QColor(0xE9, 0xE9, 0xE9) : QColor(0xCC, 0xCC, 0xCC);
        const QColor text = isTextEdit ? QColor(0x54, 0x54, 0x54) : QColor(Qt::black);

        entryPalette.setColor(QPalette::Active, QPalette::Base, base);
        entryPalette.setColor(QPalette::Inactive, QPalette::Base, base);
        entryPalette.setColor(QPalette::Active, QPalette::Text, text);
        entryPalette.setColor(QPalette::Inactive, QPalette::Text, text);
        entryPalette.setColor(QPalette::Disabled, QPalette::Base, QColor(0x66, 0x66, 0x66));   // QLineEdit:disabled background
        entryPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(0x99, 0x99, 0x99));   // QLineEdit:disabled color

        if (isTextEdit)
        {
            // The sheet's QTextEdit background covered the whole widget, scroll bar lanes
            // included, and the widget carried no frame (legacy capture: field flush to the
            // edge, scroll bar drawn over the light field). TOKENIZE: TextEditFieldColor
            entryPalette.setColor(QPalette::Window, base);
            widget->setAutoFillBackground(true);
            if (auto frame = qobject_cast<QFrame*>(widget))
            {
                frame->setFrameShape(QFrame::NoFrame);
            }
        }

        if (widget->inherits("QComboBox") || widget->inherits("QAbstractSpinBox"))
        {
            // Fusion paints the closed combo face / spin buttons with Button/ButtonText;
            // keep them light like the QSS look so labels are not black-on-dark.
            entryPalette.setColor(QPalette::Active, QPalette::Button, base);
            entryPalette.setColor(QPalette::Inactive, QPalette::Button, base);
            entryPalette.setColor(QPalette::Active, QPalette::ButtonText, text);
            entryPalette.setColor(QPalette::Inactive, QPalette::ButtonText, text);
            entryPalette.setColor(QPalette::Disabled, QPalette::Button, QColor(0x66, 0x66, 0x66));
            entryPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x99, 0x99, 0x99));
        }

        widget->setPalette(entryPalette);
    }

    void Style::polish(QWidget* widget)
    {
        static QWidget* alreadyStyling = nullptr;
        if (alreadyStyling == widget)
        {
            return;
        }

        QScopedValueRollback<QWidget*> recursionGuard(alreadyStyling, widget);

        if (StyleManager::stylesheetsDisabled())
        {
            applyFlattenedTextEntryPalette(widget);

            // Combo box popups share the menu chrome (Menu.qss #222222); the
            // container's palette propagates to the popup list view inside it.
            if (widget->inherits("QComboBoxPrivateContainer"))
            {
                widget->setPalette(buildO3DEMenuPalette(widget->palette()));
            }

            // MenuBar.qss: font-family "Amazon Ember" 12px (the sheet's family, which the
            // platform resolves the same way it did for the sheet). TOKENIZE: MenuBarFont
            if (widget->inherits("QMenuBar"))
            {
                QFont menuBarFont = widget->font();
                menuBarFont.setFamily(QStringLiteral("Amazon Ember"));
                menuBarFont.setPixelSize(12);
                widget->setFont(menuBarFont);
            }

            // TableView.qss: QTableView/QListView/QTreeView { border: none } and
            // QTableView { qproperty-showGrid: false }. TOKENIZE: ItemViewFrame, TableViewGrid
            if (auto itemView = qobject_cast<QAbstractItemView*>(widget))
            {
                itemView->setFrameShape(QFrame::NoFrame);
                if (auto tableView = qobject_cast<QTableView*>(widget))
                {
                    tableView->setShowGrid(false);
                }
            }

            // ScrollBar.qss: QAbstractScrollArea > .QWidget { background: none } -- a plain
            // QScrollArea's viewport is transparent, so the scroll area's own background
            // (e.g. a page's inline white sheet) shows through. TOKENIZE: ScrollAreaViewport
            if (auto scrollArea = qobject_cast<QScrollArea*>(widget))
            {
                // Under the universal `* { background-color: transparent }` rule the viewport
                // and a plain contents QWidget painted nothing of their own.
                QPalette viewportPalette = scrollArea->viewport()->palette();
                viewportPalette.setColor(QPalette::Window, Qt::transparent);
                viewportPalette.setColor(QPalette::Base, Qt::transparent);
                scrollArea->viewport()->setPalette(viewportPalette);
                scrollArea->viewport()->setAutoFillBackground(false);
                if (QWidget* contents = scrollArea->widget())
                {
                    if (QString::fromLatin1(contents->metaObject()->className()) == QLatin1String("QWidget"))
                    {
                        contents->setAutoFillBackground(false);
                    }
                }
            }

            // Progress bar track and fill (ProgressBar.qss: track #9A9A9A, chunk #1E70EB,
            // height 5px)
            if (widget->inherits("QProgressBar"))
            {
                QPalette progressPalette = widget->palette();
                progressPalette.setColor(QPalette::Base, QColor(0x9A, 0x9A, 0x9A));
                progressPalette.setColor(QPalette::Highlight, QColor(0x1E, 0x70, 0xEB));
                progressPalette.setColor(QPalette::HighlightedText, QColor(Qt::white));
                widget->setPalette(progressPalette);
                widget->setMaximumHeight(5);
            }

            // Scroll bars need WA_Hover for the hover lane fill; in qss mode the
            // CSS engine enabled it automatically for any widget with a :hover rule.
            if (widget->inherits("QScrollBar"))
            {
                widget->setAttribute(Qt::WA_Hover, true);
            }

            // Splitter handles need WA_Hover for the hover fill, same as scroll bars
            if (widget->inherits("QSplitterHandle"))
            {
                widget->setAttribute(Qt::WA_Hover, true);
            }

            // Toolbars: ToolBar.qss iconSize tiers + homogeneous 5px padding.
            // Set explicitly - QToolBar caches its icon size.
            if (auto toolBar = qobject_cast<QToolBar*>(widget))
            {
                int iconExtent = 16;
                if (hasClass(widget, QStringLiteral("MainToolBar")))
                {
                    iconExtent = hasClass(widget, QStringLiteral("IconLarge")) ? 32 : 20;
                }
                else if (hasClass(widget, QStringLiteral("IconLarge")))
                {
                    iconExtent = 20;
                }
                toolBar->setIconSize(QSize(iconExtent, iconExtent));
                if (toolBar->layout())
                {
                    toolBar->layout()->setContentsMargins(5, 5, 5, 5);
                }
            }

            // TabWidget.qss: the TabWidget itself is the #111111 backdrop; the stacked
            // page area sits on #444444
            if (widget->inherits("AzQtComponents::TabWidget"))
            {
                // TabWidget.qss: TabWidget.Secondary { background-color: #444444 }
                QPalette tabWidgetPalette = widget->palette();
                tabWidgetPalette.setColor(QPalette::Window, hasClass(widget, QStringLiteral("Secondary")) ? QColor(0x44, 0x44, 0x44) : QColor(0x11, 0x11, 0x11));
                widget->setPalette(tabWidgetPalette);
                widget->setAutoFillBackground(true);
            }
            if (widget->inherits("QStackedWidget") && widget->parentWidget()
                && widget->parentWidget()->inherits("AzQtComponents::TabWidget"))
            {
                QPalette pageAreaPalette = widget->palette();
                pageAreaPalette.setColor(QPalette::Window, QColor(0x44, 0x44, 0x44));
                widget->setPalette(pageAreaPalette);
                widget->setAutoFillBackground(true);
            }

            // Tab bars: qss-era #111111 row backdrop + no native base line. DockTabBar
            // (docked panel strips) keeps its own treatment.
            if (widget->inherits("QTabBar") && !widget->inherits("AzQtComponents::DockTabBar"))
            {
                if (auto tabBar = qobject_cast<QTabBar*>(widget))
                {
                    tabBar->setDrawBase(false);
                }
                // TabWidget.qss: TabBar.Secondary rows sit on the #444444 tab widget
                QPalette tabPalette = widget->palette();
                tabPalette.setColor(QPalette::Window, hasClass(widget, QStringLiteral("Secondary")) ? QColor(0x44, 0x44, 0x44) : QColor(0x11, 0x11, 0x11));
                widget->setPalette(tabPalette);
                widget->setAutoFillBackground(true);
            }

            // Headers: TableView.qss flat #2d2d2d sections (Fusion draws from Button)
            if (widget->inherits("QHeaderView"))
            {
                QPalette headerPalette = widget->palette();
                headerPalette.setColor(QPalette::Button, QColor(0x2D, 0x2D, 0x2D));
                headerPalette.setColor(QPalette::ButtonText, QColor(0xCC, 0xCC, 0xCC));
                // What shows through a section painted transparent by an inline sheet
                // (e.g. the Gallery log table) is the window colour, as in the legacy render.
                headerPalette.setColor(QPalette::Window, QColor(0x44, 0x44, 0x44));
                headerPalette.setColor(QPalette::Base, QColor(0x44, 0x44, 0x44));
                widget->setPalette(headerPalette);
            }

            // Text.qss class-driven typography
            applyFlattenedTypographyStyle(widget);

            // Scroll area corner tile: mirrors the qss [ShowBackground=true] rule.
            // The watcher toggles the property on hover and re-polishes.
            if (widget->property("ShowBackground").isValid())
            {
                const QWidget* cornerScrollArea = widget->parentWidget();
                const bool darkCorner = cornerScrollArea && Style::hasClass(cornerScrollArea, QStringLiteral("DarkScrollBar"));
                const bool showCorner = widget->property("ShowBackground").toBool();

                QPalette cornerPalette = widget->palette();
                cornerPalette.setColor(QPalette::Window, showCorner
                    ? (darkCorner ? QColor(0xDC, 0xDC, 0xDC) : QColor(0x55, 0x55, 0x55))
                    : QColor(Qt::transparent));
                widget->setAutoFillBackground(showCorner);
                widget->setPalette(cornerPalette);
            }
        }

        if (hasStyle(widget))
        {
            bool polishedAlready = false;

            polishedAlready = polishedAlready || PushButton::polish(this, widget, m_data->pushButtonConfig);
            polishedAlready = polishedAlready || CheckBox::polish(this, widget, m_data->checkBoxConfig);
            polishedAlready = polishedAlready || RadioButton::polish(this, widget, m_data->radioButtonConfig);
            polishedAlready = polishedAlready || Slider::polish(this, widget, m_data->sliderConfig);
            polishedAlready = polishedAlready || Card::polish(this, widget, m_data->cardConfig);
            polishedAlready = polishedAlready || ColorPicker::polish(this, widget, m_data->colorPickerConfig);
            polishedAlready = polishedAlready || Eyedropper::polish(this, widget, m_data->eyedropperConfig);
            polishedAlready = polishedAlready || BreadCrumbs::polish(this, widget, m_data->breadCrumbsConfig);
            polishedAlready = polishedAlready || PaletteView::polish(this, widget, m_data->paletteViewConfig);
            polishedAlready = polishedAlready || VectorElement::polish(this, widget, m_data->spinBoxConfig);
            polishedAlready = polishedAlready || SpinBox::polish(this, widget, m_data->spinBoxConfig);
            polishedAlready = polishedAlready || LineEdit::polish(this, widget, m_data->lineEditConfig);
            polishedAlready = polishedAlready || BrowseEdit::polish(this, widget, m_data->browseEditConfig, m_data->lineEditConfig);
            polishedAlready = polishedAlready || ComboBox::polish(this, widget, m_data->comboBoxConfig);
            polishedAlready = polishedAlready || AssetFolderThumbnailView::polish(this, widget, m_data->scrollBarConfig, m_data->assetFolderThumbnailViewConfig);
            polishedAlready = polishedAlready || FilteredSearchWidget::polish(this, widget, m_data->filteredSearchWidgetConfig);
            polishedAlready = polishedAlready || TableView::polish(this, widget, m_data->scrollBarConfig, m_data->tableViewConfig);
            polishedAlready = polishedAlready || TitleBar::polish(this, widget, m_data->titleBarConfig);
            polishedAlready = polishedAlready || TabBar::polish(this, widget, m_data->tabWidgetConfig);
            polishedAlready = polishedAlready || TabWidget::polish(this, widget, m_data->tabWidgetConfig);
            polishedAlready = polishedAlready || Menu::polish(this, widget, m_data->menuConfig);
            polishedAlready = polishedAlready || ToolButton::polish(this, widget, m_data->toolButtonConfig);
            polishedAlready = polishedAlready || StyledBusyLabel::polish(this, widget);
            polishedAlready = polishedAlready || ToolBar::polish(this, widget, m_data->toolBarConfig);
            polishedAlready = polishedAlready || TreeView::polish(this, widget, m_data->scrollBarConfig, m_data->treeViewConfig);
            polishedAlready = polishedAlready || DialogButtonBox::polish(this, widget);

            // A number of classes derive from QAbstractScrollArea. If one of these classes requires
            // polishing ensure that their polish function calls ScrollBar::polish.
            // ScrollBar::polish must be done last otherwise it will trap the event before it gets
            // to derived classes.
            polishedAlready = polishedAlready || ScrollBar::polish(this, widget, m_data->scrollBarConfig);
        }

        QProxyStyle::polish(widget);
    }

    void Style::unpolish(QWidget* widget)
    {
        static QWidget* alreadyUnStyling = nullptr;
        if (alreadyUnStyling == widget)
        {
            return;
        }

        QScopedValueRollback<QWidget*> recursionGuard(alreadyUnStyling, widget);

        if (hasStyle(widget))
        {
            bool unpolishedAlready = false;

            unpolishedAlready = unpolishedAlready || Card::unpolish(this, widget, m_data->cardConfig);
            unpolishedAlready = unpolishedAlready || VectorElement::unpolish(this, widget, m_data->spinBoxConfig);
            unpolishedAlready = unpolishedAlready || SpinBox::unpolish(this, widget, m_data->spinBoxConfig);
            unpolishedAlready = unpolishedAlready || LineEdit::unpolish(this, widget, m_data->lineEditConfig);
            unpolishedAlready = unpolishedAlready || BrowseEdit::unpolish(this, widget, m_data->browseEditConfig, m_data->lineEditConfig);
            unpolishedAlready = unpolishedAlready || ComboBox::unpolish(this, widget, m_data->comboBoxConfig);
            unpolishedAlready = unpolishedAlready || FilteredSearchWidget::unpolish(this, widget, m_data->filteredSearchWidgetConfig);
            unpolishedAlready = unpolishedAlready || TableView::unpolish(this, widget, m_data->scrollBarConfig, m_data->tableViewConfig);
            unpolishedAlready = unpolishedAlready || TitleBar::unpolish(this, widget, m_data->titleBarConfig);
            unpolishedAlready = unpolishedAlready || TabBar::unpolish(this, widget, m_data->tabWidgetConfig);
            unpolishedAlready = unpolishedAlready || TabWidget::unpolish(this, widget, m_data->tabWidgetConfig);
            unpolishedAlready = unpolishedAlready || Menu::unpolish(this, widget, m_data->menuConfig);
            unpolishedAlready = unpolishedAlready || StyledBusyLabel::unpolish(this, widget);

            // A number of classes derive from QAbstractScrollArea. If one of these classes requires
            // unpolishing ensure that their unpolish function calls ScrollBar::polish.
            // ScrollBar::unpolish must be done last otherwise it will trap the event before it gets
            // to derived classes.
            unpolishedAlready = unpolishedAlready || ScrollBar::unpolish(this, widget, m_data->scrollBarConfig);
        }

        QProxyStyle::unpolish(widget);
    }
    
    void Style::polish(QPalette& palette)
    {
        QProxyStyle::polish(palette);

        if (StyleManager::stylesheetsDisabled())
        {
            // Every palette Qt derives (including system color-scheme changes)
            // passes through here - replacing it wholesale is what guarantees
            // the engine's forced theme and disables reactive dark/light mode.
            palette = buildO3DEBasePalette();
        }
    }
    
    void Style::unpolish(QApplication* application)
    {
        QProxyStyle::unpolish(application);
    }

    QPalette Style::standardPalette() const
    {
        return m_data->palette;
    }

    QIcon Style::standardIcon(QStyle::StandardPixmap standardIcon, const QStyleOption* option, const QWidget* widget) const
    {
        if (!hasStyle(widget))
        {
            return QProxyStyle::standardIcon(standardIcon, option, widget);
        }

        switch (standardIcon)
        {
            case QStyle::SP_LineEditClearButton:
            {
                const QLineEdit* le = qobject_cast<const QLineEdit*>(widget);
                if (le)
                {
                    return LineEdit::clearButtonIcon(option, widget, m_data->lineEditConfig);
                }
            }
            break;

            case QStyle::SP_MessageBoxInformation:
                return QIcon(QString::fromUtf8(":/stylesheet/img/UI20/Info.svg"));
                break;

            // Title bar button glyphs (TitleBar.qss titlebar-*-icon rules). Only
            // reachable when no QStyleSheetStyle intercepts standardIcon, i.e. in
            // flattened mode - same pattern as SP_MessageBoxInformation above.
            case QStyle::SP_TitleBarCloseButton:
                return QIcon(QStringLiteral(":/Application/titlebar-close.svg"));

            case QStyle::SP_TitleBarMinButton:
                return QIcon(QStringLiteral(":/Application/titlebar-minimize.svg"));

            case QStyle::SP_TitleBarNormalButton:
                return QIcon(QStringLiteral(":/Application/titlebar-restore.svg"));

            case QStyle::SP_TitleBarMaxButton:
                return QIcon(Style::hasClass(widget, QStringLiteral("restore"))
                    ? QStringLiteral(":/Application/titlebar-restore.svg")
                    : QStringLiteral(":/Application/titlebar-maximize.svg"));

            default:
                break;
        }
        return QProxyStyle::standardIcon(standardIcon, option, widget);
    }

    int Style::styleHint(QStyle::StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData) const
    {
        if (hint == QStyle::SH_SpinBox_StepModifier)
        {
            return Qt::ShiftModifier;
        }

        // Fusion defaults SH_ComboBox_Popup to menu-mode, which draws a checkmark on
        // the current item stacked over the field. The qss-era rendering was list-mode
        // (no checkmarks, pre-highlighted current row) - force it.
        if (hint == QStyle::SH_ComboBox_Popup && StyleManager::stylesheetsDisabled())
        {
            return 0;
        }

        if (!hasStyle(widget))
        {
            return QProxyStyle::styleHint(hint, option, widget, returnData);
        }

        if (hint == QStyle::SH_Slider_AbsoluteSetButtons)
        {
            return Slider::styleHintAbsoluteSetButtons();
        }
        else if (hint == QStyle::SH_Menu_SubMenuPopupDelay)
        {
            // Default to sub-menu pop-up delay of 0 (for instant drawing of submenus, Qt defaults to 225 ms)
            const int defaultSubMenuPopupDelay = 0;
            return defaultSubMenuPopupDelay;
        }
        else if (hint == QStyle::SH_ComboBox_PopupFrameStyle)
        {
            // We want popup like combobox to have no frame
            return QFrame::NoFrame;
        }
        else if (hint == QStyle::SH_ComboBox_Popup)
        {
            // We want popup like combobox
            return 0;
        }
        else if (hint == QStyle::SH_ComboBox_UseNativePopup)
        {
            // We want non native popup like combobox
            return 0;
        }

        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }

    QPainterPath Style::borderLineEditRect(const QRect& contentsRect, int borderWidth, int borderRadius) const
    {
        const auto borderAdjustment = borderRadius - borderWidth;
        QPainterPath pathRect;

        if (borderRadius != BorderStyle::CORNER_RECTANGLE)
        {
            const auto radius = borderRadius + borderWidth;
            pathRect.addRoundedRect(contentsRect.adjusted(borderAdjustment,
                                        borderAdjustment,
                                        -borderAdjustment,
                                        -borderAdjustment),
                radius, radius);
        }
        else
        {
            pathRect.addRect(contentsRect.adjusted(borderAdjustment,
                borderAdjustment,
                -borderAdjustment,
                -borderAdjustment));
        }

        return pathRect;
    }

    QPainterPath Style::lineEditRect(const QRect& contentsRect, int borderWidth, int borderRadius) const
    {
        QPainterPath pathRect;

        if (borderRadius != BorderStyle::CORNER_RECTANGLE)
        {
            pathRect.addRoundedRect(contentsRect.adjusted(borderWidth,
                                        borderWidth,
                                        -borderWidth,
                                        -borderWidth),
                borderRadius, borderRadius);
        }
        else
        {
            pathRect.addRect(contentsRect.adjusted(borderWidth,
                borderWidth,
                -borderWidth,
                -borderWidth));
        }

        return pathRect;
    }

    void Style::repolishOnSettingsChange(QWidget* widget)
    {
        // don't listen twice for the settingsReloaded signal on the same widget
        if (m_data->widgetsToRepolishOnReload.contains(widget))
        {
            return;
        }

        m_data->widgetsToRepolishOnReload.insert(widget);

        // Qt::UniqueConnection doesn't work with lambdas, so we have to track this ourselves

        QObject::connect(widget, &QObject::destroyed, this, &Style::repolishWidgetDestroyed);
        QObject::connect(this, &Style::settingsReloaded, widget, [widget]() {
            widget->style()->unpolish(widget);
            widget->style()->polish(widget);
        });
    }

    bool Style::eventFilter(QObject* watched, QEvent* ev)
    {
        switch (ev->type())
        {
            case QEvent::ToolTipChange:
            {
                if (QWidget* w = qobject_cast<QWidget*>(watched))
                {
                    forceToolTipLineWrap(w);
                }
            }
            break;
        }

        const bool flagToContinueProcessingEvent = false;
        return flagToContinueProcessingEvent;
    }

    bool Style::hasClass(const QWidget* button, const QString& className)
    {
        QVariant buttonClassVariant = button->property("class");
        if (buttonClassVariant.isNull())
        {
            return false;
        }

        QString classText = buttonClassVariant.toString();
        QStringList classList = classText.split(QRegularExpression("\\s+"));
        return classList.contains(className, Qt::CaseInsensitive);
    }

    void Style::addClass(QWidget* button, const QString& className)
    {
        QVariant buttonClassVariant = button->property("class");
        if (buttonClassVariant.isNull())
        {
            button->setProperty("class", className);
        }
        else
        {
            QString classText = buttonClassVariant.toString();
            classText.append(QStringLiteral(" %1").arg(className));
            button->setProperty("class", classText);
        }

        button->style()->unpolish(button);
        button->style()->polish(button);
    }

    void Style::removeClass(QWidget* button, const QString& className)
    {
        QVariant buttonClassVariant = button->property("class");
        if (!buttonClassVariant.isNull())
        {
            const QString classText = buttonClassVariant.toString();
            QStringList classList = classText.split(QRegularExpression("\\s+"));
            bool changed = false;
            for (int i = static_cast<int>(classList.count()) -1; i >= 0; --i)
            {
                if (classList[i].compare(className, Qt::CaseInsensitive) == 0)
                {
                    classList.removeAt(i);
                    changed = true;
                }
            }
            if (changed)
            {
                button->setProperty("class", classList.join(QLatin1Char(' ')));
                button->style()->unpolish(button);
                button->style()->polish(button);
            }
        }
    }

    QPixmap Style::cachedPixmap(const QString& name)
    {
        QPixmap pixmap;

        if (!QPixmapCache::find(name, &pixmap))
        {
            pixmap = QPixmap(name);
            QPixmapCache::insert(name, pixmap);
        }

        return pixmap;
    }

    void Style::drawFrame(QPainter* painter, const QPainterPath& frameRect, const QPen& border, const QBrush& background)
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(border);
        painter->setBrush(background);
        painter->drawPath(frameRect);
        painter->restore();
    }

    void Style::flagToIgnore(QWidget* widget)
    {
        widget->setProperty(g_removeAllStylingProperty, true);
    }

    void Style::removeFlagToIgnore(QWidget* widget)
    {
        widget->setProperty(g_removeAllStylingProperty, QVariant());
    }

    bool Style::hasStyle(const QWidget* widget)
    {
        return (widget == nullptr) || widget->property(g_removeAllStylingProperty).isNull();
    }

    void Style::prepPainter(QPainter* painter)
    {
        // HACK:
        // QPainter is not guaranteed to have its QPaintEngine initialized in setRenderHint,
        // so go ahead and call save/restore here which ensures that.
        // See: QTBUG-51247
        painter->save();
        painter->restore();
    }

    void Style::fixProxyStyle(QProxyStyle* proxyStyle, QStyle* baseStyle)
    {
        QStyle* applicationStyle = qApp->style();
        QObject* oldParent = applicationStyle->parent();
        proxyStyle->setBaseStyle(baseStyle);
        if (baseStyle == applicationStyle)
        {
            // WORKAROUND: A QProxyStyle over qApp->style() is bad practice as both classes want the ownership over the base style, leading to possible crashes
            // Ideally all this custom styling should be moved to Style.cpp, as a new "style class"
            applicationStyle->setParent(oldParent); // Restore damage done by QProxyStyle
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // O3DE base application palette (GUI flattening)
    //
    // The single explicit owner of every application-wide color in the
    // flattened (no style sheet) mode. Built FROM SCRATCH - never derived
    // from the incoming system palette - because O3DE does not do reactive
    // OS dark/light theming: the engine FORCES its theme, and a "dark theme"
    // is simply a theme that is dark. Every value below mirrors a rule that
    // BaseStyleSheet.qss used to apply - the source rule is noted per line
    // and tracked in the flatten ledger. Fusion consumes this palette for all
    // base widget painting.
    //
    // Note on QPalette::Base: O3DE's look is dark item views with LIGHT text
    // entry fields, but Qt shares the Base role between both. The app-wide
    // palette keeps Base dark for the views; text-entry widget classes get
    // their light palette in Style::polish(QWidget*) below, mirroring the
    // QLineEdit/QSpinBox/QComboBox rule from BaseStyleSheet.qss.
    //////////////////////////////////////////////////////////////////////////
    static QPalette buildO3DEBasePalette()
    {
        QPalette palette;
        // Core surfaces
        const QColor window(0x44, 0x44, 0x44);          // QMainWindow/QDialog/QDockWidget background-color: #444444
        const QColor viewBase(0x44, 0x44, 0x44);        // parentless QTableView/QListView/QTreeView background-color: #444444
        const QColor alternateBase(0x4D, 0x4D, 0x4D);   // TableView.qss alternate-background-color: rgb(77,77,77)
        const QColor textWhite(0xFF, 0xFF, 0xFF);       // global '*' rule color: white
        const QColor disabledText(0x99, 0x99, 0x99);    // QLineEdit:disabled color: #999999
        const QColor disabledBase(0x66, 0x66, 0x66);    // QLineEdit:disabled background-color: #666666

        // Buttons and the 3D bevel ladder (Fusion derives frame/bevel shading from these)
        const QColor button(0x55, 0x55, 0x55);          // mid button surface (Original shared palette)
        const QColor light(0x66, 0x66, 0x66);
        const QColor midlight(0x55, 0x55, 0x55);
        const QColor mid(0x33, 0x33, 0x33);
        const QColor dark(0x22, 0x22, 0x22);
        const QColor shadow(0x11, 0x11, 0x11);          // QMainWindow:separator background-color: #111111

        // Selection (TableView.qss selection-background-color / selection-color)
        const QColor highlight(0x65, 0x65, 0x65);       // rgb(101,101,101)
        const QColor highlightedText(0xFF, 0xFF, 0xFF);

        // Tooltips (ToolTip.qss)
        const QColor toolTipBase(0x00, 0x00, 0x00);
        const QColor toolTipText(0xFF, 0xFF, 0xFF);

        // Hyperlinks (Text config hyperlinkColor re-applies over this at app polish)
        const QColor link(0x94, 0xD2, 0xFF);

        // Active and Inactive groups render identically in O3DE
        for (const auto group : { QPalette::Active, QPalette::Inactive })
        {
            palette.setColor(group, QPalette::Window, window);
            palette.setColor(group, QPalette::WindowText, textWhite);
            palette.setColor(group, QPalette::Base, viewBase);
            palette.setColor(group, QPalette::AlternateBase, alternateBase);
            palette.setColor(group, QPalette::Text, textWhite);
            palette.setColor(group, QPalette::PlaceholderText, disabledText);
            palette.setColor(group, QPalette::Button, button);
            palette.setColor(group, QPalette::ButtonText, textWhite);
            palette.setColor(group, QPalette::BrightText, textWhite);
            palette.setColor(group, QPalette::Light, light);
            palette.setColor(group, QPalette::Midlight, midlight);
            palette.setColor(group, QPalette::Mid, mid);
            palette.setColor(group, QPalette::Dark, dark);
            palette.setColor(group, QPalette::Shadow, shadow);
            palette.setColor(group, QPalette::Highlight, highlight);
            palette.setColor(group, QPalette::HighlightedText, highlightedText);
            palette.setColor(group, QPalette::ToolTipBase, toolTipBase);
            palette.setColor(group, QPalette::ToolTipText, toolTipText);
            palette.setColor(group, QPalette::Link, link);
            palette.setColor(group, QPalette::LinkVisited, link);
        }

        // Disabled group
        palette.setColor(QPalette::Disabled, QPalette::Window, window);
        palette.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
        palette.setColor(QPalette::Disabled, QPalette::Base, disabledBase);
        palette.setColor(QPalette::Disabled, QPalette::AlternateBase, alternateBase);
        palette.setColor(QPalette::Disabled, QPalette::Text, disabledText);
        palette.setColor(QPalette::Disabled, QPalette::PlaceholderText, disabledText);
        palette.setColor(QPalette::Disabled, QPalette::Button, button);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);
        palette.setColor(QPalette::Disabled, QPalette::BrightText, textWhite);
        palette.setColor(QPalette::Disabled, QPalette::Light, light);
        palette.setColor(QPalette::Disabled, QPalette::Midlight, midlight);
        palette.setColor(QPalette::Disabled, QPalette::Mid, mid);
        palette.setColor(QPalette::Disabled, QPalette::Dark, dark);
        palette.setColor(QPalette::Disabled, QPalette::Shadow, shadow);
        palette.setColor(QPalette::Disabled, QPalette::Highlight, highlight);
        palette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledText);
        palette.setColor(QPalette::Disabled, QPalette::ToolTipBase, toolTipBase);
        palette.setColor(QPalette::Disabled, QPalette::ToolTipText, toolTipText);
        palette.setColor(QPalette::Disabled, QPalette::Link, link);
        palette.setColor(QPalette::Disabled, QPalette::LinkVisited, link);

        return palette;
    }

    //////////////////////////////////////////////////////////////////////////
    // Menu popups are core chrome that appears everywhere, so their dark
    // surface (Menu.qss: QMenu background-color #222222, selected item
    // #444444) is owned here as a QMenu class palette rather than waiting
    // for the Menu family batch. Applied AFTER the global setPalette call,
    // which clears class palettes.
    //////////////////////////////////////////////////////////////////////////
    static QPalette buildO3DEMenuPalette(const QPalette& basePalette)
    {
        QPalette menuPalette = basePalette;
        const QColor menuSurface(0x22, 0x22, 0x22);     // Menu.qss QMenu background-color: #222222
        const QColor menuHighlight(0x44, 0x44, 0x44);   // Menu.qss selected item background: #444444
        const QColor menuText(0xFF, 0xFF, 0xFF);
        const QColor menuDisabledText(0x55, 0x55, 0x55);    // Menu.qss disabled item color: #555555

        for (const auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
        {
            menuPalette.setColor(group, QPalette::Window, menuSurface);
            menuPalette.setColor(group, QPalette::Base, menuSurface);
            menuPalette.setColor(group, QPalette::Highlight, menuHighlight);
        }

        // Force text roles: popups can inherit a light text-entry palette (combo box
        // popup containers inherit from the combo box), which would leave near-black
        // text on the dark menu surface.
        for (const auto group : { QPalette::Active, QPalette::Inactive })
        {
            menuPalette.setColor(group, QPalette::Text, menuText);
            menuPalette.setColor(group, QPalette::WindowText, menuText);
            menuPalette.setColor(group, QPalette::ButtonText, menuText);
            menuPalette.setColor(group, QPalette::HighlightedText, menuText);
        }
        menuPalette.setColor(QPalette::Disabled, QPalette::Text, menuDisabledText);
        menuPalette.setColor(QPalette::Disabled, QPalette::WindowText, menuDisabledText);
        menuPalette.setColor(QPalette::Disabled, QPalette::ButtonText, menuDisabledText);
        menuPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, menuDisabledText);

        return menuPalette;
    }

    void Style::polish(QApplication* application)
    {
        Q_UNUSED(application);

        if (StyleManager::stylesheetsDisabled())
        {
            // Forced O3DE palette - never derived from the system palette (no
            // reactive OS dark/light theming; the engine owns its look).
            m_data->palette = buildO3DEBasePalette();
        }
        else
        {
            m_data->palette = application->palette();
        }

        m_data->palette.setColor(QPalette::Link, m_data->textConfig.hyperlinkColor);

        application->setPalette(m_data->palette);

        if (StyleManager::stylesheetsDisabled())
        {
            // Class palettes must be applied after the global setPalette (which clears them)
            application->setPalette(buildO3DEMenuPalette(m_data->palette), "QMenu");

            // Menu bar strip: dark surface (user-verified vs baseline), selected item
            // #555555 (MenuBar.qss)
            QPalette menuBarPalette = m_data->palette;
            const QColor menuBarSurface(0x22, 0x22, 0x22);
            for (const auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
            {
                menuBarPalette.setColor(group, QPalette::Window, menuBarSurface);
                menuBarPalette.setColor(group, QPalette::Base, menuBarSurface);
                menuBarPalette.setColor(group, QPalette::Button, menuBarSurface);
                menuBarPalette.setColor(group, QPalette::Highlight, QColor(0x55, 0x55, 0x55));
            }
            application->setPalette(menuBarPalette, "QMenuBar");
        }

        // need to listen to and fix tooltips so that they wrap
        application->installEventFilter(this);
        application->setEffectEnabled(Qt::UI_AnimateCombo, false);

        QProxyStyle::polish(application);
    }

    void Style::repolishWidgetDestroyed(QObject* obj)
    {
        m_data->widgetsToRepolishOnReload.remove(obj);
    }

#ifdef _DEBUG
    bool Style::event(QEvent* ev)
    {
        if (ev->type() == QEvent::ParentChange)
        {
            // QApplication owns its style. If a QProxyStyle steals it it might crash, as QProxyStyle also owns its base style.
            // Let's assert to detect this early on

            bool ownershipStolenByProxyStyle = (this == qApp->style()) && qobject_cast<QProxyStyle*>(parent());
            Q_ASSERT(!ownershipStolenByProxyStyle);
        }

        return QProxyStyle::event(ev);
    }
#endif

} // namespace AzQtComponents
