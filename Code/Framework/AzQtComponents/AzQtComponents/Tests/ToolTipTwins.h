/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// ----------------------------------------------------------------------------------
// Twins of a tooltip: the tip a widget shows in the legacy GUI (Qt's own style sheet style over Fusion, with the rules of ToolTip.qss
// and the sheets of the widget) and the one it shows in the flattened GUI (the flattened Style, with the tip looks the module
// registered), read the same way. Header only, with no fixture of its own and no application of its own: the modules' tests that
// twin a tip include it (AzQtComponents, AzToolsFramework, AtomToolsFramework, ProjectManager). Everything lives in the namespace
// ToolTipTwins, so that a unity build joining this file with other test files finds no second definition.
//
// A tip is made as Qt makes one: a QEvent::ToolTip (QHelpEvent) is sent to the widget through the application, so the application
// filter of the flattened Style notes the widget the tip is for before the widget shows it (ToolTipElement::noteHelpEvent). The
// flat tip is made under the application's style (Fusion in these tests) and handed the flattened Style then, as the tip of the
// application is polished by it when it is made: the Style has to be an application filter of the test (installEventFilter).
// ----------------------------------------------------------------------------------

#include <QApplication>
#include <QColor>
#include <QFont>
#include <QFontInfo>
#include <QHelpEvent>
#include <QImage>
#include <QLabel>
#include <QMargins>
#include <QPainter>
#include <QPalette>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStyle>
#include <QToolTip>
#include <QWidget>

#include <string>

namespace AzQtComponents
{
    namespace ToolTipTwins
    {
        // ----------------------------------------------------------------------------------
        // The rules of the application's sheet that reach a tip
        // ----------------------------------------------------------------------------------

        //! The universal rule (BaseStyleSheet.qss:15-21) and the tooltip rule (ToolTip.qss:10-20): the sheet a legacy twin has on its
        //! root, above the sheets of the widget a tip is shown for. A tip is named "QToolTip" to the sheet style, so no other rule of
        //! the application's sheet (a `QLabel` or `QFrame` rule) matches one. ToolTipElementTests compares the text with the files.
        inline constexpr const char* AppRules = R"sheet(
            * { background-color: transparent; color: white; font-family: "Open Sans"; font-weight: 400; }
            QToolTip { color: #ffffff; background-color: black; border: 1px transparent black; margin-left: 2px; font-size: 12px;
                margin-top: 4px; margin-bottom: 4px; font-family: "Open Sans"; }
        )sheet";

        // ----------------------------------------------------------------------------------
        // The process state a tip needs
        // ----------------------------------------------------------------------------------

        //! Tips shown at once, without an effect widget over them, white on black (ToolTip.qss:12-13). The legacy tip takes its
        //! colours from the sheet, the flat one from the palette the style gives QToolTip when it polishes the application (which a
        //! test's style does not). The effects and the palette of QToolTip are the application's and outlive a test: they are put
        //! back with the guard.
        class Guard
        {
        public:
            Guard()
                : m_fadeTooltip(QApplication::isEffectEnabled(Qt::UI_FadeTooltip))
                , m_animateTooltip(QApplication::isEffectEnabled(Qt::UI_AnimateTooltip))
                , m_tipPalette(QToolTip::palette())
            {
                QApplication::setEffectEnabled(Qt::UI_FadeTooltip, false);
                QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, false);

                QPalette tipPalette;
                tipPalette.setColor(QPalette::ToolTipBase, Qt::black);
                tipPalette.setColor(QPalette::ToolTipText, Qt::white);
                QToolTip::setPalette(tipPalette);
            }

