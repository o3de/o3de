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
#include <AzQtComponents/Components/TitleBarOverdrawHandler.h>
#include <AzQtComponents/Utilities/TextUtilities.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
#include <QApplication>
#include <QCheckBox>
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
#include <QtGui/private/qscreen_p.h>
#include <QtWidgets/private/qstylesheetstyle_p.h>
AZ_POP_DISABLE_WARNING

#include <QtWidgets/private/qstylehelper_p.h>

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
        if (!hasStyle(widget))
        {
            return QProxyStyle::sizeFromContents(type, option, size, widget);
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
            case QStyle::CT_TabBarTab:
            {
                const auto tabSize = TabBar::sizeFromContents(this, type, option, size, widget, m_data->tabWidgetConfig);
                if (tabSize.isValid())
                {
                    return tabSize;
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
                if (BrowseEdit::drawFrame(this, option, painter, widget, m_data->browseEditConfig))
                {
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

            case CE_TabBarTabLabel:
            {
                if (qobject_cast<const QTabBar*>(widget))
                {
                    // Qt lacks a textAlignment variable in QStyleOptionTab, which is used for drawing QTabWidget and QTabBar.
                    // Text alignment for tab labels is hardcoded to horizontal center. For this reason, there is no way to customize
                    // text alignment if not creating a new member variable for QStyleOptionTab (or to subclass QStyleOptionTab) on Qt
                    // side. To avoid doing either, we set a new variable to be used from Style::drawItemText, with a scope limited to
                    // the drawing of this specific control element (the tab label).
                    QScopedValueRollback<QVariant> rollbackTabBarTabLabel(m_drawItemTextAlignmentOverride, {(int)(Qt::AlignLeft | Qt::AlignVCenter)});
                    if (TabBar::drawTabBarTabLabel(this, option, painter, widget, m_data->tabWidgetConfig))
                    {
                        return;
                    }
                }
            }
            break;

            case CE_Header:
            {
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
            case CE_MenuItem:
            {
                const QMenu* menu = qobject_cast<const QMenu*>(widget);
                QAction* action = menu->activeAction();
                if (action)
                {
                    QMenu* subMenu = action->menu();
                    if (subMenu)
                    {
                        QVariant noHover = subMenu->property("noHover");
                        if (noHover.isValid() && noHover.toBool())
                        {
                            // First draw as standard to get the correct hover background for the complete control.
                            QProxyStyle::drawControl(element, option, painter, widget);
                            // Now draw the icon as non-hovered so control behaves as designed.
                            QStyleOptionMenuItem myOpt = *qstyleoption_cast<const QStyleOptionMenuItem*>(option);
                            myOpt.state &= ~QStyle::State_Selected;
                            return QProxyStyle::drawControl(element, &myOpt, painter, widget);
                        }
                    }
                }
                // Implement checkmark with icon in menu.
                QStyleOptionMenuItem myOpt = *qstyleoption_cast<const QStyleOptionMenuItem*>(option);
                bool checkable = myOpt.checkType != QStyleOptionMenuItem::NotCheckable;
                bool checked = checkable ? myOpt.checked : false;

                if (!myOpt.icon.isNull() && checked)
                {
                    const int iconSize{ 18 };
                    int topPadding{ AZStd::max( 0 , (myOpt.rect.height() - iconSize) / 2) - 1};

                    QProxyStyle::drawControl(element, &myOpt, painter, widget);
                    myOpt.rect.adjust(0, topPadding, iconSize - myOpt.rect.width(), iconSize - myOpt.rect.height());
                    return drawPrimitive(PE_IndicatorMenuCheckMark, &myOpt, painter, widget);
                }
            }
            break;
        }

        return QProxyStyle::drawControl(element, option, painter, widget);
    }

    void Style::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
    {
        if (element == PE_IndicatorDockWidgetResizeHandle)
        {
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
                if (TreeView::drawBranchIndicator(this, option, painter, widget, m_data->treeViewConfig))
                {
                    return;
                }
                // With how we setup our styles, Style::drawPrimitive gets first crack at the treeview branch indicator.
                // Since it doesn't care to do anything, it delegates to the base style, which is a QStyleSheetStyle.
                // If there is a css rule in the loaded stylesheet, the QStyleSheetStyle will follow those rules -
                // and NOT draw the indicator arrow.
                // So below, we let the QStyleSheetStyle do its thing, then we manually call into the Fusion style
                // to draw the arrows.
                // If you really don't want this, add the g_treeViewDisableDefaultArrorPainting class to your object.
                // I.e. Style::addClass(aQTreeView, g_treeViewDisableDefaultArrorPainting);
#if !defined(AZ_PLATFORM_LINUX)

                if (qobject_cast<const QTreeView*>(widget) && !hasClass(widget, g_treeViewDisableDefaultArrorPainting))
                {
                    QStyleSheetStyle* styleSheetStyle = qobject_cast<QStyleSheetStyle*>(baseStyle());
                    if (styleSheetStyle)
                    {
                        QStyle* fusionStyle = styleSheetStyle->baseStyle();
                        if (fusionStyle && (fusionStyle != this) && (fusionStyle != styleSheetStyle))
                        {
                            QProxyStyle::drawPrimitive(element, option, painter, widget);

                            return fusionStyle->drawPrimitive(element, option, painter, widget);
                        }
                    }
                }
#endif // !defined(AZ_PLATFORM_LINUX)
            }
            break;

            case PE_PanelMenu:
            {
                if (Menu::drawFrame(this, option, painter, widget, m_data->menuConfig))
                {
                    return;
                }
            }
            break;

            case PE_IndicatorItemViewItemCheck:
            {
                if (ComboBox::drawItemCheckIndicator(this, option, painter, widget, m_data->comboBoxConfig))
                {
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
                if (ComboBox::drawComboBox(this, option, painter, widget, m_data->comboBoxConfig))
                {
                    return;
                }
                break;

            case CC_ScrollBar:
                if (ScrollBar::drawScrollBar(this, option, painter, widget, m_data->scrollBarConfig))
                {
                    return;
                }
                break;
        }

        return QProxyStyle::drawComplexControl(element, option, painter, widget);
    }

    void Style::drawItemText(QPainter* painter, const QRect& rectangle, int alignment, const QPalette& palette, bool enabled, const QString& text, QPalette::ColorRole textRole) const
    {
        if (m_drawItemTextAlignmentOverride.isValid())
        {
            alignment = m_drawItemTextAlignmentOverride.value<Qt::Alignment>();
        }
        QProxyStyle::drawItemText(painter, rectangle, alignment, palette, enabled, text, textRole);
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

                        if (vectorElement && doubleSpinBox)
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

    QRect Style::subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget) const
    {
        if (!hasStyle(widget))
        {
            return QProxyStyle::subElementRect(element, option, widget);
        }

        switch (element)
        {
            case SE_ItemViewItemText:       // intentional fall-through
            case SE_ItemViewItemDecoration: // intentional fall-through
            case SE_ItemViewItemFocusRect:
            {
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

            case QStyle::PM_MenuHMargin:
            {
                const int margin = Menu::horizontalMargin(this, option, widget, m_data->menuConfig);
                if (margin != -1)
                {
                    return margin;
                }
                break;
            }

            case QStyle::PM_MenuVMargin:
            {
                const int margin = Menu::verticalMargin(this, option, widget, m_data->menuConfig);
                if (margin != -1)
                {
                    return margin;
                }
                break;
            }

            case QStyle::PM_MenuHPlacementOffset:
            {
                const int hOffset = Menu::horizontalShadowMargin(this, option, widget, m_data->menuConfig);
                if (hOffset != std::numeric_limits<int>::lowest())
                {
                    return hOffset;
                }
                break;
            }

            case QStyle::PM_MenuVPlacementOffset:
            {
                const int vOffset = Menu::verticalShadowMargin(this, option, widget, m_data->menuConfig);
                if (vOffset != std::numeric_limits<int>::lowest())
                {
                    return vOffset;
                }
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
                const QPoint wPos = widget->pos();
                const QPoint gPos = widget->mapToGlobal(wPos);
                int retval{ 12 };

                const QScreen* thisScreen = QGuiApplication::screenAt(gPos);
                if (!thisScreen)
                {
                    thisScreen = QGuiApplication::primaryScreen();
                }
                if (thisScreen)
                {
                    // We have to do this as the KDAB Dpi functions return a strange rounded value
                    // that means dpiScaled is always returned 12
                    const qreal dpi = thisScreen->handle()->logicalDpi().first;
                    if (dpi > 0)
                    {
                        retval = int(QStyleHelper::dpiScaled(12, dpi));
                    }
                }
                return retval;
            }

            default:
                break;
        }

        return QProxyStyle::pixelMetric(metric, option, widget);
    }

    void Style::polish(QWidget* widget)
    {
        static QWidget* alreadyStyling = nullptr;
        if (alreadyStyling == widget)
        {
            return;
        }

        QScopedValueRollback<QWidget*> recursionGuard(alreadyStyling, widget);

        TitleBarOverdrawHandler::getInstance()->polish(widget);

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
            for (int i = classList.count() -1; i >= 0; --i)
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

    void Style::polish(QApplication* application)
    {
        Q_UNUSED(application);

        m_data->palette = application->palette();
        m_data->palette.setColor(QPalette::Link, m_data->textConfig.hyperlinkColor);

        application->setPalette(m_data->palette);

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

#include "Components/moc_Style.cpp"
