/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/FrameElement.h>
#include <AzQtComponents/Components/Elements/IndicatorElement.h>
#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Elements/ToolTipElement.h>
#include <AzQtComponents/Tests/ToolTipTwins.h>
#include <FlatElements.h>
#include <GemCatalog/GemInspector.h>
#include <GemCatalog/GemListHeaderWidget.h>
#include <GemCatalog/GemModel.h>
#include <GemCatalog/GemSortFilterProxyModel.h>
#include <LinkWidget.h>

#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QCompleter>
#include <QCoreApplication>
#include <QDir>
#include <QEnterEvent>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontInfo>
#include <QFrame>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QList>
#include <QListView>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QScopedPointer>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QStyleFactory>
#include <QStyleOption>
#include <QStylePainter>
#include <QVBoxLayout>
#include <QWidget>
#include <QtMath>

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

// =====================================================================================================================
// The inline sheets of the Project Manager's code against the elements that carry them in the flattened GUI (FlatElements.cpp,
// "Inline sheets of the code").
//
// The legacy twin of a widget is the real widget built in the legacy rung (so it sets the sheet its code sets) under a style sheet
// style over Fusion, with the rules of the application's sheets that reach it, quoted below with their file:line, laid on its root
// (an inline sheet of the lowest depth: Qt gives a widget's own sheet and its nearer ancestors' precedence over it, as it gives
// every inline sheet over the application's). The flat twin is the same widget built in the flattened rung (so its code sets no
// sheet) under the flattened style over Fusion, polished as the application polishes it. Both are painted at the device pixel
// ratios the GUI runs at, and their geometry, fonts and palette roles are compared.
//
// The twins of the gem list's header are the exception to "over Fusion": the header is laid out by layouts with their default spacing
// and holds push buttons, and the legacy GUI's sheet style sat on AzQtComponents::Style (StyleManager::initialize: QStyleSheetStyle ->
// Style -> Fusion), which answers a layout spacing of 3 where Fusion's is 6 (Style.cpp, PM_LayoutHorizontalSpacing) and gives every push
// button a maximum height (PushButton::polish). A header twin on Fusion alone lays out differently from the legacy GUI (its count and
// refresh button sit 3px left and 2px higher than in the captures, which show the two GUIs alike). So the legacy header sits on Style,
// which answers by the rung of the process: each header twin is built, measured and painted in its own rung, and the two are compared
// by what was taken (HeaderFacts). A tree that was never shown is laid out top down (layOutTree), as its first show lays it out.
//
// The popup of the tags field's completer is the twin of a popup the test makes as FormLineEditTagsWidget makes it, as that widget
// needs the resources of the whole window. The twins set a copy of ProjectManagerCompleterPopup.qss; a test reads the file and
// compares it with the copy.
//
// The links are twinned in both of their states, idle and under the pointer (LinkWidget.cpp:56 is the sheet of the second, with the
// underline), and so are the tooltips of a link: the sheet had no selector, so it reached the tip a link shows, in the state the link
// is in when the pointer rests on it.
//
// The tag chips of the tags field and the progress bars are twinned under the rules of ProjectManager.qss itself (:232-247, :772-780)
// and the rules of the application's sheets that reach them (BaseStyleSheet.qss, CheckBox.qss, ProgressBar.qss), all quoted below. The
// legacy twin has the application's rules on a root widget and the Project Manager's on a widget below it that stands for the window
// (Application.cpp:246-249): the sheet of the window is deeper than the application's, and depth outranks specificity, which the
// image of the chip's indicator relies on. The flat twin is the same tree under the flattened style with the module's elements. The
// mark of a chip is compared in every state of the check box, painted through the style as QCheckBox::paintEvent paints it.
// =====================================================================================================================

namespace O3DE::ProjectManager
{
    namespace FlatSheetTestsInternal
    {
        using Rung = AzQtComponents::GuiPerfFloor::Rung;

        // ------------------------------------------------------------------------------
        // The rules of the application's sheets that reach the twins
        // ------------------------------------------------------------------------------

        //! The universal rule (BaseStyleSheet.qss:15-21), the dividers (:41-48), the label (Text.qss:89-95) and the push button
        //! (PushButton.qss:30-52) rules, and the rules of ProjectManager.qss for the widgets of the gem list's header: the divider's
        //! colour (:481-483), the refresh button (:824-835), the count (:876-880), the text of a tag (:914-917)
        constexpr const char* HeaderRules = R"sheet(
            * { background-color: transparent; color: white; font-family: "Open Sans"; font-weight: 400; }
            .QFrame[frameShape="1"], .QFrame[frameShape="2"], .QFrame[frameShape="4"], .QFrame[frameShape="5"],
            .QFrame[frameShape="6"] { background-color: #444444; }
            QLabel { font-family: "Open Sans"; font-size: 12px; margin-top: 1px; margin-bottom: 1px; }
            QPushButton { color: #ffffff; font-family: "Open Sans"; font-size: 12px; }
            QPushButton:hover, QPushButton:pressed { color: #FFFFFF; }
            QPushButton:disabled { color: #999999; }
            QPushButton:flat { border: none; background-color: transparent; }
            #horizontalSeparatingLine { color: #666666; }
            #RefreshButton { background:transparent url(:/Refresh.svg) no-repeat center center; qproperty-flat: true;
                max-width:24px; min-width:24px; max-height:24px; min-height:24px; }
            #RefreshButton:hover { background:transparent url(:/Refresh_Hover.svg) no-repeat center center; }
            #GemCatalogHeaderShowCountLabel { font-size: 12px; font: italic; color: #FFFFFF; }
            #FilterTagWidgetTextLabel { color: #94D2FF; font-size: 10px; }
        )sheet";

        //! The universal rule (BaseStyleSheet.qss:15-21) and the label rule (Text.qss:89-95): what reaches a link
        constexpr const char* LabelRules = R"sheet(
            * { background-color: transparent; color: white; font-family: "Open Sans"; font-weight: 400; }
            QLabel { font-family: "Open Sans"; font-size: 12px; margin-top: 1px; margin-bottom: 1px; }
        )sheet";

        //! The label rules, and the one the inspector adds (ProjectManager.qss:956-958)
        constexpr const char* InspectorRules = R"sheet(
            * { background-color: transparent; color: white; font-family: "Open Sans"; font-weight: 400; }
            QLabel { font-family: "Open Sans"; font-size: 12px; margin-top: 1px; margin-bottom: 1px; }
            #GemCatalogInspector QLabel { margin:0; }
        )sheet";

        //! What reaches a list with no parent: the universal rule (BaseStyleSheet.qss:15-21), the views' fill (:23-30) and its reset for
        //! a view with a parent (:32-38), the rules of every view and of its items (TableView.qss:14-60), and the margin of a scroll
        //! area (QDockWidget.qss:33-36)
        constexpr const char* PopupRules = R"sheet(
            * { background-color: transparent; color: white; font-family: "Open Sans"; font-weight: 400; }
            QTableView, QListView, QTreeView { background-color: #444444; }
            QWidget > QTableView, QWidget > QListView, QWidget > QTreeView { background-color: transparent; }
            QListView { alternate-background-color: rgb(77, 77, 77); selection-background-color: rgb(101, 101, 101);
                selection-color: #ffffff; show-decoration-selected: 1; border: none; }
            QListView::item { height: 24px; padding-left: 4px; border: none; }
            QListView::item:selected { background: rgb(101, 101, 101); }
            QListView::item:hover { background: rgb(90, 90, 90); }
            QAbstractScrollArea { margin: 0; }
        )sheet";

        //! ProjectManagerCompleterPopup.qss, as the popup's code reads it (FormLineEditTagsWidget.cpp)
        constexpr const char* PopupSheet = R"sheet(QListView{
    background: #333333;
    color: #ffffff;
    selection-background-color:#444444;
    selection-color: #ffffff;
})sheet";

        constexpr QRgb ParentColor = 0x808080;          // what neither the sheet nor the element draws stays this grey
        constexpr int HeaderWidth = 844;                // the header of the captures: 844 x 74
        constexpr int HeaderHeight = 74;
        constexpr int RatioCount = 3;
        constexpr qreal Ratios[RatioCount] = { 1.0, 1.5, 2.0 };     // the device pixel ratios the GUI runs at

        // ------------------------------------------------------------------------------
        // Painting
        // ------------------------------------------------------------------------------

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

