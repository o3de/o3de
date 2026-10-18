/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/FrameElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QAbstractScrollArea>
#include <QApplication>
#include <QPainter>
#include <QPalette>
#include <QStyle>
#include <QStyleOption>
#include <QVariant>
#include <QVector>
#include <QWidget>

namespace AzQtComponents
{
    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins this file
    //! with others into one translation unit, where anonymous namespaces merge (TextElement.cpp does the same).
    namespace FrameElementInternal
    {
        // ------------------------------------------------------------------------------
        // Global elements. Each value cites the declaration that grants it, as file:line of the legacy
        // stylesheets in Components/Widgets/. What an element leaves at the defaults of FrameElement is what
        // the '*' rule (BaseStyleSheet.qss:15-21) gave every widget: a transparent background and white text.
        // ------------------------------------------------------------------------------

        const QColor PanelFill(0x44, 0x44, 0x44);       // TOKENIZE: FramePanelFill
        const QColor SeparatorLine(0x99, 0x99, 0x99);   // TOKENIZE: FrameSeparatorLine

        //! Only '*' reaches the frame, so Qt derives its bevel and its outline from a transparent background, and they
        //! are not seen: the frame of a scroll area (no rule gave one a background; Fusion outlines a panel from
        //! Window.darker(140), qfusionstyle.cpp:471-481) and a QFrame with a class tag that no element names.
        const FrameElement Clear{ .m_name = "Global.Frame.Clear" };

        //! `.QFrame[frameShape="1"|"2"|"4"|"5"|"6"] { background-color: #444444 }` (BaseStyleSheet.qss:41-48): the
        //! dividers, boxes and panels of every window, drawn by a QFrame itself (not a subclass: its comment says so).
        //! Captured: HLine and VLine (Sunken, Plain) and StyledPanel (Raised, Plain). UNVERIFIED by capture: Box, Panel
        //! and a WinPanel (which the rule leaves out, so it is transparent); they follow from the same two rules.
        const FrameElement Panel{ .m_name = "Global.Frame.Panel", .m_fill = PanelFill, .m_fillShapes = FrameElement::PanelShapes };

        //! `.separator { color: #999999 }` (Card.qss:259-264): the separator of a Card (Card.cpp:127-129), a QFrame with
        //! the class tag "separator", so `.QFrame[frameShape]` does not reach it and it has no fill of its own.
        const FrameElement Separator{ .m_name = "Global.Frame.Separator", .m_foreground = SeparatorLine };

        // ------------------------------------------------------------------------------
        // Resolution
        // ------------------------------------------------------------------------------

        //! The name of the dynamic property that keeps a widget's element
        constexpr const char* ElementProperty = "_o3de_frameElement";

        struct Row
        {
            const FrameElement* m_element;
            const QMetaObject* m_widgetClass;
            FrameElement::Matcher m_matcher;
        };

        //! The elements modules registered, checked before the Global ones
        QVector<Row>& moduleRows()
        {
            static QVector<Row> rows;
            return rows;
        }

        bool coveredByAModule(const QWidget* widget)
        {
            for (const Row& row : moduleRows())
            {
                if (widget->metaObject()->inherits(row.m_widgetClass))
                {
                    return true;
                }
            }
            return false;
        }

        //! A QFrame itself, as Qt's `.QFrame` selector sees it: `.QFrame` is [class~=QFrame], and the class attribute
        //! of a widget is its class tag if it has one, else its class name (qstylesheetstyle.cpp:1543-1551). A subclass
        //! without Q_OBJECT is a QFrame to Qt, and so is a QFrame made in Python (a dynamic meta object of that name).
        bool isExactFrame(const QWidget* widget)
        {
            const QMetaObject* metaObject = widget->metaObject();
            return metaObject == &QFrame::staticMetaObject || qstrcmp(metaObject->className(), "QFrame") == 0;
        }

        //! The widgets that may have an element: no lookup is spent on the others (labels, buttons, menus, ...).
        //! The Global elements are for a QFrame itself and for a scroll area, whatever it is called.
        bool isCandidate(const QWidget* widget)
        {
            return isExactFrame(widget)
                || qobject_cast<const QAbstractScrollArea*>(widget)
                || coveredByAModule(widget);
        }

        //! The element of a candidate, or null. Modules first, then the Global elements, the most specific first.
        const FrameElement* findElement(const QWidget* widget)
        {
            for (const Row& row : moduleRows())
            {
                if (widget->metaObject()->inherits(row.m_widgetClass) && row.m_matcher(widget))
                {
                    return row.m_element;
                }
            }
            if (isExactFrame(widget))
            {
                // A tag replaces the class name in the class attribute, so a tagged QFrame is not matched by Panel's rule
                if (!widget->property("class").isValid())
                {
                    return &Panel;
                }
                return TextElement::tagged(widget, "separator") ? &Separator : &Clear;
            }
            return qobject_cast<const QAbstractScrollArea*>(widget) ? &Clear : nullptr;
        }
    } // namespace FrameElementInternal

