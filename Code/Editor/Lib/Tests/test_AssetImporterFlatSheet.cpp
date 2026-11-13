/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/Style.h>
#include <EditorFlatSheet.h>
#include <EditorFlatValues.h>

#include <QApplication>
#include <QFrame>
#include <QImage>
#include <QLabel>
#include <QMargins>
#include <QPainter>
#include <QPushButton>
#include <QStyleFactory>
#include <QStyleOption>
#include <QWidget>

#include <string>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The element of the sheet ImporterRootDisplay.ui gave the python builder frame of the asset importer: which widgets it reaches, and
    // that it lays out and paints what Qt's own style sheet style laid out and painted, at the device ratios the GUI runs at: the
    // rounded top corners and the square bottom ones included.
    //
    // NOT covered here: AssetImporterWindow.qss, which the importer's window sets on itself in the legacy GUI; its element and twins
    // are in AssetImporterWindow.cpp and test_AssetImporterTreeFlatSheet.cpp.
    // ----------------------------------------------------------------------------------

    namespace AssetImporterFlatSheetTestsInternal
    {
        //! A widget that Qt takes for an instance of a class it is not. The matchers ask for the root display by its class name, as the
        //! selectors do, and building the real one needs the Editor's whole scene settings system.
        class ImporterClassWidget
            : public QWidget
        {
        public:
            ImporterClassWidget(const char* className, QWidget* parent = nullptr)
                : QWidget(parent)
                , m_className(className)
            {
            }

            void* qt_metacast(const char* name) override
            {
                return qstrcmp(name, m_className) == 0 ? this : QWidget::qt_metacast(name);
            }

        private:
            const char* m_className;
        };

        constexpr const char* RootDisplayClass = "ImporterRootDisplayWidget";
        constexpr const char* PythonBuilderElementName = "Editor.Box.AssetImporterPythonBuilder";
        constexpr const char* PythonBuilderName = "m_pythonBuilderLayout";

        // The sheet the form gave the frame (ImporterRootDisplay.ui:79, as it was before the flattening)
        constexpr const char* PythonBuilderSheet =
            "QFrame#m_pythonBuilderLayout {background-color: #333333; margin-bottom: 6px; padding-top: 2px; padding-bottom: 2px; "
            "border-top-left-radius: 2px; border-top-right-radius: 2px;}";

        // The rules of the application's sheet that reach that frame, with the lines they are on
        // BaseStyleSheet.qss:15-21, the universal rule
        constexpr const char* UniversalRule =
            "* { background-color: transparent; color: white; font-family: \"Open Sans\"; font-weight: 400; }";
        // BaseStyleSheet.qss:41-48, the fill of every divider, box and panel a QFrame draws itself (the frame is a StyledPanel, shape 6)
        constexpr const char* PanelRule = ".QFrame[frameShape=\"1\"], .QFrame[frameShape=\"2\"], .QFrame[frameShape=\"4\"],"
                                          " .QFrame[frameShape=\"5\"], .QFrame[frameShape=\"6\"] { background-color: #444444; }";

        // The frame as the form sizes it here: 200 by 40, and so a border box of 200 by 34 (margin-bottom 6), rounded by 2
        constexpr int FrameWidth = 200;
        constexpr int FrameHeight = 40;
        constexpr int MarginBottom = 6;
        constexpr int Radius = 2;
        constexpr QRgb FillColor = 0x333333;
        constexpr QRgb ParentColor = 0x808080;          // what neither the sheet nor the element draws stays this grey

        //! The option QFrame::paintEvent builds for a frame (QFrame::initStyleOption, protected)
        QStyleOptionFrame frameOption(const QFrame* frame)
        {
            QStyleOptionFrame option;
            option.initFrom(frame);
            option.frameShape = frame->frameShape();
            option.lineWidth = frame->lineWidth();
            option.midLineWidth = frame->midLineWidth();
            if (frame->frameShadow() == QFrame::Sunken)
            {
                option.state |= QStyle::State_Sunken;
            }
            else if (frame->frameShadow() == QFrame::Raised)
            {
                option.state |= QStyle::State_Raised;
            }
            return option;
        }

        //! What a frame paints through its own style, in the order Qt paints it: the styled background (PE_Widget,
        //! QWidgetPrivate::paintBackground), then its frame (CE_ShapedFrame, QFrame::drawFrame). The image is the painter's device, so the
        //! style scales the box to the ratio as a screen of that ratio does.
        QImage paintFrame(const QFrame& frame, qreal ratio)
        {
            QImage image((QSizeF(frame.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor::fromRgb(ParentColor));
            QPainter painter(&image);
            if (frame.testAttribute(Qt::WA_StyledBackground))
            {
                QStyleOption background;
                background.initFrom(&frame);
                frame.style()->drawPrimitive(QStyle::PE_Widget, &background, &painter, &frame);
            }
            const QStyleOptionFrame option = frameOption(&frame);
            frame.style()->drawControl(QStyle::CE_ShapedFrame, &option, &painter, &frame);
            painter.end();
            return image;
        }

        //! The first pixel two images of one size differ at, as text for a failure message ("" when they are equal)
        std::string firstDifference(const QImage& legacy, const QImage& flat)
        {
            if (legacy.size() != flat.size())
            {
                return "the sizes differ";
            }
            for (int y = 0; y < legacy.height(); ++y)
            {
                for (int x = 0; x < legacy.width(); ++x)
                {
                    if (legacy.pixel(x, y) != flat.pixel(x, y))
                    {
                        const auto hex = [](QRgb rgb)
                        {
                            return QString::number(rgb & 0xFFFFFF, 16).toStdString();
                        };
                        return "at (" + std::to_string(x) + ", " + std::to_string(y) + "): legacy #" + hex(legacy.pixel(x, y)) + ", flat #"
                            + hex(flat.pixel(x, y));
                    }
                }
            }
            return "";
        }

        //! The frame as the form has it (ImporterRootDisplay.ui:77-86): a StyledPanel, Raised, named for it
        QFrame* makePythonBuilder(QWidget* parent)
        {
            auto* frame = new QFrame(parent);
            frame->setObjectName(QString::fromLatin1(PythonBuilderName));
            frame->setFrameShape(QFrame::StyledPanel);
            frame->setFrameShadow(QFrame::Raised);
            frame->resize(FrameWidth, FrameHeight);
            return frame;
        }

        //! What the twins compare: the frame's pixels, the rect its contents sit in (SE_ShapedFrameContents) and the contents margins
        //! QFrame takes from it
        struct FrameTwin
        {
            QImage m_image;
            QRect m_contents;
            QMargins m_margins;
        };

        class AssetImporterFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            void SetUp() override
            {
                LeakDetectionFixture::SetUp();

                // The test application carries StyleManager's legacy sheets: the twins name their own
                m_applicationSheet = qApp->styleSheet();
                qApp->setStyleSheet(QString());

                // The flattened GUI's branches of Style (CE_ShapedFrame, SE_ShapedFrameContents) read the rung
                AzQtComponents::GuiPerfFloor::setRungForTesting(AzQtComponents::GuiPerfFloor::Rung::Flat);

                m_style = AZStd::make_unique<AzQtComponents::Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                m_fusion = AZStd::unique_ptr<QStyle>(QStyleFactory::create(QStringLiteral("Fusion")));

                // Once for the process, as the flattened GUI runs the registrar of a sheet once
                static const bool registered = (EditorFlatSheet::RegisterElements(), true);
                ASSERT_TRUE(registered);
            }

            void TearDown() override
            {
                m_fusion.reset();
                m_style.reset();
                AzQtComponents::GuiPerfFloor::resetForTesting();
                qApp->setStyleSheet(m_applicationSheet);
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

            //! The contents box of a frame (SE_ShapedFrameContents): the rect inside its margin, border and padding
            static QRect frameContents(QFrame& frame)
            {
                const QStyleOptionFrame option = frameOption(&frame);
                return frame.style()->subElementRect(QStyle::SE_ShapedFrameContents, &option, &frame);
            }

            //! The frame as the legacy GUI had it: Qt's style sheet style over Fusion, the sheet the form gave it, and the rules of the
            //! application's sheet on its parent (a sheet on a widget outranks the application's whatever the specificity of their rules)
            FrameTwin legacyFrame(qreal ratio) const
            {
                QWidget root;
                QFrame* frame = makePythonBuilder(&root);
                root.setStyle(m_fusion.get());
                frame->setStyle(m_fusion.get());
                frame->setStyleSheet(QString::fromLatin1(PythonBuilderSheet));
                root.setStyleSheet(QString::fromLatin1(UniversalRule) + QString::fromLatin1(PanelRule));
                root.ensurePolished();
                frame->ensurePolished();
                return twinOf(*frame, ratio);
            }

            //! The same frame in the flattened GUI: below the root display of the importer, with its element and no sheet
            FrameTwin flatFrame(qreal ratio)
            {
                ImporterClassWidget display(RootDisplayClass);
                QFrame* frame = makePythonBuilder(&display);
                flatten(*frame);
                return twinOf(*frame, ratio);
            }

            //! QFrame reads its box from the style when its frame style is set: set again, the margins are those the style gives now
            static FrameTwin twinOf(QFrame& frame, qreal ratio)
            {
                frame.setFrameStyle(frame.frameStyle());
                return FrameTwin{ paintFrame(frame, ratio), frameContents(frame), frame.contentsMargins() };
            }

            QString m_applicationSheet;
            AZStd::unique_ptr<AzQtComponents::Style> m_style;
            AZStd::unique_ptr<QStyle> m_fusion;
        };
    } // namespace AssetImporterFlatSheetTestsInternal

    // A unity build joins this file with others of namespace UnitTest: the fixture is the only name brought in, the rest is named
    // through the alias
    using AssetImporterFlatSheetTestsInternal::AssetImporterFlatSheetTests;
    namespace ImporterFlat = AssetImporterFlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // Resolution
    // ----------------------------------------------------------------------------------

    TEST_F(AssetImporterFlatSheetTests, Registry_ThePythonBuilderFrameIsFoundByItsNameBelowTheRootDisplay)
    {
        ImporterFlat::ImporterClassWidget display(ImporterFlat::RootDisplayClass);
        QFrame* frame = ImporterFlat::makePythonBuilder(&display);
        QWidget* wrapper = new QWidget(&display);                       // `QFrame#name` has no parent: the frame may sit deeper
        QFrame* deepFrame = ImporterFlat::makePythonBuilder(wrapper);

        flatten(*frame);
        flatten(*deepFrame);

        EXPECT_STREQ(elementOf(*frame), ImporterFlat::PythonBuilderElementName);
        EXPECT_STREQ(elementOf(*deepFrame), ImporterFlat::PythonBuilderElementName);
        EXPECT_TRUE(frame->testAttribute(Qt::WA_StyledBackground));     // the fill is drawn at PE_Widget
    }

    TEST_F(AssetImporterFlatSheetTests, Registry_TheNameOutsideTheRootDisplayAndTheOtherWidgetsInsideItHaveNoElement)
    {
        // The same name, but no root display above it
        QWidget elsewhere;
        QFrame* strayFrame = ImporterFlat::makePythonBuilder(&elsewhere);

        // In a root display: the label and the close button inside the frame, another frame, and a widget that is not a QFrame
        ImporterFlat::ImporterClassWidget display(ImporterFlat::RootDisplayClass);
        QFrame* frame = ImporterFlat::makePythonBuilder(&display);
        auto* label = new QLabel(QStringLiteral("Assigned Python Builder Script"), frame);
        label->setObjectName(QStringLiteral("m_pythonBuilder"));
        auto* closeButton = new QPushButton(frame);
        closeButton->setObjectName(QStringLiteral("m_closeButton"));
        auto* otherFrame = new QFrame(&display);
        otherFrame->setObjectName(QStringLiteral("m_pythonBuilderLayout2"));
        auto* notAFrame = new QWidget(&display);
        notAFrame->setObjectName(QString::fromLatin1(ImporterFlat::PythonBuilderName));

        for (QWidget* widget : { static_cast<QWidget*>(strayFrame), static_cast<QWidget*>(label), static_cast<QWidget*>(closeButton),
                                 static_cast<QWidget*>(otherFrame), notAFrame })
        {
            flatten(*widget);
        }

        EXPECT_STRNE(elementOf(*strayFrame), ImporterFlat::PythonBuilderElementName);
        EXPECT_STRNE(elementOf(*label), ImporterFlat::PythonBuilderElementName);
        EXPECT_STRNE(elementOf(*closeButton), ImporterFlat::PythonBuilderElementName);
        EXPECT_STRNE(elementOf(*otherFrame), ImporterFlat::PythonBuilderElementName);
        EXPECT_STRNE(elementOf(*notAFrame), ImporterFlat::PythonBuilderElementName);
    }

    TEST_F(AssetImporterFlatSheetTests, Element_ItHasTheValuesOfTheFormsSheet)
    {
        ImporterFlat::ImporterClassWidget display(ImporterFlat::RootDisplayClass);
        QFrame* frame = ImporterFlat::makePythonBuilder(&display);
        flatten(*frame);

        const AzQtComponents::BoxElement* element = AzQtComponents::BoxElement::forWidget(frame);
        ASSERT_TRUE(element != nullptr);
        EXPECT_EQ(element->m_margin, QMargins(0, 0, 0, ImporterFlat::MarginBottom));    // margin-bottom: 6px
        EXPECT_EQ(element->m_padding, QMargins(0, 2, 0, 2));                            // padding-top, padding-bottom: 2px
        // border-top-left-radius, border-top-right-radius: 2px; the bottom corners have no radius
        EXPECT_EQ(element->m_radii.m_topLeft, ImporterFlat::Radius);
        EXPECT_EQ(element->m_radii.m_topRight, ImporterFlat::Radius);
        EXPECT_EQ(element->m_radii.m_bottomRight, 0);
        EXPECT_EQ(element->m_radii.m_bottomLeft, 0);
        EXPECT_EQ(element->m_radius, 0);                                                // no border-radius on the four corners
        EXPECT_TRUE(element->m_idle.m_fill.color() == QColor::fromRgb(ImporterFlat::FillColor));    // background-color: #333333

        // A radius gives the border a style of its own (QRenderRule::fixupBorder): the style's native frame is not drawn
        EXPECT_FALSE(element->m_nativeBorder);
    }

    // ----------------------------------------------------------------------------------
    // Geometry: where the contents of the frame sit
    // ----------------------------------------------------------------------------------

    TEST_F(AssetImporterFlatSheetTests, Geometry_TheFramesContentsAreThoseOfItsRuleUnderTheFormsSheet)
    {
        const ImporterFlat::FrameTwin legacy = legacyFrame(1.0);
        const ImporterFlat::FrameTwin flat = flatFrame(1.0);

        EXPECT_EQ(flat.m_contents, legacy.m_contents);
        EXPECT_EQ(flat.m_margins, legacy.m_margins);

        // The legacy frame has no native frame line: the radius made its border none, 0 wide, so what lies between the box and the
        // contents is the padding alone, 2px above and below, and the margin-bottom of 6px under the box
        EXPECT_EQ(legacy.m_contents, QRect(0, 2, ImporterFlat::FrameWidth, ImporterFlat::FrameHeight - ImporterFlat::MarginBottom - 4));
        EXPECT_EQ(legacy.m_margins, QMargins(0, 2, 0, ImporterFlat::MarginBottom + 2));
        EXPECT_EQ(flat.m_contents, QRect(0, 2, ImporterFlat::FrameWidth, ImporterFlat::FrameHeight - ImporterFlat::MarginBottom - 4));
        EXPECT_EQ(flat.m_margins, QMargins(0, 2, 0, ImporterFlat::MarginBottom + 2));
    }

    // ----------------------------------------------------------------------------------
    // Pixels: the element against the form's sheet, at the device ratios of the GUI
    // ----------------------------------------------------------------------------------

    TEST_F(AssetImporterFlatSheetTests, Pixels_TheFramePaintsWhatTheFormsSheetPaintedAtEveryRatio)
    {
        for (const qreal ratio : { 1.0, 1.5, 2.0 })
        {
            const ImporterFlat::FrameTwin legacy = legacyFrame(ratio);
            const ImporterFlat::FrameTwin flat = flatFrame(ratio);

            // Every pixel, the corners included
            EXPECT_EQ(ImporterFlat::firstDifference(legacy.m_image, flat.m_image), std::string()) << "at ratio " << ratio;

            // The sheet rounded the top two corners and gave the bottom two no radius: the bottom corner pixels of the border box
            // are the fill's, the top ones are not (the border box ends above the margin-bottom), in both GUIs
            const int borderBottom = static_cast<int>((ImporterFlat::FrameHeight - ImporterFlat::MarginBottom) * ratio);
            for (const QImage* image : { &legacy.m_image, &flat.m_image })
            {
                EXPECT_EQ(image->pixel(0, borderBottom - 1) & 0xFFFFFF, ImporterFlat::FillColor) << "bottom left, at ratio " << ratio;
                EXPECT_EQ(image->pixel(image->width() - 1, borderBottom - 1) & 0xFFFFFF, ImporterFlat::FillColor)
                    << "bottom right, at ratio " << ratio;
                EXPECT_NE(image->pixel(0, 0) & 0xFFFFFF, ImporterFlat::FillColor) << "top left, at ratio " << ratio;
                EXPECT_NE(image->pixel(image->width() - 1, 0) & 0xFFFFFF, ImporterFlat::FillColor) << "top right, at ratio " << ratio;
            }
        }
    }

    TEST_F(AssetImporterFlatSheetTests, Pixels_TheFrameIsFilledToItsEdgesWithNoFrameLineAndTheMarginBelowItIsBare)
    {
        for (const qreal ratio : { 1.0, 1.5, 2.0 })
        {
            const QImage flat = flatFrame(ratio).m_image;
            const auto at = [&flat, ratio](qreal x, qreal y)
            {
                return flat.pixel(static_cast<int>(x * ratio), static_cast<int>(y * ratio)) & 0xFFFFFF;
            };

            // A native Raised panel line would lie in the first logical pixel along each edge, over the fill
            EXPECT_EQ(at(2, 17), ImporterFlat::FillColor) << "left, at ratio " << ratio;
            EXPECT_EQ(at(ImporterFlat::FrameWidth - 2, 17), ImporterFlat::FillColor) << "right, at ratio " << ratio;
            EXPECT_EQ(at(100, 2), ImporterFlat::FillColor) << "top, at ratio " << ratio;
            EXPECT_EQ(at(100, ImporterFlat::FrameHeight - ImporterFlat::MarginBottom - 2), ImporterFlat::FillColor)
                << "bottom, at ratio " << ratio;

            // The margin-bottom of 6px is left bare
            EXPECT_EQ(at(100, ImporterFlat::FrameHeight - 4), ImporterFlat::ParentColor) << "margin, at ratio " << ratio;
        }
    }

    TEST_F(AssetImporterFlatSheetTests, Value_TheSheetTheCodeSetsIsTheFormsTextByteForByteAndTheElementReadsTheSameFill)
    {
        // The code writes the text of the sheet from the fill the element reads (EditorFlatValues.h): it is the text the form had
        EXPECT_EQ(EditorFlatValues::AssetImporterPythonBuilderSheet(), QString::fromLatin1(ImporterFlat::PythonBuilderSheet));

        ImporterFlat::ImporterClassWidget display(ImporterFlat::RootDisplayClass);
        QFrame* frame = ImporterFlat::makePythonBuilder(&display);
        flatten(*frame);
        const AzQtComponents::BoxElement* element = AzQtComponents::BoxElement::forWidget(frame);
        ASSERT_TRUE(element != nullptr);
        EXPECT_STREQ(element->m_name, ImporterFlat::PythonBuilderElementName);
        EXPECT_TRUE(element->m_idle.m_fill.color() == EditorFlatValues::AssetImporterPythonBuilderFill);
    }
} // namespace UnitTest
