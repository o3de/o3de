/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/FieldTwins.h>
#include <AzQtComponents/Tests/ToolTipTwins.h>

#include <AzQtComponents/Components/Elements/ToolTipElement.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>

#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QMargins>
#include <QPoint>
#include <QPointer>
#include <QStyle>
#include <QToolTip>
#include <QVBoxLayout>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The tooltips of a card header. Every card header had the sheet `QToolTip { padding: 5px; }` (Card.cpp:101-104 in the constructor,
    // and :467 when it is polished, the padding being Card::Config::toolTipPaddingInPixels, CardConfig.ini:2): QToolTip::showText
    // hands the sheet style the widget the tip is shown for, which reads the sheets above that widget, so the tip of the header or of
    // any widget in it had the padding inside the margin of ToolTip.qss:15-18. The flattened GUI has no sheet there: Style gives the
    // tip the padding (SE_ShapedFrameContents, Card::toolTipPadding) where the widget it is shown for is in a card header. Qt makes
    // the tip a child window of that widget off Windows only (QToolTip::showText), so the widget is the one Style's application
    // filter noted when the help event reached it (ToolTipElement).
    //
    // The legacy twin is Qt's own style sheet style over Fusion with the application's real sheet (FieldTwins::appSheet) and the header's
    // sheet. The flat twin is the flattened Style. The tip is made as Qt makes it, by the help event of a widget of each twin, in turn
    // (there is one tip at a time), and found as the palette test finds it: the QTipLabel among the top level widgets.
    // ----------------------------------------------------------------------------------

    namespace CardHeaderFlatSheetTestsInternal
    {
        using namespace FieldTwins;

        class CardHeaderFlatSheetTests
            : public Fixture
        {
        protected:
            ToolTipTwins::Guard m_tips;
        };

        //! A card header of a class of its own, as the component editor's header is (ComponentEditorHeader derives from CardHeader in
        //! AzToolsFramework): the padding is the header's, so every class below CardHeader has it
        class LooseHeader
            : public CardHeader
        {
        public:
            using CardHeader::CardHeader;
        };

        //! The tooltip text of a widget, which makes it show a tip. An ElidingLabel's tip is its tooltip too (ElidingLabel::elide sets
        //! it from the description and the text when the label is shown or resized): setDescription alone only stores the string, so
        //! a label that is not elided afterwards has no tip, and the help event falls through to the widget it is in. Nothing here
        //! resizes a label after the tip is given.
        void giveTip(QWidget* widget)
        {
            widget->setToolTip(QStringLiteral("Tip"));
        }

        //! A card with a title, a secondary title and a content label, a header of a class of its own outside the card, and a label
        //! outside. What the legacy rung sets on every card header (the constructor of the card for its own, Card::polish for any other)
        //! is set on the legacy twin's: this process runs at the flat rung, where nothing sets it.
        void buildCard(QWidget& root, bool legacy)
        {
            auto* layout = new QVBoxLayout(&root);

            auto* card = new Card(&root);
            card->setObjectName(QStringLiteral("card"));
            card->setTitle(QStringLiteral("A card"));
            card->setSecondaryTitle(QStringLiteral("Second"));
            auto* content = new QLabel(QStringLiteral("content"), card);
            content->setObjectName(QStringLiteral("content"));
            card->setContentWidget(content);
            layout->addWidget(card);

            auto* loose = new LooseHeader(&root);
            loose->setObjectName(QStringLiteral("loose"));
            loose->setTitle(QStringLiteral("Loose"));
            layout->addWidget(loose);

            auto* outside = new QLabel(QStringLiteral("outside"), &root);
            outside->setObjectName(QStringLiteral("outside"));
            layout->addWidget(outside);

            if (legacy)
            {
                // Card.cpp:101-104 and :467 (the Config's value, which the constructor takes from the default)
                for (CardHeader* header : root.findChildren<CardHeader*>())
                {
                    header->setStyleSheet(QStringLiteral("QToolTip { padding: %1px; }").arg(Card::defaultConfig().toolTipPaddingInPixels));
                }
            }
        }

        //! The widgets a tip is shown for: the headers (the card's, its secondary one, and the one outside any card), the title in the
        //! first, and two outside the headers
        QWidget* widgetFor(QWidget& root, const QString& which)
        {
            auto* card = root.findChild<Card*>(QStringLiteral("card"));
            if (!card)
            {
                return nullptr;
            }
            QWidget* widget = nullptr;
            if (which == QLatin1String("header"))
            {
                widget = card->header();
            }
            else if (which == QLatin1String("title"))
            {
                widget = card->header()->findChild<ElidingLabel*>(QStringLiteral("Title"));
            }
            else if (which == QLatin1String("secondary"))
            {
                for (CardHeader* header : card->findChildren<CardHeader*>())
                {
                    if (header != card->header())
                    {
                        widget = header;
                    }
                }
            }
            else if (which == QLatin1String("loose"))
            {
                widget = root.findChild<CardHeader*>(which);
            }
            else
            {
                widget = root.findChild<QLabel*>(which);
            }
            if (widget)
            {
                giveTip(widget);
            }
            return widget;
        }

        /*
         * The ink: the bounds of the pixels that are not `background`, which is where the text of the tip sits
         */
        QRect inkBounds(const QImage& image, const QColor& background)
        {
            int left = image.width();
            int top = image.height();
            int right = -1;
            int bottom = -1;
            for (int y = 0; y < image.height(); ++y)
            {
                for (int x = 0; x < image.width(); ++x)
                {
                    if (image.pixelColor(x, y) != background)
                    {
                        left = qMin(left, x);
                        top = qMin(top, y);
                        right = qMax(right, x);
                        bottom = qMax(bottom, y);
                    }
                }
            }
            return right < 0 ? QRect() : QRect(left, top, right - left + 1, bottom - top + 1);
        }

        const char* const Widgets[] = { "header", "title", "secondary", "loose", "content", "outside" };

        // ----------------------------------------------------------------------------------
        // T-CH.1: a tip is the one of the legacy GUI, for the widgets of the header and for the others
        // ----------------------------------------------------------------------------------

        TEST_F(CardHeaderFlatSheetTests, ToolTip_HasTheContentsSizeAndTextPlaceOfTheLegacyTipForEveryWidget)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildCard, QSize(300, 240));

            for (const char* which : Widgets)
            {
                SCOPED_TRACE(which);
                QWidget* legacyWidget = widgetFor(twins.m_legacy, QString::fromLatin1(which));
                QWidget* flatWidget = widgetFor(twins.m_flat, QString::fromLatin1(which));
                ASSERT_TRUE(legacyWidget != nullptr && flatWidget != nullptr);

                // The font is the legacy tip's: what a font is made of is not what a carrier proves
                const ToolTipTwins::TipFacts legacy = ToolTipTwins::showAndReadTip(legacyWidget, nullptr);
                const ToolTipTwins::TipFacts flat = ToolTipTwins::showAndReadTip(flatWidget, m_style.get(), &legacy.m_font);
                ASSERT_TRUE(legacy.m_found && flat.m_found);

                EXPECT_EQ(ToolTipTwins::marginsText(flat.m_contents), ToolTipTwins::marginsText(legacy.m_contents));
                EXPECT_EQ(flat.m_margin, legacy.m_margin);
                EXPECT_EQ(sizeText(flat.m_hint), sizeText(legacy.m_hint));
                EXPECT_EQ(rectText(flat.m_contentsRect), rectText(legacy.m_contentsRect));
                EXPECT_EQ(legacy.m_base.name(), flat.m_base.name());
                EXPECT_EQ(legacy.m_text.name(), flat.m_text.name());

                const QRect legacyInk = inkBounds(legacy.m_images[0], Qt::black);
                EXPECT_FALSE(legacyInk.isNull()) << "the text of the tip was drawn";
                EXPECT_EQ(rectText(inkBounds(flat.m_images[0], Qt::black)), rectText(legacyInk));

                // The tip's pixels, at the ratios the GUI runs at: the fill, the margin and the padding, and the text in them
                for (int i = 0; i < 3; ++i)
                {
                    EXPECT_EQ(firstDifference(legacy.m_images[i], flat.m_images[i]), "equal") << "at ratio " << ToolTipTwins::Ratios[i];
                }
            }
        }

        TEST_F(CardHeaderFlatSheetTests, ToolTip_OfAHeaderWidgetHasTheConfigsPaddingMoreThanOneOutsideTheHeaderOnEverySide)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildCard, QSize(300, 240));

            const int padding = Card::defaultConfig().toolTipPaddingInPixels;       // CardConfig.ini:2, the default of Card.cpp:377
            ASSERT_GT(padding, 0);

            const ToolTipTwins::TipFacts outside =
                ToolTipTwins::showAndReadTip(widgetFor(twins.m_flat, QStringLiteral("outside")), m_style.get());
            ASSERT_TRUE(outside.m_found);
            // ToolTip.qss:15, :17-18
            EXPECT_EQ(ToolTipTwins::marginsText(outside.m_contents), ToolTipTwins::marginsText(QMargins(2, 4, 0, 4)));

            for (const char* which : { "header", "title", "secondary", "loose" })
            {
                SCOPED_TRACE(which);
                const ToolTipTwins::TipFacts facts =
                    ToolTipTwins::showAndReadTip(widgetFor(twins.m_flat, QString::fromLatin1(which)), m_style.get());
                ASSERT_TRUE(facts.m_found);
                EXPECT_EQ(
                    ToolTipTwins::marginsText(facts.m_contents),
                    ToolTipTwins::marginsText(outside.m_contents + QMargins(padding, padding, padding, padding)));
            }

            // A widget of the card, but not of its header, is no more padded than a widget outside the card
            const ToolTipTwins::TipFacts content =
                ToolTipTwins::showAndReadTip(widgetFor(twins.m_flat, QStringLiteral("content")), m_style.get());
            ASSERT_TRUE(content.m_found);
            EXPECT_EQ(ToolTipTwins::marginsText(content.m_contents), ToolTipTwins::marginsText(outside.m_contents));
        }

        // ----------------------------------------------------------------------------------
        // The widget a tip is shown for
        // ----------------------------------------------------------------------------------

        TEST_F(CardHeaderFlatSheetTests, ToolTip_ATipVisibleForAWidgetOfAnotherLookIsMadeNewForTheNextWidget)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildCard, QSize(300, 240));
            const int padding = Card::defaultConfig().toolTipPaddingInPixels;

            // The tip for the header is on screen, with the header's padding (made as the application makes it: below the flattened
            // Style, which the test hands it, since the application of the fixture is Fusion)
            QWidget* header = widgetFor(twins.m_flat, QStringLiteral("header"));
            QWidget* outside = widgetFor(twins.m_flat, QStringLiteral("outside"));
            const QPoint local(2, 2);
            QHelpEvent headerHelp(QEvent::ToolTip, local, header->mapToGlobal(local));
            QApplication::sendEvent(header, &headerHelp);
            QLabel* first = ToolTipTwins::findTip();
            ASSERT_NE(first, nullptr);
            first->setStyle(m_style.get());
            ASSERT_EQ(ToolTipElement::ownerOf(first), header);
            first->setMargin(1 + m_style->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, nullptr, first));
            first->resize(ToolTipTwins::TipSize);
            EXPECT_EQ(ToolTipTwins::marginsText(first->contentsMargins()),
                ToolTipTwins::marginsText(QMargins(2 + padding, 4 + padding, padding, 4 + padding)));

            // The pointer rests on a label outside the card while the tip is still up: Qt would reuse the tip and keep the header's
            // padding. The filter hides it, so that the tip the label shows is new
            QHelpEvent outsideHelp(QEvent::ToolTip, local, outside->mapToGlobal(local));
            QPointer<QLabel> firstTip(first);
            QApplication::sendEvent(outside, &outsideHelp);
            // Qt's new tip deletes the hidden one (QTipLabel::QTipLabel), so the old tip is gone and the visible one is the new one
            EXPECT_TRUE(firstTip.isNull()) << "the tip of the header is not the tip of the label outside";
            QLabel* second = ToolTipTwins::findTip();
            ASSERT_NE(second, nullptr);
            EXPECT_TRUE(second->isVisible());
            second->setStyle(m_style.get());
            EXPECT_EQ(ToolTipElement::ownerOf(second), outside);
            EXPECT_EQ(ToolTipTwins::marginsText(second->contentsMargins()), ToolTipTwins::marginsText(QMargins(2, 4, 0, 4)));
            delete second;
        }

        TEST_F(CardHeaderFlatSheetTests, ToolTip_ATipVisibleForAWidgetOfTheSameLookIsKept)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildCard, QSize(300, 240));
            QWidget* header = widgetFor(twins.m_flat, QStringLiteral("header"));
            QWidget* title = widgetFor(twins.m_flat, QStringLiteral("title"));
            const QPoint local(2, 2);

            QHelpEvent headerHelp(QEvent::ToolTip, local, header->mapToGlobal(local));
            QApplication::sendEvent(header, &headerHelp);
            QLabel* first = ToolTipTwins::findTip();
            ASSERT_NE(first, nullptr);
            first->setStyle(m_style.get());
            QPointer<QLabel> firstTip(first);

            // The title is in the header: the same padding, so Qt may reuse the tip as it does. The pointer is checked as a guarded
            // one: a tip made new would be at the address of the deleted one
            QHelpEvent titleHelp(QEvent::ToolTip, local, title->mapToGlobal(local));
            QApplication::sendEvent(title, &titleHelp);
            ASSERT_FALSE(firstTip.isNull());
            EXPECT_EQ(ToolTipTwins::findTip(), firstTip.data());
            delete first;
        }

        TEST_F(CardHeaderFlatSheetTests, ToolTip_ATipShownByCodeWithNoHelpEventIsKeptForAWidgetOfThePlainLook)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildCard, QSize(300, 240));
            QWidget* outside = widgetFor(twins.m_flat, QStringLiteral("outside"));
            QWidget* content = widgetFor(twins.m_flat, QStringLiteral("content"));

            // A tip a widget shows when the pointer moves over it (a color grid's value, a slider's): no help event made it, so it has
            // no owner on Windows, where Qt gives a tip no parent. The note of a help event is stale once the event loop is reached
            QApplication::processEvents();
            QToolTip::showText(outside->mapToGlobal(QPoint(2, 2)), QStringLiteral("hover value"), outside);
            QLabel* shown = ToolTipTwins::findTip();
            ASSERT_NE(shown, nullptr);
            shown->setStyle(m_style.get());
            QPointer<QLabel> shownTip(shown);

            // The pointer rests: QApplication sends the help event to the widget under it, whether a tip is up or not. Its look is the
            // plain one, as the tip's: the tip stays, as it does in the legacy GUI
            const QPoint local(2, 2);
            QHelpEvent help(QEvent::ToolTip, local, content->mapToGlobal(local));
            QApplication::sendEvent(content, &help);
            ASSERT_FALSE(shownTip.isNull()) << "the tip of code that answered no help event was hidden";
            EXPECT_EQ(ToolTipTwins::findTip(), shownTip.data());
            EXPECT_TRUE(shownTip->isVisible());
            delete shown;
        }

        // ----------------------------------------------------------------------------------
        // The tips of a header whose content is modified. Card.qss:172-175 `CardHeader[contentModified=true] * { color: #E57829 }`
        // reaches a tooltip through the widget it is shown for (QStyleSheetStyle::styleRules, `_q_stylesheet_parent`): the text of the
        // tip of the header and of every widget in it is orange, over QToolTip's white (ToolTip.qss:12), at a higher specificity. The
        // rule `CardHeader * { background-color: #555555 }` (Card.qss:86-89) reaches the tip too, at the specificity of QToolTip's
        // rule and before it in the file order of the sheet, so the tip stays black: that is the same for a header that names none of
        // its parts (EMotionFX's RagdollCardHeader). The widgets of a card outside its header are not reached.
        // ----------------------------------------------------------------------------------

        const char* const ModifiedRule =
            "AzQtComponents--CardHeader[contentModified=true] *\n"
            "{\n"
            "    color: #E57829;\n"
            "}\n";

        //! The header of EMotionFX's RagdollCardHeader: the background frame has no name (RagdollNodeWidget.cpp:37)
        class UnnamedBackgroundHeader
            : public CardHeader
        {
        public:
            UnnamedBackgroundHeader()
            {
                m_backgroundFrame->setObjectName(QString());
            }
        };

        //! A card with a header of its own (Card's constructor for one is protected)
        class CustomHeaderCard
            : public Card
        {
        public:
            CustomHeaderCard(CardHeader* header, QWidget* parent)
                : Card(header, parent)
            {
            }
        };

        //! A card whose header is modified, one whose header is not, a ragdoll card whose header names no frame, a modified header of a
        //! class of its own outside any card, and a label outside. The legacy twin has the sheet of every card header (Card.cpp:101-104).
        void buildModifiedCards(QWidget& root, bool legacy)
        {
            auto* layout = new QVBoxLayout(&root);

            for (const char* name : { "modified", "plain", "ragdoll" })
            {
                Card* card = qstrcmp(name, "ragdoll") == 0 ? new CustomHeaderCard(new UnnamedBackgroundHeader(), &root) : new Card(&root);
                card->setObjectName(QString::fromLatin1(name));
                card->setTitle(QStringLiteral("A card"));
                auto* content = new QLabel(QStringLiteral("content"), card);
                content->setObjectName(QStringLiteral("content"));
                card->setContentWidget(content);
                card->header()->setContentModified(qstrcmp(name, "modified") == 0);
                layout->addWidget(card);
            }

            auto* loose = new LooseHeader(&root);
            loose->setObjectName(QStringLiteral("loose"));
            loose->setTitle(QStringLiteral("Loose"));
            loose->setContentModified(true);
            layout->addWidget(loose);

            auto* outside = new QLabel(QStringLiteral("outside"), &root);
            outside->setObjectName(QStringLiteral("outside"));
            layout->addWidget(outside);

            if (legacy)
            {
                for (CardHeader* header : root.findChildren<CardHeader*>())
                {
                    header->setStyleSheet(QStringLiteral("QToolTip { padding: %1px; }").arg(Card::defaultConfig().toolTipPaddingInPixels));
                }
            }
        }

        //! The widgets a tip is shown for, each given its tooltip text
        QWidget* modifiedTipWidget(QWidget& root, const QString& which)
        {
            const auto header = [&root](const char* card) { return root.findChild<Card*>(QString::fromLatin1(card))->header(); };
            QWidget* widget = nullptr;
            if (which == QLatin1String("modifiedHeader"))
            {
                widget = header("modified");
            }
            else if (which == QLatin1String("modifiedTitle"))
            {
                widget = header("modified")->findChild<ElidingLabel*>(QStringLiteral("Title"));
            }
            else if (which == QLatin1String("modifiedContent"))
            {
                widget = root.findChild<Card*>(QStringLiteral("modified"))->findChild<QLabel*>(QStringLiteral("content"));
            }
            else if (which == QLatin1String("plainHeader"))
            {
                widget = header("plain");
            }
            else if (which == QLatin1String("plainTitle"))
            {
                widget = header("plain")->findChild<ElidingLabel*>(QStringLiteral("Title"));
            }
            else if (which == QLatin1String("ragdollHeader"))
            {
                widget = header("ragdoll");
            }
            else if (which == QLatin1String("ragdollFrame"))
            {
                widget = header("ragdoll")->findChild<QFrame*>(QString(), Qt::FindDirectChildrenOnly);
            }
            else if (which == QLatin1String("loose"))
            {
                widget = root.findChild<CardHeader*>(which);
            }
            else
            {
                widget = root.findChild<QLabel*>(which);
            }
            if (widget)
            {
                giveTip(widget);
            }
            return widget;
        }

        //! Whether the tip of the widget is orange: the widgets in a modified header
        bool isOrange(const QString& which)
        {
            const bool inModifiedCard = which.startsWith(QLatin1String("modified")) && which != QLatin1String("modifiedContent");
            return inModifiedCard || which == QLatin1String("loose");
        }

        const char* const ModifiedWidgets[] = { "modifiedHeader", "modifiedTitle", "modifiedContent", "plainHeader", "plainTitle",
            "ragdollHeader", "ragdollFrame", "loose", "outside" };

        TEST_F(CardHeaderFlatSheetTests, ToolTipText_Sheet_HoldsTheRuleTheseTestsQuote)
        {
            EXPECT_TRUE(appSheet().contains(QString::fromLatin1(ModifiedRule)));
        }

        TEST_F(CardHeaderFlatSheetTests, ToolTipText_OfAModifiedHeaderIsTheOrangeOfTheLegacyTipInEveryPixel)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildModifiedCards, QSize(300, 400));

            for (const char* which : ModifiedWidgets)
            {
                SCOPED_TRACE(which);
                QWidget* legacyWidget = modifiedTipWidget(twins.m_legacy, QString::fromLatin1(which));
                QWidget* flatWidget = modifiedTipWidget(twins.m_flat, QString::fromLatin1(which));
                ASSERT_TRUE(legacyWidget != nullptr && flatWidget != nullptr);

                const ToolTipTwins::TipFacts legacy = ToolTipTwins::showAndReadTip(legacyWidget, nullptr);
                const ToolTipTwins::TipFacts flat = ToolTipTwins::showAndReadTip(flatWidget, m_style.get(), &legacy.m_font);
                ASSERT_TRUE(legacy.m_found && flat.m_found);

                EXPECT_EQ(legacy.m_text.name(), QString::fromLatin1(isOrange(QString::fromLatin1(which)) ? "#e57829" : "#ffffff"));
                EXPECT_EQ(flat.m_text.name(), legacy.m_text.name());
                // black: ToolTip.qss:13, which `CardHeader *` (Card.qss:86-89) does not outrank for a tip
                EXPECT_EQ(legacy.m_base.name(), QStringLiteral("#000000"));
                EXPECT_EQ(flat.m_base.name(), legacy.m_base.name());
                EXPECT_EQ(ToolTipTwins::marginsText(flat.m_contents), ToolTipTwins::marginsText(legacy.m_contents));
                EXPECT_EQ(sizeText(flat.m_hint), sizeText(legacy.m_hint));
                EXPECT_EQ(rectText(inkBounds(flat.m_images[0], Qt::black)), rectText(inkBounds(legacy.m_images[0], Qt::black)));
                for (int i = 0; i < 3; ++i)
                {
                    EXPECT_EQ(firstDifference(legacy.m_images[i], flat.m_images[i]), "equal") << "at ratio " << ToolTipTwins::Ratios[i];
                }
            }
        }

        TEST_F(CardHeaderFlatSheetTests, ToolTipText_FollowsTheStateOfTheHeaderWhenTheTipIsShown)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildModifiedCards, QSize(300, 400));

            // The rule reads the property when the tip is made: a header that becomes modified shows orange tips from then on, and
            // white ones again once it is not
            for (const bool modified : { true, false })
            {
                SCOPED_TRACE(modified ? "modified" : "not modified");
                twins.m_legacy.findChild<Card*>(QStringLiteral("plain"))->header()->setContentModified(modified);
                twins.m_flat.findChild<Card*>(QStringLiteral("plain"))->header()->setContentModified(modified);

                for (const char* which : { "plainHeader", "plainTitle" })
                {
                    SCOPED_TRACE(which);
                    QWidget* legacyWidget = modifiedTipWidget(twins.m_legacy, QString::fromLatin1(which));
                    QWidget* flatWidget = modifiedTipWidget(twins.m_flat, QString::fromLatin1(which));
                    ASSERT_TRUE(legacyWidget != nullptr && flatWidget != nullptr);

                    const ToolTipTwins::TipFacts legacy = ToolTipTwins::showAndReadTip(legacyWidget, nullptr);
                    const ToolTipTwins::TipFacts flat = ToolTipTwins::showAndReadTip(flatWidget, m_style.get(), &legacy.m_font);
                    ASSERT_TRUE(legacy.m_found && flat.m_found);

                    EXPECT_EQ(legacy.m_text.name(), QString::fromLatin1(modified ? "#e57829" : "#ffffff"));
                    EXPECT_EQ(flat.m_text.name(), legacy.m_text.name());
                    for (int i = 0; i < 3; ++i)
                    {
                        EXPECT_EQ(firstDifference(legacy.m_images[i], flat.m_images[i]), "equal") << "at ratio " << ToolTipTwins::Ratios[i];
                    }
                }
            }
        }
    } // namespace CardHeaderFlatSheetTestsInternal
} // namespace AzQtComponents
