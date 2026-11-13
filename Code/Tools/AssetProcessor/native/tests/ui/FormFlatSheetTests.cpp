/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <native/ui/FlatElements.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/Style.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QFont>
#include <QFontInfo>
#include <QFontMetrics>
#include <QFrame>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QStringList>
#include <QStyleFactory>
#include <QStyleOption>
#include <QWidget>

#include <iterator>
#include <string>
#include <utility>

// ----------------------------------------------------------------------------------
// The inline sheets of the .ui forms of the main window and its detail panels, and of MainWindow.cpp (native/ui/FlatElements.cpp,
// "The inline sheets"): which widgets the elements reach, and that they give what the sheets gave (Qt's own style sheet style over
// Fusion, with the same sheet and the rules of the application sheet that reach the widget) at the device ratios the GUI runs at.
// The main window and the panels need the asset database, so the trees here are look-alikes built from the names of the forms
// (MainWindow.ui, ProductAssetDetailsPanel.ui, SourceAssetDetailsPanel.ui), under a widget that Qt takes for the class of the form.
// ----------------------------------------------------------------------------------

namespace AssetProcessor
{
    namespace FormFlatSheetTestsInternal
    {
        using AzQtComponents::BoxElement;
        using AzQtComponents::GuiPerfFloor;
        using AzQtComponents::TextElement;

        // ------------------------------------------------------------------------------
        // The forms' classes, and the rules of the application sheet that reach their widgets (the legacy twin's)
        // ------------------------------------------------------------------------------

        constexpr const char* MainWindowClass = "MainWindow";
        constexpr const char* ProductPanelClass = "AssetProcessor::ProductAssetDetailsPanel";
        constexpr const char* SourcePanelClass = "AssetProcessor::SourceAssetDetailsPanel";

        constexpr const char* DividerElementName = "AssetProcessor.Box.SettingsDivider";
        constexpr const char* RemoveButtonElementName = "AssetProcessor.Box.RemovePatternButton";

        // BaseStyleSheet.qss:15-21
        constexpr const char* UniversalRule =
            "* { background-color: transparent; color: white; font-family: \"Open Sans\"; font-weight: 400; }";
        // BaseStyleSheet.qss:40-48
        constexpr const char* FrameRule = ".QFrame[frameShape=\"1\"], .QFrame[frameShape=\"2\"], .QFrame[frameShape=\"4\"], "
                                          ".QFrame[frameShape=\"5\"], .QFrame[frameShape=\"6\"] { background-color: #444444; }";
        // Text.qss:64-70
        constexpr const char* TitleRule = ".Title { font-family: \"Open Sans\"; font-size: 18px; margin-top: 7px; margin-bottom: 7px; }";
        // Text.qss:89-95
        constexpr const char* LabelRule =
            ".Label, QLabel { font-family: \"Open Sans\"; font-size: 12px; margin-top: 1px; margin-bottom: 1px; }";
        // PushButton.qss:30-52
        constexpr const char* ButtonRules = "QPushButton { color: #ffffff; font-family: \"Open Sans\"; font-size: 12px; } "
                                            "QPushButton:hover, QPushButton:pressed { color: #FFFFFF; } "
                                            "QPushButton:disabled { color: #999999; } "
                                            "QPushButton:flat { border: none; background-color: transparent; }";

        QString applicationRules()
        {
            return QStringList{ QString::fromLatin1(UniversalRule), QString::fromLatin1(FrameRule), QString::fromLatin1(TitleRule),
                                QString::fromLatin1(LabelRule), QString::fromLatin1(ButtonRules) }
                .join(QLatin1Char('\n'));
        }

        constexpr QRgb ParentColor = 0x808080;          // what neither the sheet nor the element draws stays this grey

        // ------------------------------------------------------------------------------
        // Helpers
        // ------------------------------------------------------------------------------