        //! What a widget paints for its own box through its own style, in the order Qt paints it: the styled background (PE_Widget,
        //! QWidgetPrivate::paintBackground), then a frame's frame (CE_ShapedFrame, QFrame::drawFrame)
        QImage paintBox(const QWidget& widget, qreal ratio)
        {
            QImage image((QSizeF(widget.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            if (image.isNull())
            {
                return image;                       // a widget with no area (a spacer) paints nothing
            }
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

        //! The widget and everything in it, painted as the window paints it, but not the window's own fill (QWidget::DrawWindowBackground).
        //! The fill of the widget being rendered is the palette's Window brush, which a sheet that makes every background transparent
        //! (the universal rule, BaseStyleSheet.qss:15-21) leaves unpainted and the flattened palette paints #444444: it belongs to the
        //! window, which is not what the twins compare. What the widgets draw on it is, and a widget with a fill of its own (a styled
        //! background: the header, a tag, the popup) still paints it.
        QImage renderTree(QWidget& widget, qreal ratio)
        {
            QImage image((QSizeF(widget.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            if (image.isNull())
            {
                return image;                       // a widget with no area (a spacer) paints nothing
            }
            image.setDevicePixelRatio(ratio);
            image.fill(QColor::fromRgb(ParentColor));
            QPainter painter(&image);
            widget.render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
            painter.end();
            return image;
        }

        //! A rect and a size as text, for a failure message (googletest prints a QRect as bytes)
        std::string rectText(const QRect& rect)
        {
            return "(" + std::to_string(rect.x()) + ", " + std::to_string(rect.y()) + ", " + std::to_string(rect.width()) + " x "
                + std::to_string(rect.height()) + ")";
        }

        std::string sizeText(const QSize& size)
        {
            return std::to_string(size.width()) + " x " + std::to_string(size.height());
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
                            return QString::number(rgb, 16).toStdString();
                        };
                        return "at (" + std::to_string(x) + ", " + std::to_string(y) + "): legacy #" + hex(a.pixel(x, y)) + ", flat #"
                            + hex(b.pixel(x, y));
                    }
                }
            }
            return "";
        }

        //! How two images of one size differ, as text for a failure message ("" when they are equal): the count of the pixels that
        //! differ, the rectangle that holds them all, and the largest step of any colour channel. The first pixel alone says nothing of
        //! the extent (M-pm-completer: a residue of 2-4 grey levels was known by its first pixel only).
        std::string differenceSummary(const QImage& a, const QImage& b)
        {
            if (a.size() != b.size())
            {
                return "the sizes differ";
            }
            int count = 0;
            int left = a.width();
            int top = a.height();
            int right = -1;
            int bottom = -1;
            int largestStep = 0;
            for (int y = 0; y < a.height(); ++y)
            {
                for (int x = 0; x < a.width(); ++x)
                {
                    const QRgb first = a.pixel(x, y);
                    const QRgb second = b.pixel(x, y);
                    if (first == second)
                    {
                        continue;
                    }
                    ++count;
                    left = std::min(left, x);
                    top = std::min(top, y);
                    right = std::max(right, x);
                    bottom = std::max(bottom, y);
                    largestStep = std::max({ largestStep, std::abs(qRed(first) - qRed(second)), std::abs(qGreen(first) - qGreen(second)),
                        std::abs(qBlue(first) - qBlue(second)), std::abs(qAlpha(first) - qAlpha(second)) });
                }
            }
            if (count == 0)
            {
                return "";
            }
            return std::to_string(count) + " pixels differ, inside (" + std::to_string(left) + ", " + std::to_string(top) + ") to ("
                + std::to_string(right) + ", " + std::to_string(bottom) + "), the largest channel step " + std::to_string(largestStep);
        }

        // ------------------------------------------------------------------------------
        // What a widget looks like to the code that lays it out
        // ------------------------------------------------------------------------------

        //! The font a widget draws with, as the font database resolved it
        std::string fontText(const QWidget& widget)
        {
            const QFontInfo info(widget.font());
            return "family " + info.family().toStdString() + ", " + std::to_string(info.pixelSize()) + "px, weight "
                + std::to_string(info.weight()) + (info.italic() ? ", italic" : "") + (info.underline() ? ", underline" : "");
        }

        //! The text roles of the palette in the groups a label is drawn in
        std::string paletteText(const QWidget& widget)
        {
            std::string text;
            const QPalette palette = widget.palette();
            for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
            {
                for (const QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText })
                {
                    text += palette.color(group, role).name(QColor::HexArgb).toStdString() + " ";
                }
            }
            return text;
        }

        //! The contents box of a frame (SE_ShapedFrameContents): where a label lays out its text, and whether it indents it
        QRect frameContents(QWidget& widget)
        {
            const QStyleOptionFrame option = frameOption(qobject_cast<QFrame*>(&widget));
            return widget.style()->subElementRect(QStyle::SE_ShapedFrameContents, &option, &widget);
        }

        std::string describe(const QWidget& widget)
        {
            return std::string(widget.metaObject()->className()) + "#" + widget.objectName().toStdString();
        }

        const char* boxOf(const QWidget& widget)
        {
            const AzQtComponents::BoxElement* element = AzQtComponents::BoxElement::forWidget(&widget);
            return element ? element->m_name : "";
        }

        const char* frameElementOf(const QWidget& widget)
        {
            const AzQtComponents::FrameElement* element = AzQtComponents::FrameElement::forWidget(&widget);
            return element ? element->m_name : "";
        }

        //! The popup as FormLineEditTagsWidget makes it (FormLineEditTagsWidget.cpp:67-71): a list with no parent
        QAbstractItemView* makePopup(QCompleter& completer)
        {
            QAbstractItemView* popup = completer.popup();
            popup->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            popup->setFixedWidth(200);
            popup->setMouseTracking(true);
            popup->setObjectName(QStringLiteral("formCompleterPopup"));
            popup->resize(200, 120);
            return popup;
        }

        //! Two rows in the popup (the prefix matches two words of the completer), the second one current and so selected, and the popup
        //! laid out as its first show lays it out: the widget is never shown (a popup window takes the pointer)
        void fillPopup(QCompleter& completer, QAbstractItemView& popup)
        {
            completer.setCompletionPrefix(QStringLiteral("alp"));
            popup.setCurrentIndex(popup.model()->index(1, 0));
            QResizeEvent resize(popup.size(), QSize());
            QCoreApplication::sendEvent(&popup, &resize);
        }

        // ------------------------------------------------------------------------------
        // The header of the gem list, as the catalog builds it, with two tags in its filter
        // ------------------------------------------------------------------------------

        //! A tree that was never shown, laid out as its first show lays it out: a layout sets the geometry of what it holds, and a hidden
        //! widget is sent no resize event by setGeometry, so the layout inside it waits for the show. The widget gets the event, and
        //! then what lies in it, top down.
        void layOutTree(QWidget& widget)
        {
            if (widget.layout())
            {
                QResizeEvent resize(widget.size(), widget.size());
                QCoreApplication::sendEvent(&widget, &resize);
            }
            const QObjectList children = widget.children();
            for (QObject* child : children)
            {
                if (auto* childWidget = qobject_cast<QWidget*>(child))
                {
                    layOutTree(*childWidget);
                }
            }
        }

        class HeaderTree
        {
        public:
            //! Built in a rung: the legacy one sets the sheets of the code, the flattened one sets none
            explicit HeaderTree(Rung rung)
            {
                AzQtComponents::GuiPerfFloor::setRungForTesting(rung);
                m_gemModel.reset(new GemModel());
                m_proxy.reset(new GemSortFilterProxyModel(m_gemModel.get()));
                m_root.reset(new QWidget());
                m_header = new GemListHeaderWidget(m_proxy.get(), m_root.data());

                // The tags of the filter, which the header makes when the filter changes
                m_proxy->SetFeatures(QSet<QString>{ QStringLiteral("Gameplay"), QStringLiteral("Rendering") });

                // An icon the test makes, for the one a resource of the application holds: the contents of the button
                QPixmap pixmap(12, 12);
                pixmap.fill(Qt::red);
                for (QPushButton* button : m_header->findChildren<QPushButton*>())
                {
                    if (qobject_cast<FilterTagWidget*>(button->parentWidget()))
                    {
                        button->setIcon(QIcon(pixmap));
                    }
                }
            }

            //! Lays the header, and the tags in its filter, out at its size in the captures, polished by the style it has
            void layOut()
            {
                m_header->ensurePolished();
                m_header->resize(HeaderWidth, HeaderHeight);
                layOutTree(*m_header);
            }

            QWidget* Root() const
            {
                return m_root.data();
            }

            GemListHeaderWidget* Header() const
            {
                return m_header;
            }

        private:
            // Destroyed in the reverse order: the header before the models it watches
            QScopedPointer<GemModel> m_gemModel;
            QScopedPointer<GemSortFilterProxyModel> m_proxy;
            QScopedPointer<QWidget> m_root;
            GemListHeaderWidget* m_header = nullptr;
        };

        // ------------------------------------------------------------------------------
        // What the twins of the header are compared by
        // ------------------------------------------------------------------------------

        //! The facts of a header tree, taken while it is alive in its own rung: the legacy twin sits on AzQtComponents::Style, which
        //! answers by the rung of the process, so the two twins cannot be held side by side and measured afterwards
        struct HeaderFacts
        {
            //! A widget inside the header
            struct Widget
            {
                std::string m_name;
                QRect m_geometry;
                QRect m_contents;
                QSize m_hint;
                QSize m_minimumHint;
                bool m_label = false;
                bool m_closeButton = false;     // the push button of a tag
                bool m_ownsBox = false;         // the line, the count and a tag: the widgets whose element draws their box
                std::string m_font;             // a label's and a push button's
                std::string m_palette;          // a label's
                QImage m_box[RatioCount];       // what the widget's own style paints for its box, at each ratio
                QImage m_render[RatioCount];    // the widget and everything in it, at each ratio
            };

            std::vector<Widget> m_widgets;      // everything inside the header, in the order Qt lists it
            QImage m_tree[RatioCount];          // the header and everything in it
            QImage m_headerBox[RatioCount];     // the header's own box
        };

        // ------------------------------------------------------------------------------
        // The rules of the application's sheets and of ProjectManager.qss that the chips and the progress bars are twinned under. Each
        // block is the text of its file between the lines named, and a test compares the two (Rules_TheTwinsCopiesAreTheTextOfTheSheets).
        // ------------------------------------------------------------------------------

        //! BaseStyleSheet.qss:15-21, the universal rule
        constexpr const char* UniversalRule = R"sheet(
            *
            {
                background-color: transparent;
                color: white;
                font-family: "Open Sans";
                font-weight: 400;
            }
        )sheet";

        //! CheckBox.qss:10-73, the check box (:10-16), its disabled text (:18-21), the size and margin of every indicator (:23-28) and
        //! the image of each of its states (:30-73): what reaches a check box and its ::indicator
        constexpr const char* CheckBoxRules = R"sheet(
            QCheckBox
            {
                background-color: transparent;
                font-size: 12px;
                line-height: 16px;
                outline: 0;
            }

            QCheckBox:disabled
            {
                color: #999999;
            }

            QAbstractItemView::indicator,
            QCheckBox::indicator {
                width: 16px;
                height: 16px;
                margin: 2px;
            }

            QAbstractItemView::indicator:indeterminate,
            QCheckBox::indicator:indeterminate{
                image: url(:/stylesheet/img/UI20/checkbox/partial-selected.svg);
            }

            QAbstractItemView::indicator:indeterminate:focus,
            QCheckBox::indicator:indeterminate:focus {
                image: url(:/stylesheet/img/UI20/checkbox/partial-selected-focus.svg);
            }

            QAbstractItemView::indicator:indeterminate:disabled,
            QCheckBox::indicator:indeterminate:disabled {
                image: url(:/stylesheet/img/UI20/checkbox/partial-selected-disabled.svg);
            }

            QAbstractItemView::indicator:checked,
            QCheckBox::indicator:checked {
                image: url(:/stylesheet/img/UI20/checkbox/on.svg);
            }

            QAbstractItemView::indicator:checked:focus,
            QCheckBox::indicator:checked:focus {
                image: url(:/stylesheet/img/UI20/checkbox/on-focus.svg);
            }

            QAbstractItemView::indicator:checked:disabled,
            QCheckBox::indicator:checked:disabled {
                image: url(:/stylesheet/img/UI20/checkbox/on-disabled.svg);
            }

            QAbstractItemView::indicator:unchecked,
            QCheckBox::indicator:unchecked {
                image: url(:/stylesheet/img/UI20/checkbox/off.svg);
            }

            QAbstractItemView::indicator:unchecked:focus,
            QCheckBox::indicator:unchecked:focus {
                image: url(:/stylesheet/img/UI20/checkbox/off-focus.svg);
            }

            QAbstractItemView::indicator:unchecked:disabled,
            QCheckBox::indicator:unchecked:disabled {
                image: url(:/stylesheet/img/UI20/checkbox/off-disabled.svg);
            }
        )sheet";