            ~Guard()
            {
                QApplication::setEffectEnabled(Qt::UI_FadeTooltip, m_fadeTooltip);
                QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, m_animateTooltip);
                QToolTip::setPalette(m_tipPalette);
            }

            Guard(const Guard&) = delete;
            Guard& operator=(const Guard&) = delete;

        private:
            bool m_fadeTooltip;
            bool m_animateTooltip;
            QPalette m_tipPalette;
        };

        //! The flattened Style as an application filter for the length of a test, as Style::polish(QApplication*) installs it in the
        //! application: its filter notes the widget of each help event (ToolTipElement). Declare it after the style it filters for.
        class StyleFilter
        {
        public:
            explicit StyleFilter(QObject* style)
                : m_style(style)
            {
                QApplication::instance()->installEventFilter(m_style);
            }

            ~StyleFilter()
            {
                QApplication::instance()->removeEventFilter(m_style);
            }

            StyleFilter(const StyleFilter&) = delete;
            StyleFilter& operator=(const StyleFilter&) = delete;

        private:
            QObject* m_style;
        };

        // ----------------------------------------------------------------------------------
        // Reading a tip
        // ----------------------------------------------------------------------------------

        //! What a tip is: its contents box, the margin of its label, the size it asks for, its font and colours, and its pixels at an
        //! explicit size
        struct TipFacts
        {
            bool m_found = false;
            QMargins m_contents;
            int m_margin = 0;
            QSize m_hint;
            QRect m_contentsRect;
            QFont m_font;
            QColor m_base;              //!< the tip's ToolTipBase (the fill PE_PanelTipLabel draws)
            QColor m_text;              //!< the tip's ToolTipText
            QImage m_images[3];         //!< the tip at TipSize, painted onto images of the device pixel ratios of Ratios
        };

        inline const QSize TipSize(120, 60);

        //! The device pixel ratios the GUI runs at
        inline constexpr qreal Ratios[3] = { 1.0, 1.5, 2.0 };

        //! The tip on screen, if there is one: of several tips the visible one
        inline QLabel* findTip()
        {
            QLabel* tip = nullptr;
            const QWidgetList windows = QApplication::topLevelWidgets();
            for (QWidget* window : windows)
            {
                if (window->inherits("QTipLabel") && (!tip || window->isVisible()))
                {
                    tip = qobject_cast<QLabel*>(window);
                }
            }
            return tip;
        }

        inline std::string marginsText(const QMargins& margins)
        {
            return QStringLiteral("(%1, %2, %3, %4)")
                .arg(margins.left()).arg(margins.top()).arg(margins.right()).arg(margins.bottom()).toStdString();
        }

        //! The font a tip draws with, as the font database resolved it
        inline std::string fontText(const QFont& font)
        {
            const QFontInfo info(font);
            return "family " + info.family().toStdString() + ", " + std::to_string(info.pixelSize()) + "px, weight "
                + std::to_string(info.weight()) + (info.italic() ? ", italic" : "") + (info.underline() ? ", underline" : "");
        }

        //! The pixels of two images that differ
        inline int differingPixels(const QImage& a, const QImage& b)
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

        //! Shows the tip of a widget (the help event the application sends when the pointer rests on it) and reads it. `style` is
        //! the flat twin's: the tip is made under the application's style and is handed the flattened one then. The tip is made
        //! under the fixture's style, so it gets what its constructor takes from the style at that moment: the margin of its label
        //! (QTipLabel::QTipLabel, 1 + PM_ToolTipLabelFrameWidth). The legacy tip needs no such step: it is made below the widget and
        //! so below its sheet style. `font` is a tip's font to give this one, and `margin` the margin of its label (-1: the style's):
        //! for a twin that proves a box or a fill and not a font, in a process whose application style and fonts are not the
        //! ones the GUI has. The widget has the tooltip text already (QWidget::setToolTip: an ElidingLabel's
        //! setDescription alone stores a string and gives no tip until the label elides again).
        inline TipFacts showAndReadTip(QWidget* widget, QStyle* style, const QFont* font = nullptr, int margin = -1)
        {
            TipFacts facts;
            const QPoint local(2, 2);
            QHelpEvent help(QEvent::ToolTip, local, widget->mapToGlobal(local));
            QApplication::sendEvent(widget, &help);

            QLabel* tip = findTip();
            if (!tip)
            {
                return facts;
            }

            if (style)
            {
                tip->setStyle(style);
                tip->setMargin(margin >= 0 ? margin : 1 + style->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, nullptr, tip));
            }
            if (font)
            {
                tip->setFont(*font);
            }
            tip->resize(TipSize);

            facts.m_found = true;
            facts.m_contents = tip->contentsMargins();
            facts.m_margin = tip->margin();
            facts.m_hint = tip->sizeHint();
            facts.m_contentsRect = tip->contentsRect();
            facts.m_font = tip->font();
            facts.m_base = tip->palette().color(QPalette::Active, QPalette::ToolTipBase);
            facts.m_text = tip->palette().color(QPalette::Active, QPalette::ToolTipText);
            for (int i = 0; i < 3; ++i)
            {
                // The tip as the window paints it (its background, then its paint event), onto an image of that ratio: the style scales
                // what it draws as a screen of that ratio does
                facts.m_images[i] = QImage((QSizeF(tip->size()) * Ratios[i]).toSize(), QImage::Format_ARGB32_Premultiplied);
                facts.m_images[i].setDevicePixelRatio(Ratios[i]);
                facts.m_images[i].fill(QColor(0x80, 0x80, 0x80));
                QPainter painter(&facts.m_images[i]);
                tip->render(&painter);
                painter.end();
            }

            // ~QTipLabel clears QToolTip's instance, so that the next tip is a new one
            delete tip;
            return facts;
        }
    } // namespace ToolTipTwins
} // namespace AzQtComponents
