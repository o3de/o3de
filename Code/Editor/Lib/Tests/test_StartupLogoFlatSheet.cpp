/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Tests/SheetTwins.h>
#include <EditorFlatSheet.h>
#include <EditorFlatValues.h>
#include <StartupLogoDialog.h>

#include <QApplication>
#include <QLabel>
#include <QPalette>
#include <QString>
#include <QStyleFactory>
#include <QToolTip>
#include <QWidget>

#include <memory>
#include <string>
#include <vector>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The sheets of the startup logo dialog (StartupLogoDialog.cpp): the one the splash screen sets on itself (:54-55 before the
    // flattening) and the one the About box sets on itself (:65-66), converted to elements (EditorFlatSheet::RegisterStartupLogoElements).
    // The twins are built from the real dialog, in each mode: the legacy twin is Qt's own style sheet style over Fusion with the real
    // application sheet (BaseStyleSheet.qss and the files it imports) and the dialog's sheet quoted below on the dialog; the flat twin is
    // the flattened Style with no sheet at all, and with no registrar run by the test: the dialog registers its own elements when it is
    // made, as the splash must (it is made before the main window sets Editor.qss). The labels are compared one by one, each as the
    // dialog builds it and as the application's rules reach it (BaseStyleSheet.qss:15-21 the universal rule, :60-66 the rule of a dialog,
    // Text.qss:89-95 the rule of a label), and the dialogs whole. The rows named AsMade_ build the flat twin in the order of the
    // application: under the flattened Style as its style, which the first rows cannot (they style a dialog that is already made).
    // ----------------------------------------------------------------------------------

    namespace StartupLogoFlatSheetTestsInternal
    {
        namespace Twins = AzQtComponents::SheetTwins;

        constexpr const char* DialogName = "StartupLogoDialog";         // the name the form gives the dialog (StartupLogoDialog.ui:4)
        constexpr const char* VersionText = "Development Build";
        constexpr const char* CopyrightText = "Copyright &#xA9; Contributors to the Open 3D Engine Project";

        // StartupLogoDialog.cpp:54-55 before the flattening, the sheet the splash screen sets on itself
        constexpr const char* SplashLabelRule = "QLabel { background: transparent; color: 'white' }";
        constexpr const char* SplashNoticeRule = "QLabel#copyrightNotice { color: #AAAAAA; font-size: 9px; } ";
        // StartupLogoDialog.cpp:65-66 before the flattening, the sheet the About box sets on itself
        constexpr const char* AboutNoticeRule = "QLabel#copyrightNotice { color: #AAAAAA; font-size: 9px; }";
        constexpr const char* AboutLinkRule = "QLabel#link { text-decoration: underline; color: #94D2FF; }";

        constexpr QSize DialogSize(CStartupLogoDialog::EnforcedWidth, CStartupLogoDialog::EnforcedHeight);

        using Mode = CStartupLogoDialog::DialogType;

        QWidget* makeDialog(Mode mode, QWidget* parent)
        {
            return new CStartupLogoDialog(mode, QString::fromLatin1(VersionText), QString::fromLatin1(CopyrightText), parent);
        }

        //! The sheets the dialog of a mode sets on itself, quoted
        std::vector<Twins::Sheet> quotedSheets(Mode mode, Twins::SetOn place)
        {
            if (mode == CStartupLogoDialog::Loading)
            {
                return { { "StartupLogoDialog.cpp:54 before the flattening", place, SplashLabelRule },
                         { "StartupLogoDialog.cpp:55 before the flattening", place, SplashNoticeRule } };
            }
            return { { "StartupLogoDialog.cpp:65 before the flattening", place, AboutNoticeRule },
                     { "StartupLogoDialog.cpp:66 before the flattening", place, AboutLinkRule } };
        }

        //! The roles the sheets decide, and the link role the anchor of a rich text label is drawn with
        std::vector<QPalette::ColorRole> roles()
        {
            return { QPalette::WindowText, QPalette::Text, QPalette::ButtonText, QPalette::Link };
        }

        //! A label of the dialog, found by its name. A dialog without it is a failure of the test, and the dialog itself stands in so
        //! that the harness does not run on nothing.
        QWidget* labelOf(QWidget* dialog, const char* name)
        {
            if (QWidget* label = dialog->findChild<QLabel*>(QString::fromLatin1(name)))
            {
                return label;
            }
            ADD_FAILURE() << "the dialog has no label named " << name;
            return dialog;
        }

        //! One label of the dialog in a mode, with the sheet the dialog sets on itself in that mode. The labels are not shown: their
        //! facts, fonts, colours and pictures are what the rules decide, in the idle state and disabled (the sheets name no state, so
        //! they hold in every one).
        Twins::Row labelRow(const char* site, Mode mode, const char* labelName)
        {
            Twins::Row row;
            row.m_site = site;
            row.m_make = [labelName](QWidget* dialog)
            {
                return labelOf(dialog, labelName);
            };
            row.m_chain = { { .m_name = DialogName, .m_make = [mode](QWidget* parent) { return makeDialog(mode, parent); } } };
            row.m_sheets = quotedSheets(mode, Twins::SetOn::Ancestor);
            row.m_realApplicationSheet = true;
            row.m_states = { Twins::States::disabled() };
            row.m_compare = Twins::Compare::Geometry | Twins::Compare::Fonts | Twins::Compare::Palette | Twins::Compare::Pixels;
            row.m_roles = roles();
            row.m_rootSize = DialogSize;
            // The base of the sheet style is the AzQtComponents style, as the application's chain is (StyleManager.cpp:346-351): the
            // layouts of the form that give no margins of their own take them, and their spacing, from it (PM_Layout*), and Fusion's differ
            row.m_base = Twins::Base::AzQtComponentsStyle;
            return row;
        }

        //! The dialog whole in a mode: its geometry, and its picture at every device ratio, which holds the labels where the layout of
        //! the form puts them
        Twins::Row dialogRow(const char* site, Mode mode)
        {
            Twins::Row row;
            row.m_site = site;
            row.m_make = [mode](QWidget* parent)
            {
                return makeDialog(mode, parent);
            };
            row.m_sheets = quotedSheets(mode, Twins::SetOn::Widget);
            row.m_realApplicationSheet = true;
            row.m_compare = Twins::Compare::Geometry | Twins::Compare::Pixels;
            row.m_size = DialogSize;
            row.m_rootSize = DialogSize;
            row.m_base = Twins::Base::AzQtComponentsStyle;         // as the rows of the labels, for the layouts of the form
            return row;
        }

        //! One twin of a row, built, read idle and in each state, and gone
        std::vector<Twins::Reading> readTwin(const Twins::Row& row, Twins::Context& context, bool legacy)
        {
            Twins::RungScope rung(legacy ? row.m_legacyRung : Twins::Rung::Flat);
            const Twins::Twin twin(row, context, legacy);
            return Twins::readStates(row, *twin.widget(), context, legacy, nullptr);
        }

        // ------------------------------------------------------------------------------
        // The dialog as the application makes it
        // ------------------------------------------------------------------------------

        //! The application as the flattened GUI has it when the dialog is made: the flattened Style is the style of the application
        //! (StyleManager.cpp:244-246), so the labels of the form are polished by it while the form is built, and nothing restyles them
        //! after. The twins of the rows above set the style on a dialog that is already made (SheetTwins.h, Twin), and so the elements
        //! are first resolved with the names and the mode in place; this scope is the order of the real application. What the style
        //! changes in the process (the palette and the class fonts of the application, the palette of the tips) is put back.
        class FlatApplicationScope
        {
        public:
            FlatApplicationScope()
                : m_rung(Twins::Rung::Flat)
                , m_palette(QApplication::palette())
                , m_font(QApplication::font())
                , m_tipPalette(QToolTip::palette())
            {
                QApplication::setStyle(new AzQtComponents::Style(QStyleFactory::create(QStringLiteral("Fusion"))));
            }

            ~FlatApplicationScope()
            {
                QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
                QToolTip::setPalette(m_tipPalette);
                QApplication::setPalette(m_palette);
                QApplication::setFont(m_font);                          // no class name: the class fonts of the process are cleared
                AzQtComponents::TextElement::installClassFonts();       // and the process has those of the flattened GUI between twins
            }

            FlatApplicationScope(const FlatApplicationScope&) = delete;
            FlatApplicationScope& operator=(const FlatApplicationScope&) = delete;

        private:
            Twins::RungScope m_rung;
            QPalette m_palette;
            QFont m_font;
            QPalette m_tipPalette;
        };

        //! The flat reading of a row with the dialog made as the application makes it: under the flattened Style as the style of the
        //! application, with no parent (CryEdit.cpp makes it so), polished as showing it polishes it (QWidget::ensurePolished reaches
        //! the widgets of the hidden page too), and not styled again after. The label the row names is read; no name: the dialog.
        std::vector<Twins::Reading> readFlatAsMade(
            const Twins::Row& row,
            Mode mode,
            const char* labelName,
            const Twins::Context& context,
            const std::vector<Twins::Reading>& legacy)
        {
            const FlatApplicationScope application;
            const std::unique_ptr<QWidget> dialog(makeDialog(mode, nullptr));
            QWidget* widget = labelName ? labelOf(dialog.get(), labelName) : dialog.get();
            dialog->ensurePolished();
            if (row.m_size.isValid())
            {
                widget->resize(row.m_size);
            }
            else
            {
                widget->adjustSize();
            }
            QApplication::processEvents();
            return Twins::readStates(row, *widget, context, false, &legacy);
        }

        //! A fact of a reading by its name, or "(none)"
        std::string factOf(const Twins::Reading& reading, const char* name)
        {
            for (const Twins::Fact& fact : reading.m_facts)
            {
                if (fact.m_name == name)
                {
                    return fact.m_text;
                }
            }
            return "(none)";
        }

        //! Whether a text holds a part
        bool holds(const std::string& text, const char* part)
        {
            return text.find(part) != std::string::npos;
        }

        //! The labels whose picture is blank in the legacy twin, as the rows above set it: the splash gives m_transparentTrademarks no
        //! text, and the notice of the splash's page is a word-wrapped label of an expanding height that the layout of the hidden page
        //! has not sized when the About box's idle picture at ratio 1 is taken
        bool mayDrawNothing(Mode mode, const char* labelName)
        {
            return (mode == Mode::Loading && qstrcmp(labelName, "m_transparentTrademarks") == 0)
                || (mode == Mode::About && qstrcmp(labelName, "m_TransparentConfidential") == 0);
        }

        class StartupLogoFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            //! The row of a label (or, with no name, of the dialog) compared as the application makes the dialog: the legacy twin as the
            //! rows above build it, and the flat reading from a dialog made under the flattened Style as the application's style
            ::testing::AssertionResult asMadeMatchesTheSheet(Mode mode, const char* site, const char* labelName)
            {
                Twins::Row row = labelName ? labelRow(site, mode, labelName) : dialogRow(site, mode);
                row.m_mayDrawNothing = labelName && mayDrawNothing(mode, labelName);
                const std::vector<Twins::Reading> legacy = readTwin(row, m_context, true);
                const std::vector<Twins::Reading> flat = readFlatAsMade(row, mode, labelName, m_context, legacy);

                Twins::Result result;
                for (size_t i = 0; i < legacy.size() && i < flat.size(); ++i)
                {
                    Twins::compareReadings(row, legacy[i], flat[i], result);
                }
                if (legacy.size() != flat.size() || result.m_compared == 0)
                {
                    result.m_flaws.push_back(std::string(site) + ": the readings are not twins, or compare nothing");
                }
                if (result.ok())
                {
                    return ::testing::AssertionSuccess();
                }
                return ::testing::AssertionFailure() << "the dialog as the application makes it differs from the sheet at " << site
                                                     << ":\n" << result.text();
            }

            Twins::Context m_context;
        };
    } // namespace StartupLogoFlatSheetTestsInternal

    // A unity build joins this file with others of namespace UnitTest: the fixture is the only name brought in, the rest is named
    // through the alias
    using StartupLogoFlatSheetTestsInternal::StartupLogoFlatSheetTests;
    namespace Logo = StartupLogoFlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // The splash screen: StartupLogoDialog.cpp:54-55 before the flattening
    // ----------------------------------------------------------------------------------

    TEST_F(StartupLogoFlatSheetTests, Splash_TheNoticeIsNinePixelsAndGreyLikeTheSheet)
    {
        // C006: the label named copyrightNotice, a rich text label, in every state; the flat twin has no sheet (the harness flags one)
        EXPECT_TRUE(Logo::Twins::check(
            Logo::labelRow("C006 the copyright notice of the splash screen", Logo::Mode::Loading, "copyrightNotice"), m_context));
    }

    TEST_F(StartupLogoFlatSheetTests, Splash_TheOtherLabelsAreWhatTheApplicationsRulesMakeThem)
    {
        // `QLabel { background: transparent; color: 'white' }` (:54) is the universal rule's: the title, the version, the line of
        // progress (a code font of 8pt that the label rule's 12px outranks), and the labels of the hidden page of the About box,
        // among them the link, which the splash's sheet has no rule for
        for (const char* name : { "label_3", "m_TransparentVersion", "m_TransparentText", "link", "m_transparentTrademarks",
                                  "m_transparentAllRightReserved" })
        {
            Logo::Twins::Row row = Logo::labelRow(name, Logo::Mode::Loading, name);
            row.m_mayDrawNothing = qstrcmp(name, "m_transparentTrademarks") == 0;       // the splash gives it no text (the form: empty)
            EXPECT_TRUE(Logo::Twins::check(row, m_context)) << name;
        }
    }

    TEST_F(StartupLogoFlatSheetTests, Splash_TheDialogIsPaintedAndLaidOutAsUnderTheSheet)
    {
        EXPECT_TRUE(Logo::Twins::check(Logo::dialogRow("C006 the splash screen", Logo::Mode::Loading), m_context));
    }

    // ----------------------------------------------------------------------------------
    // The About box: StartupLogoDialog.cpp:65-66 before the flattening
    // ----------------------------------------------------------------------------------

    TEST_F(StartupLogoFlatSheetTests, About_TheNoticeIsNinePixelsAndGreyLikeTheSheet)
    {
        EXPECT_TRUE(Logo::Twins::check(
            Logo::labelRow("C007 the copyright notice of the About box", Logo::Mode::About, "copyrightNotice"), m_context));
    }

    TEST_F(StartupLogoFlatSheetTests, About_TheLinkIsUnderlinedAndLightBlueLikeTheSheet)
    {
        EXPECT_TRUE(Logo::Twins::check(Logo::labelRow("C007 the link of the About box", Logo::Mode::About, "link"), m_context));
    }

    TEST_F(StartupLogoFlatSheetTests, About_TheOtherLabelsAreWhatTheApplicationsRulesMakeThem)
    {
        // The labels of the About page the sheet has no rule for, and those of the splash's page, hidden behind it
        for (const char* name : { "label", "m_transparentTrademarks", "label_3", "m_TransparentVersion", "m_TransparentText",
                                  "m_TransparentConfidential" })
        {
            Logo::Twins::Row row = Logo::labelRow(name, Logo::Mode::About, name);
            // The notice of the splash's page is a word-wrapped label of an expanding height, which the layout of the hidden page has
            // not sized when the idle picture at ratio 1 is taken: it holds no text there, and the pictures after the layout hold it
            row.m_mayDrawNothing = qstrcmp(name, "m_TransparentConfidential") == 0;
            EXPECT_TRUE(Logo::Twins::check(row, m_context)) << name;
        }
    }

    TEST_F(StartupLogoFlatSheetTests, About_TheDialogIsPaintedAndLaidOutAsUnderTheSheet)
    {
        EXPECT_TRUE(Logo::Twins::check(Logo::dialogRow("C007 the About box", Logo::Mode::About), m_context));
    }

    // ----------------------------------------------------------------------------------
    // The dialog as the application makes it: under the flattened Style, polished while the form is built
    //
    // The form polishes its labels when it is built (QStackedLayout shows the first page when the page is added to it, and showing a
    // widget polishes it and everything below it: QWidget::ensurePolished), before the constructor names the notice and gives the
    // dialog its mode, which the matchers of the elements read. The rows above style a dialog that is already made, so they cannot
    // see that order; these rows build it under the flattened Style as the application's style, with nothing to style it again.
    // ----------------------------------------------------------------------------------

    TEST_F(StartupLogoFlatSheetTests, AsMade_Splash_TheNoticeIsNinePixelsAndGreyLikeTheSheet)
    {
        EXPECT_TRUE(asMadeMatchesTheSheet(Logo::Mode::Loading, "C006 the notice of the splash screen as made", "copyrightNotice"));
    }

    TEST_F(StartupLogoFlatSheetTests, AsMade_Splash_TheOtherLabelsAreWhatTheApplicationsRulesMakeThem)
    {
        for (const char* name : { "label_3", "m_TransparentVersion", "m_TransparentText", "link", "m_transparentTrademarks",
                                  "m_transparentAllRightReserved" })
        {
            EXPECT_TRUE(asMadeMatchesTheSheet(Logo::Mode::Loading, name, name)) << name;
        }
    }

    TEST_F(StartupLogoFlatSheetTests, AsMade_Splash_TheDialogIsPaintedAndLaidOutAsUnderTheSheet)
    {
        EXPECT_TRUE(asMadeMatchesTheSheet(Logo::Mode::Loading, "C006 the splash screen as made", nullptr));
    }

    TEST_F(StartupLogoFlatSheetTests, AsMade_About_TheNoticeIsNinePixelsAndGreyLikeTheSheet)
    {
        EXPECT_TRUE(asMadeMatchesTheSheet(Logo::Mode::About, "C007 the notice of the About box as made", "copyrightNotice"));
    }

    TEST_F(StartupLogoFlatSheetTests, AsMade_About_TheLinkIsUnderlinedAndLightBlueLikeTheSheet)
    {
        EXPECT_TRUE(asMadeMatchesTheSheet(Logo::Mode::About, "C007 the link of the About box as made", "link"));
    }

    TEST_F(StartupLogoFlatSheetTests, AsMade_About_TheOtherLabelsAreWhatTheApplicationsRulesMakeThem)
    {
        for (const char* name : { "label", "m_transparentTrademarks", "label_3", "m_TransparentVersion", "m_TransparentText",
                                  "m_TransparentConfidential" })
        {
            EXPECT_TRUE(asMadeMatchesTheSheet(Logo::Mode::About, name, name)) << name;
        }
    }

    TEST_F(StartupLogoFlatSheetTests, AsMade_About_TheDialogIsPaintedAndLaidOutAsUnderTheSheet)
    {
        EXPECT_TRUE(asMadeMatchesTheSheet(Logo::Mode::About, "C007 the About box as made", nullptr));
    }

    TEST_F(StartupLogoFlatSheetTests, AsMade_TheNoticeAndTheLinkHaveTheValuesOfTheSheets)
    {
        // The values themselves, read from the dialogs as made, so that the rows above are not the only witness of them
        const Logo::FlatApplicationScope application;
        for (const Logo::Mode mode : { Logo::Mode::Loading, Logo::Mode::About })
        {
            const std::unique_ptr<QWidget> dialog(Logo::makeDialog(mode, nullptr));
            dialog->ensurePolished();
            const QWidget* notice = Logo::labelOf(dialog.get(), "copyrightNotice");
            const QWidget* link = Logo::labelOf(dialog.get(), "link");
            const bool about = mode == Logo::Mode::About;
            const char* where = about ? "the About box" : "the splash screen";

            EXPECT_EQ(notice->font().pixelSize(), EditorFlatValues::StartupCopyrightNoticeSize) << where;
            for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
            {
                for (const QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText })
                {
                    EXPECT_EQ(Logo::Twins::colorText(notice->palette().color(group, role)),
                        Logo::Twins::colorText(EditorFlatValues::StartupCopyrightNoticeText)) << where;
                }
            }

            // The link sheet is the About box's alone: the splash's link, on the hidden page, is a plain label
            EXPECT_EQ(link->font().underline(), about) << where;
            EXPECT_EQ(link->palette().color(QPalette::Active, QPalette::WindowText) == EditorFlatValues::StartupLinkText, about) << where;
        }
    }

    // ----------------------------------------------------------------------------------
    // What the elements are, and where they hold
    // ----------------------------------------------------------------------------------

    TEST_F(StartupLogoFlatSheetTests, Facts_TheLegacyNoticeIsNinePixelsGreyAndTheFlatNoticeIsTheSame)
    {
        const Logo::Twins::Row row = Logo::labelRow("C006", Logo::Mode::Loading, "copyrightNotice");
        const std::vector<Logo::Twins::Reading> legacy = Logo::readTwin(row, m_context, true);
        const std::vector<Logo::Twins::Reading> flat = Logo::readTwin(row, m_context, false);
        ASSERT_EQ(legacy.size(), size_t(2));
        ASSERT_EQ(flat.size(), size_t(2));

        // The label rule's 12px is outranked by the dialog's 9px, in the legacy twin as in the flat one
        EXPECT_TRUE(Logo::holds(Logo::factOf(legacy[0], "font.request"), "9px")) << Logo::factOf(legacy[0], "font.request");
        EXPECT_TRUE(Logo::holds(Logo::factOf(flat[0], "font.request"), "9px")) << Logo::factOf(flat[0], "font.request");
        for (const char* group : { "Active", "Inactive", "Disabled" })
        {
            const std::string name = std::string("palette.") + group + ".WindowText";
            EXPECT_EQ(Logo::factOf(legacy[0], name.c_str()), "#ffaaaaaa") << name << " of the legacy twin";
            EXPECT_EQ(Logo::factOf(flat[0], name.c_str()), "#ffaaaaaa") << name << " of the flat twin";
        }

        // And disabled: the sheet names no state, so the grey holds in every one
        EXPECT_EQ(Logo::factOf(legacy[1], "palette.Disabled.WindowText"), "#ffaaaaaa");
        EXPECT_EQ(Logo::factOf(flat[1], "palette.Disabled.WindowText"), "#ffaaaaaa");
    }

    TEST_F(StartupLogoFlatSheetTests, Facts_TheLinkIsUnderlinedInTheAboutBoxAndInNoOtherMode)
    {
        const Logo::Twins::Row about = Logo::labelRow("C007", Logo::Mode::About, "link");
        const Logo::Twins::Row splash = Logo::labelRow("C007 on the hidden page of the splash", Logo::Mode::Loading, "link");
        for (const bool legacy : { true, false })
        {
            const std::string twin = legacy ? "legacy" : "flat";
            const std::vector<Logo::Twins::Reading> aboutReadings = Logo::readTwin(about, m_context, legacy);
            const std::vector<Logo::Twins::Reading> splashReadings = Logo::readTwin(splash, m_context, legacy);

            EXPECT_TRUE(Logo::holds(Logo::factOf(aboutReadings[0], "font.request"), "underline")) << twin;
            EXPECT_EQ(Logo::factOf(aboutReadings[0], "palette.Active.WindowText"), "#ff94d2ff") << twin;
            EXPECT_FALSE(Logo::holds(Logo::factOf(splashReadings[0], "font.request"), "underline")) << twin;
            EXPECT_EQ(Logo::factOf(splashReadings[0], "palette.Active.WindowText"), "#ffffffff") << twin;
        }
    }

    TEST_F(StartupLogoFlatSheetTests, Facts_ANoticeOutsideTheDialogHasTheLabelRulesTwelvePixelsAndTheRuleDoesNotReachIt)
    {
        // The same label and the same rule on a window that is not the dialog: the legacy twin has the rule (the sheet is on the window),
        // the flat twin has no element for a notice outside the dialog, so the twins differ in the font, the first fact that differs
        Logo::Twins::Row other = Logo::labelRow("a notice of another window", Logo::Mode::Loading, "copyrightNotice");
        other.m_make = [](QWidget* parent)
        {
            auto* label = new QLabel(QString::fromLatin1(Logo::CopyrightText), parent);
            label->setObjectName(QStringLiteral("copyrightNotice"));
            label->setTextFormat(Qt::RichText);
            return label;
        };
        other.m_chain = { { .m_name = "SomeOtherWindow", .m_className = "CSomeOtherWindow" } };
        other.m_expectedDifference = "font.request";
        EXPECT_TRUE(Logo::Twins::check(other, m_context));

        const std::vector<Logo::Twins::Reading> flat = Logo::readTwin(other, m_context, false);
        EXPECT_TRUE(Logo::holds(Logo::factOf(flat[0], "font.request"), "12px")) << Logo::factOf(flat[0], "font.request");
    }

    TEST_F(StartupLogoFlatSheetTests, Legacy_TheSheetsTheDialogSetsAreTheRulesItSetBefore)
    {
        // The legacy GUI's sheet text is written from the named values now (EditorFlatValues.h): the same rules as the strings of the
        // quotes, apart from the spaces between them and the case of a colour
        const auto normal = [](const QString& sheet)
        {
            return sheet.toLower().simplified();
        };
        EXPECT_EQ(normal(EditorFlatValues::StartupLogoLoadingSheet()),
            normal(QString::fromLatin1(Logo::SplashLabelRule) + QLatin1Char(' ') + QString::fromLatin1(Logo::SplashNoticeRule)));
        EXPECT_EQ(normal(EditorFlatValues::StartupLogoAboutSheet()),
            normal(QString::fromLatin1(Logo::AboutNoticeRule) + QLatin1Char(' ') + QString::fromLatin1(Logo::AboutLinkRule)));

        // And it is the sheet the dialog sets on itself in the legacy GUI, in each mode
        Logo::Twins::RungScope legacy(Logo::Twins::Rung::Qss);
        QWidget root;
        const QWidget* splash = Logo::makeDialog(Logo::Mode::Loading, &root);
        const QWidget* about = Logo::makeDialog(Logo::Mode::About, &root);
        EXPECT_EQ(splash->styleSheet(), EditorFlatValues::StartupLogoLoadingSheet());
        EXPECT_EQ(about->styleSheet(), EditorFlatValues::StartupLogoAboutSheet());
    }

    TEST_F(StartupLogoFlatSheetTests, Flat_TheDialogSetsNoSheetAndOnlyTheAboutBoxIsTagged)
    {
        Logo::Twins::RungScope flat(Logo::Twins::Rung::Flat);
        QWidget root;
        const QWidget* splash = Logo::makeDialog(Logo::Mode::Loading, &root);
        const QWidget* about = Logo::makeDialog(Logo::Mode::About, &root);
        EXPECT_TRUE(splash->styleSheet().isEmpty());
        EXPECT_TRUE(about->styleSheet().isEmpty());

        // The About mode is the flat GUI's to read, as the class tag of the dialog: the splash has no tag
        EXPECT_TRUE(AzQtComponents::Selectors::tagged(about, EditorFlatSheet::StartupLogoAboutTag));
        EXPECT_FALSE(AzQtComponents::Selectors::tagged(splash, EditorFlatSheet::StartupLogoAboutTag));
    }

    TEST_F(StartupLogoFlatSheetTests, Legacy_TheDialogHasNoTagAndTheSheetsAreUnchanged)
    {
        // The About tag is a state of the flattened GUI: the legacy GUI sets no tag, and its sheet reads the object names alone
        Logo::Twins::RungScope legacy(Logo::Twins::Rung::Qss);
        QWidget root;
        for (const Logo::Mode mode : { Logo::Mode::Loading, Logo::Mode::About })
        {
            const QWidget* dialog = Logo::makeDialog(mode, &root);
            EXPECT_FALSE(AzQtComponents::Selectors::tagged(dialog, EditorFlatSheet::StartupLogoAboutTag));
            EXPECT_FALSE(dialog->styleSheet().isEmpty());
        }
    }
} // namespace UnitTest