        //! A widget that Qt takes for an instance of a class it is not. The matchers ask for the form by its class name, as the
        //! selectors do, and building the real window or panel needs the asset database.
        class ClassNamedForm
            : public QWidget
        {
        public:
            explicit ClassNamedForm(const char* className, QWidget* parent = nullptr)
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

        //! What a frame paints for its box through its own style, in the order Qt paints it: the styled background (PE_Widget), then
        //! the frame (CE_ShapedFrame). The image is the painter's device, so the style scales the box to the ratio.
        QImage paintBox(const QFrame& frame, qreal ratio)
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

        //! What a widget paints (its own paint events, children included) onto an image of the ratio
        QImage renderWidget(QWidget& widget, qreal ratio)
        {
            QImage image((QSizeF(widget.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor::fromRgb(ParentColor));
            widget.render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
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

        //! The facts of a font that decide how text lays out and looks: the requested family, the weight, the slant, the size as it was
        //! requested (points or pixels: the sheet's `12pt` stays 12 points, whatever the screen's logical DPI makes of it) and what the
        //! font becomes in pixels
        std::string describeFont(const QFont& font)
        {
            const QFontInfo info(font);
            return "family '" + font.family().toStdString() + "', weight " + std::to_string(static_cast<int>(font.weight())) + ", "
                + (font.italic() ? "italic" : "upright") + ", requested " + std::to_string(font.pointSizeF()) + "pt "
                + std::to_string(font.pixelSize()) + "px, " + std::to_string(info.pixelSize()) + "px, line "
                + std::to_string(QFontMetrics(font).height());
        }

        //! The contents box of a frame (SE_ShapedFrameContents): where a label lays out its text, and whether it indents it
        QRect frameContents(const QFrame& frame)
        {
            const QStyleOptionFrame option = frameOption(&frame);
            return frame.style()->subElementRect(QStyle::SE_ShapedFrameContents, &option, &frame);
        }

        const char* boxElementOf(const QWidget& widget)
        {
            const BoxElement* element = BoxElement::forWidget(&widget);
            return element ? element->m_name : "";
        }

        // ------------------------------------------------------------------------------
        // The labels: what each form gave each of them
        // ------------------------------------------------------------------------------

        //! One label as its form built it: the font its <font> gave it (before the sheet), its class tag, and its sheet
        struct LabelCase
        {
            const char* m_owner;            //!< the class of the form that owns it
            const char* m_name;
            const char* m_sheet;            //!< InlineSheets
            const char* m_tag;              //!< the class property, or none
            bool m_hasCodeFont;
            const char* m_codeFamily;       //!< the family of the <font>, or none
            int m_codePointSize;            //!< its point size, or 0
            bool m_codeBold;
            bool m_pixmap;                  //!< a label with no text and a pixmap scaled to its fixed size (the error icon)
        };

        // One label of each kind of sheet. The fonts are those of the forms (the line numbers are those of the forms before the
        // flattening): MainWindow.ui:1326-1332 (bold), ProductAssetDetailsPanel.ui:31-39 (family "16pt", 12 points; the label is a
        // Title) and :444-452 (family "12pt", 8 points, bold). The labels of ProductAssetDetailsPanel.ui that have none:
        // platformTitleLabel (:296), outgoingProductDependenciesValueLabel (:470) and missingDependencyErrorIcon (:760: a label
        // without text, 15 by 19, a pixmap scaled to it).
        const LabelCase LabelCases[] = {
            { MainWindowClass, "builderInfoHeaderTitleName", InlineSheets::BoldFont, nullptr, true, nullptr, 0, true, false },
            { ProductPanelClass, "platformTitleLabel", InlineSheets::BoldFont, nullptr, false, nullptr, 0, false, false },
            { ProductPanelClass, "assetNameLabel", InlineSheets::PointFont, "Title", true, "16pt", 12, false, false },
            { ProductPanelClass, "outgoingProductDependenciesValueLabel", InlineSheets::PointFont, nullptr, false, nullptr, 0, false,
              false },
            { ProductPanelClass, "outgoingProductDependenciesTitleLabel", InlineSheets::BoldPointFont, nullptr, true, "12pt", 8, true,
              false },
            { ProductPanelClass, "missingDependencyErrorIcon", InlineSheets::NoPadding, nullptr, false, nullptr, 0, false, true },
        };

        //! The case of that name, for the tests that look at one
        const LabelCase& labelCaseNamed(const char* name)
        {
            for (const LabelCase& labelCase : LabelCases)
            {
                if (qstrcmp(labelCase.m_name, name) == 0)
                {
                    return labelCase;
                }
            }
            ADD_FAILURE() << "no label case " << name;
            return LabelCases[0];
        }

        //! What an element gives a label, told apart by its font
        enum Look
        {
            LookPlain,              //!< no element of the forms
            LookBold,               //!< `font: bold;`
            LookPoint,              //!< `font: 12pt;`
            LookBoldWithComma,      //!< `font: bold, 12pt;`
        };

        Look lookOfFont(const QFont& font)
        {
            if (font.family() == QStringLiteral("12pt") && font.weight() == QFont::Bold && font.pixelSize() == 12)
            {
                return LookBoldWithComma;
            }
            if (font.family() == QStringLiteral("Open Sans") && font.weight() == QFont::Bold && font.pixelSize() == 12)
            {
                return LookBold;
            }
            // 12 points as the sheet gave them, not a pixel size worked out from them
            if (font.family() == QStringLiteral("Open Sans") && font.weight() == QFont::Normal && qFuzzyCompare(font.pointSizeF(), 12.0))
            {
                return LookPoint;
            }
            return LookPlain;
        }

        Look lookOf(const QLabel& label)
        {
            return lookOfFont(label.font());
        }

        // ------------------------------------------------------------------------------
        // Fixture: an application with the fonts the GUI has, the flat rung, and the elements registered
        // ------------------------------------------------------------------------------

        class FormFlatSheetTests
            : public ::UnitTest::LeakDetectionFixture
        {
        public:
            FormFlatSheetTests()
            {
                // Before the application: its construction is where Qt warns (see FlatElementsTests)
                s_previousHandler = qInstallMessageHandler(&FormFlatSheetTests::messageHandler);
                if (QCoreApplication::instance() == nullptr)
                {
                    m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);

                    // The application font (StyleManager.cpp) and the class fonts of the flattened GUI, which the labels without an
                    // element of the forms take (the label rule: Open Sans, 12px)
                    QFont applicationFont(QStringLiteral("Open Sans"));
                    applicationFont.setPixelSize(12);
                    QApplication::setFont(applicationFont);
                    TextElement::installClassFonts();
                }
            }

            ~FormFlatSheetTests() override
            {
                m_application.reset();
                qInstallMessageHandler(s_previousHandler);
            }

            void SetUp() override
            {
                LeakDetectionFixture::SetUp();

                // The test application carries no sheet of StyleManager's: the twins name their own
                m_applicationSheet = qApp->styleSheet();
                qApp->setStyleSheet(QString());

                // The flattened GUI's branches of Style read the rung
                GuiPerfFloor::setRungForTesting(GuiPerfFloor::Rung::Flat);

                m_style = AZStd::make_unique<AzQtComponents::Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                m_fusion = AZStd::unique_ptr<QStyle>(QStyleFactory::create(QStringLiteral("Fusion")));

                // The registries last for the process: the registrar ignores a second call
                RegisterFlatElements();
            }

            void TearDown() override
            {
                m_fusion.reset();
                m_style.reset();
                GuiPerfFloor::resetForTesting();
                qApp->setStyleSheet(m_applicationSheet);
                LeakDetectionFixture::TearDown();
            }

            //! The test environment fails a test on any Qt warning (BaseAssetProcessorTest.h). From the second application of the
            //! process on, Qt's Windows platform warns that the window class of its theme observer exists already: Qt's own, nothing
            //! the elements did. That one warning is let through; every other message goes to the environment's handler.
            static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
            {
                if (type == QtWarningMsg && message.contains(QLatin1String("ThemeChangeObserverWindow")))
                {
                    return;
                }
                if (s_previousHandler)
                {
                    s_previousHandler(type, context, message);
                }
            }

            static inline QtMessageHandler s_previousHandler = nullptr;

            // ---- The flat way: the style on the widget, and the elements resolved as Style::polish does in the flattened GUI
            void flatten(QWidget& widget) const
            {
                widget.setStyle(m_style.get());
                TextElement::resolve(&widget);
                TextElement::applyColor(&widget);
                BoxElement::resolve(&widget);
            }

            // ---- Labels
            static QLabel* makeLabel(const LabelCase& labelCase, QWidget* parent)
            {
                auto* label = new QLabel(QStringLiteral("Label text"), parent);
                label->setObjectName(QString::fromLatin1(labelCase.m_name));
                if (labelCase.m_tag)
                {
                    label->setProperty("class", QString::fromLatin1(labelCase.m_tag));
                }
                if (labelCase.m_hasCodeFont)
                {
                    QFont codeFont;
                    if (labelCase.m_codeFamily)
                    {
                        codeFont.setFamily(QString::fromLatin1(labelCase.m_codeFamily));
                    }
                    if (labelCase.m_codePointSize > 0)
                    {
                        codeFont.setPointSize(labelCase.m_codePointSize);
                    }
                    codeFont.setItalic(false);
                    codeFont.setBold(labelCase.m_codeBold);
                    label->setFont(codeFont);
                }
                if (labelCase.m_pixmap)
                {
                    // ProductAssetDetailsPanel.ui:695-726: no text, a pixmap scaled to the fixed size of the label. A pixmap of two
                    // colours, so a shift of the picture shows in the pixels.
                    QPixmap pixmap(15, 19);
                    pixmap.fill(QColor(200, 40, 40));
                    {
                        QPainter painter(&pixmap);
                        painter.fillRect(0, 0, 7, 19, QColor(40, 40, 200));
                    }
                    label->setText(QString());
                    label->setPixmap(pixmap);
                    label->setScaledContents(true);
                    label->setFixedSize(15, 19);
                }
                else
                {
                    label->resize(180, 24);
                }
                return label;
            }

            //! The label as the legacy GUI styled it: Qt's style sheet style over Fusion, the form's sheet on the label and the rules of
            //! the application sheet above it. The callback gets the polished label.
            void withLegacyLabel(const LabelCase& labelCase, const AZStd::function<void(QLabel&)>& use) const
            {
                QWidget root;
                QLabel* label = makeLabel(labelCase, &root);
                root.setStyle(m_fusion.get());
                label->setStyle(m_fusion.get());
                label->setStyleSheet(QString::fromLatin1(labelCase.m_sheet));
                root.setStyleSheet(applicationRules());
                root.ensurePolished();
                label->ensurePolished();
                use(*label);
            }

            //! The same label in the flattened GUI: below the form of its class, with its elements
            void withFlatLabel(const LabelCase& labelCase, const AZStd::function<void(QLabel&)>& use) const
            {
                ClassNamedForm form(labelCase.m_owner);
                auto* container = new QWidget(&form);
                QLabel* label = makeLabel(labelCase, container);
                flatten(*label);
                use(*label);
            }

            //! The look of a label of that name, in a container, below a form of that class (none: below no form)
            Look lookBelow(const char* formClass, const char* name) const
            {
                QWidget plain;
                ClassNamedForm form(formClass ? formClass : "NoForm");
                QWidget* root = formClass ? static_cast<QWidget*>(&form) : &plain;
                auto* container = new QWidget(root);
                auto* label = new QLabel(QStringLiteral("x"), container);
                label->setObjectName(QString::fromLatin1(name));
                flatten(*label);
                return lookOf(*label);
            }

            //! The look of the menu a label of that name opens (QLabel::contextMenuEvent makes a QMenu with the label as its parent),
            //! below a form of that class (none: below no form)
            Look menuLookBelow(const char* formClass, const char* name) const
            {
                QWidget plain;
                ClassNamedForm form(formClass ? formClass : "NoForm");
                QWidget* root = formClass ? static_cast<QWidget*>(&form) : &plain;
                auto* container = new QWidget(root);
                auto* label = new QLabel(QStringLiteral("x"), container);
                label->setObjectName(QString::fromLatin1(name));
                QMenu menu(label);
                flatten(menu);
                return lookOfFont(menu.font());
            }

            // ---- Dividers: the form's line, a sunken horizontal QFrame of 3 pixels
            static QFrame* makeDivider(const char* name, QWidget* parent)
            {
                auto* line = new QFrame(parent);
                line->setObjectName(QString::fromLatin1(name));
                line->setFrameShape(QFrame::HLine);
                line->setFrameShadow(QFrame::Sunken);
                line->resize(120, 3);
                return line;
            }

            QImage legacyDivider(qreal ratio) const
            {
                QWidget root;
                QFrame* line = makeDivider("line", &root);
                root.setStyle(m_fusion.get());
                line->setStyle(m_fusion.get());
                line->setStyleSheet(QString::fromLatin1(InlineSheets::Divider));
                root.setStyleSheet(applicationRules());
                root.ensurePolished();
                line->ensurePolished();
                return paintBox(*line, ratio);
            }

            QImage flatDivider(qreal ratio) const
            {
                ClassNamedForm form(MainWindowClass);
                auto* page = new QWidget(&form);
                page->setObjectName(QStringLiteral("SettingsPage"));
                QFrame* line = makeDivider("line", page);
                flatten(*line);
                return paintBox(*line, ratio);
            }

            // ---- The remove button of a pattern: flat, with the icon of the table's cell
            static QPushButton* makeRemoveButton(QWidget* parent)
            {
                QPixmap pixmap(14, 14);
                pixmap.fill(QColor(200, 40, 40));
                auto* button = new QPushButton(parent);
                button->setFlat(true);
                button->setIcon(QIcon(pixmap));
                button->setIconSize(QSize(14, 14));
                button->resize(16, 22);
                return button;
            }

            void withLegacyButton(const AZStd::function<void(QPushButton&)>& use) const
            {
                QWidget root;
                QPushButton* button = makeRemoveButton(&root);
                root.setStyle(m_fusion.get());
                button->setStyle(m_fusion.get());
                button->setStyleSheet(QString::fromLatin1(InlineSheets::RemovePatternButton));
                root.setStyleSheet(applicationRules());
                root.ensurePolished();
                button->ensurePolished();
                use(*button);
            }

            void withFlatButton(const AZStd::function<void(QPushButton&)>& use) const
            {
                ClassNamedForm form(MainWindowClass);
                auto* table = new QWidget(&form);
                table->setObjectName(QStringLiteral("sharedCacheTable"));
                auto* viewport = new QWidget(table);
                QPushButton* button = makeRemoveButton(viewport);
                flatten(*button);
                use(*button);
            }

            //! The contents box of a push button (SE_PushButtonContents): where its icon sits
            static QRect buttonContents(const QPushButton& button)
            {
                QStyleOptionButton option;
                option.initFrom(&button);
                option.features = QStyleOptionButton::Flat;
                option.iconSize = button.iconSize();
                return button.style()->subElementRect(QStyle::SE_PushButtonContents, &option, &button);
            }

        protected:
            int m_argc = 1;                                     // QApplication keeps a reference to it
            char m_programName[5] = "test";
            char* m_argv[2] = { m_programName, nullptr };
            AZStd::unique_ptr<QApplication> m_application;
            QString m_applicationSheet;
            AZStd::unique_ptr<AzQtComponents::Style> m_style;
            AZStd::unique_ptr<QStyle> m_fusion;
        };

        // ------------------------------------------------------------------------------
        // The helper: the legacy GUI sets the form's sheet, the flattened GUI none
        // ------------------------------------------------------------------------------

        TEST_F(FormFlatSheetTests, Helper_SetsTheFormsSheetInTheLegacyGuiAndNothingInTheFlattenedGui)
        {
            for (const char* sheet : { InlineSheets::Divider, InlineSheets::PanelFill, InlineSheets::BoldFont, InlineSheets::PointFont,
                                       InlineSheets::BoldPointFont, InlineSheets::NoPadding, InlineSheets::RemovePatternButton,
                                       InlineSheets::SharedCacheStatus })
            {
                GuiPerfFloor::resetForTesting();
                GuiPerfFloor::setRungForTesting(GuiPerfFloor::Rung::Qss);
                QLabel legacy;
                SetLegacySheet(&legacy, sheet);
                EXPECT_EQ(legacy.styleSheet(), QString::fromLatin1(sheet));

                GuiPerfFloor::resetForTesting();
                GuiPerfFloor::setRungForTesting(GuiPerfFloor::Rung::Flat);
                QLabel flat;
                SetLegacySheet(&flat, sheet);
                EXPECT_TRUE(flat.styleSheet().isEmpty()) << sheet;
                EXPECT_FALSE(flat.style()->inherits("QStyleSheetStyle")) << sheet;
            }
        }

        // ------------------------------------------------------------------------------
        // Registry: the elements reach the widgets the sheets were set on, and no other
        // ------------------------------------------------------------------------------

        TEST_F(FormFlatSheetTests, Registry_TheBoldLabelsOfTheThreeFormsHaveTheBoldFontAndOnlyThose)
        {
            const char* const mainWindow[] = { "builderInfoHeaderTitleName", "builderInfoDetailsTitleVersionNumber",
                "builderInfoDetailsTitleFingerprint", "builderInfoDetailsTitleType", "builderInfoDetailsTitleBusId" };
            const char* const product[] = { "platformTitleLabel", "productAssetIdTitleLabel", "sourceAssetTitleLabel",
                "lastTimeProcessedTitleLabel", "jobKeyTitleLabel" };
            const char* const source[] = { "scanFolderTitleLabel", "sourceGuidTitleLabel", "sourceAssetTitleLabel" };

            const auto expectBold = [this](const char* formClass, const char* const* names, size_t count)
            {
                for (size_t i = 0; i < count; ++i)
                {
                    EXPECT_EQ(lookBelow(formClass, names[i]), LookBold) << formClass << " " << names[i];
                    // the same name below no form, or below another form, is a plain label
                    EXPECT_EQ(lookBelow(nullptr, names[i]), LookPlain) << names[i];
                    EXPECT_EQ(lookBelow("AssetProcessor::OtherForm", names[i]), LookPlain) << names[i];
                }
            };
            expectBold(MainWindowClass, mainWindow, std::size(mainWindow));
            expectBold(ProductPanelClass, product, std::size(product));
            expectBold(SourcePanelClass, source, std::size(source));

            // A name of one form below the class of another is not its sheet's (sourceAssetTitleLabel is in both panels)
            EXPECT_EQ(lookBelow(ProductPanelClass, "scanFolderTitleLabel"), LookPlain);
            EXPECT_EQ(lookBelow(SourcePanelClass, "platformTitleLabel"), LookPlain);
            EXPECT_EQ(lookBelow(MainWindowClass, "platformTitleLabel"), LookPlain);
            EXPECT_EQ(lookBelow(ProductPanelClass, "builderInfoHeaderTitleName"), LookPlain);
        }

        TEST_F(FormFlatSheetTests, Registry_ThePointSizeLabelsOfTheTwoPanelsHaveThePointSizeFontAndOnlyThose)
        {
            const char* const product[] = { "assetNameLabel", "outgoingProductDependenciesValueLabel",
                "outgoingUnmetPathProductDependenciesValueLabel", "incomingProductDependenciesValueLabel",
                "MissingProductDependenciesValueLabel" };
            for (const char* name : product)
            {
                EXPECT_EQ(lookBelow(ProductPanelClass, name), LookPoint) << name;
                EXPECT_EQ(lookBelow(nullptr, name), LookPlain) << name;
                EXPECT_EQ(lookBelow(MainWindowClass, name), LookPlain) << name;
            }

            // The source panel has the one, assetNameLabel (SourceAssetDetailsPanel.ui:41 before the flattening), and not the value
            // labels of the product panel
            EXPECT_EQ(lookBelow(SourcePanelClass, "assetNameLabel"), LookPoint);
            EXPECT_EQ(lookBelow(SourcePanelClass, "outgoingProductDependenciesValueLabel"), LookPlain);
        }

        TEST_F(FormFlatSheetTests, Registry_TheTitleLabelsOfTheProductPanelHaveTheFontTheShorthandWithACommaGave)
        {
            const char* const product[] = { "MissingProductDependenciesFolderTitleLabel", "outgoingProductDependenciesTitleLabel",
                "outgoingUnmetPathProductDependenciesTitleLabel", "incomingProductDependenciesTitleLabel",
                "MissingProductDependenciesTitleLabel" };
            for (const char* name : product)
            {
                EXPECT_EQ(lookBelow(ProductPanelClass, name), LookBoldWithComma) << name;
                EXPECT_EQ(lookBelow(SourcePanelClass, name), LookPlain) << name;
                EXPECT_EQ(lookBelow(nullptr, name), LookPlain) << name;
            }
        }

        TEST_F(FormFlatSheetTests, Registry_TheContextMenuOfThePointSizeValueLabelsHasTheirFontAndNoOtherMenuDoes)
        {
            // The sheet of these labels has no selector: it reached the Copy / Select All menu the selectable ones open
            for (const char* name : { "outgoingProductDependenciesValueLabel", "outgoingUnmetPathProductDependenciesValueLabel",
                                      "incomingProductDependenciesValueLabel", "MissingProductDependenciesValueLabel" })
            {
                EXPECT_EQ(menuLookBelow(ProductPanelClass, name), LookPoint) << name;
                EXPECT_EQ(menuLookBelow(SourcePanelClass, name), LookPlain) << name;
                EXPECT_EQ(menuLookBelow(nullptr, name), LookPlain) << name;
            }

            // The menus of labels whose sheet is not `font: 12pt;`, and of labels with no sheet
            for (const char* name : { "jobKeyValueLabel", "platformTitleLabel", "outgoingProductDependenciesTitleLabel" })
            {
                EXPECT_EQ(menuLookBelow(ProductPanelClass, name), LookPlain) << name;
            }
        }

        TEST_F(FormFlatSheetTests, Registry_TheFiveDividersOfTheSettingsPageHaveTheFillAndNoOtherFrameDoes)
        {
            ClassNamedForm form(MainWindowClass);
            auto* stack = new QWidget(&form);
            stack->setObjectName(QStringLiteral("dialogStack"));
            auto* settings = new QWidget(stack);
            settings->setObjectName(QStringLiteral("SettingsPage"));
            auto* logs = new QWidget(stack);
            logs->setObjectName(QStringLiteral("LogDialog"));
            auto* deeper = new QWidget(settings);

            for (const char* name : { "line", "line_2", "line_3", "line_4", "line_5" })
            {
                QFrame* divider = FormFlatSheetTests::makeDivider(name, settings);
                flatten(*divider);
                EXPECT_STREQ(boxElementOf(*divider), DividerElementName) << name;
                EXPECT_TRUE(divider->testAttribute(Qt::WA_StyledBackground)) << name;       // the fill is drawn at PE_Widget
            }

            // The page's other frames, the same name on another page, below a widget of the page, below another class of window, alone
            ClassNamedForm otherForm("AssetProcessor::OtherForm");
            auto* otherSettings = new QWidget(&otherForm);
            otherSettings->setObjectName(QStringLiteral("SettingsPage"));
            QWidget loose;
            const std::pair<const char*, QWidget*> strangers[] = { { "line_6", settings }, { "line", logs }, { "line", deeper },
                                                                   { "line", otherSettings }, { "line", &loose } };
            for (const auto& [name, parent] : strangers)
            {
                QFrame* stranger = FormFlatSheetTests::makeDivider(name, parent);
                flatten(*stranger);
                EXPECT_STRNE(boxElementOf(*stranger), DividerElementName) << name;
            }
        }

        TEST_F(FormFlatSheetTests, Registry_TheRemoveButtonsOfTheSharedCacheTableHaveTheElementAndNoOtherPushButtonDoes)
        {
            ClassNamedForm form(MainWindowClass);
            auto* table = new QWidget(&form);
            table->setObjectName(QStringLiteral("sharedCacheTable"));
            auto* viewport = new QWidget(table);
            QPushButton* remove = FormFlatSheetTests::makeRemoveButton(viewport);
            flatten(*remove);
            EXPECT_STREQ(boxElementOf(*remove), RemoveButtonElementName);

            // A push button of the window outside the table, the table's name below another class of window, a button alone
            auto* elsewhere = FormFlatSheetTests::makeRemoveButton(&form);
            ClassNamedForm otherForm("AssetProcessor::OtherForm");
            auto* otherTable = new QWidget(&otherForm);
            otherTable->setObjectName(QStringLiteral("sharedCacheTable"));
            auto* otherButton = FormFlatSheetTests::makeRemoveButton(otherTable);
            QPushButton alone;
            for (QPushButton* stranger : { elsewhere, otherButton, &alone })
            {
                flatten(*stranger);
                EXPECT_STRNE(boxElementOf(*stranger), RemoveButtonElementName);
            }
        }

        // ------------------------------------------------------------------------------
        // Fonts: the label has the font its sheet gave it
        // ------------------------------------------------------------------------------

        TEST_F(FormFlatSheetTests, Fonts_EachLabelHasTheFontItsSheetGaveIt)
        {
            for (const LabelCase& labelCase : LabelCases)
            {
                std::string legacy;
                std::string flat;
                withLegacyLabel(labelCase, [&legacy](QLabel& label) { legacy = describeFont(label.font()); });
                withFlatLabel(labelCase, [&flat](QLabel& label) { flat = describeFont(label.font()); });
                EXPECT_EQ(flat, legacy) << labelCase.m_name;
            }
        }

        TEST_F(FormFlatSheetTests, Fonts_TheShorthandWithACommaMakesTheWordAfterItTheFamily)
        {
            // Why FormBoldWithComma has the family "12pt": if Qt reads the shorthand otherwise, the legacy GUI's titles change, and the
            // element with them
            QFont legacy;
            withLegacyLabel(LabelCases[4], [&legacy](QLabel& label) { legacy = label.font(); });

            EXPECT_EQ(legacy.family(), QStringLiteral("12pt"));
            EXPECT_EQ(legacy.pixelSize(), 12);
            EXPECT_EQ(static_cast<int>(legacy.weight()), static_cast<int>(QFont::Bold));
        }

        TEST_F(FormFlatSheetTests, Fonts_TheContextMenuOfAValueLabelHasTheFontItsSheetGaveIt)
        {
            // QLabel::contextMenuEvent opens a QMenu with the label as its parent. The label's sheet has no selector, so under
            // QStyleSheetStyle it reached the menu; a window takes no font from its parent otherwise, so the flat menu has an element.
            const LabelCase& valueLabel = labelCaseNamed("outgoingProductDependenciesValueLabel");
            std::string legacy;
            std::string flat;
            withLegacyLabel(valueLabel, [&legacy](QLabel& label)
                {
                    QMenu menu(&label);
                    menu.ensurePolished();
                    legacy = describeFont(menu.font());
                });
            withFlatLabel(valueLabel, [this, &flat](QLabel& label)
                {
                    QMenu menu(&label);
                    flatten(menu);
                    flat = describeFont(menu.font());
                });
            EXPECT_EQ(flat, legacy);
        }

        TEST_F(FormFlatSheetTests, Palette_TheElementsGiveNoLabelAColourOfItsOwn)
        {
            // The sheets set a font only: the text colour is the application's (white), as the label's palette without the element
            QPalette plain;
            QPalette styled;
            {
                ClassNamedForm form(MainWindowClass);
                QLabel* unstyled = new QLabel(QStringLiteral("x"), &form);
                flatten(*unstyled);
                plain = unstyled->palette();
            }
            withFlatLabel(LabelCases[0], [&styled](QLabel& label) { styled = label.palette(); });

            for (const QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText, QPalette::Window })
            {
                for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Disabled })
                {
                    EXPECT_EQ(styled.color(group, role), plain.color(group, role))
                        << static_cast<int>(role) << " " << static_cast<int>(group);
                }
            }
        }

