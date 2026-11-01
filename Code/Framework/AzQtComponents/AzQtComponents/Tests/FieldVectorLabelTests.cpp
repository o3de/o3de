/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/FieldTwins.h>

#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <AzQtComponents/Components/Widgets/VectorInput.h>

#include <QLabel>
#include <QVBoxLayout>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The coordinate label of a vector element (VectorInput.qss:9-16, :37-55): the sheet gave the label a margin of 2px above and below
    // and filled what is inside it with the colour of the coordinate, the two left corners rounded by 2px. The flattened Style filled
    // the whole rect of the label, 4px taller than the number field it sits by (the X, Y, Z and W of the Gallery's Spin Box page and of
    // every vector row of the property editors).
    // ----------------------------------------------------------------------------------

    namespace FieldVectorLabelTestsInternal
    {
        using namespace FieldTwins;

        class FieldVectorLabelTests
            : public Fixture
        {
        };

        struct Coordinate
        {
            const char* m_name;
            const char* m_label;
            VectorElement::Coordinate m_coordinate;
            QRgb m_color;                       //!< VectorInput.qss:37-55
        };

        const Coordinate g_coordinates[] = {
            { "x", "X", VectorElement::Coordinate::X, 0xFFE25243 },
            { "y", "Y", VectorElement::Coordinate::Y, 0xFF58BC61 },
            { "z", "Z", VectorElement::Coordinate::Z, 0xFF4285F4 },
            { "w", "W", VectorElement::Coordinate::W, 0xFFE57829 },
        };

        //! One vector element for each coordinate, and one with a label of its own and no coordinate (no rule fills it)
        void buildVectors(QWidget& root, bool legacy)
        {
            Q_UNUSED(legacy);
            auto* layout = new QVBoxLayout(&root);
            for (const Coordinate& entry : g_coordinates)
            {
                auto* element = new VectorElement(&root);
                element->setObjectName(QString::fromLatin1(entry.m_name));
                element->setLabel(QString::fromLatin1(entry.m_label));
                element->setCoordinate(entry.m_coordinate);
                layout->addWidget(element);
            }
            auto* custom = new VectorElement(&root);
            custom->setObjectName(QStringLiteral("custom"));
            custom->setLabel(QStringLiteral("Custom"));
            layout->addWidget(custom);
            layout->addStretch();
        }

        //! A label as QFrame::paintEvent draws its frame: CE_ShapedFrame through the style of the label
        QImage paintLabelFrame(const QLabel* label, qreal ratio)
        {
            return paintThroughStyle(label, ratio, [label](QPainter& painter)
            {
                const QStyleOptionFrame option = frameOption(label);
                label->style()->drawControl(QStyle::CE_ShapedFrame, &option, &painter, label);
            });
        }

        // ----------------------------------------------------------------------------------
        // T-VL.1: the pixels of the label's frame are the sheet's at every ratio
        // ----------------------------------------------------------------------------------

        TEST_F(FieldVectorLabelTests, Fill_IsTheSheetsAtEveryRatio)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildVectors, QSize(240, 200));

            for (const char* name : { "x", "y", "z", "w", "custom" })
            {
                const auto* legacy = twins.legacyChild<VectorElement>(name);
                const auto* flat = twins.flatChild<VectorElement>(name);
                ASSERT_TRUE(legacy != nullptr && flat != nullptr) << name;
                const QLabel* legacyLabel = legacy->getLabelWidget();
                const QLabel* flatLabel = flat->getLabelWidget();

                EXPECT_TRUE(flatLabel->geometry() == legacyLabel->geometry())
                    << name << ": " << rectText(flatLabel->geometry()) << " against " << rectText(legacyLabel->geometry());
                for (const qreal ratio : { 1.0, 1.5, 2.0 })
                {
                    const QImage expected = paintLabelFrame(legacyLabel, ratio);
                    const QImage actual = paintLabelFrame(flatLabel, ratio);
                    if (qstrcmp(name, "custom") != 0)           // no coordinate, no fill
                    {
                        EXPECT_TRUE(isDrawnOn(expected)) << name << " ratio " << ratio;
                    }
                    EXPECT_EQ(differing(actual, expected), 0) << name << " ratio " << ratio << ": " << firstDifference(actual, expected);
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // T-VL.2: the fill is 4px shorter than the label (the number field's own height), square at the right and rounded at the left
        // ----------------------------------------------------------------------------------

        TEST_F(FieldVectorLabelTests, Fill_LiesInsideTheMarginOfTwoPixelsAboveAndBelow)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildVectors, QSize(240, 200));

            for (const Coordinate& entry : g_coordinates)
            {
                const auto* flat = twins.flatChild<VectorElement>(entry.m_name);
                ASSERT_TRUE(flat != nullptr) << entry.m_name;
                const QLabel* label = flat->getLabelWidget();
                const QImage image = paintLabelFrame(label, 1.0);
                const int middle = label->width() / 2;
                const QColor grey(0x80, 0x80, 0x80);

                // the rows of the label: two of the window grey, the fill, two of the window grey
                for (int y = 0; y < label->height(); ++y)
                {
                    const bool filled = y >= 2 && y < label->height() - 2;
                    EXPECT_EQ(image.pixelColor(middle, y), filled ? QColor(entry.m_color) : grey) << entry.m_name << " row " << y;
                }

                // the right corners are square (the rule rounds the left ones), the left ones are blends of the fill and the grey
                EXPECT_EQ(image.pixelColor(label->width() - 1, 2), QColor(entry.m_color)) << entry.m_name;
                EXPECT_EQ(image.pixelColor(label->width() - 1, label->height() - 3), QColor(entry.m_color)) << entry.m_name;
                EXPECT_NE(image.pixelColor(0, 2), QColor(entry.m_color)) << entry.m_name;
                EXPECT_NE(image.pixelColor(0, 2), grey) << entry.m_name;
                EXPECT_NE(image.pixelColor(0, label->height() - 3), QColor(entry.m_color)) << entry.m_name;
                EXPECT_NE(image.pixelColor(0, label->height() - 3), grey) << entry.m_name;
            }
        }

        TEST_F(FieldVectorLabelTests, Fill_OfALabelWithNoCoordinateIsNone)
        {
            Twins twins(*this, buildVectors, QSize(240, 200));

            const auto* flat = twins.flatChild<VectorElement>("custom");
            ASSERT_TRUE(flat != nullptr);
            const QImage image = paintLabelFrame(flat->getLabelWidget(), 1.0);
            EXPECT_EQ(differing(image, paintThroughStyle(flat->getLabelWidget(), 1.0, [](QPainter&) {})), 0);
        }
    } // namespace FieldVectorLabelTestsInternal
} // namespace AzQtComponents