    // ----------------------------------------------------------------------------------
    // Element
    // ----------------------------------------------------------------------------------

    QColor FrameElement::fillFor(int frameShape) const
    {
        return ((m_fillShapes >> frameShape) & 1u) ? m_fill : QColor(0, 0, 0, 0);
    }

    void FrameElement::apply(QStyleOptionFrame* option, QPainter* painter, const QWidget* widget) const
    {
        // TRANSITIONAL, until no widget is drawn through a style sheet. QStyleSheetStyle calls this style for the frame
        // of a widget under an inline sheet, with the option's palette configured from the sheet's own rule: a rule that
        // gave the widget a background, a border or a box owns its fill and its shades (QStyleSheetStyle::polish sets
        // WA_StyledBackground then, qstylesheetstyle.cpp:2925-2949), and a color it declared is in the widget's palette
        // (qstylesheetstyle.cpp:1454-1458). Those stay; what no inline rule gave is the element's.
        const bool throughSheet = widget->style()->inherits("QStyleSheetStyle");

        if (!(throughSheet && widget->testAttribute(Qt::WA_StyledBackground)))
        {
            // The background first: PE_Widget drew it over the whole widget (qstylesheetstyle.cpp:4719-4722), then CE_ShapedFrame
            // the frame (qstylesheetstyle.cpp:4502-4511)
            const QColor fill = fillFor(option->frameShape);
            if (fill.alpha() > 0)
            {
                painter->fillRect(widget->rect(), fill);
            }

            // The palette the base style draws the frame from: Window, Base and Button are the fill
            // (qstylesheetstyle.cpp:1436-1440), the shades derive from it (qstylesheetstyle.cpp:1399-1404).
            // QPalette::setColor sets every color group, as the sheet does.
            QPalette& palette = option->palette;
            palette.setColor(QPalette::Base, fill);
            palette.setColor(QPalette::Button, fill);
            palette.setColor(QPalette::Window, fill);
            palette.setColor(QPalette::Light, fill.lighter(115));
            palette.setColor(QPalette::Midlight, fill.lighter(107));
            palette.setColor(QPalette::Dark, fill.darker(150));
            palette.setColor(QPalette::Shadow, fill.darker(300));
        }

        // The lines of a Plain frame draw in WindowText (qcommonstyle.cpp:2344-2412); the sheet gave it Text and
        // WindowText alike (qstylesheetstyle.cpp:1410-1414), over any palette the widget has from code or a .ui
        const bool sheetColor = throughSheet
            && widget->palette().color(QPalette::Active, QPalette::WindowText) != QApplication::palette(widget).color(QPalette::Active, QPalette::WindowText);
        if (!sheetColor)
        {
            option->palette.setColor(QPalette::WindowText, m_foreground);
            option->palette.setColor(QPalette::Text, m_foreground);
        }
    }

    // ----------------------------------------------------------------------------------
    // Resolution
    // ----------------------------------------------------------------------------------

    void FrameElement::registerElement(const FrameElement* element, const QMetaObject* widgetClass, Matcher matcher)
    {
        FrameElementInternal::moduleRows().append({ element, widgetClass, matcher });
        if (!StyleManager::stylesheetsDisabled())
        {
            return;
        }

        // A module builds some windows before it sets up its styles: resolve the widgets polished before this call
        const QWidgetList widgets = QApplication::allWidgets();
        for (QWidget* widget : widgets)
        {
            if (widget->testAttribute(Qt::WA_WState_Polished) && widget->metaObject()->inherits(widgetClass))
            {
                resolve(widget);
                widget->update();
            }
        }
    }

    const FrameElement* FrameElement::resolve(const QWidget* widget)
    {
        if (!FrameElementInternal::isCandidate(widget))
        {
            return nullptr;
        }
        // Kept on the widget: the draw reads it back without resolving again (a null element is kept too, so a widget
        // a module's matcher rejected is not asked on every paint)
        const FrameElement* element = FrameElementInternal::findElement(widget);
        const_cast<QWidget*>(widget)->setProperty(FrameElementInternal::ElementProperty, QVariant::fromValue(reinterpret_cast<quintptr>(element)));
        return element;
    }

    const FrameElement* FrameElement::forFrame(const QWidget* widget)
    {
        if (!FrameElementInternal::isCandidate(widget))
        {
            return nullptr;
        }
        // A widget never polished (painted into an image, say) is resolved now, and not kept: no property is set while painting
        const QVariant stored = widget->property(FrameElementInternal::ElementProperty);
        return stored.isValid() ? reinterpret_cast<const FrameElement*>(stored.value<quintptr>()) : FrameElementInternal::findElement(widget);
    }
} // namespace AzQtComponents
