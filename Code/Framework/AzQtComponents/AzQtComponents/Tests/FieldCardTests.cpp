/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/FieldTwins.h>

#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>

#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The header of a card (Card.qss:272-287): the title of a card that is disabled, or only drawn as disabled (Card::mockDisabledState
    // disables the title and the icon and leaves the expander on), is #999999; and the expander of a header shows its caret at full
    // opacity even when the card is disabled, where the CheckBox.qss:130-139 rule for an expander gave the faded one everywhere else.
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
    } // namespace FieldCardTestsInternal
} // namespace AzQtComponents
