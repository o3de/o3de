/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/Elements/MenuElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Tests/SheetTwins.h>

#include "UiEditorFlat.h"
#include "Widgets/HierarchyWidget/HierarchyMenu.h"
#include "Widgets/HierarchyWidget/HierarchyWidget.h"
#include "Widgets/PreviewActionWidget/PreviewActionLog.h"
#include "Windows/EditorCommon.h"

#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QImage>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QRegion>
#include <QString>
#include <QWidget>

#include <memory>
#include <vector>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The sheets of the UI Editor (Gems/LyShine/Code/Editor), converted: the disabled-item grey of its menus (EditorCommon.h:50 before the
    // flattening, set by seven menu sites and the Hierarchy menu) and the fill of the preview mode's action log (PreviewActionLog.cpp:28).
    // The twins are built from the real classes where they can be made without an EditorWindow (HierarchyMenu, PreviewActionLog) and from
    // the sites' own lines where they cannot (the menus of EditorWindow's menu bar, built by a member of a window): the legacy twin is
    // Qt's own style sheet style over the application's style with the real application sheet (BaseStyleSheet.qss and the files it
    // imports) and the sheet quoted below on the widget, as the site set it; the flat twin is the flattened Style with no sheet at all.
    // ----------------------------------------------------------------------------------

    namespace UiEditorFlatSheetTestsInternal
    {
        namespace Twins = AzQtComponents::SheetTwins;

        // EditorCommon.h:50 before the flattening, the sheet the menus gave themselves
        constexpr const char* MenuRule = "QMenu::item:disabled { color: rgb(127, 127, 127); }";

        // PreviewActionLog.cpp:28 before the flattening, the sheet the log gave itself
        constexpr const char* LogRule = "background-color: #222222;";

        constexpr QColor LogFill(0x22, 0x22, 0x22);
        constexpr qreal Ratios[] = { 1.0, 1.5, 2.0 };

        // ------------------------------------------------------------------------------
        // The menus
        // ------------------------------------------------------------------------------

        //! A menu of the menu bar as EditorMenu.cpp makes it (`menuBar()->addMenu(...)`, then the helper), with a disabled item and an
        //! enabled one: the bar is the widget the menu hangs from
        QWidget* MakeBarMenu(QWidget* bar, const QString& title)
        {
            QMenu* menu = static_cast<QMenuBar*>(bar)->addMenu(title);
            UiEditorFlat::SetPopupMenuSheet(menu);
            menu->addAction(QStringLiteral("Undo"))->setEnabled(false);
            menu->addAction(QStringLiteral("New Canvas"));
            menu->addAction(QStringLiteral("Redo"))->setEnabled(false);
            return menu;
        }

        QWidget* MakeMenuBar(QWidget* parent)
        {
            return new QMenuBar(parent);
        }

        //! The right click menu of the Hierarchy dock, made by the real class: with nothing selected its cut, copy, duplicate and select
        //! items are disabled (HierarchyMenu.cpp:100-250). One enabled item is added after them, for the contrast. The real menu is a
        //! window without a parent, which the twin's root would not give its sheets and style: the twin holds it as a window below
        //! the root.
        QWidget* MakeHierarchyMenu(HierarchyWidget* hierarchy, QWidget* parent)
        {
            auto* menu = new HierarchyMenu(hierarchy, HierarchyMenu::kCutCopyPaste, false);
            menu->addAction(QStringLiteral("An enabled item"));
            menu->setParent(parent, menu->windowFlags());
            return menu;
        }

        //! The Align sub-menu of the Edit menu (EditorMenu.cpp:449, `menu->addMenu(tr("Align"))`): below the menu that was given the sheet,
        //! with the disabled items it shows when nothing is selected
        QWidget* MakeAlignSubMenu(QWidget* editMenu)
        {
            QMenu* align = qobject_cast<QMenu*>(editMenu)->addMenu(QStringLiteral("Align"));
            align->addAction(QStringLiteral("Align Left"))->setEnabled(false);
            align->addAction(QStringLiteral("Align Right"))->setEnabled(false);
            align->addAction(QStringLiteral("Align Top"));
            return align;
        }

        //! A menu with a sheet that shows nothing: the menus of EditorMenu.cpp:931 and :969 and PreviewAnimationList.cpp:98, whose
        //! items are always enabled, route the sheet to the legacy GUI alone (no element, no tag). Their line is the one below.
        QWidget* MakeAlwaysEnabledMenu(QWidget* bar)
        {
            QMenu* menu = static_cast<QMenuBar*>(bar)->addMenu(QStringLiteral("&Help"));
            AzQtComponents::StyleManager::setLegacyStyleSheet(menu, UICANVASEDITOR_QMENU_ITEM_DISABLED_STYLESHEET);
            menu->addAction(QStringLiteral("Documentation"));
            menu->addAction(QStringLiteral("Tutorials"));
            menu->addAction(QStringLiteral("Forum"));
            return menu;
        }

        Twins::Row MenuRow(const char* site, Twins::WidgetFactory make)
        {
            Twins::Row row;
            row.m_site = site;
            row.m_make = make;
            row.m_realApplicationSheet = true;
            // The menu as its paint event draws it, at each device ratio: the picture holds the size of the menu, its fill, and the
            // colour of each item's text, which is all the sheet decided
            row.m_compare = Twins::Compare::Pixels;
            // The base of the sheet style is the AzQtComponents style, as the application's chain is (StyleManager.cpp:346-351)
            row.m_base = Twins::Base::AzQtComponentsStyle;
            return row;
        }

        // ------------------------------------------------------------------------------
        // The action log
        // ------------------------------------------------------------------------------

        //! The real log, with two lines of text, held by the twin's root. The log is made without a parent (the window is the constructor's
        //! parameter, and the log ignores it)
        QWidget* MakeActionLog(QWidget* parent)
        {
            auto* log = new PreviewActionLog(nullptr);
            log->setParent(parent);
            log->setTextColor(QColor(Qt::white));
            log->append(QStringLiteral("'Pressed' from element Button (Element ID = 12)"));
            log->append(QStringLiteral("'Released' from element Button (Element ID = 12)"));
            log->show();
            return log;
        }

        //! The log with the old sheet quoted on it. Without the quoted sheet (m_sheets cleared) the legacy twin keeps what the class sets.
        Twins::Row LogRow()
        {
            Twins::Row row;
            row.m_site = "C160 the action log of the preview mode";
            row.m_make = &MakeActionLog;
            row.m_sheets = { { "PreviewActionLog.cpp:28 before the flattening", Twins::SetOn::Widget, LogRule } };
            row.m_realApplicationSheet = true;
            row.m_size = QSize(200, 100);
            row.m_base = Twins::Base::AzQtComponentsStyle;
            return row;
        }

        //! The log as a twin builds it, rendered at each ratio, and the colour of the viewport low right, where it shows no text
        std::vector<QImage> RenderLog(const Twins::Row& row, Twins::Context& context, bool legacy)
        {
            Twins::RungScope rung(legacy ? row.m_legacyRung : Twins::Rung::Flat);
            const Twins::Twin twin(row, context, legacy);
            QWidget* log = twin.widget();

            std::vector<QImage> images;
            for (const qreal ratio : Ratios)
            {
                QImage image((QSizeF(log->size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
                image.setDevicePixelRatio(ratio);
                image.fill(QColor(0x80, 0x80, 0x80));
                log->render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
                images.push_back(image);
            }
            return images;
        }

        QColor ViewportCorner(const QImage& image)
        {
            const int inset = qRound(12 * image.devicePixelRatio());
            return image.pixelColor(image.width() - inset, image.height() - inset);
        }

        // ------------------------------------------------------------------------------
        // The fixture
        // ------------------------------------------------------------------------------

        class UiEditorFlatSheetTests
            : public ::testing::Test
        {
        protected:
            UiEditorFlatSheetTests()
            {
                // The Hierarchy menu asks the clipboard whether it holds a canvas element (ClipboardContainsOurDataType,
                // EditorCommon.cpp), and reads the mime data it gets without a check. The offscreen platform the suites run on holds
                // none until something is set, so the test sets an empty one (a desktop platform always has one) and clears it in the
                // end: left set, the clipboard of the offscreen platform fails when the process exits.
                QClipboard* clipboard = QApplication::clipboard();
                if (!clipboard->mimeData())
                {
                    clipboard->setMimeData(new QMimeData);
                    m_clipboardSet = true;
                }
            }

            ~UiEditorFlatSheetTests() override
            {
                if (m_clipboardSet)
                {
                    QApplication::clipboard()->clear();
                }
            }

            //! The hierarchy the menu's actions point at. The menu reads the selection of it (none) and nothing else of it, so a hierarchy
            //! with no editor window is enough. It is a window that is never shown.
            HierarchyWidget* Hierarchy()
            {
                if (!m_hierarchy)
                {
                    m_hierarchy.reset(new HierarchyWidget(nullptr));
                }
                return m_hierarchy.get();
            }

            Twins::Context m_context;
            std::unique_ptr<HierarchyWidget> m_hierarchy;       // after the context: gone before it
            bool m_clipboardSet = false;
        };
    } // namespace UiEditorFlatSheetTestsInternal

    // A unity build joins this file with others: the fixture is the only name brought in, the rest is named through the alias
    using UiEditorFlatSheetTestsInternal::UiEditorFlatSheetTests;
    namespace UiTests = UiEditorFlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // The menus: which of them have the element, and that it draws what the sheet drew
    // ----------------------------------------------------------------------------------

    TEST_F(UiEditorFlatSheetTests, Registry_TheTaggedMenuAndTheMenusBelowItHaveTheElementAndNoOtherMenuDoes)
    {
        // The sheet on a menu reached the menu and the menus below it (the sub-menus a menu keeps are its children), and nothing else
        UiTests::Twins::RungScope flat(UiTests::Twins::Rung::Flat);
        QWidget root;
        auto* tagged = new QMenu(&root);
        UiEditorFlat::SetPopupMenuSheet(tagged);
        QMenu* sub = tagged->addMenu(QStringLiteral("Sub-menu"));
        QMenu* subOfSub = sub->addMenu(QStringLiteral("Sub-menu of the sub-menu"));
        QMenu other(&root);
        QMenu alone;
        auto* holder = new QWidget(tagged);
        QMenu belowAWidget(holder);

        const AzQtComponents::MenuElement& global = AzQtComponents::MenuElement::forWidget(nullptr);
        const AzQtComponents::MenuElement& element = AzQtComponents::MenuElement::forWidget(tagged);

        EXPECT_STREQ(element.m_name, UiEditorFlat::PopupMenuElementName);
        EXPECT_NE(&element, &global);
        EXPECT_EQ(&AzQtComponents::MenuElement::forWidget(sub), &element);
        EXPECT_EQ(&AzQtComponents::MenuElement::forWidget(subOfSub), &element);
        EXPECT_EQ(&AzQtComponents::MenuElement::forWidget(&other), &global);
        EXPECT_EQ(&AzQtComponents::MenuElement::forWidget(&alone), &global);
        EXPECT_EQ(&AzQtComponents::MenuElement::forWidget(&belowAWidget), &global);

        // Only the menu the site tagged has the tag: the menus below it have the element through their parents
        EXPECT_TRUE(AzQtComponents::Selectors::tagged(tagged, UiEditorFlat::PopupMenuTag));
        EXPECT_FALSE(AzQtComponents::Selectors::tagged(sub, UiEditorFlat::PopupMenuTag));
        EXPECT_FALSE(AzQtComponents::Selectors::tagged(&other, UiEditorFlat::PopupMenuTag));
    }

    TEST_F(UiEditorFlatSheetTests, Element_IsThePopupWithTheGreyOfTheSheetAndNothingElseChanged)
    {
        // `QMenu::item:disabled { color }` of the menus' own sheet, in place of Menu.qss:25: the one value the element changes
        UiTests::Twins::RungScope flat(UiTests::Twins::Rung::Flat);
        QWidget root;
        auto* tagged = new QMenu(&root);
        UiEditorFlat::SetPopupMenuSheet(tagged);

        const AzQtComponents::MenuElement& global = AzQtComponents::MenuElement::forWidget(nullptr);
        const AzQtComponents::MenuElement& element = AzQtComponents::MenuElement::forWidget(tagged);
        ASSERT_NE(&element, &global);

        EXPECT_EQ(element.m_disabledText, UiEditorFlat::DisabledText);
        EXPECT_EQ(element.m_disabledText, QColor(127, 127, 127));
        EXPECT_NE(global.m_disabledText, UiEditorFlat::DisabledText);

        EXPECT_EQ(element.m_surface, global.m_surface);
        EXPECT_EQ(element.m_margin, global.m_margin);
        EXPECT_EQ(element.m_paddingH, global.m_paddingH);
        EXPECT_EQ(element.m_paddingV, global.m_paddingV);
        EXPECT_EQ(element.m_text, global.m_text);
        EXPECT_EQ(element.m_selectedFill, global.m_selectedFill);
        EXPECT_EQ(element.m_itemMargin, global.m_itemMargin);
        EXPECT_EQ(element.m_separatorMarginTop, global.m_separatorMarginTop);
        EXPECT_EQ(element.m_separatorBorder, global.m_separatorBorder);
        EXPECT_EQ(element.m_separatorMarginBottom, global.m_separatorMarginBottom);
        EXPECT_EQ(element.m_separatorLine, global.m_separatorLine);
    }

    TEST_F(UiEditorFlatSheetTests, Legacy_TheMenuHasTheSheetItAlwaysHadAndNoTag)
    {
        // The legacy GUI's sheet text is written from the named value now: the same rule as the one quoted, `rgb(127, 127, 127)` being
        // the colour `#7f7f7f` names
        UiTests::Twins::RungScope legacy(UiTests::Twins::Rung::Qss);
        QWidget root;
        auto* menu = new QMenu(&root);
        UiEditorFlat::SetPopupMenuSheet(menu);

        EXPECT_EQ(menu->styleSheet(), QStringLiteral("QMenu::item:disabled { color: #7f7f7f; }"));
        EXPECT_EQ(UiEditorFlat::DisabledText.name(), QStringLiteral("#7f7f7f"));
        EXPECT_FALSE(AzQtComponents::Selectors::tagged(menu, UiEditorFlat::PopupMenuTag));
    }

    TEST_F(UiEditorFlatSheetTests, Flat_TheMenuHasTheTagAndNoSheet)
    {
        UiTests::Twins::RungScope flat(UiTests::Twins::Rung::Flat);
        QWidget root;
        auto* menu = new QMenu(&root);
        UiEditorFlat::SetPopupMenuSheet(menu);

        EXPECT_TRUE(menu->styleSheet().isEmpty());
        EXPECT_TRUE(AzQtComponents::Selectors::tagged(menu, UiEditorFlat::PopupMenuTag));
    }

    TEST_F(UiEditorFlatSheetTests, Menu_TheHierarchyMenuDrawsItsDisabledItemsInTheGreyOfItsSheet)
    {
        // C159: the real class, in either GUI, with the items disabled that nothing selected disables, and one enabled item
        HierarchyWidget* hierarchy = Hierarchy();
        UiTests::Twins::Row row = UiTests::MenuRow("C159 the Hierarchy context menu", [hierarchy](QWidget* parent)
            {
                return UiTests::MakeHierarchyMenu(hierarchy, parent);
            });
        row.m_sheets = { { "EditorCommon.h:50 before the flattening", UiTests::Twins::SetOn::Widget, UiTests::MenuRule } };
        EXPECT_TRUE(UiTests::Twins::check(row, m_context));
    }

    TEST_F(UiEditorFlatSheetTests, Menu_AMenuBarMenuDrawsItsDisabledItemsInTheGreyOfItsSheet)
    {
        // C161-C164: a menu of the menu bar, given the sheet where it is made
        UiTests::Twins::Row row = UiTests::MenuRow("C162 the Edit menu", [](QWidget* bar)
            {
                return UiTests::MakeBarMenu(bar, QStringLiteral("&Edit"));
            });
        row.m_chain = { { .m_name = "menuBar", .m_make = &UiTests::MakeMenuBar } };
        row.m_sheets = { { "EditorCommon.h:50 before the flattening", UiTests::Twins::SetOn::Widget, UiTests::MenuRule } };
        EXPECT_TRUE(UiTests::Twins::check(row, m_context));
    }

    TEST_F(UiEditorFlatSheetTests, Menu_ASubMenuDrawsItsDisabledItemsInTheGreyOfItsParentsSheet)
    {
        // C162: the sheet of the Edit menu reached the Align menu below it; the flattened GUI tagged the Edit menu alone
        UiTests::Twins::Row row = UiTests::MenuRow("C162 the Align sub-menu of the Edit menu", &UiTests::MakeAlignSubMenu);
        row.m_chain = { { .m_name = "menuBar", .m_make = &UiTests::MakeMenuBar },
                        { .m_name = "editMenu", .m_make = [](QWidget* bar)
                            {
                                return UiTests::MakeBarMenu(bar, QStringLiteral("&Edit"));
                            } } };
        row.m_sheets = { { "EditorCommon.h:50 before the flattening", UiTests::Twins::SetOn::Ancestor, UiTests::MenuRule, 1 } };
        EXPECT_TRUE(UiTests::Twins::check(row, m_context));
    }

    TEST_F(UiEditorFlatSheetTests, Menu_AMenuOfEnabledItemsDrawsAsItDoesUnderTheSheetAndHasNoElement)
    {
        // C165, C166, C167 (D.17): every item of these menus is enabled, so the rule has nothing to show; the sheet is the legacy GUI's
        // alone and the flattened menu has the Global popup, drawn the same
        UiTests::Twins::Row row = UiTests::MenuRow("C166 the Help menu", &UiTests::MakeAlwaysEnabledMenu);
        row.m_chain = { { .m_name = "menuBar", .m_make = &UiTests::MakeMenuBar } };
        row.m_sheets = { { "EditorCommon.h:50 before the flattening", UiTests::Twins::SetOn::Widget, UiTests::MenuRule } };
        EXPECT_TRUE(UiTests::Twins::check(row, m_context));

        UiTests::Twins::RungScope flat(UiTests::Twins::Rung::Flat);
        QWidget root;
        auto* bar = new QMenuBar(&root);
        const QWidget* menu = UiTests::MakeAlwaysEnabledMenu(bar);
        EXPECT_TRUE(menu->styleSheet().isEmpty());
        EXPECT_FALSE(AzQtComponents::Selectors::tagged(menu, UiEditorFlat::PopupMenuTag));
        EXPECT_EQ(&AzQtComponents::MenuElement::forWidget(menu), &AzQtComponents::MenuElement::forWidget(nullptr));
    }

    // ----------------------------------------------------------------------------------
    // The action log: the fill of the sheet, handed over as the colour of the area
    // ----------------------------------------------------------------------------------

    TEST_F(UiEditorFlatSheetTests, ActionLog_TheViewportIsTheFillOfTheSheetAtEveryRatioInBothGuis)
    {
        for (const bool legacy : { true, false })
        {
            const std::vector<QImage> images = UiTests::RenderLog(UiTests::LogRow(), m_context, legacy);
            ASSERT_EQ(images.size(), size_t(3));
            for (const QImage& image : images)
            {
                EXPECT_EQ(UiTests::ViewportCorner(image), UiTests::LogFill)
                    << (legacy ? "legacy" : "flat") << ", ratio " << image.devicePixelRatio();
            }
        }
    }

    TEST_F(UiEditorFlatSheetTests, ActionLog_TheLegacyGuisFillIsTheOldSheetsFill)
    {
        // The class sets the legacy sheet through the hand-over now (`background: #222222`), the sheet before was
        // `background-color: #222222;`: the same fill, drawn the same, at every ratio
        UiTests::Twins::Row quoted = UiTests::LogRow();
        UiTests::Twins::Row handedOver = UiTests::LogRow();
        handedOver.m_sheets.clear();

        const std::vector<QImage> before = UiTests::RenderLog(quoted, m_context, true);
        const std::vector<QImage> now = UiTests::RenderLog(handedOver, m_context, true);
        ASSERT_EQ(before.size(), now.size());
        for (size_t i = 0; i < before.size(); ++i)
        {
            EXPECT_EQ(AzQtComponents::FieldTwins::differing(before[i], now[i]), 0)
                << "ratio " << UiTests::Ratios[i] << ": " << AzQtComponents::FieldTwins::firstDifference(before[i], now[i]);
        }
    }

    TEST_F(UiEditorFlatSheetTests, ActionLog_TheFlatLogHasNoSheetAndTheFillIsTheColourOfItsArea)
    {
        const UiTests::Twins::Row row = UiTests::LogRow();
        UiTests::Twins::RungScope flat(UiTests::Twins::Rung::Flat);
        const UiTests::Twins::Twin twin(row, m_context, false);
        const QWidget* log = twin.widget();

        EXPECT_TRUE(log->styleSheet().isEmpty());
        EXPECT_FALSE(log->style()->inherits("QStyleSheetStyle"));
        for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
        {
            EXPECT_EQ(log->palette().color(group, QPalette::Base), UiTests::LogFill) << "group " << int(group);
            EXPECT_EQ(log->palette().color(group, QPalette::Window), UiTests::LogFill) << "group " << int(group);
        }
    }
} // namespace UnitTest
