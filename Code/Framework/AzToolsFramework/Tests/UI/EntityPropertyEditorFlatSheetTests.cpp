/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ToolTipElement.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Tests/SheetTwins.h>
#include <AzQtComponents/Tests/SiteTwins.h>
#include <AzQtComponents/Tests/ToolTipTwins.h>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyColorCtrl.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyColorGradientCtrl.hxx>

#include <QApplication>
#include <QFrame>
#include <QHelpEvent>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPainter>
#include <QStyleFactory>
#include <QStyleOption>
#include <QWidget>

#include <string>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The elements of the inspector's dark box: which widgets they reach, and that they paint what the form's sheets painted (Qt's
    // own style sheet style over Fusion, with the same sheet) at the device ratios the GUI runs at
    // ----------------------------------------------------------------------------------

    namespace EntityPropertyEditorFlatSheetTestsInternal
    {
        //! A widget that Qt takes for an instance of a class it is not. The matchers ask for the inspector by its class name, as the
        //! selectors do, and building a real EntityPropertyEditor needs the Editor's whole entity system.
        class ClassNamedWidget
            : public QWidget
        {
        public:
            ClassNamedWidget(const char* className, QWidget* parent = nullptr)
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

        constexpr const char* InspectorClass = "AzToolsFramework::EntityPropertyEditor";
        constexpr const char* DarkBoxElementName = "Editor.Box.InspectorDarkBox";
        constexpr const char* DarkBoxLabelElementName = "Editor.Box.InspectorDarkBoxLabel";
        constexpr const char* DarkBoxLineElementName = "Editor.Box.InspectorDarkBoxLine";

        // The sheets the form gave the box and its six widgets (EntityPropertyEditor.ui before the flattening), and the rule of
        // every label that the legacy cascade laid under the labels' own (Text.qss:93-94)
        constexpr const char* DarkBoxSheet = "QWidget#m_darkBox { background-color:rgb(51, 51, 51) }";
        constexpr const char* DarkBoxPartSheet = "background-color:rgb(51, 51, 51)";
        constexpr const char* LabelRules = "QLabel { margin-top: 1px; margin-bottom: 1px; }";

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

        //! What a widget paints for its box through its own style, in the order Qt paints it: the styled background (PE_Widget,
        //! QWidgetPrivate::paintBackground), then a frame's frame (CE_ShapedFrame, QFrame::drawFrame). The image is the painter's
        //! device, so the style scales the box to the ratio as a screen of that ratio does.
        QImage paintBox(const QWidget& widget, qreal ratio)
        {
            QImage image((QSizeF(widget.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor::fromRgb(ParentColor));
            QPainter painter(&image);
            if (widget.testAttribute(Qt::WA_StyledBackground))
            {
                QStyleOption background;
                background.initFrom(&widget);
                widget.style()->drawPrimitive(QStyle::PE_Widget, &background, &painter, &widget);
            }
            if (const auto* frame = qobject_cast<const QFrame*>(&widget))
            {
                const QStyleOptionFrame option = frameOption(frame);
                widget.style()->drawControl(QStyle::CE_ShapedFrame, &option, &painter, &widget);
            }
            painter.end();
            return image;
        }

        //! The first pixel two images of one size differ at, as text for a failure message ("" when they are equal)
        std::string firstDifference(const QImage& a, const QImage& b)
        {
            if (a.size() != b.size())
            {
                return "the sizes differ";
            }
            for (int y = 0; y < a.height(); ++y)
            {
                for (int x = 0; x < a.width(); ++x)
                {
                    if (a.pixel(x, y) != b.pixel(x, y))
                    {
                        const auto hex = [](QRgb rgb)
                        {
                            return QString::number(rgb & 0xFFFFFF, 16).toStdString();
                        };
                        return "at (" + std::to_string(x) + ", " + std::to_string(y) + "): legacy #" + hex(a.pixel(x, y)) + ", flat #"
                            + hex(b.pixel(x, y));
                    }
                }
            }
            return "";
        }

        //! The kinds of widget the form gave the sheets: the box itself, a label, and the line (a sunken horizontal QFrame)
        enum class Part
        {
            Box,
            Label,
            Line,
        };

        //! A widget of that kind with its name, sized as the GUI lays it out
        QWidget* makePart(Part part, QWidget* parent)
        {
            switch (part)
            {
            case Part::Box:
            {
                auto* box = new QWidget(parent);
                box->setObjectName(QStringLiteral("m_darkBox"));
                box->resize(120, 40);
                return box;
            }
            case Part::Label:
            {
                auto* label = new QLabel(QStringLiteral("Entity ID"), parent);
                label->setObjectName(QStringLiteral("m_entityIdLabel"));
                label->resize(80, 18);
                return label;
            }
            case Part::Line:
            {
                auto* line = new QFrame(parent);
                line->setObjectName(QStringLiteral("line_2"));
                line->setFrameShape(QFrame::HLine);
                line->setFrameShadow(QFrame::Sunken);
                line->resize(120, 3);
                return line;
            }
            }
            return nullptr;
        }

        class EntityPropertyEditorFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            void SetUp() override
            {
                LeakDetectionFixture::SetUp();

                // The test application carries StyleManager's legacy sheets: the twins name their own
                m_applicationSheet = qApp->styleSheet();
                qApp->setStyleSheet(QString());

                // The flattened GUI's branches of Style (CE_ShapedFrame) read the rung
                AzQtComponents::GuiPerfFloor::setRungForTesting(AzQtComponents::GuiPerfFloor::Rung::Flat);

                m_style = AZStd::make_unique<AzQtComponents::Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                m_fusion = AZStd::unique_ptr<QStyle>(QStyleFactory::create(QStringLiteral("Fusion")));
                AzToolsFramework::EntityPropertyEditor::RegisterFlatElements();
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

            //! The part as the legacy GUI styled it: Qt's style sheet style over Fusion, with the form's sheet on the part and the rule
            //! of every label above it. The callback gets the polished part.
            void withLegacyPart(Part part, const AZStd::function<void(QWidget&)>& use) const
            {
                QWidget root;
                QWidget* widget = makePart(part, &root);
                root.setStyle(m_fusion.get());
                widget->setStyle(m_fusion.get());
                widget->setStyleSheet(QString::fromLatin1(part == Part::Box ? DarkBoxSheet : DarkBoxPartSheet));
                root.setStyleSheet(QString::fromLatin1(LabelRules));
                root.ensurePolished();
                widget->ensurePolished();
                use(*widget);
            }

            //! The same part in the flattened GUI: in the box of an inspector, with its element
            void withFlatPart(Part part, const AZStd::function<void(QWidget&)>& use)
            {
                ClassNamedWidget inspector(InspectorClass);
                QWidget* widget = nullptr;
                if (part == Part::Box)
                {
                    widget = makePart(part, &inspector);
                }
                else
                {
                    QWidget* box = makePart(Part::Box, &inspector);
                    widget = makePart(part, box);
                }
                flatten(*widget);
                use(*widget);
            }

            QImage legacyPart(Part part, qreal ratio) const
            {
                QImage image;
                withLegacyPart(part, [&image, ratio](QWidget& widget) { image = paintBox(widget, ratio); });
                return image;
            }

            QImage flatPart(Part part, qreal ratio)
            {
                QImage image;
                withFlatPart(part, [&image, ratio](QWidget& widget) { image = paintBox(widget, ratio); });
                return image;
            }

            //! The contents box of a frame (SE_ShapedFrameContents): where a label lays out its text, and whether it indents it
            static QRect frameContents(QWidget& widget)
            {
                const QStyleOptionFrame option = frameOption(qobject_cast<QFrame*>(&widget));
                return widget.style()->subElementRect(QStyle::SE_ShapedFrameContents, &option, &widget);
            }

            QString m_applicationSheet;
            AZStd::unique_ptr<AzQtComponents::Style> m_style;
            AZStd::unique_ptr<QStyle> m_fusion;
        };
    } // namespace EntityPropertyEditorFlatSheetTestsInternal

    // A unity build joins this file with others of namespace UnitTest (EntityOutlinerFlatSheetTests has a ClassNamedWidget of its
    // own): the fixture is the only name brought in, the rest is named through the alias
    using EntityPropertyEditorFlatSheetTestsInternal::EntityPropertyEditorFlatSheetTests;
    namespace InspectorFlat = EntityPropertyEditorFlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // Resolution
    // ----------------------------------------------------------------------------------

    TEST_F(EntityPropertyEditorFlatSheetTests, Registry_TheBoxAndItsSixWidgetsBelowAnInspectorHaveTheElements)
    {
        InspectorFlat::ClassNamedWidget inspector(InspectorFlat::InspectorClass);
        auto* box = new QWidget(&inspector);
        box->setObjectName(QStringLiteral("m_darkBox"));
        QList<QWidget*> parts;
        const char* const names[] = {
            "m_entityIdLabel", "m_entityIdText", "m_prefabContainerLabel", "m_prefabContainerText", "line_2", "m_statusLabel",
        };
        for (const char* name : names)
        {
            const bool line = qstrcmp(name, "line_2") == 0;
            QWidget* part = line ? static_cast<QWidget*>(new QFrame(box)) : static_cast<QWidget*>(new QLabel(box));
            part->setObjectName(QString::fromLatin1(name));
            parts.append(part);
        }

        flatten(*box);
        for (QWidget* part : parts)
        {
            flatten(*part);
        }

        EXPECT_STREQ(elementOf(*box), InspectorFlat::DarkBoxElementName);
        EXPECT_TRUE(box->testAttribute(Qt::WA_StyledBackground));      // the fill is drawn at PE_Widget
        for (QWidget* part : parts)
        {
            const bool line = part->objectName() == QStringLiteral("line_2");
            EXPECT_STREQ(elementOf(*part), line ? InspectorFlat::DarkBoxLineElementName : InspectorFlat::DarkBoxLabelElementName)
                << part->objectName().toStdString();
        }
    }

    TEST_F(EntityPropertyEditorFlatSheetTests, Registry_TheNamesOutsideAnInspectorOrOutsideTheBoxHaveNoneOfThem)
    {
        // The same names, but no inspector above them
        QWidget window;
        auto* box = new QWidget(&window);
        box->setObjectName(QStringLiteral("m_darkBox"));
        auto* label = new QLabel(box);
        label->setObjectName(QStringLiteral("m_entityIdLabel"));
        flatten(*box);
        flatten(*label);
        EXPECT_STRNE(elementOf(*box), InspectorFlat::DarkBoxElementName);
        EXPECT_STRNE(elementOf(*label), InspectorFlat::DarkBoxLabelElementName);

        // In an inspector, but not in its box; and another widget in the box
        InspectorFlat::ClassNamedWidget inspector(InspectorFlat::InspectorClass);
        auto* looseLabel = new QLabel(&inspector);
        looseLabel->setObjectName(QStringLiteral("m_entityIdLabel"));
        auto* inspectorBox = new QWidget(&inspector);
        inspectorBox->setObjectName(QStringLiteral("m_darkBox"));
        auto* other = new QLabel(inspectorBox);
        other->setObjectName(QStringLiteral("m_entityNameLabel"));
        flatten(*looseLabel);
        flatten(*other);
        EXPECT_STRNE(elementOf(*looseLabel), InspectorFlat::DarkBoxLabelElementName);
        EXPECT_STRNE(elementOf(*other), InspectorFlat::DarkBoxLabelElementName);
    }

    // ----------------------------------------------------------------------------------
    // Pixels: the element against the form's sheet, at the device ratios of the GUI
    // ----------------------------------------------------------------------------------

    TEST_F(EntityPropertyEditorFlatSheetTests, Pixels_EachPartPaintsWhatTheFormsSheetPaintedAtEveryRatio)
    {
        using InspectorFlat::Part;
        for (const Part part : { Part::Box, Part::Label, Part::Line })
        {
            for (const qreal ratio : { 1.0, 1.5, 2.0 })
            {
                const QImage legacy = legacyPart(part, ratio);
                const QImage flat = flatPart(part, ratio);
                EXPECT_EQ(InspectorFlat::firstDifference(legacy, flat), std::string())
                    << "part " << static_cast<int>(part) << " at ratio " << ratio;
            }
        }
    }

    TEST_F(EntityPropertyEditorFlatSheetTests, Box_ALabelsContentsAreThoseOfTheLabelRuleUnderItsSheet)
    {
        // Where the label lays out its text: the label rule's margin above and below (and so QLabel's indent of half an 'x')
        QRect legacy;
        QRect flat;
        withLegacyPart(InspectorFlat::Part::Label, [&legacy](QWidget& widget) { legacy = frameContents(widget); });
        withFlatPart(InspectorFlat::Part::Label, [&flat](QWidget& widget) { flat = frameContents(widget); });

        EXPECT_EQ(flat, legacy);
        EXPECT_EQ(flat, QRect(0, 1, 80, 16));          // the label is 80 x 18
    }

    TEST_F(EntityPropertyEditorFlatSheetTests, Box_TheLineAndTheBoxKeepTheContentsQtGivesThem)
    {
        for (const InspectorFlat::Part part : { InspectorFlat::Part::Box, InspectorFlat::Part::Line })
        {
            QRect legacy;
            QRect flat;
            withLegacyPart(part, [&legacy](QWidget& widget) { legacy = widget.contentsRect(); });
            withFlatPart(part, [&flat](QWidget& widget) { flat = widget.contentsRect(); });
            EXPECT_EQ(flat, legacy) << "part " << static_cast<int>(part);
        }
    }

    TEST_F(EntityPropertyEditorFlatSheetTests, Pixels_TheBoxIsFilledWithTheFormsColourToItsEdges)
    {
        const QImage flat = flatPart(InspectorFlat::Part::Box, 1.5);

        EXPECT_EQ(flat.pixel(0, 0) & 0xFFFFFF, 0x333333u);
        EXPECT_EQ(flat.pixel(flat.width() - 1, flat.height() - 1) & 0xFFFFFF, 0x333333u);
    }
    // ----------------------------------------------------------------------------------
    // The tooltips of the labels in the box. The six sheets of the form have no selector, so each reached the tip its widget shows:
    // `background-color:rgb(51, 51, 51)` outranked `QToolTip { background-color: black }` (ToolTip.qss:13) there. The sheet style
    // names a tip "QToolTip", so the box's `QWidget#m_darkBox` rule reached none.
    // ----------------------------------------------------------------------------------

    TEST_F(EntityPropertyEditorFlatSheetTests, ToolTipRegistry_TheFiveLabelsOfTheBoxHaveTheFillAndNoOtherWidgetDoes)
    {
        using AzQtComponents::ToolTipElement;
        const QColor fill(0x33, 0x33, 0x33);

        InspectorFlat::ClassNamedWidget inspector(InspectorFlat::InspectorClass);
        auto* box = new QWidget(&inspector);
        box->setObjectName(QStringLiteral("m_darkBox"));
        const char* const names[] = {
            "m_entityIdLabel", "m_entityIdText", "m_prefabContainerLabel", "m_prefabContainerText", "m_statusLabel",
        };
        for (const char* name : names)
        {
            auto* label = new QLabel(box);
            label->setObjectName(QString::fromLatin1(name));
            const ToolTipElement::Look look = ToolTipElement::lookFor(label);
            EXPECT_EQ(look.m_fill, fill) << name;
            EXPECT_EQ(look.m_text, nullptr) << name;
        }

        // The box's own rule, a label of the box the form gave no sheet, the names in the inspector but outside the box, and outside
        // any inspector
        auto* otherInBox = new QLabel(box);
        otherInBox->setObjectName(QStringLiteral("m_entityNameLabel"));
        auto* looseLabel = new QLabel(&inspector);
        looseLabel->setObjectName(QStringLiteral("m_entityIdLabel"));
        QWidget window;
        auto* windowBox = new QWidget(&window);
        windowBox->setObjectName(QStringLiteral("m_darkBox"));
        auto* windowLabel = new QLabel(windowBox);
        windowLabel->setObjectName(QStringLiteral("m_entityIdLabel"));
        for (const QWidget* widget : { static_cast<const QWidget*>(box), static_cast<const QWidget*>(otherInBox),
                                       static_cast<const QWidget*>(looseLabel), static_cast<const QWidget*>(windowLabel) })
        {
            EXPECT_FALSE(ToolTipElement::lookFor(widget).m_fill.isValid()) << widget->objectName().toStdString();
        }
    }

    TEST_F(EntityPropertyEditorFlatSheetTests, ToolTip_ALabelOfTheBoxShowsTheFillOfItsSheetAndOneOutsideTheBoxDoesNot)
    {
        AzQtComponents::ToolTipTwins::Guard tips;
        AzQtComponents::ToolTipTwins::StyleFilter filter(m_style.get());
        namespace Twins = AzQtComponents::ToolTipTwins;

        for (const bool inBox : { true, false })
        {
            SCOPED_TRACE(inBox ? "a label in the box" : "a label beside the box");

            // The label as the legacy GUI styled it: the form's sheets on the box and on the label, the application's rules above
            Twins::TipFacts legacy;
            {
                QWidget root;
                QWidget* box = InspectorFlat::makePart(InspectorFlat::Part::Box, &root);
                auto* label = new QLabel(QStringLiteral("Entity ID"), inBox ? box : &root);
                label->setObjectName(QStringLiteral("m_entityIdLabel"));
                label->setToolTip(QStringLiteral("Tip"));
                root.setStyle(m_fusion.get());
                box->setStyle(m_fusion.get());
                label->setStyle(m_fusion.get());
                box->setStyleSheet(QString::fromLatin1(InspectorFlat::DarkBoxSheet));
                if (inBox)
                {
                    label->setStyleSheet(QString::fromLatin1(InspectorFlat::DarkBoxPartSheet));
                }
                root.setStyleSheet(QString::fromLatin1(Twins::AppRules));
                root.ensurePolished();
                label->ensurePolished();
                legacy = Twins::showAndReadTip(label, nullptr);
            }

            // The same in the flattened GUI: below an inspector, with no sheet. The font and the margin of the label are the legacy
            // tip's: this process has the platform's fonts and style, and what a fill proves is not a font
            Twins::TipFacts flat;
            {
                InspectorFlat::ClassNamedWidget inspector(InspectorFlat::InspectorClass);
                QWidget* box = InspectorFlat::makePart(InspectorFlat::Part::Box, &inspector);
                auto* label = new QLabel(QStringLiteral("Entity ID"), inBox ? box : &inspector);
                label->setObjectName(QStringLiteral("m_entityIdLabel"));
                label->setToolTip(QStringLiteral("Tip"));
                flat = Twins::showAndReadTip(label, m_style.get(), &legacy.m_font, legacy.m_margin);
            }

            ASSERT_TRUE(legacy.m_found && flat.m_found);
            EXPECT_EQ(legacy.m_base.name(), inBox ? QStringLiteral("#333333") : QStringLiteral("#000000"));
            EXPECT_EQ(flat.m_base.name(), legacy.m_base.name());
            EXPECT_EQ(flat.m_text.name(), legacy.m_text.name());
            EXPECT_EQ(Twins::marginsText(flat.m_contents), Twins::marginsText(legacy.m_contents));
            EXPECT_EQ(flat.m_hint, legacy.m_hint);
            EXPECT_EQ(flat.m_contentsRect, legacy.m_contentsRect);
            for (int i = 0; i < 3; ++i)
            {
                EXPECT_EQ(Twins::differingPixels(legacy.m_images[i], flat.m_images[i]), 0) << "at ratio " << Twins::Ratios[i];
            }
        }
    }

    TEST_F(EntityPropertyEditorFlatSheetTests, ToolTip_ATipUpForTheBoxIsNotKeptForALabelBesideIt)
    {
        AzQtComponents::ToolTipTwins::Guard tips;
        AzQtComponents::ToolTipTwins::StyleFilter filter(m_style.get());
        namespace Twins = AzQtComponents::ToolTipTwins;

        InspectorFlat::ClassNamedWidget inspector(InspectorFlat::InspectorClass);
        QWidget* box = InspectorFlat::makePart(InspectorFlat::Part::Box, &inspector);
        auto* inside = new QLabel(QStringLiteral("Entity ID"), box);
        inside->setObjectName(QStringLiteral("m_entityIdLabel"));
        inside->setToolTip(QStringLiteral("Tip"));
        auto* beside = new QLabel(QStringLiteral("Beside"), &inspector);
        beside->setObjectName(QStringLiteral("m_entityNameLabel"));
        beside->setToolTip(QStringLiteral("Tip"));

        const QPoint local(2, 2);
        QHelpEvent insideHelp(QEvent::ToolTip, local, inside->mapToGlobal(local));
        QApplication::sendEvent(inside, &insideHelp);
        QLabel* first = Twins::findTip();
        ASSERT_NE(first, nullptr);
        first->setStyle(m_style.get());
        EXPECT_EQ(first->palette().color(QPalette::Active, QPalette::ToolTipBase).name(), QStringLiteral("#333333"));

        // The pointer rests on the label beside the box while the tip is up: Qt would reuse the tip and keep the box's fill. The
        // Style hides it, so that the tip the label shows is new, and black
        QPointer<QLabel> firstTip(first);
        QHelpEvent besideHelp(QEvent::ToolTip, local, beside->mapToGlobal(local));
        QApplication::sendEvent(beside, &besideHelp);
        // Qt's new tip deletes the hidden one (QTipLabel::QTipLabel), so the old tip is gone and the visible one is the new one
        EXPECT_TRUE(firstTip.isNull()) << "the tip of the box is not the tip of the label beside it";
        QLabel* second = Twins::findTip();
        ASSERT_NE(second, nullptr);
        EXPECT_TRUE(second->isVisible());
        second->setStyle(m_style.get());
        EXPECT_EQ(second->palette().color(QPalette::Active, QPalette::ToolTipBase).name(), QStringLiteral("#000000"));
        delete second;
    }

    // ----------------------------------------------------------------------------------
    // The two sheets of the colour property controls that set a text style:
    //   - C047, PropertyColorCtrl.cpp:326: the colour field's text turns red while what is typed is not a colour (`color: red;`), and back
    //     with any other text (C048, the empty sheet);
    //   - C049, PropertyColorGradientCtrl.cpp:623: the banner of the gradient editor, built for an edit of several entities
    //     (`color: #ffcc00; font-weight: bold;`).
    // The legacy twin is Qt's own style sheet style over AzQtComponents::Style with the real application sheet and the sheet the real
    // class sets; the flat twin is the flattened Style with the text elements of the real class and no sheet at all (AzQtComponents
    // SheetTwins.h). The real classes are built; the colour field is told the text as typing tells it.
    // ----------------------------------------------------------------------------------

    namespace ColorSheetSitesFlatSheetTestsInternal
    {
        namespace Twins = AzQtComponents::SheetTwins;

        //! The test application carries StyleManager's legacy sheets, which the twins name their own of: none while a test runs
        class ApplicationSheetGuard
        {
        public:
            ApplicationSheetGuard()
                : m_sheet(qApp->styleSheet())
            {
                qApp->setStyleSheet(QString());
            }

            ~ApplicationSheetGuard()
            {
                qApp->setStyleSheet(m_sheet);
            }

            ApplicationSheetGuard(const ApplicationSheetGuard&) = delete;
            ApplicationSheetGuard& operator=(const ApplicationSheetGuard&) = delete;

        private:
            QString m_sheet;
        };

        class ColorSheetSitesFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            ApplicationSheetGuard m_applicationSheet;
            Twins::Context m_context;
        };

        // The field takes "red, green, blue" (PropertyColorCtrl::CreateTextEditValidator): a letter makes the text invalid
        constexpr const char* InvalidColorText = "12, x4";
        constexpr const char* ValidColorText = "12, 34, 56";
        constexpr const char* BannerText = "This edit will be written to every selected entity's gradient field.";

        // ------------------------------------------------------------------------------
        // C047: the colour field
        // ------------------------------------------------------------------------------

        QLineEdit* colorEditOf(QWidget* control)
        {
            return control->findChild<QLineEdit*>();
        }

        //! The field of a colour control, with a colour in it
        QWidget* makeColorEdit(QWidget* parent)
        {
            auto* control = new AzToolsFramework::PropertyColorCtrl(parent);
            QLineEdit* edit = colorEditOf(control);
            edit->setText(QString::fromLatin1(ValidColorText));
            return edit;
        }

        //! A text typed in the field: the control hears it as it hears typing (QLineEdit::textEdited), and decides whether the text is
        //! red. The field's own text stays a colour: its validator refuses invalid keystrokes, and a field holding an invalid text would
        //! show the line edit's error icon, which the control's sheet knows nothing of.
        void type(QWidget& widget, const char* text)
        {
            auto* edit = qobject_cast<QLineEdit*>(&widget);
            Q_EMIT edit->textEdited(QString::fromLatin1(text));
        }

        Twins::State typedInvalid()
        {
            return { "invalid text", [](QWidget& widget) { type(widget, InvalidColorText); },
                [](QWidget& widget) { type(widget, ValidColorText); } };
        }

        //! The state after the text turned red and valid text came back: the field looks as it did at rest
        Twins::State typedValidAgain()
        {
            return { "valid text again", [](QWidget& widget) { type(widget, ValidColorText); }, [](QWidget&) {} };
        }

        //! The text turned red and the field then disabled: the field's own sheet outranks the disabled colour of the application's
        //! (LineEdit.qss:31-35, a sheet set on the widget is deeper than the application's), so the text stays red over the disabled fill
        Twins::State typedInvalidThenDisabled()
        {
            return { "invalid text, then disabled",
                [](QWidget& widget)
                {
                    type(widget, InvalidColorText);
                    widget.setEnabled(false);
                },
                [](QWidget& widget)
                {
                    widget.setEnabled(true);
                    type(widget, ValidColorText);
                } };
        }

        Twins::Row colorEditRow()
        {
            return Twins::Row{
                .m_site = "C047 the colour field of a colour property",
                .m_make = &makeColorEdit,
                .m_realApplicationSheet = true,
                .m_states = { typedInvalid(), typedValidAgain(), Twins::States::disabled(), typedInvalidThenDisabled() },
                .m_compare = Twins::Compare::Geometry | Twins::Compare::Fonts | Twins::Compare::Palette | Twins::Compare::Pixels,
                .m_roles = { QPalette::Text, QPalette::PlaceholderText, QPalette::Base },
                // The size the control's layout gives the field. A state relays the field out in the legacy GUI (setting a sheet
                // updates its geometry) and not in the flattened one (Style::addClass repolishes it only), so both start there, as
                // the field in the inspector does
                .m_size = QSize(110, 18),
                .m_base = Twins::Base::AzQtComponentsStyle,
            };
        }

        // ------------------------------------------------------------------------------
        // C049: the banner
        // ------------------------------------------------------------------------------

        QLabel* bannerOf(QWidget* dialog)
        {
            for (QLabel* label : dialog->findChildren<QLabel*>())
            {
                if (label->text() == QString::fromLatin1(BannerText))
                {
                    return label;
                }
            }
            return nullptr;
        }

        //! The dialog of an edit of several entities, and its banner
        QWidget* makeBanner(QWidget* parent)
        {
            auto* dialog = new AzToolsFramework::ColorGradientEditorDialog(AZ::ColorGradient(), false, true, parent);
            return bannerOf(dialog);
        }

        Twins::Row bannerRow()
        {
            return Twins::Row{
                .m_site = "C049 the banner of the gradient editor for several entities",
                .m_make = &makeBanner,
                .m_realApplicationSheet = true,
                .m_states = { Twins::States::disabled() },
                .m_compare = Twins::Compare::Geometry | Twins::Compare::Fonts | Twins::Compare::Palette | Twins::Compare::Pixels,
                .m_roles = { QPalette::WindowText, QPalette::Text, QPalette::ButtonText },
                .m_size = QSize(420, 36),
                .m_base = Twins::Base::AzQtComponentsStyle,
            };
        }

        //! The widgets of this file's sites that set sheets in a rung: the control with its field told invalid text, and the dialog
        QStringList censusOfColorSites(AzQtComponents::GuiPerfFloor::Rung rung)
        {
            return AzQtComponents::SiteTwins::sheetLinesAt(rung,
                []
                {
                    QWidget host;
                    host.resize(400, 200);
                    auto* control = new AzToolsFramework::PropertyColorCtrl(&host);
                    auto* dialog = new AzToolsFramework::ColorGradientEditorDialog(AZ::ColorGradient(), false, true, &host);
                    host.setAttribute(Qt::WA_DontShowOnScreen);
                    host.show();
                    type(*colorEditOf(control), InvalidColorText);
                    for (QWidget* each : host.findChildren<QWidget*>())
                    {
                        each->ensurePolished();
                    }
                    Q_UNUSED(dialog);
                });
        }
    } // namespace ColorSheetSitesFlatSheetTestsInternal

    // A unity build joins this file with others of namespace UnitTest: the fixture is the only name brought in, the rest is named
    // through the alias
    using ColorSheetSitesFlatSheetTestsInternal::ColorSheetSitesFlatSheetTests;
    namespace ColorSheets = ColorSheetSitesFlatSheetTestsInternal;

    TEST_F(ColorSheetSitesFlatSheetTests, ColorEdit_TurnsRedWithInvalidTextAndBackAsUnderTheSheet)
    {
        // The flat twin sets no sheet (the harness flags one); its text turns red in the state the control sets, and the field is laid
        // out, filled, coloured and painted as the legacy field under `color: red;`, with the text of the field at rest before and
        // after, enabled and disabled
        EXPECT_TRUE(ColorSheets::Twins::check(ColorSheets::colorEditRow(), m_context));
    }

    TEST_F(ColorSheetSitesFlatSheetTests, ColorEdit_TheTextIsRedOnlyWhileTheTextIsInvalid)
    {
        // The twins agree on the colour; this says what the colour is: red in the state, the field's own colour at rest and after.
        // And what carries it: the legacy GUI sets the sheet while the text is invalid and takes it away with any other text; the
        // flattened GUI never sets one (the harness reads the flat twin for a sheet before the states, so the states are read here).
        const ColorSheets::Twins::Row row = ColorSheets::colorEditRow();
        ColorSheets::Twins::Context::registerOnce(row.m_register);
        for (const bool legacy : { true, false })
        {
            const char* gui = legacy ? "legacy" : "flat";
            ColorSheets::Twins::RungScope rung(legacy ? row.m_legacyRung : ColorSheets::Twins::Rung::Flat);
            const ColorSheets::Twins::Twin twin(row, m_context, legacy);
            const QColor atRest = twin.widget()->palette().color(QPalette::Active, QPalette::Text);
            EXPECT_NE(atRest, QColor(Qt::red)) << gui;
            EXPECT_TRUE(twin.widget()->styleSheet().isEmpty()) << gui;

            ColorSheets::type(*twin.widget(), ColorSheets::InvalidColorText);
            for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
            {
                EXPECT_EQ(twin.widget()->palette().color(group, QPalette::Text), QColor(Qt::red)) << gui;
            }
            EXPECT_EQ(twin.widget()->styleSheet().isEmpty(), !legacy) << gui;

            ColorSheets::type(*twin.widget(), ColorSheets::ValidColorText);
            EXPECT_EQ(twin.widget()->palette().color(QPalette::Active, QPalette::Text), atRest) << gui;
            EXPECT_TRUE(twin.widget()->styleSheet().isEmpty()) << gui;
        }
    }

    TEST_F(ColorSheetSitesFlatSheetTests, ColorEdit_OnlyTheFieldOfAColorControlTurnsRed)
    {
        // The state is the control's: the class tag on a line edit of another place gives it no colour
        ColorSheets::Twins::RungScope flat(ColorSheets::Twins::Rung::Flat);
        QWidget host;
        auto* control = new AzToolsFramework::PropertyColorCtrl(&host);     // registers the element
        auto* stray = new QLineEdit(&host);
        stray->setStyle(m_context.flatStyle());
        AzQtComponents::Style::addClass(stray, QStringLiteral("ColorEditInvalid"));
        stray->ensurePolished();
        EXPECT_NE(stray->palette().color(QPalette::Active, QPalette::Text), QColor(Qt::red));

        QLineEdit* field = ColorSheets::colorEditOf(control);
        field->setStyle(m_context.flatStyle());
        field->ensurePolished();
        AzQtComponents::Style::addClass(field, QStringLiteral("ColorEditInvalid"));
        EXPECT_EQ(field->palette().color(QPalette::Active, QPalette::Text), QColor(Qt::red));
    }

    TEST_F(ColorSheetSitesFlatSheetTests, Banner_HasTheYellowBoldTextOfItsSheet)
    {
        EXPECT_TRUE(ColorSheets::Twins::check(ColorSheets::bannerRow(), m_context));
    }

    TEST_F(ColorSheetSitesFlatSheetTests, Banner_IsTheBannerOfAnEditOfSeveralEntitiesAlone)
    {
        // An edit of one entity builds no banner, and another label of the dialog is not taken for it
        ColorSheets::Twins::RungScope flat(ColorSheets::Twins::Rung::Flat);
        QWidget host;
        auto* single = new AzToolsFramework::ColorGradientEditorDialog(AZ::ColorGradient(), false, false, &host);
        EXPECT_TRUE(ColorSheets::bannerOf(single) == nullptr);

        auto* several = new AzToolsFramework::ColorGradientEditorDialog(AZ::ColorGradient(), false, true, &host);
        QLabel* banner = ColorSheets::bannerOf(several);
        ASSERT_TRUE(banner != nullptr);
        banner->setStyle(m_context.flatStyle());
        banner->ensurePolished();
        EXPECT_EQ(banner->palette().color(QPalette::Active, QPalette::WindowText), QColor(0xFF, 0xCC, 0x00));
        EXPECT_EQ(banner->font().weight(), QFont::Bold);

        for (QLabel* other : several->findChildren<QLabel*>())
        {
            if (other == banner)
            {
                continue;
            }
            other->setStyle(m_context.flatStyle());
            other->ensurePolished();
            EXPECT_NE(other->palette().color(QPalette::Active, QPalette::WindowText), QColor(0xFF, 0xCC, 0x00))
                << other->text().toStdString();
        }
    }

    TEST_F(ColorSheetSitesFlatSheetTests, Sheets_TheLegacyGuiSetsThemAndTheFlattenedGuiNone)
    {
        const QStringList legacy = ColorSheets::censusOfColorSites(AzQtComponents::GuiPerfFloor::Rung::Qss);
        bool foundField = false;
        bool foundBanner = false;
        for (const QString& line : legacy)
        {
            foundField = foundField
                || (AzQtComponents::SiteTwins::namesCarrier(line, QStringLiteral("QLineEdit"))
                    && line.contains(QStringLiteral("color: #ff0000;")));
            foundBanner = foundBanner
                || (AzQtComponents::SiteTwins::namesCarrier(line, QStringLiteral("QLabel"))
                    && line.contains(QStringLiteral("color: #ffcc00; font-weight: bold;")));
        }
        EXPECT_TRUE(foundField) << legacy.join("\n").toStdString();
        EXPECT_TRUE(foundBanner) << legacy.join("\n").toStdString();

        const QStringList flat = ColorSheets::censusOfColorSites(AzQtComponents::GuiPerfFloor::Rung::Flat);
        EXPECT_TRUE(flat.isEmpty()) << flat.join("\n").toStdString();
    }
} // namespace UnitTest