        // ------------------------------------------------------------------------------
        // Box: where a label lays out its text and how big it asks to be
        // ------------------------------------------------------------------------------

        TEST_F(FormFlatSheetTests, Box_TheContentsAndTheSizeOfALabelAreThoseOfTheLegacyLabel)
        {
            for (const LabelCase& labelCase : LabelCases)
            {
                QRect legacyContents;
                QRect flatContents;
                QSize legacyHint;
                QSize flatHint;
                QSize legacyMinimum;
                QSize flatMinimum;
                withLegacyLabel(labelCase, [&](QLabel& label)
                    {
                        legacyContents = frameContents(label);
                        legacyHint = label.sizeHint();
                        legacyMinimum = label.minimumSizeHint();
                    });
                withFlatLabel(labelCase, [&](QLabel& label)
                    {
                        flatContents = frameContents(label);
                        flatHint = label.sizeHint();
                        flatMinimum = label.minimumSizeHint();
                    });
                EXPECT_EQ(flatContents, legacyContents) << labelCase.m_name;
                EXPECT_EQ(flatHint, legacyHint) << labelCase.m_name;
                EXPECT_EQ(flatMinimum, legacyMinimum) << labelCase.m_name;
            }
        }

        // ------------------------------------------------------------------------------
        // Pixels: the elements against the sheets, at the device ratios of the GUI
        // ------------------------------------------------------------------------------

