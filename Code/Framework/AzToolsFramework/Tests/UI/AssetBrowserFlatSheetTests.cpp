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
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/ExtendedLabel.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Tests/SheetTwins.h>
#include <AzQtComponents/Tests/SiteTwins.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntityInspectorWidget.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserListView.h>

#include <QApplication>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QStandardItemModel>
#include <QStyleFactory>
#include <QStyleOption>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>

#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The elements of the asset browser's inline sheets that live in AzToolsFramework (the preview image and the two switch buttons of
    // the asset inspector, the header of the list view), and the sheet that carries no look (`vertical-align: top` of the left pane):
    // which widgets the elements reach, and that they give what Qt's own style sheet style gave, at the device ratios the GUI runs at
    // ----------------------------------------------------------------------------------

    namespace AssetBrowserFlatSheetTestsInternal
    {
        //! A tree view that Qt takes for an instance of a class it is not. The matchers ask for the list view by its class name, as the
        //! selectors do, and building a real AssetBrowserListView needs the asset browser's models and buses.
        class ClassNamedTreeView
            : public QTreeView
        {
        public:
            ClassNamedTreeView(const char* className, QWidget* parent = nullptr)
                : QTreeView(parent)
                , m_className(className)
            {
            }

            void* qt_metacast(const char* name) override
            {
                return qstrcmp(name, m_className) == 0 ? this : QTreeView::qt_metacast(name);
            }

        private:
            const char* m_className;
        };

        constexpr const char* ListViewClass = "AzToolsFramework::AssetBrowser::AssetBrowserListView";
        constexpr const char* PreviewImageName = "m_previewImage";
        constexpr const char* PreviewImageElementName = "Editor.Box.AssetBrowserPreviewImage";

        // The sheets the widgets were given (AssetBrowserEntityInspectorWidget.cpp:169, AssetBrowserListView.cpp:50,
        // AzAssetBrowserWindow.ui:457 and AssetPickerDialog.ui:102), as they were before the flattening
        constexpr const char* PreviewImageSheet = "QLabel {background-color: #333333;}";
        constexpr const char* HeaderSheet = "QHeaderView { font-weight: bold; };";
        constexpr const char* LeftPaneSheet = "vertical-align: top";

        // The rules of the application's sheet that reach those widgets, with the lines they are on
        // BaseStyleSheet.qss:15-21, the universal rule
        constexpr const char* UniversalRule =
            "* { background-color: transparent; color: white; font-family: \"Open Sans\"; font-weight: 400; }";
        // Text.qss:91-95, the box of every label
        constexpr const char* LabelRule =
            ".Label, QLabel { font-family: \"Open Sans\"; font-size: 12px; margin-top: 1px; margin-bottom: 1px; }";
        // TableView.qss:62-76, the sections of the header of a tree
        constexpr const char* SectionRule =
            "QTreeView QHeaderView::section { height: 24px; background: #2d2d2d; border: none; font-size: 12px;"
            " font-style: normal; padding-left: 7px; }";
        // TableView.qss:14-29 and :31-40 (the parts that reach a tree and its items), BaseStyleSheet.qss:32-38 and QDockWidget.qss:33-36
        constexpr const char* TreeRule =
            "QTreeView { alternate-background-color: rgb(77, 77, 77); selection-background-color: rgb(101, 101, 101);"
            " selection-color: #ffffff; show-decoration-selected: 1; border: none; }"
            " QTreeView::item { height: 24px; padding-left: 4px; border: none; }"
            " QWidget > QTreeView { background-color: transparent; }"
            " QAbstractScrollArea { margin: 0; }";

        //! The rules of the application's sheet that reach the left pane of the form: what the legacy GUI draws it with. The section
        //! rule is in it because the tree of the twins shows the header a tree has by default: under it the legacy header is a solid
        //! fill, as the flat header is, where Qt's default section rule alone gives the base style's bevel.
        QString legacyPaneRules()
        {
            return QString::fromLatin1(UniversalRule) + QString::fromLatin1(LabelRule) + QString::fromLatin1(SectionRule)
                + QString::fromLatin1(TreeRule);
        }

        constexpr QRgb SectionFill = 0x2d2d2d;          // the section rule's background (TableView.qss:69, SectionRule above)

        // The tag the flattened GUI gives the buttons, which the elements are matched by (the inspector has no class a selector could
        // name), and the names a form of another place could give its own buttons
        constexpr const char* SwitchButtonProperty = "assetInspectorSwitchButton";
        constexpr const char* DetailsButtonTag = "details";
        constexpr const char* SceneSettingsButtonTag = "sceneSettings";
        constexpr const char* DetailsButtonName = "m_detailsButton";
        constexpr const char* SceneSettingsButtonName = "m_sceneSettingsButton";
        constexpr const char* DetailsButtonElementName = "Editor.Box.AssetInspectorDetailsButton";
        constexpr const char* SceneSettingsButtonElementName = "Editor.Box.AssetInspectorSceneSettingsButton";

        // The sheets of the Details and Scene Settings buttons (AssetBrowserEntityInspectorWidget.cpp:146-152 and :157-163, before the
        // flattening), as they were written: `border-color #222222` has no colon, and the last rule ends its selector with a colon.
        // A SNAPSHOT of the literals the widget holds: Call_TheLegacyGuiSetsTheSheetsTheButtonsAlwaysHad fails when they differ, so
        // that the legacy half of the twins is the sheet the legacy GUI sets.
        constexpr const char* DetailsButtonSheet =
            "QPushButton {background-color: #333333; border-color #222222; border-style: solid; border-width: 1px;"
            "border-top-left-radius: 3px; border-bottom-left-radius: 3px; border-top-right-radius: 0px;"
            "border-bottom-right-radius: 0px; font-size: 12px; height: 28px;}"
            "QPushButton:checked {background-color: #1E70EB;}"
            "QPushButton:hover:!checked {background-color: #444444;}"
            "QPushButton:pressed:!checked: {background-color: #444444;}";
        constexpr const char* SceneSettingsButtonSheet =
            "QPushButton {background-color: #333333; border-color #222222; border-style: solid; border-width: 1px;"
            "border-top-left-radius: 0px; border-bottom-left-radius: 0px; border-top-right-radius: 3px;"
            "border-bottom-right-radius: 0px; margin-left: -1px; font-size: 12px; height: 28px;}"
            "QPushButton:checked {background-color: #1E70EB;}"
            "QPushButton:hover:!checked {background-color: #444444;}"
            "QPushButton:pressed:!checked: {background-color: #444444;}";
        // PushButton.qss:30-35, :37-41, :43-46 and :48-52, the rules of the application's sheet that reach a push button
        constexpr const char* PushButtonRules =
            "QPushButton { color: #ffffff; font-family: \"Open Sans\"; font-size: 12px; }"
            " QPushButton:hover, QPushButton:pressed { color: #FFFFFF; }"
            " QPushButton:disabled { color: #999999; }"
            " QPushButton:flat { border: none; background-color: transparent; }";

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

        //! A colour as text for a failure message (without the alpha)
        std::string hexOf(QRgb rgb)
        {
            return QString::number(rgb & 0xFFFFFF, 16).toStdString();
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
                        return "at (" + std::to_string(x) + ", " + std::to_string(y) + "): first #" + hexOf(a.pixel(x, y)) + ", second #"
                            + hexOf(b.pixel(x, y));
                    }
                }
            }
            return "";
        }

        //! The colour (without the alpha) of the pixel of an image drawn at a device ratio that lies at a logical point
        QRgb colorAt(const QImage& image, const QPoint& logical, qreal ratio)
        {
            return image.pixel(qRound(logical.x() * ratio), qRound(logical.y() * ratio)) & 0xFFFFFF;
        }

        //! The attributes of a font the header's sheet gave and the element gives: what the twin compares
        struct FontFacts
        {
            QString m_family;
            int m_weight = 0;
            int m_pixelSize = 0;
            qreal m_pointSize = 0;
            bool m_italic = false;

            explicit FontFacts(const QFont& font)
                : m_family(font.family())
                , m_weight(font.weight())
                , m_pixelSize(font.pixelSize())
                , m_pointSize(font.pointSizeF())
                , m_italic(font.italic())
            {
            }

            bool operator==(const FontFacts& other) const
            {
                return m_family == other.m_family && m_weight == other.m_weight && m_pixelSize == other.m_pixelSize
                    && qFuzzyCompare(m_pointSize + 1.0, other.m_pointSize + 1.0) && m_italic == other.m_italic;
            }
        };

        std::ostream& operator<<(std::ostream& stream, const FontFacts& facts)
        {
            return stream << facts.m_family.toStdString() << " weight " << facts.m_weight << " px " << facts.m_pixelSize << " pt "
                          << facts.m_pointSize << (facts.m_italic ? " italic" : "");
        }

        using InspectorWidget = AzToolsFramework::AssetBrowser::AssetBrowserEntityInspectorWidget;

        //! The two switch buttons in a row, as the constructor makes them (text, checkable, flat, no space between them)
        struct SwitchButtons
        {
            QWidget m_root;
            QPushButton* m_details = nullptr;
            QPushButton* m_sceneSettings = nullptr;

            SwitchButtons()
            {
                auto* layout = new QHBoxLayout(&m_root);
                layout->setSpacing(0);
                layout->setContentsMargins(0, 0, 0, 0);
                m_details = new QPushButton(QStringLiteral("Details"), &m_root);
                m_sceneSettings = new QPushButton(QStringLiteral("Scene Settings"), &m_root);
                for (QPushButton* button : { m_details, m_sceneSettings })
                {
                    button->setCheckable(true);
                    button->setFlat(true);
                    layout->addWidget(button);
                }
                m_root.resize(300, 40);
            }
        };

        constexpr QRect SwitchButtonRect(0, 0, 100, 30);    // the box the twins draw a button in: 28px of contents and the 1px border

        //! The option QPushButton::paintEvent builds for a button, in the state asked for (QPushButton::initStyleOption, protected)
        QStyleOptionButton switchButtonOption(const QPushButton& button, QStyle::State state)
        {
            QStyleOptionButton option;
            option.initFrom(&button);
            option.state = state;
            option.features = QStyleOptionButton::Flat;     // both buttons are flat
            option.text = button.text();
            option.rect = SwitchButtonRect;
            return option;
        }

        //! The button as its own style draws it in a state (CE_PushButton: the bevel, then the label), at a device ratio, on a grey
        //! backdrop; the painter has the font of the widget, as QStylePainter has in a paint event
        QImage paintSwitchButton(const QPushButton& button, QStyle::State state, qreal ratio)
        {
            const QStyleOptionButton option = switchButtonOption(button, state);
            QImage image((QSizeF(SwitchButtonRect.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor::fromRgb(ParentColor));
            QPainter painter(&image);
            painter.setFont(button.font());
            button.style()->drawControl(QStyle::CE_PushButton, &option, &painter, &button);
            painter.end();
            return image;
        }

        constexpr QRgb LeftPaneBackdrop = 0xFF00FF;     // what the pane is drawn on: a pixel of another colour is something drawn

        //! The left pane as it is drawn: the image, and for the tree, the label and the divider in it (in that order) the rect each has
        //! in the window and the contents box it lays out its contents in; the rect of the tree's header in the window; whether the
        //! pane, the tree or its header is under Qt's style sheet style
        struct LeftPaneLook
        {
            QImage m_image;
            std::vector<QRect> m_rects;
            std::vector<QRect> m_contents;
            QRect m_header;
            bool m_sheetStyle = false;
        };

        //! How the pane is given the sheet of the form (AzAssetBrowserWindow.ui:457, before the flattening)
        enum class SheetCall
        {
            None,               //!< the pane has no sheet
            SetStyleSheet,      //!< QWidget::setStyleSheet: what the legacy GUI does
            LegacyHelper        //!< StyleManager::setLegacyStyleSheet: the call the form makes now, which sets nothing in the flat rung
        };

        //! Whether anything is drawn in a rect (logical pixels) of an image filled with a backdrop: a pixel of another colour
        bool hasInkIn(const QImage& image, const QRect& rect, qreal ratio, QRgb backdrop)
        {
            const QRect device = QRect(rect.topLeft() * ratio, rect.size() * ratio).intersected(image.rect());
            for (int y = device.top(); y <= device.bottom(); ++y)
            {
                for (int x = device.left(); x <= device.right(); ++x)
                {
                    if ((image.pixel(x, y) & 0xFFFFFF) != backdrop)
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        class AssetBrowserFlatSheetTests
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
                AzToolsFramework::AssetBrowser::AssetBrowserEntityInspectorWidget::RegisterFlatElements();
                AzToolsFramework::AssetBrowser::AssetBrowserListView::RegisterFlatElements();
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

            //! Fusion on a widget and everything in it, set on each so that the style of the screen the test runs on is not their base;
            //! before any sheet, as the legacy GUI's base style was there before the sheets
            void useFusion(QWidget& root) const
            {
                useStyle(root, m_fusion.get());
            }

            //! A style on a widget and everything in it: a style set on a widget does not reach its children
            static void useStyle(QWidget& root, QStyle* style)
            {
                root.setStyle(style);
                for (QWidget* child : root.findChildren<QWidget*>())
                {
                    child->setStyle(style);
                }
            }

            //! The rules of the application's sheet on the root, over the sheets set on the widgets in it: Qt's style sheet style over
            //! Fusion, polished
            static void applyApplicationRules(QWidget& root, const QString& rules)
            {
                root.setStyleSheet(rules);
                root.ensurePolished();
                for (QWidget* child : root.findChildren<QWidget*>())
                {
                    child->ensurePolished();
                }
            }

            //! The contents box of a frame (SE_ShapedFrameContents): where a label lays out its text, and whether it indents it
            static QRect frameContents(QWidget& widget)
            {
                const QStyleOptionFrame option = frameOption(qobject_cast<QFrame*>(&widget));
                return widget.style()->subElementRect(QStyle::SE_ShapedFrameContents, &option, &widget);
            }

            //! The preview image as the legacy GUI had it: the sheet the constructor gave it, under the rules of the application's
            QImage legacyPreviewImage(qreal ratio, QRect* contents)
            {
                QWidget root;
                auto* label = new AzQtComponents::ExtendedLabel(&root);
                label->setObjectName(QString::fromLatin1(PreviewImageName));
                label->resize(120, 60);
                useFusion(root);
                label->setStyleSheet(QString::fromLatin1(PreviewImageSheet));
                applyApplicationRules(root, QString::fromLatin1(UniversalRule) + QString::fromLatin1(LabelRule));
                *contents = frameContents(*label);
                return paintBox(*label, ratio);
            }

            //! The same label in the flattened GUI: its element, and no sheet
            QImage flatPreviewImage(qreal ratio, QRect* contents)
            {
                QWidget root;
                auto* label = new AzQtComponents::ExtendedLabel(&root);
                label->setObjectName(QString::fromLatin1(PreviewImageName));
                label->resize(120, 60);
                flatten(*label);
                *contents = frameContents(*label);
                return paintBox(*label, ratio);
            }

            //! The pair as the legacy GUI draws it: Qt's style sheet style over Fusion, the sheets the buttons were given on them, the
            //! rules of the application's sheet that reach a push button on the root. The components' polish gave every push button a
            //! maximum height before the sheet's geometry came (the sheets set none), which a plain button has.
            void legacyButtons(SwitchButtons& pair)
            {
                QPushButton plain;
                plain.setStyle(m_style.get());
                plain.ensurePolished();

                useFusion(pair.m_root);
                pair.m_details->setStyleSheet(QString::fromLatin1(DetailsButtonSheet));
                pair.m_sceneSettings->setStyleSheet(QString::fromLatin1(SceneSettingsButtonSheet));
                for (QPushButton* button : { pair.m_details, pair.m_sceneSettings })
                {
                    button->setMaximumHeight(plain.maximumHeight());
                }
                applyApplicationRules(pair.m_root, QString::fromLatin1(UniversalRule) + QString::fromLatin1(PushButtonRules));
            }

            //! The pair as the flattened GUI draws it: the flat style, no sheet, the buttons tagged by the call that styles them (the
            //! one the constructor makes), polished by the style as a shown window would
            void flatButtons(SwitchButtons& pair)
            {
                useStyle(pair.m_root, m_style.get());
                InspectorWidget::StyleSwitchButtons(pair.m_details, pair.m_sceneSettings);
                pair.m_root.ensurePolished();
            }

            //! The left pane of the form (m_leftLayout, AzAssetBrowserWindow.ui:457, before the flattening) drawn at a ratio: a widget
            //! with a tree, a label and a divider in it. `style` is the base style of every widget, set before any sheet; `sheet` is how
            //! the pane gets the sheet of the form; `rules` is the application's sheet (none in the flattened GUI). The window is shown
            //! off screen: a tree lays out and paints its rows only once it is.
            LeftPaneLook drawLeftPane(QStyle* style, SheetCall sheet, const QString& rules, qreal ratio)
            {
                QWidget outer;
                outer.resize(140, 120);
                auto* outerLayout = new QVBoxLayout(&outer);
                outerLayout->setContentsMargins(0, 0, 0, 0);
                auto* pane = new QWidget(&outer);
                pane->setObjectName(QStringLiteral("m_leftLayout"));
                outerLayout->addWidget(pane);
                auto* layout = new QVBoxLayout(pane);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(0);

                auto* tree = new QTreeView(pane);
                auto* model = new QStandardItemModel(tree);
                for (int row = 0; row < 3; ++row)
                {
                    auto* item = new QStandardItem(QStringLiteral("Entry %1").arg(row));
                    item->appendRow(new QStandardItem(QStringLiteral("Child")));
                    model->appendRow(item);
                }
                tree->setModel(model);
                tree->expandAll();
                layout->addWidget(tree);
                auto* label = new QLabel(QStringLiteral("Label"), pane);
                layout->addWidget(label);
                auto* divider = new QFrame(pane);
                divider->setFrameShape(QFrame::HLine);
                layout->addWidget(divider);

                useStyle(outer, style);
                if (sheet == SheetCall::SetStyleSheet)
                {
                    pane->setStyleSheet(QString::fromLatin1(LeftPaneSheet));
                }
                else if (sheet == SheetCall::LegacyHelper)
                {
                    AzQtComponents::StyleManager::setLegacyStyleSheet(pane, QString::fromLatin1(LeftPaneSheet));
                }
                applyApplicationRules(outer, rules);
                outer.setAttribute(Qt::WA_DontShowOnScreen);
                outer.show();
                QApplication::processEvents();
                outerLayout->activate();

                LeftPaneLook look;
                look.m_image = QImage((QSizeF(outer.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
                look.m_image.setDevicePixelRatio(ratio);
                look.m_image.fill(QColor::fromRgb(LeftPaneBackdrop));
                outer.render(&look.m_image, QPoint(), QRegion(), QWidget::DrawChildren);
                const std::vector<QWidget*> widgets{ tree, label, divider };
                for (QWidget* widget : widgets)
                {
                    look.m_rects.push_back(QRect(widget->mapTo(&outer, QPoint(0, 0)), widget->size()));
                    look.m_contents.push_back(frameContents(*widget));
                }
                QHeaderView* header = tree->header();
                look.m_header = QRect(header->mapTo(&outer, QPoint(0, 0)), header->size());
                look.m_sheetStyle = pane->style()->inherits("QStyleSheetStyle") || tree->style()->inherits("QStyleSheetStyle")
                    || header->style()->inherits("QStyleSheetStyle");
                return look;
            }

            //! The pane given the sheet of the form by `sheet` is the pane without it, at the ratios the GUI runs at: the same image
            //! (drawn at all), the same rects, the same contents boxes; and the pane is under Qt's style sheet style when an application
            //! sheet reaches it, and under none when no sheet does
            void expectTheLeftPaneSheetChangesNothing(QStyle* style, SheetCall sheet, const QString& rules)
            {
                for (const qreal ratio : { 1.0, 1.5, 2.0 })
                {
                    const LeftPaneLook plain = drawLeftPane(style, SheetCall::None, rules, ratio);
                    const LeftPaneLook declared = drawLeftPane(style, sheet, rules, ratio);

                    EXPECT_EQ(plain.m_sheetStyle, !rules.isEmpty()) << "at ratio " << ratio;
                    EXPECT_EQ(declared.m_sheetStyle, !rules.isEmpty()) << "at ratio " << ratio;

                    // The comparison is not between two blank images: the tree draws its rows and the label its text
                    ASSERT_EQ(plain.m_rects.size(), 3u);
                    EXPECT_TRUE(hasInkIn(plain.m_image, plain.m_rects[0], ratio, LeftPaneBackdrop)) << "tree at ratio " << ratio;
                    EXPECT_TRUE(hasInkIn(plain.m_image, plain.m_rects[1], ratio, LeftPaneBackdrop)) << "label at ratio " << ratio;

                    EXPECT_EQ(declared.m_rects, plain.m_rects) << "at ratio " << ratio;
                    EXPECT_EQ(declared.m_contents, plain.m_contents) << "at ratio " << ratio;
                    EXPECT_EQ(firstDifference(plain.m_image, declared.m_image), std::string()) << "at ratio " << ratio;
                }
            }

            QString m_applicationSheet;
            AZStd::unique_ptr<AzQtComponents::Style> m_style;
            AZStd::unique_ptr<QStyle> m_fusion;
        };
    } // namespace AssetBrowserFlatSheetTestsInternal

    // A unity build joins this file with others of namespace UnitTest: the fixture is the only name brought in, the rest is named
    // through the alias
    using AssetBrowserFlatSheetTestsInternal::AssetBrowserFlatSheetTests;
    namespace AssetBrowserFlat = AssetBrowserFlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // The preview image of the asset inspector (AssetBrowserEntityInspectorWidget.cpp:169)
    // ----------------------------------------------------------------------------------

    TEST_F(AssetBrowserFlatSheetTests, Registry_ThePreviewImageIsTheExtendedLabelOfThatNameAndNoOtherLabel)
    {
        QWidget root;
        auto* preview = new AzQtComponents::ExtendedLabel(&root);
        preview->setObjectName(QString::fromLatin1(AssetBrowserFlat::PreviewImageName));

        // The same name on a plain label (the sheet's `QLabel` reached the ExtendedLabel it was set on, no other widget), and another
        // name on an ExtendedLabel (the clear buttons of the search fields are ExtendedLabels too)
        auto* plainLabel = new QLabel(&root);
        plainLabel->setObjectName(QString::fromLatin1(AssetBrowserFlat::PreviewImageName));
        auto* otherExtendedLabel = new AzQtComponents::ExtendedLabel(&root);
        otherExtendedLabel->setObjectName(QStringLiteral("m_clearFiltersButton"));

        flatten(*preview);
        flatten(*plainLabel);
        flatten(*otherExtendedLabel);

        EXPECT_STREQ(elementOf(*preview), AssetBrowserFlat::PreviewImageElementName);
        EXPECT_TRUE(preview->testAttribute(Qt::WA_StyledBackground));      // the fill is drawn at PE_Widget
        EXPECT_STRNE(elementOf(*plainLabel), AssetBrowserFlat::PreviewImageElementName);
        EXPECT_STRNE(elementOf(*otherExtendedLabel), AssetBrowserFlat::PreviewImageElementName);
    }

    TEST_F(AssetBrowserFlatSheetTests, Pixels_ThePreviewImagePaintsWhatTheSheetPaintedAtEveryRatio)
    {
        for (const qreal ratio : { 1.0, 1.5, 2.0 })
        {
            QRect legacyContents;
            QRect flatContents;
            const QImage legacy = legacyPreviewImage(ratio, &legacyContents);
            const QImage flat = flatPreviewImage(ratio, &flatContents);
            EXPECT_EQ(AssetBrowserFlat::firstDifference(legacy, flat), std::string()) << "at ratio " << ratio;
        }
    }

    TEST_F(AssetBrowserFlatSheetTests, Pixels_ThePreviewImageIsFilledWithTheSheetsColourToItsEdgesExceptTheLabelRulesMargin)
    {
        QRect contents;
        const QImage flat = flatPreviewImage(1.0, &contents);

        // `QLabel { margin-top: 1px; margin-bottom: 1px }` (Text.qss:93-94) is a box of the label outside its background: the rows
        // above and below the fill stay the parent's, as the sheet style left them
        EXPECT_EQ(flat.pixel(60, 0) & 0xFFFFFF, AssetBrowserFlat::ParentColor);
        EXPECT_EQ(flat.pixel(60, 1) & 0xFFFFFF, 0x333333u);
        EXPECT_EQ(flat.pixel(0, 30) & 0xFFFFFF, 0x333333u);
        EXPECT_EQ(flat.pixel(119, 30) & 0xFFFFFF, 0x333333u);
        EXPECT_EQ(flat.pixel(60, 58) & 0xFFFFFF, 0x333333u);
        EXPECT_EQ(flat.pixel(60, 59) & 0xFFFFFF, AssetBrowserFlat::ParentColor);
    }

    TEST_F(AssetBrowserFlatSheetTests, Box_ThePreviewImageLaysItsContentsOutAsTheLabelRuleDidUnderItsSheet)
    {
        // Where the label lays out its pixmap and text: the label rule's margin above and below (and so QLabel's indent of half an 'x')
        QRect legacyContents;
        QRect flatContents;
        legacyPreviewImage(1.0, &legacyContents);
        flatPreviewImage(1.0, &flatContents);

        EXPECT_EQ(flatContents, legacyContents);
        EXPECT_EQ(flatContents, QRect(0, 1, 120, 58));         // the label is 120 x 60
    }

    // ----------------------------------------------------------------------------------
    // The header of the list view (AssetBrowserListView.cpp:50): a bold font
    // ----------------------------------------------------------------------------------

    TEST_F(AssetBrowserFlatSheetTests, Registry_OnlyTheHeaderOfAListViewHasTheBoldElement)
    {
        // The header of the asset browser's list view (a child of it, as a tree's own header is), the header of another tree, one of a
        // tree in a widget of its own, and a header with no view at all
        AssetBrowserFlat::ClassNamedTreeView listView(AssetBrowserFlat::ListViewClass);
        QTreeView otherView;
        QWidget wrapper;
        auto* nested = new QTreeView(&wrapper);
        QHeaderView stray(Qt::Horizontal);

        AzQtComponents::TextElement::resolve(listView.header());
        AzQtComponents::TextElement::resolve(otherView.header());
        AzQtComponents::TextElement::resolve(nested->header());
        AzQtComponents::TextElement::resolve(&stray);

        EXPECT_EQ(listView.header()->font().weight(), static_cast<int>(QFont::Bold));
        EXPECT_NE(otherView.header()->font().weight(), static_cast<int>(QFont::Bold));
        EXPECT_NE(nested->header()->font().weight(), static_cast<int>(QFont::Bold));
        EXPECT_NE(stray.font().weight(), static_cast<int>(QFont::Bold));
    }

    TEST_F(AssetBrowserFlatSheetTests, Font_TheHeaderAndItsViewportHaveTheFontTheSheetGaveThem)
    {
        // Legacy: the sheet on the header, under the universal rule and the section rule of the application's sheet
        QWidget legacyRoot;
        auto* legacyTree = new QTreeView(&legacyRoot);
        useFusion(legacyRoot);
        legacyTree->header()->setStyleSheet(QString::fromLatin1(AssetBrowserFlat::HeaderSheet));
        applyApplicationRules(
            legacyRoot, QString::fromLatin1(AssetBrowserFlat::UniversalRule) + QString::fromLatin1(AssetBrowserFlat::SectionRule));

        // Flat: the element, resolved on the header as Style::polish does first in the flattened GUI
        AssetBrowserFlat::ClassNamedTreeView flatTree(AssetBrowserFlat::ListViewClass);
        flatTree.setStyle(m_style.get());
        AzQtComponents::TextElement::resolve(flatTree.header());

        const AssetBrowserFlat::FontFacts legacyHeader(legacyTree->header()->font());
        const AssetBrowserFlat::FontFacts flatHeader(flatTree.header()->font());
        EXPECT_EQ(flatHeader, legacyHeader);
        EXPECT_EQ(flatHeader.m_weight, static_cast<int>(QFont::Bold));

        // What the sections are painted with: the font of the viewport, which the header's font reached in both
        const AssetBrowserFlat::FontFacts legacyViewport(legacyTree->header()->viewport()->font());
        const AssetBrowserFlat::FontFacts flatViewport(flatTree.header()->viewport()->font());
        EXPECT_EQ(flatViewport, legacyViewport);
        EXPECT_EQ(flatViewport.m_weight, static_cast<int>(QFont::Bold));
    }

    // ----------------------------------------------------------------------------------
    // The Details and Scene Settings buttons (AssetBrowserEntityInspectorWidget.cpp:145-163 before the flattening): the pair that
    // switches the inspector between the asset's details and its scene settings, round on its outer sides only. The legacy half of the
    // twins is the two sheets under the rules of the application's sheet that reach a push button (BaseStyleSheet.qss:15-21, the
    // universal rule; PushButton.qss:30-35, :37-41, :43-46 and :48-52), the flat half the elements in a flat style.
    //
    // What Qt makes of the sheets is read from its source (qcssparser.cpp, qstylesheetstyle.cpp, qcssutil.cpp of Qt 6.11): the dropped
    // `border-color #222222`, the dead `:pressed:!checked:` rule and the black edges. No capture shows the buttons, so the tests below
    // are what settles that reading: if Rules_TheButtonsPaintWhatTheRulesThatSurvivedTheParserPaintInBothGuis fails on the legacy half, the
    // reading is wrong and the element's values are to change, not the test.
    // ----------------------------------------------------------------------------------

    TEST_F(AssetBrowserFlatSheetTests, Registry_TheSwitchButtonsAreThePushButtonsTheInspectorTaggedAndNoOtherWidget)
    {
        using Inspector = AssetBrowserFlat::InspectorWidget;
        QWidget root;

        // Every push button here is made as the inspector makes its two (AssetBrowserEntityInspectorWidget.cpp:356-363): checkable
        // and flat, so that the tag alone tells them apart
        const auto newSwitchKindButton = [&root]()
        {
            auto* button = new QPushButton(&root);
            button->setCheckable(true);
            button->setFlat(true);
            return button;
        };
        auto* details = newSwitchKindButton();
        auto* sceneSettings = newSwitchKindButton();
        Inspector::StyleSwitchButtons(details, sceneSettings);

        // The flattened GUI sets no sheet, and tags the buttons for their elements (the legacy GUI's widgets are named as they were)
        EXPECT_TRUE(details->styleSheet().isEmpty());
        EXPECT_TRUE(sceneSettings->styleSheet().isEmpty());
        EXPECT_EQ(details->property(AssetBrowserFlat::SwitchButtonProperty).toString(),
            QString::fromLatin1(AssetBrowserFlat::DetailsButtonTag));
        EXPECT_EQ(sceneSettings->property(AssetBrowserFlat::SwitchButtonProperty).toString(),
            QString::fromLatin1(AssetBrowserFlat::SceneSettingsButtonTag));
        EXPECT_TRUE(details->objectName().isEmpty());
        EXPECT_TRUE(sceneSettings->objectName().isEmpty());

        // Not the buttons: push buttons of the names a form of another place could give its own (the legacy sheets were set on these
        // two widgets, whatever any other is called), the tags on widgets of other classes, another tag, and no tag
        auto* sameNameAsDetails = newSwitchKindButton();
        sameNameAsDetails->setObjectName(QString::fromLatin1(AssetBrowserFlat::DetailsButtonName));
        auto* sameNameAsSceneSettings = newSwitchKindButton();
        sameNameAsSceneSettings->setObjectName(QString::fromLatin1(AssetBrowserFlat::SceneSettingsButtonName));
        auto* toolButton = new QToolButton(&root);
        toolButton->setProperty(AssetBrowserFlat::SwitchButtonProperty, QString::fromLatin1(AssetBrowserFlat::DetailsButtonTag));
        auto* label = new QLabel(&root);
        label->setProperty(AssetBrowserFlat::SwitchButtonProperty, QString::fromLatin1(AssetBrowserFlat::SceneSettingsButtonTag));
        auto* otherTag = newSwitchKindButton();
        otherTag->setProperty(AssetBrowserFlat::SwitchButtonProperty, QStringLiteral("somethingElse"));
        auto* untagged = newSwitchKindButton();

        const std::vector<QWidget*> others{ sameNameAsDetails, sameNameAsSceneSettings, toolButton, label, otherTag, untagged };
        flatten(*details);
        flatten(*sceneSettings);
        for (QWidget* other : others)
        {
            flatten(*other);
        }

        EXPECT_STREQ(elementOf(*details), AssetBrowserFlat::DetailsButtonElementName);
        EXPECT_STREQ(elementOf(*sceneSettings), AssetBrowserFlat::SceneSettingsButtonElementName);
        EXPECT_TRUE(details->testAttribute(Qt::WA_Hover));          // the elements have a hover face, as the sheets had a :hover rule
        EXPECT_TRUE(sceneSettings->testAttribute(Qt::WA_Hover));
        for (QWidget* other : others)
        {
            EXPECT_STRNE(elementOf(*other), AssetBrowserFlat::DetailsButtonElementName) << other->objectName().toStdString();
            EXPECT_STRNE(elementOf(*other), AssetBrowserFlat::SceneSettingsButtonElementName) << other->objectName().toStdString();
        }
    }

    TEST_F(AssetBrowserFlatSheetTests, Call_TheLegacyGuiSetsTheSheetsTheButtonsAlwaysHadAndTagsNothing)
    {
        using Inspector = AssetBrowserFlat::InspectorWidget;
        AzQtComponents::GuiPerfFloor::setRungForTesting(AzQtComponents::GuiPerfFloor::Rung::Qss);

        QPushButton details;
        QPushButton sceneSettings;
        Inspector::StyleSwitchButtons(&details, &sceneSettings);

        // Each sheet on its own button: the call makes the mapping the constructor relies on
        EXPECT_EQ(details.styleSheet(), QString::fromLatin1(AssetBrowserFlat::DetailsButtonSheet));
        EXPECT_EQ(sceneSettings.styleSheet(), QString::fromLatin1(AssetBrowserFlat::SceneSettingsButtonSheet));
        EXPECT_TRUE(details.objectName().isEmpty());                // the legacy GUI's widgets are carried as they were
        EXPECT_TRUE(sceneSettings.objectName().isEmpty());
        EXPECT_FALSE(details.property(AssetBrowserFlat::SwitchButtonProperty).isValid());
        EXPECT_FALSE(sceneSettings.property(AssetBrowserFlat::SwitchButtonProperty).isValid());
    }

    TEST_F(AssetBrowserFlatSheetTests, Element_TheCornersTheMarginAndTheFacesAreThoseOfTheSheets)
    {
        AssetBrowserFlat::SwitchButtons pair;
        flatButtons(pair);
        const AzQtComponents::BoxElement* details = AzQtComponents::BoxElement::forWidget(pair.m_details);
        const AzQtComponents::BoxElement* sceneSettings = AzQtComponents::BoxElement::forWidget(pair.m_sceneSettings);
        ASSERT_TRUE(details != nullptr);
        ASSERT_TRUE(sceneSettings != nullptr);

        // Round on the outer side of each: border-top-left-radius, border-bottom-left-radius: 3px on the first, border-top-right-radius:
        // 3px on the second (its border-bottom-right-radius is 0)
        EXPECT_EQ(details->m_radii.m_topLeft, 3);
        EXPECT_EQ(details->m_radii.m_bottomLeft, 3);
        EXPECT_EQ(details->m_radii.m_topRight, 0);
        EXPECT_EQ(details->m_radii.m_bottomRight, 0);
        EXPECT_EQ(sceneSettings->m_radii.m_topRight, 3);
        EXPECT_EQ(sceneSettings->m_radii.m_topLeft, 0);
        EXPECT_EQ(sceneSettings->m_radii.m_bottomLeft, 0);
        EXPECT_EQ(sceneSettings->m_radii.m_bottomRight, 0);
        EXPECT_EQ(details->m_radius, 0);
        EXPECT_EQ(sceneSettings->m_radius, 0);

        // The box: a border of its own, 28px of contents, and the second one pixel over the first (margin-left: -1px)
        EXPECT_FALSE(details->m_nativeBorder);
        EXPECT_FALSE(sceneSettings->m_nativeBorder);
        EXPECT_EQ(details->m_size, QSize(-1, 28));
        EXPECT_EQ(sceneSettings->m_size, QSize(-1, 28));
        EXPECT_TRUE(details->m_margin.isNull());
        EXPECT_TRUE(sceneSettings->m_margin == QMargins(-1, 0, 0, 0));

        // The faces, in the states the rules cascade: #333333, #444444 under the pointer unless checked, #1E70EB checked (hovered too:
        // the checked rule outranks the hover one), and the label #999999 while disabled; the edges are the same in every state
        const QStyle::State enabled = QStyle::State_Enabled;
        const QStyle::State hover = QStyle::State_MouseOver;
        const QStyle::State checked = QStyle::State_On;
        const auto expectFace = [](const AzQtComponents::BoxElement& element, QStyle::State state, QRgb fill, QRgb text, const char* what)
        {
            QStyleOption option;
            option.state = state;
            option.rect = AssetBrowserFlat::SwitchButtonRect;
            const AzQtComponents::BoxElement::Face& face = element.face(&option);
            EXPECT_TRUE(face.m_fill.style() == Qt::SolidPattern) << what;
            EXPECT_EQ(face.m_fill.color().rgb() & 0xFFFFFF, fill) << what;
            EXPECT_EQ(face.m_text.rgb() & 0xFFFFFF, text) << what;
            for (const AzQtComponents::BoxElement::Edge& edge : { face.m_edges.m_top, face.m_edges.m_right, face.m_edges.m_bottom,
                                                                  face.m_edges.m_left })
            {
                EXPECT_EQ(edge.m_width, 1) << what;
                EXPECT_EQ(edge.m_color.rgb() & 0xFFFFFF, 0x000000u) << what;
            }
        };
        for (const AzQtComponents::BoxElement* element : { details, sceneSettings })
        {
            expectFace(*element, enabled, 0x333333, 0xFFFFFF, "idle");
            expectFace(*element, enabled | hover, 0x444444, 0xFFFFFF, "hovered");
            expectFace(*element, enabled | checked, 0x1E70EB, 0xFFFFFF, "checked");
            expectFace(*element, enabled | checked | hover, 0x1E70EB, 0xFFFFFF, "checked and hovered");
            expectFace(*element, QStyle::State_None, 0x333333, 0x999999, "disabled");
            expectFace(*element, checked, 0x1E70EB, 0x999999, "disabled and checked");
        }
    }

    TEST_F(AssetBrowserFlatSheetTests, Geometry_TheButtonsBoxHintsLimitsAndRowAreThoseTheSheetsMade)
    {
        AssetBrowserFlat::SwitchButtons legacy;
        legacyButtons(legacy);
        AssetBrowserFlat::SwitchButtons flat;
        flatButtons(flat);

        const auto contents = [](const QPushButton& button)
        {
            const QStyleOptionButton option = AssetBrowserFlat::switchButtonOption(button, QStyle::State_Enabled);
            return button.style()->subElementRect(QStyle::SE_PushButtonContents, &option, &button);
        };
        const std::vector<std::pair<const QPushButton*, const QPushButton*>> buttons{
            { legacy.m_details, flat.m_details },
            { legacy.m_sceneSettings, flat.m_sceneSettings },
        };
        for (const auto& [legacyButton, flatButton] : buttons)
        {
            const std::string which = legacyButton->text().toStdString();
            EXPECT_EQ(flatButton->sizeHint(), legacyButton->sizeHint()) << which;
            EXPECT_EQ(flatButton->minimumSizeHint(), legacyButton->minimumSizeHint()) << which;
            EXPECT_EQ(flatButton->minimumSize(), legacyButton->minimumSize()) << which;
            EXPECT_EQ(flatButton->maximumSize(), legacyButton->maximumSize()) << which;
            EXPECT_EQ(contents(*flatButton), contents(*legacyButton)) << which;

            // 28px of contents (`height: 28px`) and the 1px border on each side: the box does not depend on the corners
            EXPECT_EQ(legacyButton->sizeHint().height(), 30) << which;
        }

        // Where the label lays out in a box of 100 by 30: inside the border, and for the second inside the border one pixel over
        EXPECT_EQ(contents(*legacy.m_details), QRect(1, 1, 98, 28));
        EXPECT_EQ(contents(*legacy.m_sceneSettings), QRect(0, 1, 99, 28));

        // The row: the second begins where the first ends (spacing 0), the hint of each is a pixel narrower than its text and border
        // by the margin
        legacy.m_root.layout()->activate();
        flat.m_root.layout()->activate();
        EXPECT_EQ(flat.m_details->geometry(), legacy.m_details->geometry());
        EXPECT_EQ(flat.m_sceneSettings->geometry(), legacy.m_sceneSettings->geometry());
    }

    TEST_F(AssetBrowserFlatSheetTests, Font_TheLabelHasTheFontTheSheetsGaveItAndItsColourIsTheRulesAtRestAndDisabled)
    {
        AssetBrowserFlat::SwitchButtons legacy;
        legacyButtons(legacy);
        AssetBrowserFlat::SwitchButtons flat;
        flatButtons(flat);

        // `font-size: 12px` (the sheet, PushButton.qss:34), Open Sans (PushButton.qss:33), weight 400 (BaseStyleSheet.qss:20)
        const AssetBrowserFlat::FontFacts legacyFont(legacy.m_details->font());
        EXPECT_EQ(AssetBrowserFlat::FontFacts(flat.m_details->font()), legacyFont);
        EXPECT_EQ(AssetBrowserFlat::FontFacts(flat.m_sceneSettings->font()), AssetBrowserFlat::FontFacts(legacy.m_sceneSettings->font()));
        EXPECT_EQ(legacyFont.m_pixelSize, 12);

        // The colour of the label, which the sheet's rules gave the palette of the button per group: white (PushButton.qss:32), and
        // #999999 disabled (:45). The element's faces carry the same two.
        const QPalette& palette = legacy.m_details->palette();
        EXPECT_EQ(palette.color(QPalette::Active, QPalette::ButtonText).rgb() & 0xFFFFFF, 0xFFFFFFu);
        EXPECT_EQ(palette.color(QPalette::Disabled, QPalette::ButtonText).rgb() & 0xFFFFFF, 0x999999u);
        for (const QPushButton* button : { flat.m_details, flat.m_sceneSettings })
        {
            QStyleOption enabled;
            enabled.state = QStyle::State_Enabled;
            QStyleOption disabled;
            disabled.state = QStyle::State_None;
            const AzQtComponents::BoxElement* element = AzQtComponents::BoxElement::forWidget(button);
            ASSERT_TRUE(element != nullptr);
            EXPECT_EQ(element->face(&enabled).m_text.rgb(), palette.color(QPalette::Active, QPalette::ButtonText).rgb());
            EXPECT_EQ(element->face(&disabled).m_text.rgb(), palette.color(QPalette::Disabled, QPalette::ButtonText).rgb());
        }
    }

    TEST_F(AssetBrowserFlatSheetTests, Pixels_EachButtonPaintsWhatItsSheetPaintedInEveryStateAtEveryRatio)
    {
        AssetBrowserFlat::SwitchButtons legacy;
        legacyButtons(legacy);
        AssetBrowserFlat::SwitchButtons flat;
        flatButtons(flat);

        // The states the rules tell apart, and the ones they do not (pressed, focused), alone and with the others
        const QStyle::State e = QStyle::State_Enabled;
        const QStyle::State h = QStyle::State_MouseOver;
        const QStyle::State s = QStyle::State_Sunken;
        const QStyle::State f = QStyle::State_HasFocus;
        const QStyle::State on = QStyle::State_On;
        const QStyle::State states[] = { e, e | h, e | h | s, e | s, e | f, e | h | f, e | on, e | on | h, e | on | h | s, e | on | s,
                                         QStyle::State_None, on };
        const std::vector<std::pair<const QPushButton*, const QPushButton*>> buttons{
            { legacy.m_details, flat.m_details },
            { legacy.m_sceneSettings, flat.m_sceneSettings },
        };
        for (const qreal ratio : { 1.0, 1.5, 2.0 })
        {
            for (const auto& [legacyButton, flatButton] : buttons)
            {
                for (const QStyle::State state : states)
                {
                    EXPECT_EQ(AssetBrowserFlat::firstDifference(
                                  AssetBrowserFlat::paintSwitchButton(*legacyButton, state, ratio),
                                  AssetBrowserFlat::paintSwitchButton(*flatButton, state, ratio)),
                        std::string())
                        << legacyButton->text().toStdString() << " in state " << int(state) << " at ratio " << ratio;
                }
            }
        }
    }

    TEST_F(AssetBrowserFlatSheetTests, Rules_TheButtonsPaintWhatTheRulesThatSurvivedTheParserPaintInBothGuis)
    {
        AssetBrowserFlat::SwitchButtons legacy;
        legacyButtons(legacy);
        AssetBrowserFlat::SwitchButtons flat;
        flatButtons(flat);

        const QStyle::State e = QStyle::State_Enabled;
        const QStyle::State h = QStyle::State_MouseOver;
        const QStyle::State s = QStyle::State_Sunken;
        const QStyle::State on = QStyle::State_On;
        const QRgb black = 0x000000;

        // Both GUIs, the legacy one first: the same facts hold of the element
        const std::vector<std::pair<const AssetBrowserFlat::SwitchButtons*, const char*>> guis{ { &legacy, "legacy" }, { &flat, "flat" } };
        for (const auto& [pair, gui] : guis)
        {
            const auto at = [](const QPushButton& button, QStyle::State state, int x, int y)
            {
                return AssetBrowserFlat::paintSwitchButton(button, state, 1.0).pixel(x, y) & 0xFFFFFF;
            };
            const QPushButton& details = *pair->m_details;
            const QPushButton& sceneSettings = *pair->m_sceneSettings;

            // The points the fill and the side edges are read at lie clear of the label, whatever font the run falls back to: the label
            // is centred on the middle rows of the 30 px button and may be wider than the box (Scene Settings in a fallback font is).
            // Row 4 is inside the 28 px of contents, below the top edge and the corner arcs; row 5 of a side edge is below the arcs
            constexpr int middleX = 50;
            constexpr int fillY = 4;
            constexpr int sideY = 5;
            const auto fillOf = [&at](const QPushButton& button, QStyle::State state)
            {
                return at(button, state, middleX, fillY);
            };

            // The fill of each state
            EXPECT_EQ(fillOf(details, e), 0x333333u) << gui;
            EXPECT_EQ(fillOf(details, e | h), 0x444444u) << gui;                    // `:hover:!checked`
            EXPECT_EQ(fillOf(details, e | h | s), 0x444444u) << gui;                // `:pressed:!checked:` never existed: the hover fill
            EXPECT_EQ(fillOf(details, e | s), 0x333333u) << gui;                    // pressed with the pointer away: no rule of its own
            EXPECT_EQ(fillOf(details, e | on), 0x1E70EBu) << gui;                   // `:checked`
            EXPECT_EQ(fillOf(details, e | on | h), 0x1E70EBu) << gui;               // which outranks `:hover:!checked`
            EXPECT_EQ(fillOf(details, QStyle::State_None), 0x333333u) << gui;       // disabled: no rule of its own
            EXPECT_EQ(fillOf(sceneSettings, e), 0x333333u) << gui;
            EXPECT_EQ(fillOf(sceneSettings, e | on), 0x1E70EBu) << gui;

            // The edges: not the #222222 the sheet meant (`border-color #222222` has no colon and is dropped), not the colour of the
            // text either, but the invalid colour `border: none` of `QPushButton:flat` left, which is drawn black
            EXPECT_EQ(at(details, e, 0, sideY), black) << gui << ", left";
            EXPECT_EQ(at(details, e, middleX, 0), black) << gui << ", top";
            EXPECT_EQ(at(details, e, middleX, 29), black) << gui << ", bottom";
            EXPECT_EQ(at(details, e | on, middleX, 0), black) << gui << ", top, checked";
            EXPECT_EQ(at(details, QStyle::State_None, middleX, 0), black) << gui << ", top, disabled";
            EXPECT_EQ(at(sceneSettings, e, 99, sideY), black) << gui << ", right";

            // The corners: the first is round on the left and square on the right, the second round at the top right and square at
            // the bottom right (a pixel that is round is not the colour of the edge)
            EXPECT_EQ(at(details, e, 99, 0), black) << gui << ", Details, top right is square";
            EXPECT_EQ(at(details, e, 99, 29), black) << gui << ", Details, bottom right is square";
            EXPECT_NE(at(details, e, 0, 0), black) << gui << ", Details, top left is round";
            EXPECT_NE(at(details, e, 0, 29), black) << gui << ", Details, bottom left is round";
            EXPECT_NE(at(sceneSettings, e, 99, 0), black) << gui << ", Scene Settings, top right is round";
            EXPECT_EQ(at(sceneSettings, e, 99, 29), black) << gui << ", Scene Settings, bottom right is square";
        }
    }

    // ----------------------------------------------------------------------------------
    // `vertical-align: top` of the left pane (AzAssetBrowserWindow.ui:457, AssetPickerDialog.ui:102): no look to carry
    // ----------------------------------------------------------------------------------

    TEST_F(AssetBrowserFlatSheetTests, VerticalAlign_TheSheetOfTheLeftPaneChangesNothingTheLegacyGuiDrawsInIt)
    {
        // The pane as the form has it: a widget with a tree, a label and a divider in it. The same pane with and without the sheet,
        // under the rules of the application's that reach them, drawn at the ratios the GUI runs at.
        expectTheLeftPaneSheetChangesNothing(
            m_fusion.get(), AssetBrowserFlat::SheetCall::SetStyleSheet, AssetBrowserFlat::legacyPaneRules());
    }

    TEST_F(AssetBrowserFlatSheetTests, VerticalAlign_TheFlatGuiSetsNoSheetAndTheLeftPaneDrawsWhatTheLegacyPaneDraws)
    {
        // The flattened GUI sets no sheet: the form's call (StyleManager::setLegacyStyleSheet) leaves the pane as it is without the
        // call, at the ratios the GUI runs at, and puts nothing under Qt's style sheet style. This is not a pane under Qt's style
        // sheet style over the flat style with a sheet that has no look: that is neither GUI (Qt's default sheet gives a header
        // section a rule of its own, so QStyleSheetStyle::drawControl (CE_Header) draws the section itself and hands its bevel to the
        // base style, Fusion's gradient with its light top line, where the flat style draws the section's solid fill: the pane read
        // #505050 at the top left of the header, not #2d2d2d).
        expectTheLeftPaneSheetChangesNothing(m_style.get(), AssetBrowserFlat::SheetCall::LegacyHelper, QString());

        // And what that pane draws is what the legacy pane draws. The one part of the pane the sheet's reach could change is a
        // header (the tree of the twins has the one a tree has by default, which the Editor's tree views hide and its list view
        // shows): the legacy pane is the one with the sheet, under the application's rules, and its header section is the rule's
        // solid fill, at the top left of the header and beside the text of the section.
        for (const qreal ratio : { 1.0, 1.5, 2.0 })
        {
            const AssetBrowserFlat::LeftPaneLook flat = drawLeftPane(
                m_style.get(), AssetBrowserFlat::SheetCall::LegacyHelper, QString(), ratio);
            const AssetBrowserFlat::LeftPaneLook legacy = drawLeftPane(
                m_fusion.get(), AssetBrowserFlat::SheetCall::SetStyleSheet, AssetBrowserFlat::legacyPaneRules(), ratio);
            ASSERT_FALSE(flat.m_header.isEmpty()) << "at ratio " << ratio;
            ASSERT_FALSE(legacy.m_header.isEmpty()) << "at ratio " << ratio;

            const auto expectTheRulesFill = [&](const char* where, const QPoint& flatPoint, const QPoint& legacyPoint)
            {
                const QRgb flatColor = AssetBrowserFlat::colorAt(flat.m_image, flatPoint, ratio);
                const QRgb legacyColor = AssetBrowserFlat::colorAt(legacy.m_image, legacyPoint, ratio);
                EXPECT_EQ(AssetBrowserFlat::hexOf(legacyColor), AssetBrowserFlat::hexOf(AssetBrowserFlat::SectionFill))
                    << "legacy, " << where << ", at ratio " << ratio;
                EXPECT_EQ(AssetBrowserFlat::hexOf(flatColor), AssetBrowserFlat::hexOf(legacyColor))
                    << "flat against legacy, " << where << ", at ratio " << ratio;
            };
            expectTheRulesFill("the top left of the header", QPoint(0, 0), QPoint(0, 0));
            expectTheRulesFill(
                "beside the text of the section",
                QPoint(flat.m_header.right() - 8, flat.m_header.center().y()),
                QPoint(legacy.m_header.right() - 8, legacy.m_header.center().y()));
        }
    }
} // namespace UnitTest
