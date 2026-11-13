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
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <AzQtComponents/Components/Widgets/PushButton.h>

#include <QCheckBox>
#include <QGuiApplication>
#include <QIcon>
#include <QIconEngine>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The header of a card (Card.qss:272-287): the title of a card that is disabled, or only drawn as disabled (Card::mockDisabledState
    // disables the title and the icon and leaves the expander on), is #999999; and the expander of a header shows its caret at full
    // opacity even when the card is disabled, where the CheckBox.qss:130-139 rule for an expander gave the faded one everywhere else.
    // The icons of the Help and ContextMenu buttons of a header (Card.qss:229-257) are recolored in the modes of a disabled card and
    // of a focused button as the legacy GUI recolored them (T-CD.3).
    // ----------------------------------------------------------------------------------

    namespace FieldCardTestsInternal
    {
        using namespace FieldTwins;

        class FieldCardTests
            : public Fixture
        {
        };

        const char* const g_names[] = { "enabled", "disabled", "mock", "collapsed", "collapsedDisabled", "collapsedMock" };

        //! One card for each state: enabled, disabled, mock disabled, and the three of them collapsed
        void buildCards(QWidget& root, bool legacy)
        {
            Q_UNUSED(legacy);
            auto* layout = new QVBoxLayout(&root);
            for (const char* name : g_names)
            {
                const bool disabled = qstrcmp(name, "disabled") == 0 || qstrcmp(name, "collapsedDisabled") == 0;
                const bool mock = qstrcmp(name, "mock") == 0 || qstrcmp(name, "collapsedMock") == 0;
                const bool collapsed = QString::fromLatin1(name).startsWith(QStringLiteral("collapsed"));

                auto* card = new Card(&root);
                card->setObjectName(QString::fromLatin1(name));
                card->setTitle(QStringLiteral("A card"));
                card->setContentWidget(new QLabel(QStringLiteral("content"), card));
                card->setExpanded(!collapsed);
                if (disabled)
                {
                    card->setEnabled(false);
                }
                if (mock)
                {
                    card->mockDisabledState(true);
                }
                layout->addWidget(card);
            }
            layout->addStretch();
        }

        ElidingLabel* titleOf(const Card* card)
        {
            return card->header()->findChild<ElidingLabel*>(QStringLiteral("Title"));
        }

        QCheckBox* expanderOf(const Card* card)
        {
            return card->header()->findChild<QCheckBox*>(QStringLiteral("Expander"));
        }

        //! A check box as QCheckBox::paintEvent draws it (QCheckBox::initStyleOption)
        QImage paintCheckBox(const QCheckBox* box, qreal ratio)
        {
            return paintThroughStyle(box, ratio, [box](QPainter& painter)
            {
                QStyleOptionButton option;
                option.initFrom(box);
                option.text = box->text();
                option.icon = box->icon();
                option.iconSize = box->iconSize();
                option.state |= box->isChecked() ? QStyle::State_On : QStyle::State_Off;
                box->style()->drawControl(QStyle::CE_CheckBox, &option, &painter, box);
            });
        }

        // ----------------------------------------------------------------------------------
        // T-CD.1: the colours of a card's title. Card.qss:272 `AzQtComponents--Card QLabel:disabled { color: #999999 }`: the title of a
        // card that is disabled is #999999, where the flattened one stayed white because the label was polished (ElidingLabel::elide)
        // before its header was in a card.
        // ----------------------------------------------------------------------------------

        TEST_F(FieldCardTests, Title_IsTheColourOfTheSheetInEveryState)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildCards, QSize(300, 420));

            for (const char* name : g_names)
            {
                const auto* legacyCard = twins.legacyChild<Card>(name);
                const auto* flatCard = twins.flatChild<Card>(name);
                ASSERT_TRUE(legacyCard != nullptr && flatCard != nullptr) << name;
                const auto* legacy = titleOf(legacyCard);
                const auto* flat = titleOf(flatCard);
                ASSERT_TRUE(legacy != nullptr && flat != nullptr) << name;
                EXPECT_EQ(flat->isEnabled(), legacy->isEnabled()) << name;
                for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
                {
                    for (const QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText })
                    {
                        EXPECT_EQ(flat->palette().color(group, role).name(), legacy->palette().color(group, role).name())
                            << name << ", group " << int(group) << ", role " << int(role);
                    }
                }
            }

            // the title that is turned off, by the card or by the header alone, is the grey of Card.qss:272
            for (const char* name : { "disabled", "mock", "collapsedDisabled", "collapsedMock" })
            {
                const auto* title = titleOf(twins.flatChild<Card>(name));
                EXPECT_FALSE(title->isEnabled()) << name;
                EXPECT_EQ(title->palette().color(QPalette::Disabled, QPalette::WindowText).name(), QStringLiteral("#999999")) << name;
            }
        }

        // ----------------------------------------------------------------------------------
        // T-CD.2: the caret of the expander, at full opacity in a disabled card
        // ----------------------------------------------------------------------------------

        TEST_F(FieldCardTests, Expander_PaintsTheCaretOfTheSheetInEveryStateAndRatio)
        {
            Twins twins(*this, buildCards, QSize(300, 420));

            for (const char* name : g_names)
            {
                const auto* legacy = expanderOf(twins.legacyChild<Card>(name));
                const auto* flat = expanderOf(twins.flatChild<Card>(name));
                ASSERT_TRUE(legacy != nullptr && flat != nullptr) << name;
                EXPECT_EQ(flat->isEnabled(), legacy->isEnabled()) << name;
                EXPECT_EQ(flat->isChecked(), legacy->isChecked()) << name;
                EXPECT_TRUE(flat->size() == legacy->size()) << name << ": " << sizeText(flat->size()) << " against " << sizeText(legacy->size());
                for (const qreal ratio : { 1.0, 1.5, 2.0 })
                {
                    const QImage expected = paintCheckBox(legacy, ratio);
                    const QImage actual = paintCheckBox(flat, ratio);
                    EXPECT_TRUE(isDrawnOn(expected)) << name << " ratio " << ratio;
                    EXPECT_EQ(differing(actual, expected), 0) << name << " ratio " << ratio << ": " << firstDifference(actual, expected);
                }
            }
        }

        TEST_F(FieldCardTests, Expander_OfADisabledCardShowsTheSameCaretAsAnEnabledOne)
        {
            Twins twins(*this, buildCards, QSize(300, 420));

            // Card.qss:277-287: the faded caret is for the expanders that are not in a card header
            for (const auto& pair : { std::make_pair("enabled", "disabled"), std::make_pair("collapsed", "collapsedDisabled") })
            {
                const auto* enabled = expanderOf(twins.flatChild<Card>(pair.first));
                const auto* disabled = expanderOf(twins.flatChild<Card>(pair.second));
                ASSERT_TRUE(enabled != nullptr && disabled != nullptr);
                EXPECT_TRUE(enabled->isEnabled());
                EXPECT_FALSE(disabled->isEnabled());
                for (const qreal ratio : { 1.0, 1.5, 2.0 })
                {
                    const QImage a = paintCheckBox(enabled, ratio);
                    const QImage b = paintCheckBox(disabled, ratio);
                    EXPECT_EQ(differing(a, b), 0) << pair.first << " ratio " << ratio << ": " << firstDifference(a, b);
                }
            }
        }

        TEST_F(FieldCardTests, Expander_OutsideACardHeaderKeepsTheFadedCaret)
        {
            // CheckBox.qss:130-139: an expander that is not in a card header (a tree's, a section's) has the faded caret when disabled
            QWidget legacyRoot;
            QWidget flatRoot;
            for (QWidget* root : { &legacyRoot, &flatRoot })
            {
                auto* layout = new QVBoxLayout(root);
                for (const bool enabled : { true, false })
                {
                    auto* expander = new QCheckBox(root);
                    expander->setObjectName(enabled ? QStringLiteral("on") : QStringLiteral("off"));
                    Style::addClass(expander, QStringLiteral("Expander"));
                    expander->setChecked(true);
                    expander->setEnabled(enabled);
                    layout->addWidget(expander);
                }
            }
            legacy(legacyRoot);
            show(legacyRoot, QSize(80, 80));
            flat(flatRoot);
            show(flatRoot, QSize(80, 80));

            const auto* legacyOn = legacyRoot.findChild<QCheckBox*>(QStringLiteral("on"));
            const auto* legacyOff = legacyRoot.findChild<QCheckBox*>(QStringLiteral("off"));
            const auto* flatOn = flatRoot.findChild<QCheckBox*>(QStringLiteral("on"));
            const auto* flatOff = flatRoot.findChild<QCheckBox*>(QStringLiteral("off"));
            for (const qreal ratio : { 1.0, 1.5, 2.0 })
            {
                EXPECT_EQ(differing(paintCheckBox(flatOff, ratio), paintCheckBox(legacyOff, ratio)), 0) << "ratio " << ratio;
                EXPECT_EQ(differing(paintCheckBox(flatOn, ratio), paintCheckBox(legacyOn, ratio)), 0) << "ratio " << ratio;

                // and the faded one is not the full one
                EXPECT_GT(differing(paintCheckBox(flatOff, ratio), paintCheckBox(flatOn, ratio)), 0) << "ratio " << ratio;
            }
        }

        // ----------------------------------------------------------------------------------
        // T-CD.3: the icons of the Help and ContextMenu buttons of a header, in every mode they are drawn in
        //
        // Card::generatedIconPixmap had a guard "don't gray out menu icons in the header, even if the card is disabled". The legacy GUI
        // never ran it: QIcon asks the application's style for the icon of a mode other than Normal with a bare option
        // (QApplicationPrivate::applyQIconStyleHelper, QStyleOption(0), no styleObject), the sheet style draws the label of a push
        // button itself (QStyleSheetStyle::drawControl, CE_PushButtonLabel: ParentStyle::drawControl) so that the Style under it is
        // drawing nothing then, and the guard found no widget: PushButton::generatedIconPixmap recolored the icons of a disabled card
        // (#999999) and of a focused button (#94D2FF). The flattened Style draws the label itself, with the button as the widget it is
        // drawing, so the guard kept them white. The guard is for the legacy rung only now (Card.cpp, generatedIconPixmap). Captured:
        // Gallery 04_card, the buttons of the disabled card.
        //
        // The twins are the cards of both GUIs. The application's style is Fusion in this fixture, which would make the icons of both
        // twins; so each twin's icon asks the style that is the application's in its GUI, as Qt does: the Style under the sheet style
        // (a Style that draws nothing here) for the legacy twin, the flattened Style that draws the button for the flat twin.
        // ----------------------------------------------------------------------------------

        //! The icon of a menu button, as Card.qss's are: white, and nothing around it
        QPixmap iconDisc(const QSize& size)
        {
            QPixmap pixmap(size);
            pixmap.fill(Qt::transparent);
            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setPen(Qt::NoPen);
            painter.setBrush(Qt::white);
            painter.drawEllipse(QRectF(QPointF(0, 0), QSizeF(size)).adjusted(1, 1, -1, -1));
            return pixmap;
        }

        //! An icon whose pixmaps are made as Qt makes them for the application: the Normal one as it is, any other mode by the style
        //! of the application (QPixmapIconEngine::pixmap, then QApplicationPrivate::applyQIconStyleHelper)
        class ApplicationStyleIcon
            : public QIconEngine
        {
        public:
            explicit ApplicationStyleIcon(const QStyle* applicationStyle)
                : m_applicationStyle(applicationStyle)
            {
            }

            QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State) override
            {
                QPixmap pixmap = iconDisc(size);
                if (mode != QIcon::Normal)
                {
                    QStyleOption bare(0);
                    bare.palette = QGuiApplication::palette();
                    const QPixmap generated = m_applicationStyle->generatedIconPixmap(mode, pixmap, &bare);
                    if (!generated.isNull())
                    {
                        pixmap = generated;
                    }
                }
                return pixmap;
            }

            void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override
            {
                painter->drawPixmap(rect, pixmap(rect.size(), mode, state));
            }

            QIconEngine* clone() const override
            {
                return new ApplicationStyleIcon(m_applicationStyle);
            }

        private:
            const QStyle* m_applicationStyle;
        };

        //! The rung a test runs at for its scope, and the rung the fixture runs at after it
        class CardRungScope
        {
        public:
            explicit CardRungScope(GuiPerfFloor::Rung rung)
                : m_before(GuiPerfFloor::rung())
            {
                GuiPerfFloor::setRungForTesting(rung);
            }

            ~CardRungScope()
            {
                GuiPerfFloor::setRungForTesting(m_before);
            }

        private:
            GuiPerfFloor::Rung m_before;
        };

        //! An enabled card and a disabled one, each with the Help button and the ContextMenu button in its header
        void buildMenuCards(QWidget& root, bool legacy)
        {
            Q_UNUSED(legacy);
            auto* layout = new QVBoxLayout(&root);
            for (const bool enabled : { true, false })
            {
                auto* card = new Card(&root);
                card->setObjectName(enabled ? QStringLiteral("enabled") : QStringLiteral("disabled"));
                card->setTitle(QStringLiteral("A card"));
                card->setContentWidget(new QLabel(QStringLiteral("content"), card));
                card->header()->setHelpURL(QStringLiteral("https://o3de.org/docs/"));
                card->header()->setHasContextMenu(true);
                card->setEnabled(enabled);
                layout->addWidget(card);
            }
            layout->addStretch();
        }

        QPushButton* menuButtonOf(const Card* card, const char* name)
        {
            return card ? card->header()->findChild<QPushButton*>(QString::fromLatin1(name)) : nullptr;
        }

        //! A push button as QPushButton::paintEvent draws it (QPushButton::initStyleOption), with the icon the application's style makes
        QImage paintMenuButton(const QPushButton* button, const QStyle* applicationStyle, QStyle::State extraState, qreal ratio)
        {
            return paintThroughStyle(button, ratio, [&](QPainter& painter)
            {
                QStyleOptionButton option;
                option.initFrom(button);
                option.state |= extraState;
                option.features = button->isFlat() ? QStyleOptionButton::Flat : QStyleOptionButton::None;
                option.text = button->text();
                option.icon = QIcon(new ApplicationStyleIcon(applicationStyle));
                option.iconSize = button->iconSize();
                button->style()->drawControl(QStyle::CE_PushButton, &option, &painter, button);
            });
        }

        TEST_F(FieldCardTests, MenuButtonIcons_AreRecoloredInEveryModeAndRatioAsTheLegacyGuiRecolorsThem)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The Style under the application's sheet style in the legacy GUI: nothing is drawn through it
            Style legacyApplicationStyle(QStyleFactory::create(QStringLiteral("Fusion")));
            Twins twins(*this, buildMenuCards, QSize(320, 160));

            // PushButtonConfig.ini:47-50 (IconButton): Normal is the pixmap as it is, a focused button's icon is Active, a disabled one's
            // Disabled
            const PushButton::Config buttonConfig = PushButton::defaultConfig();
            struct IconCase
            {
                const char* m_card;
                QStyle::State m_extraState;
                QColor m_ink;
            };
            const IconCase cases[] = {
                { "enabled", QStyle::State_None, QColor(Qt::white) },
                { "enabled", QStyle::State_HasFocus, buttonConfig.iconButton.activeColor },
                { "disabled", QStyle::State_None, buttonConfig.iconButton.disabledColor },
            };

            for (const IconCase& icon : cases)
            {
                for (const char* name : { "Help", "ContextMenu" })
                {
                    SCOPED_TRACE(QStringLiteral("%1 card, state %2, %3").arg(QString::fromLatin1(icon.m_card)).arg(int(icon.m_extraState))
                        .arg(QString::fromLatin1(name)).toStdString());
                    const QPushButton* legacyButton = menuButtonOf(twins.legacyChild<Card>(icon.m_card), name);
                    const QPushButton* flatButton = menuButtonOf(twins.flatChild<Card>(icon.m_card), name);
                    ASSERT_TRUE(legacyButton != nullptr && flatButton != nullptr);
                    ASSERT_TRUE(flatButton->isVisible() && legacyButton->isVisible());
                    EXPECT_EQ(sizeText(flatButton->size()), sizeText(legacyButton->size()));
                    EXPECT_EQ(sizeText(flatButton->iconSize()), sizeText(legacyButton->iconSize()));

                    for (const qreal ratio : { 1.0, 1.5, 2.0 })
                    {
                        const QImage expected = paintMenuButton(legacyButton, &legacyApplicationStyle, icon.m_extraState, ratio);
                        const QImage actual = paintMenuButton(flatButton, flatButton->style(), icon.m_extraState, ratio);
                        ASSERT_TRUE(isDrawnOn(expected)) << "ratio " << ratio;

                        // the legacy icon is the colour the legacy GUI gave it (the capture: #999999 in the disabled card), at its centre
                        EXPECT_EQ(expected.pixelColor(expected.width() / 2, expected.height() / 2).name(), icon.m_ink.name())
                            << "ratio " << ratio;
                        EXPECT_EQ(differing(actual, expected), 0) << "ratio " << ratio << ": " << firstDifference(actual, expected);
                    }
                }
            }
        }

        TEST_F(FieldCardTests, MenuButtonIcons_TheLegacyRungKeepsTheGuardOfTheStyleThatIsDrawingTheButton)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildMenuCards, QSize(320, 160));
            const QPushButton* button = menuButtonOf(twins.flatChild<Card>("disabled"), "Help");
            ASSERT_TRUE(button != nullptr);
            ASSERT_TRUE(Style::hasClass(button, QStringLiteral("CardHeaderMenu")));        // CardHeader.cpp:139

            // The state of Style::drawControl while the label of the button is drawn: the Style is drawing the button
            Style::DrawWidgetSentinel drawing(button);
            const QPixmap disc = iconDisc(QSize(16, 16));
            QStyleOption bare(0);

            // flattened: recolored, as the legacy GUI recolored it
            const QImage flat = m_style->generatedIconPixmap(QIcon::Disabled, disc, &bare).toImage();
            EXPECT_EQ(flat.pixelColor(8, 8).name(), PushButton::defaultConfig().iconButton.disabledColor.name());

            // legacy rung: the code is as it was, the icon of the button the Style is drawing is the pixmap as it is
            {
                CardRungScope legacyRung(GuiPerfFloor::Rung::Qss);
                const QImage legacy = m_style->generatedIconPixmap(QIcon::Disabled, disc, &bare).toImage();
                EXPECT_EQ(firstDifference(legacy, disc.toImage()), "equal");
            }
        }
    } // namespace FieldCardTestsInternal
} // namespace AzQtComponents