        TEST_F(FormFlatSheetTests, Pixels_ALabelPaintsWhatItPaintedUnderItsSheetAtEveryRatio)
        {
            for (const LabelCase& labelCase : LabelCases)
            {
                for (const qreal ratio : { 1.0, 1.5, 2.0 })
                {
                    // The text colour is the application's, not the sheets': the flat label gets the legacy palette on purpose, so that
                    // the picture compares the fonts and the boxes only (Palette_TheElementsGiveNoLabelAColourOfItsOwn is the proof
                    // that the elements add no colour of their own)
                    QPalette palette;
                    QImage legacy;
                    QImage flat;
                    withLegacyLabel(labelCase, [&](QLabel& label)
                        {
                            palette = label.palette();
                            legacy = renderWidget(label, ratio);
                        });
                    withFlatLabel(labelCase, [&](QLabel& label)
                        {
                            label.setPalette(palette);
                            flat = renderWidget(label, ratio);
                        });
                    EXPECT_EQ(firstDifference(legacy, flat), std::string()) << labelCase.m_name << " at ratio " << ratio;
                }
            }
        }

        TEST_F(FormFlatSheetTests, Pixels_ADividerPaintsWhatItPaintedUnderItsSheetAtEveryRatio)
        {
            for (const qreal ratio : { 1.0, 1.5, 2.0 })
            {
                EXPECT_EQ(firstDifference(legacyDivider(ratio), flatDivider(ratio)), std::string()) << "ratio " << ratio;
            }
        }

