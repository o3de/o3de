/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Tests/SheetTwins.h>

#include <Editor/KinematicDescriptionDialog.h>

#include <QApplication>
#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QList>
#include <QPalette>
#include <QPushButton>
#include <QRadioButton>
#include <QString>
#include <QStyle>
#include <QWidget>

#include <initializer_list>
#include <string>
#include <vector>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The Kinematic description dialog (KinematicDescriptionDialog): the inline sheets its form and its code gave its widgets, carried by
    // the elements of KinematicDescriptionDialog::RegisterFlatElements. The twins are built from the real dialog, in each mode: the legacy
    // twin is Qt's own style sheet style over the AzQtComponents style with the real application sheet (BaseStyleSheet.qss and the files
    // it imports) and the sheets the dialog sets on its widgets itself in the legacy GUI (setLegacyStyleSheet); the flat twin is the
    // flattened Style with no sheet at all. Each widget the sheets decide is a row of its own, at the geometry the form gives it
    // (KinematicDescriptionDialog.ui), and the dialog whole is one, which holds each widget where the form's layouts put it.
    // ----------------------------------------------------------------------------------

    namespace KinematicDialogFlatSheetTestsInternal
    {
        namespace Twins = AzQtComponents::SheetTwins;

        constexpr const char* DialogName = "KinematicDescriptionDialog";        // the name the form gives the dialog (.ui:4)
        constexpr QSize DialogSize(641, 305);                                   // .ui:11-12

        enum class Mode
        {
            Simulated,
            Kinematic,
        };

        QWidget* makeDialog(Mode mode, QWidget* parent)
        {
            return new PhysX::Editor::KinematicDescriptionDialog(mode == Mode::Kinematic, parent);
        }

        //! A widget of the dialog by its name. A dialog without it is a failure of the test, and the dialog itself stands in so that the
        //! harness does not run on nothing.
        QWidget* childOf(QWidget* dialog, const char* name)
        {
            if (QWidget* child = dialog->findChild<QWidget*>(QString::fromLatin1(name)))
            {
                return child;
            }
            ADD_FAILURE() << "the dialog has no widget named " << name;
            return dialog;
        }

        //! The click of the user on the radio button of a mode, which the dialog answers with its look (OnButtonClicked)
        void clickRadio(QWidget& widget, const char* name)
        {
            if (auto* radio = widget.window()->findChild<QRadioButton*>(QString::fromLatin1(name)))
            {
                radio->click();
            }
            else
            {
                ADD_FAILURE() << "the dialog has no radio button named " << name;
            }
        }

        //! The state of a row: the user chooses the other mode, and then the first one again
        Twins::State choosing(Mode other)
        {
            const bool kinematic = other == Mode::Kinematic;
            const char* const chosen = kinematic ? "kinematicRadioButton" : "simulatedRadioButton";
            const char* const before = kinematic ? "simulatedRadioButton" : "kinematicRadioButton";
            return { kinematic ? "kinematic chosen" : "simulated chosen",
                [chosen](QWidget& widget) { clickRadio(widget, chosen); },
                [before](QWidget& widget) { clickRadio(widget, before); } };
        }

        Mode otherThan(Mode mode)
        {
            return mode == Mode::Simulated ? Mode::Kinematic : Mode::Simulated;
        }

        //! One widget of the dialog in a mode, and the other mode as a state. The legacy twin is the dialog the legacy GUI builds (the
        //! sheets are set by the dialog, so the row quotes only the application's), in a chain of one link, which names the dialog as
        //! the form does.
        Twins::Row widgetRow(const char* site, Mode mode, const char* name, const QSize& size, unsigned compare)
        {
            Twins::Row row;
            row.m_site = site;
            row.m_make = [name](QWidget* dialog)
            {
                return childOf(dialog, name);
            };
            row.m_chain = { { .m_name = DialogName, .m_make = [mode](QWidget* parent) { return makeDialog(mode, parent); } } };
            row.m_realApplicationSheet = true;
            row.m_mayDecideNothing = true;      // the sheets that decide the look are the dialog's own, set when it is built (not quoted)
            row.m_register = &PhysX::Editor::KinematicDescriptionDialog::RegisterFlatElements;
            row.m_states = { choosing(otherThan(mode)) };
            row.m_compare = compare;
            row.m_size = size;
            row.m_rootSize = DialogSize;
            // The base of the sheet style is the AzQtComponents style, as the application's chain is (StyleManager.cpp:346-351): the
            // layouts of the form that give no margins of their own take them, and their spacing, from it (PM_Layout*)
            row.m_base = Twins::Base::AzQtComponentsStyle;
            return row;
        }

        //! The dialog whole in a mode: its geometry, and its picture at every device ratio
        Twins::Row dialogRow(const char* site, Mode mode)
        {
            Twins::Row row;
            row.m_site = site;
            row.m_make = [mode](QWidget* parent)
            {
                return makeDialog(mode, parent);
            };
            row.m_realApplicationSheet = true;
            row.m_mayDecideNothing = true;
            row.m_register = &PhysX::Editor::KinematicDescriptionDialog::RegisterFlatElements;
            row.m_states = { choosing(otherThan(mode)) };
            row.m_compare = Twins::Compare::Geometry | Twins::Compare::Pixels;
            row.m_size = DialogSize;
            row.m_rootSize = DialogSize;
            row.m_base = Twins::Base::AzQtComponentsStyle;
            return row;
        }

        constexpr unsigned BoxLook = Twins::Compare::Geometry | Twins::Compare::Fonts | Twins::Compare::Pixels;

        //! A header's colour: the roles the sheet's `color` sets, and what the sheet decides of it
        Twins::Row headerRow(const char* site, Mode mode, const char* name)
        {
            Twins::Row row = widgetRow(site, mode, name, QSize(47, 21), BoxLook | Twins::Compare::Palette);
            row.m_roles = { QPalette::WindowText, QPalette::Text, QPalette::ButtonText };
            return row;
        }

        //! The element of a widget, as the registry gave it ("": none)
        const char* boxElementOf(const QWidget* widget)
        {
            const AzQtComponents::BoxElement* element = AzQtComponents::BoxElement::forWidget(widget);
            return element ? element->m_name : "";
        }

        bool isPhysXElement(const char* elementName)
        {
            return std::string(elementName).rfind("PhysX.", 0) == 0;
        }

        QColor textColorOf(const QWidget* widget)
        {
            return widget->palette().color(QPalette::Active, QPalette::WindowText);
        }

        //! The Open Sans fonts the application loads (StyleManager::initializeFonts) while it lives. The form's text is rich text in
        //! Open Sans, which this process does not have: the fallback has wider glyphs, which wrap the text of label_3 and label_5 up under
        //! the title above it, where the legacy sheet's fill of the title hides the first line (a fill the box draws under it in the
        //! flattened GUI), and a row would compare that wrap and not the dialog.
        struct OpenSansFonts
        {
            OpenSansFonts()
            {
                for (const char* name : { "OpenSans-Bold.ttf", "OpenSans-BoldItalic.ttf", "OpenSans-ExtraBold.ttf",
                                          "OpenSans-ExtraBoldItalic.ttf", "OpenSans-Italic.ttf", "OpenSans-Light.ttf",
                                          "OpenSans-LightItalic.ttf", "OpenSans-Regular.ttf", "OpenSans-Semibold.ttf",
                                          "OpenSans-SemiboldItalic.ttf" })
                {
                    m_ids.push_back(
                        QFontDatabase::addApplicationFont(QStringLiteral(":/AzQtFonts/Fonts/Open_Sans/%1").arg(QString::fromLatin1(name))));
                }
            }

            ~OpenSansFonts()
            {
                for (const int id : m_ids)
                {
                    QFontDatabase::removeApplicationFont(id);
                }
            }

            OpenSansFonts(const OpenSansFonts&) = delete;
            OpenSansFonts& operator=(const OpenSansFonts&) = delete;

            std::vector<int> m_ids;
        };

        class KinematicDialogFlatSheetTests
            : public ::testing::Test
        {
        protected:
            void SetUp() override
            {
                for (const int id : m_fonts.m_ids)
                {
                    ASSERT_NE(id, -1) << "an Open Sans font of the application's resources did not load";
                }
            }

            //! Every widget of a tree under the flattened Style, polished as the application polishes them (the dialog registers its
            //! elements itself when it is built in the flattened GUI)
            void flatten(QWidget& top)
            {
                QList<QWidget*> widgets = top.findChildren<QWidget*>();
                widgets.prepend(&top);
                for (QWidget* widget : widgets)
                {
                    widget->setStyle(m_context.flatStyle());
                }
                for (QWidget* widget : widgets)
                {
                    widget->ensurePolished();
                }
            }

            OpenSansFonts m_fonts;
            Twins::Context m_context;
        };
    } // namespace KinematicDialogFlatSheetTestsInternal

    // A unity build joins this file with others of namespace UnitTest: the fixture is the only name brought in, the rest is named
    // through the alias
    using KinematicDialogFlatSheetTestsInternal::KinematicDialogFlatSheetTests;
    namespace Kinematic = KinematicDialogFlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // Resolution: which widgets have which element
    // ----------------------------------------------------------------------------------

    TEST_F(KinematicDialogFlatSheetTests, Registry_TheElementsReachTheNamedWidgetsOfTheDialog)
    {
        Kinematic::Twins::RungScope flat(Kinematic::Twins::Rung::Flat);
        QWidget root;
        PhysX::Editor::KinematicDescriptionDialog dialog(false, &root);
        flatten(dialog);

        const auto elementOf = [&dialog](const char* name)
        {
            return Kinematic::boxElementOf(Kinematic::childOf(&dialog, name));
        };

        for (const char* name : { "groupBox", "groupBox_2", "groupBox_5" })
        {
            EXPECT_STREQ(elementOf(name), "PhysX.Box.KinematicFrameless") << name;
        }
        EXPECT_STREQ(elementOf("frame"), "PhysX.Box.KinematicPanel");
        EXPECT_STREQ(elementOf("line"), "PhysX.Box.KinematicDivider");
        EXPECT_STREQ(elementOf("saveButton"), "PhysX.Box.KinematicSave");
        EXPECT_STREQ(elementOf("cancelButton"), "PhysX.Box.KinematicCancel");
        for (const char* name : { "validLabel1", "validLabel2", "invalidLabel1", "invalidLabel2" })
        {
            EXPECT_STREQ(elementOf(name), "PhysX.Box.KinematicNote") << name;
        }

        // The headers have a text element: the colour is in the palette (KinematicDescriptionDialog.ui:295, :311 before the flattening)
        EXPECT_EQ(Kinematic::textColorOf(Kinematic::childOf(&dialog, "valid")), QColor(0x58, 0xBC, 0x61));
        EXPECT_EQ(Kinematic::textColorOf(Kinematic::childOf(&dialog, "label_8")), QColor(0xFA, 0x27, 0x27));
    }

    TEST_F(KinematicDialogFlatSheetTests, Registry_TheWidgetsTheSheetsOnlyRoutedHaveNoElementOfTheDialog)
    {
        // The labels and the radio buttons whose `border: none` changes nothing the user sees (a label has no frame, a radio button is
        // drawn by the indicator element either way), and the widgets the form gave no sheet
        Kinematic::Twins::RungScope flat(Kinematic::Twins::Rung::Flat);
        QWidget root;
        PhysX::Editor::KinematicDescriptionDialog dialog(false, &root);
        flatten(dialog);

        for (const char* name : { "label", "label_2", "label_3", "label_4", "label_5", "simulatedRadioButton", "kinematicRadioButton",
                                  "selectedDescriptionLabel", "validIcon1", "validIcon2", "invalidIcon1", "invalidIcon2" })
        {
            EXPECT_FALSE(Kinematic::isPhysXElement(Kinematic::boxElementOf(Kinematic::childOf(&dialog, name)))) << name;
        }
    }

    TEST_F(KinematicDialogFlatSheetTests, Registry_TheWidgetsOfTheSameNamesOutsideTheDialogHaveNone)
    {
        // The names are Designer's own, which other windows have for their widgets: a generic name reaches an element only below the dialog
        Kinematic::Twins::RungScope flat(Kinematic::Twins::Rung::Flat);
        QWidget dialogParent;
        PhysX::Editor::KinematicDescriptionDialog dialog(false, &dialogParent);     // makes the dialog's elements known to the registry

        QWidget window;
        const auto named = [&window](auto* widget, const char* name)
        {
            widget->setObjectName(QString::fromLatin1(name));
            widget->setParent(&window);
            return widget;
        };
        auto* groupBox = named(new QGroupBox, "groupBox");
        auto* groupBox2 = named(new QGroupBox, "groupBox_2");
        auto* groupBox5 = named(new QGroupBox, "groupBox_5");
        auto* frame = named(new QFrame, "frame");
        auto* line = named(new QFrame, "line");
        auto* saveButton = named(new QPushButton, "saveButton");
        auto* cancelButton = named(new QPushButton, "cancelButton");
        auto* invalidHeader = named(new QLabel, "label_8");
        flatten(window);

        for (const QWidget* widget : std::initializer_list<const QWidget*>{ groupBox, groupBox2, groupBox5, frame, line, saveButton,
                                                                          cancelButton, invalidHeader })
        {
            EXPECT_FALSE(Kinematic::isPhysXElement(Kinematic::boxElementOf(widget))) << widget->objectName().toStdString();
        }
        EXPECT_NE(Kinematic::textColorOf(invalidHeader), QColor(0xFA, 0x27, 0x27));
    }

    TEST_F(KinematicDialogFlatSheetTests, Registry_TheChosenModesBoxHasTheSelectedElementAndTheOtherTheOption)
    {
        Kinematic::Twins::RungScope flat(Kinematic::Twins::Rung::Flat);
        QWidget root;
        PhysX::Editor::KinematicDescriptionDialog dialog(false, &root);
        flatten(dialog);

        QWidget* simulatedBox = Kinematic::childOf(&dialog, "simulatedBox");
        QWidget* kinematicBox = Kinematic::childOf(&dialog, "kinematicBox");
        EXPECT_STREQ(Kinematic::boxElementOf(simulatedBox), "PhysX.Box.KinematicOptionSelected");
        EXPECT_STREQ(Kinematic::boxElementOf(kinematicBox), "PhysX.Box.KinematicOption");

        // The user chooses the other mode: the tag moves with the choice, and each box is polished again with it
        Kinematic::clickRadio(dialog, "kinematicRadioButton");
        EXPECT_STREQ(Kinematic::boxElementOf(simulatedBox), "PhysX.Box.KinematicOption");
        EXPECT_STREQ(Kinematic::boxElementOf(kinematicBox), "PhysX.Box.KinematicOptionSelected");
        EXPECT_TRUE(dialog.GetResult());

        Kinematic::clickRadio(dialog, "simulatedRadioButton");
        EXPECT_STREQ(Kinematic::boxElementOf(simulatedBox), "PhysX.Box.KinematicOptionSelected");
        EXPECT_STREQ(Kinematic::boxElementOf(kinematicBox), "PhysX.Box.KinematicOption");
        EXPECT_FALSE(dialog.GetResult());
    }

    // ----------------------------------------------------------------------------------
    // Rungs: the legacy GUI sets the sheets, the flattened GUI sets none
    // ----------------------------------------------------------------------------------

    TEST_F(KinematicDialogFlatSheetTests, Rungs_TheLegacyRungSetsTheSheetsOfTheFormAndTheBoxes)
    {
        Kinematic::Twins::RungScope legacy(Kinematic::Twins::Rung::Qss);
        QWidget root;
        PhysX::Editor::KinematicDescriptionDialog dialog(false, &root);

        const auto sheetOf = [&dialog](const char* name)
        {
            return Kinematic::childOf(&dialog, name)->styleSheet();
        };

        // The sheets of the form (the .ui lines before the flattening: groupBox :79, groupBox_2 :264, valid :295, label_8 :311, the notes
        // :327 to :393, frame :415, line :434, saveButton :519), set by the dialog now
        for (const char* name : { "groupBox", "groupBox_2", "label_2", "label_3", "label_4", "label_5", "simulatedRadioButton",
                                  "kinematicRadioButton" })
        {
            EXPECT_TRUE(sheetOf(name).contains(QStringLiteral("border: none"))) << name;
        }
        EXPECT_EQ(sheetOf("valid"), QStringLiteral("color: #58BC61"));
        EXPECT_EQ(sheetOf("label_8"), QStringLiteral("color: #FA2727"));
        for (const char* name : { "validLabel1", "validLabel2", "invalidLabel1", "invalidLabel2" })
        {
            EXPECT_EQ(sheetOf(name), QStringLiteral("margin-left: 5px;")) << name;
        }
        EXPECT_TRUE(sheetOf("frame").contains(QStringLiteral("background-color: rgb(51, 51, 51);")));
        EXPECT_TRUE(sheetOf("line").contains(QStringLiteral("border-right: 1px solid")));
        EXPECT_EQ(sheetOf("saveButton"), QStringLiteral("background-color: rgb(30, 112, 235);"));
        EXPECT_TRUE(sheetOf("cancelButton").isEmpty());

        // The boxes of the two modes: the chosen one has the border, as OnButtonClicked set it before the flattening
        EXPECT_TRUE(sheetOf("simulatedBox").contains(QStringLiteral("border: 1px solid")));
        EXPECT_TRUE(sheetOf("kinematicBox").contains(QStringLiteral("border: none")));
    }

    TEST_F(KinematicDialogFlatSheetTests, Rungs_TheFlatRungSetsNoSheetAndTagsTheChosenBox)
    {
        Kinematic::Twins::RungScope flat(Kinematic::Twins::Rung::Flat);
        QWidget root;
        PhysX::Editor::KinematicDescriptionDialog dialog(true, &root);
        flatten(dialog);

        QList<QWidget*> widgets = dialog.findChildren<QWidget*>();
        widgets.prepend(&dialog);
        for (const QWidget* widget : widgets)
        {
            EXPECT_TRUE(widget->styleSheet().isEmpty()) << widget->objectName().toStdString();
            EXPECT_FALSE(widget->style()->inherits("QStyleSheetStyle")) << widget->objectName().toStdString();
        }

        const QString tag = QStringLiteral("KinematicSelected");
        EXPECT_FALSE(AzQtComponents::Style::hasClass(Kinematic::childOf(&dialog, "simulatedBox"), tag));
        EXPECT_TRUE(AzQtComponents::Style::hasClass(Kinematic::childOf(&dialog, "kinematicBox"), tag));
    }

    // ----------------------------------------------------------------------------------
    // The twins: the group boxes (U093, U102)
    // ----------------------------------------------------------------------------------

    TEST_F(KinematicDialogFlatSheetTests, Twin_TheGroupBoxesHaveNoFrame)
    {
        // `border: none` of groupBox (:79) and groupBox_2 (:264), which reaches groupBox_5: the base style's group box frame is not drawn
        for (const Kinematic::Mode mode : { Kinematic::Mode::Simulated, Kinematic::Mode::Kinematic })
        {
            EXPECT_TRUE(Kinematic::Twins::check(Kinematic::widgetRow("U093 groupBox", mode, "groupBox", QSize(301, 171),
                Kinematic::BoxLook), m_context));
            EXPECT_TRUE(Kinematic::Twins::check(Kinematic::widgetRow("U102 groupBox_2", mode, "groupBox_2", QSize(311, 253),
                Kinematic::BoxLook), m_context));
            EXPECT_TRUE(Kinematic::Twins::check(Kinematic::widgetRow("U093 groupBox_5", mode, "groupBox_5", QSize(51, 151),
                Kinematic::BoxLook), m_context));
        }
    }

    // ----------------------------------------------------------------------------------
    // The twins: the boxes of the two modes (C179-C182, U094, U097)
    // ----------------------------------------------------------------------------------

    TEST_F(KinematicDialogFlatSheetTests, Twin_TheBoxesOfTheModesHaveTheFillAndTheChosenOneTheBorder)
    {
        // Each box in each mode, and the other mode as a state: the box takes the border and gives it away
        for (const Kinematic::Mode mode : { Kinematic::Mode::Simulated, Kinematic::Mode::Kinematic })
        {
            EXPECT_TRUE(Kinematic::Twins::check(Kinematic::widgetRow("C179 simulatedBox", mode, "simulatedBox", QSize(301, 74),
                Kinematic::BoxLook), m_context));
            EXPECT_TRUE(Kinematic::Twins::check(Kinematic::widgetRow("C181 kinematicBox", mode, "kinematicBox", QSize(301, 74),
                Kinematic::BoxLook), m_context));
        }
    }

    // ----------------------------------------------------------------------------------
    // The twins: the headers and the notes (U103-U108)
    // ----------------------------------------------------------------------------------

    TEST_F(KinematicDialogFlatSheetTests, Twin_TheHeadersAreGreenAndRed)
    {
        for (const Kinematic::Mode mode : { Kinematic::Mode::Simulated, Kinematic::Mode::Kinematic })
        {
            EXPECT_TRUE(Kinematic::Twins::check(Kinematic::headerRow("U103 valid", mode, "valid"), m_context));
            EXPECT_TRUE(Kinematic::Twins::check(Kinematic::headerRow("U104 label_8", mode, "label_8"), m_context));
        }
    }

    TEST_F(KinematicDialogFlatSheetTests, Twin_TheNotesAreIndentedFivePixels)
    {
        // `margin-left: 5px;` over the label rule's margin above and below (Text.qss:93-94): the text starts five pixels in
        const struct
        {
            const char* m_site;
            const char* m_name;
            QSize m_size;
        } notes[] = {
            { "U105 validLabel1", "validLabel1", QSize(111, 21) },
            { "U106 validLabel2", "validLabel2", QSize(111, 21) },
            { "U107 invalidLabel1", "invalidLabel1", QSize(101, 21) },
            { "U108 invalidLabel2", "invalidLabel2", QSize(101, 21) },
        };
        for (const auto& note : notes)
        {
            for (const Kinematic::Mode mode : { Kinematic::Mode::Simulated, Kinematic::Mode::Kinematic })
            {
                EXPECT_TRUE(Kinematic::Twins::check(
                    Kinematic::widgetRow(note.m_site, mode, note.m_name, note.m_size, Kinematic::BoxLook), m_context));
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // The twins: the panel, its divider and the buttons (U109-U111, U102)
    // ----------------------------------------------------------------------------------

    TEST_F(KinematicDialogFlatSheetTests, Twin_ThePanelIsFilledAndHasNoBorder)
    {
        EXPECT_TRUE(Kinematic::Twins::check(Kinematic::widgetRow("U109 frame", Kinematic::Mode::Simulated, "frame", QSize(291, 191),
            Kinematic::BoxLook), m_context));
    }

    TEST_F(KinematicDialogFlatSheetTests, Twin_TheDividerIsAOnePixelLineAtItsRightEdge)
    {
        EXPECT_TRUE(Kinematic::Twins::check(Kinematic::widgetRow("U110 line", Kinematic::Mode::Simulated, "line", QSize(20, 81),
            Kinematic::BoxLook), m_context));
    }

    TEST_F(KinematicDialogFlatSheetTests, Twin_SaveIsFlatBlueInEveryState)
    {
        Kinematic::Twins::Row row = Kinematic::widgetRow("U111 saveButton", Kinematic::Mode::Simulated, "saveButton", QSize(121, 23),
            Kinematic::BoxLook | Kinematic::Twins::Compare::Palette);
        row.m_roles = { QPalette::ButtonText, QPalette::WindowText };
        row.m_states = { Kinematic::Twins::States::hover(), Kinematic::Twins::States::pressed() };
        EXPECT_TRUE(Kinematic::Twins::check(row, m_context));
    }

    TEST_F(KinematicDialogFlatSheetTests, Twin_CancelIsItsLabelAlone)
    {
        // The legacy look of the button is the one to confirm here: the `border: none` of groupBox_2 gives its rule a border of its own,
        // which the sheet style draws as the clear fill with the label (the element is written from that reading of the Qt sources)
        Kinematic::Twins::Row row = Kinematic::widgetRow("U102 cancelButton", Kinematic::Mode::Simulated, "cancelButton",
            QSize(121, 23), Kinematic::BoxLook);
        row.m_states = { Kinematic::Twins::States::hover(), Kinematic::Twins::States::pressed() };
        EXPECT_TRUE(Kinematic::Twins::check(row, m_context));
    }

    // ----------------------------------------------------------------------------------
    // The twins: the widgets whose sheet is routed only (U095, U096, U098-U101), and the dialog whole
    // ----------------------------------------------------------------------------------

    TEST_F(KinematicDialogFlatSheetTests, Twin_TheRoutedLabelsAndRadioButtonsLookAsUnderTheirSheets)
    {
        // `border: none` on a label or a radio button changes nothing the user sees: no element, and the twins agree. A label alone is
        // compared by its geometry and its font: the legacy label draws the fill of the box above it (a cascade of the box's sheet) over
        // that same fill, which the picture of the box proves with the labels in it (the rows of the boxes), and which the label alone
        // shows against the harness's grey.
        constexpr unsigned LabelLook = Kinematic::Twins::Compare::Geometry | Kinematic::Twins::Compare::Fonts;
        const struct
        {
            const char* m_name;
            QSize m_size;
            unsigned m_compare;
        } routed[] = {
            { "label_2", QSize(71, 21), LabelLook },
            { "label_3", QSize(281, 51), LabelLook },
            { "label_4", QSize(71, 21), LabelLook },
            { "label_5", QSize(281, 51), LabelLook },
            { "simulatedRadioButton", QSize(21, 21), Kinematic::BoxLook },
            { "kinematicRadioButton", QSize(21, 21), Kinematic::BoxLook },
        };
        for (const auto& widget : routed)
        {
            const Kinematic::Twins::Row row = Kinematic::widgetRow(
                widget.m_name, Kinematic::Mode::Simulated, widget.m_name, widget.m_size, widget.m_compare);
            EXPECT_TRUE(Kinematic::Twins::check(row, m_context)) << widget.m_name;
        }
    }

    TEST_F(KinematicDialogFlatSheetTests, Twin_TheDialogIsPaintedAndLaidOutAsUnderTheSheets)
    {
        EXPECT_TRUE(Kinematic::Twins::check(Kinematic::dialogRow("C179 the dialog, Simulated", Kinematic::Mode::Simulated), m_context));
        EXPECT_TRUE(Kinematic::Twins::check(Kinematic::dialogRow("C181 the dialog, Kinematic", Kinematic::Mode::Kinematic), m_context));
    }
} // namespace UnitTest
