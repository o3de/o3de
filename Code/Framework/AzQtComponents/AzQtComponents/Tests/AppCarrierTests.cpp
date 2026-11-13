/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/FieldTwins.h>

#include <AzQtComponents/Tests/ToolTipTwins.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ColorPickerElements.h>
#include <AzQtComponents/Components/Elements/IndicatorElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/Widgets/BreadCrumbs.h>
#include <AzQtComponents/Components/Widgets/Slider.h>

#include <QCursor>
#include <QFontDatabase>
#include <QFontInfo>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMargins>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QRadioButton>
#include <QStyleOptionButton>
#include <QStyleOptionToolButton>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <optional>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // What the applications' sheets gave their widgets that the carriers of the box and indicator elements now hold, each as the legacy GUI
    // drew it (Qt's own style sheet style over Fusion, with the text of the rules the application had) against the flattened Style with the
    // element, through the style onto an image of an explicit device ratio (QWidget::render would hand the style the ratio of the screen the
    // test runs on), left to right and right to left, at the ratios 1 and 1.5:
    //   - the label of a push button whose rule has a text-align (ProjectManager.qss:1057-1066): BoxElement::m_textAlign
    //   - the face of a flat push button that is down (PushButton.qss:48-52): none, as the sheet drew it
    //   - the indicator of a radio button whose image is no file (ProjectManager.qss:97-99): IndicatorElement::m_hidden
    //   - the icon of the button of a toolbar that shows what does not fit it (ToolBar.qss:139-142): ControlElements' Global element
    //   - the frame of the breadcrumbs whose path can be edited (BreadCrumbs.qss:33-37): ControlElements' Global element
    //   - the height of the entry of a form in ems (ProjectManager.qss:305): the 24px the Project Manager's element carries
    //   - the icon and the icon size of the button of breadcrumbs (BreadCrumbs.qss:20-24): ControlElements' Global element
    //   - the padding of the two labels of a message box (MessageBox.qss:11-22): ControlElements' Global elements
    //   - the tool tip offsets of a slider (Slider.qss:10-20): ControlElements' Global elements
    // ----------------------------------------------------------------------------------

    namespace AppCarrierTestsInternal
    {
        using namespace FieldTwins;

        const qreal Ratios[] = { 1.0, 1.5 };
        const Qt::LayoutDirection Directions[] = { Qt::LeftToRight, Qt::RightToLeft };

        QColor rgb(int value)
        {
            return QColor((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
        }

        //! The widgets of the flat twin: its window carries a property, and a registration made for these tests matches nothing else
        bool flatTwin(const QWidget* widget)
        {
            return widget->window()->property("appCarrierTestsFlat").toBool();
        }

        //! A widget of the flat twin that has the property `flatLook` of that value. The element of a widget of the flat twin is chosen by it,
        //! where the legacy twin's rules are chosen by selectors
        bool flatLook(const QWidget* widget, const char* look)
        {
            return flatTwin(widget) && widget->property("flatLook").toString() == QLatin1String(look);
        }

        //! The option a button paints with in a state (initStyleOption is protected)
        class OptionPushButton
            : public QPushButton
        {
        public:
            using QPushButton::QPushButton;

            QStyleOptionButton option(QStyle::State extra) const
            {
                QStyleOptionButton option;
                initStyleOption(&option);
                option.state |= extra;
                return option;
            }
        };

        class OptionRadioButton
            : public QRadioButton
        {
        public:
            using QRadioButton::QRadioButton;

            QStyleOptionButton option(QStyle::State extra) const
            {
                QStyleOptionButton option;
                initStyleOption(&option);
                option.state |= extra;
                return option;
            }
        };

        //! The states a button is painted in: what the widget is (enabled, checked, down) and what only the option says (the pointer over it,
        //! the focus)
        struct ButtonState
        {
            const char* m_name;
            bool m_enabled;
            bool m_checked;
            bool m_down;
            QStyle::State m_extra;
        };

        const ButtonState ButtonStates[] = {
            { "idle", true, false, false, QStyle::State_None },
            { "hover", true, false, false, QStyle::State_MouseOver },
            { "pressed", true, false, true, QStyle::State_None },
            { "checked", true, true, false, QStyle::State_None },
            { "checked and pressed", true, true, true, QStyle::State_None },
            { "disabled", false, false, false, QStyle::State_None },
            { "focus", true, false, false, QStyle::State_HasFocus },
        };

        void setState(QAbstractButton* button, const ButtonState& state)
        {
            button->setEnabled(state.m_enabled);
            button->setChecked(state.m_checked);
            button->setDown(state.m_down);
        }

        //! A tree of widgets built twice and shown: the legacy twin under the sheet the test gives it, the flat twin under the style. The build
        //! function fills a root, and is told which of the two it is filling.
        struct AppTwins
        {
            template<typename Build>
            AppTwins(const Fixture& fixture, Build build, const QString& legacySheet, const QSize& size, Qt::LayoutDirection direction)
            {
                m_legacy.setLayoutDirection(direction);
                build(m_legacy, false);
                m_legacy.setStyleSheet(legacySheet);
                Fixture::show(m_legacy, size);

                m_flat.setLayoutDirection(direction);
                build(m_flat, true);
                m_flat.setProperty("appCarrierTestsFlat", true);
                fixture.flat(m_flat);
                Fixture::show(m_flat, size);
            }

            template<typename W = QWidget>
            W* legacyChild(const char* name)
            {
                return m_legacy.findChild<W*>(QString::fromLatin1(name));
            }

            template<typename W = QWidget>
            W* flatChild(const char* name)
            {
                return m_flat.findChild<W*>(QString::fromLatin1(name));
            }

            //! The buttons are the test's own (they tell the option they paint with), a class with no Q_OBJECT that findChild does not take
            OptionPushButton* legacyButton(const char* name)
            {
                return static_cast<OptionPushButton*>(legacyChild<QPushButton>(name));
            }

            OptionPushButton* flatButton(const char* name)
            {
                return static_cast<OptionPushButton*>(flatChild<QPushButton>(name));
            }

            OptionRadioButton* legacyRadio(const char* name)
            {
                return static_cast<OptionRadioButton*>(legacyChild<QRadioButton>(name));
            }

            OptionRadioButton* flatRadio(const char* name)
            {
                return static_cast<OptionRadioButton*>(flatChild<QRadioButton>(name));
            }

            //! The flat widget has the font its twin has: what a font is made of is not what a carrier proves
            void shareFont(const char* name)
            {
                flatChild(name)->setFont(legacyChild(name)->font());
            }

            QWidget m_legacy;
            QWidget m_flat;
        };

        class AppCarrierTests
            : public Fixture
        {
        };

        std::string directionText(Qt::LayoutDirection direction)
        {
            return direction == Qt::RightToLeft ? "right to left" : "left to right";
        }

        //! Something a widget draws in a rect of an explicit size (its own size is its layout's, which the twins need not share), through its
        //! own style onto an image of an explicit device ratio, on grey (QWidget::render would hand the style the ratio of the screen the test
        //! runs on). The painter is what a widget's paint event has (QWidget::initPainter, which QPainter::begin runs for a widget device and
        //! not for an image, qpainter.cpp:1800-1808): the pen and background of the widget, its font, and its layout direction, which the label
        //! of a push button reads (a painter on an image has the direction Auto, and the text beside an icon is then aligned to the right,
        //! qcommonstyle.cpp:1379-1383)
        QImage paintedIn(const QWidget* widget, const QSize& size, qreal ratio, const std::function<void(QPainter&)>& paint,
            const QColor& background = QColor(0x80, 0x80, 0x80))
        {
            QImage image((QSizeF(size) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(background);
            QPainter painter(&image);
            painter.setPen(QPen(widget->palette().brush(widget->foregroundRole()), 1));
            painter.setBackground(widget->palette().brush(widget->backgroundRole()));
            painter.setFont(widget->font());
            painter.setLayoutDirection(widget->layoutDirection());
            paint(painter);
            painter.end();
            return image;
        }

        //! The icon the buttons have: a disc with a square in it, something no base style draws
        QIcon discIcon()
        {
            QPixmap pixmap(32, 32);
            pixmap.fill(Qt::transparent);
            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(Qt::NoPen);
            painter.setBrush(rgb(0xFFD040));
            painter.drawEllipse(QRect(2, 2, 28, 28));
            painter.setBrush(rgb(0x202020));
            painter.drawRect(QRect(11, 11, 10, 10));
            painter.end();
            return QIcon(pixmap);
        }

        // ----------------------------------------------------------------------------------
        // The label of a push button whose rule has a text-align (ProjectManager.qss:1057-1066, #gemRepoAddButton, the Update All button of the
        // Remote Sources page): the sheet style laid the label out itself, the icon and the text 4px apart where the base style puts them 2px
        // ----------------------------------------------------------------------------------

        //! The rule of #gemRepoAddButton (ProjectManager.qss:1057-1066) for each alignment the carrier supports, on the plain face the elements
        //! below draw: the label is what the test is about, and the gradient of PushButton.cpp is the box element's own proof
        const char* const AlignedButtonRules =
            "QPushButton { background-color: #666666; border: none; padding: 0px; margin: 0px; }"
            "QPushButton[align] { min-width: 120px; max-width: 120px; min-height: 24px; max-height: 24px; border-radius: 3px;"
            " font-size: 12px; font-weight: 600; }"
            "QPushButton[align=\"center\"] { text-align: center; }"
            "QPushButton[align=\"left\"] { text-align: left; }"
            "QPushButton[align=\"right\"] { text-align: right; }";

        BoxElement alignedButton(const char* name, Qt::Alignment alignment)
        {
            BoxElement element;
            element.m_name = name;
            element.m_hasBox = true;                                                // margin: 0 and padding: 0 are written
            element.m_nativeBorder = false;                                         // border: none, border-radius
            element.m_radius = 3;                                                   // ProjectManager.qss:1061
            element.m_minSize = QSize(120, 24);                                     // ProjectManager.qss:1057,1059
            element.m_maxSize = QSize(120, 24);                                     // ProjectManager.qss:1058,1060
            element.m_idle.m_fill = QBrush(rgb(0x666666));
            element.m_textAlign = alignment;                                        // text-align: ProjectManager.qss:1062
            return element;
        }

        const BoxElement AlignedCenter = alignedButton("Test.AppCarrier.AlignedCenter", Qt::AlignCenter);
        const BoxElement AlignedLeft = alignedButton("Test.AppCarrier.AlignedLeft", Qt::AlignLeft);
        const BoxElement AlignedRight = alignedButton("Test.AppCarrier.AlignedRight", Qt::AlignRight);
        const BoxElement AlignedNone = alignedButton("Test.AppCarrier.AlignedNone", Qt::Alignment());

        //! A button of each content for one rule, in a column: an icon and a text, the icon alone, the text alone
        void buildAlignedButtons(QWidget& root, bool flat, const char* rule, const char* look)
        {
            auto* layout = new QVBoxLayout(&root);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(4);
            for (int content = 0; content < 3; ++content)
            {
                auto* button = new OptionPushButton(&root);
                button->setObjectName(QStringLiteral("button%1").arg(content));
                button->setProperty("align", QString::fromLatin1(rule));
                if (flat)
                {
                    button->setProperty("flatLook", QString::fromLatin1(look));
                }
                button->setCheckable(true);
                if (content != 2)
                {
                    button->setIcon(discIcon());
                    button->setIconSize(QSize(16, 16));
                }
                if (content != 1)
                {
                    button->setText(QStringLiteral("Update All"));
                }
                layout->addWidget(button);
            }
        }

        void registerAlignedButtons()
        {
            static bool registered = false;
            if (registered)
            {
                return;
            }
            registered = true;

            const QMetaObject* button = &QPushButton::staticMetaObject;
            BoxElement::registerElement(&AlignedCenter, button, [](const QWidget* w) { return flatLook(w, "center"); });
            BoxElement::registerElement(&AlignedLeft, button, [](const QWidget* w) { return flatLook(w, "left"); });
            BoxElement::registerElement(&AlignedRight, button, [](const QWidget* w) { return flatLook(w, "right"); });
            BoxElement::registerElement(&AlignedNone, button, [](const QWidget* w) { return flatLook(w, "none"); });
        }

        //! One push button as the style paints it (CE_PushButton: the bevel, then the label), on grey
        QImage paintedButton(const OptionPushButton* button, QStyle::State extra, qreal ratio, const QSize& size)
        {
            return paintedIn(button, size, ratio, [&](QPainter& painter)
            {
                QStyleOptionButton option = button->option(extra);
                option.rect = QRect(QPoint(0, 0), size);
                button->style()->drawControl(QStyle::CE_PushButton, &option, &painter, button);
            });
        }

        TEST_F(AppCarrierTests, PushButton_TheLabelOfARuleWithATextAlignIsTheSheetsInEveryAlignmentStateRatioAndDirection)
        {
            registerAlignedButtons();
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            for (const char* rule : { "center", "left", "right" })
            {
                for (const Qt::LayoutDirection direction : Directions)
                {
                    AppTwins twins(*this, [&](QWidget& root, bool flat) { buildAlignedButtons(root, flat, rule, rule); },
                        appSheet() + QString::fromUtf8(AlignedButtonRules), QSize(120, 100), direction);

                    for (int content = 0; content < 3; ++content)
                    {
                        const QByteArray name = "button" + QByteArray::number(content);
                        twins.shareFont(name.constData());
                        auto* legacyButton = twins.legacyButton(name.constData());
                        auto* flatButton = twins.flatButton(name.constData());
                        ASSERT_TRUE(legacyButton != nullptr && flatButton != nullptr);
                        ASSERT_TRUE(BoxElement::forWidget(flatButton) != nullptr);
                        EXPECT_EQ(sizeText(flatButton->size()), sizeText(legacyButton->size()));
                        EXPECT_EQ(sizeText(flatButton->sizeHint()), sizeText(legacyButton->sizeHint()));

                        for (const ButtonState& state : ButtonStates)
                        {
                            setState(legacyButton, state);
                            setState(flatButton, state);
                            for (const qreal ratio : Ratios)
                            {
                                const QImage legacyImage = paintedButton(legacyButton, state.m_extra, ratio, legacyButton->size());
                                const QImage flatImage = paintedButton(flatButton, state.m_extra, ratio, legacyButton->size());
                                EXPECT_TRUE(isDrawnOn(legacyImage));
                                EXPECT_EQ(differing(flatImage, legacyImage), 0)
                                    << rule << ", " << directionText(direction) << ", content " << content << ", " << state.m_name
                                    << ", ratio " << ratio << ": " << firstDifference(flatImage, legacyImage);
                            }
                        }
                    }
                }
            }
        }

        TEST_F(AppCarrierTests, PushButton_ARuleWithNoPositionLeavesTheLabelToTheBaseStyleAndTheCarrierIsWhatMovesTheText)
        {
            registerAlignedButtons();
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // A rule with no text-align: the label is the base style's in both twins
            {
                AppTwins plain(*this, [](QWidget& root, bool flat) { buildAlignedButtons(root, flat, "none", "none"); },
                    appSheet() + QString::fromUtf8(AlignedButtonRules), QSize(120, 100), Qt::LeftToRight);
                for (int content = 0; content < 3; ++content)
                {
                    const QByteArray name = "button" + QByteArray::number(content);
                    plain.shareFont(name.constData());
                    for (const qreal ratio : Ratios)
                    {
                        const QImage legacyImage = paintedButton(plain.legacyButton(name.constData()), QStyle::State_None, ratio, QSize(120, 24));
                        const QImage flatImage = paintedButton(plain.flatButton(name.constData()), QStyle::State_None, ratio, QSize(120, 24));
                        EXPECT_TRUE(isDrawnOn(legacyImage));
                        EXPECT_EQ(differing(flatImage, legacyImage), 0)
                            << "content " << content << ", ratio " << ratio << ": " << firstDifference(flatImage, legacyImage);
                    }
                }
            }

            // The witness: the rule has a text-align (centered) and the element does not carry it. The text of a button that has an icon too is
            // 2px nearer the icon than the sheet's 4px (qcommonstyle.cpp:1377), and the other two buttons, whose label is one thing centered,
            // are the same
            AppTwins twins(*this, [](QWidget& root, bool flat) { buildAlignedButtons(root, flat, "center", "none"); },
                appSheet() + QString::fromUtf8(AlignedButtonRules), QSize(120, 100), Qt::LeftToRight);
            for (int content = 0; content < 3; ++content)
            {
                const QByteArray name = "button" + QByteArray::number(content);
                twins.shareFont(name.constData());
                for (const qreal ratio : Ratios)
                {
                    const QImage legacyImage = paintedButton(twins.legacyButton(name.constData()), QStyle::State_None, ratio, QSize(120, 24));
                    const QImage flatImage = paintedButton(twins.flatButton(name.constData()), QStyle::State_None, ratio, QSize(120, 24));
                    if (content == 0)
                    {
                        EXPECT_GT(differing(flatImage, legacyImage), 0) << "icon and text, ratio " << ratio;
                    }
                    else
                    {
                        EXPECT_EQ(differing(flatImage, legacyImage), 0)
                            << "content " << content << ", ratio " << ratio << ": " << firstDifference(flatImage, legacyImage);
                    }
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // A flat push button (PushButton.qss:48-52, `QPushButton:flat { border: none; background-color: transparent }`) has no face in any state:
        // the sheet style drew what the rule had, which is nothing, where Fusion draws the face of a button that is down (the base style's
        // bevel asks for the panel of a flat button only while it is sunken or on, qcommonstyle.cpp:1316-1322, qfusionstyle.cpp:713-724)
        // ----------------------------------------------------------------------------------

        void buildFlatButtons(QWidget& root, bool)
        {
            auto* layout = new QVBoxLayout(&root);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(4);
            for (int content = 0; content < 2; ++content)
            {
                auto* button = new OptionPushButton(&root);
                button->setObjectName(QStringLiteral("flat%1").arg(content));
                button->setFlat(true);
                button->setCheckable(true);
                button->setIcon(discIcon());
                button->setIconSize(QSize(16, 16));
                button->setFixedSize(content == 0 ? QSize(24, 24) : QSize(140, 24));
                if (content == 1)
                {
                    button->setText(QStringLiteral("Advanced"));
                }
                layout->addWidget(button);
            }
        }

        TEST_F(AppCarrierTests, FlatPushButton_HasNoFaceInAnyStateAsTheSheetDrewIt)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            for (const Qt::LayoutDirection direction : Directions)
            {
                AppTwins twins(*this, buildFlatButtons, appSheet(), QSize(160, 80), direction);
                for (const char* name : { "flat0", "flat1" })
                {
                    twins.shareFont(name);
                    auto* legacyButton = twins.legacyButton(name);
                    auto* flatButton = twins.flatButton(name);
                    ASSERT_TRUE(legacyButton != nullptr && flatButton != nullptr);
                    // the size the button was given: the flattened style gives a push button a height of its own at polish (a minimum of 32), which
                    // is not what this is about
                    const QSize size = QString::fromLatin1(name) == QLatin1String("flat0") ? QSize(24, 24) : QSize(140, 24);
                    legacyButton->setFixedSize(size);
                    flatButton->setFixedSize(size);
                    QApplication::processEvents();
                    ASSERT_EQ(sizeText(flatButton->size()), sizeText(legacyButton->size()));
                    for (const ButtonState& state : ButtonStates)
                    {
                        setState(legacyButton, state);
                        setState(flatButton, state);
                        for (const qreal ratio : Ratios)
                        {
                            const QImage legacyImage = paintedButton(legacyButton, state.m_extra, ratio, size);
                            const QImage flatImage = paintedButton(flatButton, state.m_extra, ratio, size);
                            EXPECT_TRUE(isDrawnOn(legacyImage));
                            EXPECT_EQ(differing(flatImage, legacyImage), 0)
                                << name << ", " << directionText(direction) << ", " << state.m_name << ", ratio " << ratio << ": "
                                << firstDifference(flatImage, legacyImage);
                        }
                    }
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // The steps of the Project Manager's create-a-gem wizard (ProjectManager.qss:79-99): radio buttons whose indicator has no image
        // ----------------------------------------------------------------------------------

        //! ProjectManager.qss:79-99, as it is
        const char* const WizardStepRules =
            "#createAGemLHS QRadioButton{ font-size: 16px; height: 40px; min-width: 180px; spacing: 10px; color: #6b6b6b; }"
            "#createAGemLHS QRadioButton:enabled{ color: #aaaaaa; }"
            "#createAGemLHS QRadioButton:checked{ background-color: #333333; spacing: 30px; color: #FFFFFF; }"
            "#createAGemLHS QRadioButton::indicator{ image: url(:/nothing.png); }";

        // The elements of the Project Manager for them (ProjectManager/Source/FlatElements.cpp WizardStepRadio, WizardStepIndicator and
        // WizardStepRadioText), and the same indicator that is not hidden for the witness
        const BoxElement WizardStep{
            .m_name = "Test.AppCarrier.WizardStep",
            .m_spacing = 10,                                  // spacing: ProjectManager.qss:83
            .m_size = QSize(-1, 40),                          // height: ProjectManager.qss:81
            .m_minSize = QSize(180, -1),                      // min-width: ProjectManager.qss:82
            .m_idle = { .m_fill = QBrush(QColor(0, 0, 0, 0)) },
            .m_stateFaces = {
                { BoxElement::Checked, { .m_fill = QBrush(QColor(0x33, 0x33, 0x33)), .m_text = QColor(0xFF, 0xFF, 0xFF), .m_spacing = 30 } },
            },
        };

        const IndicatorElement WizardStepHidden{
            .m_name = "Test.AppCarrier.WizardStepHidden",
            .m_imageSize = QSize(16, 16),                     // RadioButton.qss:24-25
            .m_margin = 2,                                    // RadioButton.qss:26
            .m_focusImageSize = QSize(18, 18),                // RadioButton.qss:31-32
            .m_focusMargin = 1,                               // RadioButton.qss:33
            .m_hidden = true,                                 // image: url(:/nothing.png), ProjectManager.qss:98
        };

        const IndicatorElement WizardStepShown{
            .m_name = "Test.AppCarrier.WizardStepShown",
            .m_imageSize = QSize(16, 16),
            .m_margin = 2,
            .m_focusImageSize = QSize(18, 18),
            .m_focusMargin = 1,
        };

        const TextElement WizardStepText{
            .m_name = "Test.AppCarrier.WizardStepText",
            .m_pixelSize = 16,                                // font-size: ProjectManager.qss:80
            .m_color = QColor(0xAA, 0xAA, 0xAA),              // :enabled color: ProjectManager.qss:88
            .m_disabledColor = QColor(0x6B, 0x6B, 0x6B),      // color: ProjectManager.qss:84
        };

        void registerWizardSteps()
        {
            static bool registered = false;
            if (registered)
            {
                return;
            }
            registered = true;

            const QMetaObject* radio = &QRadioButton::staticMetaObject;
            BoxElement::registerElement(&WizardStep, radio, [](const QWidget* w) { return flatTwin(w) && Selectors::within(w, "createAGemLHS"); });
            TextElement::registerElement(&WizardStepText, radio, [](const QWidget* w) { return flatTwin(w) && Selectors::within(w, "createAGemLHS"); });
            // the indicator the witness shows (only where the radio button says so), and the hidden one of every step
            IndicatorElement::registerElement(&WizardStepShown, radio,
                [](const QWidget* w) { return flatLook(w, "shown") && Selectors::within(w, "createAGemLHS"); });
            IndicatorElement::registerElement(&WizardStepHidden, radio,
                [](const QWidget* w) { return flatTwin(w) && Selectors::within(w, "createAGemLHS"); });
        }

        //! The steps in a list that is #createAGemLHS: one checked, one not, one disabled; and a radio button outside the list, which the rules
        //! do not reach
        void buildWizardSteps(QWidget& root, bool flat, const char* look)
        {
            auto* outer = new QVBoxLayout(&root);
            outer->setContentsMargins(0, 0, 0, 0);
            auto* steps = new QWidget(&root);
            steps->setObjectName(QStringLiteral("createAGemLHS"));
            auto* list = new QVBoxLayout(steps);
            list->setContentsMargins(0, 0, 0, 0);
            list->setSpacing(0);
            const char* const names[] = { "stepOne", "stepTwo", "stepThree" };
            const char* const texts[] = { "Gem Setup", "Gem Details", "Creator" };
            for (int i = 0; i < 3; ++i)
            {
                auto* step = new OptionRadioButton(QString::fromLatin1(texts[i]), steps);
                step->setObjectName(QString::fromLatin1(names[i]));
                step->setChecked(i == 0);
                step->setEnabled(i != 2);
                if (flat)
                {
                    step->setProperty("flatLook", QString::fromLatin1(look));
                }
                list->addWidget(step);
            }
            outer->addWidget(steps);
            auto* outside = new OptionRadioButton(QStringLiteral("Outside"), &root);
            outside->setObjectName(QStringLiteral("outside"));
            outer->addWidget(outside);
            outer->addStretch();
        }

        //! One radio button as the style paints it (CE_RadioButton: the rule's box, the indicator and the label), on grey
        QImage paintedRadio(const OptionRadioButton* radio, QStyle::State extra, qreal ratio)
        {
            return paintedIn(radio, radio->size(), ratio, [&](QPainter& painter)
            {
                const QStyleOptionButton option = radio->option(extra);
                radio->style()->drawControl(QStyle::CE_RadioButton, &option, &painter, radio);
            });
        }

        TEST_F(AppCarrierTests, RadioButton_AStepWhoseIndicatorHasNoImageIsTheSheetsInItsBoxItsRectsAndItsPixels)
        {
            registerWizardSteps();
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            const QString sheet = appSheet() + QString::fromUtf8(WizardStepRules);
            for (const Qt::LayoutDirection direction : Directions)
            {
                AppTwins twins(*this, [](QWidget& root, bool flat) { buildWizardSteps(root, flat, "hidden"); }, sheet, QSize(300, 200), direction);
                for (const char* name : { "stepOne", "stepTwo", "stepThree", "outside" })
                {
                    twins.shareFont(name);
                    auto* legacyRadio = twins.legacyRadio(name);
                    auto* flatRadio = twins.flatRadio(name);
                    ASSERT_TRUE(legacyRadio != nullptr && flatRadio != nullptr);
                    EXPECT_EQ(sizeText(flatRadio->sizeHint()), sizeText(legacyRadio->sizeHint())) << name;
                    EXPECT_EQ(sizeText(flatRadio->size()), sizeText(legacyRadio->size())) << name;

                    // the rects the box and the indicator lay out: the label starts where the sheet's spacing puts it. The radio button outside the
                    // list has the Global element, whose rects are its own (the indicator rect of its image; the pixels are the sheet's)
                    const QStyleOptionButton legacyOption = legacyRadio->option(QStyle::State_None);
                    const QStyleOptionButton flatOption = flatRadio->option(QStyle::State_None);
                    for (const QStyle::SubElement element : { QStyle::SE_RadioButtonIndicator, QStyle::SE_RadioButtonContents,
                                                                QStyle::SE_RadioButtonClickRect })
                    {
                        if (QString::fromLatin1(name) != QLatin1String("outside"))
                        {
                            EXPECT_EQ(rectText(flatRadio->style()->subElementRect(element, &flatOption, flatRadio)),
                                rectText(legacyRadio->style()->subElementRect(element, &legacyOption, legacyRadio)))
                                << name << ", sub-element " << int(element);
                        }
                    }

                    // the states of an enabled radio button (a disabled one is painted as it is: the widget's state, not the option's)
                    const ButtonState* states[] = { &ButtonStates[0], &ButtonStates[1], &ButtonStates[2], &ButtonStates[6] };
                    const int count = legacyRadio->isEnabled() ? 4 : 1;
                    for (int i = 0; i < count; ++i)
                    {
                        const ButtonState& state = legacyRadio->isEnabled() ? *states[i] : ButtonStates[5];
                        legacyRadio->setDown(state.m_down);
                        flatRadio->setDown(state.m_down);
                        for (const qreal ratio : Ratios)
                        {
                            const QImage legacyImage = paintedRadio(legacyRadio, state.m_extra, ratio);
                            const QImage flatImage = paintedRadio(flatRadio, state.m_extra, ratio);
                            EXPECT_TRUE(isDrawnOn(legacyImage)) << name;
                            EXPECT_EQ(differing(flatImage, legacyImage), 0)
                                << name << ", " << directionText(direction) << ", " << state.m_name << ", ratio " << ratio << ": "
                                << firstDifference(flatImage, legacyImage);
                        }
                    }
                    legacyRadio->setDown(false);
                    flatRadio->setDown(false);
                }
            }
        }

        TEST_F(AppCarrierTests, RadioButton_AnIndicatorThatIsNotHiddenIsTheOnlyThingThatDiffersFromTheSheetsStep)
        {
            registerWizardSteps();
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The witness: the same step with the indicator drawn. Its box and its hint are the sheet's, and its pixels are not: the indicator
            // is where the sheet had none
            const QString sheet = appSheet() + QString::fromUtf8(WizardStepRules);
            AppTwins twins(*this, [](QWidget& root, bool flat) { buildWizardSteps(root, flat, "shown"); }, sheet, QSize(300, 200), Qt::LeftToRight);
            for (const char* name : { "stepOne", "stepTwo" })
            {
                twins.shareFont(name);
                auto* legacyRadio = twins.legacyRadio(name);
                auto* flatRadio = twins.flatRadio(name);
                EXPECT_EQ(sizeText(flatRadio->sizeHint()), sizeText(legacyRadio->sizeHint())) << name;
                for (const qreal ratio : Ratios)
                {
                    const QImage legacyImage = paintedRadio(legacyRadio, QStyle::State_None, ratio);
                    const QImage flatImage = paintedRadio(flatRadio, QStyle::State_None, ratio);
                    EXPECT_GT(differing(flatImage, legacyImage), 0) << name << ", ratio " << ratio;
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // The button of a toolbar that shows what does not fit it (ToolBar.qss:139-142): the sheet gave it the double caret
        // ----------------------------------------------------------------------------------

        void buildToolBar(QWidget& root, bool)
        {
            auto* layout = new QVBoxLayout(&root);
            layout->setContentsMargins(0, 0, 0, 0);
            auto* toolBar = new QToolBar(&root);
            toolBar->setObjectName(QStringLiteral("toolBar"));
            toolBar->setMovable(false);
            toolBar->setIconSize(QSize(16, 16));
            for (int i = 0; i < 6; ++i)
            {
                toolBar->addAction(discIcon(), QStringLiteral("Action %1").arg(i));
            }
            layout->addWidget(toolBar);
            layout->addStretch();
        }

        QImage iconImage(const QIcon& icon, qreal ratio)
        {
            return icon.pixmap(QSize(16, 16), ratio).toImage();
        }

        //! A tool button as the style draws it (CC_ToolButton, an auto-raised button, as a toolbar's is) in a rect of an explicit size, on nothing: what
        //! is drawn is the alpha of each pixel
        QImage paintedToolButton(const QToolButton* button, const QSize& size, qreal ratio)
        {
            return paintedIn(button, size, ratio, [&](QPainter& painter)
            {
                QStyleOptionToolButton option;
                option.initFrom(button);
                option.rect = QRect(QPoint(0, 0), size);
                option.iconSize = button->iconSize();
                option.icon = button->icon();
                option.text = button->text();
                option.toolButtonStyle = button->toolButtonStyle();
                option.arrowType = button->arrowType();
                option.subControls = QStyle::SC_ToolButton;
                option.activeSubControls = QStyle::SC_None;
                option.features = QStyleOptionToolButton::None;
                option.state |= QStyle::State_AutoRaise | QStyle::State_Raised;
                button->style()->drawComplexControl(QStyle::CC_ToolButton, &option, &painter, button);
            }, Qt::transparent);
        }

        //! The bounds of the pixels with any ink in them
        QRect inkBounds(const QImage& image)
        {
            QRect bounds;
            for (int y = 0; y < image.height(); ++y)
            {
                for (int x = 0; x < image.width(); ++x)
                {
                    if (qAlpha(image.pixel(x, y)) != 0)
                    {
                        bounds = bounds.isNull() ? QRect(x, y, 1, 1) : bounds.united(QRect(x, y, 1, 1));
                    }
                }
            }
            return bounds;
        }

        //! The pixels whose alpha is not the same in two images: where a button draws is the style's rects, what it draws it in is the base style's
        int differingAlpha(const QImage& a, const QImage& b)
        {
            if (a.size() != b.size())
            {
                return a.width() * a.height();
            }
            int count = 0;
            for (int y = 0; y < a.height(); ++y)
            {
                for (int x = 0; x < a.width(); ++x)
                {
                    count += (qAlpha(a.pixel(x, y)) != qAlpha(b.pixel(x, y))) ? 1 : 0;
                }
            }
            return count;
        }

        TEST_F(AppCarrierTests, ToolBarExtension_HasTheIconOfTheSheetAndOnlyTheExtensionOfAToolBarHasIt)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            AppTwins twins(*this, buildToolBar, appSheet(), QSize(90, 60), Qt::LeftToRight);
            auto* legacyExtension = twins.legacyChild<QToolButton>("qt_toolbar_ext_button");
            auto* flatExtension = twins.flatChild<QToolButton>("qt_toolbar_ext_button");
            ASSERT_TRUE(legacyExtension != nullptr && flatExtension != nullptr);
            EXPECT_TRUE(legacyExtension->isVisibleTo(&twins.m_legacy));
            EXPECT_TRUE(flatExtension->isVisibleTo(&twins.m_flat));

            // the icon of the sheet's rule: the file ToolBar.qss:141 names, whatever the base style's caret is
            const QIcon sheetIcon(QStringLiteral(":/stylesheet/img/hidden-icons.svg"));
            for (const qreal ratio : Ratios)
            {
                const QImage expected = iconImage(sheetIcon, ratio);
                EXPECT_TRUE(isDrawnOn(expected));
                EXPECT_EQ(differing(iconImage(legacyExtension->icon(), ratio), expected), 0) << "legacy, ratio " << ratio;
                EXPECT_EQ(differing(iconImage(flatExtension->icon(), ratio), expected), 0) << "flat, ratio " << ratio;
            }

            // the rule is the extension's: a tool button of that name that is none, and the buttons of the toolbar's actions, keep their icons
            QToolButton other(&twins.m_flat);
            other.setObjectName(QStringLiteral("qt_toolbar_ext_button"));
            other.setStyle(flatExtension->style());
            other.ensurePolished();
            EXPECT_TRUE(other.icon().isNull());

            auto* toolBar = twins.flatChild<QToolBar>("toolBar");
            ASSERT_TRUE(toolBar != nullptr);
            auto* actionButton = qobject_cast<QToolButton*>(toolBar->widgetForAction(toolBar->actions().first()));
            ASSERT_TRUE(actionButton != nullptr);
            EXPECT_EQ(differing(iconImage(actionButton->icon(), 1.0), iconImage(discIcon(), 1.0)), 0);
        }

        TEST_F(AppCarrierTests, ToolBarExtension_IsDrawnWhereTheSheetsIconWasWhenItIsDisabled)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // The extension of a toolbar whose hidden actions are all disabled is disabled itself (n2: the Material Canvas viewport toolbar, 12 x 22,
            // not enabled). A disabled tool button draws its icon recolored by the application's style (Style::generatedIconPixmap, PushButton.cpp:
            // 335-338: #999999 in n2), which is Fusion in this fixture: the colors are not what is compared, the pixels the icon covers are
            AppTwins twins(*this, buildToolBar, appSheet(), QSize(90, 60), Qt::LeftToRight);
            auto* legacyExtension = twins.legacyChild<QToolButton>("qt_toolbar_ext_button");
            auto* flatExtension = twins.flatChild<QToolButton>("qt_toolbar_ext_button");
            ASSERT_TRUE(legacyExtension != nullptr && flatExtension != nullptr);
            legacyExtension->setEnabled(false);
            flatExtension->setEnabled(false);

            const QSize size(12, 22);
            for (const qreal ratio : Ratios)
            {
                const QImage legacyImage = paintedToolButton(legacyExtension, size, ratio);
                const QImage flatImage = paintedToolButton(flatExtension, size, ratio);
                const QRect legacyInk = inkBounds(legacyImage);
                const QRect flatInk = inkBounds(flatImage);
                EXPECT_FALSE(legacyInk.isNull()) << "ratio " << ratio;
                EXPECT_EQ(rectText(flatInk), rectText(legacyInk)) << "ratio " << ratio;
                EXPECT_EQ(differingAlpha(flatImage, legacyImage), 0) << "ratio " << ratio;

                if (ratio == 1.5)
                {
                    // n2, Material Canvas, the Viewport Settings dock: the ink of the 12 x 22 button (18 x 33 device px) is 15 x 16 device px at
                    // (2, 9) (the caret of the base style, f5, was 13 x 9). The icon is the window's ratio's pixmap (1 here, 1.5 in the capture),
                    // so what is asserted is where it is drawn, not what colour: that is the application style's
                    EXPECT_EQ(rectText(flatInk), rectText(QRect(2, 9, 15, 16)));
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // The breadcrumbs whose path can be edited (BreadCrumbs.qss:33-37, `AzQtComponents--BreadCrumbs[editable="true"] { border: 1px solid
        // #2f2f2f; border-radius: 2px }`): a frame of 1px with that border, and the layout inside it
        // ----------------------------------------------------------------------------------

        void buildBreadCrumbs(QWidget& root, bool)
        {
            auto* layout = new QVBoxLayout(&root);
            layout->setContentsMargins(0, 0, 0, 0);
            for (const bool editable : { true, false })
            {
                auto* crumbs = new BreadCrumbs(&root);
                crumbs->setObjectName(editable ? QStringLiteral("editable") : QStringLiteral("plain"));
                crumbs->setEditable(editable);
                layout->addWidget(crumbs);
            }
        }

        //! A frame as its paint event draws it: the background of the rule where the style paints one (QWidgetPrivate::paintBackground), then the frame
        QImage paintedFrame(const QFrame* frame, qreal ratio)
        {
            return paintedIn(frame, frame->size(), ratio, [&](QPainter& painter)
            {
                if (frame->testAttribute(Qt::WA_StyledBackground))
                {
                    QStyleOption background;
                    background.initFrom(frame);
                    frame->style()->drawPrimitive(QStyle::PE_Widget, &background, &painter, frame);
                }
                const QStyleOptionFrame option = frameOption(frame);
                frame->style()->drawControl(QStyle::CE_ShapedFrame, &option, &painter, frame);
            });
        }

        TEST_F(AppCarrierTests, BreadCrumbs_TheFrameOfAnEditableOneIsTheBorderOfTheSheetAndNoOtherHasOne)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            AppTwins twins(*this, buildBreadCrumbs, appSheet(), QSize(200, 80), Qt::LeftToRight);
            for (const char* name : { "editable", "plain" })
            {
                SCOPED_TRACE(name);
                auto* legacyCrumbs = twins.legacyChild<BreadCrumbs>(name);
                auto* flatCrumbs = twins.flatChild<BreadCrumbs>(name);
                ASSERT_TRUE(legacyCrumbs != nullptr && flatCrumbs != nullptr);
                const bool editable = QString::fromLatin1(name) == QLatin1String("editable");

                // n2 (the Gallery's page 01_breadcrumbs, the Editor's Asset Browser): the editable ones have contents margins of 1px, the others none
                const int border = editable ? 1 : 0;
                EXPECT_EQ(legacyCrumbs->contentsMargins(), QMargins(border, border, border, border));
                EXPECT_EQ(flatCrumbs->contentsMargins(), legacyCrumbs->contentsMargins());
                EXPECT_EQ(flatCrumbs->frameWidth(), legacyCrumbs->frameWidth());

                // the frame, whatever the layout inside it makes of the size: the same box in both twins
                legacyCrumbs->setFixedSize(QSize(120, 21));
                flatCrumbs->setFixedSize(QSize(120, 21));
                QApplication::processEvents();
                for (const qreal ratio : Ratios)
                {
                    const QImage legacyImage = paintedFrame(legacyCrumbs, ratio);
                    const QImage flatImage = paintedFrame(flatCrumbs, ratio);
                    EXPECT_EQ(isDrawnOn(legacyImage), editable) << "ratio " << ratio;
                    EXPECT_EQ(differing(flatImage, legacyImage), 0) << "ratio " << ratio << ": " << firstDifference(flatImage, legacyImage);
                    if (editable && ratio == 1.0)
                    {
                        // the colour of the border in the middle of the left edge (n2: #2f2f2f)
                        EXPECT_EQ(flatImage.pixelColor(0, 10).name().toStdString(), "#2f2f2f");
                    }
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // BreadCrumbs.qss:20-24, MessageBox.qss:11-22 and Slider.qss:10-20: rules of the application's sheet that gave a widget a property
        // at polish (an icon, an icon size, a tool tip offset) or a box (the padding of a message box's labels). The legacy twin has the
        // application's real sheet (appSheet: the rules above among the files it imports); the flat twin has ControlElements' Global
        // elements, at the flat rung. What a rule decides is compared: the properties, the pixmaps of the icon, the contents of a label
        // and the hints that follow, the pixels at the ratios 1, 1.5 and 2, and where the tip of a slider is shown.
        // ----------------------------------------------------------------------------------

        const qreal AllRatios[] = { 1.0, 1.5, 2.0 };

        //! The rung the flat twin is made at, for the length of a test: another test may have left the process at another one
        class FlatRungScope
        {
        public:
            FlatRungScope()
                : m_before(GuiPerfFloor::rung())
            {
                GuiPerfFloor::setRungForTesting(GuiPerfFloor::Rung::Flat);
            }

            ~FlatRungScope()
            {
                GuiPerfFloor::setRungForTesting(m_before);
            }

            FlatRungScope(const FlatRungScope&) = delete;
            FlatRungScope& operator=(const FlatRungScope&) = delete;

        private:
            GuiPerfFloor::Rung m_before;
        };

        //! A child of the widget named "holder" of a twin's tree
        template<typename W>
        W* inHolder(QWidget& root, const char* name)
        {
            auto* holder = root.findChild<QWidget*>(QStringLiteral("holder"));
            return holder ? holder->findChild<W*>(QString::fromLatin1(name)) : nullptr;
        }

        //! The contents margins of a widget as text, for the failure message of a comparison
        std::string marginsOf(const QWidget* widget)
        {
            return ToolTipTwins::marginsText(widget->contentsMargins());
        }

        // ----------------------------------------------------------------------------------
        // BreadCrumbs.qss:20-24, `AzQtComponents--BreadCrumbs #MenuButton { qproperty-icon: url(:/Breadcrumb/img/UI20/Breadcrumb/
        // dot-dot-dot_with_arrow.svg); qproperty-iconSize: 22px 11px }`: the button that opens the menu of a path too wide for the
        // breadcrumbs
        // ----------------------------------------------------------------------------------

        const char* const BreadCrumbsMenuIcon = ":/Breadcrumb/img/UI20/Breadcrumb/dot-dot-dot_with_arrow.svg";      // BreadCrumbs.qss:22
        const QSize BreadCrumbsMenuIconSize(22, 11);                                                                // BreadCrumbs.qss:23

        //! Breadcrumbs with a path far wider than the window, so that the button that opens the menu of the path is shown; and below
        //! them two buttons no rule reaches: one named as the breadcrumbs' button but below no breadcrumbs, and one given the sheet's
        //! icon and icon size by hand, which a button with the rule's properties is compared with
        void buildOverflowingBreadCrumbs(QWidget& root, bool)
        {
            auto* layout = new QVBoxLayout(&root);
            layout->setContentsMargins(0, 0, 0, 0);

            auto* crumbs = new BreadCrumbs(&root);
            crumbs->setObjectName(QStringLiteral("overflowing"));
            crumbs->setCurrentPath(QStringLiteral("Levels/Prefabs/Characters/Heroes/Armour/Helmets"));
            layout->addWidget(crumbs);

            auto* holder = new QWidget(&root);
            holder->setObjectName(QStringLiteral("holder"));
            auto* holderLayout = new QHBoxLayout(holder);
            auto* elsewhere = new QToolButton(holder);
            elsewhere->setObjectName(QStringLiteral("MenuButton"));
            elsewhere->setAutoRaise(true);
            holderLayout->addWidget(elsewhere);
            auto* byHand = new QToolButton(holder);
            byHand->setObjectName(QStringLiteral("byHand"));
            byHand->setAutoRaise(true);
            byHand->setIcon(QIcon(QString::fromLatin1(BreadCrumbsMenuIcon)));
            byHand->setIconSize(BreadCrumbsMenuIconSize);
            holderLayout->addWidget(byHand);
            layout->addWidget(holder);
            layout->addStretch();
        }

        TEST_F(AppCarrierTests, BreadCrumbsMenuButton_HasTheIconAndTheIconSizeOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            const FlatRungScope rung;

            AppTwins twins(*this, buildOverflowingBreadCrumbs, appSheet(), QSize(120, 160), Qt::LeftToRight);
            auto* legacyCrumbs = twins.legacyChild<BreadCrumbs>("overflowing");
            auto* flatCrumbs = twins.flatChild<BreadCrumbs>("overflowing");
            ASSERT_TRUE(legacyCrumbs != nullptr && flatCrumbs != nullptr);
            auto* legacyButton = legacyCrumbs->findChild<QToolButton*>(QStringLiteral("MenuButton"));
            auto* flatButton = flatCrumbs->findChild<QToolButton*>(QStringLiteral("MenuButton"));
            ASSERT_TRUE(legacyButton != nullptr && flatButton != nullptr);
            auto* legacyByHand = twins.legacyChild<QToolButton>("byHand");
            auto* flatByHand = twins.flatChild<QToolButton>("byHand");
            ASSERT_TRUE(legacyByHand != nullptr && flatByHand != nullptr);

            // shown while the path is truncated (BreadCrumbs::resizeEvent)
            EXPECT_TRUE(legacyButton->isVisibleTo(&twins.m_legacy));
            EXPECT_TRUE(flatButton->isVisibleTo(&twins.m_flat));

            // qproperty-iconSize
            EXPECT_EQ(sizeText(legacyButton->iconSize()), sizeText(BreadCrumbsMenuIconSize));
            EXPECT_EQ(sizeText(flatButton->iconSize()), sizeText(legacyButton->iconSize()));

            // qproperty-icon: the file the rule names, as the pixmap of each ratio
            const QIcon sheetIcon{ QString::fromLatin1(BreadCrumbsMenuIcon) };
            for (const qreal ratio : AllRatios)
            {
                const QImage expected = sheetIcon.pixmap(BreadCrumbsMenuIconSize, ratio).toImage();
                EXPECT_TRUE(isDrawnOn(expected)) << "ratio " << ratio;
                EXPECT_EQ(differing(legacyButton->icon().pixmap(legacyButton->iconSize(), ratio).toImage(), expected), 0)
                    << "legacy, ratio " << ratio;
                EXPECT_EQ(differing(flatButton->icon().pixmap(flatButton->iconSize(), ratio).toImage(), expected), 0)
                    << "flat, ratio " << ratio;
            }

            // the rule decides the icon and its size and nothing else: the button is the button given the two by hand, in its hint and in
            // its pixels, and its pixels are the legacy twin's
            EXPECT_EQ(sizeText(legacyButton->sizeHint()), sizeText(legacyByHand->sizeHint()));
            EXPECT_EQ(sizeText(flatButton->sizeHint()), sizeText(flatByHand->sizeHint()));
            const QSize paintSize(36, 24);
            for (const qreal ratio : AllRatios)
            {
                const QImage legacyImage = paintedToolButton(legacyButton, paintSize, ratio);
                const QImage flatImage = paintedToolButton(flatButton, paintSize, ratio);
                EXPECT_FALSE(inkBounds(legacyImage).isNull()) << "ratio " << ratio;
                EXPECT_EQ(differing(legacyImage, paintedToolButton(legacyByHand, paintSize, ratio)), 0) << "legacy, ratio " << ratio;
                EXPECT_EQ(differing(flatImage, paintedToolButton(flatByHand, paintSize, ratio)), 0) << "flat, ratio " << ratio;
                EXPECT_EQ(rectText(inkBounds(flatImage)), rectText(inkBounds(legacyImage))) << "ratio " << ratio;
                EXPECT_EQ(differingAlpha(flatImage, legacyImage), 0) << "ratio " << ratio;
            }

            // the rule is the breadcrumbs': a button of that name below no breadcrumbs keeps the icon it has (none)
            auto* legacyElsewhere = inHolder<QToolButton>(twins.m_legacy, "MenuButton");
            auto* flatElsewhere = inHolder<QToolButton>(twins.m_flat, "MenuButton");
            ASSERT_TRUE(legacyElsewhere != nullptr && flatElsewhere != nullptr);
            EXPECT_TRUE(legacyElsewhere->icon().isNull());
            EXPECT_TRUE(flatElsewhere->icon().isNull());
            ASSERT_TRUE(BoxElement::forWidget(flatButton) != nullptr);
            EXPECT_EQ(std::string(BoxElement::forWidget(flatButton)->m_name), "Global.Box.BreadCrumbsMenuButton");
            EXPECT_TRUE(BoxElement::forWidget(flatElsewhere) == nullptr);
            EXPECT_TRUE(BoxElement::forWidget(flatByHand) == nullptr);
        }

        TEST_F(AppCarrierTests, BreadCrumbsMenuButton_TheLegacyTwinHasNoIconWithoutTheRule)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            const FlatRungScope rung;

            // the witness: the application's sheet less the rule, so that what the twin above reads is the rule's
            const Replacements withoutRule = { { "AzQtComponents--BreadCrumbs #MenuButton", "AzQtComponents--BreadCrumbs #NoSuchButton" } };
            ASSERT_TRUE(appSheet().contains(QStringLiteral("AzQtComponents--BreadCrumbs #MenuButton")));
            AppTwins twins(*this, buildOverflowingBreadCrumbs, appSheet(withoutRule), QSize(120, 160), Qt::LeftToRight);
            auto* legacyButton = twins.legacyChild<BreadCrumbs>("overflowing")->findChild<QToolButton*>(QStringLiteral("MenuButton"));
            ASSERT_TRUE(legacyButton != nullptr);
            EXPECT_TRUE(legacyButton->icon().isNull());
            EXPECT_NE(sizeText(legacyButton->iconSize()), sizeText(BreadCrumbsMenuIconSize));
        }

        // ----------------------------------------------------------------------------------
        // MessageBox.qss:11-22, `QMessageBox QLabel#qt_msgbox_label { padding-top, -right, -bottom: 20px }` and `QMessageBox
        // QLabel#qt_msgboxex_icon_label { padding-left, -top: 20px }`: the two labels Qt names in every message box. Over the label rule
        // of Text.qss:89-95 (the margin of 1px above and below), and over the picker's `AzQtComponents--ColorPicker QLabel { padding: 0px;
        // margin: 0px }` (ColorPicker.qss:45-49) where the picker is the parent of the box (ColorPicker.cpp asks its questions so).
        // ----------------------------------------------------------------------------------

        //! A frame that answers inherits() for the color picker (the class is not linked into these tests, and the matcher asks for no
        //! more). The legacy twin names the picker's rule for its labels by QFrame instead: the same two type names
        class ColorPickerStandIn
            : public QFrame
        {
        public:
            using QFrame::QFrame;

            void* qt_metacast(const char* name) override
            {
                return qstrcmp(name, "AzQtComponents::ColorPicker") == 0 ? static_cast<void*>(this) : QFrame::qt_metacast(name);
            }
        };

        QMessageBox* addMessageBox(QWidget* parent, const char* name, bool withIcon)
        {
            auto* box = new QMessageBox(parent);
            box->setObjectName(QString::fromLatin1(name));
            box->setWindowTitle(QStringLiteral("Save"));
            box->setText(QStringLiteral("The level could not be saved."));
            box->setStandardButtons(QMessageBox::NoButton);
            if (withIcon)
            {
                box->setIconPixmap(discIcon().pixmap(QSize(32, 32)));
            }
            return box;
        }

        //! A message box without an icon and one with, each in a plain window and below a color picker; and a label named as the text label
        //! of a message box but in none
        void buildMessageBoxes(QWidget& root, bool)
        {
            addMessageBox(&root, "plain", false);
            addMessageBox(&root, "iconic", true);
            auto* picker = new ColorPickerStandIn(&root);
            picker->setObjectName(QStringLiteral("picker"));
            addMessageBox(picker, "pickerPlain", false);
            addMessageBox(picker, "pickerIconic", true);

            auto* holder = new QWidget(&root);
            holder->setObjectName(QStringLiteral("holder"));
            auto* holderLayout = new QVBoxLayout(holder);
            auto* stray = new QLabel(QStringLiteral("Stray"), holder);
            stray->setObjectName(QStringLiteral("qt_msgbox_label"));
            holderLayout->addWidget(stray);
        }

        //! The boxes shown, as the application shows them but not on the screen (QMessageBox::showEvent sizes the box to its layout). The
        //! layout of a box takes its margins and spacing from the style (QLayout, PM_Layout*Margin): the application's is Style, whose
        //! values are 5 and 3 (Style.cpp), and the legacy twin's base is Fusion. Both twins are given Style's by hand, so that what
        //! the boxes differ in is the labels.
        void showMessageBoxes(QWidget& root)
        {
            constexpr int layoutMargin = 5;
            constexpr int layoutSpacing = 3;
            for (QMessageBox* box : root.findChildren<QMessageBox*>())
            {
                box->setAttribute(Qt::WA_DontShowOnScreen);
                box->show();
                box->layout()->setContentsMargins(layoutMargin, layoutMargin, layoutMargin, layoutMargin);
                box->layout()->setSpacing(layoutSpacing);
                // QMessageBox::setText sizes the box to its layout again
                box->setText(box->text());
            }
            QApplication::processEvents();
        }

        //! A label as its paint event draws it, onto an image of an explicit device ratio, on grey
        QImage renderedLabel(QLabel* label, qreal ratio)
        {
            QImage image((QSizeF(label->size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor(0x80, 0x80, 0x80));
            label->render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
            return image;
        }

        struct MessageBoxCase
        {
            const char* m_name;
            bool m_icon;
            QMargins m_text;                //!< the contents margins of the text label: left, top, right, bottom
            QMargins m_iconLabel;           //!< and of the icon label
            const char* m_element;          //!< the flat twin's element of the text label
            const char* m_iconElement;      //!< and of the icon label
        };

        const MessageBoxCase MessageBoxCases[] = {
            // the margin of 1px above and below (Text.qss:93-94) and the padding of the rules (MessageBox.qss:13-15, :20-21)
            { "plain", false, QMargins(0, 21, 20, 21), QMargins(20, 21, 0, 1), "Global.Box.MessageBoxText", "Global.Box.MessageBoxIcon" },
            { "iconic", true, QMargins(0, 21, 20, 21), QMargins(20, 21, 0, 1), "Global.Box.MessageBoxText", "Global.Box.MessageBoxIcon" },
            // below a picker: its `margin: 0px` outranks the label rule's margin (ColorPicker.qss:48), and the padding of the message box's
            // rules outranks its `padding: 0px` (:47)
            { "pickerPlain", false, QMargins(0, 20, 20, 20), QMargins(20, 20, 0, 0), "Global.Box.MessageBoxTextInColorPicker",
                "Global.Box.MessageBoxIconInColorPicker" },
            { "pickerIconic", true, QMargins(0, 20, 20, 20), QMargins(20, 20, 0, 0), "Global.Box.MessageBoxTextInColorPicker",
                "Global.Box.MessageBoxIconInColorPicker" },
        };

        TEST_F(AppCarrierTests, MessageBoxLabels_HaveTheBoxOfTheSheetWithAndWithoutAnIcon)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            const FlatRungScope rung;
            // the picker's elements are there to be outranked, as in the application
            ColorPickerElements::registerGlobal();

            const Replacements pickerAsFrame = { { "AzQtComponents--ColorPicker QLabel", "QFrame QLabel" } };
            AppTwins twins(*this, buildMessageBoxes, appSheet(pickerAsFrame), QSize(100, 100), Qt::LeftToRight);
            showMessageBoxes(twins.m_legacy);
            showMessageBoxes(twins.m_flat);

            for (const MessageBoxCase& boxCase : MessageBoxCases)
            {
                SCOPED_TRACE(boxCase.m_name);
                auto* legacyBox = twins.legacyChild<QMessageBox>(boxCase.m_name);
                auto* flatBox = twins.flatChild<QMessageBox>(boxCase.m_name);
                ASSERT_TRUE(legacyBox != nullptr && flatBox != nullptr);
                EXPECT_TRUE(legacyBox->isVisible());
                EXPECT_TRUE(flatBox->isVisible());

                for (const char* labelName : { "qt_msgbox_label", "qt_msgboxex_icon_label" })
                {
                    SCOPED_TRACE(labelName);
                    const bool text = qstrcmp(labelName, "qt_msgbox_label") == 0;
                    auto* legacyLabel = legacyBox->findChild<QLabel*>(QString::fromLatin1(labelName));
                    auto* flatLabel = flatBox->findChild<QLabel*>(QString::fromLatin1(labelName));
                    ASSERT_TRUE(legacyLabel != nullptr && flatLabel != nullptr);
                    // a label that is hidden (the icon label of a box without an icon) is polished when asked to be
                    legacyLabel->ensurePolished();
                    flatLabel->ensurePolished();

                    // the box of the label: its contents, and so the frame width Qt reads from them
                    const QMargins expected = text ? boxCase.m_text : boxCase.m_iconLabel;
                    EXPECT_EQ(marginsOf(legacyLabel), ToolTipTwins::marginsText(expected));
                    EXPECT_EQ(marginsOf(flatLabel), marginsOf(legacyLabel));
                    EXPECT_EQ(flatLabel->frameWidth(), legacyLabel->frameWidth());
                    EXPECT_EQ(rectText(flatLabel->contentsRect()), rectText(legacyLabel->contentsRect()));

                    // the hints that follow from it, and where the layout of the box put the label
                    EXPECT_EQ(sizeText(flatLabel->sizeHint()), sizeText(legacyLabel->sizeHint()));
                    EXPECT_EQ(sizeText(flatLabel->minimumSizeHint()), sizeText(legacyLabel->minimumSizeHint()));
                    EXPECT_EQ(rectText(flatLabel->geometry()), rectText(legacyLabel->geometry()));
                    EXPECT_EQ(flatLabel->isVisibleTo(flatBox), text || boxCase.m_icon);
                    EXPECT_EQ(legacyLabel->isVisibleTo(legacyBox), flatLabel->isVisibleTo(flatBox));

                    // the pixels of a label that is shown, at the ratios of the GUI
                    if (legacyLabel->isVisibleTo(legacyBox))
                    {
                        for (const qreal ratio : AllRatios)
                        {
                            const QImage legacyImage = renderedLabel(legacyLabel, ratio);
                            const QImage flatImage = renderedLabel(flatLabel, ratio);
                            EXPECT_TRUE(isDrawnOn(legacyImage)) << "ratio " << ratio;
                            EXPECT_EQ(differing(flatImage, legacyImage), 0)
                                << "ratio " << ratio << ": " << firstDifference(flatImage, legacyImage);
                        }
                    }

                    // the element of the label
                    const BoxElement* element = BoxElement::forWidget(flatLabel);
                    ASSERT_TRUE(element != nullptr);
                    EXPECT_EQ(std::string(element->m_name), text ? boxCase.m_element : boxCase.m_iconElement);
                }

                // the box that follows the layout of its labels
                EXPECT_EQ(sizeText(flatBox->size()), sizeText(legacyBox->size()));
                EXPECT_EQ(sizeText(flatBox->sizeHint()), sizeText(legacyBox->sizeHint()));
            }
        }

        TEST_F(AppCarrierTests, MessageBoxLabels_OnlyTheLabelsOfAMessageBoxHaveTheBox)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            const FlatRungScope rung;

            AppTwins twins(*this, buildMessageBoxes, appSheet(), QSize(100, 100), Qt::LeftToRight);
            auto* legacyStray = inHolder<QLabel>(twins.m_legacy, "qt_msgbox_label");
            auto* flatStray = inHolder<QLabel>(twins.m_flat, "qt_msgbox_label");
            ASSERT_TRUE(legacyStray != nullptr && flatStray != nullptr);

            // a label of that name in no message box has the label rule's margin alone
            EXPECT_EQ(marginsOf(legacyStray), ToolTipTwins::marginsText(QMargins(0, 1, 0, 1)));
            EXPECT_EQ(marginsOf(flatStray), marginsOf(legacyStray));
            EXPECT_TRUE(BoxElement::forWidget(flatStray) == nullptr);
        }

        TEST_F(AppCarrierTests, MessageBoxLabels_TheLegacyTwinHasTheLabelMarginAloneWithoutTheRules)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            const FlatRungScope rung;

            // the witness: the application's sheet less the two rules
            const Replacements withoutRules = {
                { "QMessageBox QLabel#qt_msgbox_label", "QMessageBox QLabel#no_such_label" },
                { "QMessageBox QLabel#qt_msgboxex_icon_label", "QMessageBox QLabel#no_such_icon" },
            };
            ASSERT_TRUE(appSheet().contains(QStringLiteral("QMessageBox QLabel#qt_msgbox_label")));
            ASSERT_TRUE(appSheet().contains(QStringLiteral("QMessageBox QLabel#qt_msgboxex_icon_label")));
            AppTwins twins(*this, buildMessageBoxes, appSheet(withoutRules), QSize(100, 100), Qt::LeftToRight);
            showMessageBoxes(twins.m_legacy);
            auto* legacyBox = twins.legacyChild<QMessageBox>("iconic");
            ASSERT_TRUE(legacyBox != nullptr);
            for (const char* labelName : { "qt_msgbox_label", "qt_msgboxex_icon_label" })
            {
                auto* label = legacyBox->findChild<QLabel*>(QString::fromLatin1(labelName));
                ASSERT_TRUE(label != nullptr);
                EXPECT_EQ(marginsOf(label), ToolTipTwins::marginsText(QMargins(0, 1, 0, 1))) << labelName;
            }
        }

        // ----------------------------------------------------------------------------------
        // Slider.qss:10-20, `AzQtComponents--Slider.HorizontalSlider { qproperty-toolTipOffsetX: -8; qproperty-toolTipOffsetY: -16 }` and
        // `AzQtComponents--Slider.VerticalSlider { qproperty-toolTipOffsetX: 8; qproperty-toolTipOffsetY: -24 }`: the offset of the tip
        // of a slider's value from where the Config puts it (SliderConfig.ini: (-8, -16) and (8, -24)), which Slider::ShowValueToolTip adds
        // ----------------------------------------------------------------------------------

        //! Two sliders of each orientation, one for each class that derives from Slider. The legacy twin has not the style that gives a
        //! slider its class (Slider::polish, Slider.cpp:495-519): its sliders have the classes by hand
        void buildSliders(QWidget& root, bool flat)
        {
            auto* layout = new QGridLayout(&root);
            const auto add = [&](Slider* slider, const char* name, Qt::Orientation orientation, int row, int column)
            {
                slider->setObjectName(QString::fromLatin1(name));
                if (!flat)
                {
                    const bool horizontal = orientation == Qt::Horizontal;
                    slider->setProperty("class", horizontal ? QStringLiteral("HorizontalSlider") : QStringLiteral("VerticalSlider"));
                }
                layout->addWidget(slider, row, column);
            };
            add(new SliderInt(Qt::Horizontal, &root), "horizontalInt", Qt::Horizontal, 0, 0);
            add(new SliderDouble(Qt::Horizontal, &root), "horizontalDouble", Qt::Horizontal, 0, 1);
            add(new SliderInt(Qt::Vertical, &root), "verticalInt", Qt::Vertical, 1, 0);
            add(new SliderDouble(Qt::Vertical, &root), "verticalDouble", Qt::Vertical, 1, 1);
        }

        struct SliderCase
        {
            const char* m_name;
            QPoint m_offset;
            const char* m_element;
        };

        const SliderCase SliderCases[] = {
            { "horizontalInt", QPoint(-8, -16), "Global.Box.HorizontalSliderToolTip" },         // Slider.qss:12-13
            { "horizontalDouble", QPoint(-8, -16), "Global.Box.HorizontalSliderToolTip" },
            { "verticalInt", QPoint(8, -24), "Global.Box.VerticalSliderToolTip" },              // Slider.qss:18-19
            { "verticalDouble", QPoint(8, -24), "Global.Box.VerticalSliderToolTip" },
        };

        TEST_F(AppCarrierTests, Slider_ToolTipOffsetsAreThoseOfTheSheet)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            const FlatRungScope rung;

            AppTwins twins(*this, buildSliders, appSheet(), QSize(200, 200), Qt::LeftToRight);
            for (const SliderCase& sliderCase : SliderCases)
            {
                SCOPED_TRACE(sliderCase.m_name);
                auto* legacy = twins.legacyChild<Slider>(sliderCase.m_name);
                auto* flat = twins.flatChild<Slider>(sliderCase.m_name);
                ASSERT_TRUE(legacy != nullptr && flat != nullptr);
                EXPECT_EQ(legacy->toolTipOffset(), sliderCase.m_offset);
                EXPECT_EQ(flat->toolTipOffset(), sliderCase.m_offset) << flat->toolTipOffsetX() << ", " << flat->toolTipOffsetY();
                ASSERT_TRUE(BoxElement::forWidget(flat) != nullptr);
                EXPECT_EQ(std::string(BoxElement::forWidget(flat)->m_name), sliderCase.m_element);
            }
        }

        TEST_F(AppCarrierTests, Slider_TheSliderInsideIsNotReached)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            const FlatRungScope rung;

            AppTwins twins(*this, buildSliders, appSheet(), QSize(200, 200), Qt::LeftToRight);
            // the rules name AzQtComponents::Slider: the QSlider that draws it carries the same class, and has no such property
            auto* flat = twins.flatChild<Slider>("horizontalInt");
            ASSERT_TRUE(flat != nullptr);
            auto* inner = flat->findChild<QSlider*>();
            ASSERT_TRUE(inner != nullptr);
            EXPECT_TRUE(Style::hasClass(inner, QStringLiteral("HorizontalSlider")));
            EXPECT_TRUE(BoxElement::forWidget(inner) == nullptr);
            EXPECT_FALSE(inner->property("toolTipOffsetX").isValid());
        }

        TEST_F(AppCarrierTests, Slider_TheLegacyTwinHasNoOffsetWithoutTheRules)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            const FlatRungScope rung;

            // the witness: the application's sheet less the two rules
            const Replacements withoutRules = {
                { "AzQtComponents--Slider.HorizontalSlider", "AzQtComponents--Slider.NoSuchSlider" },
                { "AzQtComponents--Slider.VerticalSlider", "AzQtComponents--Slider.NoSuchSlider" },
            };
            ASSERT_TRUE(appSheet().contains(QStringLiteral("AzQtComponents--Slider.HorizontalSlider")));
            AppTwins twins(*this, buildSliders, appSheet(withoutRules), QSize(200, 200), Qt::LeftToRight);
            for (const SliderCase& sliderCase : SliderCases)
            {
                EXPECT_EQ(twins.legacyChild<Slider>(sliderCase.m_name)->toolTipOffset(), QPoint()) << sliderCase.m_name;
            }
        }

        TEST_F(AppCarrierTests, Slider_TheClassIsKnownWhenTheElementIsResolvedAtPolish)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            const FlatRungScope rung;

            // Slider::polish tags the slider with its orientation inside Style::polish, and the element is resolved at the end of it
            QWidget root;
            root.setStyle(m_style.get());
            for (const Qt::Orientation orientation : { Qt::Horizontal, Qt::Vertical })
            {
                auto* slider = new SliderInt(orientation, &root);
                slider->setStyle(m_style.get());
                EXPECT_EQ(slider->toolTipOffset(), QPoint());
                EXPECT_TRUE(BoxElement::forWidget(slider) == nullptr);

                slider->ensurePolished();
                const bool horizontal = orientation == Qt::Horizontal;
                EXPECT_TRUE(Style::hasClass(slider, horizontal ? QStringLiteral("HorizontalSlider") : QStringLiteral("VerticalSlider")));
                EXPECT_EQ(slider->toolTipOffset(), horizontal ? QPoint(-8, -16) : QPoint(8, -24));
            }
        }

        //! Where the tip of the value of a slider is shown while it is dragged: the value changes with the slider held
        //! (Slider::sliderIsInMoving) and the slider asks QToolTip for the tip at the point it works out (Slider::ShowValueToolTip).
        //! None: no tip was shown.
        std::optional<QPoint> draggedTipPosition(SliderInt* slider)
        {
            QToolTip::hideText();
            slider->sliderIsInMoving(true);
            slider->setValue(slider->value() == 5 ? 6 : 5);
            slider->sliderIsInMoving(false);

            std::optional<QPoint> position;
            if (QLabel* tip = ToolTipTwins::findTip())
            {
                position = tip->pos();
                delete tip;
            }
            return position;
        }

        TEST_F(AppCarrierTests, Slider_TheToolTipIsShownWhereTheSheetPutsIt)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());
            // The pointer is where the tip is placed from (Slider::showHoverToolTip): it is put well inside the screen, so that the
            // screen's edge moves no tip. Only the screen of the offscreen platform is the test's to move.
            if (QGuiApplication::platformName() != QLatin1String("offscreen"))
            {
                GTEST_SKIP() << "the pointer of a screen is not the test's to move";
            }
            const FlatRungScope rung;
            const ToolTipTwins::Guard tipGuard;

            QCursor::setPos(QPoint(300, 300));
            if (QCursor::pos() != QPoint(300, 300))
            {
                GTEST_SKIP() << "the platform keeps no pointer position";
            }

            AppTwins twins(*this, buildSliders, appSheet(), QSize(200, 200), Qt::LeftToRight);
            twins.m_legacy.move(60, 100);
            twins.m_flat.move(60, 100);
            QApplication::processEvents();

            // a slider of each orientation, the first and the third of the cases (the SliderInt of each)
            for (const SliderCase* sliderCase : { &SliderCases[0], &SliderCases[2] })
            {
                SCOPED_TRACE(sliderCase->m_name);
                auto* legacy = twins.legacyChild<SliderInt>(sliderCase->m_name);
                auto* flat = twins.flatChild<SliderInt>(sliderCase->m_name);
                ASSERT_TRUE(legacy != nullptr && flat != nullptr);
                const QPoint offset = sliderCase->m_offset;
                ASSERT_EQ(legacy->toolTipOffset(), offset);
                ASSERT_EQ(flat->toolTipOffset(), offset);

                // The sliders of the twins are as large as their base styles make them, and the tip is placed from the slider's edge
                // and the pointer: what is compared is how far the sheet's offset moves the tip. The same slider with the offset a
                // slider has of itself (none) shows the tip that much nearer to the pointer and the slider.
                for (SliderInt* slider : { legacy, flat })
                {
                    SCOPED_TRACE(slider == legacy ? "legacy" : "flat");
                    const std::optional<QPoint> tip = draggedTipPosition(slider);
                    slider->setToolTipOffset(QPoint());
                    const std::optional<QPoint> bareTip = draggedTipPosition(slider);
                    slider->setToolTipOffset(offset);
                    ASSERT_TRUE(tip.has_value() && bareTip.has_value());
                    EXPECT_EQ(tip->x() - bareTip->x(), offset.x());
                    EXPECT_EQ(tip->y() - bareTip->y(), offset.y());
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // The entry of a form (ProjectManager.qss:299-307): `height: 1em`, the height of the font of the rule
        // ----------------------------------------------------------------------------------

        //! ProjectManager.qss:299-307, over LineEdit.qss:11-19 and BaseStyleSheet.qss:68-82 (both are in the application's sheet)
        const char* const FormEntryRules =
            "#formFrame QLineEdit { background-color: rgba(0,0,0,0); font-size: 18px; color: #ffffff; border:0; line-height: 30px; height: 1em;"
            " padding-top: -4px; }";

        //! The element of the Project Manager for it (ProjectManager/Source/FlatElements.cpp, FormLineEdit): the 24px of an em of Open Sans at 18px
        const BoxElement FormEntry{
            .m_name = "Test.AppCarrier.FormEntry",
            .m_padding = QMargins(0, -4, 0, 1),               // LineEdit.qss:15,18 and ProjectManager.qss:306
            .m_nativeBorder = false,                          // border: ProjectManager.qss:303
            .m_radius = 2,                                    // BaseStyleSheet.qss:79
            .m_size = QSize(-1, 24),                          // height: ProjectManager.qss:305
            .m_idle = { .m_fill = QBrush(QColor(0, 0, 0, 0)) },
        };

        void buildFormEntry(QWidget& root, bool flat)
        {
            auto* layout = new QVBoxLayout(&root);
            layout->setContentsMargins(0, 0, 0, 0);
            auto* frame = new QFrame(&root);
            frame->setObjectName(QStringLiteral("formFrame"));
            auto* frameLayout = new QVBoxLayout(frame);
            frameLayout->setContentsMargins(0, 0, 0, 0);
            auto* entry = new QLineEdit(QStringLiteral("Project name"), frame);
            entry->setObjectName(QStringLiteral("entry"));
            if (flat)
            {
                entry->setProperty("flatLook", QStringLiteral("entry"));
            }
            frameLayout->addWidget(entry);
            layout->addWidget(frame);
            layout->addStretch();
        }

        TEST_F(AppCarrierTests, FormEntry_TheHintOfAnEntryWhoseHeightIsAnEmIsTheSheetsAtTheFontOfN2)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            // An em is QFontMetrics::height() of the font of the rule (qcssparser.cpp:461-467): the 18px Open Sans of the library, 24px high as in n2
            const int fontId = QFontDatabase::addApplicationFont(QStringLiteral(":/AzQtFonts/Fonts/Open_Sans/OpenSans-Regular.ttf"));
            QFont font(QStringLiteral("Open Sans"));
            font.setPixelSize(18);
            if (fontId < 0 || QFontInfo(font).family() != QLatin1String("Open Sans"))
            {
                GTEST_SKIP() << "Open Sans is not available: an em is the height of another font";
            }

            static bool registered = false;
            if (!registered)
            {
                registered = true;
                BoxElement::registerElement(&FormEntry, &QLineEdit::staticMetaObject, [](const QWidget* w) { return flatLook(w, "entry"); });
            }

            AppTwins twins(*this, buildFormEntry, appSheet() + QString::fromUtf8(FormEntryRules), QSize(300, 100), Qt::LeftToRight);
            twins.shareFont("entry");
            auto* legacyEntry = twins.legacyChild<QLineEdit>("entry");
            auto* flatEntry = twins.flatChild<QLineEdit>("entry");
            ASSERT_TRUE(legacyEntry != nullptr && flatEntry != nullptr);
            ASSERT_TRUE(BoxElement::forWidget(flatEntry) == &FormEntry);

            // n2: the entry of every form hints 157 x 21 (24 - 4 + 1), where f5 had 23
            EXPECT_EQ(QFontMetrics(legacyEntry->font()).height(), 24);
            EXPECT_EQ(legacyEntry->sizeHint().height(), 21);
            EXPECT_EQ(sizeText(flatEntry->sizeHint()), sizeText(legacyEntry->sizeHint()));
            EXPECT_EQ(sizeText(flatEntry->minimumSizeHint()), sizeText(legacyEntry->minimumSizeHint()));
            EXPECT_EQ(sizeText(flatEntry->size()), sizeText(legacyEntry->size()));
        }
    } // namespace AppCarrierTestsInternal
} // namespace AzQtComponents