        TEST_F(FormFlatSheetTests, Pixels_TheDividersBackgroundIsTheFormsColourOverTheWholeWidget)
        {
            // The background alone (PE_Widget), without the sunken line the frame draws over it
            constexpr qreal ratio = 1.5;
            ClassNamedForm form(MainWindowClass);
            auto* page = new QWidget(&form);
            page->setObjectName(QStringLiteral("SettingsPage"));
            QFrame* line = FormFlatSheetTests::makeDivider("line", page);
            flatten(*line);
            ASSERT_TRUE(line->testAttribute(Qt::WA_StyledBackground));

            QImage image((QSizeF(line->size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor::fromRgb(ParentColor));
            {
                QPainter painter(&image);
                QStyleOption background;
                background.initFrom(line);
                line->style()->drawPrimitive(QStyle::PE_Widget, &background, &painter, line);
            }

            EXPECT_EQ(image.pixel(0, 0) & 0xFFFFFF, 0x616161u);
            EXPECT_EQ(image.pixel(image.width() - 1, image.height() - 1) & 0xFFFFFF, 0x616161u);
        }

        TEST_F(FormFlatSheetTests, Pixels_TheRemoveButtonPaintsWhatItPaintedUnderItsSheetIdleAndDownAtEveryRatio)
        {
            // Where the icon sits (the contents box) and how big the button asks to be. The states compared are idle and down (the
            // hovered and the disabled button are not twinned here)
            QRect legacyContents;
            QRect flatContents;
            QSize legacyHint;
            QSize flatHint;
            withLegacyButton([&](QPushButton& button)
                {
                    legacyContents = buttonContents(button);
                    legacyHint = button.sizeHint();
                });
            withFlatButton([&](QPushButton& button)
                {
                    flatContents = buttonContents(button);
                    flatHint = button.sizeHint();
                });
            EXPECT_EQ(flatContents, legacyContents);
            EXPECT_EQ(flatHint, legacyHint);

            for (const bool down : { false, true })
            {
                for (const qreal ratio : { 1.0, 1.5, 2.0 })
                {
                    QImage legacy;
                    QImage flat;
                    withLegacyButton([&](QPushButton& button)
                        {
                            button.setDown(down);
                            legacy = renderWidget(button, ratio);
                        });
                    withFlatButton([&](QPushButton& button)
                        {
                            button.setDown(down);
                            flat = renderWidget(button, ratio);
                        });
                    EXPECT_EQ(firstDifference(legacy, flat), std::string()) << (down ? "down" : "idle") << " at ratio " << ratio;
                }
            }
        }
    } // namespace FormFlatSheetTestsInternal
} // namespace AssetProcessor
