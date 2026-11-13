/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Tests/SheetTwins.h>
#include <AzQtComponents/Tests/SiteTwins.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerFlatSheet.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerSearchWidget.h>

#include <QApplication>
#include <QFile>
#include <QFrame>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QStyleFactory>
#include <QToolButton>
#include <QTreeView>
#include <QWidget>

#include <string>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The elements of EntityOutliner.qss: which widgets of the Entity Outliner they reach, and what they set on them
    // ----------------------------------------------------------------------------------

    namespace EntityOutlinerFlatSheetTestsInternal
    {
        //! A widget that Qt takes for an instance of a class it is not. The matchers ask for a window by its class name, as
        //! the selectors do, and building a real EntityOutlinerWidget needs the Editor's whole entity system.
        class ClassNamedWidget
            : public QWidget
        {
        public:
            ClassNamedWidget(const char* className, QWidget* parent = nullptr)
                : QWidget(parent)
                , m_className(className)
            {
            }

            void* qt_metacast(const char* name) override
            {
                return qstrcmp(name, m_className) == 0 ? this : QWidget::qt_metacast(name);
            }

        private:
            const char* m_className;
        };

        constexpr const char* MenuIcon = ":/stylesheet/img/UI20/menu-centered.svg";     // EntityOutliner.qss:11
        constexpr const char* DisplayOptionsElementName = "Editor.Box.OutlinerDisplayOptions";

        class EntityOutlinerFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            void SetUp() override
            {
                LeakDetectionFixture::SetUp();
                m_style = AZStd::make_unique<AzQtComponents::Style>(QStyleFactory::create(QStringLiteral("Fusion")));

                // Once for the process, as the flattened GUI runs the registrar of a sheet once
                static const bool registered = (AzToolsFramework::EntityOutliner::RegisterFlatSheetElements(), true);
                ASSERT_TRUE(registered);
            }

            void TearDown() override
            {
                m_style.reset();
                LeakDetectionFixture::TearDown();
            }

            //! The flat way: the style on the widget, and the element resolved as Style::polish does in the flattened GUI
            void flatten(QWidget& widget)
            {
                widget.setStyle(m_style.get());
                AzQtComponents::BoxElement::resolve(&widget);
            }

            static const char* elementOf(const QWidget& widget)
            {
                const AzQtComponents::BoxElement* element = AzQtComponents::BoxElement::forWidget(&widget);
                return element ? element->m_name : "";
            }

            AZStd::unique_ptr<AzQtComponents::Style> m_style;
        };
    } // namespace EntityOutlinerFlatSheetTestsInternal

    using EntityOutlinerFlatSheetTestsInternal::ClassNamedWidget;
    using EntityOutlinerFlatSheetTestsInternal::DisplayOptionsElementName;
    using EntityOutlinerFlatSheetTestsInternal::EntityOutlinerFlatSheetTests;
    using EntityOutlinerFlatSheetTestsInternal::MenuIcon;

    TEST_F(EntityOutlinerFlatSheetTests, DisplayOptionsButton_BelowTheOutlinerGetsTheIconAndItsSizeAtPolish)
    {
        // The widgets of the legacy capture (UiCaptures n2, Editor 01_main_window): QToolButton#m_display_options <
        // QFrame#textSearchContainer < EntityOutlinerSearchWidget#m_searchWidget < ... < EntityOutlinerWidget. Native draws the
        // menu icon there (three bars, 16px); the flat capture draws nothing.
        ClassNamedWidget outliner("AzToolsFramework::EntityOutlinerWidget");
        QFrame* container = new QFrame(&outliner);
        container->setObjectName("textSearchContainer");
        QToolButton* button = new QToolButton(container);
        button->setObjectName("m_display_options");
        button->setIconSize(QSize(32, 32));                 // not the sheet's size

        flatten(*button);

        EXPECT_STREQ(elementOf(*button), DisplayOptionsElementName);
        EXPECT_EQ(button->iconSize(), QSize(16, 16));       // qproperty-iconSize: 16px 16px

        // qproperty-icon: the same file. It is a resource of AzQtComponents; were it missing, both sides below would be an empty image
        ASSERT_TRUE(QFile::exists(QString::fromUtf8(MenuIcon)));
        const QImage expected = QIcon(MenuIcon).pixmap(16, 16).toImage();
        ASSERT_FALSE(expected.isNull());
        EXPECT_FALSE(button->icon().isNull());
        EXPECT_TRUE(button->icon().pixmap(16, 16).toImage() == expected);
    }

    TEST_F(EntityOutlinerFlatSheetTests, DisplayOptionsButton_IsFoundOnlyBelowTheOutlinerAndOnlyByThatNameAndClass)
    {
        // The same name in a window that is not the outliner, another name in it, and a widget that is not a button
        QWidget elsewhere;
        QToolButton* strayButton = new QToolButton(&elsewhere);
        strayButton->setObjectName("m_display_options");

        ClassNamedWidget outliner("AzToolsFramework::EntityOutlinerWidget");
        QToolButton* otherName = new QToolButton(&outliner);
        otherName->setObjectName("m_filter_options");
        QWidget* notAButton = new QWidget(&outliner);
        notAButton->setObjectName("m_display_options");

        for (QWidget* widget : { static_cast<QWidget*>(strayButton), static_cast<QWidget*>(otherName), notAButton })
        {
            flatten(*widget);
        }

        // A Global element of AzQtComponents may cover a tool button or a widget, never this one
        EXPECT_STRNE(elementOf(*strayButton), DisplayOptionsElementName);
        EXPECT_STRNE(elementOf(*otherName), DisplayOptionsElementName);
        EXPECT_STRNE(elementOf(*notAButton), DisplayOptionsElementName);
        EXPECT_TRUE(strayButton->icon().isNull());
        EXPECT_TRUE(otherName->icon().isNull());
    }

    TEST_F(EntityOutlinerFlatSheetTests, Tree_BelowTheOutlinerHasNoSelectionOrHoverFillOfItsOwnAndOnlyThere)
    {
        using AzQtComponents::ItemViewElement;

        // n2 Editor 01_main_window: EntityOutlinerWidget > ... > EntityOutlinerTreeView#m_objectTree (a QTreeView). The sheet makes the role of
        // the selection and the fill of a selected or hovered item and branch transparent: the delegate paints them itself
        ClassNamedWidget outliner("AzToolsFramework::EntityOutlinerWidget");
        QWidget* middle = new QWidget(&outliner);
        QTreeView* tree = new QTreeView(middle);
        tree->setObjectName("m_objectTree");
        QWidget elsewhere;
        QTreeView* strayTree = new QTreeView(&elsewhere);
        strayTree->setObjectName("m_objectTree");

        ItemViewElement::resolve(tree);
        ItemViewElement::resolve(strayTree);

        const ItemViewElement* element = ItemViewElement::forWidget(tree);
        ASSERT_TRUE(element != nullptr);
        EXPECT_STREQ(element->m_name, "Editor.ItemView.OutlinerTree");
        EXPECT_EQ(element->m_selection.alpha(), 0);
        EXPECT_EQ(tree->palette().color(QPalette::Highlight).alpha(), 0);
        for (const auto scope : { ItemViewElement::OnItem, ItemViewElement::OnBranch })
        {
            EXPECT_EQ(element->fillOf(ItemViewElement::Selected, scope).alpha(), 0);
            EXPECT_EQ(element->fillOf(ItemViewElement::Selected | ItemViewElement::Active, scope).alpha(), 0);
            EXPECT_EQ(element->fillOf(ItemViewElement::Hover, scope).alpha(), 0);
            // a selected item that is hovered, in an inactive window: nothing of the Global greys shows either
            EXPECT_EQ(element->fillOf(ItemViewElement::Selected | ItemViewElement::Hover | ItemViewElement::Inactive, scope).alpha(), 0);
        }

        // What the rules do not name is the Global tree's: the indentation, the caret, the item box and the disabled text
        EXPECT_EQ(element->m_indentation, 20);
        EXPECT_STREQ(element->m_closed.m_image, ":/TreeView/closed.svg");
        EXPECT_EQ(element->m_itemHeight, 24);
        EXPECT_TRUE(element->textOf(ItemViewElement::Disabled) == QColor(0x77, 0x77, 0x77));

        // A tree of another window has the Global element's greys
        const ItemViewElement* globalElement = ItemViewElement::forWidget(strayTree);
        ASSERT_TRUE(globalElement != nullptr);
        EXPECT_STREQ(globalElement->m_name, "Global.ItemView.Tree");
        EXPECT_TRUE(globalElement->fillOf(ItemViewElement::Selected, ItemViewElement::OnItem) == QColor(101, 101, 101));
        EXPECT_TRUE(globalElement->fillOf(ItemViewElement::Hover, ItemViewElement::OnItem) == QColor(90, 90, 90));
    }

    // ----------------------------------------------------------------------------------
    // C042: the icon of a chip of the Outliner's search box (EntityOutlinerSearchWidget.cpp:82). The chip of a global criterion
    // (Unlocked, Locked, Visible, Hidden) holds a label with the criterion's icon, which the code gave a sheet of its own,
    // `border: 0px; background-color: transparent;`. The legacy twin is Qt's own style sheet style over AzQtComponents::Style with the
    // real application sheet (FilteredSearchWidget.qss:50-55 gives the labels of a chip no margin, no padding and a clear fill, and the
    // sheet of the label comes over it); the flat twin is the flattened Style with no sheet on the label.
    // ----------------------------------------------------------------------------------

    namespace OutlinerChipIconFlatSheetTestsInternal
    {
        namespace Twins = AzQtComponents::SheetTwins;

        //! The test application carries StyleManager's legacy sheets, which the twins name their own of: none while a test runs
        class ApplicationSheetGuard
        {
        public:
            ApplicationSheetGuard()
                : m_sheet(qApp->styleSheet())
            {
                qApp->setStyleSheet(QString());
            }

            ~ApplicationSheetGuard()
            {
                qApp->setStyleSheet(m_sheet);
            }

            ApplicationSheetGuard(const ApplicationSheetGuard&) = delete;
            ApplicationSheetGuard& operator=(const ApplicationSheetGuard&) = delete;

        private:
            QString m_sheet;
        };

        class OutlinerChipIconFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            ApplicationSheetGuard m_applicationSheet;
            Twins::Context m_context;
        };

        constexpr int FirstGlobalCriterion = 0;                 // GlobalSearchCriteria::Unlocked
        constexpr QSize ChipSize(140, 24);                      // FilteredSearchWidget.qss:33-34: 22px high and 1px of border each side

        //! The icon label of the chip: the one label with no object name (the others are the chip's text and its extra icon)
        QLabel* iconOf(QWidget* chip)
        {
            for (QLabel* label : chip->findChildren<QLabel*>())
            {
                if (label->objectName().isEmpty())
                {
                    return label;
                }
            }
            return nullptr;
        }

        //! A picture of the size the code gives the icon (10 x 10): a colour with a diagonal, so that a shifted or scaled icon differs.
        //! The criteria's icons are resources of the Editor (MainWindow.qrc), which this test does not link: the code finds none and
        //! gives the label an empty picture, so the test gives it this one in both twins.
        QPixmap iconPicture()
        {
            QPixmap pixmap(10, 10);
            pixmap.fill(QColor(0x30, 0xA0, 0xE0));
            QPainter painter(&pixmap);
            painter.setPen(Qt::white);
            painter.drawLine(0, 0, 9, 9);
            return pixmap;
        }

        //! The chip of a global criterion, as the search box makes it (EntityOutlinerSearchWidget::createCriteriaButton)
        QWidget* makeChip(QWidget* parent)
        {
            auto* chip = new AzToolsFramework::OutlinerCriteriaButton(QStringLiteral("Unlocked"), parent, FirstGlobalCriterion);
            if (QLabel* icon = iconOf(chip))
            {
                icon->setPixmap(iconPicture());
            }
            return chip;
        }

        //! The icon of a chip, the widget whose sheet it was
        QWidget* makeIcon(QWidget* parent)
        {
            return iconOf(makeChip(parent));
        }

        //! The chip of a global criterion, sized as its search box lays it out
        Twins::Row chipRow()
        {
            return Twins::Row{
                .m_site = "C042 the chip of a global criterion with its icon",
                .m_make = &makeChip,
                .m_realApplicationSheet = true,
                .m_compare = Twins::Compare::Geometry,
                .m_size = ChipSize,
                .m_base = Twins::Base::AzQtComponentsStyle,
            };
        }

        //! The icon on its own: what its sheet decided (the box of the chip around it, and the chip's rounded line, are the chip's)
        Twins::Row iconRow()
        {
            return Twins::Row{
                .m_site = "C042 the icon of a global criterion's chip",
                .m_make = &makeIcon,
                .m_realApplicationSheet = true,
                .m_states = { Twins::States::disabled() },
                .m_compare = Twins::Compare::Geometry | Twins::Compare::Palette | Twins::Compare::Pixels,
                .m_roles = { QPalette::WindowText, QPalette::Text, QPalette::ButtonText },
                .m_base = Twins::Base::AzQtComponentsStyle,
            };
        }

        //! What the icon label has, of the legacy twin or the flat one
        struct IconFacts
        {
            bool m_found = false;
            QRect m_geometry;
            QRect m_contentsRect;
            QSize m_sizeHint;
            QSize m_minimumSizeHint;
            QMargins m_contentsMargins;
            int m_frameWidth = -1;
            QSize m_pixmapSize;
            bool m_hasSheet = false;

            //! The facts the sheet decides, as text for a comparison and for its report
            std::string text() const
            {
                const auto rect = [](const QRect& r)
                {
                    return QStringLiteral("(%1,%2 %3x%4)").arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height());
                };
                const auto size = [](const QSize& s)
                {
                    return QStringLiteral("%1x%2").arg(s.width()).arg(s.height());
                };
                const QMargins& m = m_contentsMargins;
                QStringList lines;
                lines << QStringLiteral("geometry ") + rect(m_geometry);
                lines << QStringLiteral("contentsRect ") + rect(m_contentsRect);
                lines << QStringLiteral("sizeHint ") + size(m_sizeHint);
                lines << QStringLiteral("minimumSizeHint ") + size(m_minimumSizeHint);
                lines << QStringLiteral("contentsMargins %1/%2/%3/%4").arg(m.left()).arg(m.top()).arg(m.right()).arg(m.bottom());
                lines << QStringLiteral("frameWidth %1").arg(m_frameWidth);
                lines << QStringLiteral("pixmap ") + size(m_pixmapSize);
                return lines.join(QStringLiteral(", ")).toStdString();
            }
        };

        IconFacts readIcon(const Twins::Row& row, Twins::Context& context, bool legacy)
        {
            Twins::Context::registerOnce(row.m_register);
            Twins::RungScope rung(legacy ? row.m_legacyRung : Twins::Rung::Flat);
            const Twins::Twin twin(row, context, legacy);
            IconFacts facts;
            if (QLabel* icon = iconOf(twin.widget()))
            {
                facts.m_found = true;
                facts.m_geometry = icon->geometry();
                facts.m_contentsRect = icon->contentsRect();
                facts.m_sizeHint = icon->sizeHint();
                facts.m_minimumSizeHint = icon->minimumSizeHint();
                facts.m_contentsMargins = icon->contentsMargins();
                facts.m_frameWidth = icon->frameWidth();
                facts.m_pixmapSize = icon->pixmap().size();
                facts.m_hasSheet = !icon->styleSheet().isEmpty();
            }
            return facts;
        }
    } // namespace OutlinerChipIconFlatSheetTestsInternal

    // A unity build joins this file with others of namespace UnitTest: the fixture is the only name brought in
    using OutlinerChipIconFlatSheetTestsInternal::OutlinerChipIconFlatSheetTests;
    namespace OutlinerChip = OutlinerChipIconFlatSheetTestsInternal;

    TEST_F(OutlinerChipIconFlatSheetTests, Icon_WithNoSheetIsTheIconOfTheLegacyGui)
    {
        // The flat twin has no sheet at all (the harness flags one on the icon), and the icon is sized, coloured and painted as under
        // its sheet, at the device ratios 1, 1.5 and 2, enabled and disabled
        EXPECT_TRUE(OutlinerChip::Twins::check(OutlinerChip::iconRow(), m_context));
    }

    TEST_F(OutlinerChipIconFlatSheetTests, Icon_HasTheBoxOfTheLegacyLabelAndNoSheet)
    {
        // In the chip as the search box lays it out: where the icon lies, what it asks for, and that it has no sheet in the flat twin
        const OutlinerChip::Twins::Row row = OutlinerChip::chipRow();
        const OutlinerChip::IconFacts legacy = OutlinerChip::readIcon(row, m_context, true);
        const OutlinerChip::IconFacts flat = OutlinerChip::readIcon(row, m_context, false);

        ASSERT_TRUE(legacy.m_found);
        ASSERT_TRUE(flat.m_found);
        EXPECT_TRUE(legacy.m_hasSheet) << "the legacy GUI sets the sheet of the icon";
        EXPECT_FALSE(flat.m_hasSheet) << "the flattened GUI sets none";
        EXPECT_EQ(flat.text(), legacy.text());
        EXPECT_EQ(legacy.m_contentsRect.size(), legacy.m_geometry.size()) << "the sheet gives the icon no margin, border or padding";
    }

    TEST_F(OutlinerChipIconFlatSheetTests, Icon_IsOnTheCensusOnlyInTheLegacyGui)
    {
        const auto build = []
        {
            QWidget host;
            host.resize(200, 40);
            new AzToolsFramework::OutlinerCriteriaButton(QStringLiteral("Visible"), &host, OutlinerChip::FirstGlobalCriterion + 2);
            // A chip of a real filter (the index is past the global criteria) has no icon label
            new AzToolsFramework::OutlinerCriteriaButton(QStringLiteral("Mesh"), &host, 40);
            host.setAttribute(Qt::WA_DontShowOnScreen);
            host.show();
            host.ensurePolished();
            for (QWidget* each : host.findChildren<QWidget*>())
            {
                each->ensurePolished();
            }
        };

        const QStringList legacy = AzQtComponents::SiteTwins::sheetLinesAt(AzQtComponents::GuiPerfFloor::Rung::Qss, build);
        ASSERT_EQ(legacy.size(), 1) << legacy.join("\n").toStdString();
        EXPECT_TRUE(AzQtComponents::SiteTwins::namesCarrier(legacy[0], QStringLiteral("QLabel"))) << legacy[0].toStdString();
        EXPECT_TRUE(legacy[0].contains(QStringLiteral("border: 0px; background-color: transparent;"))) << legacy[0].toStdString();

        const QStringList flat = AzQtComponents::SiteTwins::sheetLinesAt(AzQtComponents::GuiPerfFloor::Rung::Flat, build);
        EXPECT_TRUE(flat.isEmpty()) << flat.join("\n").toStdString();
    }
} // namespace UnitTest
