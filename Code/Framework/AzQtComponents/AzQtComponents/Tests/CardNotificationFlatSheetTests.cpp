/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/FieldTwins.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardNotification.h>

#include <QColor>
#include <QFrame>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QMargins>
#include <QPoint>
#include <QWidget>
#include <QRect>
#include <QRegion>
#include <QSize>
#include <QString>
#include <QVBoxLayout>

#include <string>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The notifications of a card (Card.qss:51-70): the message and the icon of a component the inspector warns about, in a pill of
    // their own (#HeaderFrame) on a block (the CardNotification) that holds the buttons below it. The legacy twin is Qt's own style sheet
    // style over Fusion with the application's real sheet (FieldTwins::appSheet: BaseStyleSheet.qss and the files it imports, whose
    // Card.qss these tests quote); the flat twin is the flattened Style with the Global elements of PropertyEditorElements.cpp. The
    // notification is the real class, made as Card::addNotification makes it. Same boxes, same hints, same pixels.
    // ----------------------------------------------------------------------------------

    namespace CardNotificationFlatSheetTestsInternal
    {
        using namespace FieldTwins;

        class CardNotificationFlatSheetTests
            : public Fixture
        {
        };

        // The rules, as the sheet has them (Card.qss:51-70), checked against the real sheet below
        const char* const BlockRule =
            "AzQtComponents--CardNotification\n"
            "{\n"
            "    background-color: #555555;\n"
            "    spacing: 4px;\n"
            "}\n";
        const char* const PillRule =
            "AzQtComponents--CardNotification #HeaderFrame\n"
            "{\n"
            "    padding: 0px 8px 0px 8px;\n"
            "    spacing: 4px;\n"
            "}\n";
        const char* const FillRule =
            "AzQtComponents--CardNotification #HeaderFrame,\n"
            "AzQtComponents--CardNotification #Icon,\n"
            "AzQtComponents--CardNotification #Title\n"
            "{\n"
            "    background-color: #444444;\n"
            "    border-radius: 3px;\n"
            "    border: none;\n"
            "}\n";

        constexpr QRgb BlockColor = 0xFF555555;         // Card.qss:53
        constexpr QRgb PillColor = 0xFF444444;          // Card.qss:67
        constexpr int PillPadding = 8;                  // Card.qss:59
        //! The spacing of a layout in the application: Style::pixelMetric, PM_LayoutHorizontalSpacing and PM_LayoutVerticalSpacing
        //! (Style.cpp), under both rungs. The legacy twin's Fusion would give another.
        constexpr int LayoutSpacing = 3;
        //! The height of the widget a notification holds below its message: a button's, which the base style of each twin sizes
        //! differently (the legacy twin runs over Fusion), so the feature of the twins is a widget of a size of its own
        constexpr int FeatureHeight = 24;

        const qreal Ratios[] = { 1.0, 1.5, 2.0 };

        std::string marginsText(const QMargins& margins)
        {
            return QStringLiteral("(%1, %2, %3, %4)")
                .arg(margins.left()).arg(margins.top()).arg(margins.right()).arg(margins.bottom()).toStdString();
        }

        // ----------------------------------------------------------------------------------
        // The tree
        // ----------------------------------------------------------------------------------

        //! The layouts of a notification have no spacing of their own, so the style gives it: the legacy twin's Fusion would give another
        //! than the application's, and the twins are given the application's
        void giveApplicationSpacing(CardNotification* notification)
        {
            notification->layout()->setSpacing(LayoutSpacing);
            notification->findChild<QFrame*>(QStringLiteral("HeaderFrame"))->layout()->setSpacing(LayoutSpacing);
        }

        //! Three notifications as the inspector makes them: a short message, a message with a widget below it (the button of a
        //! conflicting component), and a message that wraps. The icon and its size are the ones Card::addNotification gives
        //! (Card::Config).
        void buildNotifications(QWidget& root, bool legacy)
        {
            auto* layout = new QVBoxLayout(&root);
            layout->setContentsMargins(8, 8, 8, 8);
            layout->setSpacing(6);

            const Card::Config config = Card::defaultConfig();
            QIcon icon;
            icon.addFile(config.warningIcon, config.warningIconSize);

            const struct { const char* m_name; const char* m_message; bool m_feature; } notifications[] = {
                { "short", "Missing component", false },
                { "feature", "This component conflicts with another one", true },
                { "long", "This component cannot be activated: the services it requires are provided by no component here", false },
            };
            for (const auto& made : notifications)
            {
                auto* notification = new CardNotification(&root, QString::fromLatin1(made.m_message), icon, config.warningIconSize);
                notification->setObjectName(QString::fromLatin1(made.m_name));
                if (made.m_feature)
                {
                    auto* feature = new QWidget(notification);
                    feature->setObjectName(QStringLiteral("feature"));
                    feature->setFixedHeight(FeatureHeight);
                    notification->addFeature(feature);
                }
                layout->addWidget(notification);

                if (legacy)
                {
                    giveApplicationSpacing(notification);
                }
            }
            layout->addStretch();
        }

        //! The notifications under an expanded container card, as Card::addNotification places them in its body (the body of a
        //! container card has no fill, so the block is what shows there)
        void buildContainerCard(QWidget& root, bool legacy)
        {
            auto* layout = new QVBoxLayout(&root);
            layout->setContentsMargins(8, 8, 8, 8);

            auto* card = new Card(&root);
            card->setObjectName(QStringLiteral("card"));
            card->setTitle(QStringLiteral("A card"));
            Card::applyContainerStyle(card);
            card->setContentWidget(new QLabel(QStringLiteral("content"), card));
            CardNotification* notification = card->addNotification(QStringLiteral("Missing component"));
            notification->setObjectName(QStringLiteral("notification"));
            if (legacy)
            {
                giveApplicationSpacing(notification);
            }
            layout->addWidget(card);
            layout->addStretch();
        }

        // ----------------------------------------------------------------------------------
        // Reading a twin
        // ----------------------------------------------------------------------------------

        struct Facts
        {
            QRect m_rect;               //!< in the notification
            QMargins m_contents;        //!< the frame box (QFrame::setFrameRect: SE_ShapedFrameContents)
            QSize m_hint;
            QSize m_minimum;
        };

        Facts factsOf(const QWidget* widget, const QWidget* notification)
        {
            return { QRect(widget->mapTo(notification, QPoint(0, 0)), widget->size()), widget->contentsMargins(), widget->sizeHint(),
                widget->minimumSizeHint() };
        }

        void expectSameFacts(const Facts& legacy, const Facts& flat)
        {
            EXPECT_EQ(rectText(flat.m_rect), rectText(legacy.m_rect));
            EXPECT_EQ(marginsText(flat.m_contents), marginsText(legacy.m_contents));
            EXPECT_EQ(sizeText(flat.m_hint), sizeText(legacy.m_hint));
            EXPECT_EQ(sizeText(flat.m_minimum), sizeText(legacy.m_minimum));
        }

        //! The widget and its children as their paint events draw them onto an image of an explicit ratio, over a grey that stays where
        //! nothing was drawn
        QImage paint(QWidget* widget, qreal ratio)
        {
            QImage image((QSizeF(widget->size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor(0x80, 0x80, 0x80));
            widget->render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
            return image;
        }

        //! The pixel under a logical point
        QRgb pixelAt(const QImage& image, const QPoint& point)
        {
            const qreal ratio = image.devicePixelRatio();
            return image.pixel(qRound(point.x() * ratio), qRound(point.y() * ratio));
        }

        // ----------------------------------------------------------------------------------
        // T-CN.0: the rules these tests quote are the sheet's
        // ----------------------------------------------------------------------------------

        TEST_F(CardNotificationFlatSheetTests, Sheet_HoldsTheRulesTheseTestsQuote)
        {
            const QString sheet = appSheet();
            for (const char* rule : { BlockRule, PillRule, FillRule })
            {
                EXPECT_TRUE(sheet.contains(QString::fromLatin1(rule))) << rule;
            }
        }

        // ----------------------------------------------------------------------------------
        // T-CN.1: the boxes. Card.qss:57-61 pads the pill by 8 at each side, which insets the icon and the message; Text.qss:93-94 gives
        // each label of the pill 1px above and below. The sizes of the notification follow from them.
        // ----------------------------------------------------------------------------------

        TEST_F(CardNotificationFlatSheetTests, Boxes_AreThoseOfTheLegacyNotificationInEveryPart)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildNotifications, QSize(320, 260));

            for (const char* name : { "short", "feature", "long" })
            {
                SCOPED_TRACE(name);
                auto* legacyNotification = twins.legacyChild<CardNotification>(name);
                auto* flatNotification = twins.flatChild<CardNotification>(name);
                ASSERT_TRUE(legacyNotification != nullptr && flatNotification != nullptr);

                expectSameFacts(factsOf(legacyNotification, &twins.m_legacy), factsOf(flatNotification, &twins.m_flat));

                for (const char* id : { "HeaderFrame", "Icon", "Title" })
                {
                    SCOPED_TRACE(id);
                    auto* legacyPart = legacyNotification->findChild<QWidget*>(QString::fromLatin1(id));
                    auto* flatPart = flatNotification->findChild<QWidget*>(QString::fromLatin1(id));
                    ASSERT_TRUE(legacyPart != nullptr && flatPart != nullptr);
                    expectSameFacts(factsOf(legacyPart, legacyNotification), factsOf(flatPart, flatNotification));
                }
                if (auto* legacyFeature = legacyNotification->findChild<QWidget*>(QStringLiteral("feature")))
                {
                    auto* flatFeature = flatNotification->findChild<QWidget*>(QStringLiteral("feature"));
                    ASSERT_TRUE(flatFeature != nullptr);
                    expectSameFacts(factsOf(legacyFeature, legacyNotification), factsOf(flatFeature, flatNotification));
                }
            }
        }

        TEST_F(CardNotificationFlatSheetTests, Boxes_PadThePillBy8AtEachSideAndGiveTheLabelsTheirMargins)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildNotifications, QSize(320, 260));

            // The numbers of the rules, in the legacy twin (they are what Qt made of the sheet) and in the flat one
            for (QWidget* root : { &twins.m_legacy, &twins.m_flat })
            {
                SCOPED_TRACE(root == &twins.m_legacy ? "legacy" : "flat");
                auto* notification = root->findChild<CardNotification*>(QStringLiteral("short"));
                ASSERT_TRUE(notification != nullptr);
                auto* pill = notification->findChild<QFrame*>(QStringLiteral("HeaderFrame"));
                auto* icon = notification->findChild<QLabel*>(QStringLiteral("Icon"));
                auto* title = notification->findChild<QLabel*>(QStringLiteral("Title"));
                ASSERT_TRUE(pill != nullptr && icon != nullptr && title != nullptr);

                EXPECT_EQ(marginsText(notification->contentsMargins()), marginsText(QMargins()));
                EXPECT_EQ(marginsText(pill->contentsMargins()), marginsText(QMargins(PillPadding, 0, PillPadding, 0)));
                EXPECT_EQ(marginsText(icon->contentsMargins()), marginsText(QMargins(0, 1, 0, 1)));
                EXPECT_EQ(marginsText(title->contentsMargins()), marginsText(QMargins(0, 1, 0, 1)));

                // The pill is inset by its padding: the icon starts 8px in, the message ends 8px short of the pill's right edge
                EXPECT_EQ(icon->mapTo(pill, QPoint(0, 0)).x(), PillPadding);
                EXPECT_EQ(pill->width() - (title->mapTo(pill, QPoint(0, 0)).x() + title->width()), PillPadding);
            }
        }

        // ----------------------------------------------------------------------------------
        // T-CN.2: the pixels. The block is #555555 (Card.qss:53), the pill #444444 with 3px corners and no border (Card.qss:67-69), and
        // the labels the same over it.
        // ----------------------------------------------------------------------------------

        TEST_F(CardNotificationFlatSheetTests, Pixels_AreThoseOfTheLegacyNotificationAtEveryRatio)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildNotifications, QSize(320, 260));

            for (const char* name : { "short", "feature", "long" })
            {
                SCOPED_TRACE(name);
                auto* legacyNotification = twins.legacyChild<CardNotification>(name);
                auto* flatNotification = twins.flatChild<CardNotification>(name);
                ASSERT_TRUE(legacyNotification != nullptr && flatNotification != nullptr);

                for (const qreal ratio : Ratios)
                {
                    const QImage legacy = paint(legacyNotification, ratio);
                    const QImage flat = paint(flatNotification, ratio);
                    ASSERT_TRUE(isDrawnOn(legacy)) << "the legacy notification drew something, ratio " << ratio;
                    EXPECT_EQ(firstDifference(legacy, flat), "equal") << "ratio " << ratio;
                }
            }
        }

        TEST_F(CardNotificationFlatSheetTests, Pixels_HaveThePillColorInItsPaddingAndTheBlockColorAroundIt)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildNotifications, QSize(320, 260));

            for (QWidget* root : { &twins.m_legacy, &twins.m_flat })
            {
                SCOPED_TRACE(root == &twins.m_legacy ? "legacy" : "flat");
                auto* notification = root->findChild<CardNotification*>(QStringLiteral("feature"));
                ASSERT_TRUE(notification != nullptr);
                const QWidget* pill = notification->findChild<QFrame*>(QStringLiteral("HeaderFrame"));
                const QWidget* feature = notification->findChild<QWidget*>(QStringLiteral("feature"));
                ASSERT_TRUE(pill != nullptr && feature != nullptr);
                const QRect pillRect(pill->mapTo(notification, QPoint(0, 0)), pill->size());
                const QRect featureRect(feature->mapTo(notification, QPoint(0, 0)), feature->size());

                for (const qreal ratio : Ratios)
                {
                    SCOPED_TRACE(ratio);
                    const QImage image = paint(notification, ratio);

                    // Inside the padding, left of the icon, in the middle of the pill: the pill's fill
                    EXPECT_EQ(pixelAt(image, QPoint(pillRect.left() + PillPadding / 2, pillRect.center().y())), PillColor);
                    // Between the pill and the feature, in the layout's spacing: what the block fills
                    ASSERT_GT(featureRect.top() - pillRect.bottom() - 1, 1) << "the pill and the feature have room between them";
                    const int gap = pillRect.bottom() + 1 + (featureRect.top() - pillRect.bottom() - 1) / 2;
                    EXPECT_EQ(pixelAt(image, QPoint(pillRect.center().x(), gap)), BlockColor);
                }
            }
        }

        TEST_F(CardNotificationFlatSheetTests, Pixels_OfADisabledNotificationKeepTheFills)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildNotifications, QSize(320, 260));
            for (QWidget* root : { &twins.m_legacy, &twins.m_flat })
            {
                SCOPED_TRACE(root == &twins.m_legacy ? "legacy" : "flat");
                auto* notification = root->findChild<CardNotification*>(QStringLiteral("feature"));
                ASSERT_TRUE(notification != nullptr);
                notification->setEnabled(false);
                QApplication::processEvents();

                const QWidget* pill = notification->findChild<QFrame*>(QStringLiteral("HeaderFrame"));
                const QWidget* feature = notification->findChild<QWidget*>(QStringLiteral("feature"));
                const QRect pillRect(pill->mapTo(notification, QPoint(0, 0)), pill->size());
                const QRect featureRect(feature->mapTo(notification, QPoint(0, 0)), feature->size());
                const int gap = pillRect.bottom() + 1 + (featureRect.top() - pillRect.bottom() - 1) / 2;

                const QImage image = paint(notification, 1.0);
                EXPECT_EQ(pixelAt(image, QPoint(pillRect.left() + PillPadding / 2, pillRect.center().y())), PillColor);
                EXPECT_EQ(pixelAt(image, QPoint(pillRect.center().x(), gap)), BlockColor);
            }
        }

        // ----------------------------------------------------------------------------------
        // T-CN.3: the labels. Each is a box of its own: 1px above and below, #444444, 3px corners, no border. Over the pill they show
        // nothing; alone they do.
        // ----------------------------------------------------------------------------------

        TEST_F(CardNotificationFlatSheetTests, Labels_AreFilledAndRoundedAsTheLegacyLabelsAreAtEveryRatio)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildNotifications, QSize(320, 260));

            for (const char* name : { "short", "long" })
            {
                for (const char* id : { "Icon", "Title" })
                {
                    SCOPED_TRACE(QStringLiteral("%1 %2").arg(QString::fromLatin1(name), QString::fromLatin1(id)).toStdString());
                    auto* legacyLabel = twins.legacyChild<CardNotification>(name)->findChild<QLabel*>(QString::fromLatin1(id));
                    auto* flatLabel = twins.flatChild<CardNotification>(name)->findChild<QLabel*>(QString::fromLatin1(id));
                    ASSERT_TRUE(legacyLabel != nullptr && flatLabel != nullptr);

                    for (const qreal ratio : Ratios)
                    {
                        const QImage legacy = paint(legacyLabel, ratio);
                        const QImage flat = paint(flatLabel, ratio);
                        // The label alone has its own fill, 1px short of the top and the bottom of its rect
                        ASSERT_TRUE(isDrawnOn(legacy)) << "the legacy label drew something, ratio " << ratio;
                        EXPECT_EQ(firstDifference(legacy, flat), "equal") << "ratio " << ratio;
                    }
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // T-CN.4: in a card, as Card::addNotification places it. The body of a container card has no fill, so the block shows there.
        // ----------------------------------------------------------------------------------

        TEST_F(CardNotificationFlatSheetTests, InAContainerCard_TheBlockAndThePillAreThoseOfTheLegacyNotification)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildContainerCard, QSize(320, 200));
            auto* legacyNotification = twins.legacyChild<CardNotification>("notification");
            auto* flatNotification = twins.flatChild<CardNotification>("notification");
            ASSERT_TRUE(legacyNotification != nullptr && flatNotification != nullptr);
            ASSERT_EQ(sizeText(flatNotification->size()), sizeText(legacyNotification->size()))
                << "the card gives its notification the width the legacy card gives it";

            // Where the card puts the notification is the card's (Card.qss:21-33 against Card::shapedFrameContents); the parts are in it
            expectSameFacts(factsOf(legacyNotification, legacyNotification), factsOf(flatNotification, flatNotification));
            for (const char* id : { "HeaderFrame", "Icon", "Title" })
            {
                SCOPED_TRACE(id);
                expectSameFacts(
                    factsOf(legacyNotification->findChild<QWidget*>(QString::fromLatin1(id)), legacyNotification),
                    factsOf(flatNotification->findChild<QWidget*>(QString::fromLatin1(id)), flatNotification));
            }

            for (const qreal ratio : Ratios)
            {
                const QImage legacy = paint(legacyNotification, ratio);
                const QImage flat = paint(flatNotification, ratio);
                ASSERT_TRUE(isDrawnOn(legacy));
                EXPECT_EQ(firstDifference(legacy, flat), "equal") << "ratio " << ratio;
            }
        }
    } // namespace CardNotificationFlatSheetTestsInternal
} // namespace AzQtComponents
