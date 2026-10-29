/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/AssetFolderThumbnailView.h>
#include <AzQtComponents/Components/Widgets/BreadCrumbs.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorController.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/Palette.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/PaletteView.h>
#include <AzQtComponents/Components/Widgets/ScrollBar.h>
#include <AzQtComponents/Components/Widgets/TreeView.h>
#include <AzQtComponents/Components/Widgets/TableView.h>

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QBoxLayout>
#include <QClipboard>
#include <QComboBox>
#include <QFile>
#include <QFontDatabase>
#include <QHeaderView>
#include <QHoverEvent>
#include <QIcon>
#include <QImage>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QProxyStyle>
#include <QScrollArea>
#include <QScrollBar>
#include <QRegularExpression>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QStyleFactory>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QTreeView>
#include <QUndoStack>
#include <QtMath>

#include <algorithm>
#include <iterator>
#include <string>
#include <type_traits>
#include <vector>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The item views of the flattened GUI against the legacy GUI: the sizes, the rects, the paint and the roles of the items, the rows, the
    // branches and the headers of a QTreeView, an AzQtComponents::TableView, a QTableView and a QListView, and what a module's element says of
    // them. The legacy twin of a view is Qt's own style sheet style with the REAL application sheet (BaseStyleSheet.qss and what it imports,
    // TableView.qss among them) over the style the Native GUI had beneath it; the flat twin is the flattened Style with the element.
    //
    // What the Native GUI had beneath the sheet is the style of AzQtComponents in its legacy mode, which a process that runs the flattened
    // GUI cannot be (StyleManager::stylesheetsDisabled keeps its first answer); LegacyBaseStyle is the part of it item views see: the header
    // section's size (TableView::sizeFromContents), the vertical header's default section (PM_HeaderDefaultSectionSizeVertical, Style.cpp), the
    // alternate row of a TableView (PE_PanelItemViewRow) and the indentation TreeView::polish gave a tree.
    // ----------------------------------------------------------------------------------

    namespace ItemViewElementTestData
    {
        QColor rgb(int value)
        {
            return QColor((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
        }

        //! The application sheet as StyleSheetCache builds it from BaseStyleSheet.qss: each @import line replaced by the file it names, from the real resources
        const QString& appSheet()
        {
            static QString expanded;
            if (expanded.isEmpty())
            {
                const auto read = [](const QString& name)
                {
                    QFile file(QStringLiteral(":/AzQtComponents/Widgets/") + name);
                    EXPECT_TRUE(file.open(QIODevice::ReadOnly)) << name.toUtf8().constData();
                    return QString::fromUtf8(file.readAll());
                };
                expanded = read(QStringLiteral("BaseStyleSheet.qss"));
                const QRegularExpression import(QStringLiteral("@import\\s+\"([^\"]+)\"\\s*;"));
                for (QRegularExpressionMatch match = import.match(expanded); match.hasMatch(); match = import.match(expanded))
                {
                    expanded.replace(match.capturedStart(), match.capturedLength(), read(match.captured(1)));
                }
            }
            return expanded;
        }

        //! The style beneath the legacy sheet (see the comment above)
        class LegacyBaseStyle
            : public QProxyStyle
        {
        public:
            using QProxyStyle::polish;

            explicit LegacyBaseStyle(QStyle* base)
                : QProxyStyle(base)
            {
            }

            int pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const override
            {
                if (metric == PM_HeaderDefaultSectionSizeVertical)
                {
                    return 24;                                              // Style.cpp, in every mode
                }
                return QProxyStyle::pixelMetric(metric, option, widget);
            }

            QSize sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget) const override
            {
                // TableView::sizeFromContents (TableView.cpp:244-291), what Style::sizeFromContents answers for a horizontal header section
                auto header = qobject_cast<const QHeaderView*>(widget);
                auto headerOption = qstyleoption_cast<const QStyleOptionHeader*>(option);
                if (type == CT_HeaderSection && header && header->orientation() == Qt::Horizontal && headerOption)
                {
                    const bool nullIcon = headerOption->icon.isNull();
                    const int margin = pixelMetric(PM_HeaderMargin, headerOption, widget);
                    const int iconSize = nullIcon ? 0 : pixelMetric(PM_SmallIconSize, headerOption, widget);
                    const QSize text = headerOption->fontMetrics.size(0, headerOption->text);
                    QSize hint(0, margin + qMax(iconSize, text.height()) + margin);
                    hint.setWidth(iconSize + text.width() + 16 + margin + (nullIcon ? 0 : margin) + (headerOption->text.isNull() ? 0 : margin));
                    if (headerOption->sortIndicator != QStyleOptionHeader::None)
                    {
                        hint.rwidth() += hint.height() + margin;
                    }
                    return hint;
                }
                return QProxyStyle::sizeFromContents(type, option, size, widget);
            }

            void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override
            {
                // Style.cpp: the first row of a TableView takes the alternate colour (Style::drawPrimitive, PE_PanelItemViewRow)
                auto rowOption = qstyleoption_cast<const QStyleOptionViewItem*>(option);
                auto tableView = qobject_cast<const TableView*>(widget);
                if (element == PE_PanelItemViewRow && rowOption && tableView && tableView->alternatingRowColors())
                {
                    QStyleOptionViewItem copy = *rowOption;
                    copy.features ^= QStyleOptionViewItem::Alternate;
                    QProxyStyle::drawPrimitive(element, &copy, painter, widget);
                    return;
                }
                QProxyStyle::drawPrimitive(element, option, painter, widget);
            }

            void polish(QWidget* widget) override
            {
                QProxyStyle::polish(widget);
                // TreeView::polish: a tree that is not a TableView has the indentation 20
                if (auto tree = qobject_cast<QTreeView*>(widget); tree && !qobject_cast<TableView*>(widget))
                {
                    tree->setIndentation(20);
                }
            }
        };

        //! A view that hands out the option it hands its delegates
        template<typename View>
        class ExposedView
            : public View
        {
        public:
            using View::View;

            QStyleOptionViewItem viewOption() const
            {
                QStyleOptionViewItem option;
                this->initViewItemOption(&option);
                return option;
            }
        };

        //! A delegate that hands out the option it makes of an index
        class ExposedDelegate
            : public QStyledItemDelegate
        {
        public:
            using QStyledItemDelegate::QStyledItemDelegate;

            void option(QStyleOptionViewItem* option, const QModelIndex& index) const
            {
                initStyleOption(option, index);
            }
        };

        QIcon itemIcon(const QColor& color = Qt::red)
        {
            QPixmap pixmap(16, 16);
            pixmap.fill(color);
            return QIcon(pixmap);
        }

        //! The model the views show: a text, an icon, a check box, a check box with no text, everything, and a branch with a child
        QStandardItemModel* makeModel(QObject* parent, bool tree)
        {
            auto* model = new QStandardItemModel(parent);
            model->setColumnCount(2);
            for (int row = 0; row < 8; ++row)
            {
                auto* first = new QStandardItem(QStringLiteral("Apple %1").arg(row));
                auto* second = new QStandardItem(QStringLiteral("Second %1").arg(row));
                if (row == 1 || row == 4)
                {
                    first->setIcon(itemIcon());
                }
                if (row == 2 || row == 3 || row == 4)
                {
                    first->setCheckable(true);
                    first->setCheckState(row == 3 ? Qt::Checked : Qt::Unchecked);
                }
                if (row == 3)
                {
                    first->setText(QString());
                }
                if (tree && (row == 0 || row == 5))
                {
                    first->appendRow(new QStandardItem(QStringLiteral("child")));
                }
                model->appendRow({ first, second });
            }
            return model;
        }

        //! The flat twin is the window that carries the property; the legacy twin carries the sheet instead
        bool flatTwin(const QWidget* widget)
        {
            return widget->window()->property("itemViewElementTestsFlat").toBool();
        }

        bool named(const QWidget* widget, const char* objectName)
        {
            return flatTwin(widget) && widget->objectName() == QLatin1String(objectName);
        }

        // ---- The item view rules of the Asset Processor (AssetsTab.qss:86-105, AssetProcessor.qss:50-165), and the element that carries them. The
        // selectors are the sheets', except that `QTableWidget` is a `QTableView#apTable` here (the twin's view is a QTableView).
        const char* const ApRules =
            "AzQtComponents--TableView, #builderList {"
            "    background-color: rgb(45,45,45);"
            "    alternate-background-color: rgb(34,34,34);"
            "    selection-background-color: rgb(73,73,73);"
            "}"
            "AzQtComponents--TableView::branch:selected,"
            "AzQtComponents--TableView::item:selected {"
            "    background: rgb(73,73,73);"
            "}"
            "AzQtComponents--TableView::branch:hover,"
            "AzQtComponents--TableView::item:hover {"
            "    background: rgb(60,60,60);"
            "}"
            "AzQtComponents--TableView QHeaderView::section {"
            "    background-color: rgb(34,34,34);"
            "}"
            "QTableView#apTable {"
            "    background-color: rgb(45,45,45);"
            "    alternate-background-color: rgb(34,34,34);"
            "    selection-background-color: rgb(73,73,73);"
            "}"
            "QTableView#apTable::item:selected {"
            "    background: rgb(73,73,73);"
            "}"
            "QTableView#apTable::item:hover {"
            "    background: rgb(60,60,60);"
            "}"
            "QTableView#apTable QHeaderView::section {"
            "    background-color: rgb(34,34,34);"
            "}"
            "QTreeView#SourceAssetsTreeView,"
            "QTreeView#SourceAssetsTreeView::item,"
            "QTreeView#SourceAssetsTreeView::branch {"
            "    background-color: rgb(45,45,45);"
            "}"
            "QTreeView#SourceAssetsTreeView::item:hover,"
            "QTreeView#SourceAssetsTreeView::branch:hover {"
            "    background-color: rgb(60,60,60);"
            "}"
            "QTreeView#SourceAssetsTreeView::item:selected,"
            "QTreeView#SourceAssetsTreeView::branch:selected,"
            "QTreeView#SourceAssetsTreeView::item:selected:active {"
            "    background-color: rgb(73,73,73);"
            "}"
            "QTreeView#SourceAssetsTreeView::item:selected:!active {"
            "    background-color: rgb(60,60,60);"
            "    selection-color: rgb(192, 192, 192);"
            "}";

        // AssetsTab.qss:86-105: the TableView and the tables of the window; the colour of a header section is its rule's
        const ItemViewElement ApTable{
            .m_name = "Test.ItemView.ApTable",
            .m_alternate = rgb(0x222222),                                                                   // rgb(34,34,34)
            .m_selection = rgb(0x494949),                                                                   // rgb(73,73,73)
            .m_faces = {
                { ItemViewElement::Selected, { rgb(0x494949), QColor(), QColor() } },                       // background: rgb(73,73,73)
                { ItemViewElement::Hover, { rgb(0x3C3C3C), QColor(), QColor() } },                          // background: rgb(60,60,60)
            },
            .m_headerFill = rgb(0x222222),
        };

        // AssetsTab.qss:86-91: the builder list has the roles of the table and no rule for its items
        const ItemViewElement ApBuilderList{
            .m_name = "Test.ItemView.ApBuilderList",
            .m_alternate = rgb(0x222222),
            .m_selection = rgb(0x494949),
        };

        // AssetProcessor.qss:50-103: the asset trees fill their items and branches, and colour them by state
        const ItemViewElement ApTree{
            .m_name = "Test.ItemView.ApTree",
            .m_faces = {
                { 0, { rgb(0x2D2D2D), QColor(), QColor() } },                                               // ::item, ::branch { background-color: rgb(45,45,45) }
                { ItemViewElement::Hover, { rgb(0x3C3C3C), QColor(), QColor() } },                          // :hover rgb(60,60,60)
                { ItemViewElement::Selected, { rgb(0x494949), QColor(), QColor() } },                       // :selected, :selected:active rgb(73,73,73)
                // ::item:selected:!active, a rule for the item alone: the branch keeps its :selected colour in an inactive window
                { ItemViewElement::Selected | ItemViewElement::Inactive, { rgb(0x3C3C3C), QColor(), rgb(0xC0C0C0) }, ItemViewElement::OnItem },
            },
        };

        // AssetsTab.qss:67-77: the tree of a filter bar has no indentation
        const char* const ApFilterRules = "#apFilterTree { qproperty-indentation: 0; }";
        const ItemViewElement ApFilterTree{
            .m_name = "Test.ItemView.ApFilterTree",
            .m_indentation = 0,
        };

        // ProjectManager.qss:872-874: the sections of the gem list's header are transparent
        const char* const PmRules = "#adjustableHeaderWidget QHeaderView::section { background-color: transparent; }";
        const ItemViewElement PmAdjustableHeader{
            .m_name = "Test.ItemView.PmAdjustableHeader",
            .m_headerFill = QColor(0, 0, 0, 0),
        };

        // AssetBundler.qss:28-30, :56-58 (the second selector is `QTabWidget QTreeView QHeaderView::section`, scoped to the view here)
        const char* const AbLogRules = "QTreeView#logTableView QHeaderView::section { background-color: #1B1B1B; }";
        const char* const AbTabRules = "#abFileTable QHeaderView::section { background-color: rgb(77, 77, 77); }";
        const ItemViewElement AbLogHeader{ .m_name = "Test.ItemView.AbLogHeader", .m_headerFill = rgb(0x1B1B1B) };
        const ItemViewElement AbTabHeader{ .m_name = "Test.ItemView.AbTabHeader", .m_headerFill = rgb(0x4D4D4D) };

        // EntityOutliner.qss:15-32: the outliner draws the selection and the hover of its items itself, so its tree has none (the selector is
        // `AzToolsFramework--EntityOutlinerWidget QTreeView`, the root of the twin here)
        const char* const OutlinerRules =
            "#root QTreeView {"
            "    selection-background-color: transparent;"
            "}"
            "#root QTreeView::branch:hover,"
            "#root QTreeView::item:hover,"
            "#root QTreeView::branch:selected,"
            "#root QTreeView::item:selected,"
            "#root QTreeView::branch:selected:active,"
            "#root QTreeView::item:selected:active {"
            "    background: transparent;"
            "}";
        const ItemViewElement OutlinerTree{
            .m_name = "Test.ItemView.OutlinerTree",
            .m_selection = QColor(0, 0, 0, 0),
            .m_faces = {
                { ItemViewElement::Selected, { QColor(0, 0, 0, 0), QColor(), QColor() } },
                { ItemViewElement::Hover, { QColor(0, 0, 0, 0), QColor(), QColor() } },
            },
        };

        const char* const ViewKinds[] = { "QTreeView", "TableView", "QTableView", "QListView" };
        const Qt::LayoutDirection Directions[] = { Qt::LeftToRight, Qt::RightToLeft };
        const qreal Ratios[] = { 1.0, 1.5 };
    } // namespace ItemViewElementTestData

    // The tests and their fixture sit in a namespace of their own: the using-directive below must not leak into the
    // other files of a unity build
    namespace ItemViewElementTestsInternal
    {
        using namespace ItemViewElementTestData;

        // ----------------------------------------------------------------------------------
        // Fixture: the flattened Style over Fusion, LegacyBaseStyle as the base of the legacy sheet
        // ----------------------------------------------------------------------------------

        class ItemViewElementTests
            : public ::UnitTest::LeakDetectionFixture
        {
        public:
            ItemViewElementTests()
            {
                m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);
                QApplication::setStyle(new LegacyBaseStyle(QStyleFactory::create(QStringLiteral("Fusion"))));
                m_style = AZStd::make_unique<Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                registerElements();
            }

            //! Registered once for the process, as a module registers its own
            static void registerElements()
            {
                static bool registered = false;
                if (registered)
                {
                    return;
                }
                registered = true;

                ItemViewElement::registerElement(&ApTable, &AzQtComponents::TableView::staticMetaObject, [](const QWidget* w) { return named(w, "apTableView"); });
                ItemViewElement::registerElement(&ApTable, &QTableView::staticMetaObject, [](const QWidget* w) { return named(w, "apTable"); });
                ItemViewElement::registerElement(&ApBuilderList, &QListView::staticMetaObject, [](const QWidget* w) { return named(w, "builderList"); });
                ItemViewElement::registerElement(&ApTree, &QTreeView::staticMetaObject, [](const QWidget* w) { return named(w, "SourceAssetsTreeView"); });
                ItemViewElement::registerElement(&ApFilterTree, &QTreeView::staticMetaObject, [](const QWidget* w) { return named(w, "apFilterTree"); });
                ItemViewElement::registerElement(&PmAdjustableHeader, &QTableView::staticMetaObject, [](const QWidget* w) { return named(w, "adjustableHeaderWidget"); });
                ItemViewElement::registerElement(&AbLogHeader, &AzQtComponents::TableView::staticMetaObject, [](const QWidget* w) { return named(w, "logTableView"); });
                ItemViewElement::registerElement(&AbTabHeader, &AzQtComponents::TableView::staticMetaObject, [](const QWidget* w) { return named(w, "abFileTable"); });
                ItemViewElement::registerElement(&OutlinerTree, &QTreeView::staticMetaObject, [](const QWidget* w) { return named(w, "m_objectTree"); });
            }

            ~ItemViewElementTests() override
            {
                m_style.reset();
                m_application.reset();
            }

            //! One view, twice: a root with the view (named "view") in it, shown, the legacy one under the application sheet and the rules the test adds
            //! to it, the flat one under the style
            template<typename View>
            struct Twins
            {
                Twins(const ItemViewElementTests& tests, const QString& extraRules, Qt::LayoutDirection direction, const QSize& size = QSize(320, 240),
                    const char* name = "view")
                    : m_name(name)
                {
                    build(m_legacy, direction, size);
                    m_legacy.setStyleSheet(appSheet() + extraRules);
                    show(m_legacy, size);

                    build(m_flat, direction, size);
                    m_flat.setProperty("itemViewElementTestsFlat", true);
                    // The application palette of the flattened GUI (every palette passes through the style's polish)
                    QPalette palette;
                    tests.m_style->polish(palette);
                    m_flat.setPalette(palette);
                    m_flat.setStyle(tests.m_style.get());
                    for (QWidget* child : m_flat.findChildren<QWidget*>())
                    {
                        child->setStyle(tests.m_style.get());
                    }
                    show(m_flat, size);

                    // The font is not what the tests prove: the flat view has the one the sheet gave its twin
                    m_flatView->setFont(m_legacyView->font());
                    if (QHeaderView* header = headerOf(m_legacyView))
                    {
                        headerOf(m_flatView)->setFont(header->font());
                    }
                }

                static QHeaderView* headerOf(QTreeView* view)
                {
                    return view->header();
                }

                static QHeaderView* headerOf(QTableView* view)
                {
                    return view->horizontalHeader();
                }

                static QHeaderView* headerOf(QListView*)
                {
                    return nullptr;
                }

                void build(QWidget& root, Qt::LayoutDirection direction, const QSize& size)
                {
                    root.setObjectName(QStringLiteral("root"));
                    root.setLayoutDirection(direction);
                    auto* view = new ExposedView<View>(&root);
                    view->setObjectName(QString::fromLatin1(m_name));
                    view->setGeometry(0, 0, size.width(), size.height());
                    view->setModel(makeModel(&root, qobject_cast<QTreeView*>(view) != nullptr));
                    view->setIconSize(QSize(16, 16));
                    (&root == &m_legacy ? m_legacyView : m_flatView) = view;
                }

                static void show(QWidget& root, const QSize& size)
                {
                    root.setAttribute(Qt::WA_DontShowOnScreen);
                    root.resize(size);
                    root.show();
                    QApplication::processEvents();
                }

                const char* m_name;
                QWidget m_legacy;
                QWidget m_flat;
                ExposedView<View>* m_legacyView = nullptr;
                ExposedView<View>* m_flatView = nullptr;
            };

        public:
            Style* style() const
            {
                return m_style.get();
            }

        protected:
            int m_argc = 1;                                     // QApplication keeps a reference to it
            char m_programName[5] = "test";
            char* m_argv[2] = { m_programName, nullptr };
            AZStd::unique_ptr<QApplication> m_application;
            AZStd::unique_ptr<Style> m_style;
        };

        // ----------------------------------------------------------------------------------
        // T1: the size of an item, the room for its check box and the rects of its check box, icon and text
        // ----------------------------------------------------------------------------------

        //! The options of the items the tests look at: a text, short or long, with an icon, a check box or both, in each place of the icon, wrapped or not
        std::vector<QStyleOptionViewItem> itemOptions(const QWidget* view, const QFont& font, Qt::LayoutDirection direction)
        {
            std::vector<QStyleOptionViewItem> options;
            for (const bool text : { true, false })
            {
                for (const bool icon : { false, true })
                {
                    for (const bool check : { false, true })
                    {
                        for (const auto position : { QStyleOptionViewItem::Left, QStyleOptionViewItem::Right, QStyleOptionViewItem::Top, QStyleOptionViewItem::Bottom })
                        {
                            for (const bool wrap : { false, true })
                            {
                                QStyleOptionViewItem option;
                                option.initFrom(view);
                                option.widget = view;
                                option.styleObject = const_cast<QWidget*>(view);
                                option.direction = direction;
                                option.state = QStyle::State_Enabled | QStyle::State_Active;
                                option.font = font;
                                option.fontMetrics = QFontMetrics(font);
                                option.decorationSize = QSize(16, 16);
                                option.decorationPosition = position;
                                option.rect = QRect(7, 11, 140, (position == QStyleOptionViewItem::Top || position == QStyleOptionViewItem::Bottom) ? 64 : 24);
                                option.features = QStyleOptionViewItem::None;
                                if (text)
                                {
                                    option.features |= QStyleOptionViewItem::HasDisplay;
                                    option.text = wrap ? QStringLiteral("A text long enough to wrap in the room an item leaves it") : QStringLiteral("Apple");
                                }
                                if (wrap)
                                {
                                    option.features |= QStyleOptionViewItem::WrapText;
                                }
                                if (icon)
                                {
                                    option.features |= QStyleOptionViewItem::HasDecoration;
                                    option.icon = itemIcon();
                                }
                                if (check)
                                {
                                    option.features |= QStyleOptionViewItem::HasCheckIndicator;
                                    option.checkState = Qt::Checked;
                                }
                                options.push_back(option);
                            }
                        }
                    }
                }
            }
            return options;
        }

        QString describe(const QStyleOptionViewItem& option)
        {
            return QStringLiteral("text=%1 features=%2 position=%3 rect=%4,%5 %6x%7 direction=%8")
                .arg(option.text.isEmpty() ? QStringLiteral("-") : option.text.left(8)).arg(int(option.features)).arg(int(option.decorationPosition))
                .arg(option.rect.x()).arg(option.rect.y()).arg(option.rect.width()).arg(option.rect.height()).arg(int(option.direction));
        }

        template<typename View>
        void expectSameItemGeometry(const ItemViewElementTests::Twins<View>& twins, const char* kind)
        {
            const QWidget* legacyView = twins.m_legacyView;
            const QWidget* flatView = twins.m_flatView;
            const QFont font = legacyView->font();
            const auto legacyOptions = itemOptions(legacyView, font, twins.m_legacy.layoutDirection());
            const auto flatOptions = itemOptions(flatView, font, twins.m_flat.layoutDirection());
            ASSERT_EQ(legacyOptions.size(), flatOptions.size());
            for (size_t i = 0; i < legacyOptions.size(); ++i)
            {
                SCOPED_TRACE(std::string(kind) + " " + describe(legacyOptions[i]).toStdString());
                const QStyleOptionViewItem& legacyOption = legacyOptions[i];
                const QStyleOptionViewItem& flatOption = flatOptions[i];
                EXPECT_EQ(flatView->style()->sizeFromContents(QStyle::CT_ItemViewItem, &flatOption, QSize(), flatView),
                    legacyView->style()->sizeFromContents(QStyle::CT_ItemViewItem, &legacyOption, QSize(), legacyView));
                for (const auto element : { QStyle::SE_ItemViewItemCheckIndicator, QStyle::SE_ItemViewItemDecoration, QStyle::SE_ItemViewItemText, QStyle::SE_ItemViewItemFocusRect })
                {
                    EXPECT_EQ(flatView->style()->subElementRect(element, &flatOption, flatView), legacyView->style()->subElementRect(element, &legacyOption, legacyView))
                        << "element " << int(element);
                }
                EXPECT_EQ(flatView->style()->pixelMetric(QStyle::PM_IndicatorWidth, &flatOption, flatView),
                    legacyView->style()->pixelMetric(QStyle::PM_IndicatorWidth, &legacyOption, legacyView));
                EXPECT_EQ(flatView->style()->pixelMetric(QStyle::PM_IndicatorHeight, &flatOption, flatView),
                    legacyView->style()->pixelMetric(QStyle::PM_IndicatorHeight, &legacyOption, legacyView));
            }
        }

        TEST_F(ItemViewElementTests, Items_TheBoxOfTheRuleSizesTheItemAndLaysOutItsCheckBoxIconAndText)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());   // the test environment sets it (AzQtComponentTests.cpp)
            for (const Qt::LayoutDirection direction : Directions)
            {
                SCOPED_TRACE(direction == Qt::RightToLeft ? "right to left" : "left to right");
                {
                    Twins<QTreeView> twins(*this, QString(), direction);
                    ASSERT_TRUE(ItemViewElement::forWidget(twins.m_flatView) != nullptr);
                    ASSERT_TRUE(ItemViewElement::forWidget(twins.m_legacyView) == nullptr);
                    expectSameItemGeometry(twins, "QTreeView");
                }
                {
                    Twins<TableView> twins(*this, QString(), direction);
                    expectSameItemGeometry(twins, "TableView");
                }
                {
                    Twins<QTableView> twins(*this, QString(), direction);
                    expectSameItemGeometry(twins, "QTableView");
                }
                {
                    Twins<QListView> twins(*this, QString(), direction);
                    expectSameItemGeometry(twins, "QListView");
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // T2..T4: what a style paints, through the style onto an image of an explicit device ratio (the painter is the image's, so the style
        // scales what it draws by that ratio: QWidget::render would hand it the screen's)
        // ----------------------------------------------------------------------------------

        template<typename Draw>
        QImage drawn(const QSize& size, qreal ratio, Draw draw)
        {
            QImage image((QSizeF(size) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(rgb(0x808080));
            QPainter painter(&image);
            draw(&painter);
            painter.end();
            return image;
        }

        //! The pixels of one colour in an image
        int countOf(const QImage& image, const QColor& color)
        {
            int count = 0;
            for (int y = 0; y < image.height(); ++y)
            {
                for (int x = 0; x < image.width(); ++x)
                {
                    count += (QColor::fromRgba(image.pixel(x, y)) == color) ? 1 : 0;
                }
            }
            return count;
        }

        //! The pixels that differ
        int differing(const QImage& a, const QImage& b)
        {
            if (a.size() != b.size())
            {
                return a.width() * a.height();
            }
            int count = 0;
            for (int y = 0; y < a.height(); ++y)
            {
                for (int x = 0; x < a.width(); ++x)
                {
                    count += (a.pixel(x, y) != b.pixel(x, y)) ? 1 : 0;
                }
            }
            return count;
        }

        //! The first pixel that differs, for the message of a failure
        QString firstDifference(const QImage& a, const QImage& b)
        {
            for (int y = 0; y < qMin(a.height(), b.height()); ++y)
            {
                for (int x = 0; x < qMin(a.width(), b.width()); ++x)
                {
                    if (a.pixel(x, y) != b.pixel(x, y))
                    {
                        return QStringLiteral("(%1,%2) %3 vs %4").arg(x).arg(y).arg(QColor::fromRgba(a.pixel(x, y)).name(QColor::HexArgb)).arg(QColor::fromRgba(b.pixel(x, y)).name(QColor::HexArgb));
                    }
                }
            }
            return QStringLiteral("size %1x%2 vs %3x%4").arg(a.width()).arg(a.height()).arg(b.width()).arg(b.height());
        }

        struct StateCase
        {
            const char* m_name;
            QStyle::State m_state;
        };

        const QStyle::State StateEnabled = QStyle::State_Enabled;
        const QStyle::State StateActive = QStyle::State_Active;
        const StateCase StateCases[] = {
            { "idle", StateEnabled | StateActive },
            { "selected", StateEnabled | StateActive | QStyle::State_Selected },
            { "hover", StateEnabled | StateActive | QStyle::State_MouseOver },
            { "selected and hover", StateEnabled | StateActive | QStyle::State_Selected | QStyle::State_MouseOver },
            { "inactive window", StateEnabled },
            { "selected in an inactive window", StateEnabled | QStyle::State_Selected },
            { "disabled", StateActive },
            { "disabled and selected", StateActive | QStyle::State_Selected },
        };

        //! The option of an item: a text, an icon and a check box, in a rect
        QStyleOptionViewItem paintOption(const QWidget* view, const QStyle::State state, const QRect& rect, bool icon, bool check)
        {
            QStyleOptionViewItem option;
            option.initFrom(view);
            option.widget = view;
            option.styleObject = const_cast<QWidget*>(view);
            option.state = state;
            option.rect = rect;
            option.font = view->font();
            option.fontMetrics = QFontMetrics(view->font());
            option.decorationSize = QSize(16, 16);
            option.text = QStringLiteral("Apple");
            option.features = QStyleOptionViewItem::HasDisplay;
            option.showDecorationSelected = true;
            if (icon)
            {
                option.features |= QStyleOptionViewItem::HasDecoration;
                option.icon = itemIcon();
            }
            if (check)
            {
                option.features |= QStyleOptionViewItem::HasCheckIndicator;
                option.checkState = Qt::Checked;
            }
            // the colour group the views set (QTreeView::drawRow, QTableViewPrivate::drawCell)
            option.palette.setCurrentColorGroup((state & QStyle::State_Enabled) ? ((state & QStyle::State_Active) ? QPalette::Active : QPalette::Inactive) : QPalette::Disabled);
            return option;
        }

        //! The cases a test found different, one line each, reported together
        struct Failures
        {
            void check(const std::string& name, const QImage& flat, const QImage& legacy)
            {
                if (const int count = differing(flat, legacy))
                {
                    m_lines.push_back(name + ": " + std::to_string(count) + " px, first " + firstDifference(flat, legacy).toStdString());
                }
            }

            ~Failures()
            {
                std::string text;
                for (size_t i = 0; i < m_lines.size() && i < 40; ++i)
                {
                    text += "\n  " + m_lines[i];
                }
                EXPECT_TRUE(m_lines.empty()) << m_lines.size() << " cases differ:" << text;
            }

            std::vector<std::string> m_lines;
        };

        template<typename View>
        void expectSameItemPaint(const ItemViewElementTests::Twins<View>& twins, const char* kind, Failures& failures)
        {
            for (const qreal ratio : Ratios)
            {
                for (const StateCase& stateCase : StateCases)
                {
                    for (const bool decorated : { false, true })
                    {
                        const std::string name = std::string(kind) + " " + stateCase.m_name + (decorated ? " with an icon and a check box" : " text only") + " at ratio " + std::to_string(ratio).substr(0, 3);
                        const auto paint = [&](const ExposedView<View>* view)
                        {
                            return drawn(QSize(160, 40), ratio, [&](QPainter* painter)
                            {
                                const QStyleOptionViewItem option = paintOption(view, stateCase.m_state, QRect(8, 8, 140, 24), decorated, decorated);
                                view->style()->drawControl(QStyle::CE_ItemViewItem, &option, painter, view);
                            });
                        };
                        failures.check(name, paint(twins.m_flatView), paint(twins.m_legacyView));
                    }
                }
            }
        }

        TEST_F(ItemViewElementTests, Items_ThePaintOfEachStateIsTheRulesFillAndTextColour)
        {
            Failures failures;
            Twins<QTreeView> tree(*this, QString(), Qt::LeftToRight);
            expectSameItemPaint(tree, "QTreeView", failures);
            Twins<TableView> tableView(*this, QString(), Qt::LeftToRight);
            expectSameItemPaint(tableView, "TableView", failures);
            Twins<QTableView> table(*this, QString(), Qt::LeftToRight);
            expectSameItemPaint(table, "QTableView", failures);
            Twins<QListView> list(*this, QString(), Qt::LeftToRight);
            expectSameItemPaint(list, "QListView", failures);
        }

        // ----------------------------------------------------------------------------------
        // T3: the panel of a row (what QTreeView::drawRow, QTableViewPrivate::drawCell and QListView::paintEvent ask the style for)
        // ----------------------------------------------------------------------------------

        template<typename View>
        void expectSameRowPanel(const ItemViewElementTests::Twins<View>& twins, const char* kind, Failures& failures)
        {
            for (const qreal ratio : Ratios)
            {
                for (const auto position : { QStyleOptionViewItem::Invalid, QStyleOptionViewItem::OnlyOne, QStyleOptionViewItem::Beginning, QStyleOptionViewItem::Middle, QStyleOptionViewItem::End })
                {
                    for (const bool alternate : { false, true })
                    {
                        for (const bool decorationOfTheRoot : { false, true })
                        {
                            for (const bool showDecorationSelected : { false, true })
                            {
                                for (const StateCase& stateCase : StateCases)
                                {
                                    const std::string name = std::string(kind) + " " + stateCase.m_name + " position " + std::to_string(int(position)) + (alternate ? " alternate" : "")
                                        + (decorationOfTheRoot ? " root decoration" : "") + (showDecorationSelected ? " decoration selected" : "") + " at ratio " + std::to_string(ratio).substr(0, 3);
                                    const auto paint = [&](const ExposedView<View>* view)
                                    {
                                        return drawn(QSize(160, 40), ratio, [&](QPainter* painter)
                                        {
                                            QStyleOptionViewItem option = paintOption(view, stateCase.m_state, QRect(8, 8, 140, 24), false, false);
                                            option.viewItemPosition = position;
                                            option.showDecorationSelected = showDecorationSelected;
                                            if (alternate)
                                            {
                                                option.features |= QStyleOptionViewItem::Alternate;
                                            }
                                            if (decorationOfTheRoot)
                                            {
                                                option.features |= QStyleOptionViewItem::IsDecorationForRootColumn | QStyleOptionViewItem::IsDecoratedRootColumn;
                                            }
                                            view->style()->drawPrimitive(QStyle::PE_PanelItemViewRow, &option, painter, view);
                                        });
                                    };
                                    failures.check(name, paint(twins.m_flatView), paint(twins.m_legacyView));
                                }
                            }
                        }
                    }
                }
            }
        }

        TEST_F(ItemViewElementTests, Rows_ThePanelIsTheBaseStylesWithTheRulesFillOverItInTheStateOfTheRow)
        {
            Failures failures;
            Twins<QTreeView> tree(*this, QString(), Qt::LeftToRight);
            expectSameRowPanel(tree, "QTreeView", failures);
            Twins<TableView> tableView(*this, QString(), Qt::LeftToRight);
            expectSameRowPanel(tableView, "TableView", failures);
            Twins<QTableView> table(*this, QString(), Qt::LeftToRight);
            expectSameRowPanel(table, "QTableView", failures);
            Twins<QListView> list(*this, QString(), Qt::LeftToRight);
            expectSameRowPanel(list, "QListView", failures);
        }

        // ----------------------------------------------------------------------------------
        // T5: the header: the size of a section, the paint of one, and the fill of the header's own area beyond its sections
        // ----------------------------------------------------------------------------------

        //! The headers of a view
        std::vector<QHeaderView*> headersOf(QTreeView* view)
        {
            return { view->header() };
        }

        std::vector<QHeaderView*> headersOf(QTableView* view)
        {
            return { view->horizontalHeader(), view->verticalHeader() };
        }

        //! The widget as it paints itself and its children, over a colour that shows through what it leaves transparent
        QImage rendered(QWidget& widget, const QColor& behind)
        {
            QImage image(widget.size(), QImage::Format_ARGB32_Premultiplied);
            image.fill(behind);
            widget.render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
            return image;
        }

        QStyleOptionHeader headerOption(const QHeaderView* header, const QRect& rect, const QString& text, QStyleOptionHeader::SortIndicator sort)
        {
            QStyleOptionHeader option;
            option.initFrom(header);
            option.rect = rect;
            option.orientation = header->orientation();
            option.state |= QStyle::State_Enabled;
            if (header->orientation() == Qt::Horizontal)
            {
                option.state |= QStyle::State_Horizontal;
            }
            option.text = text;
            option.textAlignment = Qt::AlignLeft | Qt::AlignVCenter;
            option.section = 0;
            option.position = QStyleOptionHeader::OnlyOneSection;
            option.sortIndicator = sort;
            option.fontMetrics = QFontMetrics(header->font());
            return option;
        }

        template<typename View>
        void expectSameHeaders(ItemViewElementTests::Twins<View>& twins, const char* kind, Failures& failures, const QColor& behind = rgb(0xFF00FF))
        {
            const auto legacyHeaders = headersOf(twins.m_legacyView);
            const auto flatHeaders = headersOf(twins.m_flatView);
            ASSERT_EQ(legacyHeaders.size(), flatHeaders.size());
            for (size_t i = 0; i < legacyHeaders.size(); ++i)
            {
                QHeaderView* legacy = legacyHeaders[i];
                QHeaderView* flat = flatHeaders[i];
                const std::string name = std::string(kind) + (legacy->orientation() == Qt::Horizontal ? " horizontal header" : " vertical header");

                // the size of a section: no label, a label, a label with the sort arrow
                for (const QString& text : { QString(), QString(QStringLiteral("")), QStringLiteral("Name") })
                {
                    for (const auto sort : { QStyleOptionHeader::None, QStyleOptionHeader::SortUp })
                    {
                        const QStyleOptionHeader legacyOption = headerOption(legacy, QRect(0, 0, 100, 24), text, sort);
                        const QStyleOptionHeader flatOption = headerOption(flat, QRect(0, 0, 100, 24), text, sort);
                        EXPECT_EQ(flat->style()->sizeFromContents(QStyle::CT_HeaderSection, &flatOption, QSize(), flat),
                            legacy->style()->sizeFromContents(QStyle::CT_HeaderSection, &legacyOption, QSize(), legacy))
                            << name << " text '" << text.toStdString() << "'" << (sort == QStyleOptionHeader::None ? "" : " sorted");
                    }
                }
                EXPECT_EQ(flat->sectionSizeHint(0), legacy->sectionSizeHint(0)) << name;
                EXPECT_EQ(flat->geometry(), legacy->geometry()) << name;

                // the section as painted, through the style: the fill, the label in the box the padding leaves, the sort arrow
                for (const qreal ratio : Ratios)
                {
                    for (const auto sort : { QStyleOptionHeader::None, QStyleOptionHeader::SortUp, QStyleOptionHeader::SortDown })
                    {
                        const auto paint = [&](QHeaderView* header)
                        {
                            return drawn(QSize(120, 40), ratio, [&](QPainter* painter)
                            {
                                painter->setFont(header->font());
                                const QStyleOptionHeader option = headerOption(header, QRect(8, 8, 100, 24), QStringLiteral("Name"), sort);
                                header->style()->drawControl(QStyle::CE_Header, &option, painter, header);
                            });
                        };
                        failures.check(name + " section sort " + std::to_string(int(sort)) + " at ratio " + std::to_string(ratio).substr(0, 3), paint(flat), paint(legacy));
                    }
                }

                // the header itself, sections and what lies beyond them, over a colour that shows what it does not fill
                legacy->setSectionResizeMode(QHeaderView::Fixed);
                flat->setSectionResizeMode(QHeaderView::Fixed);
                legacy->setStretchLastSection(false);
                flat->setStretchLastSection(false);
                QApplication::processEvents();
                failures.check(name + " rendered", rendered(*flat, behind), rendered(*legacy, behind));
            }
        }

        TEST_F(ItemViewElementTests, Headers_TheSectionIsTheRulesHeightAndFillAndTheHeaderBeyondItsSectionsHasTheFillToo)
        {
            Failures failures;
            Twins<QTreeView> tree(*this, QString(), Qt::LeftToRight);
            expectSameHeaders(tree, "QTreeView", failures);
            Twins<TableView> tableView(*this, QString(), Qt::LeftToRight);
            expectSameHeaders(tableView, "TableView", failures);
            Twins<QTableView> table(*this, QString(), Qt::LeftToRight);
            expectSameHeaders(table, "QTableView", failures);
        }

        // ----------------------------------------------------------------------------------
        // T6: a whole view: the rows of a tree, a table and a list as the view lays them out and paints them (selected, current, hovered, alternating)
        // ----------------------------------------------------------------------------------

        //! A view with some of its items expanded, selected and hovered, as the user would have left it
        template<typename View>
        void prepareView(ExposedView<View>* view, QStandardItemModel* model, int hoverRow, bool selectAll)
        {
            // The colours a model gives its items: the rule of the item is the whole of the panel, so the sheet drew no background of the model's
            // (PE_PanelItemViewItem: qstylesheetstyle.cpp:4881-4883, 4912-4916), and the colour of the text that the model gave stayed
            model->setData(model->index(6, 0), QBrush(rgb(0xAA2222)), Qt::BackgroundRole);
            model->setData(model->index(5, 0), QBrush(rgb(0x22AA22)), Qt::ForegroundRole);

            view->setAlternatingRowColors(true);
            view->setSelectionMode(QAbstractItemView::ExtendedSelection);
            if (auto tree = qobject_cast<QTreeView*>(view))
            {
                tree->expand(model->index(0, 0));
                tree->setRootIsDecorated(true);
            }
            view->selectionModel()->select(model->index(2, 0), QItemSelectionModel::Select);
            view->selectionModel()->select(model->index(3, 0), QItemSelectionModel::Select);
            if (selectAll)
            {
                view->selectionModel()->select(QItemSelection(model->index(0, 0), model->index(1, 1)), QItemSelectionModel::Select);
            }
            view->setCurrentIndex(model->index(2, 0));
            QApplication::processEvents();
            if (hoverRow >= 0)
            {
                const QRect rect = view->visualRect(model->index(hoverRow, 0));
                QHoverEvent hover(QEvent::HoverMove, QPointF(rect.center()), view->viewport()->mapToGlobal(QPointF(rect.center())), QPointF(0, 0));
                QApplication::sendEvent(view->viewport(), &hover);
            }
        }

        template<typename View>
        void expectSameView(ItemViewElementTests::Twins<View>& twins, const char* kind, Failures& failures)
        {
            for (const int hoverRow : { -1, 4 })
            {
                for (const bool selectAll : { false, true })
                {
                    prepareView(twins.m_legacyView, qobject_cast<QStandardItemModel*>(twins.m_legacyView->model()), hoverRow, selectAll);
                    prepareView(twins.m_flatView, qobject_cast<QStandardItemModel*>(twins.m_flatView->model()), hoverRow, selectAll);
                    // the views have no focus in a window nobody activated: the colour group of the items is the inactive one in both
                    QImage flat = rendered(twins.m_flat, rgb(0x444444));
                    QImage legacy = rendered(twins.m_legacy, rgb(0x444444));
                    // The corner of a table where its headers meet is black in the Native GUI (the Style of AzQtComponents draws it so, user
                    // verdict of 2026-09-24), not the legacy twin's: it is not what the rules of the item views say
                    if (auto table = qobject_cast<QTableView*>(twins.m_flatView))
                    {
                        const QRect corner(0, 0, table->verticalHeader()->width(), table->horizontalHeader()->height());
                        QPainter(&flat).fillRect(corner, Qt::black);
                        QPainter(&legacy).fillRect(corner, Qt::black);
                    }
                    failures.check(std::string(kind) + (hoverRow >= 0 ? " hovered" : "") + (selectAll ? " more selected" : ""), flat, legacy);
                }
            }
        }

        TEST_F(ItemViewElementTests, Views_ARenderedViewIsTheSheetsRowsAndBranchesAndHeader)
        {
            Failures failures;
            Twins<QTreeView> tree(*this, QString(), Qt::LeftToRight);
            expectSameView(tree, "QTreeView", failures);
            Twins<TableView> tableView(*this, QString(), Qt::LeftToRight);
            expectSameView(tableView, "TableView", failures);
            Twins<QTableView> table(*this, QString(), Qt::LeftToRight);
            expectSameView(table, "QTableView", failures);
            Twins<QListView> list(*this, QString(), Qt::LeftToRight);
            expectSameView(list, "QListView", failures);
        }

        // ----------------------------------------------------------------------------------
        // T4: the branch cell of a row: its fill, and the caret fitted into it less the padding of the rule
        // ----------------------------------------------------------------------------------

        struct BranchCase
        {
            const char* m_name;
            QStyle::State m_state;
        };

        const QStyle::State StateChildren = QStyle::State_Children;
        const QStyle::State StateOpen = QStyle::State_Open;
        const QStyle::State StateSibling = QStyle::State_Sibling;
        const QStyle::State StateItem = QStyle::State_Item;
        const BranchCase BranchCases[] = {
            { "leaf", StateEnabled | StateActive | StateItem },
            { "closed", StateEnabled | StateActive | StateItem | StateChildren },
            { "open", StateEnabled | StateActive | StateItem | StateChildren | StateOpen },
            { "closed with siblings", StateEnabled | StateActive | StateItem | StateChildren | StateSibling },
            { "open with siblings", StateEnabled | StateActive | StateItem | StateChildren | StateOpen | StateSibling },
            { "closed and selected", StateEnabled | StateActive | StateItem | StateChildren | QStyle::State_Selected },
            { "open and selected", StateEnabled | StateActive | StateItem | StateChildren | StateOpen | QStyle::State_Selected },
            { "closed and hovered", StateEnabled | StateActive | StateItem | StateChildren | QStyle::State_MouseOver },
            { "open and hovered", StateEnabled | StateActive | StateItem | StateChildren | StateOpen | QStyle::State_MouseOver },
            { "closed, selected and hovered", StateEnabled | StateActive | StateItem | StateChildren | QStyle::State_Selected | QStyle::State_MouseOver },
            { "leaf, selected", StateEnabled | StateActive | StateItem | QStyle::State_Selected },
            { "leaf, hovered", StateEnabled | StateActive | StateItem | QStyle::State_MouseOver },
            { "an outer branch", StateEnabled | StateActive },
            { "an outer branch, selected", StateEnabled | StateActive | QStyle::State_Selected },
            { "closed in an inactive window, selected", StateEnabled | StateItem | StateChildren | QStyle::State_Selected },
            { "closed and disabled", StateActive | StateItem | StateChildren },
        };

        template<typename View>
        void expectSameBranches(const ItemViewElementTests::Twins<View>& twins, const char* kind, Failures& failures)
        {
            // The cells a tree asks for: as wide as its indentation and as high as the row (the outliner's rows are 23 and 26 high, a TableView's 24)
            const QSize cells[] = { QSize(20, 24), QSize(24, 23), QSize(24, 26), QSize(20, 20), QSize(16, 40), QSize(30, 30), QSize(10, 24), QSize(20, 12) };
            for (const qreal ratio : Ratios)
            {
                for (const QSize& cell : cells)
                {
                    for (const BranchCase& branchCase : BranchCases)
                    {
                        for (const bool alternate : { false, true })
                        {
                            const std::string name = std::string(kind) + " " + branchCase.m_name + " in a cell of " + std::to_string(cell.width()) + "x" + std::to_string(cell.height())
                                + (alternate ? " alternate" : "") + " at ratio " + std::to_string(ratio).substr(0, 3);
                            const auto paint = [&](const ExposedView<View>* view)
                            {
                                return drawn(QSize(60, 60), ratio, [&](QPainter* painter)
                                {
                                    QStyleOptionViewItem option = view->viewOption();
                                    option.state = branchCase.m_state;
                                    option.rect = QRect(QPoint(9, 7), cell);
                                    option.showDecorationSelected = true;
                                    if (alternate)
                                    {
                                        option.features |= QStyleOptionViewItem::Alternate;
                                    }
                                    option.palette.setCurrentColorGroup((branchCase.m_state & QStyle::State_Enabled) ? ((branchCase.m_state & QStyle::State_Active) ? QPalette::Active : QPalette::Inactive) : QPalette::Disabled);
                                    view->style()->drawPrimitive(QStyle::PE_IndicatorBranch, &option, painter, view);
                                });
                            };
                            failures.check(name, paint(twins.m_flatView), paint(twins.m_legacyView));
                        }
                    }
                }
            }
        }

        TEST_F(ItemViewElementTests, Branches_TheCellHasTheFillOfItsStateAndTheCaretFittedIntoItLessThePaddingOfTheRule)
        {
            Failures failures;
            Twins<QTreeView> tree(*this, QString(), Qt::LeftToRight);
            expectSameBranches(tree, "QTreeView", failures);
            Twins<TableView> tableView(*this, QString(), Qt::LeftToRight);
            expectSameBranches(tableView, "TableView", failures);
        }

        //! The Open Sans the application sets as its font, 12 px (StyleManager::initializeFonts, StyleManager::initialize), as the Native captures had it: the
        //! application font while it lives, the fonts out of the font database after it
        struct OpenSansFonts
        {
            OpenSansFonts()
            {
                for (const char* name : { "OpenSans-Regular.ttf", "OpenSans-Bold.ttf", "OpenSans-Semibold.ttf", "OpenSans-Light.ttf" })
                {
                    m_ids.push_back(QFontDatabase::addApplicationFont(QStringLiteral(":/AzQtFonts/Fonts/Open_Sans/%1").arg(QString::fromLatin1(name))));
                }
                QFont font(QStringLiteral("Open Sans"));
                font.setPixelSize(12);
                QApplication::setFont(font);
            }

            ~OpenSansFonts()
            {
                for (const int id : m_ids)
                {
                    QFontDatabase::removeApplicationFont(id);
                }
            }

            std::vector<int> m_ids;
        };

        //! A pixel of the Native capture: its place in the cell and its colour
        struct NativePixel
        {
            int m_x;
            int m_y;
            int m_rgb;
        };

        TEST_F(ItemViewElementTests, Branches_TheClosedCaretOfAnOutlinerRowIsThePixelsOfTheNativeCapture)
        {
            // n2 Editor 01_main_window, the row "Atom Default Environment" of the entity outliner (m_objectTree: indentation 20, rows of 23 px under the
            // 26 px row of the level, a branch cell of 20 x 23), device pixels at ratio 1.5, the caret over #444444. The row is at the logical y 26 of the
            // tree, which is at the logical y 1057 of its window: the cell is at an odd logical y, half a device pixel (the translation below). The caret
            // is the svg (4 x 8) fitted into the cell less the padding of the rule (8px 6px): 3 x 7 logical, where the flattened style of before drew it
            // at 4 x 8 (12 device pixels high instead of the 10 of the capture; TableView.qss:87-91)
            static const NativePixel native[] = {
                { 14, 14, 0x999999 },
                { 14, 15, 0xFFFFFF }, { 15, 15, 0x898989 },
                { 14, 16, 0xFFFFFF }, { 15, 16, 0xFDFDFD },
                { 14, 17, 0xFFFFFF }, { 15, 17, 0xFFFFFF }, { 16, 17, 0x6E6E6E },
                { 14, 18, 0xFFFFFF }, { 15, 18, 0xFFFFFF }, { 16, 18, 0xF3F3F3 }, { 17, 18, 0x636363 },
                { 14, 19, 0xFFFFFF }, { 15, 19, 0xFFFFFF }, { 16, 19, 0xF2F2F2 }, { 17, 19, 0x636363 },
                { 14, 20, 0xFFFFFF }, { 15, 20, 0xFFFFFF }, { 16, 20, 0x6E6E6E },
                { 14, 21, 0xFFFFFF }, { 15, 21, 0xFDFDFD },
                { 14, 22, 0xFFFFFF }, { 15, 22, 0x898989 },
                { 14, 23, 0x999999 },
            };
            Twins<QTreeView> twins(*this, QString(), Qt::LeftToRight);
            const qreal ratio = 1.5;
            const QImage image = drawn(QSize(20, 25), ratio, [&](QPainter* painter)
            {
                painter->fillRect(QRect(0, 0, 20, 25), rgb(0x444444));
                painter->translate(0, 1);
                QStyleOptionViewItem option = twins.m_flatView->viewOption();
                option.state = QStyle::State_Enabled | QStyle::State_Active | QStyle::State_Item | QStyle::State_Children;
                option.rect = QRect(0, 0, 20, 23);
                twins.m_flatView->style()->drawPrimitive(QStyle::PE_IndicatorBranch, &option, painter, twins.m_flatView);
            });

            // every pixel of the caret is the capture's (a channel at most 2 off: the rasterizer of a platform), every other pixel of the cell is the background
            const auto channelsAreClose = [](const QColor& a, const QColor& b)
            {
                return qAbs(a.red() - b.red()) <= 2 && qAbs(a.green() - b.green()) <= 2 && qAbs(a.blue() - b.blue()) <= 2;
            };
            for (const NativePixel& pixel : native)
            {
                const QColor flat = QColor::fromRgba(image.pixel(pixel.m_x, pixel.m_y));
                EXPECT_TRUE(channelsAreClose(flat, rgb(pixel.m_rgb))) << "(" << pixel.m_x << "," << pixel.m_y << ") " << flat.name().toStdString() << " but the capture has "
                                                                      << rgb(pixel.m_rgb).name().toStdString();
            }
            int others = 0;
            for (int y = 0; y < image.height(); ++y)
            {
                for (int x = 0; x < image.width(); ++x)
                {
                    const bool listed = std::any_of(std::begin(native), std::end(native), [&](const NativePixel& pixel) { return pixel.m_x == x && pixel.m_y == y; });
                    others += (!listed && QColor::fromRgba(image.pixel(x, y)) != rgb(0x444444)) ? 1 : 0;
                }
            }
            EXPECT_EQ(others, 0) << "pixels of the cell that the capture has as the background";
        }

        //! The pixels of the connecting lines of a tree with a BranchDelegate in the cell of a branch the flattened style draws
        int linePixelsOf(const ItemViewElementTests& tests, QTreeView* tree, QStyle::State state)
        {
            const QImage image = drawn(QSize(20, 24), 1.0, [&](QPainter* painter)
            {
                QStyleOptionViewItem option;
                option.initFrom(tree);
                option.widget = tree;
                option.state = state;
                option.rect = QRect(0, 0, 20, 24);
                tests.style()->drawPrimitive(QStyle::PE_IndicatorBranch, &option, painter, tree);
            });
            return countOf(image, QColor(QStringLiteral("#878787")));      // TreeView::defaultConfig().branchLineColor
        }

        TEST_F(ItemViewElementTests, Branches_ATreeWithABranchDelegateKeepsItsLinesWhereTheRuleDrawsNothing)
        {
            // The sheet style drew a branch from its rule where the rule had a fill or an image, and asked the base style everywhere else
            // (qstylesheetstyle.cpp:4777-4790): the base style of AzQtComponents draws the lines of a tree with a BranchDelegate
            QWidget root;
            root.setProperty("itemViewElementTestsFlat", true);
            root.setStyle(style());
            auto* tree = new QTreeView(&root);
            tree->setStyle(style());
            auto* delegate = new BranchDelegate();
            delegate->setParent(tree);
            tree->setItemDelegate(delegate);
            root.setAttribute(Qt::WA_DontShowOnScreen);
            root.show();
            QApplication::processEvents();
            ASSERT_TRUE(ItemViewElement::forWidget(tree) != nullptr);

            const QStyle::State leaf = StateEnabled | StateActive | StateItem | StateSibling;
            const QStyle::State outer = StateEnabled | StateActive | StateSibling;
            const QStyle::State closed = StateEnabled | StateActive | StateItem | StateSibling | StateChildren;
            const QStyle::State hovered = leaf | QStyle::State_MouseOver;

            TreeView::setBranchLinesEnabled(tree, true);
            EXPECT_GT(linePixelsOf(*this, tree, leaf), 20);             // the line down the cell and the one to the item
            EXPECT_GT(linePixelsOf(*this, tree, outer), 20);            // the line down the cell
            EXPECT_EQ(linePixelsOf(*this, tree, closed), 0);            // the rule draws its caret
            EXPECT_EQ(linePixelsOf(*this, tree, hovered), 0);           // the rule draws its fill

            TreeView::setBranchLinesEnabled(tree, false);
            EXPECT_EQ(linePixelsOf(*this, tree, leaf), 0);

            // a tree of another kind has no lines to keep
            auto* plain = new QTreeView(&root);
            plain->setStyle(style());
            plain->show();
            QApplication::processEvents();
            EXPECT_EQ(linePixelsOf(*this, plain, leaf), 0);
        }
        // ----------------------------------------------------------------------------------
        // T7: what a module's element says of its views: the rules of the module's own sheets against an element that lists only what they change
        // ----------------------------------------------------------------------------------

        //! A list has no header
        template<typename View>
        void expectSameHeadersIfAny(ItemViewElementTests::Twins<View>& twins, const char* label, Failures& failures)
        {
            if constexpr (std::is_base_of_v<QTreeView, View> || std::is_base_of_v<QTableView, View>)
            {
                expectSameHeaders(twins, label, failures);
            }
        }

        template<typename View>
        void expectModuleView(ItemViewElementTests::Twins<View>& twins, const char* label, const char* elementName, Failures& failures)
        {
            ASSERT_TRUE(ItemViewElement::forWidget(twins.m_flatView) != nullptr) << label;
            EXPECT_STREQ(ItemViewElement::forWidget(twins.m_flatView)->m_name, elementName) << label;
            expectSameItemPaint(twins, label, failures);
            expectSameRowPanel(twins, label, failures);
            expectSameHeadersIfAny(twins, label, failures);
        }

        TEST_F(ItemViewElementTests, Modules_TheAssetProcessorsTablesListsAndTreesAreTheirSheetsRules)
        {
            Failures failures;
            {
                Twins<QTableView> twins(*this, QString::fromUtf8(ApRules), Qt::LeftToRight, QSize(320, 240), "apTable");
                expectModuleView(twins, "AssetProcessor table", "Test.ItemView.ApTable", failures);
            }
            {
                Twins<TableView> twins(*this, QString::fromUtf8(ApRules), Qt::LeftToRight, QSize(320, 240), "apTableView");
                expectModuleView(twins, "AssetProcessor TableView", "Test.ItemView.ApTable", failures);
                expectSameBranches(twins, "AssetProcessor TableView", failures);
            }
            {
                // the builder list: the roles of AssetsTab.qss:86-91, and the items of TableView.qss
                Twins<QListView> twins(*this, QString::fromUtf8(ApRules), Qt::LeftToRight, QSize(320, 240), "builderList");
                expectModuleView(twins, "AssetProcessor builder list", "Test.ItemView.ApBuilderList", failures);
            }
            {
                Twins<QTreeView> twins(*this, QString::fromUtf8(ApRules), Qt::LeftToRight, QSize(320, 240), "SourceAssetsTreeView");
                expectModuleView(twins, "AssetProcessor asset tree", "Test.ItemView.ApTree", failures);
                expectSameBranches(twins, "AssetProcessor asset tree", failures);
            }
        }

        TEST_F(ItemViewElementTests, Modules_TheHeadersOfTheGemListAndTheBundlerAreTheirSectionRulesFill)
        {
            Failures failures;
            {
                // ProjectManager.qss:872-874: transparent sections, and nothing beyond them
                Twins<QTableView> twins(*this, QString::fromUtf8(PmRules), Qt::LeftToRight, QSize(320, 240), "adjustableHeaderWidget");
                expectSameHeaders(twins, "Project Manager gem list", failures, rgb(0xFF00FF));
                // the window behind shows: the header over another colour is that colour
                const QImage flat = rendered(*twins.m_flatView->horizontalHeader(), rgb(0x336699));
                EXPECT_GT(countOf(flat, rgb(0x336699)), flat.width() * flat.height() / 2);
            }
            {
                Twins<TableView> twins(*this, QString::fromUtf8(AbLogRules), Qt::LeftToRight, QSize(320, 240), "logTableView");
                expectSameHeaders(twins, "Asset Bundler log", failures);
            }
            {
                Twins<TableView> twins(*this, QString::fromUtf8(AbTabRules), Qt::LeftToRight, QSize(320, 240), "abFileTable");
                expectSameHeaders(twins, "Asset Bundler file table", failures);
            }
        }

        TEST_F(ItemViewElementTests, Modules_TheOutlinerHasNoSelectionOrHoverOfItsOwn)
        {
            Failures failures;
            Twins<QTreeView> twins(*this, QString::fromUtf8(OutlinerRules), Qt::LeftToRight, QSize(320, 240), "m_objectTree");
            ASSERT_TRUE(ItemViewElement::forWidget(twins.m_flatView) != nullptr);
            EXPECT_STREQ(ItemViewElement::forWidget(twins.m_flatView)->m_name, "Test.ItemView.OutlinerTree");
            expectSameItemPaint(twins, "Outliner tree", failures);
            expectSameRowPanel(twins, "Outliner tree", failures);
            expectSameBranches(twins, "Outliner tree", failures);
        }

        TEST_F(ItemViewElementTests, Modules_AViewPolishedBeforeTheRegistrationIsResolvedAgain)
        {
            // A module's registrar runs when its window gets its sheet (StyleManager::setStyleSheet), after the window built, and polished, its views
            static const ItemViewElement Late{
                .m_name = "Test.ItemView.Late",
                .m_alternate = rgb(0x123456),
                .m_headerFill = rgb(0x654321),
            };
            QWidget root;
            root.setProperty("itemViewElementTestsFlat", true);
            root.setStyle(style());
            auto* tree = new QTreeView(&root);
            tree->setObjectName(QStringLiteral("lateTree"));
            tree->setStyle(style());
            tree->setModel(makeModel(&root, true));
            tree->setGeometry(0, 0, 200, 100);
            root.resize(200, 100);
            root.setAttribute(Qt::WA_DontShowOnScreen);
            root.show();
            QApplication::processEvents();
            ASSERT_TRUE(ItemViewElement::forWidget(tree) != nullptr);
            EXPECT_STREQ(ItemViewElement::forWidget(tree)->m_name, "Global.ItemView.Tree");
            EXPECT_EQ(tree->palette().color(QPalette::AlternateBase), rgb(0x4D4D4D));
            EXPECT_EQ(tree->header()->palette().color(QPalette::Button), rgb(0x2D2D2D));

            ItemViewElement::registerElement(&Late, &QTreeView::staticMetaObject, [](const QWidget* w) { return w->objectName() == QLatin1String("lateTree"); });

            ASSERT_TRUE(ItemViewElement::forWidget(tree) != nullptr);
            EXPECT_STREQ(ItemViewElement::forWidget(tree)->m_name, "Test.ItemView.Late");
            EXPECT_EQ(tree->palette().color(QPalette::AlternateBase), rgb(0x123456));
            EXPECT_EQ(tree->header()->palette().color(QPalette::Button), rgb(0x654321));
            EXPECT_EQ(ItemViewElement::headerFillOf(tree->header()), rgb(0x654321));
            // what the module's element leaves out is the Global element's
            EXPECT_EQ(tree->palette().color(QPalette::Highlight), rgb(0x656565));
            EXPECT_EQ(tree->indentation(), 20);
        }
        // ----------------------------------------------------------------------------------
        // T8: what polishing a view gives it: the roles of its palette, the pointer tracking of its rules, the indentation a tree has when it
        // is shown, and the frame of the kinds of views the legacy rules did not name
        // ----------------------------------------------------------------------------------

        template<typename View>
        void expectSameRolesAndTracking(ItemViewElementTests::Twins<View>& twins, const char* label)
        {
            for (const auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
            {
                for (const auto role : { QPalette::AlternateBase, QPalette::Highlight, QPalette::HighlightedText })
                {
                    EXPECT_EQ(twins.m_flatView->palette().color(group, role), twins.m_legacyView->palette().color(group, role))
                        << label << " group " << int(group) << " role " << int(role);
                }
            }
            // the sheet style tracked the pointer of a view that has an :hover rule, in the view and in its viewport
            EXPECT_EQ(twins.m_flatView->testAttribute(Qt::WA_Hover), twins.m_legacyView->testAttribute(Qt::WA_Hover)) << label;
            EXPECT_EQ(twins.m_flatView->viewport()->testAttribute(Qt::WA_Hover), twins.m_legacyView->viewport()->testAttribute(Qt::WA_Hover)) << label;
            EXPECT_EQ(twins.m_flatView->viewport()->hasMouseTracking(), twins.m_legacyView->viewport()->hasMouseTracking()) << label;
            EXPECT_TRUE(twins.m_legacyView->viewport()->hasMouseTracking()) << label;
        }

        TEST_F(ItemViewElementTests, Polish_TheViewHasTheRolesOfItsRulesAndTracksThePointer)
        {
            {
                Twins<QTreeView> twins(*this, QString(), Qt::LeftToRight);
                expectSameRolesAndTracking(twins, "QTreeView");
            }
            {
                Twins<TableView> twins(*this, QString(), Qt::LeftToRight);
                expectSameRolesAndTracking(twins, "TableView");
            }
            {
                Twins<QTableView> twins(*this, QString(), Qt::LeftToRight);
                expectSameRolesAndTracking(twins, "QTableView");
            }
            {
                Twins<QListView> twins(*this, QString(), Qt::LeftToRight);
                expectSameRolesAndTracking(twins, "QListView");
            }
            {
                Twins<QTableView> twins(*this, QString::fromUtf8(ApRules), Qt::LeftToRight, QSize(320, 240), "apTable");
                expectSameRolesAndTracking(twins, "AssetProcessor table");
            }
            {
                Twins<QListView> twins(*this, QString::fromUtf8(ApRules), Qt::LeftToRight, QSize(320, 240), "builderList");
                expectSameRolesAndTracking(twins, "AssetProcessor builder list");
            }
            {
                Twins<QTreeView> twins(*this, QString::fromUtf8(OutlinerRules), Qt::LeftToRight, QSize(320, 240), "m_objectTree");
                expectSameRolesAndTracking(twins, "Outliner tree");
            }
        }

        //! A flat tree whose owner gave it an indentation after it was polished and before it was shown (the outliner sets 24 as it is built, and
        //! the legacy sheet style polished it again, with TreeView::polish, when it was put into a window), the way the legacy twin lived it
        int indentationOfAFlatTreeShown(const ItemViewElementTests& tests, const char* name, int ownersIndentation)
        {
            QWidget root;
            root.setProperty("itemViewElementTestsFlat", true);
            root.setStyle(tests.style());
            auto* tree = new QTreeView(&root);
            tree->setObjectName(QString::fromLatin1(name));
            tree->setStyle(tests.style());
            tree->setModel(makeModel(&root, true));
            tree->ensurePolished();
            tree->setIndentation(ownersIndentation);
            root.setGeometry(0, 0, 200, 100);
            tree->setGeometry(0, 0, 200, 100);
            root.setAttribute(Qt::WA_DontShowOnScreen);
            root.show();
            QApplication::processEvents();
            const int shown = tree->indentation();
            tree->setIndentation(7);        // what the owner does with it once it is shown is its own
            QApplication::processEvents();
            root.hide();
            root.show();
            QApplication::processEvents();
            EXPECT_EQ(tree->indentation(), 7) << name;
            return shown;
        }

        TEST_F(ItemViewElementTests, Polish_ATreeHasTheIndentationOfItsElementWhenItIsShown)
        {
            // the legacy twin: the sheet is put on the window after the owner built the tree, and the style's polish sets the indentation
            {
                Twins<QTreeView> twins(*this, QString(), Qt::LeftToRight);
                twins.m_legacyView->setIndentation(24);
                twins.m_legacy.setStyleSheet(appSheet());
                EXPECT_EQ(twins.m_legacyView->indentation(), 20);
            }
            EXPECT_EQ(indentationOfAFlatTreeShown(*this, "view", 24), 20);
            // a tree with an element of its own: the filter trees of the Asset Processor have none (AssetsTab.qss:75)
            {
                Twins<QTreeView> twins(*this, QString::fromUtf8(ApFilterRules), Qt::LeftToRight, QSize(320, 240), "apFilterTree");
                EXPECT_EQ(twins.m_legacyView->indentation(), 0);
                EXPECT_EQ(twins.m_flatView->indentation(), 0);
            }
            EXPECT_EQ(indentationOfAFlatTreeShown(*this, "apFilterTree", 24), 0);
            // a TableView has no indentation of its own
            {
                QWidget root;
                root.setProperty("itemViewElementTestsFlat", true);
                root.setStyle(style());
                auto* table = new TableView(&root);
                table->setStyle(style());
                table->setModel(makeModel(&root, false));
                table->setIndentation(2);
                root.setAttribute(Qt::WA_DontShowOnScreen);
                root.show();
                QApplication::processEvents();
                EXPECT_EQ(table->indentation(), 2);
            }
        }

        //! Counts what the tree watcher of TreeView.cpp does with a tree. On a change of any dynamic property whose name does not begin with "_q_" or
        //! "_o3de_" it asks StyleManager::styleSheetStyle for the style to unpolish and polish the tree with. This environment has no StyleManager
        //! instance: the call logs the warning counted here, and the unpolish and polish do not follow (in the product the flattened Style is the
        //! answer, and they do: the tree would be polished again when it is first shown)
        class TreeWatcherCalls
            : public AZ::Debug::TraceMessageBus::Handler
        {
        public:
            TreeWatcherCalls()
            {
                BusConnect();
            }

            ~TreeWatcherCalls() override
            {
                BusDisconnect();
            }

            bool OnWarning(const char* window, const char* message) override
            {
                if (isTheWatchersWarning(window, message))
                {
                    ++m_calls;
                    return true;
                }
                return false;
            }

            bool OnPrintf(const char* window, const char* message) override
            {
                return isTheWatchersWarning(window, message);       // the warning is printed as well: only its handler counts
            }

            int m_calls = 0;

        private:
            static bool isTheWatchersWarning(const char* window, const char* message)
            {
                return QLatin1String(window) == QLatin1String("StyleManager") && QString::fromUtf8(message).contains(QLatin1String("styleSheetStyle"));
            }
        };

        TEST_F(ItemViewElementTests, Polish_TheBookkeepingOfAnElementDoesNotRepolishATreeThroughItsWatcher)
        {
            // The element a tree resolves and the mark of its first show are dynamic properties of the tree (ItemViewElement.cpp); a plain QTreeView has
            // the tree watcher of TreeView.cpp installed by its polish, which unpolishes and polishes it on a change of any property a selector could read
            TreeWatcherCalls watcher;
            QWidget root;
            root.setProperty("itemViewElementTestsFlat", true);
            root.setStyle(style());
            auto* tree = new QTreeView(&root);
            tree->setStyle(style());
            tree->setModel(makeModel(&root, true));
            tree->setGeometry(0, 0, 200, 100);
            root.resize(200, 100);
            tree->ensurePolished();
            ASSERT_TRUE(ItemViewElement::forWidget(tree) != nullptr);
            EXPECT_EQ(watcher.m_calls, 0);

            root.setAttribute(Qt::WA_DontShowOnScreen);
            root.show();
            QApplication::processEvents();
            EXPECT_EQ(watcher.m_calls, 0) << "the first show of the tree";

            // the watcher is there, and answers a property of the application
            tree->setProperty("TreeWatcherProbe", true);
            EXPECT_EQ(watcher.m_calls, 1);
        }

        TEST_F(ItemViewElementTests, Polish_AViewOfAnotherKindKeepsTheFrameOfAQFrame)
        {
            // The rule that took the frame away names QTableView, QListView and QTreeView (TableView.qss:14-29). The thumbnails of the asset folder
            // are a QAbstractItemView of their own: Qt's default sheet gave them the frame of a QFrame, 1px wide, which insets the viewport.
            QWidget legacy;
            auto* legacyView = new AssetFolderThumbnailView(&legacy);
            legacyView->setGeometry(0, 0, 200, 100);
            legacy.setStyleSheet(appSheet());
            legacy.setAttribute(Qt::WA_DontShowOnScreen);
            legacy.resize(220, 120);
            legacy.show();

            QWidget flat;
            flat.setStyle(style());
            auto* flatView = new AssetFolderThumbnailView(&flat);
            flatView->setStyle(style());
            for (QWidget* child : flatView->findChildren<QWidget*>())
            {
                child->setStyle(style());
            }
            flatView->setGeometry(0, 0, 200, 100);
            flat.setAttribute(Qt::WA_DontShowOnScreen);
            flat.resize(220, 120);
            flat.show();

            // (no event is processed: the thumbnails paint themselves from a configuration this process does not load)
            EXPECT_EQ(legacyView->viewport()->geometry(), QRect(1, 1, 198, 98));
            EXPECT_EQ(flatView->viewport()->geometry(), legacyView->viewport()->geometry());
            EXPECT_EQ(flatView->frameWidth(), legacyView->frameWidth());
        }

        TEST_F(ItemViewElementTests, Polish_ThePaletteOfTheColorPickerHasNoFrame)
        {
            // ColorPicker.qss:97 `AzQtComponents--ColorPicker AzQtComponents--PaletteView { border: none }`: the Native dumps of the picker's palettes
            // (n2 Gallery 07_color-picker) have the shape of a panel and contents margins of 0, the size hint of the swatches' row alone (256 x 16)
            // The constructor of a PaletteView reads the clipboard (UnmarshalFromClipboard), which has no mime data on the offscreen platform until
            // something is put on it (without it the constructor ends the process); the clipboard of a real platform is left alone
            if (!QGuiApplication::clipboard()->mimeData())
            {
                QGuiApplication::clipboard()->setText(QString());
            }
            Palette palette;
            Internal::ColorController controller;
            QUndoStack undoStack;
            QWidget root;
            root.setStyle(style());
            auto* flatView = new PaletteView(&palette, &controller, &undoStack, &root);
            flatView->setStyle(style());
            flatView->setGeometry(0, 0, 200, 40);
            root.setAttribute(Qt::WA_DontShowOnScreen);
            root.resize(220, 60);
            root.show();
            QApplication::processEvents();

            EXPECT_EQ(flatView->frameShape(), QFrame::NoFrame);
            EXPECT_EQ(flatView->frameWidth(), 0);
            EXPECT_EQ(flatView->viewport()->geometry(), QRect(0, 0, 200, 40));
        }
        // ----------------------------------------------------------------------------------
        // T9: the colour of a console (ScrollBar::setBackgroundColor). The legacy GUI gave it with an inline sheet, `background: <colour>`, which
        // covered the text, the lanes of the scroll bars and their corner and left the bars to ScrollBar.qss (ConsoleSCB.cpp:471). Flattened, that
        // sheet kept a sheet style on the area and its bars, which drew them as the squared bars of the base style: the colour is the palette of
        // the area now, and the style draws its bars as it draws every other.
        // ----------------------------------------------------------------------------------

        //! The text area of the Editor's console (ConsoleSCB.ui): read only, with more lines than it shows, and wider than it (a long line of its
        //! text: the width of a line is what the platform's font makes of it, so the bar is shown whatever the text)
        QPlainTextEdit* makeConsole(QWidget* root, bool lightTheme)
        {
            auto* console = new QPlainTextEdit(root);
            console->setObjectName(QStringLiteral("textEdit"));
            console->setReadOnly(true);
            console->setLineWrapMode(QPlainTextEdit::NoWrap);
            console->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
            console->setGeometry(0, 0, 240, 120);
            for (int line = 0; line < 60; ++line)
            {
                console->appendPlainText(QStringLiteral("line %1").arg(line));
            }
            if (lightTheme)
            {
                // ScrollBar::applyDarkStyle, which the console calls for its light background
                Style::addClass(console, QStringLiteral("DarkScrollBar"));
            }
            return console;
        }

        //! A console in windows of their own: as the legacy GUI had it (the application sheet and the console's inline sheet), as the flattened GUI has
        //! it (the colour given through ScrollBar), and the plain text area of the flattened GUI, which has no colour of its own to give
        struct ConsoleTwins
        {
            ConsoleTwins(const ItemViewElementTests& tests, const QColor& color, bool lightTheme)
            {
                m_legacyConsole = makeConsole(&m_legacy, lightTheme);
                // the corner widget that the style gives every scroll area of the application (ScrollBar.cpp, ScrollBarWatcher::install), which
                // makes room for itself between the bars
                auto* corner = new QWidget;
                corner->setProperty("ShowBackground", false);
                m_legacyConsole->setCornerWidget(corner);
                m_legacy.setStyleSheet(appSheet());
                m_legacyConsole->setStyleSheet(QString("background: %1").arg(color.name(QColor::HexRgb)));
                show(m_legacy, m_legacyConsole);

                m_flatConsole = makeFlatConsole(tests, m_flat, lightTheme);
                ScrollBar::setBackgroundColor(m_flatConsole, color);
                show(m_flat, m_flatConsole);

                m_plainConsole = makeFlatConsole(tests, m_plain, lightTheme);
                show(m_plain, m_plainConsole);
            }

            static QPlainTextEdit* makeFlatConsole(const ItemViewElementTests& tests, QWidget& root, bool lightTheme)
            {
                QPlainTextEdit* console = makeConsole(&root, lightTheme);
                root.setProperty("itemViewElementTestsFlat", true);
                QPalette palette;
                tests.style()->polish(palette);
                root.setPalette(palette);
                root.setStyle(tests.style());
                for (QWidget* child : root.findChildren<QWidget*>())
                {
                    child->setStyle(tests.style());
                }
                return console;
            }

            //! Shown, with the vertical bar at the middle of its range
            static void show(QWidget& root, QPlainTextEdit* console)
            {
                root.setAttribute(Qt::WA_DontShowOnScreen);
                root.resize(260, 140);
                root.show();
                QApplication::processEvents();

                console->verticalScrollBar()->setValue(console->verticalScrollBar()->maximum() / 2);
                QApplication::processEvents();
            }

            QWidget m_legacy;
            QWidget m_flat;
            QWidget m_plain;
            QPlainTextEdit* m_legacyConsole = nullptr;
            QPlainTextEdit* m_flatConsole = nullptr;
            QPlainTextEdit* m_plainConsole = nullptr;
        };

        const QColor DarkConsole = rgb(0x222222);               // ConsoleSCB.cpp:457
        const QColor LightConsole = QColor(Qt::white);          // ConsoleSCB.cpp:464

        QImage renderedWindow(QWidget& root)
        {
            QImage image(root.size(), QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::magenta);
            root.render(&image);
            return image;
        }

        QRect rectInWindow(const QWidget* widget)
        {
            return QRect(widget->mapTo(widget->window(), QPoint(0, 0)), widget->size());
        }

        //! The handle of a bar in a render: what is in the bar's rect and is not the colour behind it (null: none)
        QRect handleOf(const QImage& image, const QRect& bar, const QColor& behind)
        {
            QRect handle;
            for (int y = bar.top(); y <= bar.bottom(); ++y)
            {
                for (int x = bar.left(); x <= bar.right(); ++x)
                {
                    if (QColor(image.pixel(x, y)) != behind)
                    {
                        handle = handle.united(QRect(x, y, 1, 1));
                    }
                }
            }
            return handle;
        }

        TEST_F(ItemViewElementTests, Console_TheColourIsThePaletteOfTheAreaAndNoSheetStyleDrawsItsScrollBars)
        {
            for (const bool lightTheme : { false, true })
            {
                const QColor color = lightTheme ? LightConsole : DarkConsole;
                ConsoleTwins twins(*this, color, lightTheme);
                QPlainTextEdit* console = twins.m_flatConsole;
                const QStyle* flat = style();
                SCOPED_TRACE(lightTheme ? "light console" : "dark console");

                EXPECT_TRUE(console->styleSheet().isEmpty());
                EXPECT_EQ(console->style(), flat);
                EXPECT_EQ(console->viewport()->style(), flat);
                EXPECT_EQ(console->verticalScrollBar()->style(), flat);
                EXPECT_EQ(console->horizontalScrollBar()->style(), flat);
                EXPECT_TRUE(console->autoFillBackground());
                for (const auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
                {
                    EXPECT_EQ(console->palette().color(group, QPalette::Base), color) << "group " << int(group);
                    EXPECT_EQ(console->palette().color(group, QPalette::Window), color) << "group " << int(group);
                    EXPECT_EQ(console->viewport()->palette().color(group, QPalette::Base), color) << "group " << int(group);
                }

                // the sheet the console had is what put a sheet style on its bars, here as it was before (the twin of the legacy GUI has it too)
                EXPECT_NE(twins.m_legacyConsole->verticalScrollBar()->style(), flat);
                twins.m_plainConsole->setStyleSheet(QString("background: %1").arg(color.name(QColor::HexRgb)));
                EXPECT_NE(twins.m_plainConsole->verticalScrollBar()->style(), flat);
            }
        }

        //! The scroll bar as its widget gives it to its style
        QStyleOptionSlider sliderOptionOf(const QScrollBar* bar)
        {
            QStyleOptionSlider option;
            option.initFrom(bar);
            option.subControls = QStyle::SC_All;
            option.orientation = bar->orientation();
            option.minimum = bar->minimum();
            option.maximum = bar->maximum();
            option.sliderPosition = bar->sliderPosition();
            option.sliderValue = bar->value();
            option.singleStep = bar->singleStep();
            option.pageStep = bar->pageStep();
            option.upsideDown = bar->invertedAppearance();
            if (bar->orientation() == Qt::Horizontal)
            {
                option.state |= QStyle::State_Horizontal;
            }
            return option;
        }

        //! A bar through its own style onto an image of the ratio, over the colour of the area behind it
        QImage drawnBar(const QScrollBar* bar, const QColor& behind, qreal ratio)
        {
            return drawn(bar->size(), ratio, [&](QPainter* painter)
            {
                painter->fillRect(QRect(QPoint(0, 0), bar->size()), behind);
                const QStyleOptionSlider option = sliderOptionOf(bar);
                bar->style()->drawComplexControl(QStyle::CC_ScrollBar, &option, painter, bar);
            });
        }

        //! The colour at the middle of the bar's handle in an image drawn at the ratio
        QColor handleColour(const QScrollBar* bar, const QImage& image, qreal ratio)
        {
            const QStyleOptionSlider option = sliderOptionOf(bar);
            const QPointF middle = QRectF(bar->style()->subControlRect(QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider, bar)).center();
            return QColor(image.pixel(int(middle.x() * ratio), int(middle.y() * ratio)));
        }

        TEST_F(ItemViewElementTests, Console_TheScrollBarsAreTheRegularOnesOverTheColour)
        {
            for (const bool lightTheme : { false, true })
            {
                const QColor color = lightTheme ? LightConsole : DarkConsole;
                ConsoleTwins twins(*this, color, lightTheme);
                SCOPED_TRACE(lightTheme ? "light console" : "dark console");

                // ---- the window as a viewer sees it (ratio 1): the text area, the lanes and the corner are the colour; the bars' handles are
                // rounded, 4px across the 8px lane, and of the colour the sheet of ScrollBar.qss gave them over it
                const QImage legacyImage = renderedWindow(twins.m_legacy);
                const QImage flatImage = renderedWindow(twins.m_flat);
                EXPECT_EQ(QColor(flatImage.pixel(200, 60)), color);
                EXPECT_EQ(QColor(legacyImage.pixel(200, 60)), color);
                ASSERT_TRUE(twins.m_flatConsole->horizontalScrollBar()->isVisible());
                ASSERT_TRUE(twins.m_legacyConsole->horizontalScrollBar()->isVisible());
                const QRect corner(232, 112, 8, 8);
                EXPECT_EQ(rectInWindow(twins.m_flatConsole->verticalScrollBar()).bottomRight() + QPoint(0, 1), corner.topRight());
                EXPECT_EQ(QColor(flatImage.pixel(corner.center().x(), corner.center().y())), color);
                EXPECT_EQ(QColor(legacyImage.pixel(corner.center().x(), corner.center().y())), color);

                for (const Qt::Orientation orientation : { Qt::Vertical, Qt::Horizontal })
                {
                    SCOPED_TRACE(orientation == Qt::Vertical ? "vertical bar" : "horizontal bar");
                    const auto bar = [&](QPlainTextEdit* console)
                    {
                        return orientation == Qt::Vertical ? console->verticalScrollBar() : console->horizontalScrollBar();
                    };
                    const QRect flatBar = rectInWindow(bar(twins.m_flatConsole));
                    const QRect legacyBar = rectInWindow(bar(twins.m_legacyConsole));
                    EXPECT_EQ(flatBar, legacyBar);
                    const QRect flatHandle = handleOf(flatImage, flatBar, color);
                    const QRect legacyHandle = handleOf(legacyImage, legacyBar, color);
                    ASSERT_FALSE(flatHandle.isNull());
                    ASSERT_FALSE(legacyHandle.isNull());

                    const bool vertical = orientation == Qt::Vertical;
                    EXPECT_EQ(vertical ? flatHandle.width() : flatHandle.height(), vertical ? legacyHandle.width() : legacyHandle.height());
                    EXPECT_EQ(vertical ? flatHandle.left() - flatBar.left() : flatHandle.top() - flatBar.top(),
                        vertical ? legacyHandle.left() - legacyBar.left() : legacyHandle.top() - legacyBar.top());
                    EXPECT_EQ(QColor(flatImage.pixel(flatHandle.center().x(), flatHandle.center().y())),
                        QColor(legacyImage.pixel(legacyHandle.center().x(), legacyHandle.center().y())));
                    EXPECT_NE(QColor(flatImage.pixel(flatHandle.left(), flatHandle.top())),
                        QColor(flatImage.pixel(flatHandle.center().x(), flatHandle.center().y()))) << "the handle's corner is rounded";

                    // ---- the bar through the style onto an image of each ratio: the same bar the plain text area of the flattened GUI has, and
                    // the handle of the legacy bar's colour
                    for (const qreal ratio : Ratios)
                    {
                        const QImage consoleBar = drawnBar(bar(twins.m_flatConsole), color, ratio);
                        const QImage plainBar = drawnBar(bar(twins.m_plainConsole), color, ratio);
                        const QImage legacyBarImage = drawnBar(bar(twins.m_legacyConsole), color, ratio);
                        EXPECT_EQ(differing(consoleBar, plainBar), 0) << "ratio " << ratio << ": " << firstDifference(consoleBar, plainBar).toStdString();
                        EXPECT_EQ(handleColour(bar(twins.m_flatConsole), consoleBar, ratio), handleColour(bar(twins.m_legacyConsole), legacyBarImage, ratio)) << "ratio " << ratio;
                    }
                }
            }
        }

        TEST_F(ItemViewElementTests, Console_TheColourSurvivesPolishingAgainAndFollowsANewColour)
        {
            ConsoleTwins twins(*this, DarkConsole, false);
            QPlainTextEdit* console = twins.m_flatConsole;

            // a class tag polishes the area again (the console does that when its theme changes), and the field palette of a text area is set at every polish
            Style::addClass(console, QStringLiteral("DarkScrollBar"));
            EXPECT_EQ(console->palette().color(QPalette::Base), DarkConsole);
            EXPECT_EQ(console->palette().color(QPalette::Window), DarkConsole);
            Style::removeClass(console, QStringLiteral("DarkScrollBar"));
            EXPECT_EQ(console->palette().color(QPalette::Base), DarkConsole);

            ScrollBar::setBackgroundColor(console, LightConsole);
            EXPECT_EQ(console->palette().color(QPalette::Base), LightConsole);
            EXPECT_EQ(console->palette().color(QPalette::Window), LightConsole);
            console->ensurePolished();
            Style::addClass(console, QStringLiteral("DarkScrollBar"));
            EXPECT_EQ(console->palette().color(QPalette::Base), LightConsole);
            EXPECT_EQ(console->palette().color(QPalette::Window), LightConsole);
        }

        TEST_F(ItemViewElementTests, Console_ATextAreaWithoutAColourKeepsTheFieldOfTheSheet)
        {
            ConsoleTwins twins(*this, DarkConsole, false);

            // BaseStyleSheet.qss's field of a text area: #E9E9E9 (the style's own, applyFlattenedTextEntryPalette)
            EXPECT_EQ(twins.m_plainConsole->palette().color(QPalette::Base), rgb(0xE9E9E9));
            EXPECT_EQ(twins.m_plainConsole->palette().color(QPalette::Window), rgb(0xE9E9E9));

            // any scroll area has the API, one that has no field of the sheet to start from too: it fills itself with the colour
            QWidget root;
            root.setProperty("itemViewElementTestsFlat", true);
            root.setStyle(style());
            auto* area = new QScrollArea(&root);
            area->setStyle(style());
            ASSERT_FALSE(area->autoFillBackground());
            ScrollBar::setBackgroundColor(area, DarkConsole);
            root.setAttribute(Qt::WA_DontShowOnScreen);
            root.show();
            QApplication::processEvents();
            EXPECT_EQ(area->palette().color(QPalette::Base), DarkConsole);
            EXPECT_EQ(area->palette().color(QPalette::Window), DarkConsole);
            EXPECT_TRUE(area->autoFillBackground());
            EXPECT_TRUE(area->styleSheet().isEmpty());

            ScrollBar::setBackgroundColor(nullptr, DarkConsole);        // no area: nothing to colour
        }
        // ----------------------------------------------------------------------------------
        // T10: the Asset Browser Folder of the Gallery (AssetBrowserFolderWidget.cpp:40-81): the toolbar with the back and forward buttons and the
        // breadcrumbs, and the combo box without a frame in the row of the view buttons. The path edit of the breadcrumbs made the flattened
        // GUI's toolbar (and so the page under it) 6px taller than the legacy page's (Native dumps n2 Gallery 00_assetbrowserfolder: breadcrumbs
        // 19, toolbar 29; flattened 25 and 35), and the combo box made it 2px narrower (68 and 66). The tool buttons and the toolbar around them
        // are sized by components that the twin's base style does not have, so what is compared is what the sheet sizes: the breadcrumbs and the combos.
        // ----------------------------------------------------------------------------------

        //! The parts of the folder widget that the sizes of its page come from
        struct FolderBar
        {
            BreadCrumbs* m_path = nullptr;
            QComboBox* m_sizes = nullptr;               // AssetBrowserFolderWidget.cpp:77-81: no frame
            QComboBox* m_framed = nullptr;              // a combo box with its frame, as every other
        };

        FolderBar buildFolderBar(QWidget* root)
        {
            FolderBar bar;
            auto* layout = new QVBoxLayout(root);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(0);
            bar.m_path = new BreadCrumbs(root);
            bar.m_path->setCurrentPath(QStringLiteral("D:/OffLocalDev/CIPipelineTest"));
            layout->addWidget(bar.m_path, 1);

            auto* row = new QHBoxLayout;
            layout->addLayout(row);
            row->addStretch(1);
            for (QComboBox** combo : { &bar.m_sizes, &bar.m_framed })
            {
                *combo = new QComboBox(root);
                (*combo)->addItems({ QStringLiteral("Small"), QStringLiteral("Medium"), QStringLiteral("Large") });
                (*combo)->setFrame(combo == &bar.m_framed);
                (*combo)->setCurrentIndex(1);
                row->addWidget(*combo);
            }
            layout->addStretch(1);
            return bar;
        }

        struct FolderTwins
        {
            explicit FolderTwins(const ItemViewElementTests& tests)
            {
                m_legacyBar = buildFolderBar(&m_legacy);
                m_legacy.setStyleSheet(appSheet());
                show(m_legacy);

                m_flatBar = buildFolderBar(&m_flat);
                m_flat.setProperty("itemViewElementTestsFlat", true);
                QPalette palette;
                tests.style()->polish(palette);
                m_flat.setPalette(palette);
                m_flat.setStyle(tests.style());
                for (QWidget* child : m_flat.findChildren<QWidget*>())
                {
                    child->setStyle(tests.style());
                }

                // The font is not what the tests prove: each flat widget that shows a text has the one the sheet gave its twin, before it is
                // shown (a combo box keeps the minimum size hint it computed with the font it had)
                const auto copyFont = [](QWidget* flat, const QWidget* legacy)
                {
                    ASSERT_TRUE(flat && legacy);
                    flat->setFont(legacy->font());
                };
                copyFont(m_flatBar.m_path, m_legacyBar.m_path);
                copyFont(m_flatBar.m_path->findChild<QLabel*>(), m_legacyBar.m_path->findChild<QLabel*>());
                copyFont(m_flatBar.m_path->findChild<QLineEdit*>(), m_legacyBar.m_path->findChild<QLineEdit*>());
                copyFont(m_flatBar.m_sizes, m_legacyBar.m_sizes);
                copyFont(m_flatBar.m_framed, m_legacyBar.m_framed);
                show(m_flat);
            }

            //! Shown as wide as the page of the Gallery is, so that the whole of the path fits
            static void show(QWidget& root)
            {
                root.setAttribute(Qt::WA_DontShowOnScreen);
                root.resize(1100, 200);
                root.show();
                QApplication::processEvents();
            }

            QWidget m_legacy;
            QWidget m_flat;
            FolderBar m_legacyBar;
            FolderBar m_flatBar;
        };

        TEST_F(ItemViewElementTests, Folder_ThePathOfTheBreadcrumbsAndTheCombosAreTheLegacySizes)
        {
            FolderTwins twins(*this);
            const FolderBar& legacy = twins.m_legacyBar;
            const FolderBar& flat = twins.m_flatBar;

            // the path edit (LineEdit.qss:13, BreadCrumbs.qss:26-31: 16 high), the stack of the breadcrumbs that holds it beside the label, and the breadcrumbs
            const auto* legacyEdit = legacy.m_path->findChild<QLineEdit*>();
            const auto* flatEdit = flat.m_path->findChild<QLineEdit*>();
            ASSERT_TRUE(legacyEdit && flatEdit);
            EXPECT_EQ(flatEdit->sizeHint().height(), legacyEdit->sizeHint().height());
            EXPECT_EQ(flatEdit->minimumSizeHint().height(), legacyEdit->minimumSizeHint().height());
            EXPECT_EQ(flatEdit->sizeHint().height(), 16);
            const auto* legacyStack = legacy.m_path->findChild<QStackedWidget*>();
            const auto* flatStack = flat.m_path->findChild<QStackedWidget*>();
            ASSERT_TRUE(legacyStack && flatStack);
            EXPECT_EQ(flatStack->sizeHint(), legacyStack->sizeHint());
            EXPECT_EQ(flatStack->minimumSizeHint(), legacyStack->minimumSizeHint());
            EXPECT_EQ(flat.m_path->sizeHint(), legacy.m_path->sizeHint());
            EXPECT_EQ(flat.m_path->minimumSizeHint(), legacy.m_path->minimumSizeHint());

            // the combo boxes: the legacy box did not depend on the frame
            for (const auto& [flatCombo, legacyCombo] : { std::pair{ flat.m_sizes, legacy.m_sizes }, std::pair{ flat.m_framed, legacy.m_framed } })
            {
                SCOPED_TRACE(flatCombo->hasFrame() ? "framed" : "no frame");
                EXPECT_EQ(flatCombo->sizeHint(), legacyCombo->sizeHint());
                EXPECT_EQ(flatCombo->minimumSizeHint(), legacyCombo->minimumSizeHint());
            }
            EXPECT_EQ(flat.m_sizes->sizeHint().width(), flat.m_framed->sizeHint().width());
        }

        TEST_F(ItemViewElementTests, Folder_TheHeightsOfTheBreadcrumbsAndTheSizeComboAreThoseOfTheNativeDump)
        {
            // n2 Gallery 00_assetbrowserfolder.widgets.json, which has the Open Sans font of the application: the breadcrumbs hint 195 x 19 (their stack
            // 0 x 19), the combo box without a frame 68 x 18; the flattened GUI had 195 x 25 (the 22 high path edit and its box). The widths are what the
            // font engine of a platform makes of the text (195 and 68 on the windows platform): the heights are the font's metrics and the rules'
            const OpenSansFonts fonts;
            {
                QWidget root;
                root.setStyle(style());
                auto* layout = new QVBoxLayout(&root);
                layout->setContentsMargins(0, 0, 0, 0);
                auto* path = new BreadCrumbs(&root);
                path->setCurrentPath(QStringLiteral("D:/OffLocalDev/CIPipelineTest"));
                layout->addWidget(path);
                auto* sizes = new QComboBox(&root);
                sizes->addItems({ QStringLiteral("Small"), QStringLiteral("Medium"), QStringLiteral("Large") });
                sizes->setFrame(false);
                sizes->setCurrentIndex(1);
                layout->addWidget(sizes);
                for (QWidget* child : root.findChildren<QWidget*>())
                {
                    child->setStyle(style());
                }
                root.setAttribute(Qt::WA_DontShowOnScreen);
                root.resize(1100, 100);
                root.show();
                QApplication::processEvents();

                EXPECT_EQ(path->sizeHint().height(), 19);
                EXPECT_EQ(path->findChild<QStackedWidget*>()->sizeHint().height(), 19);
                EXPECT_EQ(path->findChild<QLineEdit*>()->sizeHint().height(), 16);
                EXPECT_EQ(sizes->sizeHint().height(), 18);
            }
        }

        TEST_F(ItemViewElementTests, Views_TheRowsOfTheGalleryTreeBeginWhereTheNativeCaptureHasThem)
        {
            // n2 Gallery 31_tree-view__toggled, the tree qTreeView (TreeViewPage.cpp's model: Colors with Blue, Green, Red and a Black with Blacks in it): the
            // first pixel that is not the background of each row, in device pixels at ratio 1.5. The carets are at 9 (a top level row) and at 39 (its
            // child with children); the text of the rows from the first indentation on at 72, 71, 72 (a B, a G, an R) and, one level in, 102. The flattened
            // GUI had 66, 65, 66 and 96: the 4 px of padding of the item (TableView.qss:31-40) that the legacy GUI had left of the text
            const OpenSansFonts fonts;
            QWidget root;
            root.setStyle(style());
            auto* tree = new QTreeView(&root);
            tree->setStyle(style());
            tree->setHeaderHidden(true);
            auto* model = new QStandardItemModel(&root);
            auto* colors = new QStandardItem(QStringLiteral("Colors"));
            colors->appendRow(new QStandardItem(QStringLiteral("Blue")));
            colors->appendRow(new QStandardItem(QStringLiteral("Green")));
            colors->appendRow(new QStandardItem(QStringLiteral("Red")));
            auto* black = new QStandardItem(QStringLiteral("Black"));
            black->appendRow(new QStandardItem(QStringLiteral("Black")));
            black->appendRow(new QStandardItem(QStringLiteral("Black")));
            colors->appendRow(black);
            model->appendRow(colors);
            tree->setModel(model);
            tree->expandAll();
            tree->setGeometry(0, 0, 400, 300);
            root.resize(400, 300);
            root.setAttribute(Qt::WA_DontShowOnScreen);
            root.show();
            QApplication::processEvents();
            ASSERT_EQ(tree->indentation(), 20);
            ASSERT_EQ(tree->visualRect(model->index(0, 0)).height(), 24);

            const qreal ratio = 1.5;
            QImage image((QSizeF(400, 300) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(rgb(0x444444));
            tree->render(&image);

            const auto firstInk = [&](int row)
            {
                for (int x = 2; x < 150; ++x)
                {
                    for (int y = int((row * 24 + 4) * ratio); y < int((row * 24 + 20) * ratio); ++y)
                    {
                        const QColor c = QColor::fromRgba(image.pixel(x, y));
                        if (qAbs(c.red() - 0x44) + qAbs(c.green() - 0x44) + qAbs(c.blue() - 0x44) > 150)
                        {
                            return x;
                        }
                    }
                }
                return -1;
            };
            EXPECT_EQ(firstInk(0), 9);              // the caret of Colors
            EXPECT_EQ(firstInk(4), 39);             // the caret of Black
            const int rows[] = { 1, 2, 3, 5 };
            const int native[] = { 72, 71, 72, 102 };
            for (size_t i = 0; i < std::size(rows); ++i)
            {
                EXPECT_NEAR(firstInk(rows[i]), native[i], 1) << "row " << rows[i];      // a platform's font engine is a pixel off at most
            }
        }
    } // namespace ItemViewElementTestsInternal
} // namespace AzQtComponents
