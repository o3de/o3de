/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/FieldTwins.h>
#include <AzQtComponents/Tests/ToolTipTwins.h>

#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Elements/ToolTipElement.h>

#include <QFile>
#include <QFont>
#include <QLabel>
#include <QListWidget>
#include <QPointer>
#include <QRegularExpression>
#include <QVBoxLayout>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The looks a converted sheet gave a tooltip (ToolTipElement): which widgets' tips a look reaches, and that the flattened tip of
    // such a widget has its fill, its text and its place, as the tip of the sheet did
    // ----------------------------------------------------------------------------------

    namespace ToolTipElementTestsInternal
    {
        using namespace FieldTwins;

        //! The text a sheet of `font-size: 10px; color: #94D2FF; text-decoration: underline` gave (LinkWidget.cpp:56 of the Project
        //! Manager, before the flattening, is one), and the fills of three sheets of `background-color`
        const TextElement SmallLinkText{
            .m_name = "Test.Text.SmallLink",
            .m_pixelSize = 10,
            .m_underline = 1,
            .m_color = QColor(0x94, 0xD2, 0xFF),
        };
        const ToolTipElement FillAndText{
            .m_name = "Test.ToolTip.FillAndText",
            .m_fill = QColor(35, 35, 35),
            .m_text = &SmallLinkText,
        };
        const ToolTipElement FillOnly{
            .m_name = "Test.ToolTip.FillOnly",
            .m_fill = QColor(51, 51, 51),
        };

        bool isFillAndTextOwner(const QWidget* widget)
        {
            return Selectors::named(widget, "toolTipElementTestsFillAndText");
        }

        bool isFillOnlyOwner(const QWidget* widget)
        {
            return Selectors::named(widget, "toolTipElementTestsFillOnly");
        }

        //! A look that comes and goes with a state of its widget, as a link's does with the pointer (LinkLabel::HoverProperty)
        const ToolTipElement FillWhileHovered{
            .m_name = "Test.ToolTip.FillWhileHovered",
            .m_fill = QColor(12, 34, 56),
        };

        bool isHoveredOwner(const QWidget* widget)
        {
            return widget->property("toolTipElementTestsHover").toBool();
        }

        class ToolTipElementTests
            : public Fixture
        {
        public:
            ToolTipElementTests()
            {
                static bool registered = false;
                if (!registered)
                {
                    registered = true;
                    ToolTipElement::registerElement(&FillAndText, &QLabel::staticMetaObject, &isFillAndTextOwner);
                    ToolTipElement::registerElement(&FillOnly, &QWidget::staticMetaObject, &isFillOnlyOwner);
                    ToolTipElement::registerElement(&FillWhileHovered, &QLabel::staticMetaObject, &isHoveredOwner);
                }
            }

        protected:
            ToolTipTwins::Guard m_tips;
        };

        QLabel* makeOwner(const char* name, QWidget* parent)
        {
            auto* label = new QLabel(QStringLiteral("owner"), parent);
            label->setObjectName(QString::fromLatin1(name));
            label->setToolTip(QStringLiteral("Tip"));
            return label;
        }
    } // namespace ToolTipElementTestsInternal

    using ToolTipElementTestsInternal::ToolTipElementTests;
    namespace TipTests = ToolTipElementTestsInternal;

    // ----------------------------------------------------------------------------------
    // The sheet of the legacy twins
    // ----------------------------------------------------------------------------------

    TEST_F(ToolTipElementTests, AppRules_TheTwinsCopyIsTheTextOfTheApplicationsSheets)
    {
        const auto normalized = [](QString text)
        {
            text.remove(QRegularExpression(QStringLiteral(R"(/\*.*?\*/)"), QRegularExpression::DotMatchesEverythingOption));
            text.remove(QRegularExpression(QStringLiteral(R"(\s)")));
            return text;
        };
        const auto read = [&normalized](const QString& name)
        {
            QFile file(QStringLiteral(":/AzQtComponents/Widgets/") + name);
            EXPECT_TRUE(file.open(QIODevice::ReadOnly)) << name.toUtf8().constData();
            return normalized(QString::fromUtf8(file.readAll()));
        };

        // The tooltip rule is the whole of ToolTip.qss, and the universal rule is a rule of BaseStyleSheet.qss
        const QString twin = normalized(QString::fromLatin1(ToolTipTwins::AppRules));
        const qsizetype toolTipRule = twin.indexOf(QLatin1String("QToolTip"));
        ASSERT_GT(toolTipRule, 0);
        EXPECT_EQ(twin.mid(toolTipRule), read(QStringLiteral("ToolTip.qss")));
        EXPECT_TRUE(read(QStringLiteral("BaseStyleSheet.qss")).contains(twin.left(toolTipRule)));
    }

    // ----------------------------------------------------------------------------------
    // Resolution
    // ----------------------------------------------------------------------------------

    TEST_F(ToolTipElementTests, Look_OnlyTheWidgetsOfTheMatcherAndThoseBelowThemHaveIt)
    {
        QWidget root;
        auto* owner = TipTests::makeOwner("toolTipElementTestsFillAndText", &root);
        auto* other = TipTests::makeOwner("toolTipElementTestsOther", &root);
        // A widget below the owner: the sheet had no selector, so it reached the tips of the widgets inside the one it was set on
        auto* below = new QWidget(owner);

        const ToolTipElement::Look ownerLook = ToolTipElement::lookFor(owner);
        EXPECT_EQ(ownerLook.m_fill, QColor(35, 35, 35));
        EXPECT_EQ(ownerLook.m_text, &TipTests::SmallLinkText);
        EXPECT_TRUE(ToolTipElement::lookFor(below) == ownerLook);

        const ToolTipElement::Look otherLook = ToolTipElement::lookFor(other);
        EXPECT_FALSE(otherLook.m_fill.isValid());
        EXPECT_EQ(otherLook.m_text, nullptr);
        EXPECT_TRUE(ToolTipElement::lookFor(&root) == otherLook);
    }

    TEST_F(ToolTipElementTests, Look_TheNearestWidgetsFillAndTheNearestWidgetsTextWin)
    {
        // A sheet of a fill above one of a fill and a text: each property comes from the nearest widget that declares it
        QWidget root;
        auto* outer = new QWidget(&root);
        outer->setObjectName(QStringLiteral("toolTipElementTestsFillOnly"));
        auto* inner = TipTests::makeOwner("toolTipElementTestsFillAndText", outer);

        const ToolTipElement::Look look = ToolTipElement::lookFor(inner);
        EXPECT_EQ(look.m_fill, QColor(35, 35, 35));         // the inner widget's
        EXPECT_EQ(look.m_text, &TipTests::SmallLinkText);

        // Below the outer widget only: the outer widget's fill, and no text
        auto* plain = new QWidget(outer);
        const ToolTipElement::Look plainLook = ToolTipElement::lookFor(plain);
        EXPECT_EQ(plainLook.m_fill, QColor(51, 51, 51));
        EXPECT_EQ(plainLook.m_text, nullptr);
    }

    // ----------------------------------------------------------------------------------
    // The widget a tip is shown for
    // ----------------------------------------------------------------------------------

    TEST_F(ToolTipElementTests, Owner_AHelpEventForAViewportIsForTheScrollAreaThatShowsItsTip)
    {
        QListWidget list;
        list.addItem(QStringLiteral("item"));
        EXPECT_EQ(ToolTipElement::noteHelpEvent(list.viewport()), &list);

        QWidget plain;
        EXPECT_EQ(ToolTipElement::noteHelpEvent(&plain), &plain);
    }

    TEST_F(ToolTipElementTests, Owner_TheTipTakesTheWidgetTheHelpEventWasDeliveredTo)
    {
        QWidget root;
        auto* owner = TipTests::makeOwner("toolTipElementTestsFillAndText", &root);
        auto* other = TipTests::makeOwner("toolTipElementTestsOther", &root);

        // The tip is made under the application's style (Fusion) and handed the flattened one: it claims its widget when polished
        QPoint local(2, 2);
        QHelpEvent help(QEvent::ToolTip, local, owner->mapToGlobal(local));
        QApplication::sendEvent(owner, &help);
        QLabel* tip = ToolTipTwins::findTip();
        ASSERT_NE(tip, nullptr);
        EXPECT_TRUE(ToolTipElement::isToolTip(tip));
        tip->setStyle(m_style.get());
        EXPECT_EQ(ToolTipElement::ownerOf(tip), owner);
        delete tip;

        QHelpEvent otherHelp(QEvent::ToolTip, local, other->mapToGlobal(local));
        QApplication::sendEvent(other, &otherHelp);
        tip = ToolTipTwins::findTip();
        ASSERT_NE(tip, nullptr);
        tip->setStyle(m_style.get());
        EXPECT_EQ(ToolTipElement::ownerOf(tip), other);
        delete tip;
    }

    TEST_F(ToolTipElementTests, Owner_ATipKeepsTheLookItWasMadeWithWhileItsWidgetLosesIt)
    {
        QWidget root;
        auto* hovered = TipTests::makeOwner("toolTipElementTestsHoveredOwner", &root);
        auto* beside = TipTests::makeOwner("toolTipElementTestsOther", &root);
        hovered->setProperty("toolTipElementTestsHover", true);

        const QPoint local(2, 2);
        QHelpEvent help(QEvent::ToolTip, local, hovered->mapToGlobal(local));
        QApplication::sendEvent(hovered, &help);
        QLabel* tip = ToolTipTwins::findTip();
        ASSERT_NE(tip, nullptr);
        tip->setStyle(m_style.get());
        EXPECT_EQ(ToolTipElement::lookOf(tip).m_fill, QColor(12, 34, 56));

        // The pointer leaves the label, which loses its look, while the tip is still up (it hides a moment after a Leave)
        hovered->setProperty("toolTipElementTestsHover", false);
        EXPECT_FALSE(ToolTipElement::lookFor(hovered).m_fill.isValid());
        EXPECT_EQ(ToolTipElement::lookOf(tip).m_fill, QColor(12, 34, 56));

        // The pointer rests on a label of the plain look: the tip of the other look is not kept for it, though the label that
        // made it has the plain look now. A new tip is made, and it has no look
        QPointer<QLabel> first(tip);
        QHelpEvent besideHelp(QEvent::ToolTip, local, beside->mapToGlobal(local));
        QApplication::sendEvent(beside, &besideHelp);
        EXPECT_TRUE(first.isNull()) << "the tip of the old look is replaced";
        QLabel* second = ToolTipTwins::findTip();
        ASSERT_NE(second, nullptr);
        EXPECT_TRUE(second->isVisible());
        second->setStyle(m_style.get());
        EXPECT_FALSE(ToolTipElement::lookOf(second).m_fill.isValid());
        delete second;
    }

    // ----------------------------------------------------------------------------------
    // The tip: its fill, its text, its place
    // ----------------------------------------------------------------------------------

    TEST_F(ToolTipElementTests, Tip_HasTheFillTheTextAndTheFontOfTheLookOfItsWidget)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QWidget root;
        root.resize(200, 100);
        auto* owner = TipTests::makeOwner("toolTipElementTestsFillAndText", &root);
        auto* other = TipTests::makeOwner("toolTipElementTestsOther", &root);
        root.show();
        QApplication::processEvents();

        const ToolTipTwins::TipFacts facts = ToolTipTwins::showAndReadTip(owner, m_style.get());
        ASSERT_TRUE(facts.m_found);
        EXPECT_EQ(facts.m_base, QColor(35, 35, 35));
        EXPECT_EQ(facts.m_text, QColor(0x94, 0xD2, 0xFF));
        // The font is read from the QFont, which holds what the element set. QFontInfo::pixelSize reports the font the platform
        // matched (qfont.cpp: the engine's fontDef), and this suite runs on the offscreen platform with no font loaded: Qt then
        // uses its box engine, whose fontDef has no size (-1, qfontdatabase.cpp findFont), whatever the request is
        EXPECT_EQ(facts.m_font.pixelSize(), 10);
        EXPECT_TRUE(facts.m_font.underline());
        EXPECT_FALSE(facts.m_images[0].isNull());
        // The fill reaches the pixels of the tip, in its margin too (the window behind it has the same colour)
        const QSize tipSize = ToolTipTwins::TipSize;
        EXPECT_EQ(facts.m_images[0].pixelColor(0, 0), QColor(35, 35, 35));
        EXPECT_EQ(facts.m_images[0].pixelColor(tipSize.width() - 1, tipSize.height() - 1), QColor(35, 35, 35));

        // A widget the look does not reach has the tip of ToolTip.qss: black, white, 12 px
        const ToolTipTwins::TipFacts plain = ToolTipTwins::showAndReadTip(other, m_style.get());
        ASSERT_TRUE(plain.m_found);
        EXPECT_EQ(plain.m_base, QColor(Qt::black));
        EXPECT_EQ(plain.m_text, QColor(Qt::white));
        EXPECT_FALSE(plain.m_font.underline());
        EXPECT_EQ(plain.m_font.pixelSize(), 12);        // the class font of QTipLabel (TextElement ToolTipText, ToolTip.qss:16)
    }
} // namespace AzQtComponents
