/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/Elements/IndicatorElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QAbstractButton>
#include <QApplication>
#include <QCheckBox>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QPixmapCache>
#include <QRadioButton>
#include <QStyleFactory>
#include <QStyleOption>
#include <QSvgRenderer>
#include <QtMath>

#include <climits>
#include <cstdio>
#include <string>

namespace AzQtComponents
{
    namespace
    {
        //! The pixels outside of a rect that the draw touched: any but the magenta the image started from
        int indicatorTouchedOutside(const QImage& image, const QRect& inside)
        {
            int touched = 0;
            for (int y = 0; y < image.height(); ++y)
            {
                for (int x = 0; x < image.width(); ++x)
                {
                    if (!inside.contains(x, y) && image.pixelColor(x, y) != QColor(Qt::magenta))
                    {
                        ++touched;
                    }
                }
            }
            return touched;
        }

        //! The pixels inside a rect that the draw touched: any but the magenta the image started from
        int indicatorTouchedInside(const QImage& image, const QRect& inside)
        {
            int touched = 0;
            for (int y = 0; y < image.height(); ++y)
            {
                for (int x = 0; x < image.width(); ++x)
                {
                    if (inside.contains(x, y) && image.pixelColor(x, y) != QColor(Qt::magenta))
                    {
                        ++touched;
                    }
                }
            }
            return touched;
        }

        //! The images of the indicators are compiled into the library; a process that does not register them draws none
        bool indicatorImageRegistered(const char* path)
        {
            return QFile::exists(QString::fromLatin1(path));
        }

        //! The pixels outside of a rect that differ between two paints
        int indicatorDifferingOutside(const QImage& first, const QImage& second, const QRect& inside)
        {
            int differing = 0;
            for (int y = 0; y < first.height(); ++y)
            {
                for (int x = 0; x < first.width(); ++x)
                {
                    if (!inside.contains(x, y) && first.pixelColor(x, y) != second.pixelColor(x, y))
                    {
                        ++differing;
                    }
                }
            }
            return differing;
        }

        //! The first column from a given one on that holds a touched pixel, -1 when there is none
        int indicatorFirstInkColumn(const QImage& image, int fromColumn)
        {
            for (int x = fromColumn; x < image.width(); ++x)
            {
                for (int y = 0; y < image.height(); ++y)
                {
                    if (image.pixelColor(x, y) != QColor(Qt::magenta))
                    {
                        return x;
                    }
                }
            }
            return -1;
        }

        //! A rect as text, for the failure message of a comparison
        std::string indicatorRectText(const QRect& rect)
        {
            return QStringLiteral("(%1, %2, %3 x %4)").arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height()).toStdString();
        }

        //! A size as text, for the failure message of a comparison
        std::string indicatorSizeText(const QSize& size)
        {
            return QStringLiteral("(%1 x %2)").arg(size.width()).arg(size.height()).toStdString();
        }

        //! A transparent image of a logical size at a device ratio, as a high-DPI screen gives a painter
        QImage transparentIndicatorImage(const QSize& size, qreal ratio)
        {
            QImage image(size * ratio, QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(Qt::transparent);
            return image;
        }

        //! What an SVG of an indicator gave: the pipeline Style::drawSvg had before the toggle switch and the radio button were drawn from
        //! values. Rendered once into a pixmap of the rect's logical size at the device ratio (the logical bounds given to the renderer),
        //! then drawn in the rect.
        QImage indicatorSvgImage(const QString& resource, const QSize& size, qreal ratio)
        {
            QPixmap pixmap(size * ratio);
            pixmap.setDevicePixelRatio(ratio);
            pixmap.fill(Qt::transparent);
            QPainter pixmapPainter(&pixmap);
            QSvgRenderer renderer(resource);
            renderer.render(&pixmapPainter, QRectF(QPointF(0.0, 0.0), QSizeF(size)));
            pixmapPainter.end();

            QImage image = transparentIndicatorImage(size, ratio);
            QPainter painter(&image);
            painter.drawPixmap(QRect(QPoint(0, 0), size), pixmap);
            painter.end();
            return image;
        }

        //! The largest difference of one channel of one pixel between two images; INT_MAX where their sizes differ
        int indicatorMaxChannelDifference(const QImage& first, const QImage& second)
        {
            if (first.size() != second.size())
            {
                return INT_MAX;
            }
            int largest = 0;
            for (int y = 0; y < first.height(); ++y)
            {
                for (int x = 0; x < first.width(); ++x)
                {
                    const QRgb a = first.pixel(x, y);
                    const QRgb b = second.pixel(x, y);
                    largest = qMax(largest, qAbs(qAlpha(a) - qAlpha(b)));
                    largest = qMax(largest, qAbs(qRed(a) - qRed(b)));
                    largest = qMax(largest, qAbs(qGreen(a) - qGreen(b)));
                    largest = qMax(largest, qAbs(qBlue(a) - qBlue(b)));
                }
            }
            return largest;
        }

        //! The pixels of an image that are not transparent
        int indicatorOpaquePixels(const QImage& image)
        {
            int opaque = 0;
            for (int y = 0; y < image.height(); ++y)
            {
                for (int x = 0; x < image.width(); ++x)
                {
                    opaque += qAlpha(image.pixel(x, y)) > 0 ? 1 : 0;
                }
            }
            return opaque;
        }
    } // namespace

    // ----------------------------------------------------------------------------------
    // Fixture: the flattened style chain (Style -> Fusion) under an application
    // ----------------------------------------------------------------------------------

