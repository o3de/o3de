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
#include <AzQtComponents/Components/Elements/FrameElement.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/Widgets/AssetFolderThumbnailView.h>
#include <EditorFlatSheet.h>
#include <EditorFlatValues.h>

#include <QAbstractScrollArea>
#include <QApplication>
#include <QFrame>
#include <QImage>
#include <QPainter>
#include <QStandardItemModel>
#include <QStyleFactory>
#include <QStyleOption>
#include <QWidget>

#include <string>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The elements of the sheets AzAssetBrowserWindow.ui gave its widgets (the two dividers by the Create button, the thumbnails, and the
    // two that need none): which widgets they reach, and that they paint what Qt's own style sheet style painted, at the device ratios
    // the GUI runs at
    // ----------------------------------------------------------------------------------

    namespace AzAssetBrowserFlatSheetTestsInternal
    {
        //! A widget that Qt takes for an instance of a class it is not. The matchers ask for the asset browser window and its thumbnail
        //! view by their class names, as the selectors do, and building the real ones needs the Editor's whole asset browser.
        class NamedClassWidget
            : public QWidget
        {
        public:
            NamedClassWidget(const char* className, QWidget* parent = nullptr)
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

        constexpr const char* WindowClass = "AzAssetBrowserWindow";
        constexpr const char* ThumbnailViewClass = "AzToolsFramework::AssetBrowser::AssetBrowserThumbnailView";
        constexpr const char* DividerElementName = "Editor.Frame.AssetBrowserCreateDivider";
        constexpr const char* GlobalFrameName = "Global.Frame.Panel";
        constexpr const char* ThumbnailsElementName = "Editor.Box.AssetBrowserThumbnails";

        // The sheets the form gave its widgets (AzAssetBrowserWindow.ui:132, :206, :357, :567), as they were before the flattening
        constexpr const char* CreateDividerSheet = "QFrame { color : rgb(34, 34, 34);}";
        constexpr const char* LastDividerSheet = "QFrame { color : white;}";
        constexpr const char* ThumbnailViewSheet =
            "AzToolsFramework--AssetBrowser--AssetBrowserThumbnailView, AzQtComponents--AssetFolderThumbnailView "
            "{ background-color: #333333; }";

        // The rules of the application's sheet that reach those widgets, with the lines they are on
        // BaseStyleSheet.qss:15-21, the universal rule
        constexpr const char* UniversalRule =
            "* { background-color: transparent; color: white; font-family: \"Open Sans\"; font-weight: 400; }";
        // BaseStyleSheet.qss:41-48, the fill of every divider, box and panel a QFrame draws itself
        constexpr const char* PanelRule = ".QFrame[frameShape=\"1\"], .QFrame[frameShape=\"2\"], .QFrame[frameShape=\"4\"],"
                                          " .QFrame[frameShape=\"5\"], .QFrame[frameShape=\"6\"] { background-color: #444444; }";
        // QDockWidget.qss:33-36, the margin of every scroll area
        constexpr const char* ScrollAreaRule = "QAbstractScrollArea { margin: 0; }";
        // ScrollBar.qss:69-72, the background of the viewport of every scroll area (and of its scroll bars' containers): none
        constexpr const char* ViewportRule = "QAbstractScrollArea > .QWidget { background: none; }";

        constexpr QRgb ParentColor = 0x808080;          // what neither the sheet nor the element draws stays this grey
        constexpr QRgb PanelFill = 0x444444;            // the fill behind a divider's line (BaseStyleSheet.qss:47)

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

        //! What decides whether the viewport of a scroll area covers the area's own fill: its autofill, and its styled background
        struct ViewportFacts
        {
            bool m_autoFill = false;
            bool m_styledBackground = false;
        };

        ViewportFacts viewportFactsOf(const QAbstractScrollArea& area)
        {
            return ViewportFacts{ area.viewport()->autoFillBackground(), area.viewport()->testAttribute(Qt::WA_StyledBackground) };
        }

        //! What a scroll area and its viewport paint together, in the order Qt paints them (the parent, then its child): the area's own
        //! background and frame, then the viewport's background over the area's contents, which is the rect a scroll area gives its
        //! viewport (QWidgetPrivate::paintBackground: the autofill in the background role, then PE_Widget of a styled background). The
        //! viewport is the whole interior of the view, the surface its items are painted on: this is the view without its items.
        QImage paintScrollArea(const QAbstractScrollArea& area, qreal ratio)
        {
            QImage image = paintFrame(area, ratio);
            const QWidget* viewport = area.viewport();
            QPainter painter(&image);
            painter.translate(viewport->pos());
            if (viewport->autoFillBackground())
            {
                painter.fillRect(viewport->rect(), viewport->palette().brush(viewport->backgroundRole()));
            }
            if (viewport->testAttribute(Qt::WA_StyledBackground))
            {
                QStyleOption background;
                background.initFrom(viewport);
                viewport->style()->drawPrimitive(QStyle::PE_Widget, &background, &painter, viewport);
            }
            painter.end();
            return image;
        }

        //! The first pixel in a rect of an image that is not the colour asked for, as text for a failure message ("" when none)
        std::string firstPixelOtherThan(const QImage& image, const QRect& rect, QRgb colour)
        {
            for (int y = rect.top(); y <= rect.bottom(); ++y)
            {
                for (int x = rect.left(); x <= rect.right(); ++x)
                {
                    if ((image.pixel(x, y) & 0xFFFFFF) != colour)
                    {
                        return "at (" + std::to_string(x) + ", " + std::to_string(y) + "): #"
                            + QString::number(image.pixel(x, y) & 0xFFFFFF, 16).toStdString();
                    }
                }
            }
            return "";
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

        //! A divider as the form has it: a Box frame, Plain, line width 1, 1px by 22px
        QFrame* makeDivider(const char* name, QWidget* parent)
        {
            auto* divider = new QFrame(parent);
            divider->setObjectName(QString::fromLatin1(name));
            divider->setFrameShape(QFrame::Box);
            divider->setFrameShadow(QFrame::Plain);
            divider->setLineWidth(1);
            divider->setMidLineWidth(0);
            divider->resize(1, 22);
            return divider;
        }

        class AzAssetBrowserFlatSheetTests
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

            //! The flat way: the style on the widget, and the elements resolved as Style::polish does in the flattened GUI
            void flatten(QWidget& widget)
            {
                widget.setStyle(m_style.get());
                AzQtComponents::FrameElement::resolve(&widget);
                AzQtComponents::BoxElement::resolve(&widget);
            }

            static const char* frameElementOf(const QWidget& widget)
            {
                const AzQtComponents::FrameElement* element = AzQtComponents::FrameElement::forWidget(&widget);
                return element ? element->m_name : "";
            }

            static const char* boxElementOf(const QWidget& widget)
            {
                const AzQtComponents::BoxElement* element = AzQtComponents::BoxElement::forWidget(&widget);
                return element ? element->m_name : "";
            }

            //! Fusion on a widget and everything in it, set on each so that the style of the screen the test runs on is not their base;
            //! before any sheet, as the legacy GUI's base style was there before the sheets
            void useFusion(QWidget& root) const
            {
                root.setStyle(m_fusion.get());
                for (QWidget* child : root.findChildren<QWidget*>())
                {
                    child->setStyle(m_fusion.get());
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

            //! The contents box of a frame (SE_ShapedFrameContents): the rect inside its margin, border and padding
            static QRect frameContents(QWidget& widget)
            {
                const QStyleOptionFrame option = frameOption(qobject_cast<QFrame*>(&widget));
                return widget.style()->subElementRect(QStyle::SE_ShapedFrameContents, &option, &widget);
            }

            //! A divider as the legacy GUI had it: the sheet the form gave it, under the rules of the application's. Its contents box too.
            QImage legacyDivider(const char* name, const char* sheet, qreal ratio, QRect* contents)
            {
                QWidget root;
                QFrame* divider = makeDivider(name, &root);
                useFusion(root);
                divider->setStyleSheet(QString::fromLatin1(sheet));
                applyApplicationRules(root, QString::fromLatin1(UniversalRule) + QString::fromLatin1(PanelRule));
                *contents = frameContents(*divider);
                return paintFrame(*divider, ratio);
            }

            //! The same divider in the flattened GUI: below the asset browser window, with its element and no sheet
            QImage flatDivider(const char* name, qreal ratio, QRect* contents)
            {
                NamedClassWidget window(WindowClass);
                QFrame* divider = makeDivider(name, &window);
                flatten(*divider);
                *contents = frameContents(*divider);
                return paintFrame(*divider, ratio);
            }

            //! The thumbnails as the legacy GUI had them: the sheet the form gave the view, under the rules of the application's. The
            //! image is the area with its viewport; the facts are the viewport's, as the sheet style's polish left them.
            QImage legacyThumbnails(qreal ratio, QRect* contents, ViewportFacts* viewport = nullptr)
            {
                QWidget root;
                auto* view = new QWidget(&root);
                auto* thumbnails = new AzQtComponents::AssetFolderThumbnailView(view);
                thumbnails->resize(100, 60);
                useFusion(root);
                view->setStyleSheet(QString::fromLatin1(ThumbnailViewSheet));
                applyApplicationRules(
                    root, QString::fromLatin1(UniversalRule) + QString::fromLatin1(ScrollAreaRule) + QString::fromLatin1(ViewportRule));
                *contents = frameContents(*thumbnails);
                thumbnails->viewport()->setGeometry(*contents);         // a scroll area lays its viewport out in its frame's contents
                if (viewport)
                {
                    *viewport = viewportFactsOf(*thumbnails);
                }
                return paintScrollArea(*thumbnails, ratio);
            }

            //! The same view in the flattened GUI: in the thumbnail view of an asset browser window, with its element and no sheet. The
            //! viewport has the style too: in the application every widget has it, and a style set on a widget does not reach its children.
            QImage flatThumbnails(qreal ratio, QRect* contents, ViewportFacts* viewport = nullptr)
            {
                NamedClassWidget window(WindowClass);
                NamedClassWidget view(ThumbnailViewClass, &window);
                view.setObjectName(QStringLiteral("m_thumbnailView"));
                auto* thumbnails = new AzQtComponents::AssetFolderThumbnailView(&view);
                thumbnails->resize(100, 60);
                flatten(*thumbnails);
                thumbnails->viewport()->setStyle(m_style.get());
                *contents = frameContents(*thumbnails);
                thumbnails->viewport()->setGeometry(*contents);
                if (viewport)
                {
                    *viewport = viewportFactsOf(*thumbnails);
                }
                return paintScrollArea(*thumbnails, ratio);
            }

            QString m_applicationSheet;
            AZStd::unique_ptr<AzQtComponents::Style> m_style;
            AZStd::unique_ptr<QStyle> m_fusion;
        };
    } // namespace AzAssetBrowserFlatSheetTestsInternal

    // A unity build joins this file with others of namespace UnitTest (test_EditorFlatSheet has a ClassNamedWidget of its own): the
    // fixture is the only name brought in, the rest is named through the alias
    using AzAssetBrowserFlatSheetTestsInternal::AzAssetBrowserFlatSheetTests;
    namespace AssetBrowserWindowFlat = AzAssetBrowserFlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // The dividers (framePreCreate, framePostCreate, frame)
    // ----------------------------------------------------------------------------------

    TEST_F(AzAssetBrowserFlatSheetTests, Registry_TheDividersByTheCreateButtonAreFoundOnlyBelowTheAssetBrowserWindow)
    {
        AssetBrowserWindowFlat::NamedClassWidget window(AssetBrowserWindowFlat::WindowClass);
        QFrame* pre = AssetBrowserWindowFlat::makeDivider("framePreCreate", &window);
        QFrame* post = AssetBrowserWindowFlat::makeDivider("framePostCreate", &window);
        QFrame* last = AssetBrowserWindowFlat::makeDivider("frame", &window);
        QFrame* otherName = AssetBrowserWindowFlat::makeDivider("framePreCreate2", &window);

        // The same names in a window that is not the asset browser's
        QWidget elsewhere;
        QFrame* strayPre = AssetBrowserWindowFlat::makeDivider("framePreCreate", &elsewhere);
        QFrame* strayPost = AssetBrowserWindowFlat::makeDivider("framePostCreate", &elsewhere);

        for (QFrame* divider : { pre, post, last, otherName, strayPre, strayPost })
        {
            flatten(*divider);
        }

        EXPECT_STREQ(frameElementOf(*pre), AssetBrowserWindowFlat::DividerElementName);
        EXPECT_STREQ(frameElementOf(*post), AssetBrowserWindowFlat::DividerElementName);

        // The last divider's sheet is the colour of the universal rule: it has the Global frame, as has any other frame
        EXPECT_STREQ(frameElementOf(*last), AssetBrowserWindowFlat::GlobalFrameName);
        EXPECT_STREQ(frameElementOf(*otherName), AssetBrowserWindowFlat::GlobalFrameName);
        EXPECT_STREQ(frameElementOf(*strayPre), AssetBrowserWindowFlat::GlobalFrameName);
        EXPECT_STREQ(frameElementOf(*strayPost), AssetBrowserWindowFlat::GlobalFrameName);

        // A frame element draws its widget's background itself, in the frame's own paint: no styled background
        EXPECT_FALSE(pre->testAttribute(Qt::WA_StyledBackground));
    }

    TEST_F(AzAssetBrowserFlatSheetTests, Pixels_EachDividerPaintsWhatItsSheetPaintedAtEveryRatio)
    {
        struct Divider
        {
            const char* m_name;
            const char* m_sheet;
        };
        for (const Divider& divider : { Divider{ "framePreCreate", AssetBrowserWindowFlat::CreateDividerSheet },
                                        Divider{ "framePostCreate", AssetBrowserWindowFlat::CreateDividerSheet },
                                        Divider{ "frame", AssetBrowserWindowFlat::LastDividerSheet } })
        {
            for (const qreal ratio : { 1.0, 1.5, 2.0 })
            {
                QRect legacyContents;
                QRect flatContents;
                const QImage legacy = legacyDivider(divider.m_name, divider.m_sheet, ratio, &legacyContents);
                const QImage flat = flatDivider(divider.m_name, ratio, &flatContents);
                EXPECT_EQ(AssetBrowserWindowFlat::firstDifference(legacy, flat), std::string())
                    << divider.m_name << " at ratio " << ratio;
                EXPECT_EQ(flatContents, legacyContents) << divider.m_name << " at ratio " << ratio;
            }
        }
    }

    TEST_F(AzAssetBrowserFlatSheetTests, Pixels_TheDividersAreTheColourOfTheirLineAllTheWayDownAndAcross)
    {
        // The frame is one pixel wide and 22 high, and a Box frame of line width 1 draws its line down it: the colour is the sheet's,
        // whatever the fill behind it is. Qt draws that line as the outline of a rect without a width (qDrawPlainRect), and at ratio 1
        // its raster engine strokes it with the fast pen (QRasterPaintEngine::penChanged: a pen of width 1 at scale 1), which leaves the
        // first row to the fill. The legacy GUI's Qt does the same, and the twin above, which compares the whole frame, passes: row 0 is
        // the line or the fill (anything else is a fault), every other row is the line
        QRect contents;
        const QImage pre = flatDivider("framePreCreate", 1.0, &contents);
        const QImage last = flatDivider("frame", 1.0, &contents);

        ASSERT_EQ(pre.size(), QSize(1, 22));
        ASSERT_EQ(last.size(), QSize(1, 22));
        const auto expectLine = [](const QImage& image, QRgb line)
        {
            const QRgb firstRow = image.pixel(0, 0) & 0xFFFFFF;
            EXPECT_TRUE(firstRow == line || firstRow == AssetBrowserWindowFlat::PanelFill)
                << "row 0: #" << QString::number(firstRow, 16).toStdString();
            for (int y = 1; y < 22; ++y)
            {
                EXPECT_EQ(image.pixel(0, y) & 0xFFFFFF, line) << "row " << y;
            }
        };
        expectLine(pre, 0x222222u);
        expectLine(last, 0xFFFFFFu);
    }

    // ----------------------------------------------------------------------------------
    // The thumbnails (m_thumbnailView)
    // ----------------------------------------------------------------------------------

    TEST_F(AzAssetBrowserFlatSheetTests, Registry_TheThumbnailsAreTheAssetFolderViewInTheThumbnailViewOfTheAssetBrowserWindow)
    {
        QStandardItemModel model;

        // The thumbnails in the thumbnail view the window holds
        AssetBrowserWindowFlat::NamedClassWidget window(AssetBrowserWindowFlat::WindowClass);
        AssetBrowserWindowFlat::NamedClassWidget host(AssetBrowserWindowFlat::ThumbnailViewClass, &window);
        host.setObjectName(QStringLiteral("m_thumbnailView"));
        auto* thumbnails = new AzQtComponents::AssetFolderThumbnailView(&host);
        thumbnails->setModel(&model);

        // The same view in another view of that class (another name), in a widget of that name that is not of that class, in a thumbnail
        // view outside the asset browser window, and on its own
        AssetBrowserWindowFlat::NamedClassWidget otherHost(AssetBrowserWindowFlat::ThumbnailViewClass, &window);
        otherHost.setObjectName(QStringLiteral("m_otherThumbnailView"));
        auto* inOtherHost = new AzQtComponents::AssetFolderThumbnailView(&otherHost);
        QWidget plainHost(&window);
        plainHost.setObjectName(QStringLiteral("m_thumbnailView"));
        auto* inPlainHost = new AzQtComponents::AssetFolderThumbnailView(&plainHost);
        AssetBrowserWindowFlat::NamedClassWidget strayHost(AssetBrowserWindowFlat::ThumbnailViewClass);
        strayHost.setObjectName(QStringLiteral("m_thumbnailView"));
        auto* inStrayHost = new AzQtComponents::AssetFolderThumbnailView(&strayHost);
        AzQtComponents::AssetFolderThumbnailView alone;
        for (AzQtComponents::AssetFolderThumbnailView* view : { inOtherHost, inPlainHost, inStrayHost, &alone })
        {
            view->setModel(&model);
        }

        for (AzQtComponents::AssetFolderThumbnailView* view : { thumbnails, inOtherHost, inPlainHost, inStrayHost, &alone })
        {
            flatten(*view);
        }

        EXPECT_STREQ(boxElementOf(*thumbnails), AssetBrowserWindowFlat::ThumbnailsElementName);
        EXPECT_TRUE(thumbnails->testAttribute(Qt::WA_StyledBackground));      // the fill is drawn at PE_Widget
        EXPECT_STRNE(boxElementOf(*inOtherHost), AssetBrowserWindowFlat::ThumbnailsElementName);
        EXPECT_STRNE(boxElementOf(*inPlainHost), AssetBrowserWindowFlat::ThumbnailsElementName);
        EXPECT_STRNE(boxElementOf(*inStrayHost), AssetBrowserWindowFlat::ThumbnailsElementName);
        EXPECT_STRNE(boxElementOf(alone), AssetBrowserWindowFlat::ThumbnailsElementName);
    }

    TEST_F(AzAssetBrowserFlatSheetTests, Pixels_TheThumbnailsPaintWhatTheSheetPaintedAtEveryRatio)
    {
        // The fill over the whole area, as the sheet's background rule filled it, and the native frame of a QFrame over it; and the
        // viewport on top, which is the whole interior of the view
        for (const qreal ratio : { 1.0, 1.5, 2.0 })
        {
            QRect legacyContents;
            QRect flatContents;
            const QImage legacy = legacyThumbnails(ratio, &legacyContents);
            const QImage flat = flatThumbnails(ratio, &flatContents);
            EXPECT_EQ(AssetBrowserWindowFlat::firstDifference(legacy, flat), std::string()) << "at ratio " << ratio;
            EXPECT_EQ(flatContents, legacyContents) << "at ratio " << ratio;
        }
    }

    TEST_F(AzAssetBrowserFlatSheetTests, Box_TheThumbnailsKeepTheNativeFrameBoxOfAQFrame)
    {
        // The margin of 0 every scroll area has (QDockWidget.qss:33-36) and the frame Qt's default sheet gave every QFrame: the contents
        // sit one pixel in from the edge of the area (the Editor capture: contents [1, 1, 1, 1])
        QRect legacyContents;
        QRect flatContents;
        legacyThumbnails(1.0, &legacyContents);
        flatThumbnails(1.0, &flatContents);

        EXPECT_EQ(flatContents, legacyContents);
        EXPECT_EQ(flatContents, QRect(1, 1, 98, 58));          // the area is 100 x 60
    }

    TEST_F(AzAssetBrowserFlatSheetTests, Pixels_TheThumbnailsAreFilledWithTheSheetsColourInsideTheirFrame)
    {
        QRect contents;
        const QImage flat = flatThumbnails(1.0, &contents);

        EXPECT_EQ(flat.pixel(50, 30) & 0xFFFFFF, 0x333333u);
        EXPECT_EQ(flat.pixel(6, 6) & 0xFFFFFF, 0x333333u);
        EXPECT_EQ(flat.pixel(93, 53) & 0xFFFFFF, 0x333333u);
    }

    TEST_F(AzAssetBrowserFlatSheetTests, Viewport_TheThumbnailsViewportShowsTheFillOfTheAreaInBothGuis)
    {
        // The viewport is the whole interior of the view, the surface its items are painted on. The sheet took its autofill away and
        // the application's `QAbstractScrollArea > .QWidget { background: none }` (ScrollBar.qss:69-72) gave it no background, so
        // the area's fill shows through it. The flat viewport has to give up its autofill as well (the flattened application palette's
        // Base is #444444, which an autofill would paint over the fill) and to draw no background of its own over the fill.
        QRect legacyContents;
        QRect flatContents;
        AssetBrowserWindowFlat::ViewportFacts legacyViewport;
        AssetBrowserWindowFlat::ViewportFacts flatViewport;
        const QImage legacy = legacyThumbnails(1.0, &legacyContents, &legacyViewport);
        const QImage flat = flatThumbnails(1.0, &flatContents, &flatViewport);

        EXPECT_FALSE(flatViewport.m_autoFill);
        EXPECT_TRUE(flatViewport.m_styledBackground);
        EXPECT_EQ(flatViewport.m_autoFill, legacyViewport.m_autoFill);
        EXPECT_EQ(flatViewport.m_styledBackground, legacyViewport.m_styledBackground);

        // Every pixel of the interior is the fill, as the viewport leaves it
        ASSERT_EQ(flatContents, QRect(1, 1, 98, 58));
        EXPECT_EQ(AssetBrowserWindowFlat::firstPixelOtherThan(legacy, legacyContents, 0x333333u), std::string()) << "legacy";
        EXPECT_EQ(AssetBrowserWindowFlat::firstPixelOtherThan(flat, flatContents, 0x333333u), std::string()) << "flat";
    }

    // ----------------------------------------------------------------------------------
    // One value for both GUIs: the sheets the code sets and the elements read the same colours (EditorFlatValues.h)
    // ----------------------------------------------------------------------------------

    TEST_F(AzAssetBrowserFlatSheetTests, Values_TheSheetsTheCodeSetsAreTheFormsTextByteForByteAndTheElementsReadTheSameColours)
    {
        // The code writes the text of each sheet from the colour its element reads: it is the text the form had
        EXPECT_EQ(EditorFlatValues::AssetBrowserCreateDividerSheet(), QString::fromLatin1(AssetBrowserWindowFlat::CreateDividerSheet));
        EXPECT_EQ(EditorFlatValues::AssetBrowserThumbnailViewSheet(), QString::fromLatin1(AssetBrowserWindowFlat::ThumbnailViewSheet));

        // The divider's line and the thumbnails' fill are the constants the text is written from
        AssetBrowserWindowFlat::NamedClassWidget window(AssetBrowserWindowFlat::WindowClass);
        QFrame* divider = AssetBrowserWindowFlat::makeDivider("framePreCreate", &window);
        flatten(*divider);
        const AzQtComponents::FrameElement* dividerElement = AzQtComponents::FrameElement::forWidget(divider);
        ASSERT_TRUE(dividerElement != nullptr);
        EXPECT_STREQ(dividerElement->m_name, AssetBrowserWindowFlat::DividerElementName);
        EXPECT_TRUE(dividerElement->m_foreground == EditorFlatValues::AssetBrowserCreateDividerLine);

        AssetBrowserWindowFlat::NamedClassWidget view(AssetBrowserWindowFlat::ThumbnailViewClass, &window);
        view.setObjectName(QStringLiteral("m_thumbnailView"));
        auto* thumbnails = new AzQtComponents::AssetFolderThumbnailView(&view);
        flatten(*thumbnails);
        const AzQtComponents::BoxElement* thumbnailsElement = AzQtComponents::BoxElement::forWidget(thumbnails);
        ASSERT_TRUE(thumbnailsElement != nullptr);
        EXPECT_STREQ(thumbnailsElement->m_name, AssetBrowserWindowFlat::ThumbnailsElementName);
        EXPECT_TRUE(thumbnailsElement->m_idle.m_fill.color() == EditorFlatValues::AssetBrowserThumbnailsFill);
    }
} // namespace UnitTest
