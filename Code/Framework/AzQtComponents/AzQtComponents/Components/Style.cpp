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

    //////////////////////////////////////////////////////////////////////////
    // Flattened table header sections (GUI flattening)
    //
    // TableView.qss:62-76 gave the sections of a header inside a QTableView,
    // QListView or QTreeView: height 24px, background #2d2d2d, no border, 12px
    // normal text, padding-left 7px. No rule gave the label a colour (the '*'
    // rule's white reached it, BaseStyleSheet.qss:18) or the sort arrow a look
    // (the base style draws it). All 88 header widgets of the legacy captures
    // sit inside such a view.
    // TOKENIZE: HeaderSectionHeight, HeaderSectionPaddingLeft, HeaderSectionTextSize
    //////////////////////////////////////////////////////////////////////////
    static const int g_headerSectionHeight = 24;
    static const int g_headerSectionPaddingLeft = 7;
    static const int g_headerSectionTextSize = 12;

    //////////////////////////////////////////////////////////////////////////
    // Flattened item views (GUI flattening): the metrics of the base style
    //
    // The legacy sheet style laid an item's parts out through ITSELF (its own check box room, TableView.qss:78-85, CheckBox.qss:23-28),
    // but asked the BASE style for the item's size and for the rect of a part the item does not have (qstylesheetstyle.cpp:5557-5564,
    // 6254-6256): the base style's own check box room (Fusion's 14px) was in those. While this is set, an item option's
    // PM_IndicatorWidth and PM_IndicatorHeight are the base style's.
    //////////////////////////////////////////////////////////////////////////
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
    // FilteredSearchWidget.qss:9-13 gave each line edit inside a
    // FilteredSearchWidget a width of 260px, the contents width of its size
    // hint (the box of LineEdit.qss is added around it), and a frame (set at
    // the line edit's polish). LineEdit.qss:42-48 gave the action buttons of a
    // line edit (search, clear, error) a min and max size of 16px; the sheet's
    // box adds a tool button's native border to it, 1px a side
    // (qstylesheetstyle_default.cpp `QPushButton, QToolButton { border-style:
    // native }`, filled by qstylesheetstyle.cpp:1124-1160 with
    // PM_DefaultFrameWidth, qfusionstyle.cpp:2851, through :1721-1757): 18px.
    // TOKENIZE: SearchFieldWidth, LineEditActionButtonSize
    //////////////////////////////////////////////////////////////////////////
    static const int g_searchFieldContentsWidth = 260;
    static const int g_lineEditActionButtonSize = 16 + 2 * 1;

    //! The line edit the breadcrumbs show while their path is edited: the second page of the stack beside the label (BreadCrumbs.cpp:93-102)
    static bool isBreadCrumbsPathEdit(const QWidget* widget)
    {
        const QWidget* stack = widget->parentWidget();
        return widget->inherits("QLineEdit") && stack && stack->inherits("QStackedWidget")
            && stack->parentWidget() && stack->parentWidget()->inherits("AzQtComponents::BreadCrumbs");
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

        // BrowseEdit's line edit carries flagToIgnore (hasStyle == false); BrowseEdit.qss
        // QLineEdit { height: 16px; border-width: 0 }. TOKENIZE: BrowseEditLineEditHeight
        if (StyleManager::stylesheetsDisabled() && type == CT_LineEdit && widget && widget->parentWidget()
            && widget->parentWidget()->inherits("AzQtComponents::BrowseEdit"))
        {
            QSize lineEditSize = QProxyStyle::sizeFromContents(type, option, size, widget);
            lineEditSize.setHeight(16);
            return lineEditSize;
        }

        // The path edit of the breadcrumbs carries flagToIgnore too: LineEdit.qss:13 gave it height: 16px, BreadCrumbs.qss:26-31 no border,
        // and the padding of LineEdit.qss:19-20 (top -1px, bottom 1px) adds up to nothing
        if (StyleManager::stylesheetsDisabled() && type == CT_LineEdit && widget && isBreadCrumbsPathEdit(widget))
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
                case CT_ScrollBar:
                {
                    // ScrollBar.qss:10-25: 8px thick inside a 2px border. QScrollBar::sizeHint asks for twice
                    // PM_ScrollBarExtent plus PM_ScrollBarSliderMin along the bar, and the legacy style answered
                    // the extent with the rule's BOX (8 + 2 * 2px: qstylesheetstyle.cpp:5143-5150 over :558) before
                    // it set the thickness back to the declared 8px (CT_ScrollBar returns adjustSize, :5420-5422 over
                    // :560-571): 8 x 56 (horizontal 56 x 8), where Fusion's extent gives 8 x 48. Every scroll area's
                    // minimum size hint follows (QAbstractScrollArea::minimumSizeHint: 64 x 64 for a view in the
                    // Native dumps, 56 x 56 in the flattened ones before).
                    // TOKENIZE: ScrollBarBorder
                    const int border = 2;
                    QSize scrollBarSize = size;
                    if (auto sliderOption = qstyleoption_cast<const QStyleOptionSlider*>(option))
                    {
                        (sliderOption->orientation == Qt::Horizontal ? scrollBarSize.rwidth() : scrollBarSize.rheight()) += 2 * 2 * border;
                    }
                    return scrollBarSize;
                }
                case CT_ItemViewItem:
                {
                    // TableView.qss:31-40 `::item { height: 24px; padding-left: 4px }` (the item view element, qstylesheetstyle.cpp:5557-5564)
                    const ItemViewElement* items = ItemViewElement::forWidget(widget);
                    ++s_itemViewBaseIndicator;
                    QSize itemSize = QProxyStyle::sizeFromContents(type, option, size, widget);
                    --s_itemViewBaseIndicator;
                    if (items)
                    {
                        return items->itemSize(itemSize);
                    }
                    // qss-era item view row floor (24px) of the views no element covers (the list of a combo box popup)
                    itemSize.setHeight(qMax(itemSize.height(), 24));
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
                    // The element sizes a button as its rule did (qstylesheetstyle.cpp:5336-5350): the box around the contents
                    // within the declared size where the rule has a box or a border of its own, else the base style does
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
                    // The element boxes the indicator element as its rule did (qstylesheetstyle.cpp:5399-5414): the size of the label within
                    // the declared size, the indicator and the label spacing added, the box around it all
                    if (const BoxElement* box = BoxElement::forWidget(widget))
                    {
                        if (const IndicatorElement* indicator = IndicatorElement::forWidget(widget))
                        {
                            const int spacing = pixelMetric(type == CT_RadioButton ? PM_RadioButtonLabelSpacing : PM_CheckBoxLabelSpacing, option, widget);
                            return box->boxSize(option, widget, indicator->sizeFromContents(box->adjustSize(size), spacing));
                        }
                    }
                }
                break;
                case CT_ToolButton:
                {
                    // The element sizes a tool button as its rule did (qstylesheetstyle.cpp:5332-5350): the box around the contents
                    // within the declared size, and the 3 x 3 Qt adds for a rule with a box ("broken QToolButton"), where the rule has a
                    // box or a border of its own, else the component does. UNVERIFIED: Qt adds the 3 x 3 for a gradient or an image under
                    // the style's own border as well, and sizes it by QWindowsStyle (:5333-5349); no drafted element has one.
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
                        // A spin box line edit has the box of the spin box: the sheet returned its contents size
                        // as it was (qstylesheetstyle.cpp:5389-5393)
                        return size;
                    }

                    // The element sizes its field as its rule did (qstylesheetstyle.cpp:5389-5397): the box around the contents
                    // within the declared size where the rule has a box or a border of its own, else the base style does
                    if (const BoxElement* box = BoxElement::forWidget(widget))
                    {
                        const QSize contents = box->adjustSize(size);
                        return box->ownsBox(option) ? box->boxSize(option, widget, contents)
                                                    : QProxyStyle::sizeFromContents(type, option, contents, widget);
                    }

                    // LineEdit.qss: height 16 + 2px border -> 20; editable-combo line edits 16. The legacy box is
                    // 2px wider than Fusion's, less the padding-left -6px of a search field (LineEdit.qss:22-26).
                    // TOKENIZE: LineEditHeight, ComboBoxLineEditHeight
                    QSize lineEditSize = QProxyStyle::sizeFromContents(type, option, size, widget);
                    lineEditSize.setHeight((parent && parent->inherits("QComboBox")) ? 16 : 20);
                    lineEditSize.rwidth() += 2 + LineEdit::flattenedPaddingLeft(widget);

                    // FilteredSearchWidget.qss:12 `width: 260px` is the contents width: the box (border and
                    // padding) goes around it, for the minimum size hint as well (qstylesheetstyle.cpp:560-571, 5389-5399)
                    if (StyleHelpers::findParent<FilteredSearchWidget>(widget))
                    {
                        const QMargins box = LineEdit::flattenedFieldBox(widget);
                        lineEditSize.setWidth(g_searchFieldContentsWidth + box.left() + box.right());
                    }
                    return lineEditSize;
                }
                case CT_SpinBox:
                {
                    // SpinBox.qss: the sheet style started from the base style's size (qstylesheetstyle.cpp:5311-5312) and added the up
                    // button and the box of the rule around the contents (SpinBoxElement::extraWidth, qstylesheetstyle.cpp:5313-5329):
                    // 22 more than the flattened box asked for (n2 Gallery 23_spin-box: 80 where the hint was 58); height 16 + margin 2
                    // + padding -> 23 (legacy capture, page 23)
                    // TOKENIZE: SpinBoxHeight
                    QSize spinSize = QProxyStyle::sizeFromContents(type, option, size, widget);
                    spinSize.rwidth() += qMax(0, SpinBoxElement::extraWidth(widget));
                    spinSize.setHeight(23);
                    return spinSize;
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
                        return QSize(size.width(), MenuElement::forMenu(widget).separatorHeight());
                    }
                    break;
                }
                case CT_HeaderSection:
                {
                    // TableView.qss:62-76 QHeaderView::section { height: 24px; padding-left: 7px }: the rule's
                    // height, and the base style's width plus the padding (qstylesheetstyle.cpp:5352-5382;
                    // legacy capture: 18 wide for a one-digit row number, a column sized to its text 7 wider
                    // than the base). The base is the TableView size of a horizontal header, Qt's for a vertical one.
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
                // The element frames its widget as its rule did (qstylesheetstyle.cpp:4502-4512): a border of its own is drawn
                // with the fill (PE_Widget), the style's own by the base style inside the margin, over the palette of the fill
                if (StyleManager::stylesheetsDisabled())
                {
                    if (const BoxElement* box = BoxElement::forWidget(widget))
                    {
                        if (auto frameOption = qstyleoption_cast<const QStyleOptionFrame*>(option); frameOption && box->hasNativeBorder(option))
                        {
                            QStyleOptionFrame inset(*frameOption);
                            inset.rect = box->borderRect(option);
                            BoxElement::configurePalette(inset.palette, box->face(option));
                            QProxyStyle::drawControl(element, &inset, painter, widget);
                        }
                        return;
                    }
                }

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
                if (StyleManager::stylesheetsDisabled() && widget && hasClass(widget, QStringLiteral("HorizontalSeparator")))
                {
                    // ColorPicker.qss:83-90 `AzQtComponents--ColorPicker .HorizontalSeparator` (an exact QFrame HLine,
                    // ColorPicker.cpp:1331-1335): background #222222, 1px #393a3c top and bottom borders, none left or
                    // right. First, ahead of the exact-QFrame rule: a class tag replaces the class name in Qt's selector
                    // match (qstylesheetstyle.cpp:1545-1551), so Native never gave this frame BaseStyleSheet's .QFrame
                    // fill. The borders are antialiased fills, as qDrawEdge draws them (qcssutil.cpp:165-169 under
                    // qstylesheetstyle.cpp:1282-1285): at a fractional device ratio the edge rows blend.
                    // TOKENIZE: PickerSeparatorFill, PickerSeparatorLine
                    const QRect r = option->rect;
                    painter->fillRect(r, QColor(0x22, 0x22, 0x22));
                    painter->save();
                    painter->setRenderHint(QPainter::Antialiasing);
                    painter->setPen(Qt::NoPen);
                    painter->setBrush(QColor(0x39, 0x3A, 0x3C));
                    painter->drawRect(QRectF(r.left(), r.top(), r.width(), 1));
                    painter->drawRect(QRectF(r.left(), r.bottom(), r.width(), 1));
                    painter->restore();
                    return;
                }
                // The frame element: what the legacy sheets gave a QFrame's line, box or panel and the frame of a scroll
                // area, drawn by the base style from the palette their background derived (FrameElement.h). Behind the
                // picker's separator above: that frame has borders, which the sheet drew itself.
                if (StyleManager::stylesheetsDisabled() && widget)
                {
                    const auto frameOption = qstyleoption_cast<const QStyleOptionFrame*>(option);
                    const FrameElement* frameElement =
                        (frameOption && frameOption->frameShape != QFrame::NoFrame) ? FrameElement::forFrame(widget) : nullptr;
                    if (frameElement)
                    {
                        QStyleOptionFrame framed(*frameOption);
                        frameElement->apply(&framed, painter, widget);
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
                // Splitter.qss:9-11, :21-24: the handle's rect (the contents of QSplitterHandle, the 1px between the
                // panes: PM_SplitterWidth) is filled #222222, #1E70EB under the pointer, as a rule fills its background
                // (qstylesheetstyle.cpp:1348-1370); QMainWindow::separator (dock gaps): #111111 across the whole gap.
                // The 26 handles of n2 (Asset Processor, Editor, Gallery, the Atom tools) are all at rest: the colour under the pointer is
                // carried from the rule, UNVERIFIED.
                // TOKENIZE: SplitterLineColor, SplitterHoverColor, DockSeparatorColor
                if (StyleManager::stylesheetsDisabled())
                {
                    if (qobject_cast<const QMainWindow*>(widget))
                    {
                        painter->fillRect(option->rect, QColor(0x11, 0x11, 0x11));
                        return;
                    }
                    painter->fillRect(option->rect, option->state.testFlag(QStyle::State_MouseOver) ? QColor(0x1E, 0x70, 0xEB) : QColor(0x22, 0x22, 0x22));
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
                        // As the legacy sheet style drew it (qstylesheetstyle.cpp:4006-4016): an item with a fill (hovered or pressed)
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
                // qstylesheetstyle.cpp:3980-3989). Only a QMenu's own: a combo box in menu mode keeps the base style's.
                if (StyleManager::stylesheetsDisabled() && qobject_cast<const QMenu*>(widget))
                {
                    if (auto menuItem = qstyleoption_cast<const QStyleOptionMenuItem*>(option))
                    {
                        if (MenuElement::forMenu(widget).drawItem(painter, *menuItem, widget))
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
                        // The rule's font-weight over the painter's font, put back after the label (qstylesheetstyle.cpp:4434-4445)
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
                // QDockWidget.qss:20-23 backup path: title strip #333333 (raw QDockWidgets in gems). The legacy style
                // drew that fill and then the title text itself (qstylesheetstyle.cpp:4459-4501): in the base style's
                // title text rect (the title area less the buttons, 4px in from the left: qfusionstyle.cpp:3473-3487,
                // qcommonstyle.cpp:3022-3075), left aligned from the top, elided to the rect, in the dock widget's
                // WindowText. BaseStyleSheet.qss:60-66 gave QDockWidget #cccccc in every state. The palette of the dock
                // stays white, as one set on it would reach the widgets inside (a rule of their own held them to white),
                // so the colour is carried here. Horizontal title bars: no dock widget of the engine has a vertical one.
                // TOKENIZE: DockTitleFillColor, DockTitleTextColor
                if (StyleManager::stylesheetsDisabled())
                {
                    painter->fillRect(option->rect, QColor(0x33, 0x33, 0x33));
                    auto dockOption = qstyleoption_cast<const QStyleOptionDockWidget*>(option);
                    if (dockOption && !dockOption->title.isEmpty())
                    {
                        const QRect textRect = QProxyStyle::subElementRect(SE_DockWidgetTitleBarText, option, widget);
                        QPalette titlePalette = dockOption->palette;
                        titlePalette.setColor(QPalette::WindowText, QColor(0xCC, 0xCC, 0xCC));
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
                // A button with an element is drawn as Native drew it (qstylesheetstyle.cpp:3642-3650, qcommonstyle.cpp:1294-1307):
                // the bevel, then the label in the contents rect of the box, in the colour of the state. Fusion lays the label out
                // in the rect it asks of itself (qfusionstyle.cpp:1598-1605), which the box would not reach.
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
                // the text 4px apart where the base style's are 2px (qstylesheetstyle.cpp:3688-3783, qcommonstyle.cpp:1362-1377)
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
                // The element's bevel (qstylesheetstyle.cpp:3651-3686)
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
                            // box (qstylesheetstyle.cpp:2218-2233, 2272-2300, 3674-3686)
                            const int extent = pixelMetric(PM_MenuButtonIndicator, nullptr, widget);
                            QStyleOptionButton arrowOption(*buttonOption);
                            arrowOption.rect = QStyle::alignedRect(option->direction, Qt::AlignRight | Qt::AlignBottom, QSize(extent, extent),
                                box->contentsRect(option, widget).marginsAdded(box->m_padding));
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
                // The box of the element around the indicator element of a check box (qstylesheetstyle.cpp:3789-3794)
                if (StyleManager::stylesheetsDisabled())
                {
                    auto buttonOption = qstyleoption_cast<const QStyleOptionButton*>(option);
                    if (const BoxElement* box = BoxElement::forWidget(widget); box && buttonOption && box->drawIndicatorButton(this, buttonOption, painter, widget))
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
                // The box of the element around the indicator element of a radio button (qstylesheetstyle.cpp:3789-3794)
                if (StyleManager::stylesheetsDisabled())
                {
                    auto buttonOption = qstyleoption_cast<const QStyleOptionButton*>(option);
                    if (const BoxElement* box = BoxElement::forWidget(widget); box && buttonOption && box->drawIndicatorButton(this, buttonOption, painter, widget))
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
                // Flatten: hard-fill header sections (TableView.qss #2d2d2d) - the
                // palette-only route was unreliable because Fusion blends Button
                if (StyleManager::stylesheetsDisabled() && widget && widget->inherits("QTableCornerButton"))
                {
                    // Legacy render: the QTableView corner button is solid black (user verdict,
                    // 2026-09-24 pair 19). TOKENIZE: TableCornerButtonColor
                    painter->fillRect(option->rect, QColor(Qt::black));
                    return;
                }
                if (StyleManager::stylesheetsDisabled() && hasFlattenedHeaderSection(widget))
                {
                    // The legacy draw was QCommonStyle's CE_Header over the section rule (qcommonstyle.cpp:2158-2178):
                    // clipped to the section, the rule's fill (TableView.qss:69), the label in the rule's content box
                    // (padding only, no border: qstylesheetstyle.cpp:6264-6270), then the base style's sort arrow.
                    // The fill of the section is the item view element's (TableView.qss:69 #2d2d2d, the sheet of a module's own colour); a section
                    // declared transparent (a local sheet, or the property "SectionBackground" = "transparent") paints nothing and shows the
                    // background behind the header.
                    painter->save();
                    painter->setClipRect(option->rect);
                    if (const QColor sectionFill = ItemViewElement::headerFillOf(widget); sectionFill.alpha() > 0)
                    {
                        painter->fillRect(option->rect, sectionFill);
                    }
                    if (auto headerOption = qstyleoption_cast<const QStyleOptionHeader*>(option))
                    {
                        // font-size: 12px, font-style: normal (TableView.qss:72-73) over the painter's font, as
                        // qstylesheetstyle.cpp:4142-4145 does; family and weight stay the viewport's
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
                // The legacy header had a background (the '*' rule, BaseStyleSheet.qss:15-21: transparent), and the sheet style then drew nothing
                // for the area beyond the last section (qstylesheetstyle.cpp:4152-4155): what is behind the header shows
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
                // The popup list of a combo box paints each item from its element, not from the palette Qt hands
                // the view (the combo box replaces that, qcombobox.cpp:1430-1448): fill, text color, no outline
                if (StyleManager::stylesheetsDisabled() && ComboPopupElement::coversView(widget))
                {
                    if (auto popupOption = qstyleoption_cast<const QStyleOptionViewItem*>(option))
                    {
                        QStyleOptionViewItem popupItem = *popupOption;
                        ComboPopupElement::global().prepareItem(popupItem, painter);
                        return QProxyStyle::drawControl(element, &popupItem, painter, widget);
                    }
                }

                // The legacy sheet drew an item through the common style, with the palette of the item's rule (qstylesheetstyle.cpp:4363-4373):
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

                // TableView.qss:99-102 `QTreeView::item:disabled { color: #777777 }` (118 tree views, an AzQtComponents::TableView
                // among them): the legacy style laid the item rule's palette over the option of a disabled item, or of an item
                // in a disabled view, before it called the base style (qstylesheetstyle.cpp:4363-4373): the colour reaches
                // Text, or HighlightedText for a selected item. The view's own roles stay white, as in the dumps. A
                // BranchDelegate tree with branch lines (the Gallery's) takes the branch above.
                // UNVERIFIED: no capture holds a disabled item with text (the two disabled trees of LuaIDE are empty).
                // TOKENIZE: DisabledItemTextColor
                if (StyleManager::stylesheetsDisabled() && treeView && itemOption && !itemOption->state.testFlag(QStyle::State_Enabled))
                {
                    QStyleOptionViewItem disabledItem = *itemOption;
                    disabledItem.palette.setColor(QPalette::Text, QColor(0x77, 0x77, 0x77));
                    disabledItem.palette.setColor(QPalette::HighlightedText, QColor(0x77, 0x77, 0x77));
                    return QProxyStyle::drawControl(element, &disabledItem, painter, widget);
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
            // CheckBox.qss:130-132, :137-139 gave a disabled expander the faded caret; in a card header Card.qss:277-287 (`AzQtComponents--
            // CardHeader QCheckBox.Expander::indicator:unchecked:disabled`, one more type selector) takes it back to the full one, "at
            // full opacity even when card is disabled"
            if (!enabled && !TextElement::withinClass(widget, "AzQtComponents::CardHeader"))
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

    //////////////////////////////////////////////////////////////////////////
    // Flattened plain dock widgets (GUI flattening)
    //
    // QDockWidget.qss:14-31 is a backup style for the QDockWidget class itself,
    // "not derived classes" (:9-12): `.QDockWidget` is the class selector, which
    // matches the exact class name of a widget that has no class tag
    // (qstylesheetstyle.cpp:1545-1551). It gives the title strip (:20-23, drawn by
    // CE_DockWidgetTitle), the title icons (:16-17, Style::standardIcon) and
    // frameless transparent title buttons (:26-31, PE_PanelButtonTool). A title
    // button is a QDockWidgetTitleButton, which the legacy style matched as its
    // dock widget (qstylesheetstyle.cpp:1806-1820).
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
                            if (tabElement->m_barFill.isValid())
                            {
                                painter->fillRect(option->rect, tabElement->m_barFill);
                            }
                            TabElement::drawEdges(painter, option->rect, tabElement->m_barEdges);
                            return;
                        }
                    }
                    else if (widget->inherits("QTabWidget"))
                    {
                        if (const TabElement* tabElement = TabElement::forTabWidget(widget))
                        {
                            const QRect box = option->rect.marginsRemoved(tabElement->m_widgetMargins);
                            if (tabElement->m_widgetFill.isValid())
                            {
                                painter->fillRect(box, tabElement->m_widgetFill);
                            }
                            TabElement::drawEdges(painter, box, tabElement->m_widgetEdges);
                            return;
                        }
                    }
                    else if (MenuElement::drawHostedWidget(painter, option, widget))
                    {
                        return;
                    }

                    // The element of any other widget (qstylesheetstyle.cpp:4681-4725): its fill, image and border
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
                // The border of the element over what a picker frame has painted (qstylesheetstyle.cpp:4639-4650)
                if (StyleManager::stylesheetsDisabled() && ColorPickerElements::drawFrame(option, painter, widget))
                {
                    return;
                }
            }
            break;

            case PE_PanelLineEdit:
            {
                // The element frames its field as its rule did (qstylesheetstyle.cpp:4651-4679): a border of its own is drawn with
                // the fill; the style's own by the base style inside the margin, over the palette of the fill. A line edit inside a
                // combo box or a spin box has no panel of its own where the container draws its border or a fill the base style cannot
                // (:4655-4661). The State_Sunken of a field says its frame is sunken, not that it is pressed (:2121-2123).
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
                // background in every state, where Fusion raises a panel under the mouse. UNVERIFIED: no capture
                // holds the pointer over one.
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
                // Flatten FIRST: the BranchDelegate path below delegates to raw Fusion
                // baseStyle() (massive native arrows); in flatten mode the caret SVGs
                // own this for every tree. TableView hides branches (qss).
                if (StyleManager::stylesheetsDisabled())
                {
                    // The item view element draws the branch cell as its rule did: a fill by state, and the caret fitted into the cell
                    // less the rule's padding (TableView.qss:42-60, 87-97); a TableView's element has no caret (:104-108)
                    auto branchOption = qstyleoption_cast<const QStyleOptionViewItem*>(option);
                    if (const ItemViewElement* items = branchOption ? ItemViewElement::forWidget(widget) : nullptr)
                    {
                        if (!items->drawBranch(this, branchOption, painter, widget))
                        {
                            // The rule has nothing for this cell: the sheet style asked the base style, which draws the connecting lines of a tree
                            // with a BranchDelegate (qstylesheetstyle.cpp:4783-4785) and nothing for any other tree
                            TreeView::drawBranchIndicator(this, option, painter, widget, m_data->treeViewConfig);
                        }
                        return;
                    }
                }
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
                    // Item view check boxes share the CheckBox.qss indicator images, drawn in the rect their rule's margin leaves
                    // (TableView.qss:78-85, CheckBox.qss:23-28: a tree pulls the box 3px to the left, qstylesheetstyle.cpp:1349-1353)
                    const ItemViewElement* items = ItemViewElement::forWidget(widget);
                    drawFlattenedIndicator(flattenedCheckBoxIndicatorPath(option, widget), painter,
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
                    drawFlattenedIndicator(QStringLiteral(":/Application/titlebar-close.svg"), painter, glyphRect);
                    return;
                }
            }
            break;

            case PE_FrameDockWidget:
            {
                // StyledDockWidget.qss:9-18: the border of the rule a StyledDockWidget was polished with, none for one polished docked
                // (ControlElements.cpp). Another dock widget with a title bar of its own keeps a black line while it floats (UNVERIFIED: no
                // capture holds one).
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
                if (StyleManager::stylesheetsDisabled())
                {
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
                    painter->fillRect(option->rect, MenuElement::forMenu(widget).m_surface);
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

            case PE_FrameTabWidget:
            {
                // The tab element's pane fill and edges, on the pane rect (SE_TabWidgetTabPane)
                if (StyleManager::stylesheetsDisabled())
                {
                    if (const TabElement* tabElement = TabElement::forTabWidget(widget))
                    {
                        if (tabElement->m_paneFill.isValid())
                        {
                            painter->fillRect(option->rect, tabElement->m_paneFill);
                        }
                        TabElement::drawEdges(painter, option->rect, tabElement->m_paneEdges);
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
                // (PushButton.qss:48-52) left the sheet style nothing to draw over it, a pressed or checked one included, where Fusion draws
                // the face of a button that is down (qcommonstyle.cpp:1316-1322, qfusionstyle.cpp:713-724). The menu indicator of a flat button is the bevel's, not the panel's.
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
                    // The rect is the radio button element's image rect (IndicatorElement::imageRect), which is
                    // the 18px of the focus variant (RadioButton.qss:31-32) while the button has the focus
                    drawFlattenedIndicator(flattenedRadioButtonIndicatorPath(option), painter, option->rect);
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
                    // The panel of a row as the sheet drew it (qstylesheetstyle.cpp:4852-4884): the base style's (the alternate colour, the
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
                // (qstylesheetstyle.cpp:4881-4883, :4912-4916, QRenderRule::drawRule)
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
                // The element draws a tool button whose border is its own as its rule did (qstylesheetstyle.cpp:3308-3354)
                if (StyleManager::stylesheetsDisabled())
                {
                    auto toolOption = qstyleoption_cast<const QStyleOptionToolButton*>(option);
                    if (const BoxElement* box = BoxElement::forWidget(widget); box && toolOption && box->drawToolButton(this, toolOption, painter, widget))
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
                    // The element boxes a combo box as its rule did (qstylesheetstyle.cpp:5799-5824): the frame inside the margin, the
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

                        if (StyleManager::stylesheetsDisabled() && (qobject_cast<const QSpinBox*>(widget) || qobject_cast<const QDoubleSpinBox*>(widget)))
                        {
                            // Flatten: one box for plain and vector spin boxes (SpinBox.qss +
                            // VectorInput.qss resolved inside SpinBox::editFieldRect), for every QSpinBox and QDoubleSpinBox: the rules
                            // named the Qt classes (SpinBox.qss:9-10), so the spin boxes of a form that are not ours (the Gallery's
                            // svg label page) have the box as well. Fusion put their line edit at (3, 3) in a 343 x 17 field, where the
                            // sheet's is 341 x 19 at (2, 1).
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
                // The element boxes a tool button as its rule did (qstylesheetstyle.cpp:5961-5982): the button inside the margin, the
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
                // style's rect to the label, qcommonstyle.cpp:3438-3470), so their glyphs were never drawn.
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
        if (TextElement::named(widget, "title") && TextElement::withinClass(widget, "AzQtComponents::TitleBar"))
        {
            // TitleBar.qss:31-37 `AzQtComponents--TitleBar #title { margin-left: 10px; margin-right: 16px }` and
            // Text.qss:93-94 QLabel { margin-top: 1px; margin-bottom: 1px }. The label sits in a container of the
            // bar, not directly in it (Titlebar.cpp:150, 161-162): the selector matches at any depth.
            return option->rect.adjusted(10, 1, -16, -1);
        }

        // Text.qss tier margins; a tag is a whole word compared with case, as Qt's [class~=] (qcssparser.cpp:2168-2180)
        int margin = 1; // .Label, QLabel
        if (TextElement::tagged(widget, "Headline") || TextElement::tagged(widget, "Paragraph"))
        {
            margin = 4;
        }
        else if (TextElement::tagged(widget, "Title"))
        {
            margin = 7;
        }
        else if (TextElement::tagged(widget, "Subtitle"))
        {
            margin = 8;
        }
        else if (TextElement::tagged(widget, "Menu") || TextElement::tagged(widget, "Button"))
        {
            margin = 6;
        }
        else if (TextElement::tagged(widget, "Tooltip"))
        {
            margin = 2;
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
                    // The element is the whole definition of its frame (qstylesheetstyle.cpp:6176-6182): its contents rect where
                    // the rule has a box or a border of its own, else Qt's. None of the emulations below is the element's.
                    if (const BoxElement* box = BoxElement::forWidget(widget))
                    {
                        return box->ownsBox(option) ? visualRect(option->direction, option->rect, box->contentsRect(option, widget))
                                                    : QProxyStyle::subElementRect(element, option, widget);
                    }

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
                    if (r.isNull() && widget && widget->inherits("AzQtComponents::WindowDecorationWrapper"))
                    {
                        // WindowDecorationWrapper.qss:9-11 `border: 1px solid black`: the box of the window frame, whose
                        // line WindowDecorationWrapper::paintEvent draws. TOKENIZE: WindowFrameWidth
                        r = option->rect.adjusted(1, 1, -1, -1);
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
                // The indicator element in the box of the element (qstylesheetstyle.cpp:6184-6207): the indicator at the left of the
                // contents, centered vertically, and the label after it and the label spacing
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
                // Where a click toggles a check box: its box and its label's text, laid out as the legacy sheet's
                // indicator and contents were (qstylesheetstyle.cpp:6222-6224 leaves it to qcommonstyle.cpp:2660-2663), inside the
                // box of its element
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
                // A radio button whose indicator the sheet boxes (RadioButton.qss:24-33) is clicked anywhere in its
                // rectangle, with a label or without: qstylesheetstyle.cpp:6215-6220 returns the widget's rect
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
                // text, which the base style lays out as it does the whole rect (qstylesheetstyle.cpp:6227-6256). Only a part the item has
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
                // The label box and the focus rect of a button whose box the element owns (qstylesheetstyle.cpp:6155-6173)
                const BoxElement* box = StyleManager::stylesheetsDisabled() ? BoxElement::forWidget(widget) : nullptr;
                if (box && box->ownsBox(option))
                {
                    return visualRect(option->direction, option->rect, box->contentsRect(option, widget));
                }
            }
            break;
            case SE_LineEditContents:
            {
                // The element is the whole definition of its field (qstylesheetstyle.cpp:6176-6182): its contents rect where the rule
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
                    // (qstylesheetstyle.cpp:5006-5011)
                    if (const BoxElement* box = BoxElement::forWidget(widget); box && qobject_cast<const QToolButton*>(widget) && box->ownsBox(option))
                    {
                        return 0;
                    }
                    break;
                case PM_CheckBoxLabelSpacing:
                case PM_RadioButtonLabelSpacing:
                    // The `spacing` of the rule of the state (qstylesheetstyle.cpp:5125-5139): of a check box and of a radio button; a group
                    // asks the check box metric too and reads its title's spacing (:5136-5139), which no element carries
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
                    // TableView.qss:78-85; qstylesheetstyle.cpp:5041-5050)
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
                    return (metric == PM_IndicatorWidth && widget && hasClass(widget, QStringLiteral("ToggleSwitch"))) ? 32 : 16;
                case PM_ExclusiveIndicatorWidth:
                case PM_ExclusiveIndicatorHeight:
                    return 16;
                case PM_MenuHMargin:
                    // Menu.qss:13 QMenu padding: 4px 2px (the left padding), in the menu element
                    return MenuElement::forMenu(widget).m_paddingH;
                case PM_MenuVMargin:
                    // Menu.qss:13 (the top padding)
                    return MenuElement::forMenu(widget).m_paddingV;
                case PM_ScrollBarExtent:
                    // ScrollBar.qss QScrollBar:vertical width / :horizontal height
                    return 8;
                case PM_ScrollBarSliderMin:
                    // ScrollBar.qss handle min-height/min-width
                    return 32;
                case PM_SplitterWidth:
                    // QSplitter asks with no option, and Qt's sheet style then reads the handle rule in the state of a
                    // vertical handle (qstylesheetstyle.cpp:5110-5121, 1783-1786): Splitter.qss:17-19 `height: 1px`. The
                    // panes are 1px apart; QSplitterHandle widens its widget to 5px with contents margins of 2, grabbing
                    // the panes at its sides (qsplitter.cpp:204-229), and paints the 1px between them (CE_Splitter).
                    // TOKENIZE: SplitterHandleWidth
                    return 1;
                case PM_ToolBarSeparatorExtent:
                    // ToolBar.qss QToolBar::separator { width: 1px }  TOKENIZE: ToolBarSeparatorWidth
                    return 1;
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
                    // StyledDockWidget.qss:9-18: what a floating dock without a native frame reserves around its contents is the border of the rule
                    // it was polished with (0 for a dock polished docked: the Gallery's floating docks, Native's no inset)
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

        // Text.qss colour tags, whole words compared with case (qcssparser.cpp:2168-2180)
        QColor textColor;
        if (TextElement::tagged(widget, "secondaryText"))
        {
            textColor = QColor(0x88, 0x88, 0x88);
        }
        else if (TextElement::tagged(widget, "highlightedText"))
        {
            textColor = QColor(0x44, 0xB2, 0xF8);
        }
        else if (TextElement::tagged(widget, "blackText"))
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
        // The :disabled field of the entry classes, the only owner of a disabled Base: LineEdit.qss:28-35
        // (QLineEdit, QTextEdit, QPlainTextEdit), SpinBox.qss:69-74, ComboBox.qss:53-57.
        // TOKENIZE: TextEntryDisabledFieldColor
        entryPalette.setColor(QPalette::Disabled, QPalette::Base, QColor(0x66, 0x66, 0x66));
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

    //////////////////////////////////////////////////////////////////////////
    // Flattened disabled text (GUI flattening)
    //
    // BaseStyleSheet.qss:15-21 gave every widget `color: white`, and only a
    // :disabled rule of a widget's own class takes it back: QStyleSheetStyle
    // configures the Active, Disabled and Inactive groups from the rules that
    // match each state (qstylesheetstyle.cpp:2682-2712), and a colour sets
    // WindowText, Text and ButtonText together (QRenderRule::configurePalette,
    // :1455-1458). The Native dumps agree (n2, 17346 widgets): disabled
    // WindowText / Text / ButtonText are white except on the classes below, the
    // menus (Menu.qss:23-26, ComboBox.qss:108; the menu palette), and the labels
    // and fields of the app sheets. The application palette keeps the white
    // (buildO3DEBasePalette); each class with a :disabled colour is carried here,
    // last in Style::polish so that the class tags a component's polish adds are
    // known (a default push button is tagged Primary by PushButton::polish). The
    // items of a tree view are the exception: their colour belongs to the item rule,
    // not to the widget (the dumps hold white), and is carried at CE_ItemViewItem.
    // TOKENIZE: DisabledTextColor, DisabledPrimaryTextColor
    //////////////////////////////////////////////////////////////////////////
    static void setFlattenedDisabledColor(QWidget* widget, const QColor& color, std::initializer_list<QPalette::ColorRole> roles)
    {
        QPalette palette = widget->palette();
        bool changed = false;
        for (const QPalette::ColorRole role : roles)
        {
            if (palette.color(QPalette::Disabled, role) != color)
            {
                palette.setColor(QPalette::Disabled, role, color);
                changed = true;
            }
        }
        if (changed)
        {
            widget->setPalette(palette);
        }
    }

    static void applyFlattenedDisabledText(QWidget* widget)
    {
        const QColor disabledGrey(0x99, 0x99, 0x99);
        if (auto pushButton = qobject_cast<QPushButton*>(widget))
        {
            // PushButton.qss:43 QPushButton:disabled { color: #999999 } (510 push buttons in the dumps); :60
            // QPushButton.Primary:disabled and QPushButton:default:disabled { color: #BBBBBB } (14), the later rule
            const bool primary = pushButton->isDefault() || TextElement::tagged(widget, "Primary");
            setFlattenedDisabledColor(widget, primary ? QColor(0xBB, 0xBB, 0xBB) : disabledGrey,
                { QPalette::WindowText, QPalette::Text, QPalette::ButtonText });
        }
        else if (widget->inherits("QCheckBox") || widget->inherits("QRadioButton"))
        {
            // CheckBox.qss:18 QCheckBox:disabled, RadioButton.qss:17 QRadioButton:disabled { color: #999999 }
            setFlattenedDisabledColor(widget, disabledGrey, { QPalette::WindowText, QPalette::Text, QPalette::ButtonText });
        }
        else if (widget->inherits("QLineEdit") || widget->inherits("QTextEdit") || widget->inherits("QPlainTextEdit")
            || widget->inherits("QSpinBox") || widget->inherits("QDoubleSpinBox") || widget->inherits("QComboBox"))
        {
            // LineEdit.qss:28-35 (QLineEdit, QTextEdit, QPlainTextEdit), SpinBox.qss:69-74 (QSpinBox, QDoubleSpinBox),
            // ComboBox.qss:53-57 `color: #999999`: nothing names the other QAbstractSpinBox classes. The field (and
            // Disabled Text) of these classes is applyFlattenedTextEntryPalette's; the line edits inside a BrowseEdit or
            // a spin box are left to their own polish, and take the colour here.
            setFlattenedDisabledColor(widget, disabledGrey, { QPalette::WindowText, QPalette::Text, QPalette::ButtonText });
        }
        else if (widget->inherits("QLabel") && TextElement::withinClass(widget, "AzQtComponents::Card"))
        {
            // Card.qss:272 `AzQtComponents--Card QLabel:disabled { color: #999999 }` (474 labels in the dumps), more
            // specific than a colour tag such as `.secondaryText` (Text.qss:16-19)
            setFlattenedDisabledColor(widget, disabledGrey, { QPalette::WindowText, QPalette::Text, QPalette::ButtonText });
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Flattened parented item views (GUI flattening)
    //
    // BaseStyleSheet.qss:23-30 gave a QTableView, QListView or QTreeView #444444
    // and :32-38 reset it to transparent for a view whose parent is a widget
    // (`QWidget > QTreeView`: the child combinator follows the QObject parent,
    // qstylesheetstyle.cpp:1496-1505, 1608-1609). Only a view without a parent,
    // a completer popup, kept the fill (the resolver: the transparent rule wins on
    // 111 of the 118 tree views captured). A transparent rule leaves the viewport's
    // background role (Base) a null brush, which the dumps print as black, and Window
    // transparent (QRenderRule::configurePalette on the embedded viewport,
    // qstylesheetstyle.cpp:1434-1449), and takes the viewport's own fill away
    // (:2952-2959). The application palette holds the opaque value, so a parented
    // view gets the rule's here.
    //
    // A view whose Base is not the application's was given its fill by code, or by
    // a more specific rule carried as an element, and keeps it: the Add Component
    // tree (Editor.qss:38-41; ComponentPaletteWidget.cpp:88-107) and the popup list of a combo
    // box (the menu palette). A QHeaderView is a QAbstractItemView, but not a
    // class the rule names; its palette is carried with it (Style::polish).
    // TOKENIZE: ParentedItemViewFill
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
            palette.setColor(group, QPalette::Window, Qt::transparent);
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
            TextElement::apply(widget);

            // Frames: the frame element of a QFrame, a scroll area or a module's frame, resolved once here (adding or
            // removing a class tag re-polishes, so it is resolved again)
            FrameElement::resolve(widget);

            applyFlattenedTextEntryPalette(widget);

            // The popup of a combo box: its container and the list view inside each take the element's palette
            // (ComboBox.qss:79-117). A combo box hands its own palette to its container whenever that changes
            // (qcombobox.cpp:1430-1448, 3295-3297) and so replaces the container's palette whole, but a role
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

            // TableView.qss: QTableView/QListView/QTreeView { border: none } and
            // QTableView { qproperty-showGrid: false }. TOKENIZE: ItemViewFrame, TableViewGrid
            if (auto itemView = qobject_cast<QAbstractItemView*>(widget))
            {
                // The rule names those three classes, and ColorPicker.qss:97 `AzQtComponents--ColorPicker AzQtComponents--PaletteView
                // { border: none }` the palette of the color picker (it is always inside one: contents margins 0 in the Native dumps).
                // A view of another kind (the thumbnails of the asset folder: a QAbstractItemView of their own) kept the frame Qt's
                // default sheet gave every QFrame, a native border 1px wide (qstylesheetstyle_default.cpp:171-184), which insets its
                // viewport by 1px (contents margins 1 in the Native dumps); a header has none of its own (qheaderview.cpp:332).
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

            // TabWidget.qss: the stacked page area of a tab widget sits on #444444
            if (widget->inherits("QStackedWidget") && widget->parentWidget()
                && widget->parentWidget()->inherits("AzQtComponents::TabWidget"))
            {
                QPalette pageAreaPalette = widget->palette();
                pageAreaPalette.setColor(QPalette::Window, QColor(0x44, 0x44, 0x44));
                widget->setPalette(pageAreaPalette);
                widget->setAutoFillBackground(true);
            }

            // Tab elements: a tab bar resolves its element here, once (adding or removing a class tag
            // re-polishes; a module registering an element later has TabElement polish the bars again),
            // and the bar and its tab widget get what the element sets on them.
            TabElement::polish(widget);

            // SegmentControl.qss:14-77: what the sheet set on the buttons of a segment bar when it polished them (ControlElements.cpp)
            ControlElements::polishSegmentBar(widget);

            // MenuBar.qss:11-12 QMenuBar { min-height: 28px } inside its 2px border: the bar is not lower than that, whatever
            // its items need (the sheet style set it as the minimum, qstylesheetstyle.cpp:2570-2603)
            if (widget->inherits("QMenuBar"))
            {
                widget->setMinimumHeight(MenuBarElement::global().minHeight());
            }

            // Menu.qss:18 `QMenu::item .WidgetAction:hover`: a widget a menu hosts is filled while it is hovered
            // (MenuElement::drawHostedWidget, at PE_Widget). The sheet style gave such a widget a styled background
            // and hover (qstylesheetstyle.cpp:2894-2905, 2925-2950). UNVERIFIED: no capture shows one hovered.
            if (TextElement::tagged(widget, "WidgetAction") && TextElement::withinClass(widget, "QMenu"))
            {
                widget->setAttribute(Qt::WA_StyledBackground, true);
                widget->setAttribute(Qt::WA_Hover, true);
            }

            // Headers: TableView.qss flat #2d2d2d sections (Fusion draws from Button)
            if (widget->inherits("QHeaderView"))
            {
                QPalette headerPalette = widget->palette();
                // The colour of the section rule: the view's element has it (a module's header is its own colour), and fills the
                // area of the header beyond its sections too (ItemViewElement::apply does the same when the view resolves)
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
                // The legacy header and its viewport were transparent (the '*' rule, BaseStyleSheet.qss:15-21): the sections paint their own
                // fill, and what lies beyond the last one is what is behind the header
                if (auto header = qobject_cast<QHeaderView*>(widget))
                {
                    header->viewport()->setAutoFillBackground(false);
                }
            }

            // Text.qss class-driven label box and text colors
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

        if (StyleManager::stylesheetsDisabled())
        {
            // Last: the class tags the component polish above adds (Primary) are known now
            applyFlattenedDisabledText(widget);
        }

        QProxyStyle::polish(widget);

        // The elements go last, over what the base style and the components set (the sheet applied its geometry after:
        // qstylesheetstyle.cpp:2875-2890), and the text colour of a module's rule over the Global palettes above
        if (StyleManager::stylesheetsDisabled())
        {
            BoxElement::apply(widget);
            ItemViewElement::apply(widget);
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
                // sheet style when the form was reparented, which polished its widgets again (QWidgetPrivate::inheritStyle, setStyle_helper,
                // qwidget.cpp:2706-2712, under an application style sheet). The label is polished again here, as addClass does it.
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
    //
    // The same holds for the disabled group: the :disabled backgrounds that
    // reach a widget painted from Base are the entry classes' alone
    // (LineEdit.qss:28-35, SpinBox.qss:69-74, ComboBox.qss:53-57), so a view
    // keeps its enabled Base when disabled (BaseStyleSheet.qss:23-38 has no
    // :disabled rule); the #666666 belongs to applyFlattenedTextEntryPalette.
    //
    // So do the disabled text roles: the '*' rule's white (BaseStyleSheet.qss:15-21)
    // reached the Disabled group too, and only the classes that have a :disabled
    // colour differ (applyFlattenedDisabledText), so WindowText, Text, ButtonText
    // and HighlightedText stay white when disabled. Only the placeholder keeps the
    // entry classes' grey.
    //////////////////////////////////////////////////////////////////////////
    static QPalette buildO3DEBasePalette()
    {
        QPalette palette;
        // Core surfaces
        const QColor window(0x44, 0x44, 0x44);          // QMainWindow/QDialog/QDockWidget background-color: #444444
        const QColor viewBase(0x44, 0x44, 0x44);        // parentless QTableView/QListView/QTreeView background-color: #444444
        const QColor alternateBase(0x4D, 0x4D, 0x4D);   // TableView.qss alternate-background-color: rgb(77,77,77)
        const QColor textWhite(0xFF, 0xFF, 0xFF);       // global '*' rule color: white
        const QColor disabledText(0x99, 0x99, 0x99);    // QLineEdit:disabled color: #999999 (the placeholder only, see above)

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
        palette.setColor(QPalette::Disabled, QPalette::WindowText, textWhite);  // '*' color: white, every state (BaseStyleSheet.qss:18)
        palette.setColor(QPalette::Disabled, QPalette::Base, viewBase);     // views: no :disabled rule (BaseStyleSheet.qss:23-38)
        palette.setColor(QPalette::Disabled, QPalette::AlternateBase, alternateBase);
        palette.setColor(QPalette::Disabled, QPalette::Text, textWhite);
        palette.setColor(QPalette::Disabled, QPalette::PlaceholderText, disabledText);
        palette.setColor(QPalette::Disabled, QPalette::Button, button);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, textWhite);
        palette.setColor(QPalette::Disabled, QPalette::BrightText, textWhite);
        palette.setColor(QPalette::Disabled, QPalette::Light, light);
        palette.setColor(QPalette::Disabled, QPalette::Midlight, midlight);
        palette.setColor(QPalette::Disabled, QPalette::Mid, mid);
        palette.setColor(QPalette::Disabled, QPalette::Dark, dark);
        palette.setColor(QPalette::Disabled, QPalette::Shadow, shadow);
        palette.setColor(QPalette::Disabled, QPalette::Highlight, highlight);
        palette.setColor(QPalette::Disabled, QPalette::HighlightedText, highlightedText);   // white in every group of the Native dumps
        palette.setColor(QPalette::Disabled, QPalette::ToolTipBase, toolTipBase);
        palette.setColor(QPalette::Disabled, QPalette::ToolTipText, toolTipText);
        palette.setColor(QPalette::Disabled, QPalette::Link, link);
        palette.setColor(QPalette::Disabled, QPalette::LinkVisited, link);

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
            // out as the base palette (qapplication.cpp:1190-1198), so they never took effect. The menu and menu bar elements carry the
            // colours of both (MenuElement.cpp).

            // Class fonts: Qt rebuilt them from the platform in this setStyle, right before this polish
            TextElement::installClassFonts();

            // The Global box elements of the color picker and the color label (ColorPicker.qss, ColorLabel.qss): once, whatever the
            // number of setStyle calls
            ColorPickerElements::registerGlobal();

            // The Global box elements of the property editor (ReflectedPropertyEditor.qss, Card.qss): once, whatever the number of setStyle calls
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