    class IndicatorElementTests
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        IndicatorElementTests()
        {
            m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);

            // As StyleManager::initialize builds it (StyleManager.cpp:244-246); the application owns it
            m_style = new Style(QStyleFactory::create(QStringLiteral("Fusion")));
            QApplication::setStyle(m_style);
        }

        ~IndicatorElementTests() override
        {
            m_application.reset();
        }

        //! The label spacing the style gives a button: the room between its box and its label
        int labelSpacing(const QAbstractButton* button) const
        {
            QStyleOptionButton option;
            option.initFrom(button);
            const bool radio = qobject_cast<const QRadioButton*>(button) != nullptr;
            return m_style->pixelMetric(radio ? QStyle::PM_RadioButtonLabelSpacing : QStyle::PM_CheckBoxLabelSpacing, &option, button);
        }

        //! A button painted through the style (CE_CheckBox, CE_RadioButton) over magenta, so that every pixel the
        //! draw touches shows
        QImage paintButton(const QAbstractButton* button, QStyle::State state, const QString& text, const QSize& size) const
        {
            QStyleOptionButton option;
            option.initFrom(button);
            option.rect = QRect(QPoint(0, 0), size);
            option.state = state | QStyle::State_Enabled;
            option.text = text;

            QImage image(size, QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::magenta);
            QPainter painter(&image);
            const bool radio = qobject_cast<const QRadioButton*>(button) != nullptr;
            m_style->drawControl(radio ? QStyle::CE_RadioButton : QStyle::CE_CheckBox, &option, &painter, button);
            painter.end();
            return image;
        }

        //! The indicator of a toggle switch (a check box with its class) or of a radio button, painted through the style as a button's
        //! draw does (PE_IndicatorCheckBox, PE_IndicatorRadioButton) over transparent, at a device ratio
        QImage paintIndicator(const QWidget* widget, QStyle::State state, const QSize& size, qreal ratio) const
        {
            QStyleOptionButton option;
            option.initFrom(widget);
            option.rect = QRect(QPoint(0, 0), size);
            option.state = state;

            QImage image = transparentIndicatorImage(size, ratio);
            QPainter painter(&image);
            const bool radio = qobject_cast<const QRadioButton*>(widget) != nullptr;
            m_style->drawPrimitive(radio ? QStyle::PE_IndicatorRadioButton : QStyle::PE_IndicatorCheckBox, &option, &painter, widget);
            painter.end();
            return image;
        }

    protected:
        int m_argc = 1;                                     // QApplication keeps a reference to it
        char m_programName[5] = "test";
        char* m_argv[2] = { m_programName, nullptr };
        AZStd::unique_ptr<QApplication> m_application;
        Style* m_style = nullptr;                           // owned by the application
    };

    // ----------------------------------------------------------------------------------
    // T-IND.1: which element a widget has, and the box of each (the indicator and its margin on every side)
    // ----------------------------------------------------------------------------------

    TEST_F(IndicatorElementTests, Resolve_EachClassOfButtonHasItsElement)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QCheckBox check;
        QRadioButton radio;
        QCheckBox toggle;
        CheckBox::applyToggleSwitchStyle(&toggle);

        const IndicatorElement* checkElement = IndicatorElement::forWidget(&check);
        const IndicatorElement* radioElement = IndicatorElement::forWidget(&radio);
        const IndicatorElement* toggleElement = IndicatorElement::forWidget(&toggle);
        ASSERT_TRUE(checkElement != nullptr);
        ASSERT_TRUE(radioElement != nullptr);
        ASSERT_TRUE(toggleElement != nullptr);
        EXPECT_TRUE(checkElement != radioElement);
        EXPECT_TRUE(checkElement != toggleElement);

        // CheckBox.qss:25-27 and RadioButton.qss:24-26: 16x16 with margin 2; CheckBox.qss:77-80: 32x16 with margin 2
        EXPECT_EQ(checkElement->boxSize(), QSize(20, 20)) << indicatorSizeText(checkElement->boxSize());
        EXPECT_EQ(radioElement->boxSize(), QSize(20, 20)) << indicatorSizeText(radioElement->boxSize());
        EXPECT_EQ(toggleElement->boxSize(), QSize(36, 20)) << indicatorSizeText(toggleElement->boxSize());
    }

    TEST_F(IndicatorElementTests, Resolve_ClassTagsChangeTheElementAndTheExpanderHasNone)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QCheckBox check;
        check.ensurePolished();
        const IndicatorElement* plain = IndicatorElement::forWidget(&check);
        ASSERT_TRUE(plain != nullptr);

        // Style::addClass polishes the widget again
        CheckBox::applyToggleSwitchStyle(&check);
        EXPECT_TRUE(IndicatorElement::forWidget(&check) != plain);
        EXPECT_EQ(IndicatorElement::forWidget(&check)->boxSize(), QSize(36, 20));

        // CheckBox.qss:121-139 and :127-130: the expander is a bare 16px caret
        QCheckBox expander;
        CheckBox::applyExpanderStyle(&expander);
        EXPECT_TRUE(IndicatorElement::forWidget(&expander) == nullptr);
        EXPECT_TRUE(expander.minimumSize() == QSize(16, 16));
        EXPECT_TRUE(expander.maximumSize() == QSize(16, 16));
        EXPECT_TRUE(expander.sizeHint() == QSize(16, 16));
    }

    // ----------------------------------------------------------------------------------
    // T-IND.2: where the image and the label go in the widget's rect (qstylesheetstyle.cpp:6184-6207)
    // ----------------------------------------------------------------------------------

    TEST_F(IndicatorElementTests, ImageRect_IsTheBoxLessItsMarginCenteredVertically)
    {
        QCheckBox check;
        const IndicatorElement* element = IndicatorElement::forWidget(&check);
        ASSERT_TRUE(element != nullptr);

        // 20 is the height of the box; 18 and 22 are the heights of other rows (the box is centered, and may overhang)
        const QRect twenty = element->imageRect(QRect(0, 0, 100, 20), false, Qt::LeftToRight);
        const QRect eighteen = element->imageRect(QRect(0, 0, 100, 18), false, Qt::LeftToRight);
        const QRect twentyTwo = element->imageRect(QRect(0, 0, 100, 22), false, Qt::LeftToRight);
        EXPECT_EQ(twenty, QRect(2, 2, 16, 16)) << indicatorRectText(twenty);
        EXPECT_EQ(eighteen, QRect(2, 1, 16, 16)) << indicatorRectText(eighteen);
        EXPECT_EQ(twentyTwo, QRect(2, 3, 16, 16)) << indicatorRectText(twentyTwo);

        // The origin of the widget's rect, and the mirrored layout
        const QRect moved = element->imageRect(QRect(10, 5, 100, 20), false, Qt::LeftToRight);
        const QRect mirrored = element->imageRect(QRect(0, 0, 100, 20), false, Qt::RightToLeft);
        EXPECT_EQ(moved, QRect(12, 7, 16, 16)) << indicatorRectText(moved);
        EXPECT_EQ(mirrored, QRect(82, 2, 16, 16)) << indicatorRectText(mirrored);
    }

    TEST_F(IndicatorElementTests, ImageRect_FocusChangesOnlyTheImageOfRadioButtonsAndToggleSwitches)
    {
        QCheckBox check;
        QRadioButton radio;
        QCheckBox toggle;
        CheckBox::applyToggleSwitchStyle(&toggle);
        const QRect rect(0, 0, 100, 20);

        // CheckBox.qss has no :focus size for the plain box
        const IndicatorElement* checkElement = IndicatorElement::forWidget(&check);
        ASSERT_TRUE(checkElement != nullptr);
        EXPECT_EQ(checkElement->imageRect(rect, true, Qt::LeftToRight), QRect(2, 2, 16, 16));

        // RadioButton.qss:31-33: 18x18 with margin 1, in the same 20px box
        const IndicatorElement* radioElement = IndicatorElement::forWidget(&radio);
        ASSERT_TRUE(radioElement != nullptr);
        EXPECT_EQ(radioElement->imageRect(rect, false, Qt::LeftToRight), QRect(2, 2, 16, 16));
        EXPECT_EQ(radioElement->imageRect(rect, true, Qt::LeftToRight), QRect(1, 1, 18, 18));

        // CheckBox.qss:95-98: 34x18 with margin 1, in the same 36x20 box
        const IndicatorElement* toggleElement = IndicatorElement::forWidget(&toggle);
        ASSERT_TRUE(toggleElement != nullptr);
        EXPECT_EQ(toggleElement->imageRect(rect, false, Qt::LeftToRight), QRect(2, 2, 32, 16));
        EXPECT_EQ(toggleElement->imageRect(rect, true, Qt::LeftToRight), QRect(1, 1, 34, 18));
    }

    TEST_F(IndicatorElementTests, ContentsRect_StartsAfterTheBoxAndTheSpacing)
    {
        QCheckBox check;
        QCheckBox toggle;
        CheckBox::applyToggleSwitchStyle(&toggle);
        const int spacing = labelSpacing(&check);

        const IndicatorElement* checkElement = IndicatorElement::forWidget(&check);
        const IndicatorElement* toggleElement = IndicatorElement::forWidget(&toggle);
        ASSERT_TRUE(checkElement != nullptr);
        ASSERT_TRUE(toggleElement != nullptr);

        const QRect rect(0, 0, 100, 20);
        const QRect plain = checkElement->contentsRect(rect, spacing, Qt::LeftToRight);
        const QRect toggled = toggleElement->contentsRect(rect, spacing, Qt::LeftToRight);
        EXPECT_EQ(plain, QRect(20 + spacing, 0, 80 - spacing, 20)) << indicatorRectText(plain);
        EXPECT_EQ(toggled, QRect(36 + spacing, 0, 64 - spacing, 20)) << indicatorRectText(toggled);

        // Mirrored, the label is on the left
        const QRect mirrored = checkElement->contentsRect(rect, spacing, Qt::RightToLeft);
        EXPECT_EQ(mirrored, QRect(0, 0, 80 - spacing, 20)) << indicatorRectText(mirrored);
    }

    // ----------------------------------------------------------------------------------
    // T-IND.3: the size of a button (qstylesheetstyle.cpp:5397-5411): the box, the spacing and the text
    // ----------------------------------------------------------------------------------

    TEST_F(IndicatorElementTests, SizeFromContents_AddsTheBoxAndTheSpacingAndKeepsTheBoxHeight)
    {
        QCheckBox check;
        QCheckBox toggle;
        CheckBox::applyToggleSwitchStyle(&toggle);
        const int spacing = labelSpacing(&check);

        const IndicatorElement* checkElement = IndicatorElement::forWidget(&check);
        const IndicatorElement* toggleElement = IndicatorElement::forWidget(&toggle);
        ASSERT_TRUE(checkElement != nullptr);
        ASSERT_TRUE(toggleElement != nullptr);

        EXPECT_EQ(checkElement->sizeFromContents(QSize(55, 17), spacing), QSize(55 + 20 + spacing, 20));
        EXPECT_EQ(checkElement->sizeFromContents(QSize(55, 24), spacing), QSize(55 + 20 + spacing, 24));
        EXPECT_EQ(toggleElement->sizeFromContents(QSize(55, 17), spacing), QSize(55 + 36 + spacing, 20));

        // The legacy sheet counted the box and the spacing even without a label
        EXPECT_EQ(checkElement->sizeFromContents(QSize(0, 0), spacing), QSize(20 + spacing, 20));
        EXPECT_EQ(toggleElement->sizeFromContents(QSize(0, 0), spacing), QSize(36 + spacing, 20));
    }

    TEST_F(IndicatorElementTests, SizeHint_OfAButtonIsTheTextPlusTheBoxAndTheSpacing)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QCheckBox check(QStringLiteral("Visible"));
        QRadioButton radio(QStringLiteral("Visible"));
        QCheckBox bare;

        // The hint first: it polishes the button, and so settles the font the text is measured in
        const QSize checkHint = check.sizeHint();
        const QSize radioHint = radio.sizeHint();
        const QSize bareHint = bare.sizeHint();

        const QSize checkText = m_style->itemTextRect(check.fontMetrics(), QRect(), Qt::TextShowMnemonic, false, check.text()).size();
        const QSize radioText = m_style->itemTextRect(radio.fontMetrics(), QRect(), Qt::TextShowMnemonic, false, radio.text()).size();
        const int checkSpacing = labelSpacing(&check);
        const int radioSpacing = labelSpacing(&radio);

        EXPECT_EQ(checkHint, QSize(checkText.width() + 20 + checkSpacing, qMax(checkText.height(), 20))) << indicatorSizeText(checkHint);
        EXPECT_EQ(radioHint, QSize(radioText.width() + 20 + radioSpacing, qMax(radioText.height(), 20))) << indicatorSizeText(radioHint);
        EXPECT_EQ(bareHint, QSize(20 + checkSpacing, 20)) << indicatorSizeText(bareHint);
    }

    // ----------------------------------------------------------------------------------
    // T-IND.4: the draw. The image lies in its rect, the label after the spacing, and no focus rectangle is drawn
    // (CheckBox.qss:15 and RadioButton.qss:14 `outline: 0`)
    // ----------------------------------------------------------------------------------

    TEST_F(IndicatorElementTests, Draw_PaintsNothingOutsideTheImageWithoutALabel)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QCheckBox check;
        QRadioButton radio;

        const QImage checkImage = paintButton(&check, QStyle::State_On, QString(), QSize(80, 20));
        EXPECT_EQ(indicatorTouchedOutside(checkImage, QRect(2, 2, 16, 16)), 0);

        const QImage radioImage = paintButton(&radio, QStyle::State_On, QString(), QSize(80, 20));
        EXPECT_EQ(indicatorTouchedOutside(radioImage, QRect(2, 2, 16, 16)), 0);

        // A toggle switch: 32x16 in its 36x20 box
        QCheckBox toggle;
        CheckBox::applyToggleSwitchStyle(&toggle);
        const QImage toggleImage = paintButton(&toggle, QStyle::State_On, QString(), QSize(80, 20));
        EXPECT_EQ(indicatorTouchedOutside(toggleImage, QRect(2, 2, 32, 16)), 0);

        // And the image is drawn, inside that rect: a draw that painted nothing would pass the checks above
        if (indicatorImageRegistered(":/stylesheet/img/UI20/checkbox/on.svg"))
        {
            EXPECT_GT(indicatorTouchedInside(checkImage, QRect(2, 2, 16, 16)), 0);
        }
        if (indicatorImageRegistered(":/stylesheet/img/UI20/radiobutton/checked.svg"))
        {
            EXPECT_GT(indicatorTouchedInside(radioImage, QRect(2, 2, 16, 16)), 0);
        }
        if (indicatorImageRegistered(":/stylesheet/img/UI20/toggleswitch/checked.svg"))
        {
            EXPECT_GT(indicatorTouchedInside(toggleImage, QRect(2, 2, 32, 16)), 0);
        }
    }

    TEST_F(IndicatorElementTests, Draw_LabelStartsAfterTheBoxAndTheSpacing)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QCheckBox check;
        const QImage image = paintButton(&check, QStyle::State_Off, QStringLiteral("Label"), QSize(120, 20));

        // Nothing is drawn between the image and the label: the first mark at or after the box is the label's, and
        // a glyph's own left bearing may only move it right (a pixel of slack for the antialiasing at its edge)
        const int ink = indicatorFirstInkColumn(image, 20);
        ASSERT_GE(ink, 0);
        EXPECT_GE(ink, 20 + labelSpacing(&check) - 1);
    }

    TEST_F(IndicatorElementTests, Draw_FocusChangesNothingOutsideTheImage)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // The focus variant of the image lies in the box, and no focus rectangle is drawn around the label: the
        // two paints agree wherever the larger of the two images is not
        QRadioButton radio;
        const QImage radioPlain = paintButton(&radio, QStyle::State_On, QStringLiteral("Label"), QSize(120, 20));
        const QImage radioFocused = paintButton(&radio, QStyle::State_On | QStyle::State_HasFocus, QStringLiteral("Label"), QSize(120, 20));
        EXPECT_EQ(indicatorDifferingOutside(radioPlain, radioFocused, QRect(1, 1, 18, 18)), 0);

        QCheckBox check;
        const QImage checkPlain = paintButton(&check, QStyle::State_On, QStringLiteral("Label"), QSize(120, 20));
        const QImage checkFocused = paintButton(&check, QStyle::State_On | QStyle::State_HasFocus, QStringLiteral("Label"), QSize(120, 20));
        EXPECT_EQ(indicatorDifferingOutside(checkPlain, checkFocused, QRect(2, 2, 16, 16)), 0);

        QCheckBox toggle;
        CheckBox::applyToggleSwitchStyle(&toggle);
        const QImage togglePlain = paintButton(&toggle, QStyle::State_On, QStringLiteral("Label"), QSize(120, 20));
        const QImage toggleFocused = paintButton(&toggle, QStyle::State_On | QStyle::State_HasFocus, QStringLiteral("Label"), QSize(120, 20));
        EXPECT_EQ(indicatorDifferingOutside(togglePlain, toggleFocused, QRect(1, 1, 34, 18)), 0);
    }

    TEST_F(IndicatorElementTests, Draw_IsRefusedWhereThereIsNoElement)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QCheckBox check;
        QCheckBox expander;
        CheckBox::applyExpanderStyle(&expander);

        QImage image(80, 20, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        QStyleOptionButton option;
        option.initFrom(&check);

        EXPECT_TRUE(IndicatorElement::draw(m_style, &option, &painter, &check));
        EXPECT_FALSE(IndicatorElement::draw(m_style, &option, &painter, &expander));
        EXPECT_FALSE(IndicatorElement::draw(m_style, nullptr, &painter, &check));
        EXPECT_FALSE(IndicatorElement::draw(m_style, &option, &painter, nullptr));
    }

    // ----------------------------------------------------------------------------------
    // T-IND.5: where a click toggles the button. A check box: QCommonStyle's click rect laid on the legacy box
    // (qstylesheetstyle.cpp:6222-6224); a radio button: the whole widget (qstylesheetstyle.cpp:6215-6220)
    // ----------------------------------------------------------------------------------

    TEST_F(IndicatorElementTests, ClickRect_IsTheBoxAndTheTextWithRoomAroundIt)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QCheckBox check;
        QStyleOptionButton option;
        option.initFrom(&check);
        option.rect = QRect(0, 0, 120, 20);
        option.state |= QStyle::State_Enabled;

        // Without a label, only the box
        const QRect bare = m_style->subElementRect(QStyle::SE_CheckBoxClickRect, &option, &check);
        EXPECT_EQ(bare, QRect(0, 0, 20, 20)) << indicatorRectText(bare);

        // With one, the box and the text laid after the spacing
        option.text = QStringLiteral("Label");
        const int spacing = labelSpacing(&check);
        const QRect click = m_style->subElementRect(QStyle::SE_CheckBoxClickRect, &option, &check);
        EXPECT_TRUE(click.contains(QPoint(10, 10))) << indicatorRectText(click);
        EXPECT_TRUE(click.contains(QPoint(20 + spacing + 3, 10))) << indicatorRectText(click);
        EXPECT_FALSE(click.contains(QPoint(118, 10))) << indicatorRectText(click);
    }

    TEST_F(IndicatorElementTests, ClickRect_OfARadioButtonIsTheWholeWidget)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QRadioButton radio;
        QStyleOptionButton option;
        option.initFrom(&radio);
        option.rect = QRect(0, 0, 120, 20);
        option.state |= QStyle::State_Enabled;

        // Without a label, as with one: the whole rectangle, not the box of the indicator
        const QRect bare = m_style->subElementRect(QStyle::SE_RadioButtonClickRect, &option, &radio);
        EXPECT_EQ(bare, QRect(0, 0, 120, 20)) << indicatorRectText(bare);

        option.text = QStringLiteral("Label");
        const QRect labeled = m_style->subElementRect(QStyle::SE_RadioButtonClickRect, &option, &radio);
        EXPECT_EQ(labeled, QRect(0, 0, 120, 20)) << indicatorRectText(labeled);

        // Whatever the height a layout gave the button, the rectangle is the one the style was asked about
        option.rect = QRect(10, 5, 90, 18);
        const QRect shifted = m_style->subElementRect(QStyle::SE_RadioButtonClickRect, &option, &radio);
        EXPECT_EQ(shifted, QRect(10, 5, 90, 18)) << indicatorRectText(shifted);
    }

    // ----------------------------------------------------------------------------------
    // T-IND.6: the element of a module (registerElement), and an indicator with nothing in its box (ProjectManager.qss:97-99
    // `#createAGemLHS QRadioButton::indicator { image: url(:/nothing.png) }`: the file is no resource, the icon has no pixmap, the
    // rule's box stays)
    // ----------------------------------------------------------------------------------

    namespace IndicatorElementHideTestData
    {
        //! The radio buttons of the wizard's steps: RadioButton.qss:24-33 over a rule whose image is nothing
        const IndicatorElement HiddenStepIndicator{
            .m_name = "Test.RadioButton.HiddenStep",
            .m_imageSize = QSize(16, 16),
            .m_margin = 2,
            .m_focusImageSize = QSize(18, 18),
            .m_focusMargin = 1,
            .m_hidden = true,
        };

        bool isHiddenStep(const QWidget* widget)
        {
            return widget->objectName() == QLatin1String("hiddenStep");
        }

        bool isLateStep(const QWidget* widget)
        {
            return widget->objectName() == QLatin1String("lateStep");
        }

        //! Registered once for the process, as a module registers its own
        void registerHiddenStep()
        {
            static bool registered = false;
            if (!registered)
            {
                registered = true;
                IndicatorElement::registerElement(&HiddenStepIndicator, &QRadioButton::staticMetaObject, &isHiddenStep);
            }
        }

        //! One radio button's indicator and label as IndicatorElement::draw paints them, over magenta
        QImage drawnIndicator(const Style* style, const QRadioButton* radio, QStyle::State state, const QString& text, const QSize& size, qreal ratio)
        {
            QImage image((QSizeF(size) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(Qt::magenta);
            QPainter painter(&image);
            QStyleOptionButton option;
            option.initFrom(radio);
            option.rect = QRect(QPoint(0, 0), size);
            option.state = state | QStyle::State_Enabled;
            option.text = text;
            EXPECT_TRUE(IndicatorElement::draw(style, &option, &painter, radio));
            painter.end();
            return image;
        }
    } // namespace IndicatorElementHideTestData

    TEST_F(IndicatorElementTests, Register_AModuleElementIsTheOneOfTheWidgetsItsMatcherAcceptsAndOfNoOther)
    {
        using namespace IndicatorElementHideTestData;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());
        registerHiddenStep();

        QRadioButton step;
        step.setObjectName(QStringLiteral("hiddenStep"));
        QRadioButton other;
        other.setObjectName(QStringLiteral("otherStep"));
        QCheckBox check;                                    // a check box is no radio button: the class of the registration
        check.setObjectName(QStringLiteral("hiddenStep"));

        EXPECT_TRUE(IndicatorElement::forWidget(&step) == &HiddenStepIndicator);
        EXPECT_TRUE(IndicatorElement::forWidget(&other) != &HiddenStepIndicator);
        ASSERT_TRUE(IndicatorElement::forWidget(&other) != nullptr);
        EXPECT_FALSE(IndicatorElement::forWidget(&other)->m_hidden);
        ASSERT_TRUE(IndicatorElement::forWidget(&check) != nullptr);
        EXPECT_FALSE(IndicatorElement::forWidget(&check)->m_hidden);

        // The box of the hidden indicator is the radio button's (RadioButton.qss:24-33): the same room, the same rects
        const IndicatorElement* global = IndicatorElement::forWidget(&other);
        EXPECT_EQ(HiddenStepIndicator.boxSize(), global->boxSize());
        for (const bool focused : { false, true })
        {
            EXPECT_EQ(HiddenStepIndicator.imageRect(QRect(0, 0, 230, 40), focused, Qt::LeftToRight), global->imageRect(QRect(0, 0, 230, 40), focused, Qt::LeftToRight));
        }
        EXPECT_EQ(HiddenStepIndicator.contentsRect(QRect(0, 0, 230, 40), 10, Qt::RightToLeft), global->contentsRect(QRect(0, 0, 230, 40), 10, Qt::RightToLeft));
    }

    TEST_F(IndicatorElementTests, Register_AButtonThatHadItsElementKeptIsResolvedAgainWhenAModuleRegistersLater)
    {
        using namespace IndicatorElementHideTestData;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QRadioButton late;
        late.setObjectName(QStringLiteral("lateStep"));
        static bool registered = false;
        if (!registered)
        {
            // The window of a module is built, sized and drawn before the module sets up its styles: the element it asked for is the Global one
            ASSERT_TRUE(IndicatorElement::forWidget(&late) != nullptr);
            EXPECT_FALSE(IndicatorElement::forWidget(&late)->m_hidden);
            registered = true;
            IndicatorElement::registerElement(&HiddenStepIndicator, &QRadioButton::staticMetaObject, &isLateStep);
        }
        EXPECT_TRUE(IndicatorElement::forWidget(&late) == &HiddenStepIndicator);
    }

    TEST_F(IndicatorElementTests, Draw_AHiddenIndicatorPaintsNothingInItsBoxAndTheLabelIsDrawnAsEver)
    {
        using namespace IndicatorElementHideTestData;
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());
        registerHiddenStep();

        QRadioButton step;
        step.setObjectName(QStringLiteral("hiddenStep"));
        QRadioButton plain;
        plain.setObjectName(QStringLiteral("plainStep"));
        plain.setFont(step.font());

        const QStyle::State states[] = { QStyle::State_On, QStyle::State_Off, QStyle::State_On | QStyle::State_HasFocus, QStyle::State_Off | QStyle::State_HasFocus };
        for (const qreal ratio : { 1.0, 1.5 })
        {
            for (const QStyle::State state : states)
            {
                SCOPED_TRACE(QStringLiteral("ratio %1 state %2").arg(ratio).arg(int(state)).toStdString());
                const QSize size(120, 40);
                const QImage hidden = drawnIndicator(m_style, &step, state, QStringLiteral("Step"), size, ratio);
                const QImage shown = drawnIndicator(m_style, &plain, state, QStringLiteral("Step"), size, ratio);
                ASSERT_EQ(hidden.size(), shown.size());

                // The box of both: x 0 to 20, centered in 40 (the label starts after it and the spacing, as the glyphs show)
                const QRect box(QPoint(0, 0), QSize(qCeil(20 * ratio), qCeil(40 * ratio)));
                EXPECT_EQ(indicatorTouchedInside(hidden, box), 0);
                EXPECT_GT(indicatorTouchedInside(shown, box), 0);
                EXPECT_EQ(indicatorDifferingOutside(hidden, shown, box), 0);

                // And the label is there: ink after the box
                EXPECT_GE(indicatorFirstInkColumn(hidden, box.width()), 0);
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // T-IND.7: the toggle switch and the radio button, drawn from values: the pixels of the SVGs they replaced
    // (img/UI20/toggleswitch/ and img/UI20/radiobutton/, still in the library as the twins)
    // ----------------------------------------------------------------------------------

    namespace
    {
        //! A state of an indicator and the SVG that gave it. The ring (focus) images are the larger of the two sizes of an indicator.
        struct IndicatorSvgCase
        {
            QStyle::State m_state;
            const char* m_image;
            bool m_ring;
        };

        const IndicatorSvgCase g_indicatorSvgCases[] = {
            { QStyle::State_Enabled | QStyle::State_Off, "unchecked", false },
            { QStyle::State_Enabled | QStyle::State_Off | QStyle::State_HasFocus, "unchecked-focus", true },
            { QStyle::State_Off, "unchecked-disabled", false },
            { QStyle::State_Enabled | QStyle::State_On, "checked", false },
            { QStyle::State_Enabled | QStyle::State_On | QStyle::State_HasFocus, "checked-focus", true },
            { QStyle::State_On, "checked-disabled", false },
            // A disabled widget has no focus ring
            { QStyle::State_Off | QStyle::State_HasFocus, "unchecked-disabled", false },
            { QStyle::State_On | QStyle::State_HasFocus, "checked-disabled", false },
        };

        //! One channel of one pixel of slack between a draw from values and the SVG renderer's: they place the same curves with the same
        //! pen, and the path numbers go through two parsers
        constexpr int IndicatorChannelSlack = 2;
    } // namespace

    TEST_F(IndicatorElementTests, Draw_ToggleSwitchFromValuesIsTheImageOfItsSvg)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());
        ASSERT_TRUE(indicatorImageRegistered(":/stylesheet/img/UI20/toggleswitch/checked.svg"));

        QCheckBox toggle;
        CheckBox::applyToggleSwitchStyle(&toggle);

        for (const qreal ratio : { 1.0, 1.5, 2.0 })
        {
            for (const IndicatorSvgCase& look : g_indicatorSvgCases)
            {
                const QSize size = look.m_ring ? QSize(34, 18) : QSize(32, 16);
                SCOPED_TRACE(QStringLiteral("%1 ratio %2").arg(QLatin1String(look.m_image)).arg(ratio).toStdString());

                const QImage drawn = paintIndicator(&toggle, look.m_state, size, ratio);
                const QImage svg = indicatorSvgImage(
                    QStringLiteral(":/stylesheet/img/UI20/toggleswitch/%1.svg").arg(QLatin1String(look.m_image)), size, ratio);
                ASSERT_GT(indicatorOpaquePixels(svg), 0);
                EXPECT_GT(indicatorOpaquePixels(drawn), 0);
                EXPECT_LE(indicatorMaxChannelDifference(drawn, svg), IndicatorChannelSlack);
            }
        }
    }

    TEST_F(IndicatorElementTests, Draw_RadioButtonFromValuesIsTheImageOfItsSvg)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());
        ASSERT_TRUE(indicatorImageRegistered(":/stylesheet/img/UI20/radiobutton/checked.svg"));

        QRadioButton radio;

        for (const qreal ratio : { 1.0, 1.5, 2.0 })
        {
            for (const IndicatorSvgCase& look : g_indicatorSvgCases)
            {
                const QSize size = look.m_ring ? QSize(18, 18) : QSize(16, 16);
                SCOPED_TRACE(QStringLiteral("%1 ratio %2").arg(QLatin1String(look.m_image)).arg(ratio).toStdString());

                const QImage drawn = paintIndicator(&radio, look.m_state, size, ratio);
                const QImage svg = indicatorSvgImage(
                    QStringLiteral(":/stylesheet/img/UI20/radiobutton/%1.svg").arg(QLatin1String(look.m_image)), size, ratio);
                ASSERT_GT(indicatorOpaquePixels(svg), 0);
                EXPECT_GT(indicatorOpaquePixels(drawn), 0);
                EXPECT_LE(indicatorMaxChannelDifference(drawn, svg), IndicatorChannelSlack);
            }
        }
    }

    TEST_F(IndicatorElementTests, Draw_IndicatorsFromValuesScaleToTheirRectAsTheSvgsDid)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());
        ASSERT_TRUE(indicatorImageRegistered(":/stylesheet/img/UI20/toggleswitch/checked.svg"));

        QCheckBox toggle;
        CheckBox::applyToggleSwitchStyle(&toggle);
        QRadioButton radio;

        // A module's rule may give an indicator another size than the SVG's own: the view is scaled to the rect
        for (const qreal ratio : { 1.0, 2.0 })
        {
            for (const IndicatorSvgCase& look : g_indicatorSvgCases)
            {
                SCOPED_TRACE(QStringLiteral("%1 ratio %2").arg(QLatin1String(look.m_image)).arg(ratio).toStdString());

                const QSize toggleSize = look.m_ring ? QSize(51, 27) : QSize(48, 24);
                EXPECT_LE(indicatorMaxChannelDifference(paintIndicator(&toggle, look.m_state, toggleSize, ratio),
                              indicatorSvgImage(QStringLiteral(":/stylesheet/img/UI20/toggleswitch/%1.svg").arg(QLatin1String(look.m_image)),
                                  toggleSize, ratio)),
                    IndicatorChannelSlack);

                const QSize radioSize = look.m_ring ? QSize(27, 27) : QSize(24, 24);
                EXPECT_LE(indicatorMaxChannelDifference(paintIndicator(&radio, look.m_state, radioSize, ratio),
                              indicatorSvgImage(QStringLiteral(":/stylesheet/img/UI20/radiobutton/%1.svg").arg(QLatin1String(look.m_image)),
                                  radioSize, ratio)),
                    IndicatorChannelSlack);
            }
        }
    }

    TEST_F(IndicatorElementTests, Draw_IndicatorsFromValuesShareTheirColours)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QCheckBox toggle;
        CheckBox::applyToggleSwitchStyle(&toggle);
        QRadioButton radio;

        // The body of an indicator that is on, and its mark, are one colour in both: the blue, and white (disabled: the dark blue, and
        // light grey). The corner pixel of a switch is outside its track, so the sample is its knob and the middle of the radio's dot.
        const QImage toggleOn = paintIndicator(&toggle, QStyle::State_Enabled | QStyle::State_On, QSize(32, 16), 1.0);
        const QImage radioOn = paintIndicator(&radio, QStyle::State_Enabled | QStyle::State_On, QSize(16, 16), 1.0);
        EXPECT_EQ(toggleOn.pixelColor(24, 8), QColor(0xFF, 0xFF, 0xFF));     // the knob
        EXPECT_EQ(radioOn.pixelColor(8, 8), QColor(0xFF, 0xFF, 0xFF));       // the dot
        EXPECT_EQ(toggleOn.pixelColor(4, 8), QColor(0x1E, 0x70, 0xEB));      // the track
        EXPECT_EQ(radioOn.pixelColor(2, 8), QColor(0x1E, 0x70, 0xEB));       // the disc

        const QImage toggleDisabled = paintIndicator(&toggle, QStyle::State_On, QSize(32, 16), 1.0);
        const QImage radioDisabled = paintIndicator(&radio, QStyle::State_On, QSize(16, 16), 1.0);
        EXPECT_EQ(toggleDisabled.pixelColor(24, 8), QColor(0xBB, 0xBB, 0xBB));
        EXPECT_EQ(radioDisabled.pixelColor(8, 8), QColor(0xBB, 0xBB, 0xBB));
        EXPECT_EQ(toggleDisabled.pixelColor(4, 8), QColor(0x2A, 0x48, 0x75));
        EXPECT_EQ(radioDisabled.pixelColor(2, 8), QColor(0x2A, 0x48, 0x75));
    }

    // ----------------------------------------------------------------------------------
    // Timing, run on demand (--gtest_also_run_disabled_tests): the cost of drawing a toggle switch from values against the SVG's.
    // Warm is a draw that finds its pixmap in the cache; cold renders it.
    // ----------------------------------------------------------------------------------

    TEST_F(IndicatorElementTests, DISABLED_Perf_ToggleSwitchFromValuesAgainstSvg)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());
        ASSERT_TRUE(indicatorImageRegistered(":/stylesheet/img/UI20/toggleswitch/checked.svg"));

        QCheckBox toggle;
        CheckBox::applyToggleSwitchStyle(&toggle);
        QCheckBox check;                                // still drawn from its SVG through the same cache
        const QSize size(32, 16);
        const QStyle::State state = QStyle::State_Enabled | QStyle::State_On;

        // Warm: the cache holds the pixmap
        constexpr int warmDraws = 200000;
        QStyleOptionButton option;
        option.initFrom(&toggle);
        option.rect = QRect(QPoint(0, 0), size);
        option.state = state;
        QImage target = transparentIndicatorImage(size, 1.0);
        QPainter painter(&target);

        m_style->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, &painter, &toggle);
        m_style->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, &painter, &check);
        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < warmDraws; ++i)
        {
            m_style->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, &painter, &toggle);
        }
        const double toggleWarm = timer.nsecsElapsed() / 1000.0 / warmDraws;
        timer.restart();
        for (int i = 0; i < warmDraws; ++i)
        {
            m_style->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, &painter, &check);
        }
        const double svgWarm = timer.nsecsElapsed() / 1000.0 / warmDraws;
        painter.end();

        // Cold: each draw renders (the cache cleared before it; the clear is in both timings)
        constexpr int coldDraws = 2000;
        timer.start();
        for (int i = 0; i < coldDraws; ++i)
        {
            QPixmapCache::clear();
            paintIndicator(&toggle, state, size, 1.0);
        }
        const double toggleCold = timer.nsecsElapsed() / 1000.0 / coldDraws;
        timer.restart();
        for (int i = 0; i < coldDraws; ++i)
        {
            QPixmapCache::clear();
            paintIndicator(&check, state, size, 1.0);
        }
        const double checkSvgCold = timer.nsecsElapsed() / 1000.0 / coldDraws;
        timer.restart();
        for (int i = 0; i < coldDraws; ++i)
        {
            QPixmapCache::clear();
            indicatorSvgImage(QStringLiteral(":/stylesheet/img/UI20/toggleswitch/checked.svg"), size, 1.0);
        }
        const double toggleSvgCold = timer.nsecsElapsed() / 1000.0 / coldDraws;

        std::printf("[ PERF ] toggle switch warm: %.3f us a draw from values, %.3f us a draw from an SVG (the check box)\n", toggleWarm, svgWarm);
        std::printf("[ PERF ] toggle switch cold: %.2f us from values, %.2f us the SVG's own render, %.2f us a check box from its SVG\n",
            toggleCold, toggleSvgCold, checkSvgCold);
        std::fflush(stdout);
    }
} // namespace AzQtComponents
