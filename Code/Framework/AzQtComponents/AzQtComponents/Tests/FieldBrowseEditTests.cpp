/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/FieldTwins.h>

#include <AzQtComponents/Components/Widgets/BrowseEdit.h>

#include <QDir>
#include <QLineEdit>
#include <QPushButton>
#include <QStringList>
#include <QVBoxLayout>

#include <string>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // A BrowseEdit (BrowseEdit.qss, LineEdit.qss:11-20, BaseStyleSheet.qss:68-84): the flattened Style against the legacy sheet. The
    // sheet gave the line edit inside it a field of its own: a box with no border, padded 2px on the left and -4px on the right (a
    // line less above and a line more below, LineEdit.qss:18-19), its two left corners rounded by 2px, filled with the colour of the
    // state (#CCCCCC, #666666 when disabled, #FFFFFF with the focus or as an invalid drop target, none as a valid drop target).
    // Where the legacy GUI drew the frame of a drop target the sheet had handed the style the rect inside the 2px margin of the BrowseEdit
    // (qstylesheetstyle.cpp:4502-4511): there are no sheets to hand it in the flattened GUI, so the style carries the margin itself.
    // ----------------------------------------------------------------------------------

    namespace FieldBrowseEditTestsInternal
    {
        using namespace FieldTwins;

        class FieldBrowseEditTests
            : public Fixture
        {
        };

        struct Row
        {
            const char* m_name;
            const char* m_text;
            bool m_enabled;
            const char* m_dropClass;            //!< ValidDropTarget, InvalidDropTarget or none
        };

        const Row g_rows[] = {
            { "plain", "Some text to browse", true, nullptr },
            { "empty", "", true, nullptr },
            { "disabled", "Disabled text", false, nullptr },
            { "valid", "", true, ValidDropTarget },
            { "invalid", "", true, InvalidDropTarget },
        };

        //! One BrowseEdit for each row. The constructor of this process builds the flattened way (StyleManager::stylesheetsDisabled()):
        //! the legacy twin is put back to the line edit the constructor builds when sheets are on, whose padding is the sheet's
        void buildBrowseEdits(QWidget& root, bool legacy)
        {
            auto* layout = new QVBoxLayout(&root);
            for (const Row& row : g_rows)
            {
                auto* browseEdit = new BrowseEdit(&root);
                browseEdit->setObjectName(QString::fromLatin1(row.m_name));
                browseEdit->setText(QString::fromLatin1(row.m_text));
                browseEdit->setEnabled(row.m_enabled);
                if (row.m_dropClass)
                {
                    Style::addClass(browseEdit, row.m_dropClass);
                }
                if (legacy)
                {
                    browseEdit->lineEdit()->setTextMargins(0, 0, 0, 0);
                    browseEdit->lineEdit()->setFrame(true);
                }
                browseEdit->setFixedWidth(280);         // the margins of the root's layout are the style's, not the BrowseEdit's
                layout->addWidget(browseEdit);
            }
            layout->addStretch();
        }

        //! Where the text cursor is, in the coordinates of the line edit (QLineEdit::cursorRect is protected; this is what it answers an input
        //! method)
        QRect cursorRectOf(const QLineEdit* lineEdit)
        {
            return lineEdit->inputMethodQuery(Qt::ImCursorRectangle).toRect();
        }

        //! The panel of a line edit as QLineEdit::paintEvent draws it (QLineEdit::initStyleOption), in a state added to the widget's own
        QImage paintField(const QLineEdit* lineEdit, qreal ratio, QStyle::State extraState)
        {
            return paintThroughStyle(lineEdit, ratio, [lineEdit, extraState](QPainter& painter)
            {
                QStyleOptionFrame option;
                option.initFrom(lineEdit);
                option.rect = lineEdit->rect();
                option.lineWidth = lineEdit->hasFrame() ? lineEdit->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &option, lineEdit) : 0;
                option.midLineWidth = 0;
                option.state |= QStyle::State_Sunken | extraState;
                if (lineEdit->isReadOnly())
                {
                    option.state |= QStyle::State_ReadOnly;
                }
                option.features = QStyleOptionFrame::None;
                lineEdit->style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, &painter, lineEdit);
            });
        }

        // ----------------------------------------------------------------------------------
        // T-BE.1: the size and the rects of the BrowseEdit, of the line edit inside it and of its button
        // ----------------------------------------------------------------------------------

        TEST_F(FieldBrowseEditTests, Rects_AreTheSheetsInEveryState)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildBrowseEdits, QSize(300, 200));

            for (const Row& row : g_rows)
            {
                const auto* legacy = twins.legacyChild<BrowseEdit>(row.m_name);
                const auto* flat = twins.flatChild<BrowseEdit>(row.m_name);
                ASSERT_TRUE(legacy != nullptr && flat != nullptr) << row.m_name;

                EXPECT_TRUE(flat->sizeHint() == legacy->sizeHint()) << row.m_name << ": " << sizeText(flat->sizeHint()) << " against " << sizeText(legacy->sizeHint());
                EXPECT_TRUE(flat->size() == legacy->size()) << row.m_name << ": " << sizeText(flat->size()) << " against " << sizeText(legacy->size());
                EXPECT_TRUE(flat->lineEdit()->sizeHint() == legacy->lineEdit()->sizeHint())
                    << row.m_name << ": " << sizeText(flat->lineEdit()->sizeHint()) << " against " << sizeText(legacy->lineEdit()->sizeHint());
                EXPECT_TRUE(flat->lineEdit()->geometry() == legacy->lineEdit()->geometry())
                    << row.m_name << ": " << rectText(flat->lineEdit()->geometry()) << " against " << rectText(legacy->lineEdit()->geometry());

                const auto* legacyButton = legacy->findChild<QPushButton*>();
                const auto* flatButton = flat->findChild<QPushButton*>();
                ASSERT_TRUE(legacyButton != nullptr && flatButton != nullptr) << row.m_name;
                EXPECT_TRUE(flatButton->geometry() == legacyButton->geometry())
                    << row.m_name << ": " << rectText(flatButton->geometry()) << " against " << rectText(legacyButton->geometry());
            }

            // BrowseEdit.qss:12 margin 2px, then the field 16 high (:17): 20 high, the line edit at (0, 2)
            const auto* plain = twins.flatChild<BrowseEdit>("plain");
            EXPECT_EQ(plain->height(), 20);
            EXPECT_EQ(plain->lineEdit()->y(), 2);
            EXPECT_EQ(plain->lineEdit()->height(), 16);
        }

        // ----------------------------------------------------------------------------------
        // T-BE.2: the text starts where the padding of the sheet's rule put it
        // ----------------------------------------------------------------------------------

        TEST_F(FieldBrowseEditTests, Text_StartsWhereTheSheetPutsIt)
        {
            Twins twins(*this, buildBrowseEdits, QSize(300, 200));

            for (const Row& row : g_rows)
            {
                auto* legacy = twins.legacyChild<BrowseEdit>(row.m_name)->lineEdit();
                auto* flat = twins.flatChild<BrowseEdit>(row.m_name)->lineEdit();
                legacy->setText(QStringLiteral("Browse"));
                flat->setText(QStringLiteral("Browse"));
                for (const int position : { 0, 3, 6 })
                {
                    legacy->setCursorPosition(position);
                    flat->setCursorPosition(position);
                    EXPECT_TRUE(cursorRectOf(flat) == cursorRectOf(legacy))
                        << row.m_name << " at " << position << ": " << rectText(cursorRectOf(flat)) << " against " << rectText(cursorRectOf(legacy));
                }
            }

            // BrowseEdit.qss:26 padding-left 2px: the text is 2px further in than it would be in a line edit that has no padding
            QLineEdit bare;
            bare.setFrame(false);
            bare.setFixedSize(100, 16);
            bare.setText(QStringLiteral("Browse"));
            bare.setCursorPosition(0);
            auto* flat = twins.flatChild<BrowseEdit>("plain")->lineEdit();
            flat->setCursorPosition(0);
            EXPECT_EQ(cursorRectOf(flat).x(), cursorRectOf(&bare).x() + 2);
        }

        // ----------------------------------------------------------------------------------
        // T-BE.3: the field is painted as the sheet's rule painted it, in every state and at the ratios of the captures: the left
        // corners rounded by 2px and antialiased, the right ones square
        // ----------------------------------------------------------------------------------

        TEST_F(FieldBrowseEditTests, Field_IsPaintedAsTheSheetPaintedIt)
        {
            Twins twins(*this, buildBrowseEdits, QSize(300, 200));

            const QStyle::State states[] = { QStyle::State(), QStyle::State(QStyle::State_MouseOver), QStyle::State(QStyle::State_HasFocus),
                                             QStyle::State(QStyle::State_MouseOver | QStyle::State_HasFocus) };
            for (const Row& row : g_rows)
            {
                const QLineEdit* legacy = twins.legacyChild<BrowseEdit>(row.m_name)->lineEdit();
                const QLineEdit* flat = twins.flatChild<BrowseEdit>(row.m_name)->lineEdit();
                for (const qreal ratio : { 1.0, 1.5, 2.0 })
                {
                    for (const QStyle::State state : states)
                    {
                        // a disabled widget has neither the pointer nor the focus
                        if (!row.m_enabled && state != QStyle::State())
                        {
                            continue;
                        }
                        const QImage expected = paintField(legacy, ratio, state);
                        const QImage actual = paintField(flat, ratio, state);
                        if (qstrcmp(row.m_name, "valid") != 0)      // a valid drop target has no field of its own: the frame shows through
                        {
                            EXPECT_TRUE(isDrawnOn(expected)) << row.m_name << " ratio " << ratio << " state " << int(state);
                        }
                        EXPECT_EQ(differing(actual, expected), 0)
                            << row.m_name << " ratio " << ratio << " state " << int(state) << ": " << firstDifference(actual, expected);
                    }
                }
            }
        }

        TEST_F(FieldBrowseEditTests, Field_IsTheColourOfItsState)
        {
            Twins twins(*this, buildBrowseEdits, QSize(300, 200));

            const auto colorAt = [&](const char* name, QStyle::State state)
            {
                const QLineEdit* lineEdit = twins.flatChild<BrowseEdit>(name)->lineEdit();
                return paintField(lineEdit, 1.0, state).pixelColor(lineEdit->width() / 2, lineEdit->height() / 2);
            };
            const QColor grey(0x80, 0x80, 0x80);

            EXPECT_EQ(colorAt("plain", QStyle::State()), QColor(0xCC, 0xCC, 0xCC));                         // BaseStyleSheet.qss:76
            EXPECT_EQ(colorAt("plain", QStyle::State_HasFocus), QColor(0xFF, 0xFF, 0xFF));                  // BrowseEdit.qss:39-41
            EXPECT_EQ(colorAt("disabled", QStyle::State()), QColor(0x66, 0x66, 0x66));                      // LineEdit.qss:33
            EXPECT_EQ(colorAt("invalid", QStyle::State()), QColor(0xFF, 0xFF, 0xFF));                       // BrowseEdit.qss:38-41
            EXPECT_EQ(colorAt("valid", QStyle::State()), grey);                                             // BrowseEdit.qss:31-35: none
            EXPECT_EQ(colorAt("valid", QStyle::State_HasFocus), grey);                                      // BrowseEdit.qss:32, more specific
        }

        // ----------------------------------------------------------------------------------
        // T-BE.4: the frame of a valid drop target is the pixels of the Native capture. The legacy twin cannot hold it: the frame was
        // Style's own, drawn inside the margin the sheet style handed it. The pixels are those of n2 Gallery 02_browse-edit: the
        // BrowseEdit of the valid drop target row at window (715, 582), 874 x 20 at the ratio 1.5, so that its upper left corner is
        // at (1072.5, 873) in device pixels, on a #444444 page. The block is the 14 x 14 pixels from (1072, 873); the column the
        // 35 pixels at x = 1072 + 300 from y = 871. The image here puts the corner at (1.5, 3) instead: the same sub-pixel phase.
        // ----------------------------------------------------------------------------------

        // One row of the block per line, the colours as RRGGBB
        const char* const g_dropFrameCorner[14] = {
            "444444 444444 444444 444444 444444 444444 444444 444444 444444 444444 444444 444444 444444 444444",
            "444444 444444 444444 444444 494C4F 5C6F7C 6A879D 6C8BA2 6C8BA2 6C8BA2 6C8BA2 6C8BA2 6C8BA2 6C8BA2",
            "444444 444444 444444 5C6E7C 8DC5EE 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF",
            "444444 444444 525C64 92CFFB 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF",
            "444444 444444 7BA5C4 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF",
            "444444 444444 8CC4ED 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF",
            "444444 444444 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 99D4FF AEDDFF AFDDFF AFDDFF AFDDFF AFDDFF",
            "444444 444444 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF A5D9FF F5FBFF FFFFFF FFFFFF FFFFFF FFFFFF FFFFFF",
            "444444 444444 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF DBF0FF F3FAFF B1DEFF AFDDFF AFDDFF AFDDFF AFDDFF",
            "444444 444444 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF E5F4FF E5F4FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF",
            "444444 444444 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF E5F4FF E5F4FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF",
            "444444 444444 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF E5F4FF E5F4FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF",
            "444444 444444 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF E5F4FF E5F4FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF",
            "444444 444444 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF E5F4FF E5F4FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF",
        };

        // From y = -2 to y = 32 relative to the upper edge of the widget
        const char* const g_dropFrameColumn =
            "444444 444444 444444 6C8BA2 94D2FF 94D2FF 94D2FF 94D2FF AFDDFF FFFFFF AFDDFF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF "
            "94D2FF 94D2FF 94D2FF 94D2FF 94D2FF 94D2FF AFDDFF FFFFFF AFDDFF 94D2FF 94D2FF 94D2FF 94D2FF 6C8BA2 444444 444444 444444 444444";

        QColor colorOf(const QString& text)
        {
            return QColor(QStringLiteral("#") + text);
        }

        TEST_F(FieldBrowseEditTests, DropFrame_OfAValidDropTargetIsThePixelsOfNative)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            QWidget root;
            auto* browseEdit = new BrowseEdit(&root);
            Style::addClass(browseEdit, ValidDropTarget);
            browseEdit->setGeometry(0, 0, 874, 20);
            flat(root);
            show(root, QSize(874, 20));
            ASSERT_EQ(browseEdit->size(), QSize(874, 20));

            // The frame as QFrame::paintEvent draws it, at the ratio of the capture, on the page's colour
            QImage image(QSize(880, 30) * 3 / 2, QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(1.5);
            image.fill(QColor(0x44, 0x44, 0x44));
            {
                QPainter painter(&image);
                painter.translate(1.0, 2.0);        // device (1.5, 3)
                const QStyleOptionFrame option = frameOption(browseEdit);
                browseEdit->style()->drawControl(QStyle::CE_ShapedFrame, &option, &painter, browseEdit);
            }

            // The pixel of the capture at (1072 + x, 873 + y) is the one here at (x + 1, y + 3) (the corner is at 1.5, 3, not at 1072.5, 873)
            const int originX = 1;
            const int originY = 3;
            int wrong = 0;
            std::string firstWrong;
            const auto expect = [&](int x, int y, const QString& text)
            {
                const QColor actual = image.pixelColor(originX + x, originY + y);
                if (actual != colorOf(text))
                {
                    if (wrong++ == 0)
                    {
                        firstWrong = QStringLiteral("(%1, %2): %3 against %4").arg(x).arg(y).arg(actual.name(), colorOf(text).name()).toStdString();
                    }
                }
            };
            for (int y = 0; y < 14; ++y)
            {
                const QStringList row = QString::fromLatin1(g_dropFrameCorner[y]).split(QLatin1Char(' '));
                ASSERT_EQ(row.size(), 14);
                for (int x = 0; x < 14; ++x)
                {
                    expect(x, y, row[x]);
                }
            }
            const QStringList column = QString::fromLatin1(g_dropFrameColumn).split(QLatin1Char(' '));
            ASSERT_EQ(column.size(), 35);
            for (int y = 0; y < 35; ++y)
            {
                expect(300, y - 2, column[y]);
            }
            EXPECT_EQ(wrong, 0) << "first: " << firstWrong;
        }
    } // namespace FieldBrowseEditTestsInternal
} // namespace AzQtComponents
