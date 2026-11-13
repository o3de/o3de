/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/FieldTwins.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ControlElements.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>

#include <QAbstractAnimation>
#include <QAbstractItemView>
#include <QComboBox>
#include <QLineEdit>
#include <QProgressBar>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QStyleFactory>
#include <QStyleOptionProgressBar>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QTreeView>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <string>
#include <vector>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The rules of the item views and the progress bar that the flat rung drops:
    //   - CheckBox.qss:165-173, the eye an item view in visibility mode shows for a checked and an unchecked item
    //   - LineEdit.qss:37-40, `QAbstractItemView QLineEdit { qproperty-frame: true }`, the editor a delegate opens
    //   - FilteredSearchWidget.qss:121-128, the tree of the type selector, which has no outline and so no focus frame
    //   - ProgressBar.qss:10-24, the square track and the flat chunk
    // The legacy twin is Qt's own style sheet style over Fusion (over AzQtComponents::Style where the base style draws what the test
    // looks at) with the application's real sheet (FieldTwins::appSheet) on the root, the flat twin is the flattened Style with the
    // Global elements registered. The widgets are the real classes. Same boxes, same pixels, at the device pixel ratios of the GUI.
    // ----------------------------------------------------------------------------------

    namespace ViewsFlatSheetTestsInternal
    {
        using namespace FieldTwins;

        class ViewsFlatSheetTests
            : public Fixture
        {
        public:
            //! The flattened style of the flat twin
            QStyle* flatStyle() const
            {
                return m_style.get();
            }
        };

        const qreal ViewRatios[] = { 1.0, 1.5, 2.0 };

        const QStyle::State Enabled = QStyle::State_Enabled | QStyle::State_Active;
        const QStyle::State Disabled = QStyle::State_Active;

        //! The rung a twin is built, changed and painted under: the legacy twin under Qss, the flat one under Flat
        class RungScope
        {
        public:
            explicit RungScope(GuiPerfFloor::Rung rung)
                : m_before(GuiPerfFloor::rung())
            {
                GuiPerfFloor::setRungForTesting(rung);
            }

            ~RungScope()
            {
                GuiPerfFloor::setRungForTesting(m_before);
            }

            RungScope(const RungScope&) = delete;
            RungScope& operator=(const RungScope&) = delete;

        private:
            GuiPerfFloor::Rung m_before;
        };

        //! The cases a test found different, one line each, reported together
        struct Failures
        {
            void check(const std::string& name, const QImage& flat, const QImage& legacy)
            {
                if (const int count = differing(flat, legacy))
                {
                    m_lines.push_back(name + ": " + std::to_string(count) + " px, first " + firstDifference(flat, legacy));
                }
            }

            ~Failures()
            {
                std::string text;
                for (size_t i = 0; i < m_lines.size() && i < 40; ++i)
                {
                    text += "\n  " + m_lines[i];
                }
                EXPECT_TRUE(m_lines.empty()) << m_lines.size() << " case(s) differ:" << text;
            }

            std::vector<std::string> m_lines;
        };

        //! An image of a size and ratio, a backdrop on it, and what a test draws
        template<typename Draw>
        QImage drawnOn(const QSize& size, qreal ratio, const QColor& backdrop, Draw draw)
        {
            QImage image((QSizeF(size) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(backdrop);
            QPainter painter(&image);
            draw(&painter);
            painter.end();
            return image;
        }

        //! The pixels of one colour in an image
        int countOfColor(const QImage& image, const QColor& color)
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

        //! The option an item view hands the delegate for an item: a text, a state and a rect (QAbstractItemView::initViewItemOption)
        QStyleOptionViewItem viewItemOption(const QWidget* view, QStyle::State state, const QRect& rect)
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
            option.palette.setCurrentColorGroup(
                (state & QStyle::State_Enabled) ? ((state & QStyle::State_Active) ? QPalette::Active : QPalette::Inactive)
                                                : QPalette::Disabled);
            return option;
        }

        // ----------------------------------------------------------------------------------
        // CheckBox.qss:165-173: the check boxes of an item view in visibility mode
        // ----------------------------------------------------------------------------------

        struct CheckState
        {
            const char* m_name;
            QStyle::State m_state;
            Qt::CheckState m_check;
            bool m_partial;                 //!< a partially checked box: no rule of the mode reaches it
        };

        const CheckState CheckStates[] = {
            { "unchecked", Enabled | QStyle::State_Off, Qt::Unchecked, false },
            { "checked", Enabled | QStyle::State_On, Qt::Checked, false },
            { "unchecked and focused", Enabled | QStyle::State_Off | QStyle::State_HasFocus, Qt::Unchecked, false },
            { "checked and focused", Enabled | QStyle::State_On | QStyle::State_HasFocus, Qt::Checked, false },
            { "unchecked and disabled", Disabled | QStyle::State_Off, Qt::Unchecked, false },
            { "checked and disabled", Disabled | QStyle::State_On, Qt::Checked, false },
            { "indeterminate", Enabled | QStyle::State_NoChange, Qt::PartiallyChecked, true },
            { "indeterminate and focused", Enabled | QStyle::State_NoChange | QStyle::State_HasFocus, Qt::PartiallyChecked, true },
            { "indeterminate and disabled", Disabled | QStyle::State_NoChange, Qt::PartiallyChecked, true },
        };

        //! The tree of EMotionFX's Actors window (CheckBox::setVisibilityMode, ActorsWindow.cpp:66), and a tree beside it that is not in
        //! the mode
        void buildCheckTrees(QWidget& root, bool legacy)
        {
            Q_UNUSED(legacy);
            auto* layout = new QVBoxLayout(&root);
            for (const char* name : { "visibility", "plain" })
            {
                auto* tree = new QTreeWidget(&root);
                tree->setObjectName(QString::fromLatin1(name));
                tree->setHeaderHidden(true);
                auto* item = new QTreeWidgetItem(tree, QStringList(QStringLiteral("Actor")));
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                item->setCheckState(0, Qt::Checked);
                if (qstrcmp(name, "visibility") == 0)
                {
                    CheckBox::setVisibilityMode(tree, true);
                }
                layout->addWidget(tree);
            }
        }

        //! PE_IndicatorItemViewItemCheck in the slot the item gives its check box (QCommonStyle, CE_ItemViewItem)
        QImage paintedCheckBox(const QTreeWidget* view, const CheckState& test, qreal ratio)
        {
            return drawnOn(QSize(60, 30), ratio, QColor(0x80, 0x80, 0x80), [&](QPainter* painter)
            {
                QStyleOptionViewItem item = viewItemOption(view, test.m_state, QRect(7, 4, 50, 22));
                item.features |= QStyleOptionViewItem::HasCheckIndicator;
                item.checkState = test.m_check;
                QStyleOptionViewItem box(item);
                box.rect = view->style()->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &item, view);
                view->style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck, &box, painter, view);
            });
        }

        TEST_F(ViewsFlatSheetTests, VisibilityMode_TheEyesAreTheImagesOfTheSheetInEveryStateAtEveryRatio)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());   // the test environment sets it (AzQtComponentTests.cpp)
            // The images are the EMotionFX gem's, which this binary does not link: the tests carry a stand-in under the same names
            for (const char* image : { "Visualization", "VisualizationHidden" })
            {
                ASSERT_TRUE(QFile::exists(QStringLiteral(":/EMotionFX/%1.svg").arg(QLatin1String(image)))) << image;
            }
            for (const char* stock : { "off", "on", "partial-selected", "partial-selected-focus", "partial-selected-disabled" })
            {
                ASSERT_TRUE(QFile::exists(QStringLiteral(":/stylesheet/img/UI20/checkbox/%1.svg").arg(QLatin1String(stock)))) << stock;
            }

            Twins twins(*this, buildCheckTrees, QSize(240, 240));
            const auto* legacyEyes = twins.legacyChild<QTreeWidget>("visibility");
            const auto* flatEyes = twins.flatChild<QTreeWidget>("visibility");
            const auto* legacyPlain = twins.legacyChild<QTreeWidget>("plain");
            const auto* flatPlain = twins.flatChild<QTreeWidget>("plain");
            ASSERT_TRUE(legacyEyes && flatEyes && legacyPlain && flatPlain);
            ASSERT_TRUE(Style::hasClass(flatEyes, QStringLiteral("VisibilityMode")));

            Failures failures;
            for (const qreal ratio : ViewRatios)
            {
                for (const CheckState& test : CheckStates)
                {
                    const std::string name = std::string(test.m_name) + " at ratio " + std::to_string(ratio).substr(0, 3);
                    const QImage legacy = paintedCheckBox(legacyEyes, test, ratio);
                    failures.check(name, paintedCheckBox(flatEyes, test, ratio), legacy);
                    // The tree that is not in the mode keeps the check boxes, in both rungs
                    failures.check(
                        name + ", plain tree", paintedCheckBox(flatPlain, test, ratio), paintedCheckBox(legacyPlain, test, ratio));

                    // The twins are not two images of nothing, and the mode is what makes the eye: a checked or an unchecked item draws
                    // another image than the plain tree's check box, and a partially checked one the same
                    EXPECT_TRUE(isDrawnOn(legacy)) << name;
                    const int difference = differing(legacy, paintedCheckBox(legacyPlain, test, ratio));
                    if (test.m_partial)
                    {
                        EXPECT_EQ(difference, 0) << name;
                    }
                    else
                    {
                        EXPECT_GT(difference, 0) << name;
                    }
                }
            }
        }

        TEST_F(ViewsFlatSheetTests, VisibilityMode_TheEyeIsTheSameAtRestFocusedAndDisabled)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildCheckTrees, QSize(240, 240));
            const auto* legacyEyes = twins.legacyChild<QTreeWidget>("visibility");
            const auto* flatEyes = twins.flatChild<QTreeWidget>("visibility");
            ASSERT_TRUE(legacyEyes && flatEyes);

            // The rules of the mode have the weight of the rules of the states with a pseudo-class more, and come after them
            // (CheckBox.qss:50-57, :65-72, :165-173): the sheet draws the one image in all three, and so must the flat twin
            const auto state = [](const char* name)
            {
                for (const CheckState& test : CheckStates)
                {
                    if (qstrcmp(test.m_name, name) == 0)
                    {
                        return test;
                    }
                }
                return CheckStates[0];
            };
            const auto difference = [&](const QTreeWidget* view, const char* one, const char* other, qreal ratio)
            {
                return differing(paintedCheckBox(view, state(one), ratio), paintedCheckBox(view, state(other), ratio));
            };
            for (const QTreeWidget* view : { legacyEyes, flatEyes })
            {
                for (const qreal ratio : ViewRatios)
                {
                    EXPECT_EQ(difference(view, "checked", "checked and focused", ratio), 0);
                    EXPECT_EQ(difference(view, "checked", "checked and disabled", ratio), 0);
                    EXPECT_EQ(difference(view, "unchecked", "unchecked and focused", ratio), 0);
                    EXPECT_EQ(difference(view, "unchecked", "unchecked and disabled", ratio), 0);
                    EXPECT_GT(difference(view, "checked", "unchecked", ratio), 0);
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // LineEdit.qss:37-40: the line edit of a delegate's editor has a frame
        // ----------------------------------------------------------------------------------

        //! The delegate every view has, which makes the editors the default factory makes, with the style of its twin set on each (a style
        //! set on a widget is not its children's, and the sheet of the legacy twin reaches them through the parent)
        class EditorStyleDelegate
            : public QStyledItemDelegate
        {
        public:
            EditorStyleDelegate(QStyle* editorStyle, QObject* parent)
                : QStyledItemDelegate(parent)
                , m_editorStyle(editorStyle)
            {
            }

            QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
            {
                QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);
                if (editor && m_editorStyle)
                {
                    editor->setStyle(m_editorStyle);
                    for (QWidget* child : editor->findChildren<QWidget*>())
                    {
                        child->setStyle(m_editorStyle);
                    }
                }
                return editor;
            }

        private:
            QStyle* m_editorStyle;
        };

        //! The option QLineEdit::paintEvent gives the panel (QLineEdit::initStyleOption, protected), in a rect of an explicit size
        QStyleOptionFrame fieldOption(const QLineEdit* edit, const QSize& size)
        {
            QStyleOptionFrame option;
            option.initFrom(edit);
            option.rect = QRect(QPoint(0, 0), size);
            option.lineWidth = edit->hasFrame() ? edit->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &option, edit) : 0;
            option.midLineWidth = 0;
            option.state |= QStyle::State_Sunken;
            option.features = QStyleOptionFrame::None;
            return option;
        }

        //! The panel of the edit as its paint event draws it (PE_PanelLineEdit through its own style), in a rect of an explicit size
        QImage paintedField(const QLineEdit* edit, const QSize& size, qreal ratio)
        {
            return drawnOn(size, ratio, QColor(0x44, 0x44, 0x44), [&](QPainter* painter)
            {
                const QStyleOptionFrame option = fieldOption(edit, size);
                edit->style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, painter, edit);
            });
        }

        //! A table with a string, an integer and a string that has a widget of its own in its cell; its delegate makes the editors
        void buildTable(QWidget& root, bool legacy, QStyle* editorStyle)
        {
            auto* layout = new QVBoxLayout(&root);
            auto* view = new QTableView(&root);
            view->setObjectName(QStringLiteral("table"));
            auto* model = new QStandardItemModel(3, 1, view);
            model->setData(model->index(0, 0), QStringLiteral("text"), Qt::EditRole);
            model->setData(model->index(1, 0), 5, Qt::EditRole);
            model->setData(model->index(2, 0), QStringLiteral("held"), Qt::EditRole);
            view->setModel(model);
            view->setItemDelegate(new EditorStyleDelegate(legacy ? nullptr : editorStyle, view));

            // A line edit a view holds for an item, built with no frame (its owner's call) as the editors of the default factory are
            auto* held = new QLineEdit(QStringLiteral("held"));
            held->setObjectName(QStringLiteral("held"));
            held->setFrame(false);
            view->setIndexWidget(model->index(2, 0), held);
            layout->addWidget(view);
        }

        //! The editor the view opens for an item, once the view has been shown: the one of its viewport's children that has no name
        //! (the widgets the owner of the view gave names are its own)
        template<typename Editor>
        Editor* openEditor(QTableView* view, int row)
        {
            view->setCurrentIndex(view->model()->index(row, 0));
            view->edit(view->model()->index(row, 0));
            QApplication::processEvents();
            for (Editor* candidate : view->viewport()->findChildren<Editor*>())
            {
                if (candidate->objectName().isEmpty())
                {
                    return candidate;
                }
            }
            return nullptr;
        }

        TEST_F(ViewsFlatSheetTests, EditorLineEdit_HasTheFrameTheSheetGaveItAndDrawsTheFieldAtEveryRatio)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, [this](QWidget& root, bool legacy) { buildTable(root, legacy, flatStyle()); }, QSize(300, 200));
            auto* legacyView = twins.legacyChild<QTableView>("table");
            auto* flatView = twins.flatChild<QTableView>("table");
            ASSERT_TRUE(legacyView && flatView);

            // The default factory makes a string editor with the frame QStyle::SH_ItemView_DrawDelegateFrame says, which neither style
            // answers (0); the sheet sets `qproperty-frame: true` when it polishes the editor
            QLineEdit* legacy = openEditor<QLineEdit>(legacyView, 0);
            QLineEdit* flat = openEditor<QLineEdit>(flatView, 0);
            ASSERT_TRUE(legacy != nullptr && flat != nullptr);
            EXPECT_EQ(legacy->style()->styleHint(QStyle::SH_ItemView_DrawDelegateFrame, nullptr, legacy), 0);
            EXPECT_EQ(flat->style()->styleHint(QStyle::SH_ItemView_DrawDelegateFrame, nullptr, flat), 0);
            EXPECT_TRUE(legacy->hasFrame());
            EXPECT_TRUE(flat->hasFrame());

            // The field: the rect inside its box, and its pixels
            const QSize size(120, 24);
            const QStyleOptionFrame legacyOption = fieldOption(legacy, size);
            const QStyleOptionFrame flatOption = fieldOption(flat, size);
            EXPECT_EQ(rectText(flat->style()->subElementRect(QStyle::SE_LineEditContents, &flatOption, flat)),
                rectText(legacy->style()->subElementRect(QStyle::SE_LineEditContents, &legacyOption, legacy)));
            Failures failures;
            for (const qreal ratio : ViewRatios)
            {
                const QImage legacyImage = paintedField(legacy, size, ratio);
                failures.check("at ratio " + std::to_string(ratio).substr(0, 3), paintedField(flat, size, ratio), legacyImage);
                EXPECT_TRUE(isDrawnOn(legacyImage));
                const QPoint middle(static_cast<int>(60 * ratio), static_cast<int>(12 * ratio));
                EXPECT_EQ(legacyImage.pixelColor(middle), QColor(0xCC, 0xCC, 0xCC));
            }
        }

        TEST_F(ViewsFlatSheetTests, EditorLineEdit_ALineEditAViewHoldsHasTheFrameWhateverItsOwnerSet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, [this](QWidget& root, bool legacy) { buildTable(root, legacy, flatStyle()); }, QSize(300, 200));
            const auto* legacy = twins.legacyChild<QLineEdit>("held");
            const auto* flat = twins.flatChild<QLineEdit>("held");
            ASSERT_TRUE(legacy != nullptr && flat != nullptr);

            EXPECT_TRUE(legacy->hasFrame());
            EXPECT_TRUE(flat->hasFrame());
            const QSize size(120, 24);
            const QStyleOptionFrame legacyOption = fieldOption(legacy, size);
            const QStyleOptionFrame flatOption = fieldOption(flat, size);
            EXPECT_EQ(rectText(flat->style()->subElementRect(QStyle::SE_LineEditContents, &flatOption, flat)),
                rectText(legacy->style()->subElementRect(QStyle::SE_LineEditContents, &legacyOption, legacy)));
            Failures failures;
            for (const qreal ratio : ViewRatios)
            {
                failures.check(
                    "at ratio " + std::to_string(ratio).substr(0, 3), paintedField(flat, size, ratio), paintedField(legacy, size, ratio));
            }
        }

        TEST_F(ViewsFlatSheetTests, EditorLineEdit_TheLineEditOfASpinBoxEditorAndOfAnEditableComboBoxKeepsTheFrameItHad)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, [this](QWidget& root, bool legacy) { buildTable(root, legacy, flatStyle()); }, QSize(300, 200));
            auto* legacyView = twins.legacyChild<QTableView>("table");
            auto* flatView = twins.flatChild<QTableView>("table");
            ASSERT_TRUE(legacyView && flatView);

            // The editor of an integer is a spin box; the line edit inside it is the spin box's, and the sheet leaves it as the spin box
            // made it (QAbstractSpinBox), as it does that of a combo box
            QSpinBox* legacySpin = openEditor<QSpinBox>(legacyView, 1);
            QSpinBox* flatSpin = openEditor<QSpinBox>(flatView, 1);
            ASSERT_TRUE(legacySpin != nullptr && flatSpin != nullptr);
            ASSERT_TRUE(legacySpin->findChild<QLineEdit*>() != nullptr && flatSpin->findChild<QLineEdit*>() != nullptr);
            EXPECT_FALSE(legacySpin->findChild<QLineEdit*>()->hasFrame());
            EXPECT_EQ(flatSpin->findChild<QLineEdit*>()->hasFrame(), legacySpin->findChild<QLineEdit*>()->hasFrame());

            for (QTableView* view : { legacyView, flatView })
            {
                auto* combo = new QComboBox();
                combo->setEditable(true);
                combo->setObjectName(QStringLiteral("combo"));
                view->setIndexWidget(view->model()->index(0, 0), combo);
                if (view == flatView)
                {
                    combo->setStyle(flatStyle());
                    for (QWidget* child : combo->findChildren<QWidget*>())
                    {
                        child->setStyle(flatStyle());
                    }
                }
                combo->show();
            }
            QApplication::processEvents();
            const auto* legacyCombo = legacyView->viewport()->findChild<QComboBox*>(QStringLiteral("combo"));
            const auto* flatCombo = flatView->viewport()->findChild<QComboBox*>(QStringLiteral("combo"));
            ASSERT_TRUE(legacyCombo != nullptr && flatCombo != nullptr);
            EXPECT_FALSE(legacyCombo->lineEdit()->hasFrame());
            EXPECT_EQ(flatCombo->lineEdit()->hasFrame(), legacyCombo->lineEdit()->hasFrame());
        }

        // ----------------------------------------------------------------------------------
        // FilteredSearchWidget.qss:121-128: the tree of the type selector has no outline, and so no focus frame
        // ----------------------------------------------------------------------------------

        //! A type selector (its tree is a QTreeView in a QMenu) and a tree of the same kind beside it. The legacy twin has the
        //! AzQtComponents::Style as the base of its sheet style, which draws the frame the rule's outline would replace (TableView), and
        //! is built under the Qss rung.
        struct SelectorTwins
        {
            explicit SelectorTwins(const ViewsFlatSheetTests& fixture)
                : m_legacyBase(AZStd::make_unique<Style>(QStyleFactory::create(QStringLiteral("Fusion"))))
            {
                {
                    RungScope rung(GuiPerfFloor::Rung::Qss);
                    m_legacy.setStyle(m_legacyBase.get());
                    build(m_legacy);
                    Fixture::legacy(m_legacy);
                    Fixture::show(m_legacy, QSize(300, 200));
                    polishTrees(m_legacy);
                }
                {
                    RungScope rung(GuiPerfFloor::Rung::Flat);
                    build(m_flat);
                    fixture.flat(m_flat);
                    Fixture::show(m_flat, QSize(300, 200));
                    polishTrees(m_flat);
                }
            }

            //! The type selector is a menu that is not shown: its tree is polished by hand, in the rung of its twin
            static void polishTrees(QWidget& root)
            {
                treeOf(root, "selector")->ensurePolished();
                treeOf(root, "outside")->ensurePolished();
            }

            static void build(QWidget& root)
            {
                auto* layout = new QVBoxLayout(&root);
                auto* selector = new SearchTypeSelector(&root);
                selector->setObjectName(QStringLiteral("selector"));
                auto* outside = new QTreeView(&root);
                outside->setObjectName(QStringLiteral("outside"));
                layout->addWidget(outside);
            }

            static QTreeView* treeOf(QWidget& root, const char* name)
            {
                if (qstrcmp(name, "selector") == 0)
                {
                    return root.findChild<SearchTypeSelector*>(QStringLiteral("selector"))->GetTree();
                }
                return root.findChild<QTreeView*>(QString::fromLatin1(name));
            }

            // Declared in this order: the widgets go before the style they use
            AZStd::unique_ptr<Style> m_legacyBase;
            QWidget m_legacy;
            QWidget m_flat;
        };

        //! An item of a tree, with the keyboard focus or without, selected or not (CE_ItemViewItem draws the focus frame itself)
        QImage paintedTreeItem(const QTreeView* view, bool focused, bool selected, qreal ratio)
        {
            return drawnOn(QSize(160, 32), ratio, QColor(0x33, 0x33, 0x33), [&](QPainter* painter)
            {
                const QStyle::State state = Enabled | (focused ? QStyle::State_HasFocus : QStyle::State_None)
                    | (selected ? QStyle::State_Selected : QStyle::State_None);
                const QStyleOptionViewItem item = viewItemOption(view, state, QRect(4, 4, 150, 24));
                view->style()->drawControl(QStyle::CE_ItemViewItem, &item, painter, view);
            });
        }

        TEST_F(ViewsFlatSheetTests, SearchTypeSelector_TheTreeDrawsNoFocusFrameAndAnotherTreeDoes)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            SelectorTwins twins(*this);
            const QTreeView* legacyTree = SelectorTwins::treeOf(twins.m_legacy, "selector");
            const QTreeView* flatTree = SelectorTwins::treeOf(twins.m_flat, "selector");
            const QTreeView* legacyOutside = SelectorTwins::treeOf(twins.m_legacy, "outside");
            const QTreeView* flatOutside = SelectorTwins::treeOf(twins.m_flat, "outside");
            ASSERT_TRUE(legacyTree && flatTree && legacyOutside && flatOutside);
            ASSERT_TRUE(legacyTree->inherits("QTreeView"));

            // What the focus adds to an item: the pixels that differ from the same item without it. The legacy twin is drawn under the
            // Qss rung, as it is built.
            const auto focusAdds = [](const QTreeView* view, bool legacy, bool selected, qreal ratio)
            {
                RungScope rung(legacy ? GuiPerfFloor::Rung::Qss : GuiPerfFloor::Rung::Flat);
                return differing(paintedTreeItem(view, true, selected, ratio), paintedTreeItem(view, false, selected, ratio));
            };
            for (const qreal ratio : ViewRatios)
            {
                for (const bool selected : { false, true })
                {
                    const std::string name =
                        std::string(selected ? "selected" : "unselected") + " at ratio " + std::to_string(ratio).substr(0, 3);

                    // `outline-style: none` (FilteredSearchWidget.qss:124): the item with the keyboard focus looks as it does without
                    EXPECT_EQ(focusAdds(legacyTree, true, selected, ratio), 0) << "legacy " << name;
                    EXPECT_EQ(focusAdds(flatTree, false, selected, ratio), 0) << "flat " << name;

                    // ... and a tree that is not in the type selector draws its frame, in both twins
                    EXPECT_GT(focusAdds(legacyOutside, true, selected, ratio), 0) << "legacy outside " << name;
                    EXPECT_GT(focusAdds(flatOutside, false, selected, ratio), 0) << "flat outside " << name;
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // ProgressBar.qss:10-24: the square track and the flat chunk
        // ----------------------------------------------------------------------------------

        const QColor TrackColor(0x9A, 0x9A, 0x9A);                  // ProgressBar.qss:14
        const QColor ChunkColor(0x1E, 0x70, 0xEB);                  // ProgressBar.qss:23
        const QColor BarBackdrop(0x44, 0x44, 0x44);

        //! A bar that gives its style option out (QProgressBar::initStyleOption is protected)
        class ExposedBar
            : public QProgressBar
        {
        public:
            using QProgressBar::QProgressBar;

            QStyleOptionProgressBar optionOf(const QSize& size) const
            {
                QStyleOptionProgressBar option;
                initStyleOption(&option);
                option.rect = QRect(QPoint(0, 0), size);
                return option;
            }
        };

        //! The bars of a pair of twins, by name (the class has no Q_OBJECT for findChild to ask: the object is one, made by buildBars)
        template<typename Pair>
        ExposedBar* legacyBar(Pair& twins, const char* name)
        {
            return static_cast<ExposedBar*>(twins.template legacyChild<QProgressBar>(name));
        }

        template<typename Pair>
        ExposedBar* flatBar(Pair& twins, const char* name)
        {
            return static_cast<ExposedBar*>(twins.template flatChild<QProgressBar>(name));
        }

        struct BarCase
        {
            const char* m_name;
            int m_minimum;
            int m_maximum;
            int m_value;
        };

        const BarCase BarCases[] = {
            { "empty", 0, 100, 0 },
            { "one", 0, 100, 1 },
            { "part", 0, 100, 40 },
            { "nearly full", 0, 100, 99 },
            { "full", 0, 100, 100 },
            { "a range that starts below zero", -50, 50, 0 },
            { "no range", 5, 5, 5 },
            { "unset", 0, 100, -1 },
        };

        //! The bar in the state of a case: a value outside the range is not taken (QProgressBar::setValue), so the bar is reset first
        void setBar(ExposedBar* bar, const BarCase& test)
        {
            bar->setRange(test.m_minimum, test.m_maximum);
            bar->reset();
            bar->setValue(test.m_value);
        }

        //! The columns of 278 the chunk fills at ratio 1: the sheet takes the share of the range the value is, as an integer
        int chunkColumns(const QProgressBar* bar, int width)
        {
            if (bar->minimum() == bar->maximum())
            {
                return width / 2;
            }
            const double share = double(bar->value() - bar->minimum()) / double(bar->maximum() - bar->minimum());
            return std::clamp(static_cast<int>(width * share), 0, width);
        }

        //! The bars of the Gallery's progress page (a determinate bar) and one of the Project Manager's: a window with the module's sheet
        //! (ProjectManager.qss:772-780) that the legacy window has and the flat twin has as the element of the module
        void buildBars(QWidget& root, bool legacy)
        {
            auto* layout = new QVBoxLayout(&root);
            layout->setContentsMargins(8, 8, 8, 8);
            layout->setSpacing(6);
            auto* plain = new ExposedBar(&root);
            plain->setObjectName(QStringLiteral("plain"));
            layout->addWidget(plain);

            auto* window = new QWidget(&root);
            window->setObjectName(QStringLiteral("window"));
            auto* windowLayout = new QVBoxLayout(window);
            windowLayout->setContentsMargins(0, 0, 0, 0);
            if (legacy)
            {
                window->setStyleSheet(QStringLiteral(
                    "QProgressBar { border: none; background-color: transparent; padding: 0px; }\n"
                    "QProgressBar::chunk { background-color: #1E70EB; }\n"));
            }
            auto* moduleBar = new ExposedBar(window);
            moduleBar->setObjectName(QStringLiteral("moduleBar"));
            windowLayout->addWidget(moduleBar);
            layout->addWidget(window);
        }

        //! The element the module of the window above has for its bars, as ProjectManager.qss:772-776 gives them: no border, no fill,
        //! no padding, and the text hidden by ProgressBar.qss:18. Registered once for the process, for the bars of that name alone.
        const BoxElement ModuleBarElement{
            .m_name = "Test.Box.ModuleBar",
            .m_hasBox = true,
            .m_nativeBorder = false,
            .m_idle = { .m_fill = QBrush(QColor(0, 0, 0, 0)) },
            .m_properties = { { "textVisible", false } },
        };

        void registerModuleBar()
        {
            static bool registered = false;
            if (!registered)
            {
                registered = true;
                BoxElement::registerElement(&ModuleBarElement, &QProgressBar::staticMetaObject,
                    [](const QWidget* w) { return Selectors::named(w, "moduleBar"); });
            }
        }

        //! CE_ProgressBar through the bar's own style for an option the test made, onto an image of an explicit device ratio and a size
        //! that may be larger than the option's rect (what a style draws beyond its rect then shows)
        QImage paintedBarOption(const ExposedBar* bar, const QStyleOptionProgressBar& option, const QSize& imageSize, qreal ratio)
        {
            return drawnOn(imageSize, ratio, BarBackdrop, [&](QPainter* painter)
            {
                bar->style()->drawControl(QStyle::CE_ProgressBar, &option, painter, bar);
            });
        }

        //! CE_ProgressBar through the bar's own style, for the option of the bar in a rect of a size
        QImage paintedBar(const ExposedBar* bar, const QSize& size, qreal ratio)
        {
            return paintedBarOption(bar, bar->optionOf(size), size, ratio);
        }

        //! The animation that moves the chunk of a busy bar: both styles make it, when they first draw the bar busy, as a child of the bar
        QAbstractAnimation* animationOfBar(const ExposedBar* bar)
        {
            const QList<QAbstractAnimation*> animations = bar->findChildren<QAbstractAnimation*>();
            return animations.size() == 1 ? animations.first() : nullptr;
        }

        //! A time in the middle of a step of an animation that takes 30 steps to the second (a step is the time over 1000 / 30, cut)
        int midStepTime(int step)
        {
            return static_cast<int>((step + 0.5) * 1000.0 / 30.0);
        }

        TEST_F(ViewsFlatSheetTests, ProgressBar_TheTrackAndTheChunkArePaintedAsTheSheetPaintsThemAtEveryRatio)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildBars, QSize(300, 120));
            auto* legacy = legacyBar(twins, "plain");
            auto* flat = flatBar(twins, "plain");
            ASSERT_TRUE(legacy != nullptr && flat != nullptr);

            // The size and the text: the height of the sheet (ProgressBar.qss:15), the text hidden at polish (:18)
            EXPECT_EQ(sizeText(flat->sizeHint()), sizeText(legacy->sizeHint()));
            EXPECT_EQ(sizeText(flat->minimumSizeHint()), sizeText(legacy->minimumSizeHint()));
            EXPECT_EQ(sizeText(flat->size()), sizeText(legacy->size()));
            EXPECT_EQ(legacy->sizeHint().height(), 5);
            EXPECT_FALSE(legacy->isTextVisible());
            EXPECT_FALSE(flat->isTextVisible());

            Failures failures;
            for (const qreal ratio : ViewRatios)
            {
                for (const BarCase& test : BarCases)
                {
                    setBar(legacy, test);
                    setBar(flat, test);
                    const std::string name = std::string(test.m_name) + " at ratio " + std::to_string(ratio).substr(0, 3);
                    const QImage legacyImage = paintedBar(legacy, QSize(278, 5), ratio);
                    failures.check(name, paintedBar(flat, QSize(278, 5), ratio), legacyImage);

                    // The twins are not two images of nothing: the track is the sheet's grey wherever the chunk is not, and the chunk a
                    // plain rect (at ratio 1, where a pixel is a pixel: 111 columns of 278 at 40 of 100)
                    if (ratio == 1.0)
                    {
                        const int columns = chunkColumns(legacy, 278);
                        EXPECT_EQ(countOfColor(legacyImage, ChunkColor), columns * 5) << name;
                        EXPECT_EQ(countOfColor(legacyImage, TrackColor), (278 - columns) * 5) << name;
                    }
                }
            }

            // A disabled bar looks as it does enabled (the sheet has no rule for it)
            setBar(legacy, BarCases[2]);
            setBar(flat, BarCases[2]);
            legacy->setEnabled(false);
            flat->setEnabled(false);
            for (const qreal ratio : ViewRatios)
            {
                failures.check("disabled at ratio " + std::to_string(ratio).substr(0, 3), paintedBar(flat, QSize(278, 5), ratio),
                    paintedBar(legacy, QSize(278, 5), ratio));
            }
        }

        TEST_F(ViewsFlatSheetTests, ProgressBar_TheChunkGrowsFromTheEdgeTheDirectionAndTheOrientationSay)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildBars, QSize(300, 120));
            auto* legacy = legacyBar(twins, "plain");
            auto* flat = flatBar(twins, "plain");
            ASSERT_TRUE(legacy != nullptr && flat != nullptr);

            Failures failures;
            for (const Qt::LayoutDirection direction : { Qt::LeftToRight, Qt::RightToLeft })
            {
                for (const bool inverted : { false, true })
                {
                    for (const Qt::Orientation orientation : { Qt::Horizontal, Qt::Vertical })
                    {
                        for (ExposedBar* bar : { legacy, flat })
                        {
                            bar->setLayoutDirection(direction);
                            bar->setInvertedAppearance(inverted);
                            bar->setOrientation(orientation);
                        }
                        const QSize size = orientation == Qt::Horizontal ? QSize(120, 5) : QSize(5, 120);
                        for (const BarCase& test : { BarCases[0], BarCases[2], BarCases[4], BarCases[5] })
                        {
                            setBar(legacy, test);
                            setBar(flat, test);
                            for (const qreal ratio : ViewRatios)
                            {
                                const std::string name = std::string(direction == Qt::RightToLeft ? "right to left, " : "left to right, ")
                                    + (inverted ? "inverted, " : "") + (orientation == Qt::Vertical ? "vertical, " : "horizontal, ")
                                    + test.m_name + " at ratio " + std::to_string(ratio).substr(0, 3);
                                failures.check(name, paintedBar(flat, size, ratio), paintedBar(legacy, size, ratio));
                            }
                        }
                    }
                }
            }

            // A vertical bar fills upwards, from its bottom, and a horizontal one from its left (the sheet turns the bar by 90 degrees)
            for (ExposedBar* bar : { legacy, flat })
            {
                bar->setLayoutDirection(Qt::LeftToRight);
                bar->setInvertedAppearance(false);
                bar->setOrientation(Qt::Vertical);
                setBar(bar, BarCases[2]);
                const QImage image = paintedBar(bar, QSize(5, 100), 1.0);
                EXPECT_EQ(QColor::fromRgba(image.pixel(2, 99)), ChunkColor);
                EXPECT_EQ(QColor::fromRgba(image.pixel(2, 0)), TrackColor);
            }
        }

        TEST_F(ViewsFlatSheetTests, ProgressBar_AValueBeyondTheRangeFillsTheBarOrLeavesItClearAsTheSheetDrawsIt)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildBars, QSize(300, 120));
            auto* legacy = legacyBar(twins, "plain");
            auto* flat = flatBar(twins, "plain");
            ASSERT_TRUE(legacy != nullptr && flat != nullptr);
            for (ExposedBar* bar : { legacy, flat })
            {
                bar->setRange(0, 100);
                bar->setValue(40);
            }

            // A bar does not hold such a value (QProgressBar::setValue), but a style is handed whatever an option says: the sheet takes
            // the share of the range the value is as a width, clips the chunk to the contents (the bar is drawn at x = 10 on an image
            // wider than the bar, so that a chunk not clipped shows), and draws no chunk of a width below one
            struct Beyond
            {
                const char* m_name;
                int m_progress;
                int m_columns;                  //!< the columns of 278 the chunk has at ratio 1
            };
            const Beyond cases[] = {
                { "just over the maximum", 101, 278 },
                { "far over the maximum", 150, 278 },
                { "under the minimum", -10, 0 },
            };
            const QSize imageSize(300, 5);
            const auto optionOf = [&](const ExposedBar* bar, int progress)
            {
                QStyleOptionProgressBar option = bar->optionOf(QSize(278, 5));
                option.rect.moveLeft(10);
                option.progress = progress;
                return option;
            };

            Failures failures;
            for (const qreal ratio : ViewRatios)
            {
                for (const Beyond& test : cases)
                {
                    const std::string name = std::string(test.m_name) + " at ratio " + std::to_string(ratio).substr(0, 3);
                    const QImage legacyImage = paintedBarOption(legacy, optionOf(legacy, test.m_progress), imageSize, ratio);
                    failures.check(name, paintedBarOption(flat, optionOf(flat, test.m_progress), imageSize, ratio), legacyImage);
                    if (ratio == 1.0)
                    {
                        EXPECT_EQ(countOfColor(legacyImage, ChunkColor), test.m_columns * 5) << name;
                        EXPECT_EQ(countOfColor(legacyImage, TrackColor), (278 - test.m_columns) * 5) << name;
                    }
                }
            }
        }

        TEST_F(ViewsFlatSheetTests, ProgressBar_ABarWithNeitherRangeNorValueIsBusyAndDrawsHalfItsWidthFirst)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The animation of a busy bar starts when the bar is first drawn busy: a bar of its own for each twin, drawn once
            Failures failures;
            for (const qreal ratio : ViewRatios)
            {
                Twins twins(*this, buildBars, QSize(300, 120));
                auto* legacy = legacyBar(twins, "plain");
                auto* flat = flatBar(twins, "plain");
                ASSERT_TRUE(legacy != nullptr && flat != nullptr);
                legacy->setRange(0, 0);
                flat->setRange(0, 0);
                const QImage legacyImage = paintedBar(legacy, QSize(278, 5), ratio);
                failures.check("busy at ratio " + std::to_string(ratio).substr(0, 3), paintedBar(flat, QSize(278, 5), ratio), legacyImage);
                if (ratio == 1.0)
                {
                    EXPECT_EQ(countOfColor(legacyImage, ChunkColor), 139 * 5);
                    EXPECT_EQ(QColor::fromRgba(legacyImage.pixel(0, 2)), ChunkColor);
                }
            }
        }

        TEST_F(ViewsFlatSheetTests, ProgressBar_TheBusyChunkMovesAlongTheBarAndComesInAgainAtTheNearEdge)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            QStyleOptionProgressBar option;
            option.state = QStyle::State_Enabled | QStyle::State_Horizontal;
            option.direction = Qt::LeftToRight;
            option.minimum = 0;
            option.maximum = 0;
            option.progress = 0;
            const QRect contents(0, 0, 100, 5);

            // 8px a step (QStyleSheetStyle::drawControl, CE_ProgressBarContents): the chunk is 50 wide. At step 0 it is at the left; at
            // step 3 at 24; at step 9 it starts at 72, and the second chunk the sheet draws then starts at 0 + (122 - 99 - 50) and
            // shows the part that wrapped (a pixel more than there is to wrap, as the sheet draws it)
            const auto chunks = [&](int step) { return ControlElements::progressChunkRects(&option, contents, step); };
            EXPECT_EQ(chunks(0), QVector<QRect>({ QRect(0, 0, 50, 5) }));
            EXPECT_EQ(chunks(3), QVector<QRect>({ QRect(24, 0, 50, 5) }));
            EXPECT_EQ(chunks(9), QVector<QRect>({ QRect(72, 0, 50, 5), QRect(-27, 0, 50, 5) }));

            // Right to left it moves the other way, from the right edge; the second chunk the sheet draws then is at 99 - (0 + 24 - 50)
            option.direction = Qt::RightToLeft;
            EXPECT_EQ(chunks(0), QVector<QRect>({ QRect(50, 0, 50, 5) }));
            EXPECT_EQ(chunks(3), QVector<QRect>({ QRect(26, 0, 50, 5), QRect(125, 0, 50, 5) }));

            // A value draws one chunk, and a bar at its minimum none; a bar with a range of nothing but not at zero is half full
            option.direction = Qt::LeftToRight;
            option.minimum = 0;
            option.maximum = 200;
            option.progress = 50;
            EXPECT_EQ(chunks(7), QVector<QRect>({ QRect(0, 0, 25, 5) }));
            option.progress = 0;
            EXPECT_TRUE(chunks(0).isEmpty());
            option.minimum = 5;
            option.maximum = 5;
            option.progress = 5;
            EXPECT_EQ(chunks(0), QVector<QRect>({ QRect(0, 0, 50, 5) }));
        }

        TEST_F(ViewsFlatSheetTests, ProgressBar_TheBusyChunkIsWhereTheAnimationOfTheSheetPutsItAtEveryStepInEveryDirection)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildBars, QSize(300, 120));
            auto* legacy = legacyBar(twins, "plain");
            auto* flat = flatBar(twins, "plain");
            ASSERT_TRUE(legacy != nullptr && flat != nullptr);

            // Each style starts its animation when it first draws the bar busy. From then on the time of each animation is set to the step
            // under test (the event loop would move it in real time), and the bars drawn.
            for (ExposedBar* bar : { legacy, flat })
            {
                bar->setRange(0, 0);
                paintedBar(bar, QSize(120, 5), 1.0);
                ASSERT_TRUE(animationOfBar(bar) != nullptr);
                ASSERT_EQ(animationOfBar(bar)->state(), QAbstractAnimation::Running);
            }

            // The first step, some on the way, the ones past the middle where the chunk of 60 wraps round the 120px of the bar (8px a
            // step: 64, 72 and 112), and step 15, which is 120px and so at the start again
            const int steps[] = { 0, 1, 3, 7, 8, 9, 14, 15 };

            Failures failures;
            for (const Qt::LayoutDirection direction : { Qt::LeftToRight, Qt::RightToLeft })
            {
                for (const bool inverted : { false, true })
                {
                    for (const Qt::Orientation orientation : { Qt::Horizontal, Qt::Vertical })
                    {
                        for (ExposedBar* bar : { legacy, flat })
                        {
                            bar->setLayoutDirection(direction);
                            bar->setInvertedAppearance(inverted);
                            bar->setOrientation(orientation);
                        }
                        const QSize size = orientation == Qt::Horizontal ? QSize(120, 5) : QSize(5, 120);
                        QImage atStart;
                        for (const int step : steps)
                        {
                            for (const qreal ratio : ViewRatios)
                            {
                                const std::string name = std::string(direction == Qt::RightToLeft ? "right to left, " : "left to right, ")
                                    + (inverted ? "inverted, " : "") + (orientation == Qt::Vertical ? "vertical, " : "horizontal, ")
                                    + "step " + std::to_string(step) + " at ratio " + std::to_string(ratio).substr(0, 3);
                                animationOfBar(legacy)->setCurrentTime(midStepTime(step));
                                animationOfBar(flat)->setCurrentTime(midStepTime(step));
                                const QImage legacyImage = paintedBar(legacy, size, ratio);
                                failures.check(name, paintedBar(flat, size, ratio), legacyImage);

                                // The twins are not two images of a bar that stands still: the chunk moves, and is back at step 15
                                if (ratio == 1.0)
                                {
                                    if (step == 0)
                                    {
                                        atStart = legacyImage;
                                    }
                                    else if (step == 3)
                                    {
                                        EXPECT_GT(differing(atStart, legacyImage), 0) << name;
                                    }
                                    else if (step == 15)
                                    {
                                        EXPECT_EQ(differing(atStart, legacyImage), 0) << name;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        TEST_F(ViewsFlatSheetTests, ProgressBar_ABusyBarRepaintsItselfWhileItIsShownAndStartsAgainAfterAValue)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildBars, QSize(300, 120));
            auto* flat = flatBar(twins, "plain");
            ASSERT_TRUE(flat != nullptr);
            flat->setRange(0, 0);
            const QImage first = paintedBar(flat, QSize(278, 5), 1.0);
            EXPECT_EQ(countOfColor(first, ChunkColor), 139 * 5);        // a flat chunk of half the bar, not the shaded one of Fusion
            const QList<QAbstractAnimation*> started = flat->findChildren<QAbstractAnimation*>();
            ASSERT_EQ(started.size(), 1);
            EXPECT_EQ(started[0]->state(), QAbstractAnimation::Running);

            // A few steps on the chunk is elsewhere: 250ms is 7 steps of 33ms (the event loop drives the animation, and the unified
            // timer of Qt does not tick in a process that made and destroyed an application before, so the time is set)
            started[0]->setCurrentTime(250);
            EXPECT_GT(differing(first, paintedBar(flat, QSize(278, 5), 1.0)), 0);

            // A bar with a value has no animation, and a busy bar again begins at the left
            flat->setRange(0, 100);
            flat->setValue(10);
            paintedBar(flat, QSize(278, 5), 1.0);
            EXPECT_TRUE(flat->findChildren<QAbstractAnimation*>().isEmpty());
            flat->setRange(0, 0);
            EXPECT_EQ(differing(first, paintedBar(flat, QSize(278, 5), 1.0)), 0);
        }

        TEST_F(ViewsFlatSheetTests, ProgressBar_ABarOfAModuleHasTheElementsTrackAndTheSameChunk)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            registerModuleBar();

            Twins twins(*this, buildBars, QSize(300, 120));
            auto* legacy = legacyBar(twins, "moduleBar");
            auto* flat = flatBar(twins, "moduleBar");
            ASSERT_TRUE(legacy != nullptr && flat != nullptr);
            ASSERT_TRUE(BoxElement::forWidget(flat) == &ModuleBarElement);
            EXPECT_FALSE(legacy->isTextVisible());
            EXPECT_FALSE(flat->isTextVisible());

            Failures failures;
            for (const qreal ratio : ViewRatios)
            {
                for (const BarCase& test : BarCases)
                {
                    setBar(legacy, test);
                    setBar(flat, test);
                    const std::string name = std::string(test.m_name) + " at ratio " + std::to_string(ratio).substr(0, 3);
                    const QImage legacyImage = paintedBar(legacy, QSize(278, 5), ratio);
                    failures.check(name, paintedBar(flat, QSize(278, 5), ratio), legacyImage);

                    // The track is clear (ProjectManager.qss:774): the backdrop shows where the chunk is not
                    if (ratio == 1.0)
                    {
                        EXPECT_EQ(countOfColor(legacyImage, TrackColor), 0) << name;
                        EXPECT_EQ(countOfColor(legacyImage, ChunkColor) + countOfColor(legacyImage, BarBackdrop), 278 * 5) << name;
                    }
                }
            }
        }

        TEST_F(ViewsFlatSheetTests, ProgressBar_TheTextOfABarThatShowsItIsTheTextColourAlignedInTheBar)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildBars, QSize(300, 120));
            auto* legacy = legacyBar(twins, "plain");
            auto* flat = flatBar(twins, "plain");
            ASSERT_TRUE(legacy != nullptr && flat != nullptr);

            // The sheet hid the text when it polished the bar; a bar that shows it afterwards draws it in the colour of the text, not
            // of the chunk. The bars have a font of a size it can be read in: the sheet of the legacy twin has a rule for the bar
            // (a font a widget's own sheet sets outranks the application's `font-size: 1px`), the flat twin the font of the widget.
            legacy->setStyleSheet(QStringLiteral("QProgressBar { font-size: 11px; }"));
            QFont font = flat->font();
            font.setPixelSize(11);
            flat->setFont(font);
            Failures failures;
            for (const Qt::Alignment alignment : { Qt::Alignment(Qt::AlignCenter), Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter) })
            {
                for (ExposedBar* bar : { legacy, flat })
                {
                    bar->setTextVisible(true);
                    bar->setAlignment(alignment);
                    bar->setRange(0, 100);
                    bar->setValue(70);
                }
                EXPECT_EQ(flat->fontMetrics().height(), legacy->fontMetrics().height());
                for (const qreal ratio : ViewRatios)
                {
                    const QImage legacyImage = paintedBar(legacy, QSize(120, 16), ratio);
                    failures.check("alignment " + std::to_string(int(alignment)) + " at ratio " + std::to_string(ratio).substr(0, 3),
                        paintedBar(flat, QSize(120, 16), ratio), legacyImage);

                    // The text is drawn: the bar without it is another image
                    legacy->setTextVisible(false);
                    EXPECT_GT(differing(legacyImage, paintedBar(legacy, QSize(120, 16), ratio)), 0);
                    legacy->setTextVisible(true);
                }
            }
        }
    } // namespace ViewsFlatSheetTestsInternal
} // namespace AzQtComponents