        //! ProjectManager.qss:232-247, the tag chip (`#formTagField QCheckBox`) and its indicator
        constexpr const char* ChipRules = R"sheet(
            #formTagField QCheckBox{
                background-color: #555555;
                padding-left:7px;
                padding-top:2px;
                padding-bottom:2px;
                padding-right:5px;
                spacing:0px;
                color:#94D2FF;
            }

            #formTagField QCheckBox::indicator{
                background-color: transparent;
                image:url(:/X.svg);
                width: 10%;
                padding-left:3px;
            }
        )sheet";

        //! ProgressBar.qss:10-24, the bar and its chunk
        constexpr const char* ProgressBarRules = R"sheet(
            QProgressBar
            {
                border: 0px transparent #000000;
                border-radius: 0px;
                background: #9A9A9A;
                height: 5px;
                font-size: 1px; /* hack to get around the fact that QProgressBar's minimumSizeHint
                                   is computed from font size, even when text is invisible */
                qproperty-textVisible: False;
            }

            QProgressBar::chunk
            {
                background: #1E70EB;
            }
        )sheet";

        //! ProjectManager.qss:772-780, the bar and its chunk
        constexpr const char* PmProgressBarRules = R"sheet(
            QProgressBar {
                border: none;
                background-color: transparent;
                padding: 0px;
            }

            QProgressBar::chunk {
                background-color: #1E70EB;
            }
        )sheet";

        //! A sheet as the parser sees its tokens: no comments and no white space
        QString normalizedSheet(QString text)
        {
            text.remove(QRegularExpression(QStringLiteral(R"(/\*.*?\*/)"), QRegularExpression::DotMatchesEverythingOption));
            text.remove(QRegularExpression(QStringLiteral(R"(\s)")));
            return text;
        }

        // ------------------------------------------------------------------------------
        // The tag chip and the progress bar, as the Project Manager's code makes them
        // ------------------------------------------------------------------------------

        constexpr QRgb ChipFill = 0x555555;                 // ProjectManager.qss:233
        constexpr QRgb BarChunk = 0x1E70EB;                 // ProjectManager.qss:779, ProgressBar.qss:23
        constexpr QRgb BarTrack = 0x9A9A9A;                 // ProgressBar.qss:13, the track of a bar the module's sheet does not reach

        //! A check box that gives its style option out (QCheckBox::initStyleOption is protected)
        class ExposedCheckBox
            : public QCheckBox
        {
        public:
            using QCheckBox::QCheckBox;

            //! The option QCheckBox::paintEvent builds, in a state of the test's choosing
            QStyleOptionButton optionOf(QStyle::State state) const
            {
                QStyleOptionButton option;
                initStyleOption(&option);
                option.rect = QRect(QPoint(0, 0), size());
                option.state = state;
                return option;
            }
        };

        //! A progress bar that gives its style option out (QProgressBar::initStyleOption is protected)
        class ExposedProgressBar
            : public QProgressBar
        {
        public:
            using QProgressBar::QProgressBar;

            QStyleOptionProgressBar optionOf() const
            {
                QStyleOptionProgressBar option;
                initStyleOption(&option);
                option.rect = QRect(QPoint(0, 0), size());
                return option;
            }
        };

        //! A widget that Qt takes for the Project Manager's window: the matchers ask for the window by its class name, as the selectors
        //! do, and the real window is built from the Python bindings and the screens of the whole application
        class WindowStandIn
            : public QWidget
        {
        public:
            using QWidget::QWidget;

            void* qt_metacast(const char* name) override
            {
                return qstrcmp(name, "O3DE::ProjectManager::ProjectManagerWindow") == 0 ? this : QWidget::qt_metacast(name);
            }
        };

        //! The widgets under the window of the Project Manager, as FormLineEditTagsWidget::refreshTagFrame makes the tags (the frame
        //! named formTagField, the check boxes of the tags in it, right to left: FormLineEditTagsWidget.cpp:98-99, :147-148) and
        //! as ProjectButtonWidget.cpp:139 and GemCatalogHeaderWidget.cpp:261-263 make the bars. The root stands for the application's
        //! sheet, the window below it for the window the Project Manager's sheet is set on (Application.cpp:246-249): a sheet set on
        //! a widget is deeper than the application's, whatever the specificity of its rules. The bar beside the window is the one of the
        //! dialog ProjectUtils::ExecuteCommandResultModalDialog makes with no parent: outside the window, so under the application's
        //! sheets only.
        class ModuleTree
        {
        public:
            ModuleTree()
            {
                m_window = new WindowStandIn(&m_root);
                m_window->setObjectName(QStringLiteral("window"));
                m_window->setGeometry(0, 0, 300, 160);

                m_tags = new QFrame(m_window);
                m_tags->setObjectName(QStringLiteral("formTagField"));
                m_tags->setGeometry(0, 0, 300, 90);
                m_chip = makeChip(QStringLiteral("alpha"), QRect(20, 10, 120, 28));
                m_bare = makeChip(QString(), QRect(20, 48, 120, 28));

                m_projectBar = makeBar(QString(), QRect(10, 100, 278, 5));
                m_downloadBar = makeBar(QStringLiteral("DownloadProgressBar"), QRect(10, 120, 278, 5));
                m_downloadBar->setValue(0);                 // GemCatalogHeaderWidget.cpp:263
                m_dialogBar = makeBar(QString(), QRect(10, 140, 278, 5), &m_root);     // ProjectUtils.cpp:656-657
                m_root.resize(m_window->size());
            }

            //! The legacy twin: the application's rules on the root, the Project Manager's on the window
            void toLegacy()
            {
                m_root.setStyleSheet(QString::fromLatin1(UniversalRule) + QString::fromLatin1(CheckBoxRules)
                    + QString::fromLatin1(ProgressBarRules));
                m_window->setStyleSheet(QString::fromLatin1(ChipRules) + QString::fromLatin1(PmProgressBarRules));
                ensurePolished();
            }

            void ensurePolished()
            {
                m_root.ensurePolished();
                for (QWidget* widget : m_root.findChildren<QWidget*>())
                {
                    widget->ensurePolished();
                }
            }

            QWidget m_root;
            QWidget* m_window = nullptr;
            QFrame* m_tags = nullptr;
            ExposedCheckBox* m_chip = nullptr;              //!< a tag, with its text
            ExposedCheckBox* m_bare = nullptr;              //!< a tag with no text: the chip and nothing else
            ExposedProgressBar* m_projectBar = nullptr;     //!< ProjectButtonWidget's: a bar with the defaults of the class
            ExposedProgressBar* m_downloadBar = nullptr;    //!< the gem cart's, named DownloadProgressBar, at 0
            ExposedProgressBar* m_dialogBar = nullptr;      //!< a dialog's with no parent, outside the window

        private:
            ExposedCheckBox* makeChip(const QString& text, const QRect& geometry)
            {
                auto* chip = new ExposedCheckBox(text, m_tags);
                chip->setLayoutDirection(Qt::RightToLeft);
                chip->setGeometry(geometry);
                return chip;
            }

            ExposedProgressBar* makeBar(const QString& name, const QRect& geometry, QWidget* parent = nullptr)
            {
                auto* bar = new ExposedProgressBar(parent ? parent : m_window);
                if (!name.isEmpty())
                {
                    bar->setObjectName(name);
                }
                bar->setGeometry(geometry);
                return bar;
            }
        };

        //! A state of a check box the sheet has a rule for: the image of the indicator is a rule of each (CheckBox.qss:30-73)
        struct ChipState
        {
            const char* m_name;
            QStyle::State m_state;
        };

        const QStyle::State Live = QStyle::State_Enabled | QStyle::State_Active;
        const QStyle::State Dead = QStyle::State_Active;

        const ChipState ChipStates[] = {
            { "unchecked", Live | QStyle::State_Off },
            { "unchecked, under the pointer", Live | QStyle::State_Off | QStyle::State_MouseOver },
            { "unchecked, focused", Live | QStyle::State_Off | QStyle::State_HasFocus },
            { "checked", Live | QStyle::State_On },
            { "checked, focused", Live | QStyle::State_On | QStyle::State_HasFocus },
            { "partially checked", Live | QStyle::State_NoChange },
            { "partially checked, focused", Live | QStyle::State_NoChange | QStyle::State_HasFocus },
            { "unchecked, disabled", Dead | QStyle::State_Off },
            { "checked, disabled", Dead | QStyle::State_On },
            { "partially checked, disabled", Dead | QStyle::State_NoChange },
        };

        //! CE_CheckBox through the check box's own style, as QCheckBox::paintEvent draws it (a QStylePainter), onto an image of an
        //! explicit device ratio
        QImage paintedChip(ExposedCheckBox& chip, QStyle::State state, qreal ratio)
        {
            QImage image((QSizeF(chip.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor::fromRgb(ParentColor));
            {
                QStylePainter painter(&image, &chip);
                painter.drawControl(QStyle::CE_CheckBox, chip.optionOf(state));
            }
            return image;
        }

        //! A rect of the widget cut out of its image: the pixels a rect of logical coordinates covers at the ratio
        QImage cutOut(const QImage& image, const QRect& rect, qreal ratio)
        {
            return image.copy(QRect(
                qFloor(rect.x() * ratio), qFloor(rect.y() * ratio), qCeil(rect.width() * ratio), qCeil(rect.height() * ratio)));
        }

        //! CE_ProgressBar through the bar's own style, as QProgressBar::paintEvent draws it, onto an image of an explicit device ratio
        QImage paintedBar(ExposedProgressBar& bar, qreal ratio)
        {
            QImage image((QSizeF(bar.size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor::fromRgb(ParentColor));
            {
                QStylePainter painter(&image, &bar);
                painter.drawControl(QStyle::CE_ProgressBar, bar.optionOf());
            }
            return image;
        }

        int countOf(const QImage& image, QRgb rgb)
        {
            int count = 0;
            for (int y = 0; y < image.height(); ++y)
            {
                for (int x = 0; x < image.width(); ++x)
                {
                    count += ((image.pixel(x, y) & 0xFFFFFF) == rgb) ? 1 : 0;
                }
            }
            return count;
        }

        //! The states of a bar the tests run it through: a value outside the range is not taken (QProgressBar::setValue), so the bar is
        //! reset first
        struct BarCase
        {
            const char* m_name;
            int m_minimum;
            int m_maximum;
            int m_value;
        };

        const BarCase BarCases[] = {
            { "unset", 0, 100, -1 },
            { "empty", 0, 100, 0 },
            { "one", 0, 100, 1 },
            { "part", 0, 100, 40 },
            { "nearly full", 0, 100, 99 },
            { "full", 0, 100, 100 },
            { "a range that starts below zero", -50, 50, 0 },
        };

        void setBar(QProgressBar& bar, const BarCase& test)
        {
            bar.setRange(test.m_minimum, test.m_maximum);
            bar.reset();
            bar.setValue(test.m_value);
        }

        HeaderFacts takeFacts(QWidget& header)
        {
            HeaderFacts facts;
            const QList<QWidget*> widgets = header.findChildren<QWidget*>();

            // What lays them out first, all before the first paint (a render may lay the window out again)
            for (QWidget* widget : widgets)
            {
                HeaderFacts::Widget item;
                item.m_name = describe(*widget);
                item.m_geometry = widget->geometry();
                item.m_contents = widget->contentsRect();
                item.m_hint = widget->sizeHint();
                item.m_minimumHint = widget->minimumSizeHint();
                item.m_label = qobject_cast<QLabel*>(widget) != nullptr;
                const bool pushButton = qobject_cast<QPushButton*>(widget) != nullptr;
                item.m_closeButton = pushButton && qobject_cast<FilterTagWidget*>(widget->parentWidget()) != nullptr;
                item.m_ownsBox = qobject_cast<FilterTagWidget*>(widget) != nullptr
                    || widget->objectName() == QStringLiteral("horizontalSeparatingLine")
                    || widget->objectName() == QStringLiteral("GemCatalogHeaderShowCountLabel");
                if (item.m_label || pushButton)
                {
                    item.m_font = fontText(*widget);
                }
                if (item.m_label)
                {
                    item.m_palette = paletteText(*widget);
                }
                facts.m_widgets.push_back(item);
            }

            // Then what they paint
            for (size_t i = 0; i < facts.m_widgets.size(); ++i)
            {
                for (int r = 0; r < RatioCount; ++r)
                {
                    facts.m_widgets[i].m_box[r] = paintBox(*widgets[static_cast<int>(i)], Ratios[r]);
                    facts.m_widgets[i].m_render[r] = renderTree(*widgets[static_cast<int>(i)], Ratios[r]);
                }
            }
            for (int r = 0; r < RatioCount; ++r)
            {
                facts.m_tree[r] = renderTree(header, Ratios[r]);
                facts.m_headerBox[r] = paintBox(header, Ratios[r]);
            }
            return facts;
        }

        class ProjectManagerFlatSheetTests
            : public ::UnitTest::LeakDetectionFixture
        {
        public:
            ProjectManagerFlatSheetTests()
                : m_application(m_argc, m_argv)
            {
                // The fonts and the style of the flattened GUI (as FlatElementsTests builds them), over a Fusion application: a
                // legacy twin's sheet style has the application's style as its base, a flat twin sets the flattened style itself
                m_applicationFont = QFont(QStringLiteral("Open Sans"));
                m_applicationFont.setPixelSize(12);
                QApplication::setFont(m_applicationFont);
                QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
                AzQtComponents::TextElement::installClassFonts();

                RegisterFlatElements();
            }

            //! The fonts of the flattened GUI as the constructor made them. A twin that sets the application's sheet and clears it again
            //! leaves the application without the class fonts (TextElement::resolve warns of it, and a flat twin then takes the font
            //! of its parent whole): the base font first, as QApplication::setFont(font) clears the class fonts, then the class fonts
            void restoreFlattenedFonts() const
            {
                QApplication::setFont(m_applicationFont);
                AzQtComponents::TextElement::installClassFonts();
            }

            void SetUp() override
            {
                LeakDetectionFixture::SetUp();

                // The application of the test carries no sheet; the twins name their own
                m_applicationSheet = qApp->styleSheet();
                qApp->setStyleSheet(QString());

                m_style.reset(new AzQtComponents::Style(QStyleFactory::create(QStringLiteral("Fusion"))));
            }

            void TearDown() override
            {
                m_style.reset();
                AzQtComponents::GuiPerfFloor::resetForTesting();
                qApp->setStyleSheet(m_applicationSheet);
                LeakDetectionFixture::TearDown();
            }

            //! A tree under a style sheet style: the rules of the application's sheets on its root
            static void toLegacy(QWidget& root, const char* rules)
            {
                root.setStyleSheet(QString::fromLatin1(rules));
            }

            //! The same, over the style the legacy GUI's sheet style sat on (StyleManager::initialize: QStyleSheetStyle -> Style ->
            //! Fusion), in the legacy rung. Each widget is given the style first, as a sheet style over its own style has it as its base.
            void toLegacyOnApplicationStyle(QWidget& root, const char* rules)
            {
                root.setStyle(m_style.get());
                for (QWidget* widget : root.findChildren<QWidget*>())
                {
                    widget->setStyle(m_style.get());
                }
                root.setStyleSheet(QString::fromLatin1(rules));
            }

            //! A tree under the flattened style, with the palette the flattened GUI's application has (Style replaces every palette
            //! Qt derives with its own while the sheets are off, so the rung has to be the flat one)
            void toFlat(QWidget& root)
            {
                // QWidget::setStyle reaches no child of a widget without a sheet (QWidgetPrivate::setStyle_helper propagates only for
                // a sheet style), so each widget of the tree gets the style, as the application's style reaches every widget
                root.setStyle(m_style.get());
                for (QWidget* widget : root.findChildren<QWidget*>())
                {
                    widget->setStyle(m_style.get());
                }
                QPalette palette = root.palette();
                static_cast<QStyle*>(m_style.get())->polish(palette);
                root.setPalette(palette);
            }

            //! The header of the gem list built, laid out and measured in one rung: the legacy one on the application's style under the
            //! sheets of the application's rules, the flattened one under the flattened style. The tree is gone when the facts are.
            HeaderFacts headerFacts(Rung rung)
            {
                HeaderTree tree(rung);
                if (rung == Rung::Qss)
                {
                    toLegacyOnApplicationStyle(*tree.Root(), HeaderRules);
                }
                else
                {
                    toFlat(*tree.Root());
                }
                tree.layOut();
                return takeFacts(*tree.Header());
            }

            //! The widgets of two trees of one shape agree on what lays them out and what they paint
            static void ExpectSameTrees(QWidget& legacy, QWidget& flat)
            {
                const QList<QWidget*> legacyWidgets = legacy.findChildren<QWidget*>();
                const QList<QWidget*> flatWidgets = flat.findChildren<QWidget*>();
                ASSERT_EQ(legacyWidgets.size(), flatWidgets.size());
                for (int i = 0; i < legacyWidgets.size(); ++i)
                {
                    QWidget& a = *legacyWidgets[i];
                    QWidget& b = *flatWidgets[i];
                    ASSERT_STREQ(a.metaObject()->className(), b.metaObject()->className()) << "widget " << i;
                    EXPECT_EQ(rectText(a.geometry()), rectText(b.geometry())) << describe(a);
                    EXPECT_EQ(rectText(a.contentsRect()), rectText(b.contentsRect())) << describe(a);
                    if (qobject_cast<QLabel*>(&a) || qobject_cast<QPushButton*>(&a))
                    {
                        EXPECT_STREQ(fontText(a).c_str(), fontText(b).c_str()) << describe(a);
                    }
                    if (qobject_cast<QLabel*>(&a))
                    {
                        // What a label measures and draws its text with
                        EXPECT_EQ(sizeText(a.sizeHint()), sizeText(b.sizeHint())) << describe(a);
                        EXPECT_EQ(sizeText(a.minimumSizeHint()), sizeText(b.minimumSizeHint())) << describe(a);
                        EXPECT_STREQ(paletteText(a).c_str(), paletteText(b).c_str()) << describe(a);
                    }
                }
            }

        protected:
            int m_argc = 1;                                     // QApplication keeps a reference to it
            char m_programName[5] = "test";
            char* m_argv[2] = { m_programName, nullptr };
            QApplication m_application;

            QString m_applicationSheet;
            QScopedPointer<AzQtComponents::Style> m_style;
            QFont m_applicationFont;                            // the application's, which a twin that sets and clears a sheet loses
        };
    } // namespace FlatSheetTestsInternal

    // A unity build joins this file with the others of namespace O3DE::ProjectManager: the fixture is the only name brought in, the
    // rest is named through the alias
    using FlatSheetTestsInternal::ProjectManagerFlatSheetTests;
    namespace SheetFlat = FlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // The call sites: the legacy GUI sets the sheets its code wrote, the flattened GUI sets none
    // ----------------------------------------------------------------------------------

    TEST_F(ProjectManagerFlatSheetTests, Sheets_TheLegacyRungSetsTheSheetsOfTheCode)
    {
        SheetFlat::HeaderTree tree(SheetFlat::Rung::Qss);
        EXPECT_STREQ(tree.Header()->styleSheet().toUtf8().constData(), "background-color: #333333;");

        // GemFilterTagWidget.cpp:29 and :40
        const QList<FilterTagWidget*> tags = tree.Header()->findChildren<FilterTagWidget*>();
        ASSERT_EQ(tags.size(), 2);
        for (FilterTagWidget* tag : tags)
        {
            EXPECT_STREQ(tag->styleSheet().toUtf8().constData(), "background-color: #555555;");
            QPushButton* close = tag->findChild<QPushButton*>();
            ASSERT_NE(close, nullptr);
            EXPECT_STREQ(close->styleSheet().toUtf8().constData(), "QPushButton { background-color: transparent; border: 0px }");
        }

        // LinkWidget.cpp:56 and :76 (the lines before the flattening)
        QWidget root;
        LinkLabel link(QStringLiteral("Read Documentation"), QUrl(), 12, &root);
        EXPECT_STREQ(link.styleSheet().toUtf8().constData(), "font-size: 12px; color: #94D2FF;");
        QEnterEvent enter(QPointF(1, 1), QPointF(1, 1), QPointF(1, 1));
        QCoreApplication::sendEvent(&link, &enter);
        EXPECT_STREQ(link.styleSheet().toUtf8().constData(), "font-size: 12px; color: #94D2FF; text-decoration: underline;");
        QEvent leave(QEvent::Leave);
        QCoreApplication::sendEvent(&link, &leave);
        EXPECT_STREQ(link.styleSheet().toUtf8().constData(), "font-size: 12px; color: #94D2FF;");

        // GemInspector.cpp:316 and the two values it keeps for no one in this rung
        QVBoxLayout layout(&root);
        const QLabel* label = GemInspector::CreateStyledLabel(&layout, 18, GemInspector::s_headerColor);
        EXPECT_STREQ(label->styleSheet().toUtf8().constData(), "font-size: 18px; color: #FFFFFF;");
        EXPECT_FALSE(label->property(StyledLabelFontSizeProperty).isValid());
        EXPECT_FALSE(label->property(StyledLabelColorProperty).isValid());
    }

    TEST_F(ProjectManagerFlatSheetTests, Sheets_TheFlatRungSetsNoneAndHandsTheValuesOfTheLabelsOver)
    {
        SheetFlat::HeaderTree tree(SheetFlat::Rung::Flat);
        EXPECT_TRUE(tree.Header()->styleSheet().isEmpty());
        const QList<FilterTagWidget*> tags = tree.Header()->findChildren<FilterTagWidget*>();
        ASSERT_EQ(tags.size(), 2);
        for (FilterTagWidget* tag : tags)
        {
            EXPECT_TRUE(tag->styleSheet().isEmpty());
            EXPECT_TRUE(tag->findChild<QPushButton*>()->styleSheet().isEmpty());
        }

        QWidget root;
        LinkLabel link(QStringLiteral("Read Documentation"), QUrl(), 12, &root);
        EXPECT_TRUE(link.styleSheet().isEmpty());
        QEnterEvent enter(QPointF(1, 1), QPointF(1, 1), QPointF(1, 1));
        QCoreApplication::sendEvent(&link, &enter);
        EXPECT_TRUE(link.styleSheet().isEmpty());

        // The three pairs of values the inspector makes its labels with (GemInspector.cpp:385, :386, :513)
        QVBoxLayout layout(&root);
        const QLabel* name = GemInspector::CreateStyledLabel(&layout, 18, GemInspector::s_headerColor);
        const QLabel* creator = GemInspector::CreateStyledLabel(&layout, GemInspector::s_baseFontSize, GemInspector::s_headerColor);
        const QLabel* updated = GemInspector::CreateStyledLabel(&layout, GemInspector::s_baseFontSize, GemInspector::s_textColor);
        for (const QLabel* label : { name, creator, updated })
        {
            EXPECT_TRUE(label->styleSheet().isEmpty());
        }
        EXPECT_STREQ(name->property(StyledLabelFontSizeProperty).toString().toUtf8().constData(), "18");
        EXPECT_STREQ(name->property(StyledLabelColorProperty).toString().toUtf8().constData(), "#FFFFFF");
        EXPECT_STREQ(creator->property(StyledLabelFontSizeProperty).toString().toUtf8().constData(), "12");
        EXPECT_STREQ(creator->property(StyledLabelColorProperty).toString().toUtf8().constData(), "#FFFFFF");
        EXPECT_STREQ(updated->property(StyledLabelFontSizeProperty).toString().toUtf8().constData(), "12");
        EXPECT_STREQ(updated->property(StyledLabelColorProperty).toString().toUtf8().constData(), "#DDDDDD");
    }

    // ----------------------------------------------------------------------------------
    // The values the text elements carry: the callers' values, and a warning for any other
    // ----------------------------------------------------------------------------------

    TEST_F(ProjectManagerFlatSheetTests, Values_OnlyTheValuesInUseHaveAnElement)
    {
        // The links: the constructor's default and the base size (LinkWidget.h:28, GemInspector.h:62)
        EXPECT_TRUE(HasFlatLinkLabelText(10));
        EXPECT_TRUE(HasFlatLinkLabelText(GemInspector::s_baseFontSize));
        EXPECT_FALSE(HasFlatLinkLabelText(14));
        EXPECT_FALSE(HasFlatLinkLabelText(0));

        // The inspector's labels: the three pairs of its calls (GemInspector.cpp:385, :386, :513), in the constants' own case
        EXPECT_TRUE(HasFlatStyledLabelText(18, GemInspector::s_headerColor));
        EXPECT_TRUE(HasFlatStyledLabelText(GemInspector::s_baseFontSize, GemInspector::s_headerColor));
        EXPECT_TRUE(HasFlatStyledLabelText(GemInspector::s_baseFontSize, GemInspector::s_textColor));
        // A size and a colour that are in use, but not as a pair, and values that are not in use
        EXPECT_FALSE(HasFlatStyledLabelText(18, GemInspector::s_textColor));
        EXPECT_FALSE(HasFlatStyledLabelText(14, GemInspector::s_headerColor));
        EXPECT_FALSE(HasFlatStyledLabelText(GemInspector::s_baseFontSize, QStringLiteral("#EEEEEE")));
    }

    // ----------------------------------------------------------------------------------
    // The header of the gem list (GemListHeaderWidget.cpp:31), and the tags of its filter (GemFilterTagWidget.cpp:29, :40)
    // ----------------------------------------------------------------------------------

    TEST_F(ProjectManagerFlatSheetTests, Registry_TheHeaderAndWhatLiesInItHaveTheElementsOfTheirPlace)
    {
        SheetFlat::HeaderTree tree(SheetFlat::Rung::Flat);
        toFlat(*tree.Root());
        tree.layOut();

        EXPECT_STREQ(SheetFlat::boxOf(*tree.Header()), "ProjectManager.Box.GemListHeader");
        int tags = 0;
        for (QWidget* widget : tree.Header()->findChildren<QWidget*>())
        {
            const std::string name = widget->objectName().toStdString();
            if (name == "horizontalSeparatingLine")
            {
                EXPECT_STREQ(SheetFlat::frameElementOf(*widget), "ProjectManager.Frame.GemListHeaderSeparator");
            }
            else if (name == "GemCatalogHeaderShowCountLabel")
            {
                EXPECT_STREQ(SheetFlat::boxOf(*widget), "ProjectManager.Box.GemListCount");
            }
            else if (name == "RefreshButton")
            {
                EXPECT_STREQ(SheetFlat::boxOf(*widget), "ProjectManager.Box.GemListRefreshButton");
            }
            else if (qobject_cast<FilterTagWidget*>(widget))
            {
                ++tags;
                EXPECT_STREQ(SheetFlat::boxOf(*widget), "ProjectManager.Box.FilterTag");
            }
            else if (qobject_cast<QPushButton*>(widget))
            {
                EXPECT_STREQ(SheetFlat::boxOf(*widget), "ProjectManager.Box.FilterTagClose");
            }
        }
        EXPECT_EQ(tags, 2);
    }

    TEST_F(ProjectManagerFlatSheetTests, Registry_TheSamePlacesOutsideTheHeaderHaveNoneOfThem)
    {
        AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Flat);

        // A divider of any other page is the application's divider, over the fill of the page
        QWidget page;
        auto* line = new QFrame(&page);
        line->setObjectName(QStringLiteral("horizontalSeparatingLine"));
        line->setFrameShape(QFrame::HLine);
        // A frame and a push button that are not a header and a tag
        auto* frame = new QFrame(&page);
        auto* button = new QPushButton(frame);
        auto* tagLikeButton = new QPushButton(&page);
        toFlat(page);
        page.ensurePolished();

        EXPECT_STREQ(SheetFlat::frameElementOf(*line), "ProjectManager.Frame.HorizontalSeparator");
        EXPECT_STREQ(SheetFlat::boxOf(*frame), "");
        EXPECT_STREQ(SheetFlat::boxOf(*button), "");
        EXPECT_STREQ(SheetFlat::boxOf(*tagLikeButton), "");
    }

    TEST_F(ProjectManagerFlatSheetTests, Header_EveryWidgetLaysOutAsItDidUnderTheSheet)
    {
        const SheetFlat::HeaderFacts legacy = headerFacts(SheetFlat::Rung::Qss);
        const SheetFlat::HeaderFacts flat = headerFacts(SheetFlat::Rung::Flat);

        ASSERT_EQ(legacy.m_widgets.size(), flat.m_widgets.size());
        for (size_t i = 0; i < legacy.m_widgets.size(); ++i)
        {
            const SheetFlat::HeaderFacts::Widget& a = legacy.m_widgets[i];
            const SheetFlat::HeaderFacts::Widget& b = flat.m_widgets[i];
            ASSERT_EQ(a.m_name, b.m_name) << "widget " << i;
            EXPECT_EQ(SheetFlat::rectText(a.m_geometry), SheetFlat::rectText(b.m_geometry)) << a.m_name;
            EXPECT_EQ(SheetFlat::rectText(a.m_contents), SheetFlat::rectText(b.m_contents)) << a.m_name;
            EXPECT_EQ(a.m_font, b.m_font) << a.m_name;
            if (a.m_label)
            {
                // What a label measures and draws its text with
                EXPECT_EQ(SheetFlat::sizeText(a.m_hint), SheetFlat::sizeText(b.m_hint)) << a.m_name;
                EXPECT_EQ(SheetFlat::sizeText(a.m_minimumHint), SheetFlat::sizeText(b.m_minimumHint)) << a.m_name;
                EXPECT_EQ(a.m_palette, b.m_palette) << a.m_name;
            }
        }
    }

    TEST_F(ProjectManagerFlatSheetTests, Header_TheWholeTreePaintsAsItDidUnderTheSheetAtEveryRatio)
    {
        const SheetFlat::HeaderFacts legacy = headerFacts(SheetFlat::Rung::Qss);
        const SheetFlat::HeaderFacts flat = headerFacts(SheetFlat::Rung::Flat);

        for (int r = 0; r < SheetFlat::RatioCount; ++r)
        {
            EXPECT_EQ(SheetFlat::firstDifference(legacy.m_tree[r], flat.m_tree[r]), std::string()) << "at ratio " << SheetFlat::Ratios[r];
        }
    }

    TEST_F(ProjectManagerFlatSheetTests, Header_TheBoxesOfItsElementsPaintAsTheSheetPaintedThemAtEveryRatio)
    {
        const SheetFlat::HeaderFacts legacy = headerFacts(SheetFlat::Rung::Qss);
        const SheetFlat::HeaderFacts flat = headerFacts(SheetFlat::Rung::Flat);

        ASSERT_EQ(legacy.m_widgets.size(), flat.m_widgets.size());
        int painted = 0;
        for (size_t i = 0; i < flat.m_widgets.size(); ++i)
        {
            // The widgets whose element draws their box: the frames (the line and the tags) and the count
            if (!flat.m_widgets[i].m_ownsBox)
            {
                continue;
            }
            ++painted;
            for (int r = 0; r < SheetFlat::RatioCount; ++r)
            {
                EXPECT_EQ(SheetFlat::firstDifference(legacy.m_widgets[i].m_box[r], flat.m_widgets[i].m_box[r]), std::string())
                    << flat.m_widgets[i].m_name << " at ratio " << SheetFlat::Ratios[r];
            }
        }
        EXPECT_EQ(painted, 4);          // the line, the count and the two tags

        // The header itself, painted over the colour of its parent
        for (int r = 0; r < SheetFlat::RatioCount; ++r)
        {
            EXPECT_EQ(SheetFlat::firstDifference(legacy.m_headerBox[r], flat.m_headerBox[r]), std::string())
                << "the header at ratio " << SheetFlat::Ratios[r];
        }
    }

    TEST_F(ProjectManagerFlatSheetTests, Header_TheLineInItHasTheHeadersFillAndNotTheDividers)
    {
        SheetFlat::HeaderTree flat(SheetFlat::Rung::Flat);
        toFlat(*flat.Root());
        flat.layOut();

        QFrame* line = nullptr;
        for (QFrame* frame : flat.Header()->findChildren<QFrame*>())
        {
            if (frame->objectName() == QStringLiteral("horizontalSeparatingLine"))
            {
                line = frame;
            }
        }
        ASSERT_NE(line, nullptr);
        const QImage image = SheetFlat::paintBox(*line, 1.0);
        EXPECT_EQ(image.pixel(0, 0) & 0xFFFFFF, 0x333333u);                          // the corner: the header's fill, not #444444
        EXPECT_EQ(image.pixel(image.width() - 1, image.height() - 1) & 0xFFFFFF, 0x333333u);
    }

    TEST_F(ProjectManagerFlatSheetTests, FilterTags_TheCloseButtonIsSizedByItsIconAndPaintsAsUnderTheSheetAtEveryRatio)
    {
        const SheetFlat::HeaderFacts legacy = headerFacts(SheetFlat::Rung::Qss);
        const SheetFlat::HeaderFacts flat = headerFacts(SheetFlat::Rung::Flat);

        ASSERT_EQ(legacy.m_widgets.size(), flat.m_widgets.size());
        int buttons = 0;
        for (size_t i = 0; i < flat.m_widgets.size(); ++i)
        {
            const SheetFlat::HeaderFacts::Widget& a = legacy.m_widgets[i];
            const SheetFlat::HeaderFacts::Widget& b = flat.m_widgets[i];
            ASSERT_EQ(a.m_closeButton, b.m_closeButton) << "widget " << i;
            if (!b.m_closeButton)
            {
                continue;
            }
            ++buttons;

            // Sized by its icon alone: no border, no padding. Laid out in its tag as the legacy GUI laid it out: a push button has the
            // maximum height PushButton::polish gives it in both GUIs, so it fills the row of the tag
            EXPECT_EQ(SheetFlat::sizeText(b.m_hint), SheetFlat::sizeText(a.m_hint)) << "button " << buttons;
            EXPECT_EQ(SheetFlat::sizeText(b.m_minimumHint), SheetFlat::sizeText(a.m_minimumHint)) << "button " << buttons;
            EXPECT_EQ(SheetFlat::rectText(b.m_geometry), SheetFlat::rectText(a.m_geometry)) << "button " << buttons;
            for (int r = 0; r < SheetFlat::RatioCount; ++r)
            {
                EXPECT_EQ(SheetFlat::firstDifference(a.m_render[r], b.m_render[r]), std::string())
                    << "button " << buttons << " at ratio " << SheetFlat::Ratios[r];
            }
        }
        EXPECT_EQ(buttons, 2);          // the close button of each of the two tags
    }

    // ----------------------------------------------------------------------------------
    // The links (LinkWidget.cpp:56, :76)
    // ----------------------------------------------------------------------------------

    TEST_F(ProjectManagerFlatSheetTests, Links_ALinkOfEachSizeHasTheFontTheColourAndTheBoxItHadUnderTheSheet)
    {
        // Two places: under the label rule alone, and below the widget the gem inspector's scroll area names #GemCatalogInspector (and
        // the repository inspector's, #gemRepoInspector), where six of the seven links are. Its rule takes the label margin away from
        // the link and from the label the link keeps inside (the flattened GUI gives both the inspector label's box)
        for (const bool inInspector : { false, true })
        {
            for (const int size : { 10, 12 })
            {
                QWidget legacyRoot;
                QWidget flatRoot;
                QWidget* legacyHost = &legacyRoot;
                QWidget* flatHost = &flatRoot;

                AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Qss);
                if (inInspector)
                {
                    legacyHost = new QWidget(&legacyRoot);
                    legacyHost->setObjectName(QStringLiteral("GemCatalogInspector"));
                }
                auto* legacyLayout = new QVBoxLayout(legacyHost);
                legacyLayout->setContentsMargins(0, 0, 0, 0);
                legacyLayout->addWidget(new LinkLabel(QStringLiteral("Read Documentation"), QUrl(), size));
                toLegacy(legacyRoot, inInspector ? SheetFlat::InspectorRules : SheetFlat::LabelRules);
                legacyRoot.ensurePolished();
                legacyHost->resize(200, 40);
                legacyLayout->activate();

                AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Flat);
                if (inInspector)
                {
                    flatHost = new QWidget(&flatRoot);
                    flatHost->setObjectName(QStringLiteral("GemCatalogInspector"));
                }
                auto* flatLayout = new QVBoxLayout(flatHost);
                flatLayout->setContentsMargins(0, 0, 0, 0);
                flatLayout->addWidget(new LinkLabel(QStringLiteral("Read Documentation"), QUrl(), size));
                toFlat(flatRoot);
                flatRoot.ensurePolished();
                flatHost->resize(200, 40);
                flatLayout->activate();

                // The link and the label it keeps inside to measure its text (the sheet reached it as it reaches every widget inside)
                ExpectSameTrees(*legacyHost, *flatHost);

                QLabel* flatMeasure = flatHost->findChild<LinkLabel*>()->findChild<QLabel*>();
                ASSERT_NE(flatMeasure, nullptr);
                EXPECT_EQ(QFontInfo(flatMeasure->font()).pixelSize(), size);
                EXPECT_EQ(QFontInfo(flatHost->findChild<LinkLabel*>()->font()).pixelSize(), size);
                EXPECT_STREQ(SheetFlat::paletteText(*flatHost->findChild<LinkLabel*>()).c_str(),
                    "#ff94d2ff #ff94d2ff #ff94d2ff #ff94d2ff #ff94d2ff #ff94d2ff #ff94d2ff #ff94d2ff #ff94d2ff ");

                for (const qreal ratio : { 1.0, 1.5, 2.0 })
                {
                    EXPECT_EQ(
                        SheetFlat::firstDifference(SheetFlat::renderTree(*legacyHost, ratio), SheetFlat::renderTree(*flatHost, ratio)),
                        std::string())
                        << "a link of " << size << "px " << (inInspector ? "in the inspector" : "on its own") << " at ratio " << ratio;
                }
            }
        }
    }

    TEST_F(ProjectManagerFlatSheetTests, Links_ASizeNoElementNamesAndALabelOutsideALinkAreNotStyledAsOne)
    {
        AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Flat);
        QWidget root;
        auto* layout = new QVBoxLayout(&root);
        auto* odd = new LinkLabel(QStringLiteral("Odd"), QUrl(), 14);
        auto* plain = new QLabel(QStringLiteral("Plain"));
        layout->addWidget(odd);
        layout->addWidget(plain);
        toFlat(root);
        root.ensurePolished();

        // A link of a size no element carries is a plain label to the flattened GUI (the sizes in use are 10 and 12), and so is
        // a label of the same place that is not a link. LinkLabel's constructor warns about the size (FlatElements.h)
        EXPECT_FALSE(HasFlatLinkLabelText(14));
        EXPECT_EQ(QFontInfo(odd->font()).pixelSize(), QFontInfo(plain->font()).pixelSize());
        EXPECT_STREQ(SheetFlat::paletteText(*odd).c_str(), SheetFlat::paletteText(*plain).c_str());
    }

    // ----------------------------------------------------------------------------------
    // The labels of the gem inspector (GemInspector.cpp:316, from the calls at :385, :386, :449, :463, :513, :514)
    // ----------------------------------------------------------------------------------

    TEST_F(ProjectManagerFlatSheetTests, InspectorLabels_EachPairOfValuesGivesTheFontTheColourAndTheBoxOfItsSheet)
    {
        struct Pair
        {
            int m_size;
            const char* m_color;
        };
        const Pair pairs[] = {
            { 18, GemInspector::s_headerColor },
            { GemInspector::s_baseFontSize, GemInspector::s_headerColor },
            { GemInspector::s_baseFontSize, GemInspector::s_textColor },
        };

        for (const Pair& pair : pairs)
        {
            QWidget legacyRoot;
            QWidget flatRoot;

            // The inspector's labels sit in the widget the scroll area holds, below the area named #GemCatalogInspector
            AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Qss);
            auto* legacyInspector = new QWidget(&legacyRoot);
            legacyInspector->setObjectName(QStringLiteral("GemCatalogInspector"));
            auto* legacyLayout = new QVBoxLayout(legacyInspector);
            legacyLayout->setContentsMargins(0, 0, 0, 0);
            GemInspector::CreateStyledLabel(legacyLayout, pair.m_size, pair.m_color)->setText(QStringLiteral("Open 3D Engine"));
            toLegacy(legacyRoot, SheetFlat::InspectorRules);
            legacyRoot.ensurePolished();
            legacyInspector->resize(240, 60);
            legacyLayout->activate();

            AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Flat);
            auto* flatInspector = new QWidget(&flatRoot);
            flatInspector->setObjectName(QStringLiteral("GemCatalogInspector"));
            auto* flatLayout = new QVBoxLayout(flatInspector);
            flatLayout->setContentsMargins(0, 0, 0, 0);
            GemInspector::CreateStyledLabel(flatLayout, pair.m_size, pair.m_color)->setText(QStringLiteral("Open 3D Engine"));
            toFlat(flatRoot);
            flatRoot.ensurePolished();
            flatInspector->resize(240, 60);
            flatLayout->activate();

            ExpectSameTrees(*legacyInspector, *flatInspector);

            QLabel* legacyLabel = legacyInspector->findChild<QLabel*>();
            QLabel* flatLabel = flatInspector->findChild<QLabel*>();
            ASSERT_NE(flatLabel, nullptr);
            EXPECT_EQ(QFontInfo(flatLabel->font()).pixelSize(), pair.m_size);
            EXPECT_EQ(SheetFlat::frameContents(*flatLabel), SheetFlat::frameContents(*legacyLabel));
            EXPECT_EQ(flatLabel->contentsMargins(), legacyLabel->contentsMargins());

            for (const qreal ratio : { 1.0, 1.5, 2.0 })
            {
                EXPECT_EQ(
                    SheetFlat::firstDifference(
                        SheetFlat::renderTree(*legacyInspector, ratio), SheetFlat::renderTree(*flatInspector, ratio)),
                    std::string())
                    << pair.m_size << "px " << pair.m_color << " at ratio " << ratio;
            }
        }
    }

    TEST_F(ProjectManagerFlatSheetTests, InspectorLabels_APairNoElementNamesIsAPlainLabel)
    {
        AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Flat);
        QWidget root;
        auto* layout = new QVBoxLayout(&root);
        QLabel* odd = GemInspector::CreateStyledLabel(layout, 14, QStringLiteral("#FFFFFF"));
        QLabel* otherColor = GemInspector::CreateStyledLabel(layout, 12, QStringLiteral("#EEEEEE"));
        auto* plain = new QLabel(QStringLiteral("Plain"));
        layout->addWidget(plain);
        toFlat(root);
        root.ensurePolished();

        // CreateStyledLabel warns about both pairs (FlatElements.h)
        EXPECT_FALSE(HasFlatStyledLabelText(14, QStringLiteral("#FFFFFF")));
        EXPECT_FALSE(HasFlatStyledLabelText(12, QStringLiteral("#EEEEEE")));
        for (const QLabel* label : { odd, otherColor })
        {
            EXPECT_EQ(QFontInfo(label->font()).pixelSize(), QFontInfo(plain->font()).pixelSize());
            EXPECT_STREQ(SheetFlat::paletteText(*label).c_str(), SheetFlat::paletteText(*plain).c_str());
        }
    }

    // ----------------------------------------------------------------------------------
    // The popup of the tags field's completer (FormLineEditTagsWidget.cpp:85, ProjectManagerCompleterPopup.qss)
    // ----------------------------------------------------------------------------------

    TEST_F(ProjectManagerFlatSheetTests, CompleterPopup_TheWholePopupPaintsAsItDidUnderTheSheetAtEveryRatio)
    {
        const QStringList words{ QStringLiteral("alpha"), QStringLiteral("alphabet"), QStringLiteral("beta") };

        // The legacy popup: the application's rules, which reach a list with no parent, under the sheet its code sets. What the user
        // sees is the popup with its viewport and its two rows, the second one selected: rendered whole
        AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Qss);
        QImage legacyImages[3];
        QColor legacyHighlight;
        QColor legacyHighlightedText;
        std::string legacyFont;                         // what the rows are laid out and drawn with: the font and the rect of each row
        std::string legacyRows[2];
        {
            qApp->setStyleSheet(QString::fromLatin1(SheetFlat::PopupRules));
            // The legacy GUI has no class fonts (its sheets give the fonts, over the application's); the fixture made those of the
            // flattened GUI, and setting a font with no class name clears them
            QApplication::setFont(m_applicationFont);
            QCompleter completer(words);
            QAbstractItemView* popup = SheetFlat::makePopup(completer);
            AzQtComponents::StyleManager::setLegacyStyleSheet(popup, QString::fromLatin1(SheetFlat::PopupSheet));
            popup->ensurePolished();
            SheetFlat::fillPopup(completer, *popup);
            ASSERT_EQ(popup->model()->rowCount(), 2);
            int i = 0;
            for (const qreal ratio : { 1.0, 1.5, 2.0 })
            {
                legacyImages[i++] = SheetFlat::renderTree(*popup, ratio);
            }
            legacyHighlight = popup->palette().color(QPalette::Active, QPalette::Highlight);
            legacyHighlightedText = popup->palette().color(QPalette::Active, QPalette::HighlightedText);
            legacyFont = SheetFlat::fontText(*popup);
            for (int row = 0; row < 2; ++row)
            {
                legacyRows[row] = SheetFlat::rectText(popup->visualRect(popup->model()->index(row, 0)));
            }
            qApp->setStyleSheet(QString());
        }
        // The class fonts of the flattened GUI are back for the flat popup. (Before this, the application's sheet cleared them behind the
        // test's back: TextElement::resolve warned of it at the first polish of the flat popup, which took the application's font whole.)
        restoreFlattenedFonts();
        // The sheet's fill reaches the corners, and the selected row (the second) is filled apart from the first, far from the text
        EXPECT_EQ(legacyImages[0].pixel(0, 0) & 0xFFFFFF, 0x333333u);
        EXPECT_EQ(legacyImages[0].pixel(legacyImages[0].width() - 1, legacyImages[0].height() - 1) & 0xFFFFFF, 0x333333u);
        EXPECT_NE(legacyImages[0].pixel(190, 12), legacyImages[0].pixel(190, 36));

        // The flat one: the code sets no sheet
        AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Flat);
        {
            QCompleter completer(words);
            QAbstractItemView* popup = SheetFlat::makePopup(completer);
            AzQtComponents::StyleManager::setLegacyStyleSheet(popup, QString::fromLatin1(SheetFlat::PopupSheet));
            EXPECT_TRUE(popup->styleSheet().isEmpty());
            toFlat(*popup);
            popup->ensurePolished();
            SheetFlat::fillPopup(completer, *popup);
            ASSERT_EQ(popup->model()->rowCount(), 2);

            EXPECT_STREQ(SheetFlat::boxOf(*popup), "ProjectManager.Box.CompleterPopup");
            const AzQtComponents::ItemViewElement* items = AzQtComponents::ItemViewElement::forWidget(popup);
            ASSERT_NE(items, nullptr);
            EXPECT_STREQ(items->m_name, "ProjectManager.ItemView.CompleterPopup");

            // A list with no parent keeps the application's Base (#444444, Style.cpp applyFlattenedParentedItemView): the viewport
            // shows the element's fill only because the element turned the viewport's own fill off
            EXPECT_FALSE(popup->viewport()->autoFillBackground());

            // What the text is laid out and drawn with comes first: a picture that differs says nothing of why
            EXPECT_EQ(SheetFlat::fontText(*popup), legacyFont);
            for (int row = 0; row < 2; ++row)
            {
                EXPECT_EQ(SheetFlat::rectText(popup->visualRect(popup->model()->index(row, 0))), legacyRows[row]) << "row " << row;
            }

            int i = 0;
            for (const qreal ratio : { 1.0, 1.5, 2.0 })
            {
                const QImage flatImage = SheetFlat::renderTree(*popup, ratio);
                EXPECT_EQ(SheetFlat::firstDifference(legacyImages[i], flatImage), std::string())
                    << "at ratio " << ratio << ": " << SheetFlat::differenceSummary(legacyImages[i], flatImage);
                if (i == 0)
                {
                    EXPECT_EQ(flatImage.pixel(0, 0) & 0xFFFFFF, 0x333333u);
                    EXPECT_NE(flatImage.pixel(190, 12), flatImage.pixel(190, 36));
                }
                ++i;
            }
            EXPECT_STREQ(popup->palette().color(QPalette::Active, QPalette::Highlight).name().toUtf8().constData(),
                legacyHighlight.name().toUtf8().constData());
            EXPECT_STREQ(popup->palette().color(QPalette::Active, QPalette::HighlightedText).name().toUtf8().constData(),
                legacyHighlightedText.name().toUtf8().constData());
        }
    }

    TEST_F(ProjectManagerFlatSheetTests, CompleterPopup_TheSheetTheTwinsSetIsTheTextOfTheFile)
    {
        // The twins set a copy of the popup's sheet (the test executable carries no resources); this keeps the copy honest
        const QString path = QDir(QFileInfo(QString::fromUtf8(__FILE__)).absolutePath())
                                 .filePath(QStringLiteral("../Resources/ProjectManagerCompleterPopup.qss"));
        QFile file(path);
        if (!file.open(QFile::ReadOnly))
        {
            GTEST_SKIP() << "The sheet is not at " << path.toStdString() << " (the tests run away from the source tree)";
        }
        const QString text = QString::fromUtf8(file.readAll()).replace(QStringLiteral("\r\n"), QStringLiteral("\n")).trimmed();
        EXPECT_STREQ(text.toUtf8().constData(), SheetFlat::PopupSheet);
    }

    TEST_F(ProjectManagerFlatSheetTests, CompleterPopup_AListOfAnotherNameKeepsTheElementsOfEveryList)
    {
        AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Flat);
        QListView other;
        other.setObjectName(QStringLiteral("formCompleter"));
        QListView unnamed;
        for (QListView* list : { &other, &unnamed })
        {
            list->setStyle(m_style.get());
            list->ensurePolished();
            EXPECT_STREQ(SheetFlat::boxOf(*list), "");
            const AzQtComponents::ItemViewElement* items = AzQtComponents::ItemViewElement::forWidget(list);
            ASSERT_NE(items, nullptr);
            EXPECT_STREQ(items->m_name, "Global.ItemView.List");
        }
    }

    // ----------------------------------------------------------------------------------
    // The links under the pointer (LinkWidget.cpp:56): the sheet of the hover state is the idle one with `text-decoration: underline`
    // ----------------------------------------------------------------------------------

    TEST_F(ProjectManagerFlatSheetTests, Links_AnUnderlineComesWithThePointerAndGoesWithIt)
    {
        for (const int size : { 10, 12 })
        {
            QWidget legacyRoot;
            QWidget flatRoot;
            QEnterEvent enter(QPointF(1, 1), QPointF(1, 1), QPointF(1, 1));
            QEvent leave(QEvent::Leave);

            AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Qss);
            auto* legacyLayout = new QVBoxLayout(&legacyRoot);
            legacyLayout->setContentsMargins(0, 0, 0, 0);
            auto* legacyLink = new LinkLabel(QStringLiteral("Read Documentation"), QUrl(), size);
            legacyLayout->addWidget(legacyLink);
            toLegacy(legacyRoot, SheetFlat::LabelRules);
            legacyRoot.ensurePolished();
            legacyRoot.resize(200, 40);
            legacyLayout->activate();

            AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Flat);
            auto* flatLayout = new QVBoxLayout(&flatRoot);
            flatLayout->setContentsMargins(0, 0, 0, 0);
            auto* flatLink = new LinkLabel(QStringLiteral("Read Documentation"), QUrl(), size);
            flatLayout->addWidget(flatLink);
            toFlat(flatRoot);
            flatRoot.ensurePolished();
            flatRoot.resize(200, 40);
            flatLayout->activate();

            // Idle, under the pointer, idle again: the font (and so the size the label asks for), the colours and the pixels agree
            // at every step, at the ratios the GUI runs at
            for (const char* state : { "idle", "hover", "idle again" })
            {
                if (qstrcmp(state, "hover") == 0)
                {
                    AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Qss);
                    QCoreApplication::sendEvent(legacyLink, &enter);
                    AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Flat);
                    QCoreApplication::sendEvent(flatLink, &enter);
                }
                else if (qstrcmp(state, "idle again") == 0)
                {
                    AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Qss);
                    QCoreApplication::sendEvent(legacyLink, &leave);
                    AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Flat);
                    QCoreApplication::sendEvent(flatLink, &leave);
                }
                legacyLayout->activate();
                flatLayout->activate();
                SCOPED_TRACE(std::string("a link of ") + std::to_string(size) + "px, " + state);

                const bool underlined = qstrcmp(state, "hover") == 0;
                EXPECT_EQ(QFontInfo(legacyLink->font()).underline(), underlined);
                EXPECT_EQ(QFontInfo(flatLink->font()).underline(), underlined);
                ExpectSameTrees(legacyRoot, flatRoot);
                for (const qreal ratio : { 1.0, 1.5, 2.0 })
                {
                    EXPECT_EQ(SheetFlat::firstDifference(SheetFlat::renderTree(legacyRoot, ratio), SheetFlat::renderTree(flatRoot, ratio)),
                        std::string())
                        << "at ratio " << ratio;
                }
            }
            EXPECT_TRUE(flatLink->styleSheet().isEmpty());
        }
    }

    // ----------------------------------------------------------------------------------
    // The tooltip of a link (ElidingLabel::elide): the sheet of the link had no selector, so Qt resolved the tip through the link
    // ----------------------------------------------------------------------------------

    TEST_F(ProjectManagerFlatSheetTests, ToolTipRegistry_ALinkHasTheTextOfItsSizeAndStateAndNoOtherWidgetDoes)
    {
        AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Flat);
        using AzQtComponents::ToolTipElement;

        QWidget root;
        auto* layout = new QVBoxLayout(&root);
        auto* smallLink = new LinkLabel(QStringLiteral("Small"), QUrl(), 10);
        auto* baseLink = new LinkLabel(QStringLiteral("Base"), QUrl(), 12);
        auto* odd = new LinkLabel(QStringLiteral("Odd"), QUrl(), 14);
        auto* plain = new QLabel(QStringLiteral("Plain"));
        for (QWidget* widget : { static_cast<QWidget*>(smallLink), static_cast<QWidget*>(baseLink), static_cast<QWidget*>(odd),
                                 static_cast<QWidget*>(plain) })
        {
            layout->addWidget(widget);
        }

        const auto textOf = [](const QWidget* widget)
        {
            const ToolTipElement::Look look = ToolTipElement::lookFor(widget);
            return look.m_text ? look.m_text->m_name : "";
        };

        // The link, and the label it keeps inside to measure its text, which the sheet reached as it reaches every widget inside
        EXPECT_STREQ(textOf(smallLink), "ProjectManager.Text.LinkLabel10");
        EXPECT_STREQ(textOf(smallLink->findChild<QLabel*>()), "ProjectManager.Text.LinkLabel10");
        EXPECT_STREQ(textOf(baseLink), "ProjectManager.Text.LinkLabel12");
        EXPECT_STREQ(textOf(odd), "");
        EXPECT_STREQ(textOf(plain), "");
        EXPECT_FALSE(ToolTipElement::lookFor(smallLink).m_fill.isValid());

        // Under the pointer
        QEnterEvent enter(QPointF(1, 1), QPointF(1, 1), QPointF(1, 1));
        QCoreApplication::sendEvent(smallLink, &enter);
        QCoreApplication::sendEvent(baseLink, &enter);
        EXPECT_STREQ(textOf(smallLink), "ProjectManager.Text.LinkLabel10Hover");
        EXPECT_STREQ(textOf(baseLink), "ProjectManager.Text.LinkLabel12Hover");
        QEvent leave(QEvent::Leave);
        QCoreApplication::sendEvent(smallLink, &leave);
        EXPECT_STREQ(textOf(smallLink), "ProjectManager.Text.LinkLabel10");
    }

    TEST_F(ProjectManagerFlatSheetTests, ToolTip_ALinkShowsTheFontAndColourOfItsSheetIdleAndUnderHoverAtEveryRatio)
    {
        AzQtComponents::ToolTipTwins::Guard tips;
        AzQtComponents::ToolTipTwins::StyleFilter filter(m_style.get());
        namespace Twins = AzQtComponents::ToolTipTwins;

        // The rules of the application's sheets that reach a tip, and the label rule that does not (a tip is a "QToolTip" to the sheet
        // style: the 12px of the label rule never reached one, which is why the links' sheets mattered)
        const std::string rules = std::string(Twins::AppRules) + SheetFlat::LabelRules;

        for (const int size : { 10, 12 })
        {
            for (const bool hover : { false, true })
            {
                SCOPED_TRACE(std::string("a link of ") + std::to_string(size) + (hover ? "px under the pointer" : "px"));
                QEnterEvent enter(QPointF(1, 1), QPointF(1, 1), QPointF(1, 1));
                QEvent leave(QEvent::Leave);

                AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Qss);
                QWidget legacyRoot;
                auto* legacyLayout = new QVBoxLayout(&legacyRoot);
                auto* legacyLink = new LinkLabel(QStringLiteral("Read Documentation"), QUrl(), size);
                legacyLayout->addWidget(legacyLink);
                toLegacy(legacyRoot, rules.c_str());
                legacyRoot.ensurePolished();
                legacyRoot.resize(200, 40);
                legacyLayout->activate();
                legacyLink->setToolTip(QStringLiteral("Tip"));     // not setDescription: it stores the string, elide() sets the tip
                if (hover)
                {
                    QCoreApplication::sendEvent(legacyLink, &enter);
                }
                const Twins::TipFacts legacy = Twins::showAndReadTip(legacyLink, nullptr);
                QCoreApplication::sendEvent(legacyLink, &leave);

                AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Flat);
                QWidget flatRoot;
                auto* flatLayout = new QVBoxLayout(&flatRoot);
                auto* flatLink = new LinkLabel(QStringLiteral("Read Documentation"), QUrl(), size);
                flatLayout->addWidget(flatLink);
                toFlat(flatRoot);
                flatRoot.ensurePolished();
                flatRoot.resize(200, 40);
                flatLayout->activate();
                flatLink->setToolTip(QStringLiteral("Tip"));
                if (hover)
                {
                    QCoreApplication::sendEvent(flatLink, &enter);
                }
                // The font of the flat tip is the element's: it is what a link's tip proves. The margin is the legacy tip's (the
                // constructor of the label reads the style of the process)
                const Twins::TipFacts flat = Twins::showAndReadTip(flatLink, m_style.get(), nullptr, legacy.m_margin);
                QCoreApplication::sendEvent(flatLink, &leave);

                ASSERT_TRUE(legacy.m_found && flat.m_found);
                EXPECT_EQ(QFontInfo(legacy.m_font).pixelSize(), size);
                EXPECT_EQ(QFontInfo(legacy.m_font).underline(), hover);
                EXPECT_EQ(Twins::fontText(flat.m_font), Twins::fontText(legacy.m_font));
                EXPECT_EQ(legacy.m_text.name(), QStringLiteral("#94d2ff"));
                EXPECT_EQ(flat.m_text.name(), legacy.m_text.name());
                EXPECT_EQ(flat.m_base.name(), legacy.m_base.name());
                EXPECT_EQ(Twins::marginsText(flat.m_contents), Twins::marginsText(legacy.m_contents));
                EXPECT_EQ(flat.m_hint, legacy.m_hint);
                EXPECT_EQ(flat.m_contentsRect, legacy.m_contentsRect);
                for (int i = 0; i < 3; ++i)
                {
                    EXPECT_EQ(Twins::differingPixels(legacy.m_images[i], flat.m_images[i]), 0) << "at ratio " << Twins::Ratios[i];
                }
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // The rules the chip and progress bar twins are set under
    // ----------------------------------------------------------------------------------

    TEST_F(ProjectManagerFlatSheetTests, Rules_TheTwinsCopiesOfTheApplicationsSheetsAreTheirText)
    {
        const auto readSheet = [](const char* name)
        {
            QFile file(QStringLiteral(":/AzQtComponents/Widgets/") + QString::fromLatin1(name));
            EXPECT_TRUE(file.open(QIODevice::ReadOnly)) << name;
            return SheetFlat::normalizedSheet(QString::fromUtf8(file.readAll()));
        };
        const auto rules = [](const char* text)
        {
            return SheetFlat::normalizedSheet(QString::fromLatin1(text));
        };

        EXPECT_TRUE(readSheet("BaseStyleSheet.qss").contains(rules(SheetFlat::UniversalRule)));
        EXPECT_TRUE(readSheet("CheckBox.qss").contains(rules(SheetFlat::CheckBoxRules)));
        EXPECT_EQ(readSheet("ProgressBar.qss"), rules(SheetFlat::ProgressBarRules));
    }

    TEST_F(ProjectManagerFlatSheetTests, Rules_TheTwinsCopiesOfTheProjectManagersSheetAreItsText)
    {
        const QString path = QDir(QFileInfo(QString::fromUtf8(__FILE__)).absolutePath())
                                 .filePath(QStringLiteral("../Resources/ProjectManager.qss"));
        QFile file(path);
        if (!file.open(QFile::ReadOnly))
        {
            GTEST_SKIP() << "The sheet is not at " << path.toStdString() << " (the tests run away from the source tree)";
        }
        const QString sheet = SheetFlat::normalizedSheet(QString::fromUtf8(file.readAll()));
        EXPECT_TRUE(sheet.contains(SheetFlat::normalizedSheet(QString::fromLatin1(SheetFlat::ChipRules))));
        EXPECT_TRUE(sheet.contains(SheetFlat::normalizedSheet(QString::fromLatin1(SheetFlat::PmProgressBarRules))));
    }

    // ----------------------------------------------------------------------------------
    // The tags of the tags field (ProjectManager.qss:232-247): the mark at the end of a chip is the ::indicator of its check box
    // ----------------------------------------------------------------------------------

    TEST_F(ProjectManagerFlatSheetTests, FormTagChips_TheMarkIsAnImageThatTheTestExecutableCarries)
    {
        // The rule's image is a resource of the Project Manager (ProjectManager.qrc); the test executable has the file as its own
        // resource (FlatSheetTests.qrc), so that both twins load it
        QFile mark(QStringLiteral(":/X.svg"));
        ASSERT_TRUE(mark.open(QIODevice::ReadOnly));
        EXPECT_TRUE(QString::fromUtf8(mark.readAll()).contains(QStringLiteral("#94D2FF")));
    }

    TEST_F(ProjectManagerFlatSheetTests, FormTagChips_TheIndicatorHasTheBoxAndThePlaceItHadUnderTheSheet)
    {
        AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Flat);
        SheetFlat::ModuleTree legacy;
        legacy.toLegacy();
        SheetFlat::ModuleTree flat;
        toFlat(flat.m_root);
        flat.ensurePolished();

        // What the twins are made of: the sheets of the window, and the elements of the registry
        EXPECT_TRUE(legacy.m_window->styleSheet().contains(QStringLiteral("::indicator")));
        EXPECT_TRUE(flat.m_window->styleSheet().isEmpty());
        const AzQtComponents::IndicatorElement* element = AzQtComponents::IndicatorElement::forWidget(flat.m_chip);
        ASSERT_NE(element, nullptr);
        EXPECT_STREQ(element->m_name, "ProjectManager.Indicator.FormTagChip");
        EXPECT_STREQ(SheetFlat::boxOf(*flat.m_chip), "ProjectManager.Box.FormTagChip");

        for (SheetFlat::ExposedCheckBox* chip : { legacy.m_chip, legacy.m_bare })
        {
            const QStyleOptionButton option = chip->optionOf(SheetFlat::Live | QStyle::State_Off);
            const QStyle* style = chip->style();

            // The indicator of the legacy chip is its sub-rule's box: 10px of image (`width: 10%`) and 3px of padding to its left, 16px
            // of image (CheckBox.qss:26), and 2px of margin around (:27): 17 by 20, at the end of the chip's contents, which lies right
            // to left
            const QRect box = style->subElementRect(QStyle::SE_CheckBoxIndicator, &option, chip);
            EXPECT_EQ(box.size(), QSize(17, 20));
            EXPECT_EQ(box.right(), chip->width() - 1 - 5);          // the chip's padding on the right (ProjectManager.qss:237)
            EXPECT_EQ(box.top(), 2 + (chip->height() - 2 - 2 - 20) / 2);   // centered in the contents, below 2px of padding (:235, :236)
        }

        for (const SheetFlat::ChipState& test : SheetFlat::ChipStates)
        {
            for (int c = 0; c < 2; ++c)
            {
                SheetFlat::ExposedCheckBox* a = c == 0 ? legacy.m_chip : legacy.m_bare;
                SheetFlat::ExposedCheckBox* b = c == 0 ? flat.m_chip : flat.m_bare;
                const QStyleOptionButton legacyOption = a->optionOf(test.m_state);
                const QStyleOptionButton flatOption = b->optionOf(test.m_state);
                const std::string name = std::string(test.m_name) + (c == 0 ? ", with text" : ", with no text");

                EXPECT_EQ(SheetFlat::rectText(b->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &flatOption, b)),
                    SheetFlat::rectText(a->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &legacyOption, a))) << name;

                // The space between the mark and the label is the chip's `spacing: 0px` (ProjectManager.qss:238): the label sits against
                // the box of the indicator
                const int legacySpacing = a->style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, &legacyOption, a);
                EXPECT_EQ(legacySpacing, 0) << name;
                EXPECT_EQ(b->style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, &flatOption, b), legacySpacing) << name;

                // The size the style asks for a label of a size, and where the label goes
                const QSize label(40, 14);
                EXPECT_EQ(SheetFlat::sizeText(b->style()->sizeFromContents(QStyle::CT_CheckBox, &flatOption, label, b)),
                    SheetFlat::sizeText(a->style()->sizeFromContents(QStyle::CT_CheckBox, &legacyOption, label, a))) << name;
                EXPECT_EQ(SheetFlat::rectText(b->style()->subElementRect(QStyle::SE_CheckBoxContents, &flatOption, b)),
                    SheetFlat::rectText(a->style()->subElementRect(QStyle::SE_CheckBoxContents, &legacyOption, a))) << name;
            }
        }
    }

    TEST_F(ProjectManagerFlatSheetTests, FormTagChips_TheMarkPaintsAsItDidUnderTheSheetInEveryStateAtEveryRatio)
    {
        AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Flat);
        SheetFlat::ModuleTree legacy;
        legacy.toLegacy();
        SheetFlat::ModuleTree flat;
        toFlat(flat.m_root);
        flat.ensurePolished();

        for (const qreal ratio : SheetFlat::Ratios)
        {
            for (const SheetFlat::ChipState& test : SheetFlat::ChipStates)
            {
                const std::string name = std::string(test.m_name) + " at ratio " + std::to_string(ratio).substr(0, 3);

                // A chip with no text is the box of the chip and its indicator alone: the whole of it is compared
                const QImage legacyBare = SheetFlat::paintedChip(*legacy.m_bare, test.m_state, ratio);
                const QImage flatBare = SheetFlat::paintedChip(*flat.m_bare, test.m_state, ratio);
                EXPECT_EQ(SheetFlat::firstDifference(legacyBare, flatBare), std::string()) << "no text, " << name;

                // The twins are not two images of nothing: the chip's fill is all of it but the mark, which is drawn on it
                EXPECT_EQ(legacyBare.pixel(0, 0) & 0xFFFFFF, SheetFlat::ChipFill) << name;
                EXPECT_LT(SheetFlat::countOf(legacyBare, SheetFlat::ChipFill), legacyBare.width() * legacyBare.height()) << name;

                // A chip with its text: the mark is compared in the box the legacy sheet gave the indicator (the label is the text
                // element's, and the sheet's spacing before it is not what this compares)
                const QStyleOptionButton option = legacy.m_chip->optionOf(test.m_state);
                const QRect box = legacy.m_chip->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option, legacy.m_chip);
                const QImage legacyMark = SheetFlat::cutOut(SheetFlat::paintedChip(*legacy.m_chip, test.m_state, ratio), box, ratio);
                const QImage flatMark = SheetFlat::cutOut(SheetFlat::paintedChip(*flat.m_chip, test.m_state, ratio), box, ratio);
                EXPECT_EQ(SheetFlat::firstDifference(legacyMark, flatMark), std::string()) << "with text, " << name;
                EXPECT_LT(SheetFlat::countOf(legacyMark, SheetFlat::ChipFill), legacyMark.width() * legacyMark.height()) << name;
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // The progress bars (ProjectManager.qss:772-780): ProjectButtonWidget.cpp:139 and GemCatalogHeaderWidget.cpp:261
    // ----------------------------------------------------------------------------------

    TEST_F(ProjectManagerFlatSheetTests, ProgressBars_TheBarsOfTheModuleHaveTheSizeAndTheTextTheyHadUnderTheSheets)
    {
        AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Flat);
        SheetFlat::ModuleTree legacy;
        legacy.toLegacy();
        SheetFlat::ModuleTree flat;
        toFlat(flat.m_root);
        flat.ensurePolished();

        for (int i = 0; i < 2; ++i)
        {
            SheetFlat::ExposedProgressBar* a = i == 0 ? legacy.m_projectBar : legacy.m_downloadBar;
            SheetFlat::ExposedProgressBar* b = i == 0 ? flat.m_projectBar : flat.m_downloadBar;

            // The element of the module is the bar's, and its text is hidden at polish (ProgressBar.qss:18)
            EXPECT_STREQ(SheetFlat::boxOf(*b), "ProjectManager.Box.ProgressBar") << "bar " << i;
            EXPECT_FALSE(a->isTextVisible()) << "bar " << i;
            EXPECT_FALSE(b->isTextVisible()) << "bar " << i;
            EXPECT_EQ(SheetFlat::sizeText(b->sizeHint()), SheetFlat::sizeText(a->sizeHint())) << "bar " << i;
            EXPECT_EQ(SheetFlat::sizeText(b->minimumSizeHint()), SheetFlat::sizeText(a->minimumSizeHint())) << "bar " << i;
            EXPECT_EQ(a->sizeHint().height(), 5) << "bar " << i;        // ProgressBar.qss:15
            EXPECT_EQ(SheetFlat::rectText(b->contentsRect()), SheetFlat::rectText(a->contentsRect())) << "bar " << i;
        }
    }

    TEST_F(ProjectManagerFlatSheetTests, ProgressBars_TheChunkAndTheTrackPaintAsTheSheetsPaintedThemAtEveryRatio)
    {
        AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Flat);
        SheetFlat::ModuleTree legacy;
        legacy.toLegacy();
        SheetFlat::ModuleTree flat;
        toFlat(flat.m_root);
        flat.ensurePolished();

        // What the flat bar drew before the style drew the chunk: the base style's groove and shaded chunk
        QScopedPointer<QStyle> fusion(QStyleFactory::create(QStringLiteral("Fusion")));

        for (int i = 0; i < 2; ++i)
        {
            SheetFlat::ExposedProgressBar* a = i == 0 ? legacy.m_projectBar : legacy.m_downloadBar;
            SheetFlat::ExposedProgressBar* b = i == 0 ? flat.m_projectBar : flat.m_downloadBar;
            for (const qreal ratio : SheetFlat::Ratios)
            {
                for (const SheetFlat::BarCase& test : SheetFlat::BarCases)
                {
                    SheetFlat::setBar(*a, test);
                    SheetFlat::setBar(*b, test);
                    const std::string name = std::string("bar ") + std::to_string(i) + ", " + test.m_name + " at ratio "
                        + std::to_string(ratio).substr(0, 3);
                    const QImage legacyImage = SheetFlat::paintedBar(*a, ratio);
                    EXPECT_EQ(SheetFlat::firstDifference(legacyImage, SheetFlat::paintedBar(*b, ratio)), std::string()) << name;

                    // The twins are not two images of nothing: the chunk is a plain rect of the sheet's blue (ProjectManager.qss:779), as
                    // many columns of the bar as the share of its range, and the track is clear (:774): the backdrop shows through it
                    if (ratio == 1.0)
                    {
                        const double share = double(a->value() - a->minimum()) / double(a->maximum() - a->minimum());
                        const int columns = std::clamp(static_cast<int>(a->width() * share), 0, a->width());
                        EXPECT_EQ(SheetFlat::countOf(legacyImage, SheetFlat::BarChunk), columns * a->height()) << name;
                        EXPECT_EQ(SheetFlat::countOf(legacyImage, SheetFlat::ParentColor), (a->width() - columns) * a->height()) << name;
                    }

                    // ... and the base style draws neither
                    if (test.m_value == 40)
                    {
                        QImage baseImage((QSizeF(b->size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
                        baseImage.setDevicePixelRatio(ratio);
                        baseImage.fill(QColor::fromRgb(SheetFlat::ParentColor));
                        {
                            QPainter painter(&baseImage);
                            const QStyleOptionProgressBar option = b->optionOf();
                            fusion->drawControl(QStyle::CE_ProgressBar, &option, &painter, b);
                        }
                        EXPECT_NE(SheetFlat::firstDifference(legacyImage, baseImage), std::string()) << name;
                    }
                }
            }

            // A disabled bar looks as it does enabled: the sheets have no rule for it
            SheetFlat::setBar(*a, SheetFlat::BarCases[3]);
            SheetFlat::setBar(*b, SheetFlat::BarCases[3]);
            a->setEnabled(false);
            b->setEnabled(false);
            for (const qreal ratio : SheetFlat::Ratios)
            {
                EXPECT_EQ(SheetFlat::firstDifference(SheetFlat::paintedBar(*a, ratio), SheetFlat::paintedBar(*b, ratio)), std::string())
                    << "bar " << i << ", disabled at ratio " << ratio;
            }
            a->setEnabled(true);
            b->setEnabled(true);
        }
    }

    TEST_F(ProjectManagerFlatSheetTests, ProgressBars_ABarOutsideTheWindowIsNotTheModulesAndHadTheApplicationsGreyTrack)
    {
        AzQtComponents::GuiPerfFloor::setRungForTesting(SheetFlat::Rung::Flat);
        SheetFlat::ModuleTree legacy;
        legacy.toLegacy();
        SheetFlat::ModuleTree flat;
        toFlat(flat.m_root);
        flat.ensurePolished();

        // The bar of the dialog ProjectUtils::ExecuteCommandResultModalDialog makes with no parent (ProjectUtils.cpp:652-659) is outside
        // the window the sheet is set on (Application.cpp:246-249). Flat: the module's element is the element of the bars below the
        // window, not of this one (the Global element it keeps in the application is registered by Style::polish(QApplication*) in the
        // flattened GUI, and ViewsFlatSheetTests twin it against the application's rules)
        EXPECT_STREQ(SheetFlat::boxOf(*flat.m_projectBar), "ProjectManager.Box.ProgressBar");
        EXPECT_STREQ(SheetFlat::boxOf(*flat.m_downloadBar), "ProjectManager.Box.ProgressBar");
        EXPECT_STRNE(SheetFlat::boxOf(*flat.m_dialogBar), "ProjectManager.Box.ProgressBar");

        // Legacy: the sheet of the window does not reach it, so its track is the application's grey (ProgressBar.qss:13) beyond the
        // chunk (:23), where the bars below the window have a clear track (ProjectManager.qss:774) and show the backdrop through it
        const SheetFlat::BarCase& part = SheetFlat::BarCases[3];
        SheetFlat::setBar(*legacy.m_dialogBar, part);
        SheetFlat::setBar(*legacy.m_projectBar, part);
        const QImage dialogImage = SheetFlat::paintedBar(*legacy.m_dialogBar, 1.0);
        const QImage windowImage = SheetFlat::paintedBar(*legacy.m_projectBar, 1.0);
        const int width = legacy.m_dialogBar->width();
        const int height = legacy.m_dialogBar->height();
        const int columns = static_cast<int>(width * double(part.m_value - part.m_minimum) / double(part.m_maximum - part.m_minimum));
        ASSERT_GT(columns, 0);
        ASSERT_LT(columns, width);

        EXPECT_EQ(SheetFlat::countOf(dialogImage, SheetFlat::BarChunk), columns * height);
        EXPECT_EQ(SheetFlat::countOf(dialogImage, SheetFlat::BarTrack), (width - columns) * height);
        EXPECT_EQ(SheetFlat::countOf(dialogImage, SheetFlat::ParentColor), 0);

        EXPECT_EQ(SheetFlat::countOf(windowImage, SheetFlat::BarChunk), columns * height);
        EXPECT_EQ(SheetFlat::countOf(windowImage, SheetFlat::BarTrack), 0);
        EXPECT_EQ(SheetFlat::countOf(windowImage, SheetFlat::ParentColor), (width - columns) * height);
    }
} // namespace O3DE::ProjectManager
