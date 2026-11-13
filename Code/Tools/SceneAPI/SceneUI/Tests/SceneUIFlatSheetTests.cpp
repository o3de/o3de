/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Tests/SheetTwins.h>
#include <AzQtComponents/Tests/SiteTwins.h>

#include <SceneAPI/SceneUI/CommonWidgets/SceneSettingsCard.h>
#include <SceneAPI/SceneUI/SceneUIFlatSheet.h>
#include <SceneAPI/SceneUI/SceneWidgets/ManifestWidget.h>

#include <QApplication>
#include <QByteArray>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QPixmap>
#include <QPushButton>
#include <QStringList>
#include <QSvgWidget>
#include <QToolButton>
#include <QWidget>

#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // SceneUI's flat home (SceneUIFlatSheet): the sheets that the scene settings widgets set on themselves, twinned. The legacy twin is
    // Qt's own style sheet style over AzQtComponents::Style with the real application sheet and the sheets of the sites, quoted; the flat
    // twin is the flattened Style with the elements of SceneUIFlatSheet, and no sheet. The widgets of the library that it does not
    // export (the header widget, the vector widget, the manifest page) are stand-ins built as their .ui files build them, and the
    // Style functions of SceneUIFlatSheet are the real site code they call.
    // ----------------------------------------------------------------------------------

    namespace SceneUIFlatSheetTestsInternal
    {
        namespace Twins = AzQtComponents::SheetTwins;

        //! The Qt application of the process, made once and never destroyed (a QApplication made and destroyed for each test would
        //! rebuild the platform integration each time); the offscreen platform is the caller's (QT_QPA_PLATFORM)
        struct ApplicationOnce
        {
            ApplicationOnce()
            {
                if (!QApplication::instance())
                {
                    static int argc = 1;
                    static char name[] = "SceneUI.Tests";
                    static char* argv[] = { name, nullptr };
                    new QApplication(argc, argv);
                }
            }
        };

        class SceneUIFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            ApplicationOnce m_application;
            Twins::Context m_context;
        };

        // ------------------------------------------------------------------------------
        // The sheets, quoted as the sources have them
        // ------------------------------------------------------------------------------

        //! The sheet of the row of a group in the display divider style (PropertyRowWidget.cpp:66, set by PropertyRowWidget), which
        //! reaches the labels of the header below it
        constexpr const char* DividerRowSheet =
            "QFrame {background-color: #333333; margin-top: 5px; border-top-right-radius: 2px; border-top-left-radius: 2px;}";
        //! The sheet of the header itself (HeaderWidget.cpp, UpdateUIForManifestObject, before the flattening)
        constexpr const char* DividerHeaderSheet = "QFrame, QLabel {margin-top: 0px; font: bold;}";
        constexpr const char* PageHeaderSheet =
            "QFrame {border-top: 1px transparent; padding-top: 6px; border-bottom: 1px solid #000000; padding-bottom: 6px}";
        constexpr const char* AddButtonSheet = "padding-left: 12px; padding-right: 12px;";
        constexpr const char* AddObjectButtonSheet = "padding: 6px;";
        constexpr const char* SpinnerSheet = "background-color: rgba(0,0,0,0)";

        constexpr const char* RowClass = "AzToolsFramework::PropertyRowWidget";
        constexpr const char* HeaderClass = "AZ::SceneAPI::UI::HeaderWidget";
        constexpr const char* VectorClass = "AZ::SceneAPI::UI::ManifestVectorWidget";
        constexpr const char* PageClass = "AZ::SceneAPI::UI::ManifestWidgetPage";

        //! The state the harness leaves for the production Style functions: they are the sites' code, and at the legacy rung they set
        //! their sheet. A site that sets another text than the one quoted is a failure of its own
        void expectSheet(const QWidget* widget, const char* quoted)
        {
            if (!AzQtComponents::StyleManager::stylesheetsDisabled())
            {
                EXPECT_EQ(widget->styleSheet().toStdString(), std::string(quoted)) << "the site sets another sheet than the one quoted";
            }
        }

        // ------------------------------------------------------------------------------
        // The header of a group in the display divider style, as HeaderWidget.ui builds it
        // ------------------------------------------------------------------------------

        constexpr QSize HeaderSize(150, 40);
        constexpr QRect IconBox(10, 6, 16, 16);                 // the icon label: fixed 16 x 16 (HeaderWidget.ui)
        constexpr QRect NameBox(32, 6, 100, 18);                // the name label: 18 high (HeaderWidget.ui minimumSize and maximumSize)
        constexpr QRect DeleteBox(130, 6, 16, 16);              // the delete button: fixed 16 x 16

        //! A picture of the size of the icon, so that the icon label has something to draw
        QPixmap iconPicture()
        {
            QPixmap pixmap(16, 16);
            pixmap.fill(QColor(0x30, 0xA0, 0xE0));
            return pixmap;
        }

        //! The header with the labels and the button of HeaderWidget.ui
        QWidget* newHeader(QWidget* parent)
        {
            auto* header = new Twins::StandIn(HeaderClass, parent);
            header->setObjectName(QStringLiteral("header"));

            auto* icon = new QLabel(header);
            icon->setObjectName(QStringLiteral("m_icon"));
            icon->setMaximumSize(16, 16);
            icon->setPixmap(iconPicture());
            icon->setGeometry(IconBox);

            auto* name = new QLabel(QStringLiteral("Mesh group"), header);
            name->setObjectName(QStringLiteral("m_nameLabel"));
            name->setMinimumHeight(18);
            name->setMaximumHeight(18);
            name->setGeometry(NameBox);

            auto* remove = new QToolButton(header);
            remove->setObjectName(QStringLiteral("m_deleteButton"));
            remove->setFixedSize(16, 16);
            remove->setIcon(QIcon(QStringLiteral(":/stylesheet/img/close_small.svg")));
            remove->setGeometry(DeleteBox);
            return header;
        }

        //! The header given the look HeaderWidget::UpdateUIForManifestObject gives a header whose group has the display divider
        //! category (the real site code)
        QWidget* makeDividerHeader(QWidget* parent)
        {
            QWidget* header = newHeader(parent);
            SceneUIFlatSheet::StyleDisplayDividerHeader(header);
            expectSheet(header, DividerHeaderSheet);
            return header;
        }

        //! The roles a sheet's `color` decides. The background roles are what Qt's style sheet style derives from a `background-color`
        //! (the fill of a sheet is painted by the style, not read from the palette), so they are no fact of the flattened GUI
        std::vector<QPalette::ColorRole> textRoles()
        {
            return { QPalette::WindowText, QPalette::Text, QPalette::ButtonText };
        }

        //! The flattened Style on the widget and every widget below it (a style set on a widget reaches no child without a sheet)
        void useFlatStyle(QWidget& root, const Twins::Context& context)
        {
            root.setStyle(context.flatStyle());
            for (QWidget* each : root.findChildren<QWidget*>())
            {
                each->setStyle(context.flatStyle());
            }
        }

        //! A part of the header, by name: the widget a row compares
        template<typename Part>
        Twins::WidgetFactory partOfHeader(const char* name)
        {
            return [name](QWidget* parent) -> QWidget*
            {
                return parent->findChild<Part*>(QString::fromLatin1(name));
            };
        }

        /*! The chain of the header: the row of the group (a stand-in for the PropertyRowWidget, which carries the row's sheet) and the
            header below it, whose sheet is the deeper */
        std::vector<Twins::Link> dividerChain()
        {
            return { { .m_name = "row", .m_className = RowClass }, { .m_name = "header", .m_make = &makeDividerHeader } };
        }

        std::vector<Twins::Sheet> dividerSheets()
        {
            return {
                { "PropertyRowWidget.cpp:66", Twins::SetOn::Ancestor, DividerRowSheet, 0 },
                { "SceneUIFlatSheet.cpp:45", Twins::SetOn::Ancestor, DividerHeaderSheet, 1 },
            };
        }

        Twins::Row dividerPartRow(const char* site, Twins::WidgetFactory make, const QRect& box, unsigned compare)
        {
            return Twins::Row{
                .m_site = site,
                .m_make = std::move(make),
                .m_chain = dividerChain(),
                .m_sheets = dividerSheets(),
                .m_realApplicationSheet = true,
                .m_register = &SceneUIFlatSheet::RegisterElements,
                .m_compare = compare,
                .m_roles = textRoles(),
                .m_size = box.size(),
                .m_rootSize = HeaderSize,
                .m_base = Twins::Base::AzQtComponentsStyle,
            };
        }

        constexpr unsigned LabelFacts = Twins::Compare::Geometry | Twins::Compare::Fonts | Twins::Compare::Palette | Twins::Compare::Pixels;

        // ------------------------------------------------------------------------------
        // The buttons and the frame of the manifest page and the vector widget
        // ------------------------------------------------------------------------------

        //! The "Add Object" button as ManifestVectorWidget builds it: after setupUi it gets its look, then the class tag and an empty menu
        QWidget* makeAddObjectButton(QWidget* parent)
        {
            auto* button = new QPushButton(QStringLiteral("Add Object"), parent);
            button->setObjectName(QStringLiteral("m_addObjectButton"));
            SceneUIFlatSheet::StyleAddObjectButton(button);
            expectSheet(button, AddObjectButtonSheet);
            button->setProperty("class", "FixedMenu");
            button->setMenu(new QMenu(parent));
            return button;
        }

        //! The "Add another" button of the manifest page (ManifestWidgetPage.ui: 24 high at least, 250 wide at most)
        QPushButton* newAddButton(QWidget* parent)
        {
            auto* button = new QPushButton(QStringLiteral("Add another"), parent);
            button->setObjectName(QStringLiteral("m_addButton"));
            button->setMinimumSize(10, 24);
            button->setMaximumSize(250, QWIDGETSIZE_MAX);
            SceneUIFlatSheet::StyleAddButton(button);
            expectSheet(button, AddButtonSheet);
            return button;
        }

        QWidget* makeAddButton(QWidget* parent)
        {
            return newAddButton(parent);
        }

        //! The same button with the menu the page gives it when its group types are known (ManifestWidgetPage::BuildAndConnectAddButton)
        QWidget* makeAddButtonWithMenu(QWidget* parent)
        {
            QPushButton* button = newAddButton(parent);
            button->setMenu(new QMenu(parent));
            return button;
        }

        //! The frame of the buttons of the manifest page as ManifestWidgetPage.ui builds it: no frame, buttons side by side in a layout
        //! without margins or spacing
        QWidget* makePageHeader(QWidget* parent)
        {
            auto* header = new QFrame(parent);
            header->setObjectName(QStringLiteral("header"));
            header->setFrameShape(QFrame::NoFrame);
            auto* layout = new QHBoxLayout(header);
            layout->setSpacing(0);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->addWidget(newAddButton(header));
            layout->addWidget(new QPushButton(QStringLiteral("Save"), header));
            layout->addStretch();
            SceneUIFlatSheet::StyleManifestPageHeader(header);
            expectSheet(header, PageHeaderSheet);
            return header;
        }

        constexpr unsigned ButtonFacts =
            Twins::Compare::Geometry | Twins::Compare::Fonts | Twins::Compare::Palette | Twins::Compare::Pixels;

        //! The states of a push button but the hover one, compared with the pictures: the flattened hover fill of ANY push button is
        //! one or two levels off the legacy one (a button with no sheet and no element differs the same way), a difference of the base
        //! look that no site decides. The hover state is compared without the pictures (HoverFacts).
        std::vector<Twins::State> buttonStates()
        {
            return { Twins::States::pressed(), Twins::States::disabled(), Twins::States::focus() };
        }

        constexpr unsigned HoverFacts = Twins::Compare::Geometry | Twins::Compare::Fonts | Twins::Compare::Palette;

        std::vector<Twins::State> hoverStates()
        {
            return { Twins::States::hover() };
        }

        Twins::Row addObjectButtonRow(std::vector<Twins::State> states = buttonStates(), unsigned compare = ButtonFacts)
        {
            return Twins::Row{
                .m_site = "U057 the Add Object button of a vector widget",
                .m_make = &makeAddObjectButton,
                .m_chain = { { .m_name = "vector", .m_className = VectorClass } },
                .m_sheets = { { "ManifestVectorWidget.ui:79", Twins::SetOn::Widget, AddObjectButtonSheet } },
                .m_realApplicationSheet = true,
                .m_register = &SceneUIFlatSheet::RegisterElements,
                .m_states = std::move(states),
                .m_compare = compare,
                .m_roles = textRoles(),
                .m_base = Twins::Base::AzQtComponentsStyle,
            };
        }

        Twins::Row addButtonRow(
            const char* site, Twins::WidgetFactory make, std::vector<Twins::State> states = buttonStates(), unsigned compare = ButtonFacts)
        {
            return Twins::Row{
                .m_site = site,
                .m_make = std::move(make),
                .m_chain = { { .m_name = "page", .m_className = PageClass } },
                .m_sheets = { { "ManifestWidgetPage.ui:80", Twins::SetOn::Widget, AddButtonSheet } },
                .m_realApplicationSheet = true,
                .m_register = &SceneUIFlatSheet::RegisterElements,
                .m_states = std::move(states),
                .m_compare = compare,
                .m_roles = textRoles(),
                .m_base = Twins::Base::AzQtComponentsStyle,
            };
        }

        Twins::Row pageHeaderRow()
        {
            return Twins::Row{
                .m_site = "U058 the frame of the buttons of the manifest page",
                .m_make = &makePageHeader,
                .m_chain = { { .m_name = "page", .m_className = PageClass } },
                .m_sheets = { { "ManifestWidgetPage.ui:44", Twins::SetOn::Widget, PageHeaderSheet } },
                .m_realApplicationSheet = true,
                .m_register = &SceneUIFlatSheet::RegisterElements,
                .m_compare = Twins::Compare::Geometry | Twins::Compare::Palette | Twins::Compare::Pixels,
                .m_roles = textRoles(),
                .m_size = QSize(320, 60),
                .m_base = Twins::Base::AzQtComponentsStyle,
                .m_mayDecideNothing = true,         // the site sets its sheet itself: the twin without the quoted sheets has it too
            };
        }

        // ------------------------------------------------------------------------------
        // The busy spinner of the header of a scene settings card (the real card)
        // ------------------------------------------------------------------------------

        constexpr const char* StillSpinner =
            "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"40\" height=\"40\" viewBox=\"0 0 50 50\">"
            "<path fill=\"#1E70EB\" d=\"M43.935,25.145c0-10.318-8.364-18.683-18.683-18.683c-10.318,0-18.683,8.365-18.683,18.683h4.068"
            "c0-8.071,6.543-14.615,14.615-14.615c8.072,0,14.615,6.543,14.615,14.615H43.935z\"/></svg>";

        QSvgWidget* spinnerOf(QWidget* card)
        {
            return card->findChild<QSvgWidget*>();
        }

        QWidget* makeCardSpinner(QWidget* parent)
        {
            auto* card = new SceneSettingsCard(
                AZ::Uuid::CreateRandom(), QStringLiteral("scene.fbx"), SceneSettingsCard::Layout::Loading, parent);
            QSvgWidget* spinner = spinnerOf(card);
            if (spinner)
            {
                expectSheet(spinner, SpinnerSheet);
                // The spinner of the library turns with the clock, so that two pictures of it never match: both twins draw the same still
                // picture in its place, and the pictures show the sheet and not the clock
                spinner->load(QByteArray(StillSpinner));
            }
            return spinner;
        }

        Twins::Row spinnerRow()
        {
            return Twins::Row{
                .m_site = "C089 the busy spinner of the header of a scene settings card",
                .m_make = &makeCardSpinner,
                .m_sheets = { { "SceneSettingsCard.cpp:56", Twins::SetOn::Widget, SpinnerSheet } },
                .m_realApplicationSheet = true,
                .m_compare = Twins::Compare::Geometry | Twins::Compare::Palette | Twins::Compare::Pixels,
                .m_roles = textRoles(),
                .m_base = Twins::Base::AzQtComponentsStyle,
                .m_mayDecideNothing = true,         // the site sets its sheet itself: the twin without the quoted sheets has it too
            };
        }
    } // namespace SceneUIFlatSheetTestsInternal

    // A unity build joins this file with others of namespace UnitTest: the fixture is the only name brought in
    using SceneUIFlatSheetTestsInternal::SceneUIFlatSheetTests;
    namespace SceneUIFlat = SceneUIFlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // M-sceneui-home: the flat home registers once, from the manifest widget
    // ----------------------------------------------------------------------------------

    TEST_F(SceneUIFlatSheetTests, ManifestWidget_RegistersTheElementsOfTheScenesWidgetsInTheFlatGui)
    {
        // The constructor asks for the registration: counted by the calls, so that the proof holds whatever a test before it (or a
        // repeat of this one) registered
        SceneUIFlat::Twins::RungScope flat(AzQtComponents::GuiPerfFloor::Rung::Flat);
        {
            AZ::SerializeContext serializeContext;
            const int callsBefore = SceneUIFlatSheet::RegisterElementsCalls();
            AZ::SceneAPI::UI::ManifestWidget manifestWidget(&serializeContext);
            EXPECT_GT(SceneUIFlatSheet::RegisterElementsCalls(), callsBefore) << "the manifest widget did not ask for the elements";

            QWidget host;
            QWidget* header = SceneUIFlat::makeDividerHeader(&host);
            SceneUIFlat::useFlatStyle(host, m_context);
            auto* name = header->findChild<QLabel*>(QStringLiteral("m_nameLabel"));
            ASSERT_NE(name, nullptr);
            name->ensurePolished();
            const AzQtComponents::BoxElement* element = AzQtComponents::BoxElement::forWidget(name);
            ASSERT_NE(element, nullptr) << "no element reaches the label below a divider header";
            EXPECT_STREQ(element->m_name, "SceneUI.Box.DividerHeaderLabel");
        }
    }

    TEST_F(SceneUIFlatSheetTests, DividerHeader_LabelsPolishedBeforeTheTagAreResolvedAgain)
    {
        SceneUIFlat::Twins::Context::registerOnce(&SceneUIFlatSheet::RegisterElements);
        SceneUIFlat::Twins::RungScope flat(AzQtComponents::GuiPerfFloor::Rung::Flat);
        SceneUIFlatSheet::RegisterElements();       // a second call registers nothing again

        // A header shown and polished, and a label beside it; the group of the header turns out to be a divider afterwards (the
        // category is known when the manifest object is set, which is after the widgets were made)
        QWidget host;
        auto* plain = new QLabel(QStringLiteral("Plain"), &host);
        QWidget* header = SceneUIFlat::newHeader(&host);
        auto* name = header->findChild<QLabel*>(QStringLiteral("m_nameLabel"));
        SceneUIFlat::useFlatStyle(host, m_context);
        host.setAttribute(Qt::WA_DontShowOnScreen);
        host.show();
        for (QWidget* each : host.findChildren<QWidget*>())
        {
            each->ensurePolished();
        }
        EXPECT_EQ(AzQtComponents::BoxElement::forWidget(name), nullptr) << "no element reaches a label below a header with no tag";

        SceneUIFlatSheet::StyleDisplayDividerHeader(header);

        ASSERT_NE(AzQtComponents::BoxElement::forWidget(name), nullptr) << "the label was not resolved again";
        EXPECT_STREQ(AzQtComponents::BoxElement::forWidget(name)->m_name, "SceneUI.Box.DividerHeaderLabel");
        EXPECT_EQ(static_cast<int>(name->font().weight()), 700);
        EXPECT_EQ(AzQtComponents::BoxElement::forWidget(plain), nullptr) << "a label outside the header is no concern of the tag";
        EXPECT_EQ(static_cast<int>(plain->font().weight()), 400);
    }

    // ----------------------------------------------------------------------------------
    // C090: the sheet of the header of a divider group, below the sheet of the group's row
    // ----------------------------------------------------------------------------------

    TEST_F(SceneUIFlatSheetTests, DividerHeader_NameLabelIsTheLabelOfTheDeeperSheet)
    {
        // margin-top 0 over the row's 5px, bold over the label rule's regular, and the row's fill and top corners, at 1, 1.5 and 2
        EXPECT_TRUE(SceneUIFlat::Twins::check(
            SceneUIFlat::dividerPartRow("C090 the name label of a divider header", SceneUIFlat::partOfHeader<QLabel>("m_nameLabel"),
                SceneUIFlat::NameBox, SceneUIFlat::LabelFacts),
            m_context));
    }

    TEST_F(SceneUIFlatSheetTests, DividerHeader_IconLabelIsTheLabelOfTheDeeperSheet)
    {
        EXPECT_TRUE(SceneUIFlat::Twins::check(
            SceneUIFlat::dividerPartRow("C090 the icon label of a divider header", SceneUIFlat::partOfHeader<QLabel>("m_icon"),
                SceneUIFlat::IconBox, SceneUIFlat::LabelFacts),
            m_context));
    }

    TEST_F(SceneUIFlatSheetTests, DividerHeader_DeleteButtonIsNoFrameAndStaysAsItWas)
    {
        // `QFrame, QLabel` reaches no tool button: the sheets decide nothing of it, in either GUI
        SceneUIFlat::Twins::Row row = SceneUIFlat::dividerPartRow(
            "C090 the delete button of a divider header", SceneUIFlat::partOfHeader<QToolButton>("m_deleteButton"), SceneUIFlat::DeleteBox,
            SceneUIFlat::LabelFacts);
        row.m_mayDecideNothing = true;
        EXPECT_TRUE(SceneUIFlat::Twins::check(row, m_context));
    }

    TEST_F(SceneUIFlatSheetTests, DividerHeader_SetsNoSheetInTheFlatGuiAndTagsTheHeader)
    {
        const auto build = []
        {
            QWidget host;
            QWidget* header = SceneUIFlat::makeDividerHeader(&host);
            host.setAttribute(Qt::WA_DontShowOnScreen);
            host.show();
            for (QWidget* each : host.findChildren<QWidget*>())
            {
                each->ensurePolished();
            }
            EXPECT_EQ(
                AzQtComponents::StyleManager::stylesheetsDisabled(), AzQtComponents::Selectors::tagged(header, "SceneDividerHeader"));
        };
        const QStringList legacy = AzQtComponents::SiteTwins::sheetLinesAt(AzQtComponents::GuiPerfFloor::Rung::Qss, build);
        ASSERT_EQ(legacy.size(), 1) << legacy.join("\n").toStdString();
        EXPECT_TRUE(legacy[0].contains(QStringLiteral("font: bold"))) << legacy[0].toStdString();
        const QStringList flat = AzQtComponents::SiteTwins::sheetLinesAt(AzQtComponents::GuiPerfFloor::Rung::Flat, build);
        EXPECT_TRUE(flat.isEmpty()) << flat.join("\n").toStdString();
    }

    // ----------------------------------------------------------------------------------
    // U057, U058, U059: the sheets of the .ui files
    // ----------------------------------------------------------------------------------

    TEST_F(SceneUIFlatSheetTests, AddObjectButton_HasThePaddingOfItsSheetInEveryState)
    {
        EXPECT_TRUE(SceneUIFlat::Twins::check(SceneUIFlat::addObjectButtonRow(), m_context));
    }

    TEST_F(SceneUIFlatSheetTests, AddObjectButton_HasThePaddingOfItsSheetInHover)
    {
        // the box, the fonts and the text colours; the hover fill is the base push button's (buttonStates)
        EXPECT_TRUE(SceneUIFlat::Twins::check(
            SceneUIFlat::addObjectButtonRow(SceneUIFlat::hoverStates(), SceneUIFlat::HoverFacts), m_context));
    }

    TEST_F(SceneUIFlatSheetTests, PageHeader_HasThePaddingAndTheLineOfItsSheet)
    {
        EXPECT_TRUE(SceneUIFlat::Twins::check(SceneUIFlat::pageHeaderRow(), m_context));
    }

    TEST_F(SceneUIFlatSheetTests, PageHeader_FlatBoxIsThePaddingAndTheLineOfItsSheet)
    {
        // The twin above sets the sheet in its legacy half by the site's code, so it cannot show that the sheet decides something: the
        // box the sheet gave the frame (6px above, 6px and the 1px line below) is asserted of the flattened frame directly
        SceneUIFlat::Twins::RungScope flat(AzQtComponents::GuiPerfFloor::Rung::Flat);
        SceneUIFlat::Twins::Context::registerOnce(&SceneUIFlatSheet::RegisterElements);
        QWidget host;
        QWidget* page = new SceneUIFlat::Twins::StandIn(SceneUIFlat::PageClass, &host);
        QWidget* header = SceneUIFlat::makePageHeader(page);
        header->resize(320, 60);
        SceneUIFlat::useFlatStyle(host, m_context);
        host.setAttribute(Qt::WA_DontShowOnScreen);
        host.show();
        for (QWidget* each : host.findChildren<QWidget*>())
        {
            each->ensurePolished();
        }
        const AzQtComponents::BoxElement* element = AzQtComponents::BoxElement::forWidget(header);
        ASSERT_NE(element, nullptr);
        EXPECT_STREQ(element->m_name, "SceneUI.Box.PageHeader");
        EXPECT_EQ(header->contentsMargins(), QMargins(0, 6, 0, 7));
    }

    TEST_F(SceneUIFlatSheetTests, AddButton_HasThePaddingOfItsSheetInEveryState)
    {
        EXPECT_TRUE(SceneUIFlat::Twins::check(
            SceneUIFlat::addButtonRow("U059 the Add another button of the manifest page", &SceneUIFlat::makeAddButton), m_context));
    }

    TEST_F(SceneUIFlatSheetTests, AddButton_WithAMenuHasThePaddingOfItsSheetInEveryState)
    {
        EXPECT_TRUE(SceneUIFlat::Twins::check(
            SceneUIFlat::addButtonRow("U059 the Add another button of the manifest page, with a menu", &SceneUIFlat::makeAddButtonWithMenu),
            m_context));
    }

    TEST_F(SceneUIFlatSheetTests, AddButton_HasThePaddingOfItsSheetInHover)
    {
        EXPECT_TRUE(SceneUIFlat::Twins::check(
            SceneUIFlat::addButtonRow("U059 the Add another button of the manifest page, hovered", &SceneUIFlat::makeAddButton,
                SceneUIFlat::hoverStates(), SceneUIFlat::HoverFacts),
            m_context));
        EXPECT_TRUE(SceneUIFlat::Twins::check(
            SceneUIFlat::addButtonRow("U059 the Add another button of the manifest page, with a menu, hovered",
                &SceneUIFlat::makeAddButtonWithMenu, SceneUIFlat::hoverStates(), SceneUIFlat::HoverFacts),
            m_context));
    }

    TEST_F(SceneUIFlatSheetTests, ManifestPageSites_SetSheetsOnlyInTheLegacyGui)
    {
        const auto build = []
        {
            QWidget host;
            host.resize(400, 200);
            QWidget* vectorWidget = new SceneUIFlat::Twins::StandIn(SceneUIFlat::VectorClass, &host);
            SceneUIFlat::makeAddObjectButton(vectorWidget);
            QWidget* page = new SceneUIFlat::Twins::StandIn(SceneUIFlat::PageClass, &host);
            SceneUIFlat::makePageHeader(page);
            SceneUIFlat::makeAddButton(page);
            host.setAttribute(Qt::WA_DontShowOnScreen);
            host.show();
            for (QWidget* each : host.findChildren<QWidget*>())
            {
                each->ensurePolished();
            }
        };
        // the buttons of the .ui sheets: the vector widget's, the page header's own and the "Add another" button of the page header,
        // and the one more "Add another" the test makes beside the header
        const QStringList legacy = AzQtComponents::SiteTwins::sheetLinesAt(AzQtComponents::GuiPerfFloor::Rung::Qss, build);
        int pushButtons = 0;
        int frames = 0;
        for (const QString& line : legacy)
        {
            pushButtons += AzQtComponents::SiteTwins::namesCarrier(line, QStringLiteral("QPushButton")) ? 1 : 0;
            frames += AzQtComponents::SiteTwins::namesCarrier(line, QStringLiteral("QFrame")) ? 1 : 0;
        }
        EXPECT_EQ(pushButtons, 3) << legacy.join("\n").toStdString();
        EXPECT_EQ(frames, 1) << legacy.join("\n").toStdString();
        const QStringList flat = AzQtComponents::SiteTwins::sheetLinesAt(AzQtComponents::GuiPerfFloor::Rung::Flat, build);
        EXPECT_TRUE(flat.isEmpty()) << flat.join("\n").toStdString();
    }

    // ----------------------------------------------------------------------------------
    // C089: the spinner of the header of a scene settings card
    // ----------------------------------------------------------------------------------

    TEST_F(SceneUIFlatSheetTests, CardSpinner_IsTransparentAsItsSheetMadeIt)
    {
        EXPECT_TRUE(SceneUIFlat::Twins::check(SceneUIFlat::spinnerRow(), m_context));
    }

    TEST_F(SceneUIFlatSheetTests, CardSpinner_SetsItsSheetOnlyInTheLegacyGui)
    {
        const auto build = []
        {
            QWidget host;
            host.resize(400, 200);
            new SceneSettingsCard(AZ::Uuid::CreateRandom(), QStringLiteral("scene.fbx"), SceneSettingsCard::Layout::Loading, &host);
            host.setAttribute(Qt::WA_DontShowOnScreen);
            host.show();
            for (QWidget* each : host.findChildren<QWidget*>())
            {
                each->ensurePolished();
            }
        };
        const QStringList legacy = AzQtComponents::SiteTwins::sheetLinesAt(AzQtComponents::GuiPerfFloor::Rung::Qss, build);
        int spinners = 0;
        for (const QString& line : legacy)
        {
            spinners += AzQtComponents::SiteTwins::namesCarrier(line, QStringLiteral("QSvgWidget")) ? 1 : 0;
        }
        EXPECT_EQ(spinners, 1) << legacy.join("\n").toStdString();
        const QStringList flat = AzQtComponents::SiteTwins::sheetLinesAt(AzQtComponents::GuiPerfFloor::Rung::Flat, build);
        for (const QString& line : flat)
        {
            EXPECT_FALSE(AzQtComponents::SiteTwins::namesCarrier(line, QStringLiteral("QSvgWidget"))) << line.toStdString();
        }
    }
} // namespace UnitTest
