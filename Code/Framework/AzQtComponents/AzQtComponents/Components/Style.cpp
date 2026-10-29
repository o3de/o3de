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
#include <AzQtComponents/Components/GuiPerfCounters.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
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
#include <AzQtComponents/Components/Elements/ColorPickerElements.h>
#include <AzQtComponents/Components/Elements/ComboPopupElement.h>
#include <AzQtComponents/Components/Elements/ControlElements.h>
#include <AzQtComponents/Components/Elements/SpinBoxElement.h>
#include <AzQtComponents/Components/Elements/IndicatorElement.h>
#include <AzQtComponents/Components/Elements/MenuElement.h>
#include <AzQtComponents/Components/Elements/FrameElement.h>
#include <AzQtComponents/Components/Elements/Foundations.h>
#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/PropertyEditorElements.h>
#include <AzQtComponents/Components/Elements/TabElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
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
#include <QTabWidget>
#include <QMenu>
#include <QAbstractItemView>
#include <QMainWindow>
#include <QScrollArea>
#include <QSvgRenderer>
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
#include <QPushButton>
#include <QRadioButton>
#include <QScopedValueRollback>
#include <QSet>
#include <QSettings>
#include <QStyleOptionToolButton>
#include <QTableView>
#include <QTextEdit>
#include <QToolButton>
#include <QToolTip>
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

    //////////////////////////////////////////////////////////////////////////
    // Flattened item views and header sections: TableView.qss gave an item its height, and the sections of a header in a QTableView,
    // QListView or QTreeView their height, text and padding (their fill is the item view element's; the label's colour is the
    // universal rule's, the sort arrow the base style's)
    //////////////////////////////////////////////////////////////////////////
    static constexpr int g_itemHeight = 24;                 // TableView.qss:36 $size.item.idle
    static constexpr int g_headerSectionHeight = 24;        // TableView.qss:67 $size.header.idle
    static constexpr int g_headerSectionPaddingLeft = 7;    // TableView.qss:75 $space.header.idle
    static constexpr int g_headerSectionTextSize = 12;      // TableView.qss:72 $type.header.idle.size

    // The legacy sheet laid an item's parts out itself (its check box room, TableView.qss:78-85, CheckBox.qss:23-28), but asked the base
    // style for the item's size and for the rect of a part the item lacks (QStyleSheetStyle::sizeFromContents, subElementRect), where
    // the base style's own check box room is. While this is set, an item's PM_IndicatorWidth and PM_IndicatorHeight are the base style's.
    static int s_itemViewBaseIndicator = 0;

    static bool hasFlattenedHeaderSection(const QWidget* widget)
    {
        return qobject_cast<const QHeaderView*>(widget)
            && (StyleHelpers::findParent<QTableView>(widget) || StyleHelpers::findParent<QListView>(widget)
                || StyleHelpers::findParent<QTreeView>(widget));
    }

    //////////////////////////////////////////////////////////////////////////
    // Flattened search fields (GUI flattening)
    //
    // FilteredSearchWidget.qss:9-13 gave each line edit inside a FilteredSearchWidget a contents width (the box of LineEdit.qss goes
    // around it) and a frame (set at its polish). LineEdit.qss:42-48 gave the buttons of a line edit (search, clear, error) a size, to
    // which the sheet adds a tool button's native border (Qt's default sheet: `QPushButton, QToolButton { border-style: native }`):
    // the base style's PM_DefaultFrameWidth on each side (QFusionStyle::pixelMetric: 1).
    //////////////////////////////////////////////////////////////////////////
    static constexpr int g_searchFieldContentsWidth = 260;     // FilteredSearchWidget.qss:12 $size.field.idle.width
    //! LineEdit.qss:44-47 $size.field.idle.max-width.Alt1 $size.field.idle.min-width.Alt1 $size.field.idle.max-height
    //! $size.field.idle.min-height, and the native border
    static constexpr int g_lineEditActionButtonSize = 16 + 2 * 1;

    //////////////////////////////////////////////////////////////////////////
    // Flattened scroll bars: ScrollBar.qss's lane (transparent, inside a transparent border) and handle, and the lane's fill under the
    // pointer; the bars of a scroll area tagged DarkScrollBar (the console, the Python terminal) have a handle and a fill of their own.
    // No arrow buttons (:49-67). The corner tile between two bars is filled while the watcher shows it (:90-100).
    //////////////////////////////////////////////////////////////////////////
    //! ScrollBar.qss:19, :24 $size.scroll.idle.width $size.scroll.idle.height
    static constexpr int g_scrollBarExtent = 8;
    static constexpr int g_scrollBarBorder = 2;                            // ScrollBar.qss:12 $line.scroll.idle.width
    //! ScrollBar.qss:42, :46 $size.scroll-handle.idle.min-height $size.scroll-handle.idle.min-width
    static constexpr int g_scrollHandleMin = 32;
    static constexpr int g_scrollHandleRadius = 2;                         // ScrollBar.qss:30 $radius.scroll-handle.idle
    static constexpr QColor g_scrollHandleFill(255, 255, 255, 102);        // ScrollBar.qss:31 $fill.scroll-handle.idle.Alt1
    static constexpr QColor g_darkScrollHandleFill(136, 136, 136, 163);    // ScrollBar.qss:38 $fill.scroll-handle.idle.Alt2
    static constexpr QColor g_scrollHoverFill = Foundations::Highest;      // ScrollBar.qss:79 $fill.scroll.hover.Highest
    static constexpr QColor g_darkScrollHoverFill(0xDC, 0xDC, 0xDC);       // ScrollBar.qss:86 $fill.scroll.hover.Alt1
    static constexpr QColor g_cornerFill = Foundations::Highest;           // ScrollBar.qss:93 $fill.surface.hover.Highest
    static constexpr QColor g_darkCornerFill(0xDC, 0xDC, 0xDC);            // ScrollBar.qss:99 $fill.surface.hover.Alt1

    //////////////////////////////////////////////////////////////////////////
    // Flattened chrome: the separators of a main window and of a toolbar, the handle of a splitter, the title strip of a plain dock
    // widget, the chips of a search field, the frame of a decorated window and the margin of a tooltip
    //////////////////////////////////////////////////////////////////////////
    //! BaseStyleSheet.qss:52, ToolBar.qss:73 $fill.separator.idle.FarBG
    static constexpr QColor g_separatorFill = Foundations::FarBG;
    static constexpr QColor g_separatorHoverFill(0x1E, 0x70, 0xEB);        // BaseStyleSheet.qss:57 $fill.separator.hover
    //! ToolBar.qss:74-75 $size.separator.idle.width $size.separator.idle.height.Alt1
    static constexpr int g_toolBarSeparatorWidth = 1;
    static constexpr int g_toolBarSeparatorMargin = 4;                     // ToolBar.qss:77 $space.separator.idle.margin
    static constexpr QColor g_splitterFill = Foundations::BG;              // Splitter.qss:10 $fill.scroll-handle.idle.BG
    static constexpr QColor g_splitterHoverFill(0x1E, 0x70, 0xEB);         // Splitter.qss:23 $fill.scroll-handle.hover
    static constexpr int g_splitterWidth = 1;                              // Splitter.qss:18 $size.scroll-handle.idle.height
    static constexpr QColor g_dockTitleFill = Foundations::Low;            // QDockWidget.qss:22 $fill.title.idle
    static constexpr QColor g_dockTitleText(0xCC, 0xCC, 0xCC);             // BaseStyleSheet.qss:64 $text.surface.idle.Alt2
    static constexpr QColor g_chipFill(46, 46, 46);                        // FilteredSearchWidget.qss:31, :73 $fill.button.idle.fill.Alt1
    static constexpr QColor g_chipLine(0x80, 0x80, 0x80);                  // FilteredSearchWidget.qss:30, :72 $line.button.idle.color.Alt1
    static constexpr int g_chipLineWidth = 1;                              // FilteredSearchWidget.qss:30, :72 $line.button.idle.width.Alt2
    static constexpr qreal g_chipRadius = 2.0;                             // FilteredSearchWidget.qss:29, :71 $radius.button.idle.all.Alt1
    static constexpr int g_segmentBarMarginBottom = 15;                    // SegmentControl.qss:11 $space.surface.idle.margin-bottom.Alt3
    static constexpr int g_windowFrameWidth = 1;                           // WindowDecorationWrapper.qss:11 $line.surface.idle.width.Alt1
    static constexpr QMargins g_toolTipMargins(2, 4, 0, 4);                // ToolTip.qss:15, :17-18 (margin-left, -top, -bottom)

    //! The breadcrumbs' path edit: the second page of the stack beside the label (BreadCrumbs.cpp:93-102)
    static bool isBreadCrumbsPathEdit(const QWidget* widget)
    {
        return widget->inherits("QLineEdit") && Selectors::childOfClass(widget, "QStackedWidget")
            && Selectors::childOfClass(widget->parentWidget(), "AzQtComponents::BreadCrumbs");
    }

    //////////////////////////////////////////////////////////////////////////
    // Passthrough (GuiPerfFloor, the last rung of the ladder)
    //
    // Every override of QProxyStyle below starts with the same few lines: at
    // the passthrough rung (--style-passthrough) it hands the call to the base
    // style and returns, so the flattened styling of this class takes no part
    // and what remains is the cost of the base style (Fusion) alone. Style
    // stays the application style all the same: code casts the application
    // style to it (EntityPropertyEditor.cpp, PropertiesContainer.cpp of the
    // LyShine editor) and calls drawDragIndicator, which needs no override.
    //////////////////////////////////////////////////////////////////////////

    QSize Style::sizeFromContents(QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget) const
    {
        if (GuiPerfFloor::isPassthrough())
        {
            return QProxyStyle::sizeFromContents(type, option, size, widget);
        }

        // SegmentBar buttons carry flagToIgnore (hasStyle == false) - size them before the gate (SegmentControl.qss:14-33)
        if (StyleManager::stylesheetsDisabled() && type == CT_PushButton)
        {
            const QSize segmentSize = ControlElements::segmentButtonSize(size, widget);
            if (segmentSize.isValid())
            {
                return segmentSize;
            }
        }

        // The line edits of a BrowseEdit and of the breadcrumbs carry flagToIgnore (hasStyle == false): the field's height, no border
        // (BrowseEdit.qss:18, BreadCrumbs.qss:26-31), and a vertical padding that adds up to nothing (LineEdit.qss:18-19)
        if (StyleManager::stylesheetsDisabled() && type == CT_LineEdit && widget
            && (Selectors::childOfClass(widget, "AzQtComponents::BrowseEdit") || isBreadCrumbsPathEdit(widget)))
        {
            return QSize(QProxyStyle::sizeFromContents(type, option, size, widget).width(), Foundations::Field::Height);
        }

        if (!hasStyle(widget))
        {
            return QProxyStyle::sizeFromContents(type, option, size, widget);
        }

        if (StyleManager::stylesheetsDisabled())
        {
            switch (type)
            {
                case CT_ScrollBar:
                {
                    // QScrollBar::sizeHint asks for twice PM_ScrollBarExtent plus PM_ScrollBarSliderMin along the bar, and the legacy style
                    // answered the extent with the rule's box, the border on each side included (QStyleSheetStyle::pixelMetric,
                    // PM_ScrollBarExtent: QRenderRule::boxSize), before it set the thickness back to the declared one
                    // (QStyleSheetStyle::sizeFromContents, CT_ScrollBar: QRenderRule::adjustSize); every scroll area's minimum follows
                    QSize scrollBarSize = size;
                    if (auto sliderOption = qstyleoption_cast<const QStyleOptionSlider*>(option))
                    {
                        int& length = sliderOption->orientation == Qt::Horizontal ? scrollBarSize.rwidth() : scrollBarSize.rheight();
                        length += 2 * 2 * g_scrollBarBorder;
                    }
                    return scrollBarSize;
                }
                case CT_ItemViewItem:
                {
                    // TableView.qss:31-40 (the item view element; QStyleSheetStyle::sizeFromContents, CT_ItemViewItem)
                    const ItemViewElement* items = ItemViewElement::forWidget(widget);
                    ++s_itemViewBaseIndicator;
                    QSize itemSize = QProxyStyle::sizeFromContents(type, option, size, widget);
                    --s_itemViewBaseIndicator;
                    if (items)
                    {
                        return items->itemSize(itemSize);
                    }
                    // A view no element covers (a combo box popup's list): the item rule's height at least
                    itemSize.setHeight(qMax(itemSize.height(), g_itemHeight));
                    return itemSize;
                }
                case CT_TabBarTab:
                {
                    // The tab element sizes the tab around Qt's contents (text and tab buttons)
                    if (const TabElement* tabElement = TabElement::forTabBar(widget))
                    {
                        return tabElement->tabSize(size);
                    }
                    // A plain tab keeps Qt's size, its width bound by the legacy rule (TabWidget.qss:16)
                    if (const TabElement* plain = TabElement::plainForTabBar(widget))
                    {
                        return QProxyStyle::sizeFromContents(type, option, plain->tabSize(size), widget);
                    }
                }
                break;
                case CT_PushButton:
                {
                    // The element sizes a button as its rule did (QStyleSheetStyle::sizeFromContents, CT_PushButton): the box around the
                    // contents within the declared size where the rule has a box or a border of its own, else the base style does
                    if (const BoxElement* box = BoxElement::forWidget(widget))
                    {
                        const QSize contents = box->adjustSize(size);
                        return box->ownsBox(option) ? box->boxSize(option, widget, contents)
                            : PushButton::sizeFromContents(this, type, option, contents, widget, m_data->pushButtonConfig);
                    }
                }
                break;
                case CT_CheckBox:
                case CT_RadioButton:
                {
                    // The element boxes the indicator element as its rule did (QStyleSheetStyle::sizeFromContents, CT_CheckBox): the size
                    // of the label within the declared size, the indicator and the label spacing added, the box around it all
                    if (const BoxElement* box = BoxElement::forWidget(widget))
                    {
                        if (const IndicatorElement* indicator = IndicatorElement::forWidget(widget))
                        {
                            const int spacing =
                                pixelMetric(type == CT_RadioButton ? PM_RadioButtonLabelSpacing : PM_CheckBoxLabelSpacing, option, widget);
                            return box->boxSize(option, widget, indicator->sizeFromContents(box->adjustSize(size), spacing));
                        }
                    }
                }
                break;
                case CT_ToolButton:
                {
                    // The element sizes a tool button as its rule did (QStyleSheetStyle::sizeFromContents, CT_ToolButton): the box around
                    // the contents within the declared size, and the 3 x 3 Qt adds for a rule with a box ("broken QToolButton"), where the
                    // rule has a box or a border of its own, else the component does (Qt adds the 3 x 3 for a gradient or an image under
                    // the style's own border as well, sized by QWindowsStyle: no element has one)
                    if (const BoxElement* box = BoxElement::forWidget(widget); box && qobject_cast<const QToolButton*>(widget))
                    {
                        const QSize contents = box->adjustSize(size);
                        return box->ownsBox(option) ? box->boxSize(option, widget, contents + QSize(3, 3))
                            : ToolButton::sizeFromContents(this, type, option, contents, widget, m_data->toolButtonConfig);
                    }
                }
                break;
                case CT_ComboBox:
                    if (qobject_cast<const QComboBox*>(widget))
                    {
                        return ComboBox::sizeFromContents(this, type, option, size, widget, m_data->comboBoxConfig);
                    }
                    break;
                case CT_LineEdit:
                {
                    const QWidget* parent = widget ? widget->parentWidget() : nullptr;
                    if (parent && parent->inherits("QAbstractSpinBox"))
                    {
                        // A spin box line edit has the box of the spin box: the sheet returned its contents size as it was
                        // (QStyleSheetStyle::sizeFromContents, CT_LineEdit)
                        return size;
                    }

                    // The element sizes its field as its rule did (QStyleSheetStyle::sizeFromContents, CT_LineEdit): the box around
                    // the contents within the declared size where the rule has a box or a border of its own, else the base style does
                    if (const BoxElement* box = BoxElement::forWidget(widget))
                    {
                        const QSize contents = box->adjustSize(size);
                        return box->ownsBox(option) ? box->boxSize(option, widget, contents)
                                                    : QProxyStyle::sizeFromContents(type, option, contents, widget);
                    }

                    // The field's height in its box, none for the line edit of a combo box. The legacy box is 2px wider than Fusion's,
                    // less the padding-left of a search field (LineEdit.qss:22-26).
                    const QMargins box = (parent && parent->inherits("QComboBox")) ? QMargins() : LineEdit::flattenedFieldBox(widget);
                    QSize lineEditSize = QProxyStyle::sizeFromContents(type, option, size, widget);
                    lineEditSize.setHeight(Foundations::Field::Height + box.top() + box.bottom());
                    lineEditSize.rwidth() += 2 + LineEdit::flattenedPaddingLeft(widget);

                    // A search field's width is the contents width: the box goes around it, for the minimum size hint as well
                    // (QStyleSheetStyle::sizeFromContents, CT_LineEdit)
                    if (StyleHelpers::findParent<FilteredSearchWidget>(widget))
                    {
                        lineEditSize.setWidth(g_searchFieldContentsWidth + box.left() + box.right());
                    }
                    return lineEditSize;
                }
                case CT_SpinBox:
                {
                    // As the sheet sized a QSpinBox or a QDoubleSpinBox (QStyleSheetStyle::sizeFromContents, CT_SpinBox): the field's
                    // height through the base style, and the element's box, padding and up button around it. No rule sizes another one.
                    const QSize extra = SpinBoxElement::extraSize(widget);
                    if (extra.isValid())
                    {
                        return QProxyStyle::sizeFromContents(type, option, QSize(size.width(), Foundations::Field::Height), widget) + extra;
                    }
                    break;
                }
                case CT_MenuBarItem:
                    // The bar's rule gives an item its height and the item's rule its padding; contents that are not there stay not
                    // there, which makes Qt leave the item out (LuaIDE's menu without a title took a slot of 27px)
                    return MenuBarElement::global().itemSize(size);
                case CT_MenuItem:
                {
                    // QMenu::separator is the sheet's box; every other item is the base style's size, as in the legacy GUI
                    auto menuItem = qstyleoption_cast<const QStyleOptionMenuItem*>(option);
                    if (menuItem && menuItem->menuItemType == QStyleOptionMenuItem::Separator)
                    {
                        return QSize(size.width(), MenuElement::forWidget(widget).separatorHeight());
                    }
                    break;
                }
                case CT_HeaderSection:
                {
                    // The section rule's height, and the base style's width plus the padding (QStyleSheetStyle::sizeFromContents,
                    // CT_HeaderSection). The base is the TableView size of a horizontal header, Qt's for a vertical one.
                    if (hasFlattenedHeaderSection(widget))
                    {
                        QSize sectionSize = TableView::sizeFromContents(this, type, option, size, widget, m_data->tableViewConfig);
                        if (!sectionSize.isValid())
                        {
                            sectionSize = QProxyStyle::sizeFromContents(type, option, size, widget);
                        }
                        return QSize(sectionSize.width() + g_headerSectionPaddingLeft, g_headerSectionHeight);
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
        if (GuiPerfFloor::isPassthrough())
        {
            QProxyStyle::drawControl(element, option, painter, widget);
            return;
        }

        QScopedValueRollback<const QWidget*> rollbackDrawControl(m_drawControlWidget, widget);

        // SegmentBar buttons carry flagToIgnore (hasStyle == false), so their flattened look is drawn BEFORE the gate
        if (StyleManager::stylesheetsDisabled() && ControlElements::drawSegmentButton(this, element, option, painter, widget))
        {
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
                // The element frames its widget as its rule did (QStyleSheetStyle::drawControl, CE_ShapedFrame): a border of its own
                // is drawn with the fill (PE_Widget), the style's own by the base style inside the margin, over the palette of the fill
                if (StyleManager::stylesheetsDisabled())
                {
                    if (const BoxElement* box = BoxElement::forWidget(widget))
                    {
                        auto frameOption = qstyleoption_cast<const QStyleOptionFrame*>(option);
                        if (frameOption && box->hasNativeBorder(option))
                        {
                            QStyleOptionFrame inset(*frameOption);
                            inset.rect = box->borderRect(option);
                            BoxElement::configurePalette(inset.palette, box->face(option));
                            QProxyStyle::drawControl(element, &inset, painter, widget);
                        }
                        return;
                    }
                }

                // The chips of a search field (FilteredSearchWidget.qss:26-35): a rounded block with a line around it
                if (StyleManager::stylesheetsDisabled() && widget
                    && (widget->inherits("AzQtComponents::FilterCriteriaButton")
                        || widget->inherits("AzQtComponents::FilterTextButton")))
                {
                    painter->save();
                    painter->setRenderHint(QPainter::Antialiasing);
                    painter->setPen(g_chipLine);
                    painter->setBrush(g_chipFill);
                    painter->drawRoundedRect(QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5), g_chipRadius, g_chipRadius);
                    painter->restore();
                    return;
                }
                // The frame element: what the legacy sheets gave a QFrame's line, box or panel and the frame of a scroll
                // area, drawn by the base style from the palette their background derived (FrameElement.h)
                if (StyleManager::stylesheetsDisabled() && widget)
                {
                    const auto frameOption = qstyleoption_cast<const QStyleOptionFrame*>(option);
                    const FrameElement* frameElement =
                        (frameOption && frameOption->frameShape != QFrame::NoFrame) ? FrameElement::forWidget(widget) : nullptr;
                    if (frameElement)
                    {
                        QStyleOptionFrame framed(*frameOption);
                        frameElement->drawBackground(&framed, painter, widget);
                        QProxyStyle::drawControl(element, &framed, painter, widget);
                        return;
                    }
                }
                // VectorInput.qss:9-16, :37-55: the fill of the coordinate label of a vector element
                if (StyleManager::stylesheetsDisabled() && VectorElement::drawFlattenedLabel(option, painter, widget))
                {
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
                // The handle's rect (the contents of QSplitterHandle, the pixels between the panes: PM_SplitterWidth) filled as a rule
                // fills its background (QRenderRule::drawBackground), another fill under the pointer; a main window's separator (the
                // gaps between docks) across the whole gap
                if (StyleManager::stylesheetsDisabled())
                {
                    const bool hover = option->state.testFlag(QStyle::State_MouseOver);
                    const bool mainWindow = qobject_cast<const QMainWindow*>(widget) != nullptr;
                    painter->fillRect(option->rect, mainWindow ? g_separatorFill : (hover ? g_splitterHoverFill : g_splitterFill));
                    return;
                }
            }
            break;

            case CE_ToolBar:
            {
                // The toolbar element's panel instead of the base style's bevel (Fusion draws one for the toolbars of a QMainWindow
                // and none for the others, which QCommonStyle sends to PE_PanelToolBar)
                if (StyleManager::stylesheetsDisabled() && ControlElements::drawToolBarPanel(option, painter, widget))
                {
                    return;
                }
            }
            break;

            case CE_TabBarTabShape:
            {
                // The tab element: fill and edges of the tab's state
                if (StyleManager::stylesheetsDisabled())
                {
                    if (const TabElement* tabElement = TabElement::forTabBar(widget))
                    {
                        tabElement->drawTab(painter, option);
                        return;
                    }
                    // A plain tab is the base style's, drawn over the palette the legacy sheet gave it
                    const TabElement* plain = TabElement::plainForTabBar(widget);
                    auto tabOption = qstyleoption_cast<const QStyleOptionTab*>(option);
                    if (plain && tabOption)
                    {
                        QStyleOptionTab plainOption = *tabOption;
                        plain->applyPalette(plainOption.palette);
                        QProxyStyle::drawControl(element, &plainOption, painter, widget);
                        return;
                    }
                }
            }
            break;

            case CE_MenuBarEmptyArea:
            {
                // The legacy bar drew nothing here but its border: the bar is transparent over its window (BaseStyleSheet.qss:17)
                if (StyleManager::stylesheetsDisabled())
                {
                    MenuBarElement::global().drawBorder(painter, option->rect);
                    return;
                }
            }
            break;

            case CE_MenuBarItem:
            {
                if (StyleManager::stylesheetsDisabled())
                {
                    if (auto menuBarItem = qstyleoption_cast<const QStyleOptionMenuItem*>(option))
                    {
                        // As the legacy sheet style drew it (QStyleSheetStyle::drawControl): an item with a fill (hovered or pressed)
                        // is filled by its own rule and labelled by QCommonStyle, the base style bypassed; an idle item is the base
                        // style's, which lays its label out one pixel higher
                        const MenuBarElement& menuBar = MenuBarElement::global();
                        const QColor fill = menuBar.itemFill(option);
                        const QStyleOptionMenuItem label = menuBar.labelOption(*menuBarItem);
                        if (fill.isValid())
                        {
                            painter->fillRect(option->rect, fill);
                            QCommonStyle::drawControl(element, &label, painter, widget);
                        }
                        else
                        {
                            QProxyStyle::drawControl(element, &label, painter, widget);
                        }
                        return;
                    }
                }
            }
            break;

            case CE_MenuItem:
            {
                // The items of a popup menu: QWindowsStyle's, over the menu element's palette (the legacy sheet style handed them to it,
                // QStyleSheetStyle::drawControl, CE_MenuItem). Only a QMenu's own: a combo box in menu mode keeps the base style's.
                if (StyleManager::stylesheetsDisabled() && qobject_cast<const QMenu*>(widget))
                {
                    if (auto menuItem = qstyleoption_cast<const QStyleOptionMenuItem*>(option))
                    {
                        if (MenuElement::forWidget(widget).drawItem(painter, *menuItem, widget))
                        {
                            return;
                        }
                    }
                }
            }
            break;

            case CE_TabBarTabLabel:
            {
                // The tab element: the label is laid out inside the tab's contents box (the stylesheet
                // passed Qt the rect minus margin, edges and padding) in the state's text color and
                // weight, and without Qt's focus rectangle where the stylesheet gave none
                if (StyleManager::stylesheetsDisabled())
                {
                    const TabElement* tabElement = TabElement::forTabBar(widget);
                    auto tabOption = qstyleoption_cast<const QStyleOptionTab*>(option);
                    if (tabElement && tabOption)
                    {
                        const TabElement::TabFace& tabFace = tabElement->face(option);
                        QStyleOptionTab labelOption = *tabOption;
                        labelOption.rect = tabElement->contentsRect(option->rect);
                        labelOption.palette.setColor(QPalette::WindowText, tabFace.m_text);
                        labelOption.palette.setColor(QPalette::ButtonText, tabFace.m_text);
                        if (tabElement->m_hideFocusRect)
                        {
                            labelOption.state &= ~QStyle::State_HasFocus;
                        }
                        // The rule's font-weight over the painter's font, put back after the label (QStyleSheetStyle::drawControl)
                        const QFont painterFont = painter->font();
                        if (tabFace.m_weight > 0)
                        {
                            QFont labelFont = painterFont;
                            labelFont.setWeight(static_cast<QFont::Weight>(tabFace.m_weight));
                            painter->setFont(labelFont);
                        }
                        QProxyStyle::drawControl(CE_TabBarTabLabel, &labelOption, painter, widget);
                        painter->setFont(painterFont);
                        return;
                    }
                }
            }
            break;

            case CE_DockWidgetTitle:
            {
                // The title strip of a plain dock widget (QDockWidget.qss:20-23; the dock widgets of gems): the legacy style drew the
                // fill, then the title itself (QStyleSheetStyle::drawControl, CE_DockWidgetTitle) in the base style's title rect (the
                // title area less the buttons: QFusionStyle::subElementRect, SE_DockWidgetTitleBarText), left aligned from the top,
                // elided, in the dock widget's colour (BaseStyleSheet.qss:62-66). The dock's palette stays white, as one set on it would
                // reach the widgets inside (a rule of their own held them to white), so the colour is carried here. No dock widget of the
                // engine has a vertical title bar.
                if (StyleManager::stylesheetsDisabled())
                {
                    painter->fillRect(option->rect, g_dockTitleFill);
                    auto dockOption = qstyleoption_cast<const QStyleOptionDockWidget*>(option);
                    if (dockOption && !dockOption->title.isEmpty())
                    {
                        const QRect textRect = QProxyStyle::subElementRect(SE_DockWidgetTitleBarText, option, widget);
                        QPalette titlePalette = dockOption->palette;
                        titlePalette.setColor(QPalette::WindowText, g_dockTitleText);
                        int textFlags = Qt::AlignLeft;
                        textFlags |= Qt::TextHideMnemonic;
                        drawItemText(painter, textRect, textFlags, titlePalette, dockOption->state.testFlag(QStyle::State_Enabled),
                            painter->fontMetrics().elidedText(dockOption->title, Qt::ElideRight, textRect.width()), QPalette::WindowText);
                    }
                    return;
                }
            }
            break;

            case CE_PushButton:
            {
                // A button with an element is drawn as the sheet drew it (QStyleSheetStyle::drawControl, QCommonStyle::drawControl,
                // CE_PushButton): the bevel, then the label in the contents rect of the box, in the colour of the state. Fusion lays the
                // label out in the rect it asks of itself (QFusionStyle::drawControl), which the box would not reach.
                if (StyleManager::stylesheetsDisabled())
                {
                    const BoxElement* box = BoxElement::forWidget(widget);
                    auto buttonOption = qstyleoption_cast<const QStyleOptionButton*>(option);
                    if (box && buttonOption)
                    {
                        drawControl(CE_PushButtonBevel, option, painter, widget);
                        QStyleOptionButton labelOption(*buttonOption);
                        labelOption.rect = subElementRect(SE_PushButtonContents, option, widget);
                        if (const QColor text = box->face(option).m_text; text.isValid())
                        {
                            labelOption.palette.setColor(QPalette::ButtonText, text);
                        }
                        drawControl(CE_PushButtonLabel, &labelOption, painter, widget);
                        return;
                    }
                }
            }
            break;

            case CE_PushButtonLabel:
            {
                // A rule with a text-align has a position, and the sheet style laid the label of its push buttons out itself: the icon and
                // the text 4px apart where the base style's are 2px (QStyleSheetStyle::drawControl, QCommonStyle::drawControl)
                if (StyleManager::stylesheetsDisabled())
                {
                    const BoxElement* box = BoxElement::forWidget(widget);
                    auto buttonOption = qstyleoption_cast<const QStyleOptionButton*>(option);
                    if (box && buttonOption && box->drawPushButtonLabel(this, buttonOption, painter, widget))
                    {
                        return;
                    }
                }
            }
            break;

            case CE_PushButtonBevel:
            {
                // The element's bevel (QStyleSheetStyle::drawControl, CE_PushButtonBevel)
                if (StyleManager::stylesheetsDisabled())
                {
                    const BoxElement* box = BoxElement::forWidget(widget);
                    auto buttonOption = qstyleoption_cast<const QStyleOptionButton*>(option);
                    if (box && buttonOption && !box->hasNativeBorder(option))
                    {
                        // A border of its own, or the none of a flat button: the element draws the face
                        box->draw(painter, option, widget);
                        if (buttonOption->features.testFlag(QStyleOptionButton::HasMenu))
                        {
                            // the menu indicator: the style's arrow in a square of its size at the bottom right of the padding
                            // box (defaultPosition and QStyleSheetStyle::defaultSize of the sheet style)
                            const int extent = pixelMetric(PM_MenuButtonIndicator, nullptr, widget);
                            QStyleOptionButton arrowOption(*buttonOption);
                            arrowOption.rect = QStyle::alignedRect(option->direction, Qt::AlignRight | Qt::AlignBottom,
                                QSize(extent, extent), box->contentsRect(option, widget).marginsAdded(box->m_padding));
                            drawPrimitive(PE_IndicatorArrowDown, &arrowOption, painter, widget);
                        }
                        return;
                    }
                    if (box && buttonOption && box->ownsBox(option))
                    {
                        // The style's own bevel, inside the margin of the box
                        QStyleOptionButton innerOption(*buttonOption);
                        innerOption.rect = box->borderRect(option);
                        if (!PushButton::drawPushButtonBevel(this, &innerOption, painter, widget, m_data->pushButtonConfig))
                        {
                            QProxyStyle::drawControl(element, &innerOption, painter, widget);
                        }
                        return;
                    }
                }

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
                // The box of the element around the indicator element of a check box (QStyleSheetStyle::drawControl, CE_CheckBox)
                if (StyleManager::stylesheetsDisabled())
                {
                    auto buttonOption = qstyleoption_cast<const QStyleOptionButton*>(option);
                    const BoxElement* box = BoxElement::forWidget(widget);
                    if (box && buttonOption && box->drawIndicatorButton(this, buttonOption, painter, widget))
                    {
                        return;
                    }
                }

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
                // The box of the element around the indicator element of a radio button (QStyleSheetStyle::drawControl, CE_RadioButton)
                if (StyleManager::stylesheetsDisabled())
                {
                    auto buttonOption = qstyleoption_cast<const QStyleOptionButton*>(option);
                    const BoxElement* box = BoxElement::forWidget(widget);
                    if (box && buttonOption && box->drawIndicatorButton(this, buttonOption, painter, widget))
                    {
                        return;
                    }
                }

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
                if (StyleManager::stylesheetsDisabled() && widget && widget->inherits("QTableCornerButton"))
                {
                    // The corner button of a table, drawn as a section (QTableCornerButton::paintEvent): solid black, as the legacy GUI
                    // drew it; the rule that granted it is not located (ledger) $fill.table-corner.idle
                    painter->fillRect(option->rect, QColor(Qt::black));
                    return;
                }
                if (StyleManager::stylesheetsDisabled() && hasFlattenedHeaderSection(widget))
                {
                    // QCommonStyle::drawControl (CE_Header) over the section rule: clipped to the section, the item view element's fill (a
                    // section declared transparent paints none), the label in the rule's content box (QStyleSheetStyle::subElementRect,
                    // SE_HeaderLabel: the padding, no border), then the base style's sort arrow
                    painter->save();
                    painter->setClipRect(option->rect);
                    if (const QColor sectionFill = ItemViewElement::headerFillOf(widget); sectionFill.alpha() > 0)
                    {
                        painter->fillRect(option->rect, sectionFill);
                    }
                    if (auto headerOption = qstyleoption_cast<const QStyleOptionHeader*>(option))
                    {
                        // The rule's size and normal style (TableView.qss:72-73) over the painter's font, as QStyleSheetStyle::drawControl
                        // (CE_HeaderLabel) sets them; family and weight stay the viewport's
                        QFont sectionFont;
                        sectionFont.setPixelSize(g_headerSectionTextSize);
                        sectionFont.setStyle(QFont::StyleNormal);
                        painter->setFont(sectionFont.resolve(painter->font()));

                        QStyleOptionHeader labelOption = *headerOption;
                        labelOption.rect = option->rect.adjusted(g_headerSectionPaddingLeft, 0, 0, 0);
                        QProxyStyle::drawControl(CE_HeaderLabel, &labelOption, painter, widget);

                        if (headerOption->sortIndicator != QStyleOptionHeader::None)
                        {
                            QStyleOptionHeader arrowOption = *headerOption;
                            arrowOption.rect = QProxyStyle::subElementRect(SE_HeaderArrow, option, widget);
                            QProxyStyle::drawPrimitive(PE_IndicatorHeaderArrow, &arrowOption, painter, widget);
                        }
                    }
                    painter->restore();
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

            case CE_HeaderEmptyArea:
            {
                // The universal rule gave the header a (transparent) background, so the sheet drew nothing beyond the last section
                // (QStyleSheetStyle::drawControl, CE_HeaderEmptyArea): what is behind the header shows
                if (StyleManager::stylesheetsDisabled() && qobject_cast<const QHeaderView*>(widget))
                {
                    return;
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
                // The popup list of a combo box paints each item from its element, not from the palette Qt hands the view (the combo box
                // replaces it, QComboBoxPrivate::updateViewContainerPaletteAndOpacity): fill, text colour, no outline
                if (StyleManager::stylesheetsDisabled() && ComboPopupElement::coversView(widget))
                {
                    if (auto popupOption = qstyleoption_cast<const QStyleOptionViewItem*>(option))
                    {
                        QStyleOptionViewItem popupItem = *popupOption;
                        ComboPopupElement::global().prepareItem(popupItem, painter);
                        return QProxyStyle::drawControl(element, &popupItem, painter, widget);
                    }
                }

                // The legacy sheet drew an item through the common style, with the palette of its rule (QStyleSheetStyle::drawControl):
                // the colour of the rule's text over the palette the view hands the item. The model's background and the base style's
                // selection fill are not drawn (PE_PanelItemViewItem, above); the item view element is the whole of the look.
                if (StyleManager::stylesheetsDisabled())
                {
                    auto elementOption = qstyleoption_cast<const QStyleOptionViewItem*>(option);
                    if (const ItemViewElement* items = elementOption ? ItemViewElement::forWidget(widget) : nullptr)
                    {
                        QStyleOptionViewItem item(*elementOption);
                        items->prepareItem(&item);
                        return QProxyStyle::drawControl(element, &item, painter, widget);
                    }
                }

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
    // Flattened indicator images (GUI flattening): the images CheckBox.qss, RadioButton.qss and Card.qss give each state of an
    // indicator, and EntityOutliner.qss each toggle of the outliner, drawn from their SVG resources through the pixmap cache
    //////////////////////////////////////////////////////////////////////////

    //! An SVG resource drawn into a rect, rendered once for each size and device ratio. The logical bounds are given: render(painter)
    //! alone takes the pixmap's device size and draws the image too large at a high device ratio.
    static void drawSvg(QPainter* painter, const QString& resource, const QRect& rect)
    {
        if (resource.isEmpty() || !rect.isValid())
        {
            return;
        }
        const qreal devicePixelRatio = painter->device() ? painter->device()->devicePixelRatio() : 1.0;
        const QString cacheKey = QString::fromLatin1("O3DEFlatInd|%1|%2x%3|%4")
            .arg(resource).arg(rect.width()).arg(rect.height()).arg(devicePixelRatio);
        QPixmap pixmap;
        if (!QPixmapCache::find(cacheKey, &pixmap))
        {
            QSvgRenderer renderer(resource);
            pixmap = QPixmap(rect.size() * devicePixelRatio);
            pixmap.setDevicePixelRatio(devicePixelRatio);
            pixmap.fill(Qt::transparent);
            QPainter pixmapPainter(&pixmap);
            renderer.render(&pixmapPainter, QRectF(QPointF(0.0, 0.0), QSizeF(rect.size())));
            pixmapPainter.end();
            QPixmapCache::insert(cacheKey, pixmap);
        }
        painter->drawPixmap(rect, pixmap);
    }

    //! The images of one check state at rest, focused and disabled (nullptr: the one at rest)
    struct FlatIndicatorImages
    {
        const char* m_idle;
        const char* m_focus;
        const char* m_disabled;
    };

    //! An indicator's folder of images, and its images unchecked, checked and partially checked (none: as unchecked)
    struct FlatIndicator
    {
        const char* m_folder;
        FlatIndicatorImages m_unchecked;
        FlatIndicatorImages m_checked;
        FlatIndicatorImages m_partial;
    };

    // CheckBox.qss:62, :67, :72; :47, :52, :57; :32, :37, :42 $asset.check-indicator.unchecked.Alt1
    // $asset.check-indicator.unchecked+focus.Alt1 $asset.check-indicator.unchecked+disabled.Alt1 $asset.check-indicator.checked.Alt1
    // $asset.check-indicator.checked+focus.Alt1 $asset.check-indicator.checked+disabled.Alt1 $asset.check-indicator.indeterminate
    // $asset.check-indicator.indeterminate+focus $asset.check-indicator.indeterminate+disabled
    static constexpr FlatIndicator g_checkBoxImages{ ":/stylesheet/img/UI20/checkbox/%1.svg", { "off", "off-focus", "off-disabled" },
        { "on", "on-focus", "on-disabled" }, { "partial-selected", "partial-selected-focus", "partial-selected-disabled" } };
    // CheckBox.qss:85, :99, :90; :104, :118, :109 $asset.check-indicator.unchecked.Alt3 $asset.check-indicator.unchecked+focus.Alt3
    // $asset.check-indicator.disabled $asset.check-indicator.checked.Alt3 $asset.check-indicator.checked+focus.Alt3
    // $asset.check-indicator.checked+disabled.Alt2
    static constexpr FlatIndicator g_toggleSwitchImages{ ":/stylesheet/img/UI20/toggleswitch/%1.svg",
        { "unchecked", "unchecked-focus", "unchecked-disabled" }, { "checked", "checked-focus", "checked-disabled" }, {} };
    // CheckBox.qss:157 (at rest and focused), :162; :145 (at rest and focused), :150 $asset.check-indicator.unchecked.Alt2
    // $asset.check-indicator.unchecked+focus.Alt2 $asset.check-indicator.unchecked+disabled.Alt2 $asset.check-indicator.checked.Alt2
    // $asset.check-indicator.checked+focus.Alt2 $asset.check-indicator.checked+disabled.Alt3
    static constexpr FlatIndicator g_expanderImages{ ":/Cards/img/UI20/Cards/%1.svg", { "caret-right", nullptr, "caret-right-disabled" },
        { "caret-down", nullptr, "caret-down-disabled" }, {} };
    // A card header's, whose disabled caret is not faded: Card.qss:281, :286 $asset.check-indicator.unchecked+disabled.Alt3
    // $asset.check-indicator.checked+disabled.Alt4
    static constexpr FlatIndicator g_cardHeaderExpanderImages{ ":/Cards/img/UI20/Cards/%1.svg", { "caret-right", nullptr, "caret-right" },
        { "caret-down", nullptr, "caret-down" }, {} };
    // RadioButton.qss:53, :63, :58; :38, :48, :43 $asset.check-indicator.unchecked.Alt4 $asset.check-indicator.unchecked+focus.Alt4
    // $asset.check-indicator.unchecked+disabled.Alt4 $asset.check-indicator.checked.Alt4 $asset.check-indicator.checked+focus.Alt4
    // $asset.check-indicator.checked+disabled.Alt5
    static constexpr FlatIndicator g_radioButtonImages{ ":/stylesheet/img/UI20/radiobutton/%1.svg",
        { "unchecked", "unchecked-focus", "unchecked-disabled" }, { "checked", "checked-focus", "checked-disabled" }, {} };

    //! The images of a check box's indicator: a toggle switch's, an expander's, or a check box's, which an item view's check boxes share
    static const FlatIndicator& flatCheckBoxImages(const QWidget* widget)
    {
        if (widget && Style::hasClass(widget, QStringLiteral("ToggleSwitch")))
        {
            return g_toggleSwitchImages;
        }
        if (widget && Style::hasClass(widget, QStringLiteral("Expander")))
        {
            return Selectors::withinClass(widget, "AzQtComponents::CardHeader") ? g_cardHeaderExpanderImages : g_expanderImages;
        }
        return g_checkBoxImages;
    }

    //! The image of a state: a disabled widget's disabled image, else a focused one's focus image, else the one at rest
    static QString flatIndicatorImage(const FlatIndicator& indicator, QStyle::State state)
    {
        const FlatIndicatorImages& images = (state.testFlag(QStyle::State_NoChange) && indicator.m_partial.m_idle) ? indicator.m_partial
            : state.testFlag(QStyle::State_On) ? indicator.m_checked : indicator.m_unchecked;
        const char* image = !state.testFlag(QStyle::State_Enabled) ? images.m_disabled
            : state.testFlag(QStyle::State_HasFocus) ? images.m_focus : nullptr;
        return QString::fromLatin1(indicator.m_folder).arg(QLatin1String(image ? image : images.m_idle));
    }

    //! A toggle of the outliner (EntityOutliner.qss:37-166), by name: its image unchecked and checked (in :/Outliner/), the top of its
    //! box in each, and the rounded pad behind the toggle of a row whose children are mixed
    struct FlatOutlinerToggle
    {
        const char* m_name;
        const char* m_unchecked;
        int m_uncheckedTop;
        const char* m_checked;
        int m_checkedTop;
        bool m_mixed;
    };
    // EntityOutliner.qss:60 (:95, :106, :114, :122 restate it) $space.outliner-toggle.idle.margin-top
    static constexpr int g_toggleTop = 3;
    static constexpr int g_lockImageTop = 1;        // EntityOutliner.qss:132, :149, :157, :165 $space.outliner-toggle.lock.margin-top
    // The box and the image in it, one pixel in (the transparent border of :55): EntityOutliner.qss:62, :53, :52-53
    // $size.outliner-toggle.idle.width $size.outliner-toggle.idle.height $size.outliner-toggle.idle.image
    static constexpr QSize g_toggleBox(18, 16);
    static constexpr QRect g_toggleImage(1, 0, 16, 16);
    static constexpr QColor g_mixedTogglePad(0, 0, 0, 80);         // EntityOutliner.qss:78 $fill.outliner-toggle.mixed
    static constexpr int g_mixedTogglePadRadius = 5;               // EntityOutliner.qss:79 $radius.outliner-toggle.mixed
    // EntityOutliner.qss:85-166 (unchecked, checked) $asset.visibility-toggle.unchecked $asset.visibility-toggle.checked and their
    // +overridden and +hover, $asset.lock-toggle.unchecked $asset.lock-toggle.checked and theirs
    static constexpr FlatOutlinerToggle g_outlinerToggles[] = {
        { "Visibility", "visibility_on", g_toggleTop, "visibility_default", g_toggleTop, false },                       // :94, :88
        { "VisibilityMixed", "visibility_on", g_toggleTop, "visibility_default", g_toggleTop, true },                  // :94, :88
        { "VisibilityOverridden", "visibility_on_transparent", g_toggleTop, "visibility_default_transparent", g_toggleTop, false },
        { "VisibilityHover", "visibility_on_hover", g_toggleTop, "visibility_default_hover", g_toggleTop, false },     // :121, :113
        { "VisibilityMixedHover", "visibility_on_hover", g_toggleTop, "visibility_default_hover", g_toggleTop, true },
        { "VisibilityOverriddenHover", "visibility_on_hover", g_toggleTop, "visibility_default_hover", g_toggleTop, false },
        { "Lock", "lock_default", g_toggleTop, "lock_on", g_lockImageTop, false },                                    // :138, :131
        { "LockMixed", "lock_default", g_toggleTop, "lock_on", g_lockImageTop, true },                                 // :138, :131
        { "LockOverridden", "lock_default_transparent", g_lockImageTop, "lock_on_transparent", g_toggleTop, false },  // :148, :143
        { "LockHover", "lock_default_hover", g_lockImageTop, "lock_on_hover", g_lockImageTop, false },                // :164, :156
        { "LockMixedHover", "lock_default_hover", g_lockImageTop, "lock_on_hover", g_lockImageTop, true },
        { "LockOverriddenHover", "lock_default_hover", g_lockImageTop, "lock_on_hover", g_lockImageTop, false },
    };

    //! The outliner toggle a widget is, by its name; none for any other widget
    static const FlatOutlinerToggle* flatOutlinerToggle(const QWidget* widget)
    {
        if (widget && widget->inherits("AzToolsFramework::EntityOutlinerCheckBox"))
        {
            for (const FlatOutlinerToggle& toggle : g_outlinerToggles)
            {
                if (widget->objectName() == QLatin1String(toggle.m_name))
                {
                    return &toggle;
                }
            }
        }
        return nullptr;
    }

    //! An outliner toggle: its box at the widget's left, at the top of its state, the pad behind a mixed one, the image in the box
    static void drawFlatOutlinerToggle(const FlatOutlinerToggle& toggle, const QStyleOption* option, QPainter* painter, const QWidget* widget)
    {
        const bool checked = option->state.testFlag(QStyle::State_On);
        const QRect area = widget->rect();
        const QRect box(QPoint(area.left(), area.top() + (checked ? toggle.m_checkedTop : toggle.m_uncheckedTop)), g_toggleBox);
        if (toggle.m_mixed)
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->setPen(Qt::NoPen);
            painter->setBrush(g_mixedTogglePad);
            painter->drawRoundedRect(box, g_mixedTogglePadRadius, g_mixedTogglePadRadius);
            painter->restore();
        }
        const QString image = QStringLiteral(":/Outliner/%1.svg").arg(QLatin1String(checked ? toggle.m_checked : toggle.m_unchecked));
        drawSvg(painter, image, g_toggleImage.translated(box.topLeft()));
    }

    //! A flattened scroll bar: the lane's fill under the pointer, then the handle inside the lane's border, rounded
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

        const QColor hoverFill = dark ? g_darkScrollHoverFill : g_scrollHoverFill;
        const QColor handleColor = dark ? g_darkScrollHandleFill : g_scrollHandleFill;

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
            const int border = g_scrollBarBorder;
            handleRect = horizontal ? handleRect.adjusted(0, border, 0, -border) : handleRect.adjusted(border, 0, -border, 0);
            painter->setPen(Qt::NoPen);
            painter->setBrush(handleColor);
            painter->drawRoundedRect(handleRect, g_scrollHandleRadius, g_scrollHandleRadius);
        }

        painter->restore();
    }

    //////////////////////////////////////////////////////////////////////////
    // Flattened plain dock widgets: QDockWidget.qss:14-31 is a backup style for the QDockWidget class itself, "not derived classes"
    // (:9-12): `.QDockWidget` matches the exact class name of a widget that has no class tag (QStyleSheetStyleSelector::attributeValue).
    // It gives the title strip (:20-23, CE_DockWidgetTitle), the title icons (:16-17, Style::standardIcon) and frameless transparent
    // title buttons (:26-31, PE_PanelButtonTool). A title button is a QDockWidgetTitleButton, which the legacy style matched as its
    // dock widget (qt_check_if_internal_object).
    //////////////////////////////////////////////////////////////////////////
    static bool isFlattenedPlainDockWidget(const QWidget* widget)
    {
        return widget && QString::fromLatin1(widget->metaObject()->className()) == QLatin1String("QDockWidget");
    }

    static bool isFlattenedPlainDockTitleButton(const QWidget* widget)
    {
        return widget && QString::fromLatin1(widget->metaObject()->className()) == QLatin1String("QDockWidgetTitleButton")
            && isFlattenedPlainDockWidget(widget->parentWidget());
    }

    void Style::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
    {
        if (GuiPerfFloor::isPassthrough())
        {
            QProxyStyle::drawPrimitive(element, option, painter, widget);
            return;
        }

        if (element == PE_IndicatorDockWidgetResizeHandle)
        {
            // A main window's separator (BaseStyleSheet.qss:50-58), where Fusion would draw its light resize line
            if (StyleManager::stylesheetsDisabled())
            {
                painter->fillRect(option->rect, option->state.testFlag(QStyle::State_MouseOver) ? g_separatorHoverFill : g_separatorFill);
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

        // The line edit inside a BrowseEdit carries flagToIgnore (hasStyle == false); BrowseEdit.qss:15-29 gave it a field of its own
        if (element == PE_PanelLineEdit && StyleManager::stylesheetsDisabled() && BrowseEdit::drawFlattenedField(option, painter, widget))
        {
            return;
        }

        if (!hasStyle(widget))
        {
            QProxyStyle::drawPrimitive(element, option, painter, widget);
            return;
        }

        prepPainter(painter);
        switch (element)
        {
            case PE_Widget:
            {
                // The tab element's bar and tab widget boxes, and the fill of a widget hosted in a menu while it is hovered
                // (the stylesheet drew a widget's own background and border here, for widgets with WA_StyledBackground)
                if (StyleManager::stylesheetsDisabled() && widget)
                {
                    if (widget->inherits("QTabBar"))
                    {
                        // The bar of an element, or a plain bar's (its sheet's background-color, TabWidget.qss:10)
                        const TabElement* tabElement = TabElement::forTabBar(widget);
                        if (!tabElement)
                        {
                            tabElement = TabElement::plainForTabBar(widget);
                        }
                        if (tabElement)
                        {
                            TabElement::drawFrame(painter, option->rect, tabElement->m_barFill, tabElement->m_barEdges);
                            return;
                        }
                    }
                    else if (widget->inherits("QTabWidget"))
                    {
                        if (const TabElement* tabElement = TabElement::forTabWidget(widget))
                        {
                            const QRect box = option->rect.marginsRemoved(tabElement->m_widgetMargins);
                            TabElement::drawFrame(painter, box, tabElement->m_widgetFill, tabElement->m_widgetEdges);
                            return;
                        }
                    }
                    else if (MenuElement::drawHostedWidget(painter, option, widget))
                    {
                        return;
                    }

                    // The element of any other widget (QStyleSheetStyle::drawPrimitive, PE_Widget): its fill, image and border
                    if (const BoxElement* boxElement = BoxElement::forWidget(widget); boxElement && boxElement->draw(painter, option, widget))
                    {
                        return;
                    }

                    // What the element took the autofill of is painted in its background role where it draws nothing (:4682-4691)
                    if (BoxElement::fillsBackgroundRole(widget, option))
                    {
                        painter->fillRect(option->rect, option->palette.brush(widget->backgroundRole()));
                        return;
                    }
                }
            }
            break;

            case PE_Frame:
            {
                // The border of the element over what a picker frame has painted (QStyleSheetStyle::drawPrimitive, PE_Frame)
                if (StyleManager::stylesheetsDisabled() && ColorPickerElements::drawFrame(option, painter, widget))
                {
                    return;
                }
            }
            break;

            case PE_PanelLineEdit:
            {
                // The element frames its field as its rule did (QStyleSheetStyle::drawPrimitive, PE_PanelLineEdit): a border of its own
                // is drawn with the fill; the style's own by the base style inside the margin, over the palette of the fill. A line edit
                // inside a combo box or a spin box has no panel of its own where the container draws its border or a fill the base
                // style cannot. A field's State_Sunken says its frame is sunken, not that it is pressed (QStyleSheetStyle::renderRule).
                if (StyleManager::stylesheetsDisabled())
                {
                    const QWidget* container = BoxElement::containerOf(widget);
                    auto frameOption = qstyleoption_cast<const QStyleOptionFrame*>(option);
                    if (const BoxElement* box = BoxElement::forWidget(container); box && frameOption)
                    {
                        QStyleOptionFrame field(*frameOption);
                        field.state &= ~QStyle::State_Sunken;
                        const BoxElement::Face& face = box->face(&field);
                        if (!box->hasNativeBorder(&field))
                        {
                            if (container == widget)
                            {
                                box->draw(painter, &field, widget);
                            }
                            return;
                        }
                        const bool baseDraws = BoxElement::baseStyleCanDraw(face, container);
                        if (container != widget && !baseDraws)
                        {
                            return;
                        }
                        QStyleOptionFrame inset(field);
                        inset.rect = box->borderRect(&field);
                        BoxElement::configurePalette(inset.palette, face);
                        if (baseDraws)
                        {
                            QProxyStyle::drawPrimitive(element, &inset, painter, widget);
                        }
                        else
                        {
                            box->draw(painter, &field, widget);
                            if (inset.lineWidth > 0)
                            {
                                QProxyStyle::drawPrimitive(PE_FrameLineEdit, &inset, painter, widget);
                            }
                        }
                        return;
                    }
                }

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
                // QDockWidget.qss:26-31: the title buttons of a plain dock widget have no border and a transparent
                // background in every state, where Fusion raises a panel under the mouse
                if (StyleManager::stylesheetsDisabled() && isFlattenedPlainDockTitleButton(widget))
                {
                    return;
                }
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
                if (StyleManager::stylesheetsDisabled())
                {
                    // The item view element draws the branch cell as its rule did: a fill by state, and the caret fitted into the cell
                    // less the rule's padding (TableView.qss:42-60, 87-97); a TableView's element has no caret (:104-108)
                    auto branchOption = qstyleoption_cast<const QStyleOptionViewItem*>(option);
                    if (const ItemViewElement* items = branchOption ? ItemViewElement::forWidget(widget) : nullptr)
                    {
                        if (!items->drawBranch(this, branchOption, painter, widget))
                        {
                            // The rule has nothing for this cell: the sheet style asked the base style, which draws a tree's connecting lines
                            // with a BranchDelegate (QStyleSheetStyle::drawPrimitive, PE_IndicatorBranch) and nothing for any other tree
                            TreeView::drawBranchIndicator(this, option, painter, widget, m_data->treeViewConfig);
                        }
                        return;
                    }
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
                    // Item view check boxes share the check box's images, drawn in the rect the item rule's margin leaves (a tree's is
                    // pulled 3px to the left, TableView.qss:84: ItemViewElement::indicatorImageRect)
                    const ItemViewElement* items = ItemViewElement::forWidget(widget);
                    drawSvg(painter, flatIndicatorImage(flatCheckBoxImages(widget), option->state),
                        items ? items->indicatorImageRect(option->rect) : option->rect);
                    return;
                }
            }
            break;

            case PE_IndicatorTabClose:
            {
                if (StyleManager::stylesheetsDisabled())
                {
                    // The close button widget belongs to the tab bar (the stylesheet matched the bar)
                    if (const TabElement* tabElement = TabElement::forTabBar(widget ? widget->parentWidget() : nullptr))
                    {
                        tabElement->drawClose(painter, option);
                        return;
                    }
                    if (option->state.testFlag(QStyle::State_MouseOver))
                    {
                        painter->fillRect(option->rect, QColor(0x44, 0x44, 0x44));
                    }
                    QRect glyphRect(0, 0, 12, 12);
                    glyphRect.moveCenter(option->rect.center());
                    drawSvg(painter, QStringLiteral(":/Application/titlebar-close.svg"), glyphRect);
                    return;
                }
            }
            break;

            case PE_FrameDockWidget:
            {
                // StyledDockWidget.qss:9-18: the border of the rule a StyledDockWidget was polished with, none for one polished docked
                // (ControlElements.cpp). Another dock widget with a title bar of its own keeps a black line while it floats.
                if (StyleManager::stylesheetsDisabled())
                {
                    if (ControlElements::drawDockFrame(option, painter, widget))
                    {
                        return;
                    }
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
                // The image of the option's state, or of an outliner toggle (EntityOutliner.qss)
                if (StyleManager::stylesheetsDisabled())
                {
                    if (const FlatOutlinerToggle* toggle = flatOutlinerToggle(widget))
                    {
                        drawFlatOutlinerToggle(*toggle, option, painter, widget);
                        return;
                    }
                    drawSvg(painter, flatIndicatorImage(flatCheckBoxImages(widget), option->state), option->rect);
                    return;
                }
            }
            break;

            case PE_PanelToolBar:
            {
                // ToolBar.qss QToolBar { background-color: #444444; border: 0 none };
                // MainWindow QToolBar { border-bottom: 2px solid #111111 }: the toolbar element's panel (ControlElements.cpp)
                if (StyleManager::stylesheetsDisabled() && ControlElements::drawToolBarPanel(option, painter, widget))
                {
                    return;
                }
            }
            break;

            case PE_IndicatorToolBarHandle:
            {
                // ToolBar.qss QToolBar::handle: the sheet's image in the padding of its rule, not the base style's dots
                if (StyleManager::stylesheetsDisabled() && ControlElements::drawToolBarHandle(option, painter, widget))
                {
                    return;
                }
            }
            break;

            case PE_IndicatorToolBarSeparator:
            {
                // A toolbar's separator (ToolBar.qss:71-79), inside its margin along the bar
                if (StyleManager::stylesheetsDisabled())
                {
                    const int margin = g_toolBarSeparatorMargin;
                    const QRect line = option->state.testFlag(QStyle::State_Horizontal) ? option->rect.adjusted(0, margin, 0, -margin)
                                                                                         : option->rect.adjusted(margin, 0, -margin, 0);
                    painter->fillRect(line, g_separatorFill);
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
                // Tab elements draw no base line (the stylesheet set drawBase false); the bar's
                // own edges are part of its box (PE_Widget)
                if (StyleManager::stylesheetsDisabled() && TabElement::forTabBar(widget))
                {
                    return;
                }
            }
            break;

            case PE_PanelMenu:
            {
                if (StyleManager::stylesheetsDisabled())
                {
                    // Menu.qss:12 QMenu { background-color }, no frame (the menu element's surface)
                    painter->fillRect(option->rect, MenuElement::forWidget(widget).m_surface);
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

            case PE_PanelTipLabel:
            {
                if (StyleManager::stylesheetsDisabled())
                {
                    // ToolTip.qss:13 QToolTip { background-color }, no frame (:14 names no border style: Qt reads none)
                    painter->fillRect(option->rect, option->palette.toolTipBase());
                    return;
                }
            }
            break;

            case PE_FrameTabWidget:
            {
                // The tab element's pane fill and edges, on the pane rect (SE_TabWidgetTabPane)
                if (StyleManager::stylesheetsDisabled())
                {
                    if (const TabElement* tabElement = TabElement::forTabWidget(widget))
                    {
                        TabElement::drawFrame(painter, option->rect, tabElement->m_paneFill, tabElement->m_paneEdges);
                        return;
                    }
                    // A plain pane is the base style's, drawn over the palette the legacy sheet gave the tab widget
                    const TabElement* plain = TabElement::plainForTabWidget(widget);
                    auto frameOption = qstyleoption_cast<const QStyleOptionTabWidgetFrame*>(option);
                    if (plain && frameOption)
                    {
                        QStyleOptionTabWidgetFrame plainOption = *frameOption;
                        plain->applyPalette(plainOption.palette);
                        QProxyStyle::drawPrimitive(element, &plainOption, painter, widget);
                        return;
                    }
                }
            }
            break;

            case PE_PanelButtonCommand:
            {
                // A flat push button has no face in any state: `QPushButton:flat { border: none; background-color: transparent }`
                // (PushButton.qss:48-52) left the sheet style nothing to draw over it, pressed or checked, where Fusion draws
                // the face of a button that is down (QCommonStyle::drawControl, CE_PushButtonBevel; QFusionStyle::drawPrimitive). The menu
                // indicator of a flat button is the bevel's, not the panel's.
                if (StyleManager::stylesheetsDisabled() && qobject_cast<const QPushButton*>(widget))
                {
                    auto buttonOption = qstyleoption_cast<const QStyleOptionButton*>(option);
                    if (buttonOption && buttonOption->features.testFlag(QStyleOptionButton::Flat))
                    {
                        return;
                    }
                }
            }
            break;

            case PE_IndicatorRadioButton:
            {
                if (StyleManager::stylesheetsDisabled())
                {
                    // In the indicator element's image rect (IndicatorElement::imageRect), the focus size while the button has the focus
                    drawSvg(painter, flatIndicatorImage(g_radioButtonImages, option->state), option->rect);
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
                auto rowOption = qstyleoption_cast<const QStyleOptionViewItem*>(option);
                auto* tableView = qobject_cast<const TableView*>(widget);
                const bool flipped = tableView && tableView->alternatingRowColors() && rowOption;
                if (StyleManager::stylesheetsDisabled() && rowOption)
                {
                    // A row's panel as the sheet drew it (QStyleSheetStyle::drawPrimitive): the base style's (the alternate colour, the
                    // selection), then the rule of the item over it in the state of the row. The rule is not asked for the empty
                    // space of the view (a position the view does not give).
                    if (const ItemViewElement* items = ItemViewElement::forWidget(widget))
                    {
                        auto copy = *rowOption;
                        if (flipped)
                        {
                            copy.features ^= QStyleOptionViewItem::Alternate;
                        }
                        QProxyStyle::drawPrimitive(element, &copy, painter, widget);
                        if (rowOption->viewItemPosition != QStyleOptionViewItem::Invalid)
                        {
                            items->drawFill(painter, &copy);
                        }
                        return;
                    }
                }
                if (flipped)
                {
                    auto copy = *rowOption;
                    copy.features ^= QStyleOptionViewItem::Alternate;
                    return QProxyStyle::drawPrimitive(element, &copy, painter, widget);
                }
            }
            break;

            case PE_PanelItemViewItem:
            {
                // The sheet drew the item's own panel from its rule alone, no model background and no selection of the base style
                // (QStyleSheetStyle::drawPrimitive, QRenderRule::drawRule)
                if (StyleManager::stylesheetsDisabled())
                {
                    if (const ItemViewElement* items = ItemViewElement::forWidget(widget))
                    {
                        items->drawFill(painter, option);
                        return;
                    }
                }
            }
            break;
        }

        return QProxyStyle::drawPrimitive(element, option, painter, widget);
    }

    void Style::drawComplexControl(QStyle::ComplexControl element, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget) const
    {
        if (GuiPerfFloor::isPassthrough())
        {
            QProxyStyle::drawComplexControl(element, option, painter, widget);
            return;
        }

        if (!hasStyle(widget))
        {
            QProxyStyle::drawComplexControl(element, option, painter, widget);
            return;
        }

        prepPainter(painter);

        switch (element)
        {
            case CC_SpinBox:
                // A QSpinBox or a QDoubleSpinBox, flattened: the spin box element
                if (StyleManager::stylesheetsDisabled()
                    && SpinBoxElement::draw(qstyleoption_cast<const QStyleOptionSpinBox*>(option), painter, widget))
                {
                    return;
                }
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
                // The element draws a tool button whose border is its own as its rule did (QStyleSheetStyle::drawComplexControl)
                if (StyleManager::stylesheetsDisabled())
                {
                    auto toolOption = qstyleoption_cast<const QStyleOptionToolButton*>(option);
                    const BoxElement* box = BoxElement::forWidget(widget);
                    if (box && toolOption && box->drawToolButton(this, toolOption, painter, widget))
                    {
                        return;
                    }
                }
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
        if (GuiPerfFloor::isPassthrough())
        {
            return QProxyStyle::generatedIconPixmap(iconMode, pixmap, option);
        }

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
        if (GuiPerfFloor::isPassthrough())
        {
            return QProxyStyle::subControlRect(control, option, subControl, widget);
        }

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
                    // The element boxes a combo box as its rule did (QStyleSheetStyle::subControlRect): the frame inside the margin, the
                    // edit field and the drop-down in its contents; the base style's rects for the rest
                    if (StyleManager::stylesheetsDisabled())
                    {
                        const BoxElement* box = BoxElement::forWidget(widget);
                        if (box && box->ownsBox(option) && (subControl == SC_ComboBoxFrame || subControl == SC_ComboBoxEditField || subControl == SC_ComboBoxArrow))
                        {
                            return subControl == SC_ComboBoxFrame ? box->borderRect(option)
                                 : subControl == SC_ComboBoxEditField ? box->editFieldRect(option, widget)
                                                                      : box->dropDownRect(option, widget);
                        }
                    }

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
                                return QRect();

                            case SC_ScrollBarGroove:
                                return option->rect;

                            case SC_ScrollBarSlider:
                            {
                                const bool horizontal = sliderOption->orientation == Qt::Horizontal;
                                const int trackLength = horizontal ? option->rect.width() : option->rect.height();
                                const int range = sliderOption->maximum - sliderOption->minimum;
                                int sliderLength = range <= 0
                                    ? trackLength
                                    : (sliderOption->pageStep * trackLength) / (range + sliderOption->pageStep);
                                sliderLength = qBound(g_scrollHandleMin, sliderLength, trackLength);
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

                        if (StyleManager::stylesheetsDisabled())
                        {
                            // The element's, for every QSpinBox and QDoubleSpinBox (the rules named the Qt classes, SpinBox.qss:9-10)
                            r = SpinBoxElement::editFieldRect(option->rect, widget);
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
                // The element boxes a tool button as its rule did (QStyleSheetStyle::subControlRect): the button inside the margin, the
                // menu button at its right, the base style's rects for the rest in the rect inside the margin
                if (StyleManager::stylesheetsDisabled())
                {
                    auto toolOption = qstyleoption_cast<const QStyleOptionToolButton*>(option);
                    if (const BoxElement* box = BoxElement::forWidget(widget); box && toolOption && qobject_cast<const QToolButton*>(widget) && box->ownsBox(option))
                    {
                        if (subControl == SC_ToolButton || subControl == SC_ToolButtonMenu)
                        {
                            return box->toolButtonRect(toolOption, subControl, ToolButton::menuButtonIndicatorWidth(this, option, widget, m_data->toolButtonConfig));
                        }
                        QStyleOptionToolButton inset(*toolOption);
                        inset.rect = box->borderRect(option);
                        return QProxyStyle::subControlRect(control, &inset, subControl, widget);
                    }
                }

                // ToolButton::subControlRect insets the rect by PM_ButtonMargin on every side for any tool button
                // option. Only a QToolButton has that margin (ToolButton::buttonMargin): the title buttons of a dock
                // widget are QAbstractButtons whose 18x14 rect it left a 6x2 label box (QCommonStyle gives the base
                // style's rect to the label, QCommonStyle::drawComplexControl), so their glyphs were never drawn.
                if (StyleManager::stylesheetsDisabled() && widget && !qobject_cast<const QToolButton*>(widget))
                {
                    break;
                }
                return ToolButton::subControlRect(this, option, subControl, widget, m_data->toolButtonConfig);
            }
        }

        return QProxyStyle::subControlRect(control, option, subControl, widget);
    }

    //////////////////////////////////////////////////////////////////////////
    // Flattened label box: Text.qss gives every QLabel a vertical margin per typography class. It reached Qt as the label's frame box
    // (SE_ShapedFrameContents), which also switches on QLabel's automatic half-'x' text indent, so it is carried at that contact point
    // rather than as a contents margin.
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
            // The labels of a chip have no margin, but its icon's on the right and its text's on both sides
            //! FilteredSearchWidget.qss:59, :64-65 $space.label.idle.margin-right.Alt2 $space.label.idle.margin-left.Alt1
            constexpr int chipLabelMargin = 4;
            if (widget->objectName() == QLatin1String("tagIcon"))
            {
                return option->rect.adjusted(0, 0, -chipLabelMargin, 0);
            }
            if (widget->objectName() == QLatin1String("tagText"))
            {
                return option->rect.adjusted(chipLabelMargin, 0, -chipLabelMargin, 0);
            }
            return option->rect;
        }
        if (parent && parent->inherits("AzQtComponents::VectorElement"))
        {
            // VectorInput.qss: VectorElement QLabel { margin-top: 2px; margin-bottom: 2px }
            return option->rect.adjusted(0, 2, 0, -2);
        }
        if (Selectors::named(widget, "title") && Selectors::withinClass(widget, "AzQtComponents::TitleBar"))
        {
            // TitleBar.qss:31-37 `AzQtComponents--TitleBar #title { margin-left: 10px; margin-right: 16px }` and
            // Text.qss:93-94 QLabel { margin-top: 1px; margin-bottom: 1px }. The label sits in a container of the
            // bar, not directly in it (Titlebar.cpp:150, 161-162): the selector matches at any depth.
            return option->rect.adjusted(10, 1, -16, -1);
        }

        // The margin above and below of the label's typography tier, the first tag that has one (a tag is a whole word compared with
        // case, QCss::StyleSelector::basicSelectorMatches); the label rule's where none has
        struct TierMargin
        {
            const char* m_tag;
            int m_margin;
        };
        static constexpr TierMargin tiers[] = {
            { "Headline", 4 },      // Text.qss:60-61 $space.label.idle.margin-top.Alt6 $space.label.idle.margin-bottom.Alt6
            { "Paragraph", 4 },     // Text.qss:108-109 $space.label.idle.margin-top.Alt6 $space.label.idle.margin-bottom.Alt6
            { "Title", 7 },         // Text.qss:68-69 $space.label.idle.margin-top.Alt4 $space.label.idle.margin-bottom.Alt4
            { "Subtitle", 8 },      // Text.qss:77-78 $space.label.idle.margin-top.Alt8 $space.label.idle.margin-bottom.Alt9
            { "Menu", 6 },          // Text.qss:85-86 $space.label.idle.margin-top.Alt5 $space.label.idle.margin-bottom.Alt5
            { "Button", 6 },        // Text.qss:125-126 $space.label.idle.margin-top.Alt5 $space.label.idle.margin-bottom.Alt5
            { "Tooltip", 2 },       // Text.qss:116-117 $space.label.idle.margin-top.Alt2 $space.label.idle.margin-bottom.Alt2
        };
        int margin = Foundations::Label::MarginV;
        for (const TierMargin& tier : tiers)
        {
            if (Selectors::tagged(widget, tier.m_tag))
            {
                margin = tier.m_margin;
                break;
            }
        }
        return option->rect.adjusted(0, margin, 0, -margin);
    }

    QRect Style::subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget) const
    {
        if (GuiPerfFloor::isPassthrough())
        {
            return QProxyStyle::subElementRect(element, option, widget);
        }

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
                    // The element is the whole definition of its frame (QStyleSheetStyle::subElementRect): its contents rect where the
                    // rule has a box or a border of its own, else Qt's. None of the emulations below is the element's.
                    if (const BoxElement* box = BoxElement::forWidget(widget))
                    {
                        return box->ownsBox(option) ? visualRect(option->direction, option->rect, box->contentsRect(option, widget))
                                                    : QProxyStyle::subElementRect(element, option, widget);
                    }

                    // The sheet's box (margin, border, padding) of a QFrame reached Qt through this element; so do the flattened ones
                    QRect r = Card::shapedFrameContents(option, widget);
                    if (r.isNull() && widget && widget->inherits("AzQtComponents::BrowseEdit"))
                    {
                        // The BrowseEdit's margin
                        r = option->rect.marginsRemoved(Foundations::Field::Margin);
                    }
                    if (r.isNull() && widget
                        && (widget->inherits("AzQtComponents::FilterCriteriaButton") || widget->inherits("AzQtComponents::FilterTextButton")
                            || widget->inherits("AzQtComponents::SelectionCountButton")))
                    {
                        // The line around a chip
                        r = option->rect.adjusted(g_chipLineWidth, g_chipLineWidth, -g_chipLineWidth, -g_chipLineWidth);
                    }
                    if (r.isNull() && widget && widget->inherits("AzQtComponents::SegmentBar"))
                    {
                        r = option->rect.adjusted(0, 0, 0, -g_segmentBarMarginBottom);
                    }
                    if (r.isNull() && widget && widget->inherits("AzQtComponents::WindowDecorationWrapper"))
                    {
                        // The box of the window frame, whose line WindowDecorationWrapper::paintEvent draws
                        r = option->rect.adjusted(g_windowFrameWidth, g_windowFrameWidth, -g_windowFrameWidth, -g_windowFrameWidth);
                    }
                    if (r.isNull() && widget && widget->inherits("QTipLabel"))
                    {
                        // A tooltip's margin: no QLabel rule reaches it (the sheet names a QTipLabel QToolTip only)
                        r = option->rect.marginsRemoved(g_toolTipMargins);
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
            case SE_CheckBoxIndicator:
            case SE_RadioButtonIndicator:
            case SE_CheckBoxContents:
            case SE_RadioButtonContents:
            {
                // The indicator element in the box of the element (QStyleSheetStyle::subElementRect, SE_CheckBoxIndicator): the indicator
                // at the left of the contents, centered vertically, and the label after it and the label spacing
                const BoxElement* box = StyleManager::stylesheetsDisabled() ? BoxElement::forWidget(widget) : nullptr;
                const IndicatorElement* indicator = box ? IndicatorElement::forWidget(widget) : nullptr;
                if (indicator)
                {
                    const QRect area = box->contentsRect(option, widget);
                    if (element == SE_CheckBoxIndicator || element == SE_RadioButtonIndicator)
                    {
                        return QStyle::alignedRect(option->direction, Qt::AlignLeft | Qt::AlignVCenter, indicator->boxSize(), area);
                    }
                    const int spacing = pixelMetric(element == SE_RadioButtonContents ? PM_RadioButtonLabelSpacing : PM_CheckBoxLabelSpacing, nullptr, widget);
                    return indicator->contentsRect(area, spacing, option->direction);
                }
            }
            break;
            case SE_CheckBoxClickRect:
            {
                // Where a click toggles a check box: its box and its label's text, laid out as the legacy sheet's indicator and contents
                // were (QStyleSheetStyle::subElementRect leaves it to QCommonStyle::subElementRect), inside the box of its element
                const IndicatorElement* indicator = StyleManager::stylesheetsDisabled() ? IndicatorElement::forWidget(widget) : nullptr;
                auto buttonOption = qstyleoption_cast<const QStyleOptionButton*>(option);
                if (indicator && buttonOption)
                {
                    // QCommonStyle lays the focus rectangle on SE_CheckBoxContents, which asks the spacing with no option (:6200)
                    const BoxElement* box = BoxElement::forWidget(widget);
                    return indicator->clickRect(this, *buttonOption, pixelMetric(PM_CheckBoxLabelSpacing, nullptr, widget),
                        box ? box->contentsRect(option, widget) : option->rect);
                }
            }
            break;
            case SE_RadioButtonClickRect:
            {
                // A radio button whose indicator the sheet boxes (RadioButton.qss:24-33) is clicked anywhere in its rectangle, with a label
                // or without: QStyleSheetStyle::subElementRect returns the widget's rect
                if (StyleManager::stylesheetsDisabled() && IndicatorElement::forWidget(widget))
                {
                    return option->rect;
                }
            }
            break;
            case SE_TabWidgetTabBar:
            case SE_TabWidgetTabPane:
            case SE_TabWidgetTabContents:
            {
                // The tab element lays out its tab widget the way the stylesheet did: the bar at
                // its left offset with a minimum slot height, the pane below the slot (or below the
                // empty offset when there are no tabs), the contents inside the pane's edges and
                // padding (legacy geometry: Bordered bar (1, 0, w, 33), pages (3, 35)).
                const TabElement* tabElement = StyleManager::stylesheetsDisabled() ? TabElement::forTabWidget(widget) : nullptr;
                auto frame = qstyleoption_cast<const QStyleOptionTabWidgetFrame*>(option);
                if (tabElement && frame)
                {
                    // Pane top = the bar's slot (when there are tabs) + the empty offset while the
                    // widget carries the "Empty" class tag (TabWidget.Empty:pane { margin-top }). The
                    // tag can outlive the first tab (log panels keep it), and the offset then sits
                    // below the bar, as in the legacy run.
                    const QRect area = frame->rect;
                    const int barSlot = frame->tabBarSize.isEmpty() ? 0 : qMax(frame->tabBarSize.height(), tabElement->m_barAreaMinHeight);
                    const int slot = barSlot + (hasClass(widget, QStringLiteral("Empty")) ? tabElement->m_paneEmptyOffset : 0);
                    if (element == SE_TabWidgetTabBar)
                    {
                        const int left = area.left() + frame->leftCornerWidgetSize.width() + tabElement->m_barLeft;
                        const int room = area.right() - frame->rightCornerWidgetSize.width() - left + 1;
                        return QRect(left, area.top(), qMax(0, qMin(frame->tabBarSize.width(), room)),
                            qMax(frame->tabBarSize.height(), tabElement->m_barAreaMinHeight));
                    }
                    const QRect pane(area.left(), area.top() + slot, area.width(), area.height() - slot);
                    if (element == SE_TabWidgetTabPane)
                    {
                        return pane;
                    }
                    const TabElement::Edges& edges = tabElement->m_paneEdges;
                    const QMargins& padding = tabElement->m_panePadding;
                    return pane.adjusted(edges.m_left.m_width + padding.left(), edges.m_top.m_width + padding.top(),
                        -(edges.m_right.m_width + padding.right()), -(edges.m_bottom.m_width + padding.bottom()));
                }
            }
            break;
            case SE_ItemViewItemCheckIndicator: // intentional fall-through
            case SE_ItemViewItemText:           // intentional fall-through
            case SE_ItemViewItemDecoration:     // intentional fall-through
            case SE_ItemViewItemFocusRect:
            {
                // The item's box (TableView.qss:31-40 `::item { padding-left: 4px }`) leaves a rect for the check box, the icon and the
                // text, which the base style lays out like the whole rect (QStyleSheetStyle::subElementRect). Only a part the item has
                // is laid out in it: an icon or a check box the item lacks is where the base style puts it.
                if (StyleManager::stylesheetsDisabled())
                {
                    auto viewOption = qstyleoption_cast<const QStyleOptionViewItem*>(option);
                    const ItemViewElement* items = viewOption ? ItemViewElement::forWidget(widget) : nullptr;
                    if (items
                        && (element == SE_ItemViewItemText || element == SE_ItemViewItemFocusRect
                            || (element == SE_ItemViewItemDecoration && viewOption->features.testFlag(QStyleOptionViewItem::HasDecoration))
                            || (element == SE_ItemViewItemCheckIndicator && viewOption->features.testFlag(QStyleOptionViewItem::HasCheckIndicator))))
                    {
                        QStyleOptionViewItem inner(*viewOption);
                        inner.rect = items->itemContentsRect(viewOption->rect);
                        return QProxyStyle::subElementRect(element, &inner, widget);
                    }
                    if (items)
                    {
                        ++s_itemViewBaseIndicator;
                        const QRect baseRect = QProxyStyle::subElementRect(element, option, widget);
                        --s_itemViewBaseIndicator;
                        return baseRect;
                    }
                }
                if (StyleManager::stylesheetsDisabled() && element == SE_ItemViewItemText && ComboPopupElement::coversView(widget))
                {
                    // ComboBox.qss QComboBox QAbstractItemView::item { padding-left: 24px;
                    // padding-right: 24px } (room for the delegate's check mark), from the popup element.
                    const ComboPopupElement& popup = ComboPopupElement::global();
                    return QProxyStyle::subElementRect(element, option, widget).adjusted(popup.m_itemPaddingLeft, 0, -popup.m_itemPaddingRight, 0);
                }
                auto optionItemView = qstyleoption_cast<const QStyleOptionViewItem*>(option);
                if (element != SE_ItemViewItemCheckIndicator && qobject_cast<const TableView*>(widget) && optionItemView)
                {
                    QRect r = TableView::itemViewItemRect(this, element, optionItemView, widget, m_data->tableViewConfig);
                    if (!r.isNull())
                    {
                        return r;
                    }
                }
            }
            break;
            case SE_PushButtonContents:     // intentional fall-through
            case SE_PushButtonFocusRect:
            {
                // The label box and the focus rect of a button whose box the element owns (QStyleSheetStyle::subElementRect)
                const BoxElement* box = StyleManager::stylesheetsDisabled() ? BoxElement::forWidget(widget) : nullptr;
                if (box && box->ownsBox(option))
                {
                    return visualRect(option->direction, option->rect, box->contentsRect(option, widget));
                }
            }
            break;
            case SE_LineEditContents:
            {
                // The element is the whole definition of its field (QStyleSheetStyle::subElementRect): its contents rect where the rule
                // has a box or a border of its own, else Qt's. QLineEdit lays its text out in it.
                if (StyleManager::stylesheetsDisabled())
                {
                    if (const BoxElement* box = BoxElement::forWidget(widget))
                    {
                        return box->ownsBox(option) ? visualRect(option->direction, option->rect, box->contentsRect(option, widget))
                                                    : QProxyStyle::subElementRect(element, option, widget);
                    }
                }

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
        if (GuiPerfFloor::isPassthrough())
        {
            return QProxyStyle::pixelMetric(metric, option, widget);
        }

        if (!hasStyle(widget))
        {
            return QProxyStyle::pixelMetric(metric, option, widget);
        }

        if (StyleManager::stylesheetsDisabled())
        {
            // Indicator footprints from CheckBox.qss / RadioButton.qss (16x16; toggle switch 32x16)
            switch (metric)
            {
                case PM_TabBarTabHSpace:
                case PM_TabBarTabVSpace:
                case PM_TabBarTabShiftHorizontal:
                case PM_TabBarTabShiftVertical:
                    // A tab element's box replaces Qt's tab spacing and the selected-tab shift
                    // (QStyleSheetStyle returned 0 for a tab rule with a box or border)
                    if (TabElement::forTabBar(widget))
                    {
                        return 0;
                    }
                    break;
                case PM_MenuButtonIndicator:
                    // A tool button whose rule has a box or a border of its own has no room for the indicator in its hint
                    // (QStyleSheetStyle::pixelMetric)
                    if (const BoxElement* box = BoxElement::forWidget(widget); box && qobject_cast<const QToolButton*>(widget) && box->ownsBox(option))
                    {
                        return 0;
                    }
                    break;
                case PM_CheckBoxLabelSpacing:
                case PM_RadioButtonLabelSpacing:
                    // The `spacing` of the rule of the state (QStyleSheetStyle::pixelMetric): of a check box and of a radio button; a group
                    // asks the check box metric too and reads its title's spacing, which no element carries
                    if (const BoxElement* box = BoxElement::forWidget(widget);
                        box && (metric == PM_RadioButtonLabelSpacing || qobject_cast<const QCheckBox*>(widget)))
                    {
                        if (const int labelSpacing = box->spacing(option); labelSpacing != BoxElement::Unset)
                        {
                            return labelSpacing;
                        }
                    }
                    break;
                case PM_IndicatorWidth:
                case PM_IndicatorHeight:
                    // The check box of an item is the room its rule makes: the image and the margin around it (CheckBox.qss:23-28,
                    // TableView.qss:78-85; QStyleSheetStyle::pixelMetric). Any other widget: its indicator element's image
                    if (qstyleoption_cast<const QStyleOptionViewItem*>(option))
                    {
                        if (s_itemViewBaseIndicator > 0)
                        {
                            break;
                        }
                        if (const ItemViewElement* items = ItemViewElement::forWidget(widget))
                        {
                            return metric == PM_IndicatorWidth ? items->indicatorBox().width() : items->indicatorBox().height();
                        }
                    }
                    if (const IndicatorElement* indicator = IndicatorElement::forWidget(widget))
                    {
                        return metric == PM_IndicatorWidth ? indicator->m_imageSize.width() : indicator->m_imageSize.height();
                    }
                    return metric == PM_IndicatorWidth ? IndicatorElement::ImageSize.width() : IndicatorElement::ImageSize.height();
                case PM_ExclusiveIndicatorWidth:
                case PM_ExclusiveIndicatorHeight:
                    return metric == PM_ExclusiveIndicatorWidth ? IndicatorElement::ImageSize.width() : IndicatorElement::ImageSize.height();
                case PM_MenuHMargin:
                    // Menu.qss:13 QMenu padding: 4px 2px (the left padding), in the menu element
                    return MenuElement::forWidget(widget).m_paddingH;
                case PM_MenuVMargin:
                    // Menu.qss:13 (the top padding)
                    return MenuElement::forWidget(widget).m_paddingV;
                case PM_ScrollBarExtent:
                    return g_scrollBarExtent;
                case PM_ScrollBarSliderMin:
                    return g_scrollHandleMin;
                case PM_SplitterWidth:
                    // QSplitter asks with no option, and the sheet style then read the handle rule in the state of a vertical handle
                    // (QStyleSheetStyle::pixelMetric, PM_SplitterWidth). QSplitterHandle widens its widget with contents margins, grabbing
                    // the panes at its sides (QSplitterHandle::resizeEvent), and paints the pixels between them (CE_Splitter).
                    return g_splitterWidth;
                case PM_ToolBarSeparatorExtent:
                    return g_toolBarSeparatorWidth;
                case PM_ToolBarHandleExtent:
                {
                    // ToolBar.qss QToolBar::handle: the image and the padding of the winning rule, 10px for most
                    // toolbars (the base style's is 9)
                    const int extent = ControlElements::toolBarHandleExtent(option, widget);
                    if (extent >= 0)
                    {
                        return extent;
                    }
                    break;
                }
                case PM_DockWidgetFrameWidth:
                {
                    // StyledDockWidget.qss:9-18: a floating dock without a native frame reserves around its contents the border of the rule
                    // it was polished with (0 for a dock polished docked)
                    const int frameWidth = ControlElements::dockFrameWidth(widget);
                    if (frameWidth >= 0)
                    {
                        return frameWidth;
                    }
                    break;
                }
                case PM_MenuBarVMargin:
                case PM_MenuBarHMargin:
                case PM_MenuBarItemSpacing:
                case PM_MenuBarPanelWidth:
                    // MenuBar.qss:15-16 QMenuBar { margin: 0; spacing: 0 }; the item box carries the padding
                    // (CT_MenuBarItem) and the bar its bottom border (CE_MenuBarEmptyArea).
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

    //////////////////////////////////////////////////////////////////////////
    // Flattened typography (GUI flattening)
    //
    // Text.qss set the colors of QLabel via the secondaryText, highlightedText and
    // blackText class tags (Text::addSecondaryStyle, addHighlightedStyle, addBlackStyle).
    // The font sizes and weights of the tiers are TextElement's, applied first in
    // Style::polish.
    //////////////////////////////////////////////////////////////////////////
    static void applyFlattenedTypographyStyle(QWidget* widget)
    {
        if (!widget->inherits("QLabel"))
        {
            return;
        }

        // The class tags arrive after construction; re-read the label's frame box
        // (flattenedLabelFrameContents) now that they are known.
        if (auto frame = qobject_cast<QFrame*>(widget))
        {
            frame->setFrameStyle(frame->frameStyle());
        }

        // Text.qss colour tags, whole words compared with case (QCss::StyleSelector::basicSelectorMatches)
        QColor textColor;
        if (Selectors::tagged(widget, "secondaryText"))
        {
            textColor = QColor(0x88, 0x88, 0x88);
        }
        else if (Selectors::tagged(widget, "highlightedText"))
        {
            textColor = QColor(0x44, 0xB2, 0xF8);
        }
        else if (Selectors::tagged(widget, "blackText"))
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
    // Flattened text-entry palette: the entry field (BaseStyleSheet.qss:68-84) and the text edits' (:86-95), with the :disabled field
    // (LineEdit.qss:28-35, SpinBox.qss:69-74, ComboBox.qss:53-57), per widget class: the application palette keeps Base dark for the
    // item views
    //////////////////////////////////////////////////////////////////////////
    static constexpr QColor g_textEditFill(0xE9, 0xE9, 0xE9);      // BaseStyleSheet.qss:90 $fill.field.idle.Alt2
    static constexpr QColor g_textEditText(0x54, 0x54, 0x54);      // BaseStyleSheet.qss:91 $text.field.idle.Alt2

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

        // Fusion paints the closed face of a combo box and the buttons of a spin box from Button and ButtonText: the field's too
        const bool buttonFace = widget->inherits("QComboBox") || widget->inherits("QAbstractSpinBox");
        // The selection is the application's: under the sheet no field took its parent's palette (QWidgetPrivate::naturalWidgetPalette),
        // so an editor in a view does not show the view's selection roles
        const QPalette application = QApplication::palette(widget);
        QPalette entryPalette = widget->palette();
        for (const auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
        {
            const bool disabled = group == QPalette::Disabled;
            const QColor fill = disabled ? Foundations::Field::DisabledFill : (isTextEdit ? g_textEditFill : Foundations::Field::Fill);
            const QColor text = disabled ? Foundations::Field::DisabledText : (isTextEdit ? g_textEditText : Foundations::Field::Text);
            entryPalette.setColor(group, QPalette::Base, fill);
            entryPalette.setColor(group, QPalette::Text, text);
            entryPalette.setColor(group, QPalette::Highlight, application.color(group, QPalette::Highlight));
            entryPalette.setColor(group, QPalette::HighlightedText, application.color(group, QPalette::HighlightedText));
            if (buttonFace)
            {
                entryPalette.setColor(group, QPalette::Button, fill);
                entryPalette.setColor(group, QPalette::ButtonText, text);
            }
        }

        if (isTextEdit)
        {
            // The sheet's background covered the whole widget, its scroll bar lanes included, and the widget had no frame: the field
            // reaches the edges, under the scroll bar
            entryPalette.setColor(QPalette::Window, g_textEditFill);
            widget->setAutoFillBackground(true);
            if (auto frame = qobject_cast<QFrame*>(widget))
            {
                frame->setFrameShape(QFrame::NoFrame);
            }
        }

        widget->setPalette(entryPalette);
    }

    //////////////////////////////////////////////////////////////////////////
    // Flattened disabled text: the universal rule's white reached every group, and only a :disabled rule of a widget's own class takes it
    // back (QStyleSheetStyle::setPalette configures each group from the rules of its state; a colour sets WindowText, Text and
    // ButtonText together, QRenderRule::configurePalette). The application palette keeps the white (buildO3DEBasePalette); the classes
    // with a :disabled colour are carried here, last in Style::polish, once the class tags a component's polish adds are known
    // (PushButton::polish tags a default button Primary). A tree item's colour belongs to the item rule (CE_ItemViewItem).
    //////////////////////////////////////////////////////////////////////////
    static void applyFlattenedDisabledText(QWidget* widget)
    {
        QColor disabledText;
        if (auto pushButton = qobject_cast<QPushButton*>(widget))
        {
            // `QPushButton.Primary:disabled, QPushButton:default:disabled` (PushButton.qss:60-64) after QPushButton:disabled (:43-46)
            //! PushButton.qss:63 $text.button.disabled.Alt2 $text.button.default+disabled
            const QColor primaryText(0xBB, 0xBB, 0xBB);
            const QColor buttonText(0x99, 0x99, 0x99);          // PushButton.qss:45 $text.button.disabled.Alt1
            disabledText = (pushButton->isDefault() || Selectors::tagged(widget, "Primary")) ? primaryText : buttonText;
        }
        else if (widget->inherits("QCheckBox") || widget->inherits("QRadioButton"))
        {
            disabledText = QColor(0x99, 0x99, 0x99);           // CheckBox.qss:20, RadioButton.qss:19 $text.check.disabled
        }
        else if (widget->inherits("QLineEdit") || widget->inherits("QTextEdit") || widget->inherits("QPlainTextEdit")
            || widget->inherits("QSpinBox") || widget->inherits("QDoubleSpinBox") || widget->inherits("QComboBox"))
        {
            // The field's (no rule names another QAbstractSpinBox); applyFlattenedTextEntryPalette sets Text too, except on the line
            // edits of a BrowseEdit or a spin box
            disabledText = Foundations::Field::DisabledText;
        }
        else if (widget->inherits("QLabel") && Selectors::withinClass(widget, "AzQtComponents::Card"))
        {
            // More specific than a colour tag such as `.secondaryText` (Text.qss:16-19)
            disabledText = QColor(0x99, 0x99, 0x99);           // Card.qss:274 $text.label.disabled.Alt1
        }
        if (!disabledText.isValid())
        {
            return;
        }

        QPalette palette = widget->palette();
        bool changed = false;
        for (const QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText })
        {
            if (palette.color(QPalette::Disabled, role) != disabledText)
            {
                palette.setColor(QPalette::Disabled, role, disabledText);
                changed = true;
            }
        }
        if (changed)
        {
            widget->setPalette(palette);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Flattened parented item views: BaseStyleSheet.qss:23-30 gave a QTableView, QListView or QTreeView a fill, and :32-38 reset it to
    // transparent for a view whose parent is a widget (`QWidget > QTreeView`: the child combinator follows the QObject parent,
    // parentObject). Only a view without a parent, a completer popup, kept the fill. A transparent rule leaves the viewport's background
    // role (Base) a null brush and Window transparent (QRenderRule::configurePalette on the embedded viewport), and takes the viewport's
    // own fill away. The application palette holds the opaque value, so a parented view gets the rule's here.
    //
    // A view whose Base is not the application's was given its fill by code, or by a more specific rule carried as an element, and
    // keeps it: the Add Component tree (Editor.qss:38-41; ComponentPaletteWidget.cpp:88-107) and the popup list of a combo box (the menu
    // palette). A QHeaderView is a QAbstractItemView, but not a class the rule names; its palette is carried with it (Style::polish).
    //////////////////////////////////////////////////////////////////////////
    static void applyFlattenedParentedItemView(QAbstractItemView* itemView)
    {
        if (!qobject_cast<QWidget*>(itemView->parent())
            || !(itemView->inherits("QTableView") || itemView->inherits("QListView") || itemView->inherits("QTreeView")))
        {
            return;
        }
        if (itemView->palette().color(QPalette::Active, QPalette::Base) != QApplication::palette().color(QPalette::Active, QPalette::Base))
        {
            return;
        }

        QPalette palette = itemView->palette();
        for (const auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
        {
            palette.setBrush(group, QPalette::Base, QBrush(Qt::NoBrush));
            palette.setColor(group, QPalette::Window, Foundations::Clear);     // BaseStyleSheet.qss:37 $fill.view.idle.fill.Clear
        }
        itemView->setPalette(palette);
        itemView->viewport()->setAutoFillBackground(false);
    }

    void Style::polish(QWidget* widget)
    {
        GuiPerfCounters::PolishScope perfScope(widget);     // counts and times this call, if O3DE_UI_PERF_COUNTERS is set

        if (GuiPerfFloor::isPassthrough())
        {
            QProxyStyle::polish(widget);
            return;
        }

        static QWidget* alreadyStyling = nullptr;
        if (alreadyStyling == widget)
        {
            return;
        }

        QScopedValueRollback<QWidget*> recursionGuard(alreadyStyling, widget);

        if (StyleManager::stylesheetsDisabled())
        {
            // Fonts first: the text element of the widget (its class tags, its place in the window) over the font
            // Qt gives it. Under an inline style sheet too: QStyleSheetStyle::polish calls this style's polish first.
            TextElement::resolve(widget);

            // Frames: the frame element of a QFrame, a scroll area or a module's frame, resolved once here (adding or
            // removing a class tag re-polishes, so it is resolved again)
            FrameElement::resolve(widget);

            applyFlattenedTextEntryPalette(widget);

            // The popup of a combo box: its container and the list view inside each take the element's palette
            // (ComboBox.qss:79-117). A combo box hands its own palette to its container whenever that changes
            // (QComboBoxPrivate::updateViewContainerPaletteAndOpacity) and so replaces the container's palette whole, but a role
            // the view set on itself stays.
            if (ComboPopupElement::covers(widget))
            {
                widget->setPalette(ComboPopupElement::global().palette(widget->palette()));
            }

            // LineEdit.qss:42-48: the action buttons of a line edit have the box of the flattened search fields
            if (widget->inherits("QLineEditIconButton"))
            {
                widget->setFixedSize(g_lineEditActionButtonSize, g_lineEditActionButtonSize);
            }

            // A QFrame reads its frame box (SE_ShapedFrameContents) while it is constructed, before its own class is
            // known to the style: read again, for the frames whose box the sheet gave them
            if (widget->inherits("AzQtComponents::WindowDecorationWrapper"))
            {
                if (auto frame = qobject_cast<QFrame*>(widget))
                {
                    frame->setFrameStyle(frame->frameStyle());
                }
            }

            // No frame for a QTableView, QListView or QTreeView (TableView.qss:28 `border: none`), no grid for a table (:11
            // $other.view.idle.showgrid)
            if (auto itemView = qobject_cast<QAbstractItemView*>(widget))
            {
                // The rule names those three classes, and ColorPicker.qss:97 the palette of the color picker (it is always inside one).
                // A view of another kind (the thumbnails of the asset folder: a QAbstractItemView of their own) kept the frame Qt's
                // default sheet gave every QFrame, a native border 1px wide (QStyleSheetStyle::getDefaultStyleSheet), which insets its
                // viewport by 1px; a header has no frame of its own.
                if (itemView->inherits("QTableView") || itemView->inherits("QListView") || itemView->inherits("QTreeView")
                    || qobject_cast<PaletteView*>(itemView))
                {
                    itemView->setFrameShape(QFrame::NoFrame);
                }
                if (auto tableView = qobject_cast<QTableView*>(widget))
                {
                    tableView->setShowGrid(false);
                }
                applyFlattenedParentedItemView(itemView);
            }

            // The viewport of a QScrollArea and a plain contents widget paint nothing (ScrollBar.qss:69-72 `QAbstractScrollArea >
            // .QWidget { background: none }`, and the universal rule): the scroll area's own background shows through
            if (auto scrollArea = qobject_cast<QScrollArea*>(widget))
            {
                QPalette viewportPalette = scrollArea->viewport()->palette();
                viewportPalette.setColor(QPalette::Window, Foundations::Clear);
                viewportPalette.setColor(QPalette::Base, Foundations::Clear);
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

            // The lane's fill under the pointer needs the hover attribute, which the sheet style gave every widget with a :hover rule
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

            // TabWidget.qss: the stacked page area of a tab widget sits on #444444
            if (widget->inherits("QStackedWidget") && Selectors::childOfClass(widget, "AzQtComponents::TabWidget"))
            {
                QPalette pageAreaPalette = widget->palette();
                pageAreaPalette.setColor(QPalette::Window, QColor(0x44, 0x44, 0x44));
                widget->setPalette(pageAreaPalette);
                widget->setAutoFillBackground(true);
            }

            // Tab elements: a tab bar resolves its element here, once (adding or removing a class tag
            // re-polishes; a module registering an element later has TabElement polish the bars again),
            // and the bar and its tab widget get what the element sets on them.
            TabElement::resolve(widget);

            // SegmentControl.qss:14-77: what the sheet set on the buttons of a segment bar when it polished them (ControlElements.cpp)
            ControlElements::polishSegmentBar(widget);

            // MenuBar.qss:11-12 QMenuBar { min-height: 28px } inside its 2px border: the bar is not lower than that, whatever
            // its items need (the sheet style set it as the minimum, QStyleSheetStyle::setGeometry)
            if (widget->inherits("QMenuBar"))
            {
                widget->setMinimumHeight(MenuBarElement::global().minHeight());
            }

            // Menu.qss:18 `QMenu::item .WidgetAction:hover`: a widget a menu hosts is filled while it is hovered
            // (MenuElement::drawHostedWidget, at PE_Widget). The sheet style gave such a widget a styled background
            // and hover (QStyleSheetStyle::polish).
            if (Selectors::tagged(widget, "WidgetAction") && Selectors::withinClass(widget, "QMenu"))
            {
                widget->setAttribute(Qt::WA_StyledBackground, true);
                widget->setAttribute(Qt::WA_Hover, true);
            }

            // Headers: TableView.qss flat #2d2d2d sections (Fusion draws from Button)
            if (widget->inherits("QHeaderView"))
            {
                QPalette headerPalette = widget->palette();
                // The colour of the section rule: the view's element has it (a module's header is its own colour), and fills the
                // area of the header beyond its sections too (ItemViewElement::resolve does the same)
                headerPalette.setColor(QPalette::Button, ItemViewElement::headerFillOf(widget));
                // The section rule sets no colour: the '*' rule's white (BaseStyleSheet.qss:18) reached the label
                // (ButtonText) and the sort arrow (WindowText) in every state, disabled included (the legacy
                // palette dump of LuaIDE's two disabled headers has both white in its disabled group)
                headerPalette.setColor(QPalette::ButtonText, QColor(Qt::white));
                headerPalette.setColor(QPalette::WindowText, QColor(Qt::white));
                // What shows through a section painted transparent by an inline sheet
                // (e.g. the Gallery log table) is the window colour, as in the legacy render.
                headerPalette.setColor(QPalette::Window, QColor(0x44, 0x44, 0x44));
                headerPalette.setColor(QPalette::Base, QColor(0x44, 0x44, 0x44));
                widget->setPalette(headerPalette);
                // The legacy header and its viewport were transparent (BaseStyleSheet.qss:15-21): the sections paint their own
                // fill, and what lies beyond the last one is what is behind the header
                if (auto header = qobject_cast<QHeaderView*>(widget))
                {
                    header->viewport()->setAutoFillBackground(false);
                }
            }

            // Text.qss class-driven label box and text colors
            applyFlattenedTypographyStyle(widget);

            // The corner tile of a scroll area: `[ShowBackground=true]`, a property the watcher toggles on hover
            if (widget->property("ShowBackground").isValid())
            {
                const QWidget* cornerScrollArea = widget->parentWidget();
                const bool darkCorner = cornerScrollArea && Style::hasClass(cornerScrollArea, QStringLiteral("DarkScrollBar"));
                const bool showCorner = widget->property("ShowBackground").toBool();

                QPalette cornerPalette = widget->palette();
                cornerPalette.setColor(QPalette::Window, showCorner ? (darkCorner ? g_darkCornerFill : g_cornerFill) : Foundations::Clear);
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

        if (StyleManager::stylesheetsDisabled())
        {
            // Last: the class tags the component polish above adds (Primary) are known now
            applyFlattenedDisabledText(widget);
        }

        QProxyStyle::polish(widget);

        // The elements go last, over what the base style and the components set (the sheet applied its geometry after:
        // QStyleSheetStyle::polish), and the text colour of a module's rule over the Global palettes above
        if (StyleManager::stylesheetsDisabled())
        {
            BoxElement::resolve(widget);
            ItemViewElement::resolve(widget);
            ScrollBar::applyBackgroundColor(widget);        // a console's colour: the inline sheet it had outranked every rule above
            TextElement::applyColor(widget);
            // The widgets under a color picker resolved again, the header of a palette card polished again (ColorPickerElements.h)
            ColorPickerElements::polished(widget);
        }
    }

    void Style::unpolish(QWidget* widget)
    {
        GuiPerfCounters::UnpolishScope perfScope(widget);   // counts and times this call, if O3DE_UI_PERF_COUNTERS is set

        if (GuiPerfFloor::isPassthrough())
        {
            QProxyStyle::unpolish(widget);
            return;
        }

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

        if (GuiPerfFloor::isPassthrough())
        {
            return;
        }

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
        if (GuiPerfFloor::isPassthrough())
        {
            return QProxyStyle::standardPalette();
        }

        return m_data->palette;
    }

    QIcon Style::standardIcon(QStyle::StandardPixmap standardIcon, const QStyleOption* option, const QWidget* widget) const
    {
        if (GuiPerfFloor::isPassthrough())
        {
            return QProxyStyle::standardIcon(standardIcon, option, widget);
        }

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
                // QDockWidget.qss:17 titlebar-normal-icon: the float button of a plain dock widget shows the pop-out glyph
                if (StyleManager::stylesheetsDisabled() && isFlattenedPlainDockWidget(widget))
                {
                    return QIcon(QStringLiteral(":/Application/titlebar-popout-small.svg"));
                }
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
        if (GuiPerfFloor::isPassthrough())
        {
            return QProxyStyle::styleHint(hint, option, widget, returnData);
        }

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

        // Tab bars: the tab element gives how long a moved tab takes to slide into place. QTabBar
        // animates only above 0; the legacy stylesheet set 0 (TabWidget.qss:46), so tabs snap.
        if (hint == QStyle::SH_Widget_Animation_Duration && StyleManager::stylesheetsDisabled() && widget && widget->inherits("QTabBar"))
        {
            if (const TabElement* tabElement = TabElement::forTabBar(widget))
            {
                return tabElement->m_moveAnimationDuration;
            }
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
        if (GuiPerfFloor::isPassthrough())
        {
            return QProxyStyle::eventFilter(watched, ev);
        }

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

            case QEvent::DynamicPropertyChange:
            {
                // Flattened GUI only. A class tag a label gets after its polish (a .ui file sets it in retranslateUi, after the layout of
                // the form polished the labels through QSplitter::addWidget: the Gallery's spin box page, `secondaryText`) was read by the
                // sheet style when the form was reparented, which polished its widgets again (QWidgetPrivate::inheritStyle,
                // setStyle_helper, under an application style sheet). The label is polished again here, as addClass does it.
                if (StyleManager::stylesheetsDisabled() && static_cast<QDynamicPropertyChangeEvent*>(ev)->propertyName() == "class"
                    && watched->isWidgetType() && watched->inherits("QLabel"))
                {
                    auto* label = static_cast<QWidget*>(watched);
                    if (label->testAttribute(Qt::WA_WState_Polished))
                    {
                        label->style()->unpolish(label);
                        label->style()->polish(label);
                    }
                }
            }
            break;
        }

        const bool flagToContinueProcessingEvent = false;
        return flagToContinueProcessingEvent;
    }

    //! The separator of the class tags, compiled once: hasClass runs for every widget polish
    static const QRegularExpression& classTagSeparator()
    {
        static const QRegularExpression separator(QStringLiteral("\\s+"));
        return separator;
    }

    bool Style::hasClass(const QWidget* button, const QString& className)
    {
        QVariant buttonClassVariant = button->property("class");
        if (buttonClassVariant.isNull())
        {
            return false;
        }

        QString classText = buttonClassVariant.toString();
        QStringList classList = classText.split(classTagSeparator());
        return classList.contains(className, Qt::CaseInsensitive);
    }

    //! A change of the class tags polishes the widget again, for the rules and elements that read them. At passthrough nothing reads
    //! them, and the pair would only make the base style do its work twice for every tag a widget is given when it is built.
    static void repolishAfterClassChange(QWidget* widget)
    {
        if (!GuiPerfFloor::isPassthrough())
        {
            widget->style()->unpolish(widget);
            widget->style()->polish(widget);
        }
    }

    void Style::addClass(QWidget* button, const QString& className)
    {
        // A tag the widget has already changes nothing: no second copy, no polish again. PushButton::polish and Slider::polish add
        // theirs on every polish, which grew the class text and unpolished the widget inside its own polish each time.
        if (hasClass(button, className))
        {
            return;
        }

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

        repolishAfterClassChange(button);
    }

    void Style::removeClass(QWidget* button, const QString& className)
    {
        QVariant buttonClassVariant = button->property("class");
        if (!buttonClassVariant.isNull())
        {
            const QString classText = buttonClassVariant.toString();
            QStringList classList = classText.split(classTagSeparator());
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
                repolishAfterClassChange(button);
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
    // The application palette of the flattened GUI: every application-wide colour, from the sheets' rules (never from the system
    // palette: the engine forces its theme). Base is the dark of the item views; the entry fields carry their light Base themselves
    // (applyFlattenedTextEntryPalette). No :disabled rule reaches these roles: a view keeps its Base when disabled, and the universal
    // rule's white reaches the disabled text, where only the classes with a :disabled colour differ (applyFlattenedDisabledText).
    //////////////////////////////////////////////////////////////////////////
    static QPalette buildO3DEBasePalette()
    {
        QPalette palette;
        const QColor window = Foundations::Mid;             // BaseStyleSheet.qss:65 $fill.surface.idle.Mid
        const QColor viewBase = Foundations::Mid;           // BaseStyleSheet.qss:29 (a view without a parent)
        const QColor alternateBase(0x4D, 0x4D, 0x4D);       // TableView.qss:21 $fill.view.idle.alternate-fill.Alt1
        const QColor textWhite = Foundations::Surface::Text;
        const QColor disabledText = Foundations::Field::DisabledText;     // the placeholder only: the entry fields' (see above)
        const QColor highlight(0x65, 0x65, 0x65);           // TableView.qss:23 $fill.view.idle.selection-fill.Alt1
        const QColor highlightedText(0xFF, 0xFF, 0xFF);     // TableView.qss:24
        const QColor toolTipBase(0x00, 0x00, 0x00);         // ToolTip.qss:13
        const QColor toolTipText(0xFF, 0xFF, 0xFF);         // ToolTip.qss:12
        const QColor link(0x94, 0xD2, 0xFF);                // the Text config's hyperlinkColor applies over it at the application's polish

        // The ladder Fusion shades frames and bevels from, on the tiers (no rule gave it: ledger)
        const QColor button = Foundations::Highest;
        const QColor light(0x66, 0x66, 0x66);
        const QColor midlight = Foundations::Highest;
        const QColor mid = Foundations::Low;
        const QColor dark = Foundations::BG;
        const QColor shadow = Foundations::FarBG;

        // Every group alike: Active and Inactive render identically, and no :disabled rule reaches these roles (see above)
        for (const auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
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

        return palette;
    }

    void Style::polish(QApplication* application)
    {
        // Passthrough: none of what follows. No palette of ours (the application keeps the one the base style gave it), no elements, no
        // class fonts, no tooltip filter. The application font is not set here: StyleManager set it before the style, and it stays, so
        // that the text metrics of the rungs stay comparable. The combo box animation is a setting of the application, not of a style.
        if (GuiPerfFloor::isPassthrough())
        {
            application->setEffectEnabled(Qt::UI_AnimateCombo, false);
            QProxyStyle::polish(application);
            return;
        }

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
            // The Global box elements of the small controls (TitleBar.qss, FilteredSearchWidget.qss): once, whatever the number of
            // setStyle calls
            ControlElements::registerGlobal();

            // No class palettes for QMenu and QMenuBar: one set through QApplication::setPalette goes through polish(QPalette&) and comes
            // out as the base palette (QApplication::setPalette), so they never took effect. The menu and menu bar elements carry the
            // colours of both (MenuElement.cpp).

            // A tooltip takes the palette of QToolTip, which Qt filled from the platform theme (qt_init_tooltip_palette), never the
            // application's: ToolTipBase and ToolTipText are ToolTip.qss:12-13
            QToolTip::setPalette(m_data->palette);

            // Class fonts: Qt rebuilt them from the platform in this setStyle, right before this polish
            TextElement::installClassFonts();

            // The Global box elements of the color picker and the color label (ColorPicker.qss, ColorLabel.qss): once, whatever the
            // number of setStyle calls
            ColorPickerElements::registerGlobal();

            // The Global box elements of the property editor (ReflectedPropertyEditor.qss, Card.qss): once, however often setStyle is called
            PropertyEditorElements::registerGlobal();

            // The Global box elements of the combo box (ComboBox.qss): once, whatever the number of setStyle calls
            ComboBox::registerFlatElements();
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
