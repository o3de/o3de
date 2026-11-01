/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Inspector/InspectorGroupHeaderWidget.h>
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QApplication>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QStyleFactory>
#include <QStyleOption>
#include <QWidget>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The element of the inspector's group header: it is found on the header and on nothing else, and it paints the box the
    // legacy sheets gave the header: the margin of every label, then the header's own fill and lines. The expected pixels are
    // those of the legacy capture (UiCaptures n2, MaterialCanvas, a group header at logical y 94, ratio 1.5), read down a column.
    // ----------------------------------------------------------------------------------

    namespace InspectorGroupHeaderWidgetTestsInternal
    {
        constexpr QRgb ParentColor = 0x444444;          // what the headers sit on in the legacy capture
        constexpr const char* GroupHeaderElementName = "AtomTools.Box.InspectorGroupHeader";

        // The sheet the header sets in its constructor in the legacy GUI (InspectorGroupHeaderWidget.cpp:74), and the margin of
        // every label (Text.qss:89-95)
        constexpr const char* HeaderSheet =
            "background-color: #333333; border-style: solid; border-color: #1B1B1B; border-width: 1px; border-left: none; border-right: none;";
        constexpr const char* LabelRules = "QLabel { margin-top: 1px; margin-bottom: 1px; }";

        class InspectorGroupHeaderWidgetTests
            : public LeakDetectionFixture
        {
        protected:
            void SetUp() override
            {
                LeakDetectionFixture::SetUp();
                if (!QApplication::instance())
                {
                    m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);
                }
                m_style = AZStd::make_unique<AzQtComponents::Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                m_fusion = AZStd::unique_ptr<QStyle>(QStyleFactory::create(QStringLiteral("Fusion")));
                AtomToolsFramework::InspectorGroupHeaderWidget::RegisterFlatElement();
            }

            void TearDown() override
            {
                m_fusion.reset();
                m_style.reset();
                m_application.reset();
                LeakDetectionFixture::TearDown();
            }

            //! The flat way: the style on the widget, and the element resolved as Style::polish does in the flattened GUI
            void flatten(QWidget& widget)
            {
                widget.setStyle(m_style.get());
                AzQtComponents::BoxElement::resolve(&widget);
            }

            static const char* elementOf(const QWidget& widget)
            {
                const AzQtComponents::BoxElement* element = AzQtComponents::BoxElement::forWidget(&widget);
                return element ? element->m_name : "";
            }

            //! What a widget paints as its background and border (PE_Widget) at a logical offset on a parent-coloured image of the
            //! given device ratio; the image is the painter's device, so the style scales the box as the screen did
            static QImage paint(const QWidget& widget, const AzQtComponents::BoxElement* element, const QPoint& offset, qreal ratio, const QSize& deviceSize)
            {
                QImage image(deviceSize, QImage::Format_ARGB32_Premultiplied);
                image.setDevicePixelRatio(ratio);
                image.fill(QColor::fromRgb(ParentColor));

                QPainter painter(&image);
                painter.translate(offset);
                painter.setClipRect(widget.rect());
                QStyleOption option;
                option.initFrom(&widget);
                if (element)
                {
                    element->draw(&painter, &option, &widget);
                }
                else
                {
                    widget.style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, &widget);
                }
                painter.end();
                return image;
            }

            //! The header as the legacy GUI drew it: Qt's own style sheet style over Fusion, with the header's sheet and the margin of
            //! every label
            QImage legacyHeader(const QPoint& offset, qreal ratio, const QSize& deviceSize) const
            {
                QWidget root;
                QLabel* header = new QLabel(&root);
                header->resize(40, 24);
                root.setStyle(m_fusion.get());
                header->setStyle(m_fusion.get());
                header->setStyleSheet(QString::fromUtf8(HeaderSheet));
                root.setStyleSheet(QString::fromUtf8(LabelRules));
                root.ensurePolished();
                header->ensurePolished();
                return paint(*header, nullptr, offset, ratio, deviceSize);
            }

            //! The same header in the flattened GUI: the element
            QImage flatHeader(const QPoint& offset, qreal ratio, const QSize& deviceSize)
            {
                QWidget root;
                auto* header = new AtomToolsFramework::InspectorGroupHeaderWidget(&root);
                header->setFixedSize(40, 24);
                flatten(*header);
                return paint(*header, AzQtComponents::BoxElement::forWidget(header), offset, ratio, deviceSize);
            }

            static QRgb pixelAt(const QImage& image, int x, int y)
            {
                return image.pixel(x, y) & 0xFFFFFF;
            }

            int m_argc = 1;                                     // QApplication keeps a reference to it
            char m_programName[5] = "test";
            char* m_argv[2] = { m_programName, nullptr };
            AZStd::unique_ptr<QApplication> m_application;
            AZStd::unique_ptr<AzQtComponents::Style> m_style;
            AZStd::unique_ptr<QStyle> m_fusion;
        };
    } // namespace InspectorGroupHeaderWidgetTestsInternal

    using InspectorGroupHeaderWidgetTestsInternal::GroupHeaderElementName;
    using InspectorGroupHeaderWidgetTestsInternal::HeaderSheet;
    using InspectorGroupHeaderWidgetTestsInternal::InspectorGroupHeaderWidgetTests;
    using InspectorGroupHeaderWidgetTestsInternal::ParentColor;

    // ----------------------------------------------------------------------------------
    // Resolution
    // ----------------------------------------------------------------------------------

    TEST_F(InspectorGroupHeaderWidgetTests, Registry_EveryGroupHeaderHasTheElementAndNoOtherLabelDoes)
    {
        QWidget root;
        auto* header = new AtomToolsFramework::InspectorGroupHeaderWidget(&root);
        auto* anotherHeader = new AtomToolsFramework::InspectorGroupHeaderWidget(&root);
        auto* label = new QLabel(&root);

        flatten(*header);
        flatten(*anotherHeader);
        flatten(*label);

        EXPECT_STREQ(elementOf(*header), GroupHeaderElementName);
        EXPECT_STREQ(elementOf(*anotherHeader), GroupHeaderElementName);
        EXPECT_STRNE(elementOf(*label), GroupHeaderElementName);               // a Global element of AzQtComponents may cover a label, never this one
        EXPECT_TRUE(header->testAttribute(Qt::WA_StyledBackground));        // the fill and lines are drawn at PE_Widget
    }

    TEST_F(InspectorGroupHeaderWidgetTests, Constructor_SetsTheLegacySheetOnlyInTheLegacyGUI)
    {
        // The flattened GUI sets no sheet: it would shadow the element and miss the margin of the label. The switch is the process's
        // (StyleManager::stylesheetsDisabled keeps its first answer), so the test asks which of the two GUIs this process is.
        QWidget root;
        auto* header = new AtomToolsFramework::InspectorGroupHeaderWidget(&root);

        EXPECT_EQ(header->styleSheet().isEmpty(), AzQtComponents::StyleManager::stylesheetsDisabled());
        if (!AzQtComponents::StyleManager::stylesheetsDisabled())
        {
            EXPECT_TRUE(header->styleSheet() == QString::fromUtf8(HeaderSheet));        // the sheet it always set
        }
    }

    // ----------------------------------------------------------------------------------
    // Pixels: the element paints the box the sheets painted
    // ----------------------------------------------------------------------------------

    TEST_F(InspectorGroupHeaderWidgetTests, Pixels_TheMarginTheFillAndTheTwoLinesAreTheSheetsAtRatioOnePointFive)
    {
        // Legacy capture, MaterialCanvas Viewport Settings: the header is 24px high at device row 141 (whole). Its column reads
        // #444444 (the margin), #2f2f2f, #1b1b1b (the top line), #333333 (the fill) ... #1b1b1b (device row 174), #272727, #444444 (the
        // margin below). The flat capture had no margin: #1b1b1b, #272727, #333333 from the first row.
        const QImage sheet = legacyHeader(QPoint(0, 0), 1.5, QSize(60, 54));
        const QImage flat = flatHeader(QPoint(0, 0), 1.5, QSize(60, 54));

        EXPECT_EQ(pixelAt(flat, 20, 0), ParentColor);
        EXPECT_EQ(pixelAt(flat, 20, 1), 0x2F2F2Fu);
        EXPECT_EQ(pixelAt(flat, 20, 2), 0x1B1B1Bu);
        EXPECT_EQ(pixelAt(flat, 20, 3), 0x333333u);
        EXPECT_EQ(pixelAt(flat, 20, 18), 0x333333u);
        EXPECT_EQ(pixelAt(flat, 20, 33), 0x1B1B1Bu);
        EXPECT_EQ(pixelAt(flat, 20, 34), 0x272727u);
        EXPECT_EQ(pixelAt(flat, 20, 35), ParentColor);
        EXPECT_EQ(pixelAt(flat, 0, 18), 0x333333u);         // no line on the left, none on the right
        EXPECT_EQ(pixelAt(flat, 59, 18), 0x333333u);
        EXPECT_TRUE(flat == sheet);

        // and at ratio 1, where the margin is one row
        EXPECT_TRUE(flatHeader(QPoint(0, 0), 1.0, QSize(40, 30)) == legacyHeader(QPoint(0, 0), 1.0, QSize(40, 30)));
    }

    TEST_F(InspectorGroupHeaderWidgetTests, Pixels_AtTheHalfPixelPhaseTheRowsAreTheLegacyOnes)
    {
        // Legacy capture, PassCanvas: the headers at logical y 743 and 969 sit at device rows 1114.5 and 1453.5. Down a column from the
        // margin: #444444, #1b1b1b, #272727, #333333 (the fill) ... #333333, #272727, #1b1b1b, #444444. The same phase here is logical y 1
        // (device 1.5): the top line is device rows 3-4, the bottom line rows 34-35. The fill is not smoothed and the lines are, so the
        // half covered row of each line shows the fill through it (#272727).
        const QImage sheet = legacyHeader(QPoint(0, 1), 1.5, QSize(60, 54));
        const QImage flat = flatHeader(QPoint(0, 1), 1.5, QSize(60, 54));

        EXPECT_EQ(pixelAt(flat, 20, 2), ParentColor);
        EXPECT_EQ(pixelAt(flat, 20, 3), 0x1B1B1Bu);
        EXPECT_EQ(pixelAt(flat, 20, 4), 0x272727u);
        EXPECT_EQ(pixelAt(flat, 20, 5), 0x333333u);
        EXPECT_EQ(pixelAt(flat, 20, 33), 0x333333u);
        EXPECT_EQ(pixelAt(flat, 20, 34), 0x272727u);
        EXPECT_EQ(pixelAt(flat, 20, 35), 0x1B1B1Bu);
        EXPECT_EQ(pixelAt(flat, 20, 36), ParentColor);
        EXPECT_TRUE(flat == sheet);
    }
} // namespace UnitTest
