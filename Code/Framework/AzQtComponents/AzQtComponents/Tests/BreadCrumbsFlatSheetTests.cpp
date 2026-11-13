/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/FieldTwins.h>

#include <AzQtComponents/Components/Widgets/BreadCrumbs.h>

#include <QLineEdit>
#include <QPalette>
#include <QSize>
#include <QStackedWidget>
#include <QStyleOption>
#include <QVBoxLayout>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The path edit of the breadcrumbs: the line edit the breadcrumbs show in place of the label while the path is edited
    // (BreadCrumbs.cpp:98-102, flagged to be ignored by Style). The application's sheet reaches it with BreadCrumbs.qss:26-31
    // (`border: 0px; background: white; color: black`) over BaseStyleSheet.qss:68-84 and LineEdit.qss:11-35. The legacy twin is Qt's own
    // style sheet style over Fusion with the application's real sheet (FieldTwins::appSheet); the flat twin is the flattened Style.
    // Native Editor, n3 07_main_window_final__page-QStackedWidget-QLineEdit1, the viewport's path bar: a white field, the text 1px up.
    // ----------------------------------------------------------------------------------

    namespace BreadCrumbsFlatSheetTestsInternal
    {
        using namespace FieldTwins;

        class BreadCrumbsFlatSheetTests
            : public Fixture
        {
        };

        //! The path bar of an editable breadcrumbs, and beside it a line edit on the page of a stacked widget that is not the breadcrumbs'
        void buildCrumbs(QWidget& root, bool legacy)
        {
            Q_UNUSED(legacy);
            auto* layout = new QVBoxLayout(&root);
            layout->setContentsMargins(0, 0, 0, 0);

            auto* crumbs = new BreadCrumbs(&root);
            crumbs->setObjectName(QStringLiteral("crumbs"));
            crumbs->setEditable(true);
            crumbs->setCurrentPath(QStringLiteral("DefaultLevel.prefab"));
            layout->addWidget(crumbs);

            auto* stack = new QStackedWidget(&root);
            auto* other = new QLineEdit(QStringLiteral("other"), stack);
            other->setObjectName(QStringLiteral("other"));
            stack->addWidget(other);
            layout->addWidget(stack);
        }

        QLineEdit* pathEditOf(const BreadCrumbs* crumbs)
        {
            return crumbs ? crumbs->findChild<QLineEdit*>() : nullptr;
        }

        //! The option QLineEdit::paintEvent gives the panel (QLineEdit::initStyleOption, protected), in a rect of an explicit size: the
        //! layouts of the twins need not share the size of the edit
        QStyleOptionFrame panelOption(const QLineEdit* edit, const QSize& size)
        {
            QStyleOptionFrame option;
            option.initFrom(edit);
            option.rect = QRect(QPoint(0, 0), size);
            option.lineWidth = edit->hasFrame() ? edit->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &option, edit) : 0;
            option.midLineWidth = 0;
            option.state |= QStyle::State_Sunken;
            option.features = QStyleOptionFrame::None;
            return option;
        }

        //! The panel of the edit as its paint event draws it (PE_PanelLineEdit through its own style), onto an image of an explicit device
        //! ratio, on the colour of the bar behind it (QWidget::render would hand the style the ratio of the screen the test runs on)
        QImage paintPanel(const QLineEdit* edit, const QSize& size, qreal ratio)
        {
            QImage image((QSizeF(size) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor(0x44, 0x44, 0x44));
            QPainter painter(&image);
            const QStyleOptionFrame option = panelOption(edit, size);
            edit->style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, &painter, edit);
            painter.end();
            return image;
        }

        const QPalette::ColorGroup Groups[] = { QPalette::Active, QPalette::Inactive, QPalette::Disabled };
        const QPalette::ColorRole Roles[] = {
            QPalette::Base, QPalette::Button, QPalette::ButtonText, QPalette::Text, QPalette::Window, QPalette::WindowText,
            QPalette::PlaceholderText,
        };

        const char* groupName(QPalette::ColorGroup group)
        {
            return group == QPalette::Active ? "Active" : (group == QPalette::Inactive ? "Inactive" : "Disabled");
        }

        // ----------------------------------------------------------------------------------
        // T-BC.1: the palette of the edit, every group and role the rules set
        // ----------------------------------------------------------------------------------

        TEST_F(BreadCrumbsFlatSheetTests, PathEdit_HasThePaletteOfTheSheetInEveryGroup)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildCrumbs, QSize(300, 120));
            const QLineEdit* legacy = pathEditOf(twins.legacyChild<BreadCrumbs>("crumbs"));
            const QLineEdit* flat = pathEditOf(twins.flatChild<BreadCrumbs>("crumbs"));
            ASSERT_TRUE(legacy != nullptr && flat != nullptr);

            for (const QPalette::ColorGroup group : Groups)
            {
                for (const QPalette::ColorRole role : Roles)
                {
                    EXPECT_EQ(flat->palette().color(group, role).name(QColor::HexArgb).toStdString(),
                        legacy->palette().color(group, role).name(QColor::HexArgb).toStdString())
                        << groupName(group) << ", role " << static_cast<int>(role);
                }
            }
        }

        TEST_F(BreadCrumbsFlatSheetTests, PathEdit_IsWhiteWithBlackTextAndTheDisabledFieldWhenDisabled)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildCrumbs, QSize(300, 120));
            const QLineEdit* flat = pathEditOf(twins.flatChild<BreadCrumbs>("crumbs"));
            ASSERT_TRUE(flat != nullptr);

            // The Native numbers (n3 Editor dump of the screen above): `background: white` (BreadCrumbs.qss:29) on Base, Button and Window,
            // `color: black` (:30) on the text roles and, at half the alpha, on the placeholder; the :disabled rule (LineEdit.qss:31-35)
            // outranks them for the Disabled group
            for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive })
            {
                for (const QPalette::ColorRole role : { QPalette::Base, QPalette::Button, QPalette::Window })
                {
                    EXPECT_EQ(flat->palette().color(group, role).name(QColor::HexArgb).toStdString(), "#ffffffff")
                        << groupName(group) << ", role " << static_cast<int>(role);
                }
                for (const QPalette::ColorRole role : { QPalette::Text, QPalette::ButtonText, QPalette::WindowText })
                {
                    EXPECT_EQ(flat->palette().color(group, role).name(QColor::HexArgb).toStdString(), "#ff000000")
                        << groupName(group) << ", role " << static_cast<int>(role);
                }
                EXPECT_EQ(flat->palette().color(group, QPalette::PlaceholderText).name(QColor::HexArgb).toStdString(), "#80000000")
                    << groupName(group);
            }
            for (const QPalette::ColorRole role : { QPalette::Base, QPalette::Button, QPalette::Window })
            {
                EXPECT_EQ(flat->palette().color(QPalette::Disabled, role).name(QColor::HexArgb).toStdString(), "#ff666666")
                    << "role " << static_cast<int>(role);
            }
            for (const QPalette::ColorRole role : { QPalette::Text, QPalette::ButtonText, QPalette::WindowText })
            {
                EXPECT_EQ(flat->palette().color(QPalette::Disabled, role).name(QColor::HexArgb).toStdString(), "#ff999999")
                    << "role " << static_cast<int>(role);
            }
            EXPECT_EQ(
                flat->palette().color(QPalette::Disabled, QPalette::PlaceholderText).name(QColor::HexArgb).toStdString(), "#80999999");
        }

        TEST_F(BreadCrumbsFlatSheetTests, PathEdit_ALineEditOfAnotherStackKeepsTheFieldsPalette)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildCrumbs, QSize(300, 120));
            const QLineEdit* other = twins.flatChild<QLineEdit>("other");
            ASSERT_TRUE(other != nullptr);

            // The field's own fill (BaseStyleSheet.qss:76) and not the white of the breadcrumbs' rule: of the roles the rule of the
            // breadcrumbs sets, a line edit keeps the application's Button and Window (only a combo box or a spin box takes the field's
            // fill for its button face, Style.cpp applyFlattenedTextEntryPalette), and the field's text
            const QPalette application = QApplication::palette(other);
            for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive })
            {
                EXPECT_EQ(other->palette().color(group, QPalette::Base).name().toStdString(), "#cccccc") << groupName(group);
                EXPECT_EQ(other->palette().color(group, QPalette::Text).name().toStdString(), "#000000") << groupName(group);
                for (const QPalette::ColorRole role : { QPalette::Button, QPalette::Window, QPalette::ButtonText, QPalette::WindowText })
                {
                    EXPECT_EQ(other->palette().color(group, role).name(QColor::HexArgb).toStdString(),
                        application.color(group, role).name(QColor::HexArgb).toStdString())
                        << groupName(group) << ", role " << static_cast<int>(role);
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // T-BC.2: the field, in pixels at the device ratios of the GUI, and where it lays its text out
        // ----------------------------------------------------------------------------------

        TEST_F(BreadCrumbsFlatSheetTests, PathEdit_PaintsTheFieldOfTheSheetAtEveryRatioEnabledAndDisabled)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildCrumbs, QSize(300, 120));
            QLineEdit* legacy = pathEditOf(twins.legacyChild<BreadCrumbs>("crumbs"));
            QLineEdit* flat = pathEditOf(twins.flatChild<BreadCrumbs>("crumbs"));
            ASSERT_TRUE(legacy != nullptr && flat != nullptr);

            const QSize size(120, 22);          // the field of the Native capture is 22 high
            for (const bool enabled : { true, false })
            {
                legacy->setEnabled(enabled);
                flat->setEnabled(enabled);
                for (const qreal ratio : { 1.0, 1.5, 2.0 })
                {
                    const QImage legacyImage = paintPanel(legacy, size, ratio);
                    const QImage flatImage = paintPanel(flat, size, ratio);

                    // Drawn: the middle of the field is its fill in the legacy twin (white, #666666 when disabled), whole to the edges
                    const QColor fill = enabled ? QColor(0xFF, 0xFF, 0xFF) : QColor(0x66, 0x66, 0x66);
                    EXPECT_EQ(legacyImage.pixelColor(QPoint(static_cast<int>(60 * ratio), static_cast<int>(11 * ratio))), fill)
                        << "enabled " << enabled << " at ratio " << ratio;
                    EXPECT_EQ(differing(legacyImage, flatImage), 0)
                        << "enabled " << enabled << " at ratio " << ratio << ": " << firstDifference(flatImage, legacyImage);
                }
            }
        }

        TEST_F(BreadCrumbsFlatSheetTests, PathEdit_LaysItsTextOutInTheRectLessThePaddingOfTheFieldAndHasTheHintOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildCrumbs, QSize(300, 120));
            QLineEdit* legacy = pathEditOf(twins.legacyChild<BreadCrumbs>("crumbs"));
            QLineEdit* flat = pathEditOf(twins.flatChild<BreadCrumbs>("crumbs"));
            ASSERT_TRUE(legacy != nullptr && flat != nullptr);

            // The text is laid out in SE_LineEditContents: the padding of LineEdit.qss:15, :18-19 (left 0, bottom 1, top -1) moves the
            // contents up a pixel, over a border of 0 (the text in the Native capture sits 1px above where Fusion's frame puts it)
            const QStyleOptionFrame legacyOption = panelOption(legacy, QSize(120, 22));
            const QStyleOptionFrame flatOption = panelOption(flat, QSize(120, 22));
            const QRect legacyContents = legacy->style()->subElementRect(QStyle::SE_LineEditContents, &legacyOption, legacy);
            const QRect flatContents = flat->style()->subElementRect(QStyle::SE_LineEditContents, &flatOption, flat);
            EXPECT_EQ(rectText(flatContents), rectText(legacyContents));
            EXPECT_EQ(rectText(flatContents), rectText(QRect(0, -1, 120, 22)));

            // The hint follows the font, and a font is not what the carrier proves: the flat edit has the font of its twin. The Native
            // numbers (n3 Editor): the hint [106, 16] and the minimum hint [19, 16] in the legacy dump, [108, 16] and [21, 16] in the flat
            // one before
            flat->setFont(legacy->font());
            EXPECT_EQ(sizeText(flat->sizeHint()), sizeText(legacy->sizeHint()));
            EXPECT_EQ(sizeText(flat->minimumSizeHint()), sizeText(legacy->minimumSizeHint()));
            EXPECT_EQ(legacy->sizeHint().height(), 16);
        }
    } // namespace BreadCrumbsFlatSheetTestsInternal
} // namespace AzQtComponents
