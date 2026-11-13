/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Tests/SheetTwins.h>
#include <AzQtComponents/Tests/SiteTwins.h>
#include <AzToolsFramework/UI/UICore/ProgressShield.hxx>

#include <QApplication>
#include <QBrush>
#include <QFrame>
#include <QGradient>
#include <QLabel>
#include <QLinearGradient>
#include <QProgressBar>
#include <QWidget>

#include <string>
#include <vector>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // U022 and U023, ProgressShield.ui: the sheets of the shield (a gradient behind the window, and the colour of its labels) and of its
    // frame (a translucent card with a white line and round corners). They were set from the form; they are set through
    // StyleManager::setLegacyStyleSheet after setupUi now, and the flattened GUI carries them in elements (BoxElement for the shield and
    // the frame, TextElement for the label). The shield is a Qt::ToolTip window with a translucent background over its parent.
    // The legacy twin is Qt's own style sheet style over AzQtComponents::Style with the application's real sheet; the flat twin is the
    // flattened Style with the elements of the real class (SheetTwins.h).
    // ----------------------------------------------------------------------------------

    namespace ProgressShieldFlatSheetTestsInternal
    {
        namespace Twins = AzQtComponents::SheetTwins;
        using ProgressShield = AzToolsFramework::ProgressShield;

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

        class ProgressShieldFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            ApplicationSheetGuard m_applicationSheet;
            Twins::Context m_context;
        };

        // The sheets the form set, verbatim (ProgressShield.ui:17-22 and :56-61, before the flattening)
        constexpr const char* FormShieldSheet =
            "QWidget #progressShield { background-color:qlineargradient(spread:reflect, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(0, 0, 0, 50), "
            "stop:1 rgba(0, 0, 0, 90));  }\n\nQLabel\n{\ncolor:rgba(0,0,0,255);\n}\n";
        constexpr const char* FormFrameSheet =
            "QFrame#frame \n{\nbackground-color: rgba(255, 255, 255,128);\nborder: 2px solid white;\nborder-radius: 10px;\n}";

        // ------------------------------------------------------------------------------
        // The sites, built by the real class
        // ------------------------------------------------------------------------------

        //! The GUI shows the shield over its parent: it is laid out, and so are the widgets in it when one of them is read. The shield
        //! is made busy (setProgress(0, 0), a bar with no range), and a busy bar's chunk moves with the clock, so that two readings of
        //! one shield would differ by the moment they were taken: the bar is given a value of its range, as LegacyShowAndWait gives it
        QWidget* makeShield(QWidget* parent)
        {
            auto* shield = new ProgressShield(parent);
            if (auto* bar = shield->findChild<QProgressBar*>(QStringLiteral("progressBar")))
            {
                bar->setRange(0, 2);
                bar->setValue(1);
            }
            shield->setAttribute(Qt::WA_DontShowOnScreen);
            shield->show();
            return shield;
        }

        QWidget* makeFrame(QWidget* parent)
        {
            return makeShield(parent)->findChild<QFrame*>(QStringLiteral("frame"));
        }

        QWidget* makeLabel(QWidget* parent)
        {
            return makeShield(parent)->findChild<QLabel*>(QStringLiteral("progressLabel"));
        }

        // ------------------------------------------------------------------------------
        // The rows
        // ------------------------------------------------------------------------------

        //! U022: the whole shield, a window over its parent with the card in the middle: the gradient is what it paints as the window
        //! (the Window brush of its palette), and the card, the label and the bar are painted over it
        Twins::Row shieldRow(const char* site)
        {
            return Twins::Row{
                .m_site = site,
                .m_make = &makeShield,
                .m_realApplicationSheet = true,
                .m_states = { Twins::States::disabled() },
                .m_compare = Twins::Compare::Geometry | Twins::Compare::Fonts | Twins::Compare::Palette | Twins::Compare::Pixels,
                .m_roles = { QPalette::WindowText, QPalette::Text, QPalette::ButtonText },
                .m_size = QSize(300, 200),
                .m_base = Twins::Base::AzQtComponentsStyle,
            };
        }

        //! U023: the card in the middle of the shield with its label and bar inside it: the box, the line and the corners
        Twins::Row frameRow(const char* site)
        {
            return Twins::Row{
                .m_site = site,
                .m_make = &makeFrame,
                .m_realApplicationSheet = true,
                .m_states = { Twins::States::disabled() },
                .m_compare = Twins::Compare::Geometry | Twins::Compare::Fonts | Twins::Compare::Palette | Twins::Compare::Pixels,
                // The fill of the frame's rule is its Base, Button and Window in the legacy GUI (QRenderRule::configurePalette)
                .m_roles = { QPalette::Base, QPalette::Button, QPalette::Window, QPalette::WindowText, QPalette::Text,
                    QPalette::ButtonText },
                .m_base = Twins::Base::AzQtComponentsStyle,
            };
        }

        //! U022: the label of the shield, which `QLabel { color }` of the shield's sheet gave black text in every state
        Twins::Row labelRow(const char* site)
        {
            return Twins::Row{
                .m_site = site,
                .m_make = &makeLabel,
                .m_realApplicationSheet = true,
                .m_states = { Twins::States::disabled() },
                .m_compare = Twins::Compare::Geometry | Twins::Compare::Fonts | Twins::Compare::Palette | Twins::Compare::Pixels,
                .m_roles = { QPalette::WindowText, QPalette::Text, QPalette::ButtonText },
                .m_base = Twins::Base::AzQtComponentsStyle,
            };
        }

        //! The brush a twin's shield paints its window with
        QBrush windowBrushOf(Twins::Context& context, bool legacy)
        {
            Twins::RungScope rung(legacy ? Twins::Rung::Qss : Twins::Rung::Flat);
            const Twins::Row row = shieldRow("window brush");
            const Twins::Twin twin(row, context, legacy);
            return twin.widget()->palette().brush(QPalette::Active, QPalette::Window);
        }

        //! The names of the facts and pictures in which two readings of a widget differ, for the report of a failure
        std::string differencesOf(const Twins::Reading& a, const Twins::Reading& b)
        {
            std::string text;
            for (const Twins::Fact& fact : a.m_facts)
            {
                for (const Twins::Fact& other : b.m_facts)
                {
                    if (other.m_name == fact.m_name && other.m_text != fact.m_text)
                    {
                        text += fact.m_name + ": " + fact.m_text + " against " + other.m_text + "; ";
                    }
                }
            }
            for (size_t i = 0; i < a.m_pictures.size() && i < b.m_pictures.size(); ++i)
            {
                if (AzQtComponents::FieldTwins::differing(a.m_pictures[i].m_image, b.m_pictures[i].m_image) != 0)
                {
                    text += a.m_pictures[i].m_name + " differs; ";
                }
            }
            return text;
        }

        //! The sheets of the widgets a shield holds, one line each, as the census lists them in a rung
        QStringList censusOfShield(AzQtComponents::GuiPerfFloor::Rung rung)
        {
            return AzQtComponents::SiteTwins::sheetLinesAt(rung,
                []
                {
                    QWidget host;
                    host.resize(300, 200);
                    new ProgressShield(&host);
                    host.setAttribute(Qt::WA_DontShowOnScreen);
                    host.show();
                    for (QWidget* each : host.findChildren<QWidget*>())
                    {
                        each->ensurePolished();
                    }
                });
        }
    } // namespace ProgressShieldFlatSheetTestsInternal

    // A unity build joins this file with others of namespace UnitTest: the fixture is the only name brought in, the rest is named
    // through the alias
    using ProgressShieldFlatSheetTestsInternal::ProgressShieldFlatSheetTests;
    namespace Shield = ProgressShieldFlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // U022: the shield
    // ----------------------------------------------------------------------------------

    TEST_F(ProgressShieldFlatSheetTests, Shield_PaintsTheGradientTheCardTheLabelAndTheBarAsTheSheetsPaintedThemAtEveryRatio)
    {
        EXPECT_TRUE(Shield::Twins::check(Shield::shieldRow("U022 the progress shield"), m_context));
    }

    TEST_F(ProgressShieldFlatSheetTests, Shield_WindowBrushIsTheVerticalGradientOfTheSheet)
    {
        const QBrush legacy = Shield::windowBrushOf(m_context, true);
        const QBrush flat = Shield::windowBrushOf(m_context, false);

        ASSERT_EQ(legacy.style(), Qt::LinearGradientPattern);
        ASSERT_EQ(flat.style(), Qt::LinearGradientPattern);
        const auto* legacyGradient = static_cast<const QLinearGradient*>(legacy.gradient());
        const auto* flatGradient = static_cast<const QLinearGradient*>(flat.gradient());
        EXPECT_EQ(flatGradient->coordinateMode(), legacyGradient->coordinateMode());
        EXPECT_EQ(flatGradient->start(), legacyGradient->start());
        EXPECT_EQ(flatGradient->finalStop(), legacyGradient->finalStop());
        ASSERT_EQ(flatGradient->stops().size(), legacyGradient->stops().size());
        for (qsizetype i = 0; i < legacyGradient->stops().size(); ++i)
        {
            EXPECT_EQ(flatGradient->stops().at(i).first, legacyGradient->stops().at(i).first) << "stop " << i;
            EXPECT_EQ(flatGradient->stops().at(i).second, legacyGradient->stops().at(i).second) << "stop " << i;
        }
        EXPECT_EQ(legacyGradient->stops().first().second, QColor(0, 0, 0, 50));
        EXPECT_EQ(legacyGradient->stops().last().second, QColor(0, 0, 0, 90));
    }

    TEST_F(ProgressShieldFlatSheetTests, Shield_WindowBrushOfEveryColourGroupIsTheGradient)
    {
        Shield::Twins::RungScope flat(Shield::Twins::Rung::Flat);
        const Shield::Twins::Row row = Shield::shieldRow("window brush in every group");
        const Shield::Twins::Twin twin(row, m_context, false);
        const QPalette palette = twin.widget()->palette();
        for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
        {
            for (const QPalette::ColorRole role : { QPalette::Base, QPalette::Button, QPalette::Window })
            {
                EXPECT_EQ(palette.brush(group, role).style(), Qt::LinearGradientPattern) << group << " " << role;
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // U023: the frame
    // ----------------------------------------------------------------------------------

    TEST_F(ProgressShieldFlatSheetTests, Frame_HasTheBoxTheLineTheCornersAndTheFillOfTheSheetAtEveryRatio)
    {
        EXPECT_TRUE(Shield::Twins::check(Shield::frameRow("U023 the frame of the progress shield"), m_context));
    }

    // ----------------------------------------------------------------------------------
    // The label
    // ----------------------------------------------------------------------------------

    TEST_F(ProgressShieldFlatSheetTests, Label_HasTheBlackTextOfTheSheetInEveryStateAndTheFontOfEveryLabel)
    {
        EXPECT_TRUE(Shield::Twins::check(Shield::labelRow("U022 the label of the progress shield"), m_context));
    }

    // ----------------------------------------------------------------------------------
    // The elements reach the shield and what is in it, and nothing else
    // ----------------------------------------------------------------------------------

    TEST_F(ProgressShieldFlatSheetTests, Elements_AreThoseOfTheShieldItsFrameAndNoOtherWidgetOfTheSameNames)
    {
        Shield::Twins::RungScope flat(Shield::Twins::Rung::Flat);

        QWidget root;
        auto* shield = new Shield::ProgressShield(&root);
        auto* frame = shield->findChild<QFrame*>(QStringLiteral("frame"));
        ASSERT_NE(frame, nullptr);

        // A frame of the same name and a widget of the shield's name, which are not the shield's
        auto* otherFrame = new QFrame(&root);
        otherFrame->setObjectName(QStringLiteral("frame"));
        auto* otherWidget = new QWidget(&root);
        otherWidget->setObjectName(QStringLiteral("progressShield"));

        const QList<QWidget*> widgets = root.findChildren<QWidget*>();
        for (QWidget* each : widgets)
        {
            each->setStyle(m_context.flatStyle());
        }
        root.setStyle(m_context.flatStyle());
        for (QWidget* each : widgets)
        {
            each->ensurePolished();
        }

        const AzQtComponents::BoxElement* shieldElement = AzQtComponents::BoxElement::forWidget(shield);
        const AzQtComponents::BoxElement* frameElement = AzQtComponents::BoxElement::forWidget(frame);
        ASSERT_NE(shieldElement, nullptr);
        ASSERT_NE(frameElement, nullptr);
        EXPECT_STREQ(shieldElement->m_name, "Editor.Box.ProgressShield");
        EXPECT_STREQ(frameElement->m_name, "Editor.Box.ProgressShieldFrame");
        EXPECT_EQ(AzQtComponents::BoxElement::forWidget(otherFrame), nullptr);
        EXPECT_EQ(AzQtComponents::BoxElement::forWidget(otherWidget), nullptr);

        // The text of the shield's label is black; the text of a label outside it keeps the application's white
        auto* label = shield->findChild<QLabel*>(QStringLiteral("progressLabel"));
        ASSERT_NE(label, nullptr);
        QLabel outside(&root);
        outside.setStyle(m_context.flatStyle());
        outside.ensurePolished();
        EXPECT_EQ(label->palette().color(QPalette::Active, QPalette::WindowText), QColor(0, 0, 0));
        EXPECT_EQ(label->palette().color(QPalette::Disabled, QPalette::WindowText), QColor(0, 0, 0));
        EXPECT_EQ(outside.palette().color(QPalette::Active, QPalette::WindowText), QColor(255, 255, 255));
    }

    // ----------------------------------------------------------------------------------
    // The call site: the legacy GUI sets the sheets the form had, the flattened GUI none
    // ----------------------------------------------------------------------------------

    TEST_F(ProgressShieldFlatSheetTests, Sheets_TheLegacyGuiSetsTheFormsSheetsAndTheFlattenedGuiNone)
    {
        const QStringList legacy = Shield::censusOfShield(Shield::Twins::Rung::Qss);
        bool foundShield = false;
        bool foundFrame = false;
        for (const QString& line : legacy)
        {
            foundShield = foundShield
                || (AzQtComponents::SiteTwins::namesCarrier(line, QStringLiteral("AzToolsFramework::ProgressShield#progressShield"))
                    && line.contains(QStringLiteral("qlineargradient")));
            foundFrame = foundFrame
                || (AzQtComponents::SiteTwins::namesCarrier(line, QStringLiteral("QFrame#frame"))
                    && line.contains(QStringLiteral("QFrame#frame")) && line.contains(QStringLiteral("border-radius: 10px")));
        }
        EXPECT_TRUE(foundShield) << legacy.join("\n").toStdString();
        EXPECT_TRUE(foundFrame) << legacy.join("\n").toStdString();

        const QStringList flat = Shield::censusOfShield(Shield::Twins::Rung::Flat);
        EXPECT_TRUE(flat.isEmpty()) << flat.join("\n").toStdString();
    }

    TEST_F(ProgressShieldFlatSheetTests, Sheets_TheLegacySheetsDecideWhatTheFormsSheetsDid)
    {
        // The legacy shield with the sheets its code sets, then the same shield with the form's own text in their place: whatever the
        // sheets decide (geometry, fonts, palette, pixels at every ratio, idle and disabled) is the same
        const Shield::Twins::Row row = Shield::shieldRow("U022 the sheets of the code against the sheets of the form");
        Shield::Twins::RungScope qss(Shield::Twins::Rung::Qss);
        const Shield::Twins::Twin twin(row, m_context, true);
        QWidget* shield = twin.widget();
        auto* frame = shield->findChild<QFrame*>(QStringLiteral("frame"));
        ASSERT_NE(frame, nullptr);

        const std::vector<Shield::Twins::Reading> code = Shield::Twins::readStates(row, *shield, m_context, true, nullptr);

        shield->setStyleSheet(QString::fromLatin1(Shield::FormShieldSheet));
        frame->setStyleSheet(QString::fromLatin1(Shield::FormFrameSheet));
        QApplication::processEvents();
        const std::vector<Shield::Twins::Reading> form = Shield::Twins::readStates(row, *shield, m_context, true, nullptr);

        ASSERT_EQ(code.size(), form.size());
        for (size_t i = 0; i < code.size(); ++i)
        {
            EXPECT_FALSE(Shield::Twins::readingsDiffer(code[i], form[i]))
                << code[i].m_state << ": " << Shield::differencesOf(code[i], form[i]);
        }
    }
} // namespace UnitTest
