/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ColorPickerElements.h>
#include <AzQtComponents/Components/Elements/PropertyEditorElements.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <AzQtComponents/Components/Widgets/IconElidingLabel.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QCheckBox>
#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpacerItem>
#include <QStyleFactory>
#include <QStyleOption>
#include <QToolButton>
#include <QVBoxLayout>

#include <string>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The property editor's Global box elements (PropertyEditorElements.cpp) against the legacy rules they carry. The legacy
    // twin of a widget is Qt's own style sheet style over Fusion with the application's real sheet (BaseStyleSheet.qss and the
    // files it imports, from the library's resources); the flat twin is the flattened Style with the registered Global
    // elements. Same box, same hints, same pixels.
    // ----------------------------------------------------------------------------------

    namespace PropertyEditorElementTestData
    {
        //! A widget of a class this library does not have (AzToolsFramework::PropertyRowWidget) or one that needs no more than
        //! its name here (a card header, a color picker): a QFrame that answers inherits() for the class the matchers ask.
        //! The legacy sheet names it by a type selector instead (appSheet).
        class StandIn
            : public QFrame
        {
        public:
            StandIn(const char* className, const char* objectName, QWidget* parent)
                : QFrame(parent)
                , m_className(className)
            {
                setObjectName(QString::fromLatin1(objectName));
            }

            void* qt_metacast(const char* name) override
            {
                return qstrcmp(name, m_className) == 0 ? static_cast<void*>(this) : QFrame::qt_metacast(name);
            }

        private:
            const char* m_className;
        };

        const char* const PropertyRowClass = "AzToolsFramework::PropertyRowWidget";
        const char* const CardHeaderClass = "AzQtComponents::CardHeader";
        const char* const ColorPickerClass = "AzQtComponents::ColorPicker";
        const char* const BrowseEditClass = "AzQtComponents::BrowseEdit";

        const char* const PropertyRowSelector = "AzToolsFramework--PropertyRowWidget";
        const char* const CardHeaderSelector = "AzQtComponents--CardHeader";
        const char* const ColorPickerSelector = "AzQtComponents--ColorPicker";
        const char* const BrowseEditSelector = "AzQtComponents--BrowseEdit";

        //! What StyleSheetCache makes of BaseStyleSheet.qss: each @import line replaced by the file it names, from the real
        //! resources. The class of the widget the twin stands in for (the property row, or the card header) is named by a type
        //! selector with the specificity of a class selector's, QFrame (what the stand-in is, and all that sits in its tree); the
        //! other classes the sheet names, by a type no widget has, so that none of their rules reaches the twin. The rest of the
        //! sheet is as it is; a twin of real widgets (no stand-in selector) has the sheet whole. A twin of a card header in a color picker
        //! has two stand-ins (both are QFrame to the sheet: every label of that tree is inside the picker).
        QString appSheet(const char* standInSelector, const char* secondStandIn = nullptr)
        {
            static QString expanded;
            if (expanded.isEmpty())
            {
                const auto read = [](const QString& name)
                {
                    QFile file(QStringLiteral(":/AzQtComponents/Widgets/") + name);
                    EXPECT_TRUE(file.open(QIODevice::ReadOnly)) << name.toUtf8().constData();
                    return QString::fromUtf8(file.readAll());
                };
                expanded = read(QStringLiteral("BaseStyleSheet.qss"));
                const QRegularExpression import(QStringLiteral("@import\\s+\"([^\"]+)\"\\s*;"));
                for (QRegularExpressionMatch match = import.match(expanded); match.hasMatch(); match = import.match(expanded))
                {
                    expanded.replace(match.capturedStart(), match.capturedLength(), read(match.captured(1)));
                }
            }
            QString sheet = expanded;
            for (const char* selector : { PropertyRowSelector, CardHeaderSelector, ColorPickerSelector, BrowseEditSelector })
            {
                if (standInSelector)
                {
                    const bool standIn = qstrcmp(selector, standInSelector) == 0 || (secondStandIn && qstrcmp(selector, secondStandIn) == 0);
                    sheet.replace(QString::fromLatin1(selector), QString::fromLatin1(standIn ? "QFrame" : "NoSuchType"));
                }
            }
            return sheet;
        }

        //! The flat twin is the window that carries the property; the legacy twin carries the sheet instead
        bool flatTwin(const QWidget* widget)
        {
            return widget->window()->property("propertyEditorTestsFlat").toBool();
        }

        //! #Background of a card header is a box of its own: padding 2/7/3/7 and a 1px border but below (Card.qss:94-96), so the
        //! contents are (8, 3, 8, 3). A real header gets it from Card::shapedFrameContents (Card.cpp:523-533), which a stand-in
        //! does not have, so the flat twin gets it as an element of the test.
        const BoxElement Background{
            .m_name = "Test.Box.Background",
            .m_padding = QMargins(7, 2, 7, 3),                                      // Card.qss:94
            .m_nativeBorder = false,                                                // Card.qss:95-96
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                                  // Card.qss:98
                .m_edges = { { 1, QColor(Qt::transparent) }, { 1, QColor(Qt::transparent) }, {}, { 1, QColor(Qt::transparent) } },
            },
        };

        QPixmap marker(const QSize& size, const QColor& color)
        {
            QPixmap pixmap(size);
            pixmap.fill(color);
            QPainter painter(&pixmap);
            painter.fillRect(0, 0, 4, 4, Qt::white);
            return pixmap;
        }
    } // namespace PropertyEditorElementTestData

    // The tests and their fixture sit in a namespace of their own: the using-directive below must not leak into the other files
    // of a unity build
    namespace PropertyEditorElementTestsInternal
    {
        using namespace PropertyEditorElementTestData;

        // ----------------------------------------------------------------------------------
        // Fixture: the flattened Style over Fusion, Fusion as the base of the legacy sheet, the Global elements registered
        // ----------------------------------------------------------------------------------

        class PropertyEditorElementTests
            : public ::UnitTest::LeakDetectionFixture
        {
        public:
            PropertyEditorElementTests()
            {
                m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);

                // The fonts and the text colour the application has when StyleManager sets the style (StyleManager.cpp:305-307,
                // Style.cpp:3420-3427 buildO3DEBasePalette): the legacy sheet gives labels and buttons the same
                QFont applicationFont(QStringLiteral("Open Sans"));
                applicationFont.setPixelSize(12);
                QApplication::setFont(applicationFont);
                QPalette palette;
                for (const QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText })
                {
                    palette.setColor(role, Qt::white);
                }
                QApplication::setPalette(palette);

                QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
                m_style = AZStd::make_unique<Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                TextElement::installClassFonts();

                registerElements();
            }

            ~PropertyEditorElementTests() override
            {
                m_style.reset();
                m_application.reset();
            }

            //! Registered once for the process, as the application does from Style::polish(QApplication*), and the element of the
            //! test for the stand-in header's #Background
            static void registerElements()
            {
                ColorPickerElements::registerGlobal();      // the labels of a picker are its elements' too (the registry test below)
                PropertyEditorElements::registerGlobal();

                static bool registered = false;
                if (!registered)
                {
                    registered = true;
                    BoxElement::registerElement(&Background, &QWidget::staticMetaObject,
                        [](const QWidget* w) { return flatTwin(w) && w->objectName() == QLatin1String("Background"); });
                }
            }

            //! The legacy twin: the sheet on the root, over the application's Fusion. The flat twin: the style on the root and on
            //! everything under it (a widget's style is not its children's).
            static void legacy(QWidget& root, const char* standInSelector, const char* secondStandIn)
            {
                root.setStyleSheet(appSheet(standInSelector, secondStandIn));
            }

            void flat(QWidget& root) const
            {
                root.setProperty("propertyEditorTestsFlat", true);
                root.setStyle(m_style.get());
                for (QWidget* child : root.findChildren<QWidget*>())
                {
                    child->setStyle(m_style.get());
                }
            }

            static void show(QWidget& root, const QSize& size)
            {
                root.setAttribute(Qt::WA_DontShowOnScreen);
                root.resize(size);
                root.show();
                QApplication::processEvents();
            }

            //! One tree twice, shown: the build function fills a root with it
            struct Twins
            {
                template<typename Build>
                Twins(const PropertyEditorElementTests& tests, Build build, const QSize& size, const char* standInSelector,
                    const char* secondStandIn = nullptr)
                {
                    build(m_legacy);
                    legacy(m_legacy, standInSelector, secondStandIn);
                    show(m_legacy, size);
                    build(m_flat);
                    tests.flat(m_flat);
                    show(m_flat, size);
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

                QWidget m_legacy;
                QWidget m_flat;
            };

            //! The pixels of two images that differ
            static int differing(const QImage& a, const QImage& b)
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
                        count += (a.pixel(x, y) != b.pixel(x, y)) ? 1 : 0;
                    }
                }
                return count;
            }

            //! The option QFrame::paintEvent builds for a frame (QFrame::initStyleOption)
            static QStyleOptionFrame frameOption(const QFrame* frame)
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

            //! A frame (a label is one) as a window paints it, through its own style onto an image of an explicit device ratio:
            //! its styled background (PE_Widget, QWidgetPrivate::paintBackground), then its frame (CE_ShapedFrame, QFrame::paintEvent).
            //! QWidget::render would hand the style the widget instead, and the ratio of the screen the test runs on. The image is
            //! grey: what stays grey is what neither the sheet nor the element draws.
            static QImage drawFrame(const QFrame* frame, qreal ratio)
            {
                QImage image((QSizeF(frame->size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
                image.setDevicePixelRatio(ratio);
                image.fill(QColor(0x80, 0x80, 0x80));
                QPainter painter(&image);
                if (frame->testAttribute(Qt::WA_StyledBackground))
                {
                    QStyleOption background;
                    background.initFrom(frame);
                    frame->style()->drawPrimitive(QStyle::PE_Widget, &background, &painter, frame);
                }
                const QStyleOptionFrame option = frameOption(frame);
                frame->style()->drawControl(QStyle::CE_ShapedFrame, &option, &painter, frame);
                painter.end();
                return image;
            }

            //! The frames of the named widgets (those that are one) are drawn alike by both twins, at the device ratios of the captures
            template<typename Twin>
            static void expectSameFrames(Twin& twins, std::initializer_list<const char*> names)
            {
                for (const char* name : names)
                {
                    const auto* legacyFrame = qobject_cast<const QFrame*>(twins.template legacyChild<QWidget>(name));
                    const auto* flatFrame = qobject_cast<const QFrame*>(twins.template flatChild<QWidget>(name));
                    ASSERT_TRUE(legacyFrame != nullptr && flatFrame != nullptr) << name;
                    for (const qreal ratio : { 1.0, 1.5 })
                    {
                        EXPECT_EQ(differing(drawFrame(legacyFrame, ratio), drawFrame(flatFrame, ratio)), 0) << name << " at ratio " << ratio;
                    }
                }
            }

            //! The expander is its caret (CheckBox.qss:133-139: the indicator is as large as the contents, 16x16): the sheet put the
            //! indicator of the legacy twin in the contents rect of its box, so the contents rect of the element is that rect: the 4px
            //! of the margin are at the right of the caret, where Native has them. (The caret itself is drawn by
            //! Style::drawPrimitive, in the rect the indicator gets, not by this element.)
            static void expectContentsAreTheCaret(const QCheckBox* legacyBox, const QCheckBox* flatBox)
            {
                QStyleOptionButton legacyOption;
                legacyOption.initFrom(legacyBox);
                legacyOption.rect = QRect(QPoint(0, 0), legacyBox->size());
                const QRect caret = legacyBox->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &legacyOption, legacyBox);
                EXPECT_EQ(caret.size(), QSize(16, 16));

                QStyleOptionButton flatOption;
                flatOption.initFrom(flatBox);
                flatOption.rect = QRect(QPoint(0, 0), flatBox->size());
                const BoxElement* element = BoxElement::forWidget(flatBox);
                ASSERT_TRUE(element != nullptr);
                EXPECT_EQ(element->contentsRect(&flatOption, flatBox), caret);
            }

            //! The box Qt derived from the element is the one it derived from the sheet: geometry, contents, limits and hints (not
            //! those of a widget whose size the limits fix, such as the expander: the layout takes the limits)
            template<typename Twin>
            static void expectSameBox(Twin& twins, std::initializer_list<const char*> names, bool hints = true)
            {
                for (const char* name : names)
                {
                    const QWidget* legacyWidget = twins.template legacyChild<QWidget>(name);
                    const QWidget* flatWidget = twins.template flatChild<QWidget>(name);
                    ASSERT_TRUE(legacyWidget != nullptr && flatWidget != nullptr) << name;
                    EXPECT_EQ(flatWidget->geometry(), legacyWidget->geometry()) << name;
                    EXPECT_TRUE(flatWidget->contentsMargins() == legacyWidget->contentsMargins()) << name;
                    EXPECT_EQ(flatWidget->minimumSize(), legacyWidget->minimumSize()) << name;
                    EXPECT_EQ(flatWidget->maximumSize(), legacyWidget->maximumSize()) << name;
                    if (hints)
                    {
                        EXPECT_EQ(flatWidget->sizeHint(), legacyWidget->sizeHint()) << name;
                        EXPECT_EQ(flatWidget->minimumSizeHint(), legacyWidget->minimumSizeHint()) << name;
                    }
                    if (const auto* legacyFrame = qobject_cast<const QFrame*>(legacyWidget))
                    {
                        EXPECT_EQ(qobject_cast<const QFrame*>(flatWidget)->frameWidth(), legacyFrame->frameWidth()) << name;
                    }
                }
            }

            //! A push button as its style draws it for a state (CE_PushButton: bevel and label), at a device ratio
            static QImage draw(const QPushButton* button, QStyle::State state, const QRect& rect, qreal ratio)
            {
                QStyleOptionButton option;
                option.initFrom(button);
                option.state = state;
                option.features = button->isFlat() ? QStyleOptionButton::Flat : QStyleOptionButton::None;
                option.rect = rect;
                option.text = button->text();
                option.icon = button->icon();
                option.iconSize = button->iconSize();
                QImage image((QSizeF(rect.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
                image.setDevicePixelRatio(ratio);
                image.fill(QColor(0x80, 0x80, 0x80));
                QPainter painter(&image);
                button->style()->drawControl(QStyle::CE_PushButton, &option, &painter, button);
                return image;
            }

            //! One property row as PropertyRowWidget builds it (PropertyRowWidget.cpp:37-123), in a vertical layout like the
            //! rows of a property editor
            static void buildRow(QWidget& root, const char* text, bool withDefaultLabel)
            {
                auto* column = new QVBoxLayout(&root);
                column->setContentsMargins(0, 0, 0, 0);

                auto* row = new StandIn(PropertyRowClass, "propertyRow", &root);
                row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
                column->addWidget(row);
                column->addStretch(1);

                auto* mainLayout = new QHBoxLayout();
                mainLayout->setSpacing(0);
                mainLayout->setContentsMargins(0, 1, 0, 1);                         // PropertyRowWidget.cpp:47-49
                auto* leftParent = new QVBoxLayout(nullptr);
                auto* leftRow = new QHBoxLayout(nullptr);
                leftParent->addLayout(leftRow);
                leftParent->setSpacing(0);
                leftRow->setSpacing(0);
                leftParent->setContentsMargins(0, 0, 0, 0);
                leftRow->setContentsMargins(0, 0, 0, 0);
                auto* middleLayout = new QHBoxLayout(nullptr);
                middleLayout->setSpacing(0);
                middleLayout->setContentsMargins(0, 0, 0, 0);

                // the indicator button (16x16 and nothing else here) and the indent of a row one level down
                auto* indicator = new QWidget(row);
                indicator->setObjectName(QStringLiteral("Indicator"));
                indicator->setFixedSize(16, 16);
                auto* left = new QWidget(row);
                left->setObjectName(QStringLiteral("left"));
                auto* middle = new QWidget(row);
                middle->setObjectName(QStringLiteral("middle"));
                middle->setMinimumWidth(142);                                       // PropertyRowWidget.cpp:77-78
                mainLayout->addWidget(left, 2, Qt::AlignLeft);
                mainLayout->addWidget(middle, 3);
                left->setLayout(leftParent);
                middle->setLayout(middleLayout);
                middleLayout->addStretch();
                leftRow->addWidget(indicator);
                leftRow->addSpacerItem(new QSpacerItem(20, 1, QSizePolicy::Fixed, QSizePolicy::Fixed));

                auto* name = new ElidingLabel(row);                                 // ElidingLabel::setObjectName names its hidden label too
                name->setObjectName(QStringLiteral("Name"));
                name->setText(QString::fromLatin1(text));
                leftRow->addWidget(name);

                if (withDefaultLabel)
                {
                    auto* defaultLabel = new QLabel(row);
                    defaultLabel->setObjectName(QStringLiteral("DefaultLabel"));
                    defaultLabel->setText(QStringLiteral("2 elements"));
                    middleLayout->insertWidget(0, defaultLabel, 1);
                    defaultLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
                }
                row->setLayout(mainLayout);
            }

            //! A card header as CardHeader builds it (CardHeader.cpp:98-166): a frame with the parts in a horizontal layout
            //! that has no spacing, the parts named as Card.qss names them
            static void buildHeader(QWidget& root, const char* menuName, bool framedIcon = false)
            {
                auto* outer = new QVBoxLayout(&root);
                outer->setContentsMargins(0, 0, 0, 0);

                auto* header = new StandIn(CardHeaderClass, "cardHeader", &root);
                header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
                auto* background = new QFrame(header);
                background->setObjectName(QStringLiteral("Background"));
                background->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

                auto* expander = new QCheckBox(background);
                expander->setObjectName(QStringLiteral("Expander"));
                expander->setProperty("class", QStringLiteral("Expander"));         // CheckBox::applyExpanderStyle
                expander->setChecked(true);
                auto* icon = new QLabel(background);
                icon->setObjectName(QStringLiteral("Icon"));
                icon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                icon->setProperty("class", QStringLiteral("CardHeaderIcon"));
                icon->setPixmap(marker(QSize(16, 16), QColor(0x40, 0xA0, 0x60)));
                if (framedIcon)
                {
                    icon->setFrameShape(QFrame::StyledPanel);
                }
                auto* title = new ElidingLabel(background);
                title->setObjectName(QStringLiteral("Title"));
                title->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
                title->setText(QStringLiteral("Camera"));
                auto* warning = new QLabel(background);
                warning->setObjectName(QStringLiteral("WarningIcon"));
                warning->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                warning->setPixmap(marker(QSize(16, 14), QColor(0xF0, 0xC3, 0x2D)));
                auto* help = new QPushButton(background);
                help->setObjectName(QStringLiteral("Help"));
                help->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                help->setProperty("class", QStringLiteral("CardHeaderMenu"));
                auto* menu = new QPushButton(background);
                menu->setObjectName(QString::fromLatin1(menuName));
                menu->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                menu->setProperty("class", QStringLiteral("CardHeaderMenu"));

                auto* backgroundLayout = new QHBoxLayout(background);
                backgroundLayout->setSizeConstraint(QLayout::SetMinimumSize);
                backgroundLayout->setSpacing(0);                                    // CardHeader.cpp:152
                backgroundLayout->setContentsMargins(0, 0, 0, 0);
                backgroundLayout->addWidget(expander);
                backgroundLayout->addWidget(icon);
                backgroundLayout->addWidget(title);
                backgroundLayout->addStretch(1);
                backgroundLayout->addWidget(warning);
                backgroundLayout->addWidget(help);
                backgroundLayout->addWidget(menu);
                background->setLayout(backgroundLayout);

                auto* mainLayout = new QVBoxLayout(header);
                mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
                mainLayout->setSpacing(0);
                mainLayout->setContentsMargins(0, 0, 0, 0);
                mainLayout->addWidget(background);
                outer->addWidget(header);
                outer->addStretch(1);
            }

            //! A real card header with its parts shown: an icon, a title, a help button and a context menu button, and a warning
            //! (the warning also tiles the header with an image, which each style draws its own way: no pixels are compared then)
            static void buildRealHeader(QWidget& root, bool warning = false)
            {
                auto* column = new QVBoxLayout(&root);
                column->setContentsMargins(0, 0, 0, 0);
                auto* header = new CardHeader(&root);
                header->setObjectName(QStringLiteral("realHeader"));
                header->setIcon(QIcon(marker(QSize(16, 16), QColor(0x40, 0xA0, 0x60))));
                header->setTitle(QStringLiteral("Camera"));
                header->setWarningIcon(QIcon(marker(QSize(16, 14), QColor(0xF0, 0xC3, 0x2D))));
                header->setWarning(warning);
                header->setHelpURL(QStringLiteral("https://o3de.org"));
                header->setHasContextMenu(true);
                column->addWidget(header);
                column->addStretch(1);
            }

            //! A BrowseEdit as the flattened BrowseEdit builds it (BrowseEdit.cpp:41-87): a frame with a line edit and the attached
            //! button, flat and fixed at 16x16, side by side in a layout without margins or spacing. The line edit is fixed at the
            //! 16 its rule gives it (BrowseEdit.qss:17), so that the fonts of a test machine do not decide the height of the frame.
            static void buildBrowseEdit(QWidget& root)
            {
                auto* column = new QVBoxLayout(&root);
                column->setContentsMargins(0, 0, 0, 0);
                auto* browseEdit = new StandIn(BrowseEditClass, "browse-edit", &root);
                column->addWidget(browseEdit);
                column->addStretch(1);

                auto* layout = new QHBoxLayout(browseEdit);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(0);
                auto* lineEdit = new QLineEdit(browseEdit);
                lineEdit->setObjectName(QStringLiteral("line-edit"));
                lineEdit->setFixedHeight(16);
                layout->addWidget(lineEdit);
                auto* button = new QPushButton(browseEdit);
                button->setObjectName(QStringLiteral("attached-button"));
                button->setFlat(true);                                              // BrowseEdit.cpp:77-79, the flattened mode
                button->setFixedSize(16, 16);
                button->setIconSize(QSize(12, 12));
                button->setIcon(QIcon(marker(QSize(12, 12), QColor(0x40, 0x60, 0xA0))));
                layout->addWidget(button);
            }

        protected:
            int m_argc = 1;                                     // QApplication keeps a reference to it
            char m_programName[5] = "test";
            char* m_argv[2] = { m_programName, nullptr };
            AZStd::unique_ptr<QApplication> m_application;
            AZStd::unique_ptr<Style> m_style;
        };

        // ----------------------------------------------------------------------------------
        // T1: the labels of a property row: ReflectedPropertyEditor.qss
        // ----------------------------------------------------------------------------------

        TEST_F(PropertyEditorElementTests, PropertyRow_NameAndDefaultLabelHaveTheBoxOfTheRule)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());   // the test environment sets it (AzQtComponentTests.cpp)

            Twins twins(*this, [](QWidget& root) { buildRow(root, "Max Time Step (sec)", true); }, QSize(600, 60), PropertyRowSelector);
            const auto* legacyName = twins.legacyChild<ElidingLabel>("Name");
            const auto* flatName = twins.flatChild<ElidingLabel>("Name");
            const auto* legacyDefault = twins.legacyChild<QLabel>("DefaultLabel");

            // The Native numbers (n2, Gallery 17_reflected-property-editor and Editor PhysX Configuration): the name label has the
            // frame box [4, 0, 0, 0] (so a frame width of 4) and is 16 high in the layout whatever its hint is (17); the default
            // label [0, 1, 0, 1], 18 high with the margins. The left column is 16 high (the indicator's height); the row 18 high
            // with the name alone, 20 with the default label (Native: the Gallery component row and its container row).
            EXPECT_TRUE(legacyName->contentsMargins() == QMargins(4, 0, 0, 0));
            EXPECT_EQ(legacyName->frameWidth(), 4);
            EXPECT_EQ(legacyName->minimumHeight(), 16);
            EXPECT_EQ(legacyName->maximumHeight(), 16);
            EXPECT_TRUE(legacyDefault->contentsMargins() == QMargins(0, 1, 0, 1));
            EXPECT_EQ(legacyDefault->minimumHeight(), 18);
            EXPECT_EQ(legacyDefault->maximumHeight(), 18);
            EXPECT_EQ(twins.legacyChild("left")->sizeHint().height(), 16);
            EXPECT_EQ(twins.legacyChild("propertyRow")->height(), 20);

            expectSameBox(twins, { "Name", "DefaultLabel", "Indicator", "left", "middle", "propertyRow" });

            // The hidden label ElidingLabel measures its size hint with is named alike, and the rule reached it too: the left
            // margin is counted by both (Native: a hint 8 wider than the text and the frame of a plain label)
            const auto* legacyMetrics = legacyName->findChild<QLabel*>(QStringLiteral("Name"));
            const auto* flatMetrics = flatName->findChild<QLabel*>(QStringLiteral("Name"));
            ASSERT_TRUE(legacyMetrics != nullptr && flatMetrics != nullptr);
            EXPECT_TRUE(legacyMetrics->contentsMargins() == QMargins(4, 0, 0, 0));
            EXPECT_TRUE(flatMetrics->contentsMargins() == legacyMetrics->contentsMargins());
            EXPECT_EQ(flatMetrics->minimumSize(), legacyMetrics->minimumSize());
            EXPECT_EQ(flatMetrics->maximumSize(), legacyMetrics->maximumSize());
            EXPECT_EQ(flatName->sizeHint(), legacyName->sizeHint());

            // What the style draws of the row's frames and labels: nothing, as the sheet drew nothing (fills transparent, no border)
            expectSameFrames(twins, { "Name", "DefaultLabel", "propertyRow" });
        }

        TEST_F(PropertyEditorElementTests, PropertyRow_ARowWithoutADefaultLabelOrWithALongNameHasTheBoxToo)
        {
            // a row in a narrow column: the label is elided, and the box is the same
            Twins twins(*this, [](QWidget& root) { buildRow(root, "A property with a name that is far too long for its column", false); },
                QSize(300, 60), PropertyRowSelector);
            expectSameBox(twins, { "Name", "left", "middle", "propertyRow" });
            EXPECT_EQ(twins.legacyChild("propertyRow")->height(), 18);
            EXPECT_EQ(twins.flatChild<ElidingLabel>("Name")->minimumHeight(), 16);
            EXPECT_EQ(twins.flatChild<ElidingLabel>("Name")->maximumHeight(), 16);

            // The text is elided to the same width: ElidingLabel::TextRect reads the contents rect of the box
            const QString legacyText = twins.legacyChild<ElidingLabel>("Name")->elidedText();
            EXPECT_NE(legacyText, twins.legacyChild<ElidingLabel>("Name")->text());
            EXPECT_EQ(twins.flatChild<ElidingLabel>("Name")->elidedText(), legacyText);
        }

        // ----------------------------------------------------------------------------------
        // T1b / T1c: the override label of a DPE row: PrefabOverrideLabelHandler's composite, whose label its inline sheet makes bold
        // when overridden, and the compact label (IconElidingLabel) that stands in for it
        // ----------------------------------------------------------------------------------

        //! The override label of a DPE row both ways, each alone in a root of the same size: the composite of
        //! PrefabOverrideLabelHandler (a container whose layout has no margins and no spacing, a tool button with the 6x6 icon, an
        //! ElidingLabel) and the IconElidingLabel of CompactPrefabOverrideLabelHandler. The legacy twins carry the handlers' inline
        //! sheet on the label and the application's sheet on the root; the flat twins have the style on everything and no sheet.
        struct OverrideLabelTwins
        {
            OverrideLabelTwins(const PropertyEditorElementTests& tests, bool flatMode, const QSize& size)
            {
                auto* compositeColumn = new QHBoxLayout(&m_compositeRoot);
                compositeColumn->setContentsMargins(0, 0, 0, 0);
                m_container = new QWidget(&m_compositeRoot);
                compositeColumn->addWidget(m_container);
                m_button = new QToolButton();
                m_label = new ElidingLabel();
                auto* containerLayout = new QHBoxLayout(m_container);
                containerLayout->setContentsMargins(0, 0, 0, 0);
                containerLayout->setSpacing(0);
                containerLayout->addWidget(m_button);
                containerLayout->addWidget(m_label);

                auto* compactColumn = new QHBoxLayout(&m_compactRoot);
                compactColumn->setContentsMargins(0, 0, 0, 0);
                m_compact = new IconElidingLabel(&m_compactRoot);
                compactColumn->addWidget(m_compact);

                for (ElidingLabel* label : { m_label, static_cast<ElidingLabel*>(m_compact) })
                {
                    label->setProperty("overridden", false);
                    if (!flatMode)
                    {
                        label->setStyleSheet(QStringLiteral("[overridden=\"true\"] { font-weight: bold }"));
                    }
                }
                set(QStringLiteral("Width"), false);

                for (QWidget* root : { &m_compositeRoot, &m_compactRoot })
                {
                    if (flatMode)
                    {
                        tests.flat(*root);
                    }
                    else
                    {
                        PropertyEditorElementTests::legacy(*root, nullptr, nullptr);
                    }
                    PropertyEditorElementTests::show(*root, size);
                }
            }

            //! What RefreshUI does to both: the text, the override state (the property set, the label repolished) and the icon
            void set(const QString& text, bool overridden)
            {
                const QIcon icon = overridden ? QIcon(marker(QSize(6, 6), QColor(0xE0, 0x80, 0x20))) : QIcon();
                for (ElidingLabel* label : { m_label, static_cast<ElidingLabel*>(m_compact) })
                {
                    label->setText(text);
                    if (StyleManager::setObjectProperty(label, "overridden", overridden))
                    {
                        label->RefreshStyle();
                    }
                }
                m_button->setIcon(icon);
                m_button->setIconSize(QSize(6, 6));
                m_compact->setIcon(icon, QSize(6, 6));
                QApplication::processEvents();
            }

            //! A root as its window paints it (QWidget::render: the background, then the children), on grey at a device ratio
            static QImage render(QWidget& root, qreal ratio)
            {
                QImage image((QSizeF(root.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
                image.setDevicePixelRatio(ratio);
                image.fill(QColor(0x80, 0x80, 0x80));
                root.render(&image);
                return image;
            }

            //! The same pixels at both ratios, the hints the row's layout reads, and the same elided text and tooltip
            void expectSame(const std::string& what)
            {
                for (const qreal ratio : { 1.0, 1.5 })
                {
                    EXPECT_EQ(PropertyEditorElementTests::differing(render(m_compositeRoot, ratio), render(m_compactRoot, ratio)), 0)
                        << what << " at ratio " << ratio;
                }
                EXPECT_EQ(m_compact->sizeHint(), m_container->sizeHint()) << what;
                EXPECT_EQ(m_compact->minimumSizeHint(), m_container->minimumSizeHint()) << what;
                EXPECT_EQ(m_compact->elidedText(), m_label->elidedText()) << what;
                EXPECT_EQ(m_compact->toolTip(), m_label->toolTip()) << what;
            }

            QWidget m_compositeRoot;
            QWidget m_compactRoot;
            QWidget* m_container = nullptr;
            QToolButton* m_button = nullptr;
            ElidingLabel* m_label = nullptr;
            IconElidingLabel* m_compact = nullptr;
        };

        TEST_F(PropertyEditorElementTests, PrefabOverride_AnOverriddenLabelIsBoldAsTheHandlersSheetMadeItAndPlainAgainAfter)
        {
            // The composite's label as RefreshUI leaves it: with style sheets its inline sheet gives the weight, flattened the element
            // for an overridden label does (and the label carries no sheet)
            OverrideLabelTwins legacy(*this, false, QSize(300, 40));
            OverrideLabelTwins flat(*this, true, QSize(300, 40));
            for (const bool overridden : { false, true, false, true })
            {
                legacy.set(QStringLiteral("Width"), overridden);
                flat.set(QStringLiteral("Width"), overridden);

                const QFont legacyFont = legacy.m_label->font();
                const QFont flatFont = flat.m_label->font();
                EXPECT_EQ(legacyFont.weight(), overridden ? QFont::Bold : QFont::Normal);
                EXPECT_EQ(flatFont.weight(), legacyFont.weight()) << "overridden " << overridden;
                EXPECT_EQ(flatFont.pixelSize(), legacyFont.pixelSize()) << "overridden " << overridden;
                EXPECT_EQ(flatFont.family(), legacyFont.family()) << "overridden " << overridden;
                EXPECT_EQ(flat.m_label->sizeHint(), legacy.m_label->sizeHint()) << "overridden " << overridden;
            }
            EXPECT_TRUE(flat.m_label->styleSheet().isEmpty());
        }

        TEST_F(PropertyEditorElementTests, CompactOverrideLabel_LooksAndSizesAsTheCompositeWithAndWithoutTheOverride)
        {
            // a row as tall as its spin boxes, one as tall as the label, and a column too narrow for the text
            for (const bool flatMode : { false, true })
            {
                for (const QSize& size : { QSize(220, 24), QSize(220, 16), QSize(44, 24) })
                {
                    OverrideLabelTwins twins(*this, flatMode, size);
                    const std::string mode = std::string(flatMode ? "flat " : "legacy ") + std::to_string(size.width()) + "x" +
                        std::to_string(size.height());
                    for (const bool overridden : { false, true, false })
                    {
                        twins.set(QStringLiteral("Width"), overridden);
                        twins.expectSame(mode + (overridden ? " overridden" : " plain"));
                    }
                    twins.set(QStringLiteral("A property with a name that is far too long for its column"), true);
                    twins.expectSame(mode + " long name");
                }
            }
        }

        TEST_F(PropertyEditorElementTests, CompactOverrideLabel_IsFilteredAndDisabledAsTheComposite)
        {
            for (const bool flatMode : { false, true })
            {
                OverrideLabelTwins twins(*this, flatMode, QSize(220, 24));
                const std::string mode = flatMode ? "flat" : "legacy";
                twins.set(QStringLiteral("Width"), true);

                // the filter's highlight is drawn in ElidingLabel::TextRect, which starts after the button
                twins.m_label->setFilter(QStringLiteral("dt"));
                twins.m_compact->setFilter(QStringLiteral("dt"));
                twins.expectSame(mode + " filtered");
                twins.m_label->setFilter(QString());
                twins.m_compact->setFilter(QString());

                // a disabled row: the button's icon and the text as the style draws them disabled
                twins.m_compositeRoot.setEnabled(false);
                twins.m_compactRoot.setEnabled(false);
                QApplication::processEvents();
                twins.expectSame(mode + " disabled");
            }
        }

        TEST_F(PropertyEditorElementTests, CompactOverrideLabel_TheFlatButtonUnderTheMouseOrPressedIsDrawnAsIdle)
        {
            // The compact label draws the button idle only. The flattened style draws a tool button without a panel and without a
            // shift (ToolButton::drawToolButton, Fusion's PM_ButtonShift* of 0), so under the mouse and pressed it looked the same.
            OverrideLabelTwins twins(*this, true, QSize(220, 24));
            twins.set(QStringLiteral("Width"), true);
            twins.m_button->setAttribute(Qt::WA_UnderMouse, true);
            twins.expectSame("under the mouse");
            twins.m_button->setDown(true);
            twins.expectSame("pressed");
        }

        TEST_F(PropertyEditorElementTests, CompactOverrideLabel_IsOneWidgetWithoutATabStopAndItsWindowHasOneButton)
        {
            OverrideLabelTwins twins(*this, true, QSize(220, 48));
            twins.set(QStringLiteral("Width"), true);

            // the composite: the container, the tool button (a Tab stop, which the DPE's Tab skips), the label and its hidden
            // metrics label; the compact label: itself and its metrics label, and the window's hidden button
            EXPECT_EQ(twins.m_compositeRoot.findChildren<QWidget*>().size(), 4);
            EXPECT_TRUE(twins.m_button->focusPolicy() & Qt::TabFocus);
            EXPECT_EQ(twins.m_compactRoot.findChildren<QWidget*>().size(), 3);
            EXPECT_EQ(twins.m_compact->focusPolicy(), Qt::NoFocus);
            const QList<QToolButton*> buttons = twins.m_compactRoot.findChildren<QToolButton*>();
            ASSERT_EQ(buttons.size(), 1);
            EXPECT_FALSE(buttons.front()->isVisible());
            EXPECT_FALSE(buttons.front()->isVisibleTo(&twins.m_compactRoot));

            // A second label, made without a parent and placed after (as the DPE places a handler's widget): it shares the
            // window's button, and has the first one's size once it is in the window
            auto* second = new IconElidingLabel();
            second->setIcon(QIcon(marker(QSize(6, 6), QColor(0xE0, 0x80, 0x20))), QSize(6, 6));
            second->setText(QStringLiteral("Width"));
            second->setProperty("overridden", true);
            twins.m_compactRoot.layout()->addWidget(second);
            flat(twins.m_compactRoot);                          // the style on the label and its metrics label, as on the others
            second->show();
            QApplication::processEvents();
            EXPECT_EQ(twins.m_compactRoot.findChildren<QToolButton*>().size(), 1);
            EXPECT_EQ(second->sizeHint(), twins.m_compact->sizeHint());
            EXPECT_EQ(second->indent(), twins.m_compact->indent());
            EXPECT_GT(second->indent(), 0);
        }

        // ----------------------------------------------------------------------------------
        // T2: the parts of a card header: Card.qss
        // ----------------------------------------------------------------------------------

        TEST_F(PropertyEditorElementTests, CardHeader_PartsHaveTheBoxesOfTheRulesAndTheLayoutNoSpacing)
        {
            Twins twins(*this, [](QWidget& root) { buildHeader(root, "ContextMenu"); }, QSize(420, 80), CardHeaderSelector);

            // The Native numbers (n2, Editor 02_main_entity_selected ComponentEditorHeader, Gallery 04_card): the expander 20x16
            // (16 and 4 of margin) at both limits; the icon [0, 1, 4, 1], hint 20x18; the warning icon [4, 1, 4, 1]; the help
            // button limited to 22x18 (16x16, its margin and the native border Qt counted before qproperty-flat) with the hint
            // 20x16; the context menu button to 30x20 (28x18 and the border), hint 22x18
            const auto* expander = twins.legacyChild<QCheckBox>("Expander");
            EXPECT_EQ(expander->minimumSize(), QSize(20, 16));
            EXPECT_EQ(expander->maximumSize(), QSize(20, 16));
            EXPECT_TRUE(twins.legacyChild<QLabel>("Icon")->contentsMargins() == QMargins(0, 1, 4, 1));
            EXPECT_EQ(twins.legacyChild<QLabel>("Icon")->sizeHint(), QSize(20, 18));
            EXPECT_TRUE(twins.legacyChild<QLabel>("WarningIcon")->contentsMargins() == QMargins(4, 1, 4, 1));
            EXPECT_EQ(twins.legacyChild<QPushButton>("Help")->maximumSize(), QSize(22, 18));
            EXPECT_EQ(twins.legacyChild<QPushButton>("ContextMenu")->maximumSize(), QSize(30, 20));
            if (!twins.legacyChild<QPushButton>("Help")->icon().isNull())      // the hints are the icons' (the svg engine is a plugin)
            {
                EXPECT_EQ(twins.legacyChild<QPushButton>("Help")->sizeHint(), QSize(20, 16));
                EXPECT_EQ(twins.legacyChild<QPushButton>("ContextMenu")->sizeHint(), QSize(22, 18));
            }

            // Every part, the frames around them and the header: the same boxes, the same places
            expectSameBox(twins, { "Icon", "Title", "WarningIcon", "Help", "ContextMenu", "Background", "cardHeader" });
            expectSameBox(twins, { "Expander" }, false);
            EXPECT_TRUE(twins.flatChild<QPushButton>("Help")->isFlat());
            EXPECT_TRUE(twins.flatChild<QPushButton>("ContextMenu")->isFlat());
            EXPECT_EQ(twins.flatChild<QPushButton>("Help")->iconSize(), twins.legacyChild<QPushButton>("Help")->iconSize());
            EXPECT_EQ(twins.flatChild<QPushButton>("ContextMenu")->iconSize(), twins.legacyChild<QPushButton>("ContextMenu")->iconSize());
            EXPECT_EQ(twins.flatChild<QPushButton>("Help")->icon().isNull(), twins.legacyChild<QPushButton>("Help")->icon().isNull());
            EXPECT_EQ(twins.flatChild<QPushButton>("ContextMenu")->icon().isNull(), twins.legacyChild<QPushButton>("ContextMenu")->icon().isNull());

            // The 4px of the expander's margin are at the right of its caret; what the style draws of the frames and labels of the
            // header: nothing, as the sheet drew nothing
            expectContentsAreTheCaret(twins.legacyChild<QCheckBox>("Expander"), twins.flatChild<QCheckBox>("Expander"));
            expectSameFrames(twins, { "Icon", "Title", "WarningIcon", "Background", "cardHeader" });
        }

        TEST_F(PropertyEditorElementTests, CardHeader_AButtonIsDrawnAsTheSheetDrewItInEveryState)
        {
            Twins twins(*this, [](QWidget& root) { buildHeader(root, "ContextMenu"); }, QSize(420, 80), CardHeaderSelector);
            const QStyle::State enabled = QStyle::State_Enabled;
            const QStyle::State states[] = { enabled, enabled | QStyle::State_MouseOver, enabled | QStyle::State_Sunken | QStyle::State_MouseOver,
                enabled | QStyle::State_HasFocus, QStyle::State_None };
            for (const char* name : { "Help", "ContextMenu" })
            {
                const auto* legacyButton = twins.legacyChild<QPushButton>(name);
                const auto* flatButton = twins.flatChild<QPushButton>(name);
                const QRect rect(QPoint(0, 0), legacyButton->size());
                for (const qreal ratio : { 1.0, 1.5 })
                {
                    for (const QStyle::State state : states)
                    {
                        EXPECT_EQ(differing(draw(legacyButton, state, rect, ratio), draw(flatButton, state, rect, ratio)), 0)
                            << name << " in state " << int(state) << " at ratio " << ratio;
                    }
                }
            }
        }

        TEST_F(PropertyEditorElementTests, CardHeader_AnIconLabelWithAFrameShapeHasNoBorderLikeTheRule)
        {
            // Card.qss:220 `border: none`: the label's frame is not the base style's, whatever its shape (a label of a header has none)
            Twins twins(*this, [](QWidget& root) { buildHeader(root, "ContextMenu", true); }, QSize(420, 80), CardHeaderSelector);
            EXPECT_TRUE(twins.legacyChild<QLabel>("Icon")->contentsMargins() == QMargins(0, 1, 4, 1));
            expectSameBox(twins, { "Icon", "Title", "Help", "ContextMenu" });

            // and none is drawn: the base style's frame of a StyledPanel label would show as lines on the grey
            expectSameFrames(twins, { "Icon" });
        }

        TEST_F(PropertyEditorElementTests, CardHeader_AButtonThatIsNotFlatIsBoxedAsTheRuleBoxedIt)
        {
            // qproperty-flat comes from the rule itself; when something turns it off, the margin of 0 still gives the rule a box
            // (QRenderRule::hasBox), and the native border is counted
            Twins twins(*this, [](QWidget& root) { buildHeader(root, "ContextMenu"); }, QSize(420, 80), CardHeaderSelector);
            for (const char* name : { "Help", "ContextMenu" })
            {
                twins.legacyChild<QPushButton>(name)->setFlat(false);
                twins.flatChild<QPushButton>(name)->setFlat(false);
                EXPECT_EQ(twins.flatChild<QPushButton>(name)->sizeHint(), twins.legacyChild<QPushButton>(name)->sizeHint()) << name;
            }
        }

        TEST_F(PropertyEditorElementTests, CardHeader_ThePlusButtonHasItsOwnIconAndLimits)
        {
            Twins twins(*this, [](QWidget& root) { buildHeader(root, "ContextMenuPlusIcon"); }, QSize(420, 80), CardHeaderSelector);

            // Card.qss:239-247: 18x18 at most, 20x20 with the native border; UNVERIFIED by capture, the sheet is the proof
            EXPECT_EQ(twins.legacyChild<QPushButton>("ContextMenuPlusIcon")->maximumSize(), QSize(20, 20));
            expectSameBox(twins, { "ContextMenuPlusIcon", "Help", "Title" });
            EXPECT_EQ(twins.flatChild<QPushButton>("ContextMenuPlusIcon")->icon().isNull(),
                twins.legacyChild<QPushButton>("ContextMenuPlusIcon")->icon().isNull());

            // CardHeader::setContextMenuIcon renames a button after its first polish: the element follows the name
            Twins renamed(*this, [](QWidget& root) { buildHeader(root, "ContextMenu"); }, QSize(420, 80), CardHeaderSelector);
            QPushButton* button = renamed.flatChild<QPushButton>("ContextMenu");
            button->setObjectName(QStringLiteral("ContextMenuPlusIcon"));
            button->style()->unpolish(button);
            button->style()->polish(button);
            EXPECT_EQ(button->maximumSize(), QSize(20, 20));
            EXPECT_EQ(button->iconSize(), twins.flatChild<QPushButton>("ContextMenuPlusIcon")->iconSize());
        }

        TEST_F(PropertyEditorElementTests, CardHeader_TheLimitsAreTheElementsWhateverTheComponentSetBefore)
        {
            // PushButton::polish gives every push button a maximum height (the default frame's, 24 in PushButtonConfig.ini) before the
            // element is applied (Native: the sheet's geometry came after it, qstylesheetstyle.cpp:2875-2890): it must be gone
            Twins twins(*this, [](QWidget& root) { buildHeader(root, "ContextMenu"); }, QSize(420, 80), CardHeaderSelector);
            EXPECT_EQ(twins.flatChild<QPushButton>("Help")->maximumHeight(), 18);
            EXPECT_EQ(twins.flatChild<QPushButton>("ContextMenu")->maximumHeight(), 20);
            QPushButton plain;
            plain.setStyle(m_style.get());
            plain.ensurePolished();
            EXPECT_GT(plain.maximumHeight(), 20);               // a button no element covers keeps it
        }

        // ----------------------------------------------------------------------------------
        // T3: a real card header, through the polish of the components (Card::polish, CheckBox::polish, PushButton::polish)
        // ----------------------------------------------------------------------------------

        TEST_F(PropertyEditorElementTests, RealCardHeader_HasTheBoxesOfTheRulesAndNoSpacingInItsLayout)
        {
            Twins twins(*this, [](QWidget& root) { buildRealHeader(root); }, QSize(420, 80), nullptr);

            // the header's own parts, the frame around them (a box of Card::shapedFrameContents here, of the sheet there) and the header
            expectSameBox(twins, { "Icon", "Title", "Help", "ContextMenu", "Background", "realHeader" });
            expectSameBox(twins, { "Expander" }, false);
            EXPECT_EQ(twins.flatChild<QFrame>("Background")->layout()->spacing(), 0);
            EXPECT_EQ(twins.legacyChild<QFrame>("Background")->layout()->spacing(), 0);

            // the Native numbers (n2, Editor 02_main_entity_selected ComponentEditorHeader): the expander 20x16, the icon label 20 wide,
            // the buttons 20 and 22 wide, the header's minimum width 98 with the title of that capture
            EXPECT_EQ(twins.flatChild<QCheckBox>("Expander")->size(), QSize(20, 16));
            EXPECT_EQ(twins.flatChild<QLabel>("Icon")->width(), 20);
            EXPECT_EQ(twins.flatChild<QPushButton>("Help")->icon().isNull(), twins.legacyChild<QPushButton>("Help")->icon().isNull());
            if (!twins.flatChild<QPushButton>("Help")->icon().isNull())        // the widths are the icons' (the svg engine is a plugin)
            {
                EXPECT_EQ(twins.flatChild<QPushButton>("Help")->width(), 20);
                EXPECT_EQ(twins.flatChild<QPushButton>("ContextMenu")->width(), 22);
            }

            expectContentsAreTheCaret(twins.legacyChild<QCheckBox>("Expander"), twins.flatChild<QCheckBox>("Expander"));
            expectSameFrames(twins, { "Icon", "Title", "Background" });
        }

        TEST_F(PropertyEditorElementTests, RealCardHeader_AWarningIconHasItsBoxToo)
        {
            Twins twins(*this, [](QWidget& root) { buildRealHeader(root, true); }, QSize(420, 80), nullptr);
            expectSameBox(twins, { "WarningIcon", "Icon", "Title", "Help", "ContextMenu", "Background", "realHeader" });
            expectSameBox(twins, { "Expander" }, false);
            EXPECT_TRUE(twins.flatChild<QLabel>("WarningIcon")->contentsMargins() == QMargins(4, 1, 4, 1));
        }

        TEST_F(PropertyEditorElementTests, CardHeader_AnIconInAColorPickerHasTheMarginsOfBothRules)
        {
            // ColorPicker.qss:45-49 `AzQtComponents--ColorPicker QLabel { margin: 0px }` is more specific than Text.qss's `QLabel` margins of
            // 1px above and below, and Card.qss's ids are more specific than both: the icon [0, 0, 4, 0], the warning icon [4, 0, 4, 0].
            // UNVERIFIED by capture (the header of a palette card shows no icon); the sheet twin is the proof. The title is no part of
            // this: its box in a picker is the label elements' (the same rule gives it margin 0).
            Twins twins(*this, [](QWidget& root)
            {
                auto* column = new QVBoxLayout(&root);
                column->setContentsMargins(0, 0, 0, 0);
                auto* picker = new StandIn(ColorPickerClass, "colorPicker", &root);
                column->addWidget(picker);
                column->addStretch(1);
                buildHeader(*picker, "ContextMenu", true);
            }, QSize(420, 80), CardHeaderSelector, ColorPickerSelector);

            EXPECT_TRUE(twins.legacyChild<QLabel>("Icon")->contentsMargins() == QMargins(0, 0, 4, 0));
            EXPECT_TRUE(twins.legacyChild<QLabel>("WarningIcon")->contentsMargins() == QMargins(4, 0, 4, 0));
            for (const char* name : { "Icon", "WarningIcon" })
            {
                const QWidget* legacyWidget = twins.legacyChild<QWidget>(name);
                const QWidget* flatWidget = twins.flatChild<QWidget>(name);
                EXPECT_TRUE(flatWidget->contentsMargins() == legacyWidget->contentsMargins()) << name;
                EXPECT_EQ(flatWidget->sizeHint(), legacyWidget->sizeHint()) << name;
                EXPECT_EQ(flatWidget->width(), legacyWidget->width()) << name;
            }
            expectSameFrames(twins, { "Icon", "WarningIcon" });

            // the other parts keep the boxes of the picker-less header
            EXPECT_EQ(twins.flatChild<QCheckBox>("Expander")->minimumSize(), twins.legacyChild<QCheckBox>("Expander")->minimumSize());
            EXPECT_EQ(twins.flatChild<QPushButton>("Help")->maximumSize(), twins.legacyChild<QPushButton>("Help")->maximumSize());
            EXPECT_EQ(twins.flatChild<QPushButton>("ContextMenu")->maximumSize(), twins.legacyChild<QPushButton>("ContextMenu")->maximumSize());
        }

        TEST_F(PropertyEditorElementTests, RealCard_TheBoxesAroundTheHeaderAreNativesToo)
        {
            // Card.qss:10 `padding: -4px -5px -4px -5px` and the border image give the card the frame box (2, 1, 2, 9), which the
            // flattened card has as Card::shapedFrameContents: the card, its header and its body are boxed as Native boxes them once
            // the parts of the header are (a ComponentEditor is not too high because of the card's padding)
            Twins twins(*this, [](QWidget& root)
            {
                auto* column = new QVBoxLayout(&root);
                column->setContentsMargins(0, 0, 0, 0);
                auto* card = new Card(&root);
                card->setObjectName(QStringLiteral("realCard"));
                card->setTitle(QStringLiteral("Camera"));
                card->header()->setObjectName(QStringLiteral("cardsHeader"));
                card->header()->setIcon(QIcon(marker(QSize(16, 16), QColor(0x40, 0xA0, 0x60))));
                card->header()->setHelpURL(QStringLiteral("https://o3de.org"));
                auto* content = new QLabel(QStringLiteral("content"));
                content->setObjectName(QStringLiteral("cardContent"));
                card->setContentWidget(content);
                card->layout()->setSpacing(0);          // Card::polish does (Card.cpp:394), which the legacy twin over Fusion does not run
                column->addWidget(card);
                column->addStretch(1);
            }, QSize(420, 200), nullptr);
            EXPECT_TRUE(twins.legacyChild("realCard")->contentsMargins() == QMargins(2, 1, 2, 9));
            expectSameBox(twins, { "realCard", "cardsHeader", "contentContainer", "cardContent", "Icon", "Help", "ContextMenu", "Background" });
            expectSameBox(twins, { "Expander" }, false);
        }

        TEST_F(PropertyEditorElementTests, RealCardHeader_ThePlusIconIsTheElementsToo)
        {
            Twins twins(*this, [](QWidget& root)
            {
                buildRealHeader(root);
                root.findChild<CardHeader*>()->setContextMenuIcon(CardHeader::ContextMenuIcon::Plus);
            }, QSize(420, 80), nullptr);
            expectSameBox(twins, { "ContextMenuPlusIcon", "Help", "Title", "Background" });
            EXPECT_EQ(twins.flatChild<QPushButton>("ContextMenuPlusIcon")->maximumSize(), QSize(20, 20));
        }

        // ----------------------------------------------------------------------------------
        // T4: the button of a BrowseEdit keeps the 16x16 its rule gave it (PushButton::polish)
        // ----------------------------------------------------------------------------------

        TEST_F(PropertyEditorElementTests, BrowseEdit_TheAttachedButtonKeepsItsSizeSoTheFrameIsTheEditsHeightPlusItsMargin)
        {
            Twins twins(*this, [](QWidget& root) { buildBrowseEdit(root); }, QSize(300, 60), BrowseEditSelector);

            // The Native numbers (n2, Gallery 17_reflected-property-editor AssetData): the button 16x16 at both limits (the rule's
            // min and max, BrowseEdit.qss:55-58; no border of the style's own counted: it declares one), the frame 20 high: the edit's
            // 16 and the margin of 2 on each side (BrowseEdit.qss:12). PushButton::polish gives every push button a maximum height
            // before this, which raised the flat button to 24 and the frame to 24 + 4.
            EXPECT_EQ(twins.legacyChild<QPushButton>("attached-button")->minimumSize(), QSize(16, 16));
            EXPECT_EQ(twins.legacyChild<QPushButton>("attached-button")->maximumSize(), QSize(16, 16));
            EXPECT_EQ(twins.legacyChild("browse-edit")->height(), 20);
            expectSameBox(twins, { "attached-button", "line-edit", "browse-edit" }, false);
            EXPECT_EQ(twins.flatChild<QPushButton>("attached-button")->maximumSize(), QSize(16, 16));
            EXPECT_EQ(twins.flatChild("browse-edit")->height(), 20);
        }

        TEST_F(PropertyEditorElementTests, BrowseEdit_OnlyTheButtonOfABrowseEditIsLeftAtItsSize)
        {
            // a button called attached-button elsewhere, and another button of a BrowseEdit, take the maximum height every push button gets
            QWidget root;
            root.setStyle(m_style.get());
            auto* browseEdit = new StandIn(BrowseEditClass, "browse-edit", &root);
            auto* other = new StandIn("Other", "other", &root);
            const auto polished = [this](QWidget* parent, const char* name)
            {
                auto* button = new QPushButton(parent);
                button->setObjectName(QString::fromLatin1(name));
                button->setFixedSize(16, 16);
                button->setStyle(m_style.get());
                button->ensurePolished();
                return button;
            };
            EXPECT_EQ(polished(browseEdit, "attached-button")->maximumHeight(), 16);
            EXPECT_GT(polished(other, "attached-button")->maximumHeight(), 16);
            EXPECT_GT(polished(browseEdit, "another")->maximumHeight(), 16);
        }

        // ----------------------------------------------------------------------------------
        // T5: what the elements do not cover
        // ----------------------------------------------------------------------------------

        TEST_F(PropertyEditorElementTests, Registry_AWidgetOutsideTheSelectorsOfTheRulesHasNoElement)
        {
            QWidget root;
            root.setProperty("propertyEditorTestsFlat", true);
            root.setStyle(m_style.get());
            auto* row = new StandIn(PropertyRowClass, "propertyRow", &root);
            auto* header = new StandIn(CardHeaderClass, "cardHeader", &root);
            auto* picker = new StandIn(ColorPickerClass, "colorPicker", &root);
            auto* pickerHeader = new StandIn(CardHeaderClass, "cardHeader", picker);

            const auto polished = [this](QWidget* widget)
            {
                widget->setStyle(m_style.get());
                widget->ensurePolished();
                return widget;
            };
            const auto elementOf = [&](QWidget* widget) -> QString
            {
                const BoxElement* element = BoxElement::forWidget(polished(widget));
                return element ? QString::fromLatin1(element->m_name) : QString();
            };
            const auto label = [](QWidget* parent, const char* name)
            {
                auto* widget = new QLabel(parent);
                widget->setObjectName(QString::fromLatin1(name));
                return widget;
            };
            const auto button = [](QWidget* parent, const char* name)
            {
                auto* widget = new QPushButton(parent);
                widget->setObjectName(QString::fromLatin1(name));
                return widget;
            };

            // the selectors of the rules: a name inside the class
            EXPECT_EQ(elementOf(label(row, "Name")), QStringLiteral("Global.Box.PropertyRowName"));
            EXPECT_EQ(elementOf(label(row, "DefaultLabel")), QStringLiteral("Global.Box.PropertyRowDefaultLabel"));
            EXPECT_EQ(elementOf(label(header, "Icon")), QStringLiteral("Global.Box.CardHeaderIcon"));
            EXPECT_EQ(elementOf(label(header, "WarningIcon")), QStringLiteral("Global.Box.CardHeaderWarningIcon"));
            EXPECT_EQ(elementOf(button(header, "Help")), QStringLiteral("Global.Box.CardHeaderHelp"));
            EXPECT_EQ(elementOf(button(header, "ContextMenu")), QStringLiteral("Global.Box.CardHeaderContextMenu"));
            EXPECT_EQ(elementOf(button(header, "ContextMenuPlusIcon")), QStringLiteral("Global.Box.CardHeaderContextMenuPlus"));

            // the same names elsewhere, and the other classes' names inside
            EXPECT_EQ(elementOf(label(&root, "Name")), QString());
            EXPECT_EQ(elementOf(label(&root, "DefaultLabel")), QString());
            EXPECT_EQ(elementOf(label(row, "Icon")), QString());
            EXPECT_EQ(elementOf(label(header, "Name")), QString());
            EXPECT_EQ(elementOf(label(&root, "Icon")), QString());
            EXPECT_EQ(elementOf(button(&root, "Help")), QString());
            EXPECT_EQ(elementOf(button(row, "ContextMenu")), QString());
            EXPECT_EQ(elementOf(button(header, "Other")), QString());

            // a label of a card header inside a color picker has the margins of ColorPicker.qss:45-49 (0 on every side): the elements of
            // that case, and none of the others (the order of the registrations decides nothing). The icons have the elements of their ids
            // (Card.qss:214-227); any other label of a picker has the picker's (ColorPickerElements.cpp)
            EXPECT_EQ(elementOf(label(pickerHeader, "Icon")), QStringLiteral("Global.Box.CardHeaderIconInPicker"));
            EXPECT_EQ(elementOf(label(pickerHeader, "WarningIcon")), QStringLiteral("Global.Box.CardHeaderWarningIconInPicker"));
            EXPECT_EQ(elementOf(label(pickerHeader, "Name")), QStringLiteral("Global.Box.PickerLabel"));
            EXPECT_EQ(elementOf(button(pickerHeader, "Help")), QStringLiteral("Global.Box.CardHeaderHelp"));

            // the expander of a header needs the class tag the rule names too (CheckBox.qss:121)
            auto* expander = new QCheckBox(header);
            expander->setObjectName(QStringLiteral("Expander"));
            EXPECT_EQ(elementOf(expander), QString());
            expander->setProperty("class", QStringLiteral("Expander"));
            expander->style()->unpolish(expander);
            expander->style()->polish(expander);
            EXPECT_EQ(elementOf(expander), QStringLiteral("Global.Box.CardHeaderExpander"));
        }

        TEST_F(PropertyEditorElementTests, Registry_RegisteringAgainChangesNothing)
        {
            // the application registers from Style::polish(QApplication*), which runs at every setStyle: a second call changes
            // which element a widget gets no more than a first one does (a duplicate row could not be told apart by the result:
            // registerGlobal keeps a flag so that it adds none)
            PropertyEditorElements::registerGlobal();
            PropertyEditorElements::registerGlobal();
            QWidget root;
            root.setStyle(m_style.get());
            auto* row = new StandIn(PropertyRowClass, "propertyRow", &root);
            auto* name = new QLabel(row);
            name->setObjectName(QStringLiteral("Name"));
            name->setStyle(m_style.get());
            name->ensurePolished();
            ASSERT_TRUE(BoxElement::forWidget(name) != nullptr);
            EXPECT_STREQ(BoxElement::forWidget(name)->m_name, "Global.Box.PropertyRowName");
        }
    } // namespace PropertyEditorElementTestsInternal
} // namespace AzQtComponents
