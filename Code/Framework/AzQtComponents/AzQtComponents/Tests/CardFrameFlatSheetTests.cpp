/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/FieldTwins.h>

#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Titlebar.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>

#include <QEvent>
#include <QFrame>
#include <QHash>
#include <QIcon>
#include <QLabel>
#include <QMargins>
#include <QPaintEvent>
#include <QPointer>
#include <QPoint>
#include <QRect>
#include <QRegion>
#include <QStyle>
#include <QStyleFactory>
#include <QVBoxLayout>
#include <QtMath>

#include <functional>
#include <string>
#include <vector>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The frame of a card (Card.qss): the shadow it casts and the frame that hides it (:10-19, :46-49), the fill of the parts of its
    // header (:86-89), the header of a card that is selected and collapsed (:109-113), and what repaints a card when it is selected.
    //
    // The legacy twin is Qt's own style sheet style over AzQtComponents::Style over Fusion, with the application's real sheet
    // (FieldTwins::appSheet: BaseStyleSheet.qss and the files it imports, whose Card.qss these tests quote) on the root: the cards
    // are built, shown, changed and painted under the Qss rung, where Card::paintEvent and CardHeader::paintEvent leave the frame to
    // the style, as the legacy GUI does. The flat twin is the flattened Style, with the Global elements, under the Flat rung. The
    // cards are the real classes. Same boxes, same pixels, at the device pixel ratios the GUI runs at.
    // ----------------------------------------------------------------------------------

    namespace CardFrameFlatSheetTestsInternal
    {
        using namespace FieldTwins;

        class CardFrameFlatSheetTests
            : public Fixture
        {
        };

        // ----------------------------------------------------------------------------------
        // The rules, as the sheet has them, checked against the real sheet in T-CF.0
        // ----------------------------------------------------------------------------------

        const char* const CardRule =
            "AzQtComponents--Card\n"
            "{\n"
            "    margin: 0px;\n"
            "    margin-bottom: 4px;\n"
            "    padding: -4px -5px -4px -5px;\n"
            "\n"
            "    border-color: transparent;\n"
            "    border-width: 5px 7px 9px 7px;\n"
            "    border-image: url(:/Cards/img/UI20/Cards/shadow.png) 5 7 9 7;\n"
            "}\n";
        const char* const HideFrameRule =
            "AzQtComponents--Card[hideFrame=true]\n"
            "{\n"
            "    border: 1px transparent rgba(0, 0, 0, 0);\n"
            "}\n";
        const char* const HeaderPartsRule =
            "AzQtComponents--CardHeader *\n"
            "{\n"
            "    background-color: #555555;\n"
            "}\n";
        const char* const CollapsedSelectedRule =
            "AzQtComponents--Card[selected=\"true\"][expanded=\"false\"] > AzQtComponents--CardHeader #Background\n"
            "{\n"
            "    border: 1px solid #94D2FF;\n"
            "    border-radius: 2px\n"
            "}\n";
        const char* const WarningRule =
            "AzQtComponents--CardHeader[warning=true] #Background\n"
            "{\n"
            "    border-top-left-radius: 2px;\n"
            "    border-top-right-radius: 2px;\n"
            "\n"
            "    background-origin: border;\n"
            "    background-clip: border;\n"
            "    background-image: url(:/Cards/img/UI20/Cards/error-conclict-state.svg);\n"
            "}\n";

        constexpr QRgb HeaderPartsColor = 0xFF555555;       // Card.qss:88
        constexpr QRgb HeaderFillColor = 0xFF333333;        // Card.qss:155
        constexpr QRgb SelectedBorderColor = 0xFF94D2FF;    // Card.qss:111
        constexpr QRgb Untouched = 0xFF808080;              // the grey the images start from (paint)
        constexpr int BottomMargin = 4;                     // Card.qss:13

        const qreal Ratios[] = { 1.0, 1.5, 2.0 };

        // ----------------------------------------------------------------------------------
        // The twins
        // ----------------------------------------------------------------------------------

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

        constexpr GuiPerfFloor::Rung LegacyRung = GuiPerfFloor::Rung::Qss;
        constexpr GuiPerfFloor::Rung FlatRung = GuiPerfFloor::Rung::Flat;

        //! One tree twice. The legacy root has the AzQtComponents::Style as its base style, below the sheet style the application's
        //! sheet makes (as the application's style is below its sheet style), and is built under the Qss rung.
        struct CardTwins
        {
            template<typename Build>
            CardTwins(const Fixture& fixture, Build build, const QSize& size)
                : m_legacyBase(AZStd::make_unique<Style>(QStyleFactory::create(QStringLiteral("Fusion"))))
            {
                {
                    RungScope rung(LegacyRung);
                    m_legacy.setStyle(m_legacyBase.get());
                    build(m_legacy, true);
                    Fixture::legacy(m_legacy);
                    Fixture::show(m_legacy, size);
                }
                {
                    RungScope rung(FlatRung);
                    build(m_flat, false);
                    fixture.flat(m_flat);
                    Fixture::show(m_flat, size);
                }
            }

            template<typename W = Card>
            W* legacyChild(const char* name)
            {
                return m_legacy.findChild<W*>(QString::fromLatin1(name));
            }

            template<typename W = Card>
            W* flatChild(const char* name)
            {
                return m_flat.findChild<W*>(QString::fromLatin1(name));
            }

            // Declared in this order: the widgets go before the style they use
            AZStd::unique_ptr<Style> m_legacyBase;
            QWidget m_legacy;
            QWidget m_flat;
        };

        //! Lets the events a change posted run, and the events those post
        void settle()
        {
            for (int i = 0; i < 5; ++i)
            {
                QApplication::processEvents();
            }
        }

        // ----------------------------------------------------------------------------------
        // The trees
        // ----------------------------------------------------------------------------------

        //! The layout of a root, the same in both twins (the base styles of the twins give another)
        QVBoxLayout* rootLayout(QWidget& root)
        {
            auto* layout = new QVBoxLayout(&root);
            layout->setContentsMargins(8, 8, 8, 8);
            layout->setSpacing(6);
            return layout;
        }

        Card* makeCard(QWidget& root, const char* name)
        {
            auto* card = new Card(&root);
            card->setObjectName(QString::fromLatin1(name));
            card->setTitle(QStringLiteral("A card"));
            card->setContentWidget(new QLabel(QStringLiteral("content"), card));
            return card;
        }

        //! A card with the frame and one without (Card::hideFrame, called when the card is made as its users call it), each expanded
        //! and collapsed
        void buildFrames(QWidget& root, bool legacy)
        {
            Q_UNUSED(legacy);
            QVBoxLayout* layout = rootLayout(root);
            for (const char* name : { "plain", "hidden", "plainCollapsed", "hiddenCollapsed" })
            {
                Card* card = makeCard(root, name);
                if (QString::fromLatin1(name).startsWith(QStringLiteral("hidden")))
                {
                    card->hideFrame();
                }
                if (QString::fromLatin1(name).endsWith(QStringLiteral("Collapsed")))
                {
                    card->setExpanded(false);
                }
                layout->addWidget(card);
            }
            layout->addStretch();
        }

        //! The header of EMotionFX's RagdollCardHeader: its background frame has no name (RagdollNodeWidget.cpp:37), so #Background
        //! does not reach it
        class UnnamedBackgroundHeader
            : public CardHeader
        {
        public:
            UnnamedBackgroundHeader()
            {
                m_backgroundFrame->setObjectName(QString());
            }
        };

        //! EMotionFX's RagdollCard: a card with a header of its own that hides its frame when it is made (RagdollNodeWidget.cpp:40-44).
        //! Card's constructor for a custom header is protected.
        class CustomHeaderCard
            : public Card
        {
        public:
            CustomHeaderCard(CardHeader* header, QWidget* parent)
                : Card(header, parent)
            {
                hideFrame();
            }
        };

        //! The card of EMotionFX's ragdoll, an ordinary card with its frame hidden, as the asset browser's inspector makes its own, and
        //! a card whose header shows every named part: the icon, the help button and the context menu button
        void buildHeaders(QWidget& root, bool legacy)
        {
            Q_UNUSED(legacy);
            QVBoxLayout* layout = rootLayout(root);
            for (const char* name : { "ragdoll", "ordinary", "decorated" })
            {
                Card* card = qstrcmp(name, "ragdoll") == 0 ? new CustomHeaderCard(new UnnamedBackgroundHeader(), &root) : new Card(&root);
                card->setObjectName(QString::fromLatin1(name));
                card->setTitle(QStringLiteral("Ragdoll properties"));
                card->setContentWidget(new QLabel(QStringLiteral("content"), card));
                card->header()->setHasContextMenu(qstrcmp(name, "decorated") == 0);
                if (qstrcmp(name, "ordinary") == 0)
                {
                    card->hideFrame();
                }
                if (qstrcmp(name, "decorated") == 0)
                {
                    card->header()->setIcon(QIcon(QStringLiteral(":/Cards/img/UI20/Cards/menu_ico.svg")));
                    card->header()->setHelpURL(QStringLiteral("https://o3de.org/docs/"));
                }
                layout->addWidget(card);
            }
            layout->addStretch();
        }

        //! Cards in every combination of the two states, set before the cards are shown (before the frames of their headers are first
        //! polished), and one that is expanded and unselected when it is shown and changes later
        void buildStates(QWidget& root, bool legacy)
        {
            Q_UNUSED(legacy);
            QVBoxLayout* layout = rootLayout(root);
            for (const char* name : { "plain", "selectedExpanded", "selectedCollapsed", "collapsed", "later" })
            {
                Card* card = makeCard(root, name);
                if (qstrcmp(name, "selectedExpanded") == 0 || qstrcmp(name, "selectedCollapsed") == 0)
                {
                    card->setSelected(true);
                }
                if (qstrcmp(name, "selectedCollapsed") == 0 || qstrcmp(name, "collapsed") == 0)
                {
                    card->setExpanded(false);
                }
                layout->addWidget(card);
            }
            layout->addStretch();
        }

        //! The card of the states above that changes after it is shown, alone
        void buildLater(QWidget& root, bool legacy)
        {
            Q_UNUSED(legacy);
            QVBoxLayout* layout = rootLayout(root);
            layout->addWidget(makeCard(root, "later"));
            layout->addStretch();
        }

        // ----------------------------------------------------------------------------------
        // Reading a twin
        // ----------------------------------------------------------------------------------

        std::string marginsText(const QMargins& margins)
        {
            return QStringLiteral("(%1, %2, %3, %4)")
                .arg(margins.left()).arg(margins.top()).arg(margins.right()).arg(margins.bottom()).toStdString();
        }

        QFrame* backgroundOf(const Card* card)
        {
            return card->header()->findChild<QFrame*>(QString(), Qt::FindDirectChildrenOnly);
        }

        QFrame* bodyOf(const Card* card)
        {
            return card->findChild<QFrame*>(QStringLiteral("contentContainer"));
        }

        //! The boxes of a card: its own frame box (QFrame::setFrameRect: SE_ShapedFrameContents), where it sits, where its header and
        //! body sit in it, the frame box of the header's background, and the size it asks for
        struct CardFacts
        {
            QRect m_card;
            QMargins m_frame;
            QRect m_header;
            QRect m_body;
            QMargins m_background;
            QSize m_hint;
        };

        CardFacts factsOf(const Card* card)
        {
            return { card->geometry(), card->contentsMargins(), card->header()->geometry(), bodyOf(card)->geometry(),
                backgroundOf(card)->contentsMargins(), card->sizeHint() };
        }

        void expectSameFacts(const CardFacts& legacy, const CardFacts& flat)
        {
            EXPECT_EQ(rectText(flat.m_card), rectText(legacy.m_card));
            EXPECT_EQ(marginsText(flat.m_frame), marginsText(legacy.m_frame));
            EXPECT_EQ(rectText(flat.m_header), rectText(legacy.m_header));
            EXPECT_EQ(rectText(flat.m_body), rectText(legacy.m_body));
            EXPECT_EQ(marginsText(flat.m_background), marginsText(legacy.m_background));
            EXPECT_EQ(sizeText(flat.m_hint), sizeText(legacy.m_hint));
        }

        //! What a card's signal does when its property changes (StyleHelpers::repolishWhenPropertyChanges, StyleHelpers.h:60-72): the
        //! widget and each widget below it are unpolished and polished again by the style sheet style, which is the style
        //! StyleManager::styleSheetStyle answers in the legacy GUI, and by the Style itself in the flattened one (it has no sheet
        //! style). A test cannot run the helper itself: it asks for the StyleManager, which has no instance here.
        void repolishAsTheHelperDoes(QWidget* widget, QStyle* style)
        {
            style->unpolish(widget);
            style->polish(widget);
            for (QWidget* child : widget->findChildren<QWidget*>())
            {
                style->unpolish(child);
                style->polish(child);
            }
        }

        //! A card changed as its users change it, and repolished as its signal repolishes it
        void change(Card* card, bool legacy, QStyle* flatStyle, void (*apply)(Card*))
        {
            RungScope rung(legacy ? LegacyRung : FlatRung);
            apply(card);
            repolishAsTheHelperDoes(card, legacy ? card->style() : flatStyle);
            settle();
        }

        //! The same change to a card in each twin
        void changeBoth(Card* legacy, Card* flat, QStyle* flatStyle, void (*apply)(Card*))
        {
            change(legacy, true, flatStyle, apply);
            change(flat, false, flatStyle, apply);
        }

        void select(Card* card)
        {
            card->setSelected(true);
        }

        void selectAndCollapse(Card* card)
        {
            card->setSelected(true);
            card->setExpanded(false);
        }

        void collapse(Card* card)
        {
            card->setExpanded(false);
        }

        //! The widget painted onto an image of an explicit ratio, over a grey that stays where nothing was drawn. The flags say what is
        //! drawn: the widget alone (its background and its paint event), or the widgets inside it too.
        QImage paint(QWidget* widget, qreal ratio, QWidget::RenderFlags flags, bool legacy)
        {
            RungScope rung(legacy ? LegacyRung : FlatRung);
            QImage image((QSizeF(widget->size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor(Untouched));
            widget->render(&image, QPoint(), QRegion(), flags);
            return image;
        }

        //! The pixel under a logical point
        QRgb pixelAt(const QImage& image, const QPoint& point)
        {
            const qreal ratio = image.devicePixelRatio();
            return image.pixel(qRound(point.x() * ratio), qRound(point.y() * ratio));
        }

        //! The pixels of two images that differ inside a rect of logical points (whose corners are on device pixels at every ratio)
        int differingInside(const QImage& a, const QImage& b, const QRect& logical)
        {
            const qreal ratio = a.devicePixelRatio();
            const QRect device(QPoint(qRound(logical.left() * ratio), qRound(logical.top() * ratio)),
                QPoint(qRound((logical.right() + 1) * ratio) - 1, qRound((logical.bottom() + 1) * ratio) - 1));
            int count = 0;
            for (int y = device.top(); y <= device.bottom(); ++y)
            {
                for (int x = device.left(); x <= device.right(); ++x)
                {
                    count += (a.pixel(x, y) != b.pixel(x, y)) ? 1 : 0;
                }
            }
            return count;
        }

        //! The pixels of two images that differ in the device pixels a rect of logical points touches: those it covers in part too, which
        //! an edge that falls between device pixels (an odd logical edge at a ratio of 1.5) leaves out of differingInside
        int differingCovering(const QImage& a, const QImage& b, const QRect& logical)
        {
            const qreal ratio = a.devicePixelRatio();
            const QRect device(QPoint(qFloor(logical.left() * ratio), qFloor(logical.top() * ratio)),
                QPoint(qCeil((logical.right() + 1) * ratio) - 1, qCeil((logical.bottom() + 1) * ratio) - 1));
            int count = 0;
            for (int y = device.top(); y <= device.bottom(); ++y)
            {
                for (int x = device.left(); x <= device.right(); ++x)
                {
                    count += (a.pixel(x, y) != b.pixel(x, y)) ? 1 : 0;
                }
            }
            return count;
        }

        //! How two images of one size differ, for the failure message of a comparison: the pixels, the largest difference of a channel
        //! and the bounds of the pixels
        std::string differenceText(const QImage& a, const QImage& b)
        {
            if (a.size() != b.size())
            {
                return firstDifference(a, b);
            }
            int count = 0;
            int largest = 0;
            QRect bounds;
            for (int y = 0; y < a.height(); ++y)
            {
                for (int x = 0; x < a.width(); ++x)
                {
                    const QRgb one = a.pixel(x, y);
                    const QRgb other = b.pixel(x, y);
                    if (one != other)
                    {
                        ++count;
                        largest = qMax(largest, qMax(qMax(qAbs(qRed(one) - qRed(other)), qAbs(qGreen(one) - qGreen(other))),
                            qAbs(qBlue(one) - qBlue(other))));
                        bounds = bounds.isNull() ? QRect(x, y, 1, 1) : bounds.united(QRect(x, y, 1, 1));
                    }
                }
            }
            return QStringLiteral("%1 px differ, up to %2 levels, within %3, first %4").arg(count).arg(largest)
                .arg(QString::fromStdString(rectText(bounds)), QString::fromStdString(firstDifference(a, b))).toStdString();
        }

        //! The two widgets painted alike at every ratio. The legacy one must draw something: two images nothing was drawn on are equal.
        void expectSamePixels(QWidget* legacy, QWidget* flat, QWidget::RenderFlags flags)
        {
            for (const qreal ratio : Ratios)
            {
                const QImage expected = paint(legacy, ratio, flags, true);
                const QImage actual = paint(flat, ratio, flags, false);
                EXPECT_TRUE(isDrawnOn(expected)) << "ratio " << ratio;
                EXPECT_EQ(differing(actual, expected), 0) << "ratio " << ratio << ": " << firstDifference(actual, expected);
            }
        }

        // ----------------------------------------------------------------------------------
        // T-CF.0: the rules these tests quote are the sheet's
        // ----------------------------------------------------------------------------------

        TEST_F(CardFrameFlatSheetTests, Sheet_HoldsTheRulesTheseTestsQuote)
        {
            const QString sheet = appSheet();
            for (const char* rule : { CardRule, HideFrameRule, HeaderPartsRule, CollapsedSelectedRule, WarningRule })
            {
                EXPECT_TRUE(sheet.contains(QString::fromLatin1(rule))) << rule;
            }
        }

        // ----------------------------------------------------------------------------------
        // T-CF.1: Card.qss:46-49, a card with its frame hidden. `border: 1px transparent rgba(0, 0, 0, 0)` makes the border 1px on each
        // side and leaves the border image of :18, which the four 1px edges show squashed: the shadow is a line of its own along the
        // edges, not the soft shadow of the card, and not nothing.
        // ----------------------------------------------------------------------------------

        TEST_F(CardFrameFlatSheetTests, HideFrame_BoxesAreThoseOfTheLegacyCard)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            CardTwins twins(*this, buildFrames, QSize(300, 420));

            for (const char* name : { "plain", "hidden", "plainCollapsed", "hiddenCollapsed" })
            {
                SCOPED_TRACE(name);
                const Card* legacy = twins.legacyChild(name);
                const Card* flat = twins.flatChild(name);
                ASSERT_TRUE(legacy != nullptr && flat != nullptr);
                expectSameFacts(factsOf(legacy), factsOf(flat));
            }

            // The numbers of the rules: the padding of :14 less the borders of :17 (the frame) and of :48 (the hidden frame)
            EXPECT_EQ(marginsText(twins.legacyChild("plain")->contentsMargins()), marginsText(QMargins(2, 1, 2, 9)));
            EXPECT_EQ(marginsText(twins.legacyChild("hidden")->contentsMargins()), marginsText(QMargins(-4, -3, -4, 1)));
            EXPECT_EQ(marginsText(twins.flatChild("hidden")->contentsMargins()), marginsText(QMargins(-4, -3, -4, 1)));
        }

        TEST_F(CardFrameFlatSheetTests, HideFrame_ShadowIsTheBorderImageSquashedIntoTheOnePixelBorders)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            CardTwins twins(*this, buildFrames, QSize(300, 420));

            // A collapsed card paints its shadow and nothing else (its body is hidden), so its own paint is the shadow
            Card* legacy = twins.legacyChild("hiddenCollapsed");
            Card* flat = twins.flatChild("hiddenCollapsed");
            ASSERT_TRUE(legacy != nullptr && flat != nullptr);
            expectSamePixels(legacy, flat, QWidget::RenderFlags());

            // and the squashed shadow is there: a line along the bottom edge of the box left by the 4px margin
            const QImage expected = paint(legacy, 1.0, QWidget::RenderFlags(), true);
            const int bottomEdge = legacy->height() - BottomMargin - 1;
            EXPECT_NE(pixelAt(expected, QPoint(legacy->width() / 2, bottomEdge)), Untouched);
            EXPECT_EQ(pixelAt(expected, QPoint(legacy->width() / 2, bottomEdge + 1)), Untouched);
        }

        TEST_F(CardFrameFlatSheetTests, HideFrame_BodyLiesOverTheLowerRowsOfTheHeaderAsItDidUnderTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            CardTwins twins(*this, buildFrames, QSize(300, 420));

            // The padding of -3px at the top (Card.qss:14, :48) puts the header out of its box, over the first rows of the body, which
            // the sheet's container (a child painted after the header) covered
            Card* legacy = twins.legacyChild("hidden");
            Card* flat = twins.flatChild("hidden");
            ASSERT_TRUE(legacy != nullptr && flat != nullptr);
            const QRect header = legacy->header()->geometry();
            const QRect body = bodyOf(legacy)->geometry();
            ASSERT_LT(body.top(), header.bottom() + 1);

            // The whole of the strip, from the first row of the body to the edges of the container, in the device pixels it touches
            // too: the top of the body is on an odd row, which at a ratio of 1.5 falls between two device rows, where the way the
            // body is filled (a rounded rect and a rect, against the one path of the sheet) could show
            ASSERT_EQ(body.top() % 2, 1);
            const QRect overlap(body.left(), body.top(), body.width(), header.bottom() + 1 - body.top());
            ASSERT_FALSE(overlap.isEmpty());

            for (const qreal ratio : Ratios)
            {
                SCOPED_TRACE(ratio);
                const QImage expected = paint(legacy, ratio, QWidget::DrawChildren, true);
                const QImage actual = paint(flat, ratio, QWidget::DrawChildren, false);
                EXPECT_EQ(pixelAt(expected, overlap.center()), HeaderPartsColor);
                EXPECT_EQ(differingCovering(actual, expected, overlap), 0);
            }

            // and a card whose header and body do not overlap paints the body once, as it did
            Card* plain = twins.flatChild("plain");
            ASSERT_TRUE(plain != nullptr);
            EXPECT_GE(bodyOf(plain)->geometry().top(), plain->header()->geometry().bottom() + 1);
        }

        // ----------------------------------------------------------------------------------
        // T-CF.2: the border image is drawn smoothed (QRenderRule::drawBorderImage sets SmoothPixmapTransform): its centre is
        // stretched at ratio 1, and all of it is scaled at a higher ratio
        // ----------------------------------------------------------------------------------

        TEST_F(CardFrameFlatSheetTests, Shadow_IsSmoothedAsTheLegacyBorderImageIsAtEveryRatio)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            CardTwins twins(*this, buildFrames, QSize(300, 420));

            Card* legacy = twins.legacyChild("plainCollapsed");
            Card* flat = twins.flatChild("plainCollapsed");
            ASSERT_TRUE(legacy != nullptr && flat != nullptr);
            expectSamePixels(legacy, flat, QWidget::RenderFlags());
        }

        // ----------------------------------------------------------------------------------
        // T-CF.3: Card.qss:86-89, `CardHeader * { background-color: #555555 }`. Each named part of a header has a rule that outranks
        // it, so the headers keep their look; the background frame of EMotionFX's ragdoll header has no name, and is filled #555555
        // over the header's own fill.
        // ----------------------------------------------------------------------------------

        TEST_F(CardFrameFlatSheetTests, HeaderParts_FrameOfAHeaderThatNamesNoneIsFilledByTheRule)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            CardTwins twins(*this, buildHeaders, QSize(300, 160));

            // Every part of a header that has a name keeps its own fill: the rule does not reach it
            for (const char* name : { "ragdoll", "ordinary", "decorated" })
            {
                SCOPED_TRACE(name);
                const Card* legacy = twins.legacyChild(name);
                const Card* flat = twins.flatChild(name);
                ASSERT_TRUE(legacy != nullptr && flat != nullptr);
                expectSameFacts(factsOf(legacy), factsOf(flat));
                EXPECT_EQ(rectText(legacy->header()->findChild<QFrame*>()->geometry()),
                    rectText(flat->header()->findChild<QFrame*>()->geometry()));
                expectSamePixels(legacy->header(), flat->header(), QWidget::DrawChildren);
            }

            // The fill: the frame of the ragdoll header covers the whole header in #555555, the corner the header rounds too (pixel
            // (0, 0)), where an ordinary header shows its own #333333 beside its title
            for (const bool legacyTwin : { true, false })
            {
                SCOPED_TRACE(legacyTwin ? "legacy" : "flat");
                Card* ragdoll = legacyTwin ? twins.legacyChild("ragdoll") : twins.flatChild("ragdoll");
                Card* ordinary = legacyTwin ? twins.legacyChild("ordinary") : twins.flatChild("ordinary");
                ASSERT_TRUE(ragdoll != nullptr && ordinary != nullptr);
                const QImage ragdollHeader = paint(ragdoll->header(), 1.0, QWidget::DrawChildren, legacyTwin);
                const QImage ordinaryHeader = paint(ordinary->header(), 1.0, QWidget::DrawChildren, legacyTwin);
                const QPoint right(ragdoll->header()->width() - 20, 4);
                EXPECT_EQ(pixelAt(ragdollHeader, QPoint(0, 0)), HeaderPartsColor);
                EXPECT_EQ(pixelAt(ragdollHeader, right), HeaderPartsColor);
                EXPECT_EQ(pixelAt(ordinaryHeader, right), HeaderFillColor);
            }
        }

        // ----------------------------------------------------------------------------------
        // T-CF.4: Card.qss:109-113, the header of a card that is selected and collapsed: `border: 1px solid #94D2FF` follows
        // `border-bottom: none` (:96, :104) at a higher specificity, so the bottom edge is 1px too, and `border-radius: 2px` rounds
        // all four corners.
        // ----------------------------------------------------------------------------------

        TEST_F(CardFrameFlatSheetTests, SelectedCollapsed_BackgroundHasTheBottomEdgeWhenTheStateIsSetBeforeThePolish)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            CardTwins twins(*this, buildStates, QSize(300, 420));

            for (const char* name : { "plain", "selectedExpanded", "selectedCollapsed", "collapsed" })
            {
                SCOPED_TRACE(name);
                const Card* legacy = twins.legacyChild(name);
                const Card* flat = twins.flatChild(name);
                ASSERT_TRUE(legacy != nullptr && flat != nullptr);
                expectSameFacts(factsOf(legacy), factsOf(flat));
            }

            // The numbers of the rules: padding 2px 7px 3px 7px (:94) and the border of :95-96, bottom edge 0; with :111 it is 1px
            const Card* expanded = twins.legacyChild("selectedExpanded");
            EXPECT_EQ(marginsText(backgroundOf(expanded)->contentsMargins()), marginsText(QMargins(8, 3, 8, 3)));
            for (const bool legacyTwin : { true, false })
            {
                SCOPED_TRACE(legacyTwin ? "legacy" : "flat");
                const Card* card = legacyTwin ? twins.legacyChild("selectedCollapsed") : twins.flatChild("selectedCollapsed");
                EXPECT_EQ(marginsText(backgroundOf(card)->contentsMargins()), marginsText(QMargins(8, 3, 8, 4)));
                EXPECT_EQ(card->header()->height(), backgroundOf(card)->height());
                EXPECT_EQ(card->header()->height(), twins.legacyChild("collapsed")->header()->height() + 1);
            }
        }

        TEST_F(CardFrameFlatSheetTests, SelectedCollapsed_BackgroundKeepsItsBoxWhenTheStateChangesAfterThePolish)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            CardTwins twins(*this, buildLater, QSize(300, 120));

            // The card is selected and collapsed after it was shown, and its repolish (StyleHelpers.h:60-72) sends the background frame
            // neither a Polish nor a StyleChange event, on which a QFrame reads its box (QFramePrivate::updateFrameWidth)
            change(twins.legacyChild("later"), true, m_style.get(), &selectAndCollapse);
            change(twins.flatChild("later"), false, m_style.get(), &selectAndCollapse);

            const Card* legacy = twins.legacyChild("later");
            const Card* flat = twins.flatChild("later");
            expectSameFacts(factsOf(legacy), factsOf(flat));
            EXPECT_EQ(marginsText(backgroundOf(legacy)->contentsMargins()), marginsText(QMargins(8, 3, 8, 3)));
            EXPECT_EQ(marginsText(backgroundOf(flat)->contentsMargins()), marginsText(QMargins(8, 3, 8, 3)));
        }

        TEST_F(CardFrameFlatSheetTests, SelectedCollapsed_HeaderIsPaintedWithItsFourEdgesAndCornersAtEveryRatio)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            CardTwins twins(*this, buildStates, QSize(300, 420));
            change(twins.legacyChild("later"), true, m_style.get(), &selectAndCollapse);
            change(twins.flatChild("later"), false, m_style.get(), &selectAndCollapse);

            // set before the polish (the box has its bottom edge), and set after it (the box is the old one, the edges are drawn)
            for (const char* name : { "selectedCollapsed", "later" })
            {
                SCOPED_TRACE(name);
                Card* legacy = twins.legacyChild(name);
                Card* flat = twins.flatChild(name);
                ASSERT_TRUE(legacy != nullptr && flat != nullptr);
                ASSERT_TRUE(legacy->isSelected() && !legacy->isExpanded());
                expectSamePixels(legacy->header(), flat->header(), QWidget::DrawChildren);

                // the edges, bottom one too, in the colour of :111
                const QImage expected = paint(legacy->header(), 1.0, QWidget::DrawChildren, true);
                const QPoint middle(legacy->header()->width() / 2, 0);
                EXPECT_EQ(pixelAt(expected, middle), SelectedBorderColor);
                EXPECT_EQ(pixelAt(expected, QPoint(middle.x(), legacy->header()->height() - 1)), SelectedBorderColor);
                EXPECT_EQ(pixelAt(expected, QPoint(0, legacy->header()->height() / 2)), SelectedBorderColor);
            }
        }

        // The warning tile of a header (Card.qss:177-185) is the rule of the background frame (#Background): error-conclict-state.svg laid
        // from the top left of the frame's border box, repeating in both directions (`background-origin` and `background-clip` are the
        // border, :182-183). It is the file at its natural size (QStyleSheetStyle::loadPixmap: a ratio of 1 for a file with no @2x
        // variant), smoothed where a ratio above 1 scales it (QWidgetPrivate::drawWidget paints the background of a widget with
        // SmoothPixmapTransform set), and clipped to the border of the frame, whose corners are those of the rules that reach it: round at
        // the top (:179-180), and round at the bottom too once the card is selected and collapsed (:109-113 `border-radius: 2px`, above
        // :177-185 by specificity). The clip is the path through the middle of the 1px border (none at the bottom of an expanded
        // header, :96), and it is not antialiased (QRenderRule::borderClip), so the tile leaves the top row to the fill of the header,
        // where the border is transparent.
        // The outline of a selected header (Card.qss:101-107, :109-113) is the border of that frame, drawn over the tile as qDrawBorder
        // draws it (QRenderRule::drawBorder).

        //! A card of the states above, with a header of the given class and warning, set before the cards are shown
        void buildHeaderKinds(QWidget& root, const char* kind, bool warning)
        {
            QVBoxLayout* layout = rootLayout(root);
            for (const char* name : { "plain", "selectedExpanded", "selectedCollapsed", "collapsed", "later" })
            {
                Card* card = makeCard(root, name);
                if (qstrcmp(kind, "container") == 0)
                {
                    Card::applyContainerStyle(card);
                }
                else if (qstrcmp(kind, "section") == 0)
                {
                    Card::applySectionStyle(card);
                }
                card->header()->setWarning(warning);
                if (qstrcmp(name, "selectedExpanded") == 0 || qstrcmp(name, "selectedCollapsed") == 0)
                {
                    card->setSelected(true);
                }
                if (qstrcmp(name, "selectedCollapsed") == 0 || qstrcmp(name, "collapsed") == 0)
                {
                    card->setExpanded(false);
                }
                layout->addWidget(card);
            }
            layout->addStretch();
        }

        //! The headers of a card in every state, as built and then as the user changes the last card after it is shown (selected, then
        //! collapsed), painted alike by both GUIs at every ratio
        void expectHeadersPaintedAlike(const Fixture& fixture, QStyle* flatStyle, const char* kind, bool warning)
        {
            // A root of 301 gives a container header (the card less the frame it hangs out of, Card.qss:115-119) an even width: the fill
            // of a square header (ContainerCardHeader, Card.qss:127-132) is a fillRect in the legacy GUI (QRenderRule::drawBackground),
            // which ends on a whole device pixel at the ratio of 1.5 where the header's antialiased fill covers the last pixel in half.
            // The same holds for the last row of the header of a card that is selected and collapsed, 25px high: that fill is not the
            // rule of the background frame (:101-113, :177-185) these tests are about.
            CardTwins twins(fixture, [&](QWidget& root, bool) { buildHeaderKinds(root, kind, warning); }, QSize(301, 420));
            const auto expectAll = [&](const char* when)
            {
                for (const char* name : { "plain", "selectedExpanded", "selectedCollapsed", "collapsed", "later" })
                {
                    if (qstrcmp(kind, "container") == 0 && qstrcmp(name, "selectedCollapsed") == 0)
                    {
                        continue;
                    }
                    SCOPED_TRACE(std::string(kind) + (warning ? " with a warning, " : ", ") + name + ", " + when);
                    Card* legacy = twins.legacyChild(name);
                    Card* flat = twins.flatChild(name);
                    ASSERT_TRUE(legacy != nullptr && flat != nullptr);
                    ASSERT_EQ(legacy->header()->isWarning(), warning);
                    expectSamePixels(legacy->header(), flat->header(), QWidget::DrawChildren);
                }
            };
            expectAll("as built");
            changeBoth(twins.legacyChild("later"), twins.flatChild("later"), flatStyle, &select);
            expectAll("selected after the show");
            changeBoth(twins.legacyChild("later"), twins.flatChild("later"), flatStyle, &collapse);
            expectAll("collapsed after the show");
        }

        TEST_F(CardFrameFlatSheetTests, Warning_TileIsThePixelsOfTheLegacyTileInEveryStateAtEveryRatio)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            expectHeadersPaintedAlike(*this, m_style.get(), "plain", true);

            // and the tile is drawn: the legacy header of a card with a warning is not the one without
            CardTwins withWarning(*this, [](QWidget& root, bool) { buildHeaderKinds(root, "plain", true); }, QSize(300, 420));
            CardTwins withoutWarning(*this, [](QWidget& root, bool) { buildHeaderKinds(root, "plain", false); }, QSize(300, 420));
            for (const char* name : { "plain", "selectedExpanded", "selectedCollapsed", "collapsed" })
            {
                SCOPED_TRACE(name);
                const QImage warned = paint(withWarning.legacyChild(name)->header(), 1.0, QWidget::DrawChildren, true);
                const QImage clear = paint(withoutWarning.legacyChild(name)->header(), 1.0, QWidget::DrawChildren, true);
                EXPECT_GT(differing(warned, clear), 1000);
            }
        }

        TEST_F(CardFrameFlatSheetTests, Warning_TileOfAContainerAndASectionHeaderHasTheRoundTopCornersOfTheBackgroundFrame)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            expectHeadersPaintedAlike(*this, m_style.get(), "container", true);
            expectHeadersPaintedAlike(*this, m_style.get(), "section", true);
        }

        // The outline of a selected header that is expanded: the border of the background frame, 1px at the top and at the sides and none
        // at the bottom, with a radius of 2 at the two top corners (Card.qss:101-107), alone and over the tile
        TEST_F(CardFrameFlatSheetTests, SelectedExpanded_OutlineIsTheBorderOfTheLegacyRuleAtEveryRatio)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            for (const char* kind : { "plain", "container", "section" })
            {
                for (const bool warning : { false, true })
                {
                    expectHeadersPaintedAlike(*this, m_style.get(), kind, warning);
                }
            }

            // the corners of the outline are drawn: the header of a selected card is not that of an unselected one at its corners
            CardTwins twins(*this, [](QWidget& root, bool) { buildHeaderKinds(root, "plain", false); }, QSize(300, 420));
            const QImage selected = paint(twins.legacyChild("selectedExpanded")->header(), 1.0, QWidget::DrawChildren, true);
            const QImage unselected = paint(twins.legacyChild("plain")->header(), 1.0, QWidget::DrawChildren, true);
            EXPECT_NE(selected.pixel(0, 0), unselected.pixel(0, 0));
            EXPECT_NE(selected.pixel(selected.width() - 1, 0), unselected.pixel(unselected.width() - 1, 0));
            EXPECT_EQ(selected.pixel(selected.width() / 2, 0), SelectedBorderColor);
            EXPECT_EQ(selected.pixel(0, selected.height() / 2), SelectedBorderColor);
            // no edge at the bottom (:104)
            EXPECT_NE(selected.pixel(selected.width() / 2, selected.height() - 1), SelectedBorderColor);
        }

        // The body of a card is the rule of its container (Card.qss:21-44), which the sheet style paints after the header, a widget below
        // it in the card: #555555 with its 1px border of the same colour (#94D2FF for a selected card, no edge at the top) and a radius of
        // 2 at the bottom corners, the fill of the path through the middle of the border and the border over it, both antialiased
        // (QRenderRule::drawRule). The header ends on a half device pixel at a ratio of 1.5, and the pixel the two share is the header
        // over the body.
        TEST_F(CardFrameFlatSheetTests, Body_IsThePixelsOfTheLegacyContainerRuleAtEveryRatio)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            const struct { const char* m_kind; const char* m_state; } cards[] = {
                { "plain", "plain" }, { "plain", "selectedExpanded" }, { "plain", "later" },
                { "container", "plain" }, { "container", "selectedExpanded" },
                { "section", "plain" }, { "section", "selectedExpanded" },
            };
            for (const auto& card : cards)
            {
                CardTwins twins(*this, [&](QWidget& root, bool) { buildHeaderKinds(root, card.m_kind, false); }, QSize(300, 420));
                for (const bool afterTheShow : { false, true })
                {
                    if (afterTheShow)
                    {
                        if (qstrcmp(card.m_state, "later") != 0)
                        {
                            break;
                        }
                        changeBoth(twins.legacyChild("later"), twins.flatChild("later"), m_style.get(), &select);
                    }
                    SCOPED_TRACE(std::string(card.m_kind) + " " + card.m_state + (afterTheShow ? " selected after the show" : ""));
                    Card* legacy = twins.legacyChild(card.m_state);
                    Card* flat = twins.flatChild(card.m_state);
                    ASSERT_TRUE(legacy != nullptr && flat != nullptr);

                    // the card with everything in it, and the container with its own paint: the body is the container's
                    expectSamePixels(legacy, flat, QWidget::DrawChildren);
                    for (const qreal ratio : Ratios)
                    {
                        const QImage expected = paint(bodyOf(legacy), ratio, QWidget::RenderFlags(), true);
                        const QImage actual = paint(bodyOf(flat), ratio, QWidget::RenderFlags(), false);
                        EXPECT_EQ(differing(actual, expected), 0)
                            << "the container alone, ratio " << ratio << ": " << differenceText(actual, expected);
                        if (qstrcmp(card.m_kind, "plain") == 0)
                        {
                            const QPoint center = QPoint(bodyOf(legacy)->width() / 2, bodyOf(legacy)->height() / 2);
                            EXPECT_EQ(pixelAt(expected, center), HeaderPartsColor);       // the body is there: Card.qss:23
                        }
                    }
                }
            }
        }

        // The top row of the body of a plain card, where the header above it ends on a half device pixel at a ratio of 1.5
        // (Card.qss:21-33: no border at the top, so the fill of the container starts on the first row of its rect)
        TEST_F(CardFrameFlatSheetTests, Body_TopRowAtARatioOfOneAndAHalfIsTheBlendOfTheHeaderOverTheBody)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            CardTwins twins(*this, [](QWidget& root, bool) { buildHeaderKinds(root, "plain", false); }, QSize(300, 420));
            Card* legacy = twins.legacyChild("plain");
            Card* flat = twins.flatChild("plain");
            ASSERT_TRUE(legacy != nullptr && flat != nullptr);
            const int bodyTop = bodyOf(legacy)->geometry().top();
            ASSERT_EQ(bodyTop % 2, 1);      // an odd logical row is a half device row at 1.5
            const QImage expected = paint(legacy, 1.5, QWidget::DrawChildren, true);
            const QImage actual = paint(flat, 1.5, QWidget::DrawChildren, false);
            const int row = qFloor(bodyTop * 1.5);
            for (const int column : { 60, 150, 300 })
            {
                EXPECT_EQ(actual.pixel(column, row), expected.pixel(column, row)) << "column " << column << ", row " << row;
            }
        }

        // ----------------------------------------------------------------------------------
        // T-CF.5: what repaints a card that is selected. The legacy GUI repolished the card and everything below it when the
        // selection changed (StyleHelpers.h:60-72), which set each widget's palette again and updated it: the whole card was painted
        // again, outline included. The flattened GUI repolishes the same widgets, but through a Style that paints again only the
        // region inside the outline, which leaves the outline as it was.
        // ----------------------------------------------------------------------------------

        //! The paint events of some widgets, and the region each was asked to paint
        class PaintRegions
            : public QObject
        {
        public:
            bool eventFilter(QObject* watched, QEvent* event) override
            {
                if (event->type() == QEvent::Paint)
                {
                    m_regions[watched] += static_cast<QPaintEvent*>(event)->region();
                }
                return false;
            }

            void watch(QWidget* widget)
            {
                widget->installEventFilter(this);
            }

            //! The widget was painted, in every point of it
            bool paintedWhole(const QWidget* widget) const
            {
                const auto found = m_regions.constFind(widget);
                return found != m_regions.constEnd() && QRegion(widget->rect()).subtracted(found.value()).isEmpty();
            }

            bool painted(const QWidget* widget) const
            {
                return m_regions.contains(widget);
            }

            void clear()
            {
                m_regions.clear();
            }

        private:
            QHash<const QObject*, QRegion> m_regions;
        };

        //! The card, its header, the frame in the header, the body, and the widget in the body: what a selection changes the look of,
        //! and what lies in the card
        struct CardParts
        {
            explicit CardParts(Card* card)
                : m_widgets{ card, card->header(), backgroundOf(card), bodyOf(card), card->contentWidget() }
            {
            }

            static constexpr const char* Names[5] = { "card", "header", "background", "body", "content" };
            QWidget* m_widgets[5];
        };

        //! Watches the parts of the card and lets the paints of the show pass
        void watchSettled(PaintRegions& paints, const CardParts& parts)
        {
            for (QWidget* widget : parts.m_widgets)
            {
                paints.watch(widget);
            }
            settle();
            paints.clear();
        }

        TEST_F(CardFrameFlatSheetTests, Selection_PaintsTheWholeCardAgainAsTheLegacyRepolishDoes)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            CardTwins twins(*this, buildStates, QSize(300, 420));

            for (const bool legacyTwin : { true, false })
            {
                SCOPED_TRACE(legacyTwin ? "legacy" : "flat");
                Card* card = legacyTwin ? twins.legacyChild("later") : twins.flatChild("later");
                ASSERT_TRUE(card != nullptr);
                const CardParts parts(card);
                PaintRegions paints;
                watchSettled(paints, parts);

                change(card, legacyTwin, m_style.get(), &select);

                // The outline of the selection is at the edge of the card and of its header
                for (int i = 0; i < 5; ++i)
                {
                    EXPECT_TRUE(paints.paintedWhole(parts.m_widgets[i])) << CardParts::Names[i];
                }
            }
        }

        TEST_F(CardFrameFlatSheetTests, Selection_OfACardThatIsSelectedAlreadyPaintsNothing)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            CardTwins twins(*this, buildStates, QSize(300, 420));
            Card* card = twins.flatChild("selectedExpanded");
            ASSERT_TRUE(card != nullptr && card->isSelected());
            const CardParts parts(card);
            PaintRegions paints;
            watchSettled(paints, parts);

            card->setSelected(true);
            settle();

            for (int i = 0; i < 5; ++i)
            {
                EXPECT_FALSE(paints.painted(parts.m_widgets[i])) << CardParts::Names[i];
            }
        }

        TEST_F(CardFrameFlatSheetTests, Selection_TheLegacyRungAsksForNoPaintOfItsOwn)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            CardTwins twins(*this, buildStates, QSize(300, 420));
            Card* card = twins.flatChild("later");
            ASSERT_TRUE(card != nullptr);
            const CardParts parts(card);
            PaintRegions paints;
            watchSettled(paints, parts);

            {
                // The sheet repolishes it, which is what paints it in the legacy GUI (the twin above); the card asks for nothing
                RungScope rung(LegacyRung);
                card->setSelected(true);
                settle();
            }

            for (int i = 0; i < 5; ++i)
            {
                EXPECT_FALSE(paints.painted(parts.m_widgets[i])) << CardParts::Names[i];
            }
        }

        TEST_F(CardFrameFlatSheetTests, Collapse_OfASelectedCardPaintsItsHeaderAgainAsTheLegacyRepolishDoes)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            CardTwins twins(*this, buildStates, QSize(300, 420));

            // The outline of a selected header is open at the bottom while the card is expanded, and closed once it is collapsed
            for (const bool legacyTwin : { true, false })
            {
                SCOPED_TRACE(legacyTwin ? "legacy" : "flat");
                Card* card = legacyTwin ? twins.legacyChild("selectedExpanded") : twins.flatChild("selectedExpanded");
                ASSERT_TRUE(card != nullptr);
                const CardParts parts(card);
                PaintRegions paints;
                watchSettled(paints, parts);

                change(card, legacyTwin, m_style.get(), &collapse);

                EXPECT_TRUE(paints.paintedWhole(parts.m_widgets[1])) << "header";
                EXPECT_TRUE(paints.paintedWhole(parts.m_widgets[2])) << "background";
            }

            expectSamePixels(twins.legacyChild("selectedExpanded")->header(), twins.flatChild("selectedExpanded")->header(),
                QWidget::DrawChildren);
        }

        // ----------------------------------------------------------------------------------
        // T-CF.6: what a property change paints again, for every widget that repolishes itself when the property changes
        // (StyleHelpers::repolishWhenPropertyChanges: the cards, the card headers and the title bars). The legacy repolish unpolished and
        // polished the widget and each widget below it, which set the palette of each of them again, and a widget whose palette changes
        // is updated (QWidget::setPalette): the widget and everything in it were painted whole. The flattened widgets draw some looks
        // from the property in their own paint code, which nothing painted again.
        // ----------------------------------------------------------------------------------

        //! A card and a title bar, the widgets that repolish themselves
        void buildRepolishUsers(QWidget& root, bool legacy)
        {
            Q_UNUSED(legacy);
            QVBoxLayout* layout = rootLayout(root);
            layout->addWidget(makeCard(root, "card"));
            auto* bar = new TitleBar(&root);
            bar->setObjectName(QStringLiteral("bar"));
            layout->addWidget(bar);
            layout->addStretch();
        }

        //! The widgets of a tree that are shown, which a paint can reach, as pointers that tell when one is gone (a title bar makes its
        //! buttons again when its mode changes)
        QList<QPointer<QWidget>> shownIn(QWidget* root)
        {
            QList<QPointer<QWidget>> shown;
            QList<QWidget*> all = root->findChildren<QWidget*>();
            all.prepend(root);
            for (QWidget* widget : all)
            {
                if (widget->isVisible())
                {
                    shown.append(widget);
                }
            }
            return shown;
        }

        //! Which of the shown widgets of a tree were painted whole after a change that repolishes it, in the order of the tree. The
        //! legacy twin is repolished as the helper does it (repolishAsTheHelperDoes: the helper itself returns before its repolish where
        //! there is no StyleManager, as in a test), and the flat twin runs the connection of the helper to the change, which paints.
        std::vector<bool> paintedWholeAfter(QWidget* root, bool legacy, QStyle* flatStyle, const std::function<void()>& change)
        {
            const QList<QPointer<QWidget>> shown = shownIn(root);
            PaintRegions paints;
            for (const QPointer<QWidget>& widget : shown)
            {
                paints.watch(widget);
            }
            settle();
            paints.clear();

            {
                RungScope rung(legacy ? LegacyRung : FlatRung);
                change();
                repolishAsTheHelperDoes(root, legacy ? root->style() : flatStyle);
            }
            settle();

            std::vector<bool> painted;
            for (const QPointer<QWidget>& widget : shown)
            {
                painted.push_back(widget && paints.paintedWhole(widget));
            }
            return painted;
        }

        //! The change in both twins of the tree above paints the same widgets whole, and the legacy twin paints at least the widget
        void expectSameRepaint(const Fixture& fixture, QStyle* flatStyle, const std::function<QWidget*(QWidget&)>& pick,
            const std::function<void(QWidget*)>& change)
        {
            CardTwins twins(fixture, buildRepolishUsers, QSize(300, 420));
            QWidget* legacy = pick(twins.m_legacy);
            QWidget* flat = pick(twins.m_flat);
            ASSERT_TRUE(legacy != nullptr && flat != nullptr);

            const std::vector<bool> expected = paintedWholeAfter(legacy, true, flatStyle, [&]() { change(legacy); });
            const std::vector<bool> actual = paintedWholeAfter(flat, false, flatStyle, [&]() { change(flat); });
            ASSERT_FALSE(expected.empty());
            EXPECT_TRUE(expected.front()) << "the legacy repolish paints the widget whole";
            EXPECT_EQ(actual, expected);
        }

        QWidget* cardOf(QWidget& root)
        {
            return root.findChild<Card*>(QStringLiteral("card"));
        }

        QWidget* headerOf(QWidget& root)
        {
            return root.findChild<Card*>(QStringLiteral("card"))->header();
        }

        QWidget* barOf(QWidget& root)
        {
            return root.findChild<TitleBar*>(QStringLiteral("bar"));
        }

        TEST_F(CardFrameFlatSheetTests, Repolish_SelectedOfACardPaintsTheCardWholeAsTheLegacyRepolishDoes)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            expectSameRepaint(*this, m_style.get(), cardOf, [](QWidget* card) { static_cast<Card*>(card)->setSelected(true); });
        }

        TEST_F(CardFrameFlatSheetTests, Repolish_ExpandedOfACardPaintsTheCardWholeAsTheLegacyRepolishDoes)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            expectSameRepaint(*this, m_style.get(), cardOf, [](QWidget* card) { static_cast<Card*>(card)->setExpanded(false); });
        }

        TEST_F(CardFrameFlatSheetTests, Repolish_WarningOfAHeaderPaintsTheHeaderWholeAsTheLegacyRepolishDoes)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            expectSameRepaint(*this, m_style.get(), headerOf, [](QWidget* header) { static_cast<CardHeader*>(header)->setWarning(true); });
        }

        TEST_F(CardFrameFlatSheetTests, Repolish_ReadOnlyOfAHeaderPaintsTheHeaderWholeAsTheLegacyRepolishDoes)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            expectSameRepaint(*this, m_style.get(), headerOf, [](QWidget* header) { static_cast<CardHeader*>(header)->setReadOnly(true); });
        }

        TEST_F(CardFrameFlatSheetTests, Repolish_ContentModifiedOfAHeaderPaintsTheHeaderWholeAsTheLegacyRepolishDoes)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            expectSameRepaint(*this, m_style.get(), headerOf,
                [](QWidget* header) { static_cast<CardHeader*>(header)->setContentModified(true); });
        }

        TEST_F(CardFrameFlatSheetTests, Repolish_DrawSimpleOfATitleBarPaintsTheBarWholeAsTheLegacyRepolishDoes)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            expectSameRepaint(*this, m_style.get(), barOf, [](QWidget* bar) { static_cast<TitleBar*>(bar)->setDrawSimple(true); });
        }

        TEST_F(CardFrameFlatSheetTests, Repolish_TearEnabledOfATitleBarPaintsTheBarWholeAsTheLegacyRepolishDoes)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            expectSameRepaint(*this, m_style.get(), barOf, [](QWidget* bar) { static_cast<TitleBar*>(bar)->setTearEnabled(true); });
        }

        TEST_F(CardFrameFlatSheetTests, Repolish_TheLegacyRungAsksForNoPaintOfItsOwn)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The sheet style repolishes the widget, which is what paints it in the legacy GUI (the twins above): the helper's own
            // connection asks for nothing at the Qss rung, nor at the passthrough rung, where no rule and no element reads the property
            CardTwins twins(*this, buildRepolishUsers, QSize(300, 420));
            auto* header = static_cast<CardHeader*>(headerOf(twins.m_flat));
            for (const GuiPerfFloor::Rung rung : { LegacyRung, GuiPerfFloor::Rung::Passthrough })
            {
                const QList<QPointer<QWidget>> shown = shownIn(header);
                PaintRegions paints;
                for (const QPointer<QWidget>& widget : shown)
                {
                    paints.watch(widget);
                }
                settle();
                paints.clear();
                {
                    RungScope scope(rung);
                    header->setReadOnly(!header->isReadOnly());
                    settle();
                }
                for (const QPointer<QWidget>& widget : shown)
                {
                    EXPECT_FALSE(paints.painted(widget)) << (rung == LegacyRung ? "Qss" : "passthrough");
                }
            }
        }
    } // namespace CardFrameFlatSheetTestsInternal
} // namespace AzQtComponents
