/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/FieldTwins.h>

#include <AzQtComponents/Components/Elements/MenuElement.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>

#include <QAction>
#include <QCheckBox>
#include <QFontInfo>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPixmap>
#include <QWidgetAction>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The popup menus that gave themselves a fill with an inline sheet: the standard context menus of the entity search box
    // (EntityPropertyEditor.cpp:753), of FilteredSearchWidget's text search (FilteredSearchWidget.cpp:1412) and of the search box of the
    // asset selection grid (AssetSelectionGrid.cpp:266), `menu->setStyleSheet("background-color: #333333")`; and the asset type menu of
    // SearchAssetTypeSelectorWidget (SearchAssetTypeSelectorWidget.cpp:87) and the graph type menu of GraphTypeSelector
    // (GraphTypeSelector.cpp:31), `menu->setStyleSheet("border: none; background-color: #333333;")`. All the forms before the flattening.
    //
    // The legacy twin is Qt's own style sheet style over Fusion: the menu below a root that carries the application's real sheet
    // (FieldTwins::appSheet; the cascade the engine has, in which a widget's own sheet outranks the application's), with the menu's own
    // inline sheet. The rules of the application's sheet that reach the menu and what it hosts:
    //   BaseStyleSheet.qss:15-21   * { background-color: transparent; color: white; font-family; font-weight }
    //   Menu.qss:10-14             QMenu { background-color: #222222; padding: 4px 2px }
    //   Menu.qss:17-21             QMenu::item:selected, QMenu::item .WidgetAction:hover { background-color: #444444 }
    //   Menu.qss:23-26             QMenu::item:disabled { color: #555555 }
    //   Menu.qss:28-34             QMenu::separator { height: 0px; border-bottom: 1px solid #454545; margin-top: 2px; margin-bottom: 3px }
    //   CheckBox.qss:10-16, :18-21, :23-28   a hosted QCheckBox: transparent, its text size, its indicator
    //   Text.qss:89-95             a hosted QLabel: font, margin-top and margin-bottom of 1px
    // The flat twin is the flattened Style over Fusion, with the menu given its sheet as the call sites give it
    // (MenuElement::setFilledPopupSheet, which sets no sheet there and marks the menu).
    // ----------------------------------------------------------------------------------

    namespace FilledPopupFlatSheetTestsInternal
    {
        using namespace FieldTwins;

        //! The inline sheets of the five menus
        struct InlineSheet
        {
            const char* m_name;
            const char* m_text;
        };

        const InlineSheet InlineSheets[] = {
            { "fill only", "background-color: #333333" },
            { "fill and no border", "border: none; background-color: #333333;" },
        };

        constexpr QRgb FilledSurface = 0x333333;        // the inline sheets' fill
        constexpr QRgb GlobalSurface = 0x222222;        // Menu.qss:12, what a menu without the sheet is filled with
        constexpr QRgb SelectedFill = 0x444444;         // Menu.qss:20
        constexpr QRgb SeparatorLine = 0x454545;        // Menu.qss:31

        const qreal Ratios[] = { 1.0, 1.5, 2.0 };

        //! QMenu::initStyleOption is protected: the option a paint event builds for an action
        class TestMenu
            : public QMenu
        {
        public:
            using QMenu::QMenu;
            using QMenu::initStyleOption;
        };

        //! The rung a test runs at for its scope, and the rung the fixture runs at after it
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

        private:
            GuiPerfFloor::Rung m_before;
        };

        class FilledPopupFlatSheetTests
            : public Fixture
        {
        };

        // ------------------------------------------------------------------------------
        // What the menus hold
        // ------------------------------------------------------------------------------

        //! A menu that has every look of an item: plain, with a mnemonic, with a shortcut, disabled, checkable (checked and not), with an
        //! icon, checked with an icon, a submenu, and separators between them
        void fillMenu(QMenu& menu)
        {
            QPixmap pixmap(16, 16);
            pixmap.fill(QColor(0xC8, 0x3C, 0x3C));
            const QIcon icon(pixmap);

            menu.addAction(QStringLiteral("Plain item"));
            menu.addAction(QStringLiteral("Toggle &Breakpoint"));
            menu.addAction(QStringLiteral("With shortcut"))->setShortcut(QKeySequence(QStringLiteral("Ctrl+K")));
            menu.addSeparator();
            menu.addAction(QStringLiteral("Disabled item"))->setEnabled(false);
            QAction* unchecked = menu.addAction(QStringLiteral("Unchecked item"));
            unchecked->setCheckable(true);
            QAction* checked = menu.addAction(QStringLiteral("Checked item"));
            checked->setCheckable(true);
            checked->setChecked(true);
            menu.addSeparator();
            menu.addAction(icon, QStringLiteral("Item with icon"));
            QAction* checkedIcon = menu.addAction(icon, QStringLiteral("Checked item with icon"));
            checkedIcon->setCheckable(true);
            checkedIcon->setChecked(true);
            menu.addAction(QStringLiteral("Submenu"))->setMenu(new QMenu(&menu));
        }

        //! What SearchAssetTypeSelectorWidget hosts (AddAllAction, AddAssetTypeGroup): check boxes and a label as widget actions
        void fillHostedMenu(QMenu& menu)
        {
            const auto host = [&menu](QWidget* widget)
            {
                auto* action = new QWidgetAction(&menu);
                action->setDefaultWidget(widget);
                menu.addAction(action);
            };

            auto* filterBy = new QLabel(QStringLiteral("Filter by:"), &menu);
            filterBy->setObjectName(QStringLiteral("filterBy"));
            host(filterBy);
            menu.addSeparator();
            auto* material = new QCheckBox(QStringLiteral("Material"), &menu);
            material->setObjectName(QStringLiteral("material"));
            host(material);
            auto* mesh = new QCheckBox(QStringLiteral("Mesh"), &menu);
            mesh->setObjectName(QStringLiteral("mesh"));
            mesh->setChecked(true);
            host(mesh);
            QMenu* group = menu.addMenu(QStringLiteral("GS"));
            auto* core = new QCheckBox(QStringLiteral("Core"), group);
            core->setObjectName(QStringLiteral("core"));
            auto* coreAction = new QWidgetAction(group);
            coreAction->setDefaultWidget(core);
            group->addAction(coreAction);
        }

        // ------------------------------------------------------------------------------
        // The twins
        // ------------------------------------------------------------------------------

        //! The menu of the legacy GUI: below a root that carries the application's sheet, with its own inline sheet
        struct LegacyTree
        {
            LegacyTree(const char* inlineSheet, void (*build)(QMenu&))
            {
                m_menu = new TestMenu(&m_root);
                build(*m_menu);
                m_menu->setStyleSheet(QString::fromLatin1(inlineSheet));
                Fixture::legacy(m_root);
                m_menu->ensurePolished();
            }

            QWidget m_root;
            TestMenu* m_menu = nullptr;
        };

        //! The menu of the flattened GUI: the style on it and on everything below it, given the inline sheet as the call sites give it
        //! (which sets no sheet and marks the menu), or left as it is for a null sheet
        struct FlatTree
        {
            FlatTree(const Fixture& fixture, void (*build)(QMenu&), const char* inlineSheet)
            {
                m_menu = new TestMenu(&m_root);
                build(*m_menu);
                fixture.flat(m_root);
                if (inlineSheet)
                {
                    MenuElement::setFilledPopupSheet(m_menu, QString::fromLatin1(inlineSheet));
                }
                m_menu->ensurePolished();
            }

            QWidget m_root;
            TestMenu* m_menu = nullptr;
        };

        // ------------------------------------------------------------------------------
        // Reading what a twin drew
        // ------------------------------------------------------------------------------

        QRgb pixelOf(const QImage& image, int x, int y)
        {
            return image.pixel(x, y) & 0xFFFFFF;
        }

        //! The menu as its paint event draws it, onto an image of an explicit device ratio (QWidget::render would hand the style the ratio
        //! of the screen the test runs on): the panel (PE_PanelMenu), then each item with its own clip. `selected` is the item the pointer
        //! is on, none for null.
        QImage paintMenu(TestMenu& menu, const QAction* selected, qreal ratio)
        {
            const QSize size = menu.sizeHint();
            menu.resize(size);

            QImage image((QSizeF(size) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor(0x80, 0x80, 0x80));
            QPainter painter(&image);

            QStyleOptionMenuItem panel;
            panel.initFrom(&menu);
            panel.state = QStyle::State_None;
            menu.style()->drawPrimitive(QStyle::PE_PanelMenu, &panel, &painter, &menu);

            for (QAction* action : menu.actions())
            {
                QStyleOptionMenuItem option;
                menu.initStyleOption(&option, action);
                option.rect = menu.actionGeometry(action);
                if (action == selected)
                {
                    option.state |= QStyle::State_Selected;
                }
                painter.save();
                painter.setClipRect(option.rect);
                menu.style()->drawControl(QStyle::CE_MenuItem, &option, &painter, &menu);
                painter.restore();
            }
            painter.end();
            return image;
        }

        //! A widget a menu hosts as it paints itself over the panel behind it
        QImage paintHosted(QWidget& widget, const QSize& size)
        {
            widget.ensurePolished();
            widget.resize(size);
            QImage image(size, QImage::Format_ARGB32_Premultiplied);
            image.fill(QColor::fromRgb(FilledSurface));
            widget.render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
            return image;
        }

        //! Whether an action can be the one the pointer is on: a separator never is (QMenu::initStyleOption), a disabled item is when the
        //! pointer moves over it, and the standard menu of a line edit shows its disabled rows (Undo, Cut, Copy, Delete of an empty box)
        bool canBeSelected(const QAction* action)
        {
            return !action->isSeparator();
        }

        //! The color of a role in a group as text, for the failure message of a comparison
        std::string roleText(const QWidget& widget, QPalette::ColorGroup group, QPalette::ColorRole role)
        {
            return widget.palette().color(group, role).name(QColor::HexArgb).toStdString();
        }

        // ------------------------------------------------------------------------------
        // T-FILLED.1: which menus have the element
        // ------------------------------------------------------------------------------

        TEST_F(FilledPopupFlatSheetTests, Registry_AMarkedMenuAndEveryMenuBelowItHaveTheFilledPopup)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            QWidget root;
            auto* menu = new TestMenu(&root);
            QMenu* group = menu->addMenu(QStringLiteral("GS"));
            QMenu* leaf = group->addMenu(QStringLiteral("Core"));
            flat(root);
            MenuElement::markFilledPopup(menu);

            for (QMenu* each : { static_cast<QMenu*>(menu), group, leaf })
            {
                each->ensurePolished();
                EXPECT_EQ(&MenuElement::forWidget(each), &MenuElement::filledPopup()) << each->title().toStdString();
                EXPECT_STREQ(MenuElement::forWidget(each).m_name, "Global.Menu.FilledPopup");

                // Polish gave the widgets the menu hosts the surface, as it does for every popup (MenuElement::resolve)
                EXPECT_EQ(roleText(*each, QPalette::Active, QPalette::Window), "#ff333333") << each->title().toStdString();
                EXPECT_EQ(roleText(*each, QPalette::Active, QPalette::Base), "#ff333333") << each->title().toStdString();
            }
        }

        TEST_F(FilledPopupFlatSheetTests, Registry_NoOtherMenuHasIt)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            QWidget root;
            auto* marked = new TestMenu(&root);
            marked->addMenu(QStringLiteral("Below"));
            auto* sibling = new TestMenu(&root);                    // another menu of the same parent: the sheet was on one menu
            QMenu* siblingBelow = sibling->addMenu(QStringLiteral("Below"));
            auto* above = new TestMenu(&root);                      // a menu above a marked one: the sheet reached down only
            QMenu* markedBelowAbove = above->addMenu(QStringLiteral("Marked"));
            QMenu alone;                                            // a menu with no parent at all
            flat(root);
            MenuElement::markFilledPopup(marked);
            MenuElement::markFilledPopup(markedBelowAbove);

            const MenuElement& global = MenuElement::forWidget(nullptr);
            EXPECT_STREQ(global.m_name, "Global.Menu.Popup");
            EXPECT_EQ(&MenuElement::forWidget(sibling), &global);
            EXPECT_EQ(&MenuElement::forWidget(siblingBelow), &global);
            EXPECT_EQ(&MenuElement::forWidget(above), &global);
            EXPECT_EQ(&MenuElement::forWidget(&alone), &global);
            EXPECT_EQ(&MenuElement::forWidget(marked), &MenuElement::filledPopup());
            EXPECT_EQ(&MenuElement::forWidget(markedBelowAbove), &MenuElement::filledPopup());

            // and the surface of those is the popup's own
            for (QMenu* each : { static_cast<QMenu*>(sibling), siblingBelow, static_cast<QMenu*>(above) })
            {
                each->ensurePolished();
                EXPECT_EQ(roleText(*each, QPalette::Active, QPalette::Window), "#ff222222") << each->title().toStdString();
            }
        }

        TEST_F(FilledPopupFlatSheetTests, Registry_ATagOnAWidgetThatIsNoMenuChangesNothing)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            QLabel label;
            label.setProperty("class", QStringLiteral("FilledPopup"));
            EXPECT_EQ(&MenuElement::forWidget(&label), &MenuElement::forWidget(nullptr));
        }

        TEST_F(FilledPopupFlatSheetTests, Mark_IsOneTagAmongTheClassesOfAMenuAndIsGivenOnce)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            QMenu menu;
            menu.setProperty("class", QStringLiteral("SourceControlMenu"));          // Menu.qss:36
            MenuElement::markFilledPopup(&menu);
            MenuElement::markFilledPopup(&menu);

            EXPECT_EQ(menu.property("class").toString().toStdString(), "SourceControlMenu FilledPopup");
            EXPECT_TRUE(Selectors::tagged(&menu, "SourceControlMenu"));
            EXPECT_TRUE(Selectors::tagged(&menu, "FilledPopup"));

            QMenu plain;
            MenuElement::markFilledPopup(&plain);
            EXPECT_EQ(plain.property("class").toString().toStdString(), "FilledPopup");

            MenuElement::markFilledPopup(nullptr);          // nothing to mark: no failure
        }

        // The one line of the five call sites (MenuElement::setFilledPopupSheet): the sheet text of each site is a literal next to its
        // citation, and these tests prove what the line does with it in each GUI

        TEST_F(FilledPopupFlatSheetTests, CallSite_TheLegacyGuiKeepsTheSheetAndLeavesTheMenuUnmarked)
        {
            const RungScope legacyGui(GuiPerfFloor::Rung::Qss);
            ASSERT_FALSE(StyleManager::stylesheetsDisabled());

            for (const InlineSheet& sheet : InlineSheets)
            {
                QWidget root;
                auto* menu = new QMenu(&root);
                QMenu* below = menu->addMenu(QStringLiteral("Below"));
                MenuElement::setFilledPopupSheet(menu, QString::fromLatin1(sheet.m_text));

                EXPECT_EQ(menu->styleSheet().toStdString(), std::string(sheet.m_text)) << sheet.m_name;
                EXPECT_FALSE(menu->property("class").isValid()) << sheet.m_name;
                EXPECT_TRUE(below->styleSheet().isEmpty()) << sheet.m_name;         // set on the one menu; the cascade reaches the submenu

                MenuElement::setFilledPopupSheet(nullptr, QString::fromLatin1(sheet.m_text));        // nothing to set: no failure
            }
        }

        TEST_F(FilledPopupFlatSheetTests, CallSite_TheFlattenedGuiSetsNoSheetAndMarksTheMenu)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            for (const InlineSheet& sheet : InlineSheets)
            {
                QWidget root;
                auto* menu = new TestMenu(&root);
                QMenu* group = menu->addMenu(QStringLiteral("GS"));
                QMenu* leaf = group->addMenu(QStringLiteral("Core"));
                auto* sibling = new TestMenu(&root);
                flat(root);
                MenuElement::setFilledPopupSheet(menu, QString::fromLatin1(sheet.m_text));

                EXPECT_TRUE(menu->styleSheet().isEmpty()) << sheet.m_name;
                EXPECT_EQ(menu->property("class").toString().toStdString(), "FilledPopup") << sheet.m_name;
                for (QMenu* each : { static_cast<QMenu*>(menu), group, leaf })
                {
                    EXPECT_EQ(&MenuElement::forWidget(each), &MenuElement::filledPopup())
                        << sheet.m_name << ", " << each->title().toStdString();
                }
                EXPECT_FALSE(group->property("class").isValid()) << sheet.m_name;          // the tag is on the one menu
                EXPECT_EQ(&MenuElement::forWidget(sibling), &MenuElement::forWidget(nullptr)) << sheet.m_name;

                MenuElement::setFilledPopupSheet(nullptr, QString::fromLatin1(sheet.m_text));        // nothing to set: no failure
            }
        }

        TEST_F(FilledPopupFlatSheetTests, Registry_TheStandardContextMenuOfALineEditIsFilledOnlyWhenMarked)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The menu of the three search boxes (EntityPropertyEditor, FilteredSearchWidget, AssetSelectionGrid): Qt makes it a child
            // of the line edit and names it, and the submenu it keeps (Insert Unicode control character, where the platform has one) is
            // its child
            QLineEdit edit;
            QMenu* unmarked = edit.createStandardContextMenu();
            QMenu* marked = edit.createStandardContextMenu();
            ASSERT_TRUE(unmarked != nullptr && marked != nullptr);
            EXPECT_EQ(marked->objectName().toStdString(), "qt_edit_menu");
            EXPECT_EQ(marked->parentWidget(), &edit);
            flat(edit);
            MenuElement::markFilledPopup(marked);

            EXPECT_EQ(&MenuElement::forWidget(unmarked), &MenuElement::forWidget(nullptr));
            EXPECT_EQ(&MenuElement::forWidget(marked), &MenuElement::filledPopup());
            for (QMenu* below : marked->findChildren<QMenu*>())
            {
                EXPECT_EQ(&MenuElement::forWidget(below), &MenuElement::filledPopup()) << below->metaObject()->className();
            }
            for (QMenu* below : unmarked->findChildren<QMenu*>())
            {
                EXPECT_EQ(&MenuElement::forWidget(below), &MenuElement::forWidget(nullptr)) << below->metaObject()->className();
            }
        }

        TEST_F(FilledPopupFlatSheetTests, Element_IsThePopupOnTheFillOfTheSheets)
        {
            const MenuElement& popup = MenuElement::forWidget(nullptr);
            const MenuElement& filled = MenuElement::filledPopup();

            EXPECT_STREQ(filled.m_name, "Global.Menu.FilledPopup");
            EXPECT_TRUE(filled.m_surface == QColor(0x33, 0x33, 0x33));              // the sheets' `background-color: #333333`
            EXPECT_TRUE(popup.m_surface == QColor(0x22, 0x22, 0x22));               // Menu.qss:12

            // Nothing else differs: the items, the separators and the padding are the popup's
            EXPECT_EQ(filled.m_paddingH, popup.m_paddingH);
            EXPECT_EQ(filled.m_paddingV, popup.m_paddingV);
            EXPECT_TRUE(filled.m_text == popup.m_text);
            EXPECT_TRUE(filled.m_selectedFill == popup.m_selectedFill);
            EXPECT_TRUE(filled.m_disabledText == popup.m_disabledText);
            EXPECT_EQ(filled.m_separatorMarginTop, popup.m_separatorMarginTop);
            EXPECT_EQ(filled.m_separatorBorder, popup.m_separatorBorder);
            EXPECT_EQ(filled.m_separatorMarginBottom, popup.m_separatorMarginBottom);
            EXPECT_TRUE(filled.m_separatorLine == popup.m_separatorLine);
            EXPECT_EQ(filled.separatorHeight(), popup.separatorHeight());

            // The palette the base style draws an item over: the surface for an idle item, the selected fill for the selected one
            const QPalette menuPalette;
            const QPalette selected = filled.itemPalette(menuPalette, true, true);
            EXPECT_TRUE(selected.color(QPalette::Highlight) == QColor(0x44, 0x44, 0x44));
            EXPECT_TRUE(selected.color(QPalette::Button) == QColor(0x44, 0x44, 0x44));
            const QPalette disabled = filled.itemPalette(menuPalette, false, false);
            EXPECT_TRUE(disabled.color(QPalette::ButtonText) == QColor(0x55, 0x55, 0x55));
            EXPECT_TRUE(disabled.color(QPalette::Dark) == QColor(0x33, 0x33, 0x33).darker(150));       // shaded from the menu's own surface
        }

        // ------------------------------------------------------------------------------
        // T-FILLED.2: the layout, the font and the palette the children inherit
        // ------------------------------------------------------------------------------

        TEST_F(FilledPopupFlatSheetTests, Geometry_TheMenuLaysItsItemsOutAsTheLegacyMenuDid)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            for (const InlineSheet& sheet : InlineSheets)
            {
                LegacyTree legacy(sheet.m_text, fillMenu);
                FlatTree flatTree(*this, fillMenu, sheet.m_text);
                TestMenu& expected = *legacy.m_menu;
                TestMenu& actual = *flatTree.m_menu;

                EXPECT_TRUE(actual.sizeHint() == expected.sizeHint()) << sheet.m_name;
                EXPECT_TRUE(actual.minimumSizeHint() == expected.minimumSizeHint()) << sheet.m_name;
                const QList<QAction*> expectedActions = expected.actions();
                const QList<QAction*> actualActions = actual.actions();
                ASSERT_EQ(expectedActions.size(), actualActions.size());
                for (int index = 0; index < expectedActions.size(); ++index)
                {
                    EXPECT_TRUE(actual.actionGeometry(actualActions[index]) == expected.actionGeometry(expectedActions[index]))
                        << sheet.m_name << ", action " << index;
                }

                // The contents box of the menu, and the margins that make it (PM_MenuPanelWidth, PM_MenuHMargin, PM_MenuVMargin)
                expected.resize(expected.sizeHint());
                actual.resize(actual.sizeHint());
                EXPECT_TRUE(actual.contentsMargins() == expected.contentsMargins()) << sheet.m_name;
                EXPECT_TRUE(actual.contentsRect() == expected.contentsRect()) << sheet.m_name;
                QStyleOption expectedOption;
                expectedOption.initFrom(&expected);
                QStyleOption actualOption;
                actualOption.initFrom(&actual);
                for (const QStyle::PixelMetric metric : { QStyle::PM_MenuPanelWidth, QStyle::PM_MenuHMargin, QStyle::PM_MenuVMargin })
                {
                    EXPECT_EQ(actual.style()->pixelMetric(metric, &actualOption, &actual),
                        expected.style()->pixelMetric(metric, &expectedOption, &expected))
                        << sheet.m_name << ", metric " << static_cast<int>(metric);
                }

                // A separator is the sheet's box (2px + 1px + 3px), whatever the menu's fill
                QStyleOptionMenuItem separator;
                separator.menuItemType = QStyleOptionMenuItem::Separator;
                separator.state = QStyle::State_Enabled;
                EXPECT_TRUE(actual.style()->sizeFromContents(QStyle::CT_MenuItem, &separator, QSize(2, 2), &actual)
                    == expected.style()->sizeFromContents(QStyle::CT_MenuItem, &separator, QSize(2, 2), &expected))
                    << sheet.m_name;
            }
        }

        TEST_F(FilledPopupFlatSheetTests, Font_TheMenuHasTheFontTheLegacyMenuHad)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            for (const InlineSheet& sheet : InlineSheets)
            {
                LegacyTree legacy(sheet.m_text, fillMenu);
                FlatTree flatTree(*this, fillMenu, sheet.m_text);
                const QFontInfo expected(legacy.m_menu->font());
                const QFontInfo actual(flatTree.m_menu->font());

                EXPECT_EQ(actual.family().toStdString(), expected.family().toStdString()) << sheet.m_name;
                EXPECT_EQ(actual.pixelSize(), expected.pixelSize()) << sheet.m_name;
                EXPECT_EQ(actual.weight(), expected.weight()) << sheet.m_name;
                EXPECT_EQ(actual.italic(), expected.italic()) << sheet.m_name;
            }
        }

        TEST_F(FilledPopupFlatSheetTests, Palette_TheSurfaceReachesTheRolesTheHostedWidgetsInherit)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The sheet's fill reached the roles a widget fills from (QRenderRule::configurePalette: Base, Button and the background
            // role); the flat menu hands its children Window and Base (MenuElement::resolve), the roles a child inherits
            for (const InlineSheet& sheet : InlineSheets)
            {
                LegacyTree legacy(sheet.m_text, fillMenu);
                FlatTree flatTree(*this, fillMenu, sheet.m_text);

                for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
                {
                    for (const QPalette::ColorRole role : { QPalette::Window, QPalette::Base })
                    {
                        EXPECT_EQ(roleText(*flatTree.m_menu, group, role), roleText(*legacy.m_menu, group, role))
                            << sheet.m_name << ", group " << static_cast<int>(group) << ", role " << static_cast<int>(role);
                    }
                }
            }
        }

        // ------------------------------------------------------------------------------
        // T-FILLED.3: the pixels, at the device ratios of the GUI
        // ------------------------------------------------------------------------------

        TEST_F(FilledPopupFlatSheetTests, Pixels_ThePanelAndEveryItemPaintWhatTheSheetPaintedAtEveryRatio)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            for (const InlineSheet& sheet : InlineSheets)
            {
                LegacyTree legacy(sheet.m_text, fillMenu);
                FlatTree flatTree(*this, fillMenu, sheet.m_text);
                const QList<QAction*> legacyActions = legacy.m_menu->actions();
                const QList<QAction*> flatActions = flatTree.m_menu->actions();
                ASSERT_EQ(legacyActions.size(), flatActions.size());
                ASSERT_TRUE(flatTree.m_menu->sizeHint() == legacy.m_menu->sizeHint()) << sheet.m_name;

                for (const qreal ratio : Ratios)
                {
                    // index -1: no item has the pointer. Every item but a separator can have it, a disabled item among them.
                    for (int index = -1; index < legacyActions.size(); ++index)
                    {
                        if (index >= 0 && !canBeSelected(legacyActions[index]))
                        {
                            continue;
                        }
                        const QImage expected = paintMenu(*legacy.m_menu, index < 0 ? nullptr : legacyActions[index], ratio);
                        const QImage actual = paintMenu(*flatTree.m_menu, index < 0 ? nullptr : flatActions[index], ratio);

                        const int different = differing(expected, actual);
                        EXPECT_EQ(different, 0) << sheet.m_name << ", item " << index << " at ratio " << ratio << ": "
                                                << (different ? firstDifference(expected, actual) : std::string());
                    }

                    // The twin paints something: the panel has the sheet's fill in its padding, and the item with the pointer the
                    // selected fill (an unmarked flat menu would paint the popup's)
                    const QImage legacyIdle = paintMenu(*legacy.m_menu, nullptr, ratio);
                    EXPECT_EQ(pixelOf(legacyIdle, 2, 2), FilledSurface) << sheet.m_name << " at ratio " << ratio;
                    QAction* first = legacyActions.first();
                    const QRect firstRect = legacy.m_menu->actionGeometry(first);
                    const QImage legacySelected = paintMenu(*legacy.m_menu, first, ratio);
                    const int x = qRound((firstRect.left() + 2) * ratio);
                    const int y = qRound((firstRect.top() + 2) * ratio);
                    EXPECT_EQ(pixelOf(legacySelected, x, y), SelectedFill) << sheet.m_name << " at ratio " << ratio;
                }
            }
        }

        TEST_F(FilledPopupFlatSheetTests, Pixels_AMenuWithoutTheSheetOrTheMarkIsFilledWithThePopupsOwnSurface)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The twins above are not two blanks of one colour: the same menu without the sheet, and without the mark, is #222222
            LegacyTree legacy("", fillMenu);
            FlatTree flatTree(*this, fillMenu, nullptr);
            for (const qreal ratio : Ratios)
            {
                const QImage expected = paintMenu(*legacy.m_menu, nullptr, ratio);
                const QImage actual = paintMenu(*flatTree.m_menu, nullptr, ratio);
                EXPECT_EQ(pixelOf(expected, 2, 2), GlobalSurface) << "ratio " << ratio;
                EXPECT_EQ(pixelOf(actual, 2, 2), GlobalSurface) << "ratio " << ratio;
                EXPECT_EQ(differing(expected, actual), 0) << "ratio " << ratio;
            }
        }

        TEST_F(FilledPopupFlatSheetTests, Pixels_TheSeparatorKeepsItsLineOnTheNewFill)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            FlatTree flatTree(*this, fillMenu, InlineSheets[0].m_text);
            const QList<QAction*> actions = flatTree.m_menu->actions();
            QAction* separator = nullptr;
            for (QAction* action : actions)
            {
                if (action->isSeparator())
                {
                    separator = action;
                    break;
                }
            }
            ASSERT_TRUE(separator != nullptr);

            // Menu.qss:28-34: the box has 2px above the line and 3px below it, the line is one row (#454545) at the bottom of the box
            const QImage image = paintMenu(*flatTree.m_menu, nullptr, 1.0);
            const QRect rect = flatTree.m_menu->actionGeometry(separator);
            EXPECT_EQ(pixelOf(image, rect.left() + 20, rect.top() + 1), FilledSurface);
            EXPECT_EQ(pixelOf(image, rect.left() + 20, rect.top() + 2), SeparatorLine);
            EXPECT_EQ(pixelOf(image, rect.left() + 20, rect.top() + 3), FilledSurface);
        }

        // ------------------------------------------------------------------------------
        // T-FILLED.4: what the menus host, which the sheet's rule reached
        // ------------------------------------------------------------------------------

        TEST_F(FilledPopupFlatSheetTests, Hosted_TheWidgetsOfAMenuPaintOverTheNewFillAsTheyDidUnderTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The rule had no selector, so it filled each widget the menu hosts, and each menu below it, with the colour of the menu:
            // under the flattened GUI the menu's panel is that colour, and the widgets paint nothing of their own below their content
            for (const InlineSheet& sheet : InlineSheets)
            {
                LegacyTree legacy(sheet.m_text, fillHostedMenu);
                FlatTree flatTree(*this, fillHostedMenu, sheet.m_text);

                for (const char* name : { "filterBy", "material", "mesh", "core" })
                {
                    QWidget* expected = legacy.m_menu->findChild<QWidget*>(QString::fromLatin1(name));
                    QWidget* actual = flatTree.m_menu->findChild<QWidget*>(QString::fromLatin1(name));
                    ASSERT_TRUE(expected != nullptr && actual != nullptr) << name;

                    // Both hosted by a menu, the one that holds "core" being the submenu
                    EXPECT_EQ(std::string(actual->parentWidget()->metaObject()->className()),
                        std::string(expected->parentWidget()->metaObject()->className())) << name;

                    expected->ensurePolished();
                    actual->ensurePolished();
                    const QSize size = expected->sizeHint();
                    EXPECT_TRUE(actual->sizeHint() == size) << sheet.m_name << ", " << name;

                    // The pixels over the panel behind the widget, and the box its contents lay out in (the sheet's rule had no box of
                    // its own: what a check box and a label have is the application's, Text.qss:93-94 for the label)
                    const QImage expectedImage = paintHosted(*expected, size);
                    const QImage actualImage = paintHosted(*actual, size);
                    const int different = differing(expectedImage, actualImage);
                    EXPECT_EQ(different, 0) << sheet.m_name << ", " << name << ": "
                                            << (different ? firstDifference(expectedImage, actualImage) : std::string());
                    EXPECT_TRUE(actual->contentsRect() == expected->contentsRect()) << sheet.m_name << ", " << name;
                    const auto* expectedFrame = qobject_cast<const QFrame*>(expected);
                    const auto* actualFrame = qobject_cast<const QFrame*>(actual);
                    if (expectedFrame && actualFrame)
                    {
                        const QStyleOptionFrame expectedOption = frameOption(expectedFrame);
                        const QStyleOptionFrame actualOption = frameOption(actualFrame);
                        EXPECT_TRUE(actual->style()->subElementRect(QStyle::SE_ShapedFrameContents, &actualOption, actual)
                            == expected->style()->subElementRect(QStyle::SE_ShapedFrameContents, &expectedOption, expected))
                            << sheet.m_name << ", " << name;
                    }
                }

                // The submenu that holds a hosted widget has the element too
                QMenu* group = flatTree.m_menu->findChild<QMenu*>();
                ASSERT_TRUE(group != nullptr);
                EXPECT_EQ(&MenuElement::forWidget(group), &MenuElement::filledPopup());
            }
        }
    } // namespace FilledPopupFlatSheetTestsInternal
} // namespace AzQtComponents
